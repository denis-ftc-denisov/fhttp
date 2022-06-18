#include "constants.h"
#include "configparser.h"
#include "clientf.h"

#include <iostream>
#include <fstream>
#include <cstdio>

#include <errno.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

// локальная константа (нужна для вызова setsockopt)
const int SOCKETOPTIONTRUE = 1;

// сокет, на котором мы слушаем соединения
// сделано глобальным, чтобы можно было его закрыть при обработке SIGINT
int listensock;

// количество работающих в настоящиу момент процессов-потомков
// также сделано глобальным, так как используется в обработчиках сигналов
int numchildren;

// обработчик SIGCHLD. Нужен, чтобы не плодить зомби в таблице процессов
void OnSIGCHLD(int signum)
{
    int pid, status, olderr;
  
    // сохраним значение errno 
    olderr = errno;

    // обрабатываем всех завершившихся детей
    // если нам вернут 0, значит обрабатывать больше нечего
    while((pid = waitpid(WAIT_ANY, &status, WNOHANG)) != 0) 
	{
        // на всякий случай - вдруг произойдут ошибки
        if (pid == -1) 
		{
            if (errno != ECHILD) 
			{
                exit(EXIT_FAILURE);
            }
            break;
        }
		// завершился потомок
		numchildren--;
    }
    
    // восстанавливаем старое значение errno
    errno = olderr;
}

// обработчит SIGINT. Должен устроить корректный выход из сервера, закрыв всякие сокеты и т.п.
void OnSIGINT(int signum)
{
	// закроем сокет, слушающий соединения
	if (close(listensock) == -1)
	{
		// если закрыть не удалось - вылетаем с ошибкой
		// писать ничего не будем, ибо при обработке сигнала это делать нехорошо
		exit(EXIT_FAILURE);
	}
	// выйдем из процесса
    exit(EXIT_SUCCESS);
}

int main()
{
	// загружаем параметры
    ConfigParser cfg;
	int listenport;
	// если не хватит критичных параметров, сгенерируется exception
	// после чего он будет перехвачен и программа выйдет с сообщением 
	// об ошибке
	try{
    	cfg.Load(CONFIGFILE);
		listenport = atoi(cfg.GetParam(LISTENPORTPARAMNAME).c_str());
    	cfg.GetParam(DOCUMENTROOTPARAMNAME);
	}
	catch(...)
	{
		cout << "Критическая ошибка при загрузке конфигурационного файла. Продолжение работы невозможно.";
		return EXIT_FAILURE;
	}
    // загружаем ограничение на количество потомков
	string mch = cfg.GetParamSilent(MAXIMUMCHILDRENPARAMNAME);
	int maxchildren = DEFAULTMAXIMUMCHILDREN; // значение по умолчанию
	if (mch != "") maxchildren = atoi(mch.c_str());
	
	struct sigaction sa;
    
    // устанавливаем обработчик SIGCHLD
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = OnSIGCHLD;
    sa.sa_flags = SA_NOCLDSTOP | SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) 
    {
		cerr << "Could not set SIGCHLD handler\n";
        exit(EXIT_FAILURE);
    }
    
    // устанавливаем обработчик SIGINT
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = OnSIGINT;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa, NULL) == -1) 
    {
        cerr << "Could not set SIGINT handler\n";
        exit(EXIT_FAILURE);
    }
	
	// создаем сокет для приема соединений
    if ((listensock = socket(PF_INET, SOCK_STREAM, 0)) == -1) 
	{
        cerr << "Could not create socket for listening...\n";
        exit(EXIT_FAILURE);
    }
	
	// задаем опцию REUSEADDR на сокете. Это нужно для его пересозданияв случае, когда
	// прием соединений временно прекращается
	if (setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, (&SOCKETOPTIONTRUE), sizeof(SOCKETOPTIONTRUE)) == -1)
	{
		cerr << "Could not set socket option...\n";
		exit(EXIT_FAILURE);
	}
	
	struct sockaddr_in listenaddr;
	
    // Заполнить структуру данных с адресом сокета
    memset(&listenaddr, 0, sizeof(listenaddr));
    listenaddr.sin_family      = AF_INET;
    listenaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    listenaddr.sin_port        = htons(listenport);
	
	// привязываем сокет к адресу
    if (bind(listensock, (struct  sockaddr *)(&listenaddr), sizeof(struct sockaddr_in)) == -1) 
	{
        cerr << "Could not bind listening socket...\n";
        exit(EXIT_FAILURE);
    }
	
	// включаем прием входящих соединений. Размер очереди - LISTENBACKLOG
    if(listen(listensock, LISTENBACKLOG) == -1) 
	{
        cerr << "Could not set socket into listening mode...\n";
        exit(EXIT_FAILURE);
    }

	struct sockaddr_in peeraddr;
	socklen_t paddrlen;
	paddrlen = sizeof(peeraddr);
	int recsock;
	// принимаем соединения. Выход будет по CTRL+C (он же SIGINT).
	// при  ошибке в accept - делать нечего, тоже выйдем
    cout << "Для завершения работы нажмите Ctrl-C\n";
	cout << "Соединения принимаются на порт " << listenport << "\n";
    while((recsock = accept(listensock, (struct sockaddr *)(&peeraddr), &paddrlen)) != -1) 
	{
		// напишем чего-нибудь про пришедшее соединение
        printf("принято входящее соединение с адреса %s порта %d\n",
            inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port));
		
		// порождаем процесс, отвечающий за работу с этим клиентом
		int pid;
	    switch(pid = fork()) {

            // потомок
            case 0:

                // сокет для приема входящих соединений в потомке не нужен
                if(close(listensock) == -1) 
				{
                    cerr << "Unable to close listening socket in child process...\n";
                    exit(EXIT_FAILURE);
                }
				// обработаем запрос клиента
				ProcessRequest(recsock, cfg);
		        
                exit(EXIT_SUCCESS);
                // недостижимо
                break;

            // ошибка
            case -1:
            	cerr << "Some error while forking...\n";
                exit(EXIT_FAILURE);
                // тоже недостижимо
                break;

            // родитель
            default:
				break;
        }
        // закрыть сокет поступившего соединения
        // т.к. в родителе он не нужен
        if(close(recsock) == -1) 
		{
            cerr << "Unable to close client socket in parent process...\n";
            exit(EXIT_FAILURE);
        }
		
		// здесь мы закроем сокет и тут же его заново создадим, однако слушать соединения
		// начнем только после того, как количество потомков будет меньше максимума
		// это позволит выдавать Connection refused всем пожключающимя в ненужное время клиентам
		close(listensock);
		// создаем сокет для приема соединений
	    if ((listensock = socket(PF_INET, SOCK_STREAM, 0)) == -1) 
		{
	        cerr << "Could not create socket for listening...\n";
    	    exit(EXIT_FAILURE);
	    }
		// так как сокет пересоздается сразу после закрытия, нужна опция SO_REUSEADDR
		if (setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, (&SOCKETOPTIONTRUE), sizeof(SOCKETOPTIONTRUE)) == -1)
		{
			cerr << "Could not set socket option...\n";
			exit(EXIT_FAILURE);
		}
		// привязываем сокет к адресу
    	if (bind(listensock, (struct  sockaddr *)(&listenaddr), sizeof(struct sockaddr_in)) == -1) 
		{
    	    cerr << "Could not bind listening socket...\n";
			perror("bind");
	        exit(EXIT_FAILURE);
    	}
		
		// порожден потомок
		numchildren++;
        // пока не будет возможности принять соединение - ждем
		while (numchildren >= maxchildren)
		{
			sleep(1);
		}
	
		// а теперь включаем прием входящих соединений. Размер очереди - LISTENBACKLOG
    	if(listen(listensock, LISTENBACKLOG) == -1) 
		{
        	cerr << "Could not set socket into listening mode...\n";
    	    exit(EXIT_FAILURE);
	    }

	}

    // выход из цикла возможен только при ошибке accept()
	// в таком случае закрыть сокет и завершиться с ошибкой
    cerr << "Accept error...\n";
    if(close(listensock) == -1) 
	{
        cerr << "Could not close listening socket...\n";
        exit(EXIT_FAILURE);
    }
    return EXIT_FAILURE;
}
