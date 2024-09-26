#ifndef _COMP1_HPP_
#define _COMP1_HPP_

#include "comp.hpp"
#include "file_sys_util.hpp"
#include "sockets_util.hpp"

class Comp1 : public Comp {
private:
  static void signal_handler(int signal_num);
  void register_signals();
  void err_n_die(const char* fmt, ...) override;

public:
  Comp1(const std::string& dir_to_sync, const std::string& ip_addr,
        const int port_num);

  void connect_and_run() override;

  void listen_for_connection() override {};
};

#endif