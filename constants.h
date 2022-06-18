#ifndef FHTTP_CONSTANTS_HEADER_FILE_INCLUDED_JKHWKJFHKJDHFKSHKHKJHKJHKJHKJHKSFNBDSFBMSN
#define FHTTP_CONSTANTS_HEADER_FILE_INCLUDED_JKHWKJFHKJDHFKSHKHKJHKJHKJHKJHKSFNBDSFBMSN

#include <ctime>
using namespace std;

// размер блока при отправке данных из файла (в байтах)
const unsigned int BLOCKSIZE = 65536;

// имя конфигурационного файла
const char CONFIGFILE[] = "fhttp.cfg";

// названия параметров в конфигурационном файле
const char LISTENPORTPARAMNAME[] = "ListenPort";
const char DOCUMENTROOTPARAMNAME[] = "DocumentRoot";
const char DEFAULTDOCUMENTPARAMNAME[] = "DefaultDocument";
const char MIMETABLEPARAMNAME[] = "MimeTable";
const char LOGFILEPARAMNAME[] = "LogFile";
const char MAXIMUMCHILDRENPARAMNAME[] = "MaximumChildren";

// максимальное количество потомков по умолчанию 
const int DEFAULTMAXIMUMCHILDREN = 1;

// стандартный (для этой программы) код успешного завершения и ошибки
const int FHTTPFAIL = -1;
const int FHTTPOK = 0;

// размер очереди приема
const int LISTENBACKLOG = 10;

// сколько времени ждем данных от клиента
const int WAITTIMEOUT = 30; // (в секундах)

// коды состояния HTTP
const int HTTPOK = 200;
const int HTTPPARTIALCONTENT = 206;
const int HTTPNOTMODIFIED = 304;
const int HTTPBADREQUEST = 400;
const int HTTPFORBIDDEN = 403;
const int HTTPNOTFOUND = 404;
const int HTTPREQUESTURITOOLONG = 414;
const int HTTPREQUESTEDRANGENOTSATISFIABLE = 416;
const int HTTPINTERNALSERVERERROR = 500;
const int HTTPNOTIMPLEMENTED = 501;
const int HTTPVERSIONNOTSUPPORTED = 505;

// группы кодов состояния HTTP
const int HTTPCLIENTERRORCODE = 4;
const int HTTPSERVERERRORCODE = 5;

// специальные символы
const unsigned char HT = 9;
const unsigned char SP = 32;
// перевод строки
const int CR = 13;
const int LF = 10;

// Reason phrases для разных кодов ошибки.
const char REASONCONTINUE[] = "Continue";
const char REASONSWITCHINGPROTOCOLS[] = "Switching Protocols";
const char REASONOK[] = "OK";
const char REASONCREATED[] = "Created";
const char REASONACCEPTED[] = "Accepted";
const char REASONNONAUTHORITATIVEINFORMATION[] = "Non-Authoritative Information";
const char REASONNOCONTENT[] = "No Content";
const char REASONRESETCONTENT[] = "Reset Content";
const char REASONPARTIALCONTENT[] = "Partial Content";
const char REASONMULTIPLECHOICES[] = "Multiple Choices";
const char REASONMOVEDPERMANENTLY[] = "Moved Permanently";
const char REASONFOUND[] = "Found";
const char REASONSEEOTHER[] = "See Other";
const char REASONNOTMODIFIED[] = "Not Modified";
const char REASONUSEPROXY[] = "Use proxy";
const char REASONTEMPORARYREDIRECT[] = "Temporary Redirect";
const char REASONBADREQUEST[] = "Bad Request";
const char REASONUNAUTHORIZED[] = "Unauthorized";
const char REASONPAYMENTREQUIRED[] = "Payment Required";
const char REASONFORBIDDEN[] = "Forbidden";
const char REASONNOTFOUND[] = "Not Found";
const char REASONMETHODNOTALLOWED[] = "Method Not Allowed";
const char REASONNOTACCEPTABLE[] = "Not Acceptable";
const char REASONPROXYAUTHENTIFICATIONREQUIRED[] = "Proxy Authentication Required";
const char REASONREQUESTTIMEOUT[] = "Request Time-out";
const char REASONCONFLICT[] = "Conflict";
const char REASONGONE[] = "Gone";
const char REASONLENGTHREQUIRED[] = "Length Required";
const char REASONPRECONDITIONFAILED[] = "Precondition Failed";
const char REASONREQUESTENTITYTOOLARGE[] = "Request Entity Too Large";
const char REASONREQUESTURITOOLARGE[] = "Request-URI Too Large";
const char REASONUNSUPPORTEDMEDIATYPE[] = "Unsupported Media Type";
const char REASONREQUESTEDRANGENOTSATISFIABLE[] = "Requested range not satisfiable";
const char REASONEXPECTATIONFAILED[] = "Expectation Failed";
const char REASONINTERNALSERVERERROR[] = "Internal Server Error";
const char REASONNOTIMPLEMENTED[] = "Not Implemented";
const char REASONBADGATEWAY[] = "Bad Gateway";
const char REASONSERVICEUNAVAILABLE[] = "Service Unavailable";
const char REASONGATEWAYTIMEOUT[] = "Gateway Time-out";
const char REASONHTTPVERSIONNOTSUPPORTED[] = "HTTP Version not supported";

// Размер временного буфера, используемого при преобразованиях времени
const int TIMEBUFFERSIZE = 100;

// идентификация сервера
const char SERVERNAME[] = "fhttp";
const char SERVERVERSION[] = "ver 0.1";

// ошибка при разборе даты
const int ERRONEUSDATE = ((time_t)-1);

// стандартная локаль
const char DEFAULTLOCALE[] = "C";

#endif
