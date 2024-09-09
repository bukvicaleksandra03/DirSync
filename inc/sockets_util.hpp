#ifndef _SOCKETS_UTIL_HPP
#define _SOCKETS_UTIL_HPP

#include "common.hpp"
#include "file_sys_util.hpp"
class Comp;

class SocketsUtil {
private:
    ofstream* log_file;
    Comp* comp;

    int socket_fd;
    bool socket_fd_initialized;

    string base_dir;
    FileSysUtil* fsu;

public:
    SocketsUtil(string base_dir, FileSysUtil* fsu, ofstream* log_file, Comp* comp) {
        this->log_file = log_file;
        this->base_dir = base_dir;
        this->fsu = fsu;
        this->socket_fd_initialized = false;
        this->comp = comp;
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