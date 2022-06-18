#ifndef FHTTP_DEBUG_LOGGING_HEADER_FILE_INCLUDED_IUEGFWEGHKJDHGKJHKJSDHDKJGHKJDSHGKJHDKJHGKJSDHKJHDFG
#define FHTTP_DEBUG_LOGGING_HEADER_FILE_INCLUDED_IUEGFWEGHKJDHGKJHKJSDHDKJGHKJDSHGKJHDKJHGKJSDHKJHDFG

// имя файла для отладочного вывода
const char DEBUGLOGFILENAME[] = "debuglog.txt";

// макрос отладочного вывода
#ifdef FTC_DEBUG
#define DEBUGLOG(a) {ofstream out; out.open(DEBUGLOGFILENAME, ios::out | ios::app); if (out.is_open()) { out << (a); } out.close(); }
#else
#define DEBUGLOG(a) 
#endif

#endif
