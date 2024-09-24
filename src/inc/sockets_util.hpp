#ifndef _SOCKETS_UTIL_HPP
#define _SOCKETS_UTIL_HPP

extern "C" {
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <unistd.h>
}

#include <stdexcept>

#include "file_sys_util.hpp"
class Comp;

struct SocketError : public std::runtime_error {
  explicit SocketError() : std::runtime_error("socket error") {}
};

class SocketsUtil {
private:
  std::shared_ptr<LogFile> log_file;

  int socket_fd;
  bool socket_fd_initialized;

  std::string base_dir;
  FileSysUtil* fsu;

public:
  static ssize_t readn(int fd, void* buf, size_t n);
  static ssize_t writen(int fd, const void* buf, size_t n);

  SocketsUtil(std::string base_dir, FileSysUtil* fsu,
              const std::shared_ptr<LogFile>& log_file) {
    this->log_file = log_file;
    this->base_dir = base_dir;
    this->fsu = fsu;
    this->socket_fd_initialized = false;
  }

  void set_socket_fd(int sfd) {
    socket_fd = sfd;
    socket_fd_initialized = true;
  }

  void send_fname_and_modif_time(const std::string& file_name,
                                 uint64_t modif_time);
  void receive_fname_and_modif_time(
      std::unordered_map<std::string, uint64_t>& files);

  void send_file_over_socket(const std::string& file_name);
  void receive_file_over_socket();
};

#endif