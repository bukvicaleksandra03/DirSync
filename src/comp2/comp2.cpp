#include "comp2.hpp"

Comp2::Comp2(const std::string& dir_to_sync, const int port_num)
    : Comp(dir_to_sync) {
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port_num);

  register_signals();
}

void Comp2::register_signals() {
  // Register the signal handler for each signal in the array
  for (int sig : signals) {
    if (signal(sig, signal_handler) == SIG_ERR) {
      std::cerr << "Error registering signal: " << sig << '\n';
    }
  }
}

void Comp2::listen_for_connection() {
  int opt = 1;

  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    err_n_die("socket error");

  // Set SO_REUSEADDR option
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    err_n_die("setsockopt(SO_REUSEADDR) error");

  while ((bind(listenfd, (SA*)&servaddr, sizeof(servaddr))) < 0)
    err_n_die("bind error");

  if ((listen(listenfd, 10)) < 0) err_n_die("listen error");
}

void Comp2::connect_and_run() {
  while (true) {
    std::cout << "Waiting for connection\n";
    sockfd = accept(listenfd, (SA*)NULL, NULL);
    if (sockfd == -1) err_n_die("accept error");
    std::cout << "Connected\n";
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
        // sending info about directories deleted by me
        int num_del_dirs = my_deleted_dirs.size();
        SocketsUtil::writen(sockfd, &num_del_dirs, sizeof(int));
        if (num_del_dirs > 0) {
          *log_file << "Sending...\nI deleted directores:\n";
          for (std::string& df : my_deleted_dirs) {
            *log_file << df << '\n';
            sock_util->send_fname_and_modif_time(df, 0);
          }
          *log_file << '\n';
        }
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

        // ---- deleted files ----
        std::vector<std::string> my_deleted_files =
            fsu->find_deleted(&prev_file_condition, &curr_files);
        // sending info about files deleted by me
        int num_del_files = my_deleted_files.size();
        SocketsUtil::writen(sockfd, &num_del_files, sizeof(int));
        if (num_del_files > 0) {
          *log_file << "Sending...\nI deleted files:\n";
          for (std::string& df : my_deleted_files) {
            *log_file << df << '\n';
            sock_util->send_fname_and_modif_time(df, 0);
          }
          *log_file << '\n';
        }
        // receiving info about files deleted by peer
        int client_num_del_files;
        SocketsUtil::readn(sockfd, &client_num_del_files, sizeof(int));
        if (client_num_del_files > 0) {
          std::unordered_map<std::string, uint64_t> client_del_files;
          *log_file << "Receiving...\nPeer deleted files:\n";
          for (int i = 0; i < client_num_del_files; i++) {
            sock_util->receive_fname_and_modif_time(client_del_files);
          }
          log_file->write_out_map(client_del_files);
          *log_file << '\n';
          fsu->delete_files(&client_del_files);
        }

        // ---- directories list ----
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
        // receiving directories list
        int recv_num_dirs;
        SocketsUtil::readn(sockfd, &recv_num_dirs, sizeof(int));
        if (recv_num_dirs > 0) {
          *log_file << "Receiving...\nPeer has directories:\n";
          std::unordered_map<std::string, uint64_t> received_dirs;
          for (int i = 0; i < recv_num_dirs; i++) {
            sock_util->receive_fname_and_modif_time(received_dirs);
          }
          log_file->write_out_map(received_dirs);
          *log_file << '\n';
          fsu->create_dirs(&received_dirs);
        }

        // ---- files list ----
        // sending file list and modification times
        int num_files = curr_files.size();
        SocketsUtil::writen(sockfd, &num_files, sizeof(int));
        if (num_files > 0) {
          *log_file << "Sending...\nMy files are:\n";
          for (auto& um : curr_files) {
            *log_file << um.first << '-' << um.second << '\n';
            sock_util->send_fname_and_modif_time(um.first, um.second);
          }
          *log_file << '\n';
        }
        // receiving names of files we need to send
        int num_to_send;
        SocketsUtil::readn(sockfd, &num_to_send, sizeof(int));
        if (num_to_send > 0) {
          std::unordered_map<std::string, uint64_t> files_to_send;
          for (int i = 0; i < num_to_send; i++) {
            sock_util->receive_fname_and_modif_time(files_to_send);
          }
          // sending those files
          for (auto& fs : files_to_send) {
            sock_util->send_file_over_socket(fs.first);
          }
        }
        // receiving number of files we need to receive
        int num_to_receive;
        SocketsUtil::readn(sockfd, &num_to_receive, sizeof(int));
        if (num_to_receive > 0) {
          for (int i = 0; i < num_to_receive; i++) {
            sock_util->receive_file_over_socket();
          }
        }

        fsu->save_dir_content();
        log_file->close();
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

void Comp2::signal_handler(int signal_num) {
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
  close(instance->listenfd);
  // It terminates the  program
  exit(0);
}

void Comp2::err_n_die(const char* fmt, ...) {
  std::cout << "err_n_die comp2" << '\n';

  // Forward variadic arguments to the base class method
  va_list ap;
  va_start(ap, fmt);
  Comp::err_n_die(fmt,
                  ap); // Call the base class method with variadic arguments
  va_end(ap);

  close(listenfd);
  exit(1);
}