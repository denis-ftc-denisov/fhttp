#ifndef FHTTP_HTTP_PARSER_HEADER_FILE_INCLUDED_IUETHWUVNBDKSJFJKHKJDHFKJHSDKJFHSDKJFVMSMN
#define FHTTP_HTTP_PARSER_HEADER_FILE_INCLUDED_IUETHWUVNBDKSJFJKHKJDHFKJHSDKJFHSDKJFVMSMN
#include "configparser.h"
#include "mimeparser.h"

#include <map>
#include <string>
using namespace std;

// класс HTTP Parser. Должен разбираться с запросом, которым в нас бросается клиент.
// 
class HTTPParser
{
private:
	// хранит значения полей HTTP-запроса название -> значение
	map <string, string> values;
	// разбивает строку s по символу delim. Результат кладет в список res
	// при этом пустые строки считаются значащими
	void Split(string &s, vector<string> &res, char delim);
	// переводит заведомо неотрицательное число записанное в строке s 
	// в тип off_t
	// будет генерировать exceptions при переполнении
	// вернет -1 при неверном формате строки
	// система счисления предполагается дестяичная
	off_t ConvertToOffsetType(string s);
public:
	// данные, получаемые в результате разбора запроса
	string method;						// метод запроса (GET, HEAD и т.п.)
	string abspath;						// путь к объекту
	string uri;							// URI (см. RFC 2616)
	string querystring;					// строка запроса - часть URI (см. RFC)
	string version;						// версия HTTP. Строка вида "HTTP/1.1"
	int majorversion;					// два числа, обозначающие версию 
	int minorversion;					//
	int parseresult;					// результат разбора (код HTTP-ответа)
	string requestline;					// строка запроса
	vector<pair<off_t, off_t> > ranges;	// интервалы, запрошенные клиентом
	int haverangefield;					// флаг, нужно ли учитывать поле Range (если оно есть)
										// иногда есть поле, но учитываеть его не нужно
	// данные, получаемые в результате формирования ответа
	string response;					// заголовок ответа
	string entity;						// объект, полученный при формировании ответа (если пусто использовать entitypath)
	string entitypath;					// путь к файлу, из которого надо брать ответ
										// если обе эти строки пусты, значит entity тоже пуст
	// разбирает запрос, записанный в строке res
	// заполняет переменные, содержащие данные, получаемые
	// в процессе разбора запроса (см. выше)
	void Load(string req);
	// разбирает строку запроса reqline
	int ParseRequestLine(string reqline);
	// разбирает версию протокола HTTP
	// версия должна быть уже записана в строке version
	int ParseVersion();
	// разбирает URI (должен быть записан в строке uri)
	int ParseURI();
	// формирует ответ на запрос
	// cfg - объект ConfigParser с загруженными параметрами
	// заполняет строки response, entity, entitypath
	// может менять результат разбора parseresult (использует как внутреннюю переменную)
	void MakeResponse(ConfigParser &cfg);
	// возвращает строковую репрезентацию кода http ответа code
	string GetStringRepresent(int code);
	// разбирает поля запроса заданного списком строк заголовка request
	// вернет FHTTPOK если все удачно и FHTTPFAIL если запрос испорчен
	int ParseRequestFields(vector<string> &request);
	// возвращает расширение (extension) файла, заданного путем path. Нужно для определения MIME type
	string GetExtension(string path);
	// возвращает значение поля http-запроса с названием name
	// вернет пустую строку если такого поля не оказалось
	string GetField(string name);
	// декодирует строку s согласно правилам кодирования URI (когда символ заменяется
	// на %(его код в HEX-формате). Вернет результат
	string DecodeString(string s);
	// разбирает поле Range на составляющие. Формирует массив ranges такой, что интервалы
	// в нем не повторяются и содержат обе компоненты. Для работы требует размер файла entitylen.
	// может менять результат разбора.
	void ParseRangeField(off_t entitylen);
	// разбирает bytesrangespec (из описания поля Range)
	// заполняет список ranges и устанавливает флаг haverangefield
	// Также требует размер файла entitylen
	void ParseBytesRangeSpec(off_t entitylen, string &bytesrangespec);
	// делает интервалы в списке ranges непересекающимися
	void MakeDisjointRanges();
	// удаляет из ranges вырожденные интервалы
	void DeleteDegenerateRanges();
};


#endif
