#include "configparser.h"
// реализация методов класса ConfigParser
#include <fstream>
using namespace std;

int ConfigParser::Load(string fname)
{
	// чистим структуру данных
	data.clear();
	// пытаемся открыть файл
	FILE *fin;
	fin = fopen(fname.c_str(), "rt");
	// не вышло - так и скажем
	if (fin == NULL)
	{
		throw "Can not open file for reading...";
	}
	isreading = 0;
	string curstr;
	curstr = "";
	// разбиваем файл на строки
	char c = getc(fin);
	while (c != EOF)
	{
		if (c == 13)
		{
			c = getc(fin);
		}
		if (c == 10)
		{
			// и обрабатываем их по мере надобности
			ParseString(curstr);
			curstr = "";
		}
		else curstr += c;
		c = getc(fin);
	}
	// если вдруг чего-то еще осталось - разобрать эту строку
	if (curstr != "")
	{
		ParseString(curstr);
	}
	fclose(fin);
	return 0;
}

// разбирает строку. Нужные параметры сразу запихивает в data
void ConfigParser::ParseString(string &s)
{
	string tmp = "";
	size_t i;
	// обрезаем возможный комментарий
	// считаем комментарием все после символа ';'
	// не заключенного в кавычки
	int isquote = 0;
	for (i = 0; i < s.length(); i++)
	{
		if (s[i] == '\"') isquote = 1 - isquote;
		if ((isquote == 0) && (s[i] == ';'))
		{
			break;
		}
		tmp += s[i];
	}
	s = tmp;
	tmp = "";
	string paramval;
	// если многострочный параметр --- делать особо нечего
	if (isreading)
	{
		TrimSpaces(s);
		curstr += s;
	}
	else 
	{
		// теперь вытаскиваем имя параметра. 
		// Это - все до символа '='
		paramname = "";
		int foundequ = 0;
		int equpos = -1;
		for (i = 0; i < s.length(); i++)
		{
			if (s[i] == '=')
			{
				foundequ = 1;
				equpos = i;
				break;
			}
			paramname += s[i];
		}
		// ну не нашли символ равенства - значит это и не 
		// строка с параметрами вовсе
		if (foundequ == 0)
		{
			return;
		}
		// если перед = стоит +, то мы должны добавить к параметру строку
		needadd = 0;
		if ((equpos > 0) && (s[equpos - 1] == '+'))
		{
			paramname.erase(--paramname.end());
			needadd = 1;
		}
		// Убили лишние пробелы в начале и в конце имени параметра
		TrimSpaces(paramname);
		// Если получилась ботва - сказали об этом
		if (paramname == "")
		{
			throw "Blank option names are not allowed";
		}
		// теперь значение параметра
		paramval = "";
		for (i = equpos + 1; i < s.length(); i++)
		{
			paramval += s[i];
		}
		TrimSpaces(paramval);
		// если пока читаем строку - просто приписать
		// проверили его
		if (paramval == "")
		{
			throw "Blank values are not allowed";
		}
		// проверили на скобочку (если в несколько строк параметр)
		if (paramval[0] == '{')
		{
			// перевбили наш параметр в текущую строку
			isreading = 1;
			curstr = "";
			size_t i;
			for (i = 1; i < paramval.length(); i++)
			{
				curstr += paramval[i];
			}
		}
		else	
		{
			// проверили его на предмет кавычек
			if (paramval[0] == '\"')
			{
				if (paramval[paramval.length() - 1] != '\"')
				{
					throw "Some errors with quoting in line";
				}
				// откинули кавычки
				tmp = paramval;
				paramval = "";
				for (i = 1; i < tmp.length() - 1; i++)
				{
					paramval += tmp[i];
				}
			}
		}
	}
	// если наткнулись на хвост multiline - параметра
	if ((isreading) && (curstr.length() != 0) && (curstr[curstr.length() - 1] == '}') )
	{
		curstr.erase(--curstr.end());
		paramval = curstr;
		isreading = 0;
	}
	if (isreading == 0)
	{
		// если нет такого параметра, его надо создать
		map<string, vector<string> >::iterator p;
		p = data.find(paramname);
		if (p == data.end()) needadd = 0;
		// заполнили то что получилось
		if (needadd == 0)
		{
			// заново
			vector<string> a;
			a.clear();
			a.push_back(paramval);
			data[paramname] = a;
		}
		else
		{
			// добавили
			data[paramname].push_back(paramval);
		}
	}
}

// убивает в строке пробелы в начале и в конце
void ConfigParser::TrimSpaces(string &s)
{
	string tmp = s;
	s = "";
	int wf = 0;
	size_t i;
	for (i = 0; i < tmp.length(); i++)
	{
		if (! IsSpace(tmp[i]) )
		{
			wf = 1;
		}
		if (wf == 1) s += tmp[i];
	}
	while ((s.length() > 0) && (IsSpace(s[s.length() - 1])))
	{
		s.erase(--s.end());
	}
}

// проверяет, является ли c пробельным символом
int ConfigParser::IsSpace(char c)
{
	if (c == ' ') return 1;
	if (c == '\t') return 1;
	return 0;
}

// вытаскивает параметр после загрузки конфига
// швыряется exception-ом если обнаруживает проблемы
string ConfigParser::GetParam(string paramname)
{
	map<string, vector<string> >::iterator p;
	p = data.find(paramname);
	if (p == data.end())
	{
		throw "Could not find parameter " + paramname;
	}
	if (p->second.size() < 1) throw "Could not find parameter " + paramname;
	return p->second[0];
}

// вытаскивает параметр после загрузки конфига
// не швыряется exception-ом если обнаруживает проблемы
string ConfigParser::GetParamSilent(string paramname)
{
	map<string, vector<string> >::iterator p;
	p = data.find(paramname);
	if (p == data.end())
	{
		return "";
	}
	if (p->second.size() < 1) throw "Could not find parameter " + paramname;
	return p->second[0];
}

// вытаскивает многозначный параметр
void ConfigParser::GetMultiValuedParam(string paramname, vector<string> &res)
{
	// получить итератор на значение
	res.clear();
	map<string, vector<string> >::iterator p;
	p = data.find(paramname);
	if (p == data.end())
	{
		throw "Could not find parameter " + paramname;
	}
	// перекачать данные
	size_t i;
	for (i = 0; i < p->second.size(); i++) res.push_back(p->second[i]);
}

// убивает в строке кавычки в начале/конце
void ConfigParser::TrimQuotes(string &s)
{
	if (s.length() < 2) return;
	if ((s[0] == '\"') && (s[s.length() - 1] == '\"'))
	{
		// отпилим кавычки в начале и в конце
		string tmp = s;
		s = "";
		size_t i;
		for (i = 1; i < tmp.length() - 1; i++) s += tmp[i];
	}
}

// разваливает большую строку на куски, разделенные запятыми
// делает тупо, то есть любая запятая после четного количества 
// кавычек, считается разделителем
void ConfigParser::SplitMultiValuedString(string s, vector<string> &res)
{
	res.clear();
	int no;
	size_t i;
	string curs;
	no = 0;
	curs = "";
	for (i = 0; i < s.length(); i++)
	{
		if (s[i] == '\"') no = 1 - no;
		if ((s[i] == ',') && (no == 0))
		{
			TrimSpaces(curs);
			TrimQuotes(curs);
			res.push_back(curs);
			curs = "";
		}
		else curs += s[i];
	}
	// если чего-то осталось, надо это учесть
	if (curs != "")
	{
		TrimSpaces(curs);
		TrimQuotes(curs);
		res.push_back(curs);		
	}
}

