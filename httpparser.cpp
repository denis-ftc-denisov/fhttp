#include "httpparser.h"
#include "configparser.h"
#include "constants.h"
#include "libdate.h"
#include "debuglog.h"
#include "libfs.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

// реализация методов класса HTTPParser

void HTTPParser::Load(string req)
{
	// разбиваем запрос на строки
	// пустые строки пропускаем как ненужные
	vector<string> strs;
	size_t i;
	string curstr = "";
	for (i = 0; i < req.length(); i++)
	{
		if (req[i] == CR) continue;
		if (req[i] == LF)
		{
			if (curstr != "") strs.push_back(curstr);
			curstr = "";
		}
		else
		{
			curstr += req[i];
		}
	}
	
	DEBUGLOG("Request split into lines:\n")
	for (i = 0; i < strs.size(); i++)
	{
		DEBUGLOG("|" + strs[i] + "|\n")
	}
	DEBUGLOG("\n")
	
	if (strs.size() == 0)
	{
		// нет строк или все строки пустые - плохой запрос
		parseresult = HTTPBADREQUEST;
		return;
	}
	// теперь анализируем строку запроса
	if (ParseRequestLine(strs[0]) == FHTTPFAIL)
	{
		// требуется закончить разбор
		return;
	}
	// разбираем поля запроса. первую строку прибьем, так как это request line
	for (i = 0; i < strs.size() - 1; i++)
	{
		strs[i] = strs[i + 1];
	}
	strs.pop_back();
	if (ParseRequestFields(strs) == FHTTPFAIL)
	{
		// заканчиваем разбор
		return;
	}
	parseresult = HTTPOK;
}

// разбирает строку запроса
int HTTPParser::ParseRequestLine(string reqline)
{
	DEBUGLOG("parsing request line |" + reqline + "|\n")
	// сохраним строку запроса на случай, а вдруг кому понадобится
	requestline = reqline;
	// согласно RFC, метод, URI и версия разделены пробелами
	// либо табуляциями, причем их может быть несколько
	// теперь в строке запроса заменим табуляции на пробелы
	size_t i;
	for (i = 0; i < reqline.length(); i++)
	{
		if (reqline[i] == HT)
		{
			reqline[i] = SP;
		}
	}
	// теперь прибить подряд идущие пробелы
	string tmp = "";
	for (i = 0; i < reqline.length(); i++)
	{
		if (reqline[i] == SP)
		{
			if ((i > 0) && (reqline[i - 1] == SP))
			{
				continue;
			}
		}
		tmp += reqline[i];
	}
	reqline = tmp;
	// теперь разбиваем по пробелам строку на версию, метод и URI
	// если пробелов оказалось больше ожидаемого, не будем обрабатывать такой запрос
	string cur = "";
	vector<string> tokens;
	for (i = 0; i < reqline.length(); i++)
	{
		if (reqline[i] == SP)
		{
			tokens.push_back(cur);
			cur = "";
		}
		else cur += reqline[i];
	}
	if (cur != "") tokens.push_back(cur);
	// если строка состоит неправильного количества токенов
	// (а именно метод, URI и версия) то запрос плохой
	// в случае запроса HTTP 0.9 количество токенов должно быть 2 (нет версии)
	if ((tokens.size() != 3) && (tokens.size() != 2))
	{
		parseresult = HTTPBADREQUEST;
		return FHTTPFAIL;
	}
	// заполняем нужные поля
	method = tokens[0];
	uri = tokens[1];
	// разбор версии. Если она указана разбираем, иначе предполагаем 0.9
	if (tokens.size() == 3)
	{
		version = tokens[2];
		// разберем версию
		if (ParseVersion() == FHTTPFAIL)
		{
			return FHTTPFAIL;
		}
	}
	else
	{
		// предполагается версия 0.9
		majorversion = 0;
		minorversion = 9;
		version="HTTP/0.9";
	}
	DEBUGLOG("request line parsed\n")
	DEBUGLOG("method=" + method + " uri=" + uri + " version=" + version + "\n")
	// и URI
	if (ParseURI() == FHTTPFAIL)
	{
		return FHTTPFAIL;
	}
	// все разобралось - совсем здорово
	return FHTTPOK;
}

// разбирает версию протокола HTTP
int HTTPParser::ParseVersion()
{
	istringstream iss(version);
	// прочитаем первые символы версии (должны быть "HTTP/")
	string http = "";
	http += iss.get();
	http += iss.get();
	http += iss.get();
	http += iss.get();
	http += iss.get();
	if (http != "HTTP/")
	{
		parseresult = HTTPBADREQUEST;
		return FHTTPFAIL;
	}
	// читаем версии и точку между ними
	char dot;
	iss >> majorversion >> dot >> minorversion;
	if (dot != '.')
	{
		parseresult = HTTPBADREQUEST;
		return FHTTPFAIL;
	}
	// проверяем, поддерживается ли нами эта версия HTTP
	// (поддерживаются 1.0 и 1.1)
	if (! ( ((majorversion == 1) && (minorversion == 0)) ||
			((majorversion == 1) && (minorversion == 1)) ) )
	{
		parseresult = HTTPVERSIONNOTSUPPORTED;
		return FHTTPFAIL;
	}
	// все!
	return FHTTPOK;
}

// возвращает числовое значение hex-цифры
int GetDigit(char c)
{
	if ((c >= '0') && (c <= '9')) return (c - '0');
	if ((c >= 'A') && (c <= 'Z')) return (c - 'A' + 10);
	if ((c >= 'a') && (c <= 'z')) return (c - 'a' + 10);
	return -1;
}

// разбирает URI
int HTTPParser::ParseURI()
{
	DEBUGLOG("parsing uri\n")	
	// текущее состояние uri при разборе (возможно какие-то части уже отсоединены)
	string cururi = uri;
	// найдем в URI строку запроса (если она там есть) и отсоединим ее
	size_t i;
	querystring = "";
	for (i = 0; i < cururi.length(); i++)
	{
		// строка запроса отделяется символом '?'
		if (cururi[i] == '?')
		{
			// скопировать строку запроса
			size_t j;
			for (j = i + 1; j < cururi.length(); j++)
			{
				querystring += cururi[j];
			}
			// и стереть ее из текущего состояния URI
			cururi.erase(i, cururi.length());
			// выйти из цикла
			break;
		}
	}
	// надйдем первый "/" перед которым не "/" и после которого не "/"
	// начиная с этого места и будет путь
	abspath = "";
	for (i = 0; i < cururi.length(); i++)
	{
		if (cururi[i] == '/')
		{
			if ( ((i == 0) || (cururi[i - 1] != '/')) && ((i == cururi.length() - 1) || (cururi[i + 1] != '/')) )
			{
				// собираем путь
				size_t j;
				for (j = i; j < cururi.length(); j++)
				{
					abspath += cururi[j];
				}
				// проверим, что в пути нет ".." - скажем bad request
				for (j = 0; j < abspath.length() - 1; j++)
				{
					if ((abspath[j] == '.') && (abspath[j + 1] == '.'))
					{
						parseresult = HTTPBADREQUEST;
						return FHTTPFAIL;
					}
				}
				// декодируем возможно кодированный abspath
				abspath = DecodeString(abspath);
				DEBUGLOG("uri parsed abspath=" + abspath + "\n")
				return FHTTPOK;
			}
		}
	}
	// если такой не нашли, значит что-то плохое с запросом
	parseresult = HTTPBADREQUEST;
	return FHTTPFAIL;
}

// формирует ответ на запрос
void HTTPParser::MakeResponse(ConfigParser &cfg)
{
	// если мы не понимаем этот метод, так и скажем
	if ((method != "GET") && (method != "HEAD"))
	{
		parseresult = HTTPNOTIMPLEMENTED;
	}
	// если версия 0.9, а метод не GET, то такого запроса не бывает
	if ((majorversion == 0) && (minorversion == 9) 
		&& (method != "GET"))
	{
		parseresult = HTTPBADREQUEST;
	}
	// посмотрим, бывает ли такой файл, какой у нас спросили
	string allpath = cfg.GetParam(DOCUMENTROOTPARAMNAME) + abspath;
	struct stat fileinfo;
	if (stat(allpath.c_str(), &fileinfo) == -1)
	{
		// случилась какая-то непонятная вещь и про файл ничего узнать нельзя
		// в зависимости от ошибок ставим результат разбора
		switch(errno)
		{
			case EACCES:
				parseresult = HTTPFORBIDDEN;
				break;
			case ENOENT:
				parseresult = HTTPNOTFOUND;
				break;
			case ENAMETOOLONG:
				parseresult = HTTPREQUESTURITOOLONG;
				break;
			default:
				parseresult = HTTPINTERNALSERVERERROR;
				break;
		}
	}
	// если это не файл, а каталог, тогда надо сделать страшную вещь - узнать, какой у нас по умолчанию
	// документ при запросе каталога и поискать его
	if (S_ISDIR(fileinfo.st_mode))
	{
		string defdoc = cfg.GetParamSilent(DEFAULTDOCUMENTPARAMNAME);
		// если не умеем документ по умолчанию - скажем, что так нельзя (Apache так делает)
		if (defdoc == "")
		{
			parseresult = HTTPFORBIDDEN;
		}
		else
		{
			allpath = allpath + "/" + defdoc;
			if (stat(allpath.c_str(), &fileinfo) == -1)
			{
				// случилась какая-то непонятная вещь и про файл ничего узнать нельзя
				// в зависимости от ошибок ставим результат разбора
				switch(errno)
				{
					case EACCES:
						parseresult = HTTPFORBIDDEN;
						break;
					case ENOENT:
						parseresult = HTTPNOTFOUND;
						break;
					case ENAMETOOLONG:
						parseresult = HTTPREQUESTURITOOLONG;
						break;
					default:
						parseresult = HTTPINTERNALSERVERERROR;
						break;
				}
			}
		}
	}
	// разберем поле Range (если его нет все тоже корректно разберется)
	ParseRangeField(fileinfo.st_size);
	// если есть поле Range и оно учитывается, скажем, что код ответа 206
	// разбор поля Range сделан раньше, чем условного GET, так как результат
	// условного GET приоритетнее
	if (haverangefield)
	{
		parseresult = HTTPPARTIALCONTENT;
		// если в результате приведения к нормальному виду интервалов не осталось
		// надо выдать код Requested range not satisfiable (RFC 2616)
		if (ranges.size() == 0)
		{
			parseresult = HTTPREQUESTEDRANGENOTSATISFIABLE;
		}
	}	
	// размер ответа считается теперь не просто по полю fileinfo.st_size
	// а по сумме длин интервалов в списке ranges
	off_t entitylen = 0;
	size_t i;
	for (i = 0; i < ranges.size(); i++)
	{
		entitylen += ranges[i].second - ranges[i].first + 1;
	}
	// сформируем дату (поле Date ответа)
	string date = GetRFCGMTDate(time(NULL));
	// если произошла ошибка
	if (date == "")
	{
		// значит у нас ничего не получится
		parseresult = HTTPINTERNALSERVERERROR;
	}
	// сформируем поле Last Modified
	string lastmodified = GetRFCGMTDate(fileinfo.st_mtime);
	// если произошла ошибка
	if (lastmodified == "")
	{
		// значит у нас ничего не получится
		parseresult = HTTPINTERNALSERVERERROR;
	}
	// сформируем поле Content Type
	MIMEParser mp;
	mp.Load(cfg.GetParamSilent(MIMETABLEPARAMNAME));
	string contenttype = mp.GetContentType(GetExtension(allpath));
	// теперь посмотрим, а вдруг GET условный (когда есть поле If-Modified-Since)
	string lastmoddate = GetField("If-Modified-Since");
	if (lastmoddate != "")
	{
		time_t lastmodtime = ParseDate(lastmoddate);
		// если дата неверна или больше чем текущее время
		// считаем ее invalid и работаем как в случае нормального GET (согласно RFC)
		if (! ((lastmodtime == ERRONEUSDATE) || (lastmodtime > time(NULL))) )
		{
			// если дата модификации файла меньше либо  такая же как нам сказали,
			// говорим not modified
			if (fileinfo.st_mtime <= lastmodtime)
			{
				parseresult = HTTPNOTMODIFIED;
			}
		}
	}
	// если в результате разбора получился какой-то страшный код
	// то выдадим ответ с этим кодом
	if ((parseresult != HTTPOK) && (parseresult != HTTPPARTIALCONTENT))
	{
		// если версия не 0.9 формируем правильный ответ
		if (!((majorversion == 0) && (minorversion == 9)))
		{
			// сформируем министраничку с описанием того, что случилось в случае кодов начинающихся на 4 или 5
			// иначе ничего формировать не будем - пустой ответ (согласно RFC)
			if ((parseresult / 100 == HTTPSERVERERRORCODE) || (parseresult / 100 == HTTPCLIENTERRORCODE))
			{
				entity = "<html><body>" + GetStringRepresent(parseresult) + "</body></html>";
			}
			else
			{
				entity = "";
			}
			ostringstream oss;
			oss << "HTTP/1.1 " << parseresult << " " << GetStringRepresent(parseresult) << (char)CR << (char)LF;
			oss << "Server: " << SERVERNAME << " " << SERVERVERSION << (char)CR << (char)LF;
			// если дату удалось получить, надо ее выдать в ответ
			if (date != "")
			{
				oss << "Date: " << date << (char)CR << (char)LF;
			}
			// если что-то отдается, оно типа text/html
			if (entity.length() != 0) oss << "Content-Type: text/html" << (char)CR << (char)LF;
			oss << "Content-Length: " << entity.length() << (char)CR << (char)LF;
			oss << (char)CR << (char)LF;
			response = oss.str();
		}
		else
		{
			// версия 0.9, но ответ мы выдать не можем - ничего не выдаем в ответ
			// функция выше сама закроет соединение
			response = "";
		}
		return;		
	}
	ostringstream oss(ios::out | ios::binary);
	// если версия не 0.9 формируем заголовок ответа
	if (!((majorversion == 0) && (minorversion == 9)))
	{
		oss << "HTTP/1.1 " << parseresult << " " << GetStringRepresent(parseresult) << (char)CR << (char)LF;
		// имя и версия сервера
		oss << "Server: " << SERVERNAME << " " << SERVERVERSION << (char)CR << (char)LF;
		// дата (должна получиться)
		oss << "Date: " << date << (char)CR << (char)LF;
		// поля заголовка объекта
		oss << "Content-Type: " << contenttype << (char)CR << (char)LF;
		oss << "Content-Length: " << entitylen << (char)CR << (char)LF;
		// поле Last Modified
		oss << "Last-Modified: " << lastmodified << (char)CR << (char)LF;	
		oss << (char)CR << (char)LF;
	}
	if (method != "HEAD")
	{
		// заполним поле entitypath, чтобы можно было отправлять параллельно с чтением
		entity = "";
		entitypath = allpath;
	}
	else
	{
		entity = "";
		entitypath = "";
	}
	response = oss.str();		
}

// возвращает строковую репрезентацию кода http ответа
string HTTPParser::GetStringRepresent(int code)
{
	switch(code)
	{
		case 100:  return REASONCONTINUE;
        case 101:  return REASONSWITCHINGPROTOCOLS;
        case 200:  return REASONOK;
        case 201:  return REASONCREATED;
        case 202:  return REASONACCEPTED;
        case 203:  return REASONNONAUTHORITATIVEINFORMATION;
        case 204:  return REASONNOCONTENT;
        case 205:  return REASONRESETCONTENT;
        case 206:  return REASONPARTIALCONTENT;
        case 300:  return REASONMULTIPLECHOICES;
        case 301:  return REASONMOVEDPERMANENTLY;
        case 302:  return REASONFOUND;
        case 303:  return REASONSEEOTHER;
        case 304:  return REASONNOTMODIFIED;
        case 305:  return REASONUSEPROXY;
        case 307:  return REASONTEMPORARYREDIRECT;
        case 400:  return REASONBADREQUEST;
        case 401:  return REASONUNAUTHORIZED;
        case 402:  return REASONPAYMENTREQUIRED;
        case 403:  return REASONFORBIDDEN;
        case 404:  return REASONNOTFOUND;
        case 405:  return REASONMETHODNOTALLOWED;
        case 406:  return REASONNOTACCEPTABLE;
        case 407:  return REASONPROXYAUTHENTIFICATIONREQUIRED;
        case 408:  return REASONREQUESTTIMEOUT;
        case 409:  return REASONCONFLICT;
        case 410:  return REASONGONE;
        case 411:  return REASONLENGTHREQUIRED;
        case 412:  return REASONPRECONDITIONFAILED;
        case 413:  return REASONREQUESTENTITYTOOLARGE;
        case 414:  return REASONREQUESTURITOOLARGE;
        case 415:  return REASONUNSUPPORTEDMEDIATYPE;
        case 416:  return REASONREQUESTEDRANGENOTSATISFIABLE;
        case 417:  return REASONEXPECTATIONFAILED;
        case 500:  return REASONINTERNALSERVERERROR;
        case 501:  return REASONNOTIMPLEMENTED;
        case 502:  return REASONBADGATEWAY;
        case 503:  return REASONSERVICEUNAVAILABLE;
        case 504:  return REASONGATEWAYTIMEOUT;
        case 505:  return REASONHTTPVERSIONNOTSUPPORTED;
		default: break;
	}
	// что-то странное - нам передан не HTTP код
	throw "Not a HTTP code";
	return "";
}

// разбор полей запроса. вернет FHTTPFAIL в случае проблем
int HTTPParser::ParseRequestFields(vector<string> &strs)
{
	// почистим структуру, хранящую значения
	values.clear();
	// если полей запроса вдруг не оказалось - ничего и не разбираем
	if (strs.size() == 0) return FHTTPOK;
	// заменим все HT на SP (для простоты)
	size_t i, j;
	for (i = 0; i < strs.size(); i++)
	{
		for (j = 0; j < strs[i].length(); j++)
		{
			if (strs[i][j] == HT)
			{
				strs[i][j] = SP;
			}
		}
	}
	// объединим строки, относящиеся к одному и тому же полю
	vector<string> fields;
	fields.clear();
	fields.push_back(strs[0]);
	for (i = 1; i < strs.size(); i++)
	{
		if (strs[i][0] == SP)
		{
			fields[fields.size() - 1] += strs[i];
		}
		else
		{
			fields.push_back(strs[i]);
		}
	}
	// развалим каждую строку на название и значение
	for (i = 0; i < strs.size(); i++)
	{
		int tpos = -1;
		string name = "";
		for (j = 0; j < strs[i].length(); j++)
		{
			if (strs[i][j] == ':')
			{
				tpos = j;
				break;
			}
			name += strs[i][j];
		}
		// нет двоеточия - плохой запрос
		if (tpos == -1)
		{
			parseresult = HTTPBADREQUEST;
			return FHTTPFAIL;
		}
		string curvalue = "";
		for (j = tpos + 2; j < strs[i].length(); j++) curvalue += strs[i][j];
		// название поля пустое - тоже не очень хороший запрос
		if (name == "")
		{
			parseresult = HTTPBADREQUEST;
			return FHTTPFAIL;
		}
		// если такого поля мы еще не встречали
		if (values.find(name) == values.end())
		{
			// то создадим его
			values[name] = curvalue;
		}
		else
		{
			// иначе добавим к его значению то что нам передали
			values[name] += "," + curvalue;
		}
	}
	// все хорошо - возвращаем правильное значение
	return FHTTPOK;
}

// возвращает расширение файла
string HTTPParser::GetExtension(string path)
{
	// все очень просто - ищет точку, а все после нее считаем расширением
	size_t i;
	for (i = path.length() - 1; i >= 0; i--)
	{
		if (path[i] == '.')
		{
			// нашли точку - формируем расширение и возвращаем его
			string res = "";
			size_t j;
			for (j = i + 1; j < path.length(); j++)
			{
				res += path[j];
			}
			return res;
		}
	}
	return "";
}

// возвращает значение поля http-запроса. Вернет пустую строку если такого поля не оказалось
string HTTPParser::GetField(string value)
{
	if (values.find(value) != values.end()) return values[value];
	else return "";
}

// декодирует строку согласно правилам кодирования URI
string HTTPParser::DecodeString(string s)
{
	// найдем в s все символы % и декодируем 
	string res = "";
	size_t i = 0;
	while (i < s.length())
	{
		if ((s[i] == '%') && (i + 2 < s.length()))
		{
			res += (char)(GetDigit(s[i + 1]) * 16 + GetDigit(s[i + 2]));
			i += 3;
		}
		else 
		{
			res += s[i];
			i++;
		}
	}
	return res;
}

// разбирает поле Range согласно RFC
void HTTPParser::ParseRangeField(off_t entitylen)
{
	// очистим список интервалов
	ranges.clear();
	// проверим, а есть ли вообще такое поле
	if (GetField("Range") == "")
	{
		// добавим интервал, соответствующий всему объекту
		ranges.push_back(make_pair(0, entitylen - 1));
		haverangefield = 0;
		// и более ничего не делаем
		return;
	}
	// если такое поле есть, то оно состоит из bytes-unit и bytes-range-spec,
	// разделенных знаком равенства
	string rangefield = GetField("Range");
	// делим поле на bytesunit и bytesrangespec
	vector<string> rangefieldparts;
	Split(rangefield, rangefieldparts, '=');
	// если неверное количество частей - плохой запрос
	if (rangefieldparts.size() != 2)
	{
		parseresult = HTTPBADREQUEST;
		return;	
	}
	string bytesunit, bytesrangespec;
	bytesunit = rangefieldparts[0];
	bytesrangespec = rangefieldparts[1];
	// если bytesunit не "bytes", то запрос не очень хороший
	if (bytesunit != "bytes")
	{
		parseresult = HTTPBADREQUEST;
		return;	
	}
	// разбираем спецификацию интервалов
	ParseBytesRangeSpec(entitylen, bytesrangespec);
	// поле не нужно учитывать - ничего более и не делаем
	if (haverangefield == 0) return;
	// сделаем интервалы непересекающимися
	MakeDisjointRanges();
	// и отсортируем их
	sort(ranges.begin(), ranges.end());
}

// разбивает строку на несколько по заданному символу
void HTTPParser::Split(string &s, vector<string> &res, char delim)
{
	res.clear();
	// ищем символ и заодно храним текущую строку
	string cur = "";
	size_t i;
	for (i = 0; i < s.length(); i++)
	{
		if (s[i] == delim)
		{
			res.push_back(cur);
			cur = "";
		}
		else cur += s[i];
	}
	// добавим то, что оставалось после последнего разделителя
	res.push_back(cur);
}

// переводит заведомо неотрицательное число в тип off_t
// будет бросаться exceptions при переполнении
// вернет -1 при неверном формате строки
// система счисления предполагается дестяичная
off_t HTTPParser::ConvertToOffsetType(string s)
{
	// получаем максимальное значения off_t
	unsigned long long maxofft = 0x7F;
	size_t i;
	for (i = 1; i < sizeof(off_t); i++)
	{
		maxofft <<= 8;
		maxofft |= 0xFF;
	}
	// теперь формируем ответ в типе unsigned long long (наибольший из возможных на текущий момент)
	unsigned long long res = 0;
	for (i = 0; i < s.length(); i++)
	{
		// в строке должно быть неотрицательное число, то есть только цифры
		if (! ((s[i] >= '0') && (s[i] <= '9')) )
		{
			return (off_t)(-1);
		}	
		res *= 10;
		res += (s[i] - '0');
		// переполнение?
		if (res > maxofft)
		{
			throw "Overflow exception!!!";
		}
	}
	return (off_t)(res);
}

// разбирает bytesrangespec (см. описание поля Range в RFC 2616)
void HTTPParser::ParseBytesRangeSpec(off_t entitylen, string &bytesrangespec)
{
	size_t i;
	// разделим bytesrangespec по запятым, так как может быть задано несколько интервалов
	vector<string> byterangesets;
	Split(bytesrangespec, byterangesets, ',');
	// обрабатываем каждый интервал
	// при приведении строк к числам будет генерироваться исключение в случае переполнений
	// исключения обрабатываются в клиентской функции, здесь мы этим не занимаемся
	for (i = 0; i < byterangesets.size(); i++)
	{
		// первый и последний байт
		off_t firstbyte, lastbyte;
		// разделяем интервал по дефису
		vector<string> currange;
		Split(byterangesets[i], currange, '-');
		// если неверное количество частей после разделения
		// это неверный byterangeset. В таком случае поле Range игнорируется
		if (currange.size() != 2)
		{
			// добавим интервал, соответствующий всему объекту
			ranges.push_back(make_pair(0, entitylen - 1));
			haverangefield = 0;
			// и более ничего не делаем
			return;
		}
		// если нет первой половины, это суффикс - обрабатывается по-особому
		if (currange[0] == "")
		{
			off_t numlast = ConvertToOffsetType(currange[1]);
			lastbyte = entitylen - 1;
			firstbyte = lastbyte - numlast + 1;
			if (firstbyte < 0) firstbyte = 0;
		}
		else
		{
			firstbyte = ConvertToOffsetType(currange[0]);
			// если есть правая граница интервала, используем ее. Иначе - граница объекта
			if (currange[1] != "") 
			{
				lastbyte = ConvertToOffsetType(currange[1]);
				// проверим этот интервал на корректность
				if (firstbyte > lastbyte)				
				{
					// плохой интервал - игнорировать поле Range
					// добавим интервал, соответствующий всему объекту
					ranges.push_back(make_pair(0, entitylen - 1));
					haverangefield = 0;
					// и более ничего не делаем
					return;
				}
			}
			else lastbyte = entitylen - 1;
			if (lastbyte >= entitylen) lastbyte = entitylen - 1;
		}
		// если полученный интервал не вырожден и покрывает какую-то часть нашего объекта, добавим его
		if ((firstbyte < lastbyte) && (firstbyte < entitylen))
		{
			ranges.push_back(make_pair(firstbyte, lastbyte));
		}
	}
	haverangefield = 1;
}

// делает интервалы в списке ranges непересекающимися
void HTTPParser::MakeDisjointRanges()
{
	// достаточно отсортировать интервалы по началу и далее если два соседних накладываются,
	// уменьшить правую границу того интервала, который левее
	// сортируем
	sort(ranges.begin(), ranges.end());
	// изменяем границы
	size_t i;
	for (i = 0; i < ranges.size() - 1; i++)
	{
		if (ranges[i].second >= ranges[i + 1].first)
		{
			ranges[i].second = ranges[i + 1].first - 1;
		}
	}
	// ну и осталось удалить вырожденные интервалы
	DeleteDegenerateRanges();
}

// удаляет вырожденные интервалы
void HTTPParser::DeleteDegenerateRanges()
{
	vector<pair<off_t, off_t> > tmp;
	tmp = ranges;
	ranges.clear();
	size_t i;
	for (i = 0; i < tmp.size(); i++)
	{
		if (tmp[i].first <= tmp[i].second)
		{
			ranges.push_back(tmp[i]);
		}
	}
}
