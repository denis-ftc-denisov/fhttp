#include "libfs.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
using namespace std;

// получает размер файла с помощью функции stat
// в случае ошибки значение не определено
off_t GetFileSize(string path)
{
	struct stat fileinfo;
	if (stat(path.c_str(), &fileinfo) == -1)
	{
		return (off_t)0;
	}
	return fileinfo.st_size;
}
