#include "comp1.hpp"

Comp1::Comp1(const std::string& dir_to_sync, const std::string& ip_addr,
             const int port_num)
    : Comp(dir_to_sync) {
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port_num);

  if (inet_pton(AF_INET, ip_addr.c_str(), &servaddr.sin_addr) <= 0)
    err_n_die("inet_pton error for %s", ip_addr.c_str());

  register_signals();
}

void Comp1::register_signals() {
  // Register the signal handler for each signal in the array
  for (int sig : signals) {
    if (signal(sig, signal_handler) == SIG_ERR) {
      std::cerr << "Error registering signal: " << sig << '\n';
    }
  }
}

void Comp1::connect_and_run() {
  while (true) {
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      err_n_die("Error while creating the socket!");
    while (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) < 0) {
      std::cout << "Trying to connect\n";
      sleep(TRY_TO_RECONNECT_PERIOD);
    }
    std::cout << "Connected successfully!\n";
    reconnect_needed = 0;

    try {
      sock_util->set_socket_fd(sockfd);

      // Connection has been established, now we synchronize every SYNC_PERIOD
      // seconds
      while (reconnect_needed == 0) {
        log_file->open();

        fsu->get_dir_content();
        fsu->get_last_sync_content(&prev_file_condition, &prev_dir_condition);

        // ---- deleted directories ----
        std::vector<std::string> my_deleted_dirs =
            fsu->find_deleted(&prev_dir_condition, &curr_dirs);
        // receiving info about directories deleted by peer
        int rcv_num_del_dirs;
        SocketsUtil::readn(sockfd, &rcv_num_del_dirs, sizeof(int));
        if (rcv_num_del_dirs > 0) {
          std::unordered_map<std::string, uint64_t> rcv_del_dirs;
          *log_file << "Receiving...\nPeer deleted directores:\n";
          for (int i = 0; i < rcv_num_del_dirs; i++) {
            sock_util->receive_fname_and_modif_time(rcv_del_dirs);
          }
          log_file->write_out_map(rcv_del_dirs);
          *log_file << '\n';
          fsu->delete_dirs(&rcv_del_dirs);
        }
        // sending info about directories deleted by me
        const int num_del_dirs = my_deleted_dirs.size();
        SocketsUtil::writen(sockfd, &num_del_dirs, sizeof(int));
        if (num_del_dirs > 0) {
          *log_file << "Sending...\nI deleted directores:\n";
          for (std::string& df : my_deleted_dirs) {
            *log_file << df << '\n';
            sock_util->send_fname_and_modif_time(df, 0);
          }
          *log_file << '\n';
        }

        // ---- deleted files ----
        const std::vector<std::string> my_deleted_files =
            fsu->find_deleted(&prev_file_condition, &curr_files);
        // receiving info about files deleted by peer
        int server_num_del_files;
        SocketsUtil::readn(sockfd, &server_num_del_files, sizeof(int));
        if (server_num_del_files > 0) {
          std::unordered_map<std::string, uint64_t> serv_del_files;
          *log_file << "Receiving...\nPeer deleted files:\n";
          for (int i = 0; i < server_num_del_files; i++) {
            sock_util->receive_fname_and_modif_time(serv_del_files);
          }
          log_file->write_out_map(serv_del_files);
          *log_file << '\n';
          fsu->delete_files(&serv_del_files);
        }
        // sending info about files deleted by me
        int num_del_files = my_deleted_files.size();
        SocketsUtil::writen(sockfd, &num_del_files, sizeof(int));
        if (num_del_files > 0) {
          *log_file << "Sending...\nI deleted files:\n";
          for (const std::string& df : my_deleted_files) {
            *log_file << df << '\n';
            sock_util->send_fname_and_modif_time(df, 0);
          }
          *log_file << '\n';
        }

        // ---- directories list ----
        // receiving directories list
        int recv_num_dirs;
        SocketsUtil::readn(sockfd, &recv_num_dirs, sizeof(int));
        std::unordered_map<std::string, uint64_t> received_dirs;
        if (recv_num_dirs > 0) {
          *log_file << "Receiving...\nPeer has directories:\n";
          for (int i = 0; i < recv_num_dirs; i++) {
            sock_util->receive_fname_and_modif_time(received_dirs);
          }
          log_file->write_out_map(received_dirs);
          *log_file << '\n';
        }
        // sending directories list
        int num_dirs = curr_dirs.size();
        SocketsUtil::writen(sockfd, &num_dirs, sizeof(int));
        if (num_dirs > 0) {
          *log_file << "Sending...\nI have directories:\n";
          for (auto& um : curr_dirs) {
            *log_file << um.first << '-' << um.second << '\n';
            sock_util->send_fname_and_modif_time(um.first, um.second);
          }
          *log_file << '\n';
        }
        fsu->create_dirs(&received_dirs);

        // ---- files list ----
        // receiving file list and modification times
        int num_files;
        SocketsUtil::readn(sockfd, &num_files, sizeof(int));
        std::unordered_map<std::string, uint64_t> received_files;
        if (num_files > 0) {
          for (int i = 0; i < num_files; i++) {
            sock_util->receive_fname_and_modif_time(received_files);
          }
        }

        std::vector<std::string> out_of_date =
            fsu->out_of_date_files(&received_files);
        std::vector<std::string> newer_here = fsu->newer_files(&received_files);

        // sending names of files we want to receive
        int num_to_receive = out_of_date.size();
        SocketsUtil::writen(sockfd, &num_to_receive, sizeof(int));
        if (num_to_receive > 0) {
          for (auto& s : out_of_date)
            sock_util->send_fname_and_modif_time(s, 0);
          // receiving those files
          for (int i = 0; i < num_to_receive; i++)
            sock_util->receive_file_over_socket();
        }

        // sending number of files we want to send
        int num_to_send = newer_here.size();
        SocketsUtil::writen(sockfd, &num_to_send, sizeof(int));
        if (num_to_send > 0) {
          // sending those files
          for (int i = 0; i < num_to_send; i++) {
            sock_util->send_file_over_socket(newer_here[i]);
          }
        }

        fsu->save_dir_content();
        log_file->open();
        sleep(SYNC_PERIOD);
      }
    } catch (SocketError& se) {
      fsu->save_dir_content();
      log_file->open();
      close(sockfd);
    } catch (runtime_error& re) {
      err_n_die(re.what());
    }
  }
}

void Comp1::signal_handler(int signal_num) {
  std::cout << "Program interrupted by signal: ";
  switch (signal_num) {
  case SIGPIPE:
    std::cout << "SIGPIPE" << '\n';
    instance->fsu->save_dir_content();
    close(instance->sockfd);
    instance->reconnect_needed = 1;
    return;
    break;
  case SIGABRT:
    std::cout << "SIGABRT" << '\n';
    break;
  case SIGTERM:
    std::cout << "SIGTERM" << '\n';
    break;
  case SIGTSTP:
    std::cout << "SIGTSTP" << '\n';
    break;
  default:
    std::cout << signal_num << '\n';
    break;
  }

  instance->fsu->save_dir_content();
  close(instance->sockfd);
  // It terminates the program
  exit(1);
}

void Comp1::err_n_die(const char* fmt, ...) {
  std::cout << "err_n_die comp1" << '\n';

  // Forward variadic arguments to the base class method
  va_list ap;
  va_start(ap, fmt);
  Comp::err_n_die(fmt,
                  ap); // Call the base class method with variadic arguments
  va_end(ap);

  exit(1);
}