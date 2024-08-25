#ifndef _COMMON_H_
#define _COMMON_H_

#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>

#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#define SYNC_PERIOD 120

void err_n_die(const char *fmt, ...);

#endif