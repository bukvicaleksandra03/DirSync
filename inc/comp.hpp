#ifndef _COMP_HPP_
#define _COMP_HPP_

#include "file_sys_util.hpp"
#include "sockets_util.hpp"
#include "common.hpp"

class Comp {
public:
    // we use this in in signal_handler() instead of "this",
    // because signal_handler() has to be static
    static Comp* instance;

    FileSysUtil* fsu;
    SocketsUtil* sock_util;

    string dir_to_sync;
    ofstream log_file;
    fstream last_sync_state;

    unordered_map<string, uint64_t> curr_files;
    unordered_map<string, uint64_t> curr_dirs;

    unordered_map<string, uint64_t> prev_file_condition;
    unordered_map<string, uint64_t> prev_dir_condition;

    int sockfd;
    int listenfd;
    struct sockaddr_in servaddr;

    virtual void run() = 0;
   
    virtual void connect_and_run() = 0;

    virtual void listen_for_connection() = 0;

    virtual void err_n_die(const char *fmt, ...);

    Comp(const string& dir_to_sync);

    ~Comp();
};

#endif