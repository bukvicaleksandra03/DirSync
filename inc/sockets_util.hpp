#ifndef _SOCKETS_UTIL_HPP
#define _SOCKETS_UTIL_HPP

#include "common.hpp"

#define SERVER_PORT 18010
#define SA struct sockaddr

void send_fname_and_modif_time(int socket_fd,  const string& file_name, uint64_t modif_time);
void receive_fname_and_modif_time(int socket_fd, unordered_map<string, uint64_t>& files);

void send_file_over_socket(int socket_fd, const string& dir_name, const string& file_name);
void receive_file_over_socket(int socket_fd, const string& dir_name);

#endif