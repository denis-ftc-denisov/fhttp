#ifndef FHTTP_LIBDATE_HEADER_FILE_INCLUDED_IUWERYETDHJXCJGFJGERGJDFJSCNVBMFBAYEFGJDHFGJDFHKSDJFHSKJFH
#define FHTTP_LIBDATE_HEADER_FILE_INCLUDED_IUWERYETDHJXCJGFJGERGJDFJSCNVBMFBAYEFGJDHFGJDFHKSDJFHSKJFH

#include <time.h>

#include <string>
using namespace std;

// преобразует количество секунд с начала эпохи time в дату
// в формате RFC 1123 (HTTP Date)
// вернет пустую строку при ошибках
string GetRFCGMTDate(time_t time);

// преобразуют номер дня недели wday и месяца month в строку 
// в соответствии с RFC 1123
// wday и month означают номер дня недели/месяца, начиная с 0
string GetWkDayName(int wday);
string GetMonthName(int month);

// возвращает строку, являющуюся дополнением числа a 
// нулями с начала до длины l
string IntToLength(int a, size_t l);

// преобразует количество секунд с начала эпохи time в дату
// в формате файлов журнала Apache 
// вернет пустую строку при ошибках
string GetApacheLogDate(time_t time);

// возвращает время получаемое при разборе даты
// понимать 3 формата согласно RFC 2616
// при ошибках вернет ((time_t)-1)
// date - сама дата, записанная в строку
time_t ParseDate(string date);

#endif
