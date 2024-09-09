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
#include <filesystem>

#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <assert.h>
using namespace std;

//#define DEBUG

#define SYNC_PERIOD 10
#define TRY_TO_RECONNECT_PERIOD 10
#define PORT_NUM 18200
#define SA struct sockaddr

void write_out_map(string s, unordered_map<string, uint64_t>& um, ofstream* ofs);
void write_out_vector(string s, vector<string>& vec, ofstream* ofs);

#endif