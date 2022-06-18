#include "mimeparser.h"
#include "constants.h"

#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <cstdio>
using namespace std;

// загружает таблицу с MIME-типами из файла
void MIMEParser::Load(string path)
{
	// пока ничего не прочитали
	data.clear();
	// пытаемся открыть файл
	FILE *fin = fopen(path.c_str(), "rt");
	// файл не открылся? - тогда будем считать, что ничего не загрузилось
	if (fin == NULL)
	{
		return;
	}
	// читаем по одной строки из файла и что-то с ними делаем
	while (1)
	{
		// прочитываем текущую строку
		char c = fgetc(fin);
		string cur = "";
		while (! ((c == 10) || (c == EOF)))
		{
			cur += c;
			c = fgetc(fin);
		}
		// добавляем ее
		AddString(cur);
		// если после текущей строки был EOF, значит файл кончился
		// и надо перестать чего-то там читать
		if (c == EOF) break;
	}
	fclose(fin);
}

// добавляет строку к таблице (для внутреннего использования)
void MIMEParser::AddString(string s)
{
	// заменим все HT на SP (для простоты разбора)
	size_t i;
	for (i = 0; i < s.length(); i++)
	{
		if (s[i] == HT) s[i] = SP;
	}
	// символ # отвечает за комментарии. Все после него считается комментарием и далее
	// не учитывается
	for (i = 0; i < s.length(); i++)
	{
		if (s[i] == '#')
		{
			size_t j;
			for (j = i; j < s.length(); j++)
			{
				s[j] = SP;
			}
			break;
		}
	}
	// лишние пробелы в начале и в конце не нужны
	s = TrimSpaces(s);
	// строка пустая - добавлять нечего
	if (s.length() == 0) return;
	// теперь сожмем все пробелы, идущие подряд
	string tmp = "";
	for (i = 0; i < s.length(); i++)
	{
		if ((i > 0) && (s[i] == SP) && (s[i - 1] == SP)) continue;
		tmp += s[i];
	}
	s = tmp;
	// теперь разобьем строку по пробелам. Первое значение - content-type, остальные - 
	// расширения, которые ему соответствуют
	tmp = "";
	vector<string> ress;
	ress.clear();
	for (i = 0; i < s.length(); i++)
	{
		if (s[i]== SP)
		{
			ress.push_back(tmp);
			tmp = "";
		}
		else tmp += s[i];
	}
	if (tmp != "")
	{
		ress.push_back(tmp);
	}
	// добавим полученные значения в таблицу
	if (ress.size() == 0) return;
	for (i = 1; i < ress.size(); i++)
	{
		if (data.find(ress[i]) == data.end()) 
		{
			data[ress[i]] = ress[0];
		}
	}
}

// убирает пробелы в начале и в конце строки
string MIMEParser::TrimSpaces(string s)
{
	string res = "";
	// флаг, был ли непробельный символ
	int f = 0;
	size_t i;
	for (i = 0; i < s.length(); i++)
	{
		if (s[i] != SP) f = 1;
		if (f == 1) res += s[i];
	}
	// теперь избавимся от пробелов в конце
	while ((res.length() > 0) && (res[res.length() - 1] == SP))
	{
		res.erase(--res.end());
	}
	return res;
}

// возвращает content type по расширению файла
string MIMEParser::GetContentType(string ext)
{
	if (data.find(ext) == data.end()) return "*/*";
	else return data[ext];
}
