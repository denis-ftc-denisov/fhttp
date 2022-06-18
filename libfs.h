#ifndef FHTTP_FILESYSTEM_HEADER_FILE_INCLUDED_WUYRUYSFHJBFNMWGRYWDHGFJKSFHJKSFKJSHFKJNGMWFJKHW
#define FHTTP_FILESYSTEM_HEADER_FILE_INCLUDED_WUYRUYSFHJBFNMWGRYWDHGFJKSFHJKSFKJSHFKJNGMWFJKHW

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
using namespace std;

// получает размер файла, заданного путем path
// с помощью функции stat
// в случае ошибки значение не определено
off_t GetFileSize(string path);

#endif
