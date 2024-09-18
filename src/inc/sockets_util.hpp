#ifndef _SOCKETS_UTIL_HPP
#define _SOCKETS_UTIL_HPP

#include <fcntl.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <stdexcept>
#include "file_sys_util.hpp"
class Comp;

struct SocketError : public std::runtime_error {
    explicit SocketError()
        : std::runtime_error("socket error") {}
};

class SocketsUtil {
private:
    shared_ptr<LogFile> log_file;

    int socket_fd;
    bool socket_fd_initialized;

    string base_dir;
    FileSysUtil* fsu;

public:

    static ssize_t readn(int fd, void *buf, size_t n);
    static ssize_t writen(int fd, const void *buf, size_t n);

    SocketsUtil(string base_dir, FileSysUtil* fsu, const shared_ptr<LogFile>& log_file) {
        this->log_file = log_file;
        this->base_dir = base_dir;
        this->fsu = fsu;
        this->socket_fd_initialized = false;
    }

    void set_socket_fd(int sfd) 
    { 
        socket_fd = sfd; 
        socket_fd_initialized = true;
    }

    void send_fname_and_modif_time(const string& file_name, uint64_t modif_time);
    void receive_fname_and_modif_time(unordered_map<string, uint64_t>& files);
    
    void send_file_over_socket(const string& file_name);
    void receive_file_over_socket();
};

#endif