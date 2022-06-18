#include "clientf.h"
#include "constants.h"
#include "configparser.h"
#include "httpparser.h"
#include "libdate.h"
#include "debuglog.h"
#include "libfs.h"

#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstdlib>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
using namespace std;

// локальные константы
const int NOTDEFINED = -1;
const int TRUE = 1;
const int FALSE = 0;

// переменная - идентификатор сокета с клиентским соединением
// сделана глобальной, чтобы можно было ее закрыть в
// обработчике SIGALRM
int clientsock;

void ProcessRequest(int clients, ConfigParser &cfg)
{
	// создаем обработчик SIGALRM (нужен при чтении HTTP - запроса)
	if (signal(SIGALRM, &(OnAlarm)) == SIG_ERR)
	{
		// не вышло - выходим с ошибкой
		
		exit(EXIT_FAILURE);
	}
	// сокет должен быть глобальным
	clientsock = clients;
	// весь запрос
	string req;
	// необходимо ли закрыть соединение
	// если было поле Connection
	// то результат берется по его значению
	// иначе - согласно версии
	//  причем при повторных запросах keepalive не меняется
	int keepalive = NOTDEFINED;
	// читаем запросы из сокета пока чего-то читается
	while (ReadHTTPRequest(clientsock, req) != FHTTPFAIL)
	{
		DEBUGLOG("Got http request:\n")
		DEBUGLOG(req);
		// запускаем разборщик запроса
		HTTPParser http;
		try{
			http.Load(req);
		}
		catch(...)
		{
			// если получился exception при разборе - сказать Internal Server Error
			http.parseresult = HTTPINTERNALSERVERERROR;
		}
		
		DEBUGLOG("http request parsed\n")
		DEBUGLOG("results are:\n")
		DEBUGLOG("URI:" + http.uri + "\n")
		DEBUGLOG("abspath: " + http.abspath + "\n")
		DEBUGLOG("version: " + http.version + "\n")
		DEBUGLOG("method: " + http.method + "\n")
		
		// создаем ответ и выдаем его клиенту
		string response = "";
		string entity = "";
		try{
			http.MakeResponse(cfg);
			response = http.response;
			entity = http.entity;
		}
		catch(...)
		{
			// какой-то странный exception. В экстренном порядке генерируем Internal Server Error
			ostringstream oss;
			oss << "HTTP/1.1 500 Internal Server Error" << (char)CR << (char)LF << "\n";
			oss << "Content-Type: */*" << (char)CR << (char)LF;
			oss << "Content-Length: 0" << (char)CR << (char)LF;
			oss << (char)CR << (char)LF;
			response = oss.str();
			entity = "";
		}
		
		DEBUGLOG("Made http response:\n")
		DEBUGLOG("###" + response + "###")
		
		// получим длину объекта
		off_t entitylen;
		if (entity != "") entitylen = entity.length();
		else entitylen = GetFileSize(http.entitypath);
		
		// сформируем запись в файл журнала, если сответствующий параметр задан
		string logfile = cfg.GetParamSilent(LOGFILEPARAMNAME);
		if (logfile != "")
		{
			// здесь будем собирать строку для лога
			ostringstream oss;
			// сформируем IP клиента
			sockaddr_in peeraddr;
			socklen_t sz = sizeof(peeraddr);
			if (getpeername(clientsock, (sockaddr*)&peeraddr, &sz) == 0)
			{
				oss << inet_ntoa(peeraddr.sin_addr);
				oss << " - - "; 							// неиспользуемые нами поля в файле журнала Apache
				oss << GetApacheLogDate(time(NULL)); 		// поле даты в формате Apache по умолчанию 
				oss << " \"" << http.requestline << "\" "; 	// строка запроса
				oss << http.parseresult << " "; 			// код ответа
				if (entitylen == 0) oss << "-"; 		// длина объекта
				else oss << entitylen;
				oss << " \"";
				if (http.GetField("Referer") == "") oss << "-";		// поля Referer и User-Agent
				else oss << http.GetField("Referer");
				oss << "\" \"";
				if (http.GetField("User-Agent") == "") oss << "-";
				else oss << http.GetField("User-Agent");
				oss << "\"";
				// запишем полученную строку в журнал
				{
					ofstream out;
					out.open(logfile.c_str(), ios::out | ios::app);
					if (out.is_open())
					{
						out << oss.str() << "\n";
						out.close();
					}
				}
			}
			// ну если не получилось IP, ничего делать и не будем
		}		
		
		// пишем заголовок ответа в сокет
		if (WriteStringToSocket(clientsock, response) == FHTTPFAIL)
		{
			// ошибка при записи - делать нечего, выходим
			DEBUGLOG("Error during sending of response header\n")
			break;
		}
		// пишем ответ в сокет
		// здесь бывает 2 случая: у нас есть строка с ответом
		// или он берется из файла
		if (entity != "")
		{
			if (WriteStringToSocket(clientsock, entity) == FHTTPFAIL)
			{
				// ошибка при записи - делать нечего, выходим
				DEBUGLOG("Error during sending of response entity\n")
				break;
			}
		}
		else
		{
			if (http.entitypath != "")
			{
				// отправка файла ведется с учетом списка ranges
				if (WriteRangedFileToSocket(clientsock, http.entitypath, http.ranges) == FHTTPFAIL)
				{
					// ошибка при записи - делать нечего, выходим
					DEBUGLOG("Error during sending of response entity\n")
					break;
				}
			}
		}
		DEBUGLOG("HTTP response sent\n")
	
		// если версия протокола 0.9 надо закрыть соединение
		// (в версии 1.0 этого делать не надо)
		if ((http.majorversion == 0) && (http.minorversion == 9))
		{
			DEBUGLOG("Child process exiting due to 0.9 version\n")
			break;
		}
		// анализируем необходимость закрывать соединение
		if (http.GetField("Connection") != "")
		{
			if (http.GetField("Connection") == "keep-alive")
			{
				keepalive = TRUE;
			}
			else
			{
				keepalive = FALSE;
			}
		}
		else if (keepalive == NOTDEFINED)
		{
			if ((http.majorversion == 1) && (http.minorversion == 1))
			{
				keepalive = TRUE;
			}
			else
			{
				keepalive = FALSE;
			}
		}
		// если надо - закрываем
		if (keepalive == FALSE) break;
	}
	// запросы закончились - закрываем сокет и выходим
	if (close(clientsock) == -1)
	{
		DEBUGLOG("Child process was unable to close client socket\n")
		exit(EXIT_FAILURE);
	}
	// нужно обязательно выйти из процесса, иначе 
	// этот процесс из потомка станет родителем, начнет плодить потомков,
	// а потом прибежит злой системный администратор и будет ругаться 
	DEBUGLOG("Child process exited\n")
	exit(EXIT_SUCCESS);
}

// читает запрос из сокета
int ReadHTTPRequest(int clientsock, string &req)
{
	// пока ничего не прочитали
	req = "";
	// заводим будильник. По сигналу SIGALRM закроем соединение и не будем мучаться
	alarm(WAITTIMEOUT);
	// начинаем таскать из сокета данные по одному символу
	char c;
	while (recv(clientsock, &c, 1, 0) == 1)
	{
		// снимаем будильник
		alarm(0);
		// приписали полученный символ к запросу
		req += c;
		// если у нас последовательность CRLF перед запросом (или несколько таких)
		// ее надо проигнорировать (RFC 2616)
		if ((req.length() == 2) && (req[0] == CR) && (req[1] == LF))
		{
			req = "";
		}
		// если запрос закончился (два CRLF-а после чего-то разумного)
		// то вернемся с успешным завершением
		if ( (req.length() >= 4) &&
			 (req[req.length() - 4] == CR) &&
			 (req[req.length() - 3] == LF) &&
			 (req[req.length() - 2] == CR) &&
			 (req[req.length() - 1] == LF) )
		{
			// снимаем будильник
			alarm(0);
			return FHTTPOK;
		}
		// снова заводим будильник
		alarm(WAITTIMEOUT);
	}
	// снимаем будильник (так, на всякий случай)
	alarm(0);
	// проверим, если вдруг прочитался запрос для HTTP 0.9 надо его как-нибудь обработать
	if (req != "")
	{
		// считаем, что потом разберутся, что с ним делать
		return FHTTPOK;
	}
	// если толком не удалось прочитать запрос 
	// (ошибка в recv или что-то такое) - вернуть ошибку
	return FHTTPFAIL;
}

void OnAlarm(int signum)
{
	DEBUGLOG("Child process exits on alarm\n")	
	// прилетел будильник - значит нужно закрыть соединение с клиентом
	if (close(clientsock) == -1)
	{
		// не закрылось соединение - выходим с ошибкой
		exit(EXIT_FAILURE);
	}
	// все нормально, клиент больше ничего не шлет, можно не мучаться
	exit(EXIT_SUCCESS);
}

// записывает строку в сокет
// вернет FHTTPOK или FHTTPFAIL
int WriteStringToSocket(int clientsock, string &s)
{
	// пишем по одному символу
	size_t i;
	for (i = 0; i < s.length(); i++)
	{
		char tmp = s[i];
		// заводим будильник (на случай зависания функции send)
		alarm(WAITTIMEOUT);
		if (send(clientsock, &tmp, 1, 0) != 1)
		{
			// случилась ошибка
			return FHTTPFAIL;
		}
		// снимаем будильник
		alarm(0);
	}
	// все хорошо отправилось
	return FHTTPOK;
}

// записывает файл в сокет с учетом отрезков (предполагается, что они отсортированы по первой 
// координате и не пересекаются)
// вернет FHTTPOK или FHTTPFAIL
int WriteRangedFileToSocket(int clientsock, string path, vector<pair<off_t, off_t> > &ranges)
{
	{
		DEBUGLOG("Sending ranged file to socket\n")
		size_t i;
		DEBUGLOG("Got ranges:\n")
		for (i = 0; i < ranges.size(); i++)
		{
			DEBUGLOG(ranges[i].first)
			DEBUGLOG(" ")
			DEBUGLOG(ranges[i].second)
			DEBUGLOG("\n")
		}
	}
	// получим информацию о файле
	struct stat fileinfo;
	if (stat(path.c_str(), &fileinfo) == -1)
	{
		// не вышло 
		return FHTTPFAIL;
	}
	// откроем файл
	FILE *fin = fopen(path.c_str(), "rb");
	// количество отправленных байт
	off_t sent = 0;
	// количество прочитанных байт
	off_t read = 0;
	// буфер для чтения из файла/отправки в сокет
	unsigned char *buf = new unsigned char[BLOCKSIZE];
	// позиция байта в буфере перед которой заканчиваются неотправленные данные
	unsigned int bufpos = 0;
	// считаем размер того, что нужно отправить
	off_t needsend = 0;
	size_t i;
	for (i = 0; i < ranges.size(); i++)
	{
		needsend += ranges[i].second - ranges[i].first + 1;
	}
	// текущий отрезок
	size_t currange = 0;
	// отправка
	while (sent < needsend)
	{
		// заполняем буфер
		while ((bufpos < BLOCKSIZE) && (read < fileinfo.st_size))
		{
			buf[bufpos] = fgetc(fin);
			bufpos++;
			// сейчас read - номер прочитанного байта в файле
			while ((currange < ranges.size()) && (read > ranges[currange].second)) currange++;
			// если уже все отрезки отправлены, делать нечего
			if (currange >= ranges.size()) break;
			// если байт не попадет в текущий отрезок - не учитываем его
			if ((read < ranges[currange].first) || (read > ranges[currange].second))
			{
				bufpos--;
			}
			read++;
		}
		// если уже все отрезки и текущий буфер отправлены, делать нечего
		if ((bufpos == 0) && (currange >= ranges.size())) break;	
		// заводим будильник (на случай зависания функции send)
		alarm(WAITTIMEOUT);
		if (send(clientsock, buf, bufpos, 0) != (ssize_t)bufpos)
		{
			// случилась ошибка
			// не забыть почистить память
			delete []buf;
			return FHTTPFAIL;
		}
		// снимаем будильник
		alarm(0);
		// весь буфер отправлен - опустошаем его
		sent += bufpos;
		bufpos = 0;
	}
	delete []buf;
	return FHTTPOK;
}

