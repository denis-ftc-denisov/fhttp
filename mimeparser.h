#ifndef FHTTP_MIME_TYPES_TABLE_PARSER_HEADER_FILE_INCLUDED_IUEWHNMHSBVMHWBVFVVWSJFBVSJVFVJHSFIOUWEGHJHWGSSFGQJHFVGWS
#define FHTTP_MIME_TYPES_TABLE_PARSER_HEADER_FILE_INCLUDED_IUEWHNMHSBVMHWBVFVVWSJFBVSJVFVJHSFIOUWEGHJHWGSSFGQJHFVGWS

#include <vector>
#include <string>
#include <map>
using namespace std;

// Mime Parser реализует разбор таблицы mime-типов (в формате /etc/mime.types)

class MIMEParser
{
	// хранит собственно перевод из суффикса файла в content type
	map<string, string> data;
	// добавляет прочитанную строку s
	void AddString(string s);
	// убирает пробелы в начале и в конце строки s
	string TrimSpaces(string s);
public:
	// загрузка таблицы из файла, заданного путем path
	void Load(string path);
	// возвращает content type по суффиксу файла ext
	// вернет */* если ничего не нашлось
	string GetContentType(string ext);
};

#endif
