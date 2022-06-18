#ifndef FHTTP_CLIENT_FUNCTION_HEADER_FILE_INCLUDED_QIUERHBHVFJHGYREGDJFHMFJHSGJDGJFGDGFJHGENM
#define FHTTP_CLIENT_FUNCTION_HEADER_FILE_INCLUDED_QIUERHBHVFJHGYREGDJFHMFJHSGJDGJFGDGFJHGENM

#include "configparser.h"
#include "httpparser.h"

#include <sys/types.h>

#include <string>
#include <vector>
using namespace std;

// функция, работающая с клиентом. Должна сама из сокета таскать запросы пока не надоест
// и отвечать на них чем положено.
// clientsock - идетнификатор сокета
// cfg - объект ConfigParser с загруженными параметрами
void ProcessRequest(int clientsock, ConfigParser &cfg);

// читает HTTP-запрос из сокета. Вернет FHTTPFAIL, если прочитать не удалось
// и что-нибудь другое в противном случае.
// clientsock - идетнификатор сокета
// req - строка, в которую помещается прочитанный запрос
int ReadHTTPRequest(int clientsock, string &req);

// реакция на будильник. Закрывает соединение с клиентом и выходит
void OnAlarm(int signum);

// записывает строку в сокет клиенту
// вернет FHTTPOK при удаче, иначе FHTTPFAIL
// clientsock - идетнификатор сокета
// s - строка, которую нужно записать
int WriteStringToSocket(int clientsock, string &s);
// записывает файл в сокет клиенту
// вернет FHTTPOK при удаче, иначе FHTTPFAIL
// clientsock - идетнификатор сокета
// path - путь к файлу, который нужно записать
// ranges - список отрезков байт, которые нужны клиенту
int WriteRangedFileToSocket(int clientsock, string path, vector<pair<off_t, off_t> > &ranges);


#endif
