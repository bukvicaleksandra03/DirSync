#ifndef _COMP1_HPP_
#define _COMP1_HPP_

#include "comp.hpp"
#include "file_sys_util.hpp"
#include "sockets_util.hpp"
#include "common.hpp"

class Comp1 : public Comp {
private:
    void run() override;
    static void signal_handler(int signal_num);
    void err_n_die(const char *fmt, ...) override;

public:
    Comp1(const string& dir_to_sync, const string& ip_addr);

    void connect_and_run() override;

    void listen_for_connection() override {};

};

#endif