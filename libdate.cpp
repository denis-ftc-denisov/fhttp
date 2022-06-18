#define _XOPEN_SOURCE
#include "libdate.h"
#include "constants.h"

#include <time.h>
#include <locale.h>
#include <cstring>

#include <sstream>
#include <string>
#include <vector>
using namespace std;

// внутренние константы
const int NOTAMONTH = -1;

// преобразует время с начала эпохи в дату в формате RFC (HTTP-Date)
// вернет пустую строку при ошибках
string GetRFCGMTDate(time_t time)
{
	// какое-то странное неправильное время
	if (time == ((time_t)-1))
	{
		// такого быть не может - это ошибка
		return "";
	}
	struct tm *ctime = new struct tm;
	// получаем структуру со временем (используем thread-safe функцию)
	if (gmtime_r(&time, ctime) == NULL)
	{
		return "";
	}
	// формируем  строку с ответом
	ostringstream oss;
	oss << GetWkDayName(ctime->tm_wday) << ", " << IntToLength(ctime->tm_mday, 2) << " " << GetMonthName(ctime->tm_mon) << " " << IntToLength(ctime->tm_year + 1900, 4);
	oss << " " << IntToLength(ctime->tm_hour, 2) << ":" << IntToLength(ctime->tm_min, 2) << ":" << IntToLength(ctime->tm_sec, 2) << " GMT";
	// прибираем память
	delete ctime;
	return oss.str();
}

// преобразует количество секунд с начала эпохи в дату
// в формате файлов журнала Apache 
// вернет пустую строку при ошибках
string GetApacheLogDate(time_t time)
{
	// какое-то странное неправильное время
	if (time == ((time_t)-1))
	{
		// такого быть не может - это ошибка
		return "";
	}
	struct tm *ctime = new struct tm;
	// получаем структуру со временем (используем thread-safe функцию)
	if (localtime_r(&time, ctime) == NULL)
	{
		return "";
	}
	// теперь формируем строку
	ostringstream oss;
	char tmp[100];
	memset(tmp, 0, sizeof(tmp));
	if (strftime(tmp, sizeof(tmp), "%d/%b/%Y:%H:%M:%S %z", ctime) > sizeof(tmp)) return "";
	oss << "[" << tmp << "]";
	delete ctime;
	return oss.str();
}


// возвращает название дня недели
string GetWkDayName(int wday)
{
	switch(wday)
	{
		case 0: return "Sun";
		case 1: return "Mon";
		case 2: return "Tue";
		case 3: return "Wed";
		case 4: return "Thu";
		case 5: return "Fri";
		case 6: return "Sat";
		default: break;
	}
	return "NOT A WEEK DAY!!!";
}

// возвращает название месяца
string GetMonthName(int month)
{
	switch(month)
	{
		case 0: return "Jan";
		case 1: return "Feb";
		case 2: return "Mar";
		case 3: return "Apr";
		case 4: return "May";
		case 5: return "Jun";
		case 6: return "Jul";
		case 7: return "Aug";
		case 8: return "Sep";
		case 9: return "Oct";
		case 10: return "Nov";
		case 11: return "Dec";
		default: break;
	}
	return "NOT A MONTH!!!";
}

// дополняет число a с начала нулями до длины l
// вернет строку, которая получится
string IntToLength(int a, size_t l)
{
	ostringstream oss;
	oss << a;
	string s = oss.str();
	while (s.length() < l)
	{
		s = "0" + s;
	}
	return s;
}

// возвращает время получаемое при разборе даты
// понимать 3 формата согласно RFC 2616
// при ошибках вернет ((time_t)-1) (константа ERRONEUSDATE)
time_t ParseDate(string date)
{
	// пытаемся установить локаль, чтобы можно было  пользоваться функцией strptime
	// для разбора HTTP дат
	if (setlocale(LC_ALL, DEFAULTLOCALE) == NULL)
	{
		return ERRONEUSDATE;
	}
	
	// пытаемся посмотреть на дату в одном из допустимых форматов
	struct tm ourtime;
	if (strptime(date.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &ourtime) == NULL) 
	{
	    if (strptime(date.c_str(), "%A, %d-%b-%y %H:%M:%S GMT", &ourtime) == NULL)
	    {
			if (strptime(date.c_str(), "%a %b %e %H:%M:%S %Y", &ourtime) == NULL)
			{
				// ни один не подошел - ошибка
				return ERRONEUSDATE;
			}
	    }
	}
	// теперь вернем в формате функции time
	// если mktime вернет ошибку, ее результат будет равен константе ERRONEUSDATE
	// что собственно и требуется
	return mktime(&ourtime);
}
