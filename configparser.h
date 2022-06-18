#ifndef FELIX_THE_CAT_CONFIGURATION_FILE_PARSER_HEADER_FILE_INCLUDED_FJHSSFKJSLKFLKSJFLKJWLJR
#define FELIX_THE_CAT_CONFIGURATION_FILE_PARSER_HEADER_FILE_INCLUDED_FJHSSFKJSLKFLKSJFLKJWLJR
#include <vector>
#include <string>
#include <map>
using namespace std;


// небольшой класс, предназначенный для разбора cfg-файлов
// комментарии вида ; up to the end of line
// понимает параметр, записанный в фигурных скобках
// на нескольких строках
// еще умеет в один параметр пихать много значений
// может бросать всякие exceptions типа char*
// с сообщениями об ошибках
class ConfigParser
{
private:
	// собственно параметры
	map<string, vector<string> > data;
	// может, мы читаем какую-то строку?
	int isreading;
	// вот эту!!!
	string curstr;
	// нужно добавлять значение или заново писать?
	int needadd;
	// имя параметра, который мы читаем
	string paramname;
	// производит разбор очередной строки s из входного файла
	void ParseString(string &s);
	// убирает пробелы в начале и в конце строки s
	void TrimSpaces(string &s);
	// убирает кавычки в начале и в конце строки s
	void TrimQuotes(string &s);
	// проверяет, является ли символ c пробельным
	int IsSpace(char c);
public:
	// загрузить файл 0 если получилось 1 если нет
	// fname - путь и имя загружаемого файла
	int Load(string fname);
	// получает значение параметра paramname
	// генерирует exception типа char* при его отсутствии
	// если параметр многозначен возвращает первое значение
	string GetParam(string paramname);
	// возвращает список значений многозначного параметра paramname
	// в список res. Генерирует исключение при его отсутствии
	void GetMultiValuedParam(string paramname, vector<string> &res);
	// разбивает строку s на опции параметра (по запятой)
	// результат - в списке res
	void SplitMultiValuedString(string s, vector<string> &res);
	// получает значение параметра paramname. Возвращает пустую строку
	// в случае его отсутствия
	string GetParamSilent(string paramname);
};

#endif
