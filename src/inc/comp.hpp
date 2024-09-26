#ifndef _COMP_HPP_
#define _COMP_HPP_

#define SYNC_PERIOD 120
#define TRY_TO_RECONNECT_PERIOD 60
#define SA struct sockaddr

extern "C" {
#include <arpa/inet.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
}

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "file_state_file.hpp"
#include "file_sys_util.hpp"
#include "log_file.hpp"
#include "sockets_util.hpp"

class Comp {
protected:
  int signals[7] = {SIGPIPE, SIGABRT, SIGTERM, SIGTSTP,
                    SIGINT,  SIGHUP,  SIGQUIT};

public:
  // we use this in in signal_handler() instead of "this",
  // because signal_handler() has to be static
  static Comp* instance;

  volatile sig_atomic_t reconnect_needed;

  FileSysUtil* fsu;
  SocketsUtil* sock_util;

  string dir_to_sync;

  std::shared_ptr<LogFile> log_file;
  std::shared_ptr<FileStateFile> last_sync_state;

  std::unordered_map<string, uint64_t> curr_files;
  std::unordered_map<string, uint64_t> curr_dirs;

  std::unordered_map<string, uint64_t> prev_file_condition;
  std::unordered_map<string, uint64_t> prev_dir_condition;

  int sockfd;
  int listenfd;
  struct sockaddr_in servaddr;

  virtual void connect_and_run() = 0;

  virtual void listen_for_connection() = 0;

  virtual void err_n_die(const char* fmt, ...);

  Comp(const string& dir_to_sync);

  ~Comp();
};

#endif