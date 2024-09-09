#include "comp1.hpp"

Comp1::Comp1(const string& dir_to_sync, const string& ip_addr) : Comp(dir_to_sync) {
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT_NUM);

    if (inet_pton(AF_INET, ip_addr.c_str(), &servaddr.sin_addr) <= 0)
        err_n_die("inet_pton error for %s", ip_addr.c_str());

    signal(SIGPIPE, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGTSTP, signal_handler);
}

void Comp1::connect_and_run() {
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_n_die("Error while creating the socket!");
    while (connect(sockfd, (SA*)& servaddr, sizeof(servaddr)) < 0) {
        printf("trying to connect\n");
        sleep(TRY_TO_RECONNECT_PERIOD);
    }
    printf("Connected successfully!\n");
    
    run();
}

void Comp1::run() 
{
    sock_util->set_socket_fd(sockfd);
    //Connection has been established, now we synchronize every SYNC_PERIOD seconds
    while (true)
    {
        log_file.close();
        log_file.open(dir_to_sync + '/' + "sync_log.txt", std::ios::out | std::ios::trunc);
        if (!log_file.is_open()) {
            err_n_die("Log file could not be opened.");
        }

        fsu->get_dir_content();
        fsu->get_last_sync_content(&prev_file_condition, &prev_dir_condition);

        // synchronizing directory deletion
        vector<string> my_deleted_dirs = fsu->find_deleted(&prev_dir_condition, &curr_dirs);
        int rcv_num_del_dirs;
        recv(sockfd, &rcv_num_del_dirs, sizeof(int), 0);
        unordered_map<string, uint64_t> rcv_del_dirs;
        write_out_map("Rcv_del_dirs: ", rcv_del_dirs, &log_file);
        for (int i = 0; i < rcv_num_del_dirs; i++) {
            sock_util->receive_fname_and_modif_time(rcv_del_dirs);
        }
        fsu->delete_dirs(&rcv_del_dirs);
     
        int num_del_dirs = my_deleted_dirs.size();
        write(sockfd, &num_del_dirs, sizeof(int));
        for (string& df: my_deleted_dirs) {
            sock_util->send_fname_and_modif_time(df, 0);
        }

        // synchronizing file deletion
        vector<string> my_deleted_files = fsu->find_deleted(&prev_file_condition, &curr_files);
        int server_num_del_files;
        recv(sockfd, &server_num_del_files, sizeof(int), 0);
        unordered_map<string, uint64_t> serv_del_files;
        for (int i = 0; i < server_num_del_files; i++) {
            sock_util->receive_fname_and_modif_time(serv_del_files);
        }
        write_out_map("serv_del_files: ", serv_del_files, &log_file);
        fsu->delete_files(&serv_del_files);
        int num_del_files = my_deleted_files.size();
        write(sockfd, &num_del_files, sizeof(int));
        for (string& df: my_deleted_files) {
            sock_util->send_fname_and_modif_time(df, 0);
        }
        write_out_vector("my_deleted_files: ", my_deleted_files, &log_file);

        // receiving directories list
        int recv_num_dirs;
        recv(sockfd, &recv_num_dirs, sizeof(int), 0);
        unordered_map<string, uint64_t> received_dirs;
        for (int i = 0; i < recv_num_dirs; i++) {
            sock_util->receive_fname_and_modif_time(received_dirs);
        }

        write_out_map("received_dirs", received_dirs, &log_file);
        // sending directories list
        int num_dirs = curr_dirs.size();
        write(sockfd, &num_dirs, sizeof(int));
        for (auto &um: curr_dirs) 
            sock_util->send_fname_and_modif_time(um.first, um.second);
        fsu->create_dirs(&received_dirs);
        write_out_map("sent_dirs", curr_dirs, &log_file);
        
        // receiving file list and modification times
        int num_files;
        recv(sockfd, &num_files, sizeof(int), 0);
        unordered_map<string, uint64_t> received_files;
        for (int i = 0; i < num_files; i++) 
            sock_util->receive_fname_and_modif_time(received_files);

        write_out_map("received_files", received_files, &log_file);

        vector<string> out_of_date = fsu->out_of_date_files(&received_files);
        vector<string> newer_here = fsu->newer_files(&received_files);

        write_out_vector("out_of_date: ", out_of_date, &log_file);
        write_out_vector("newer_here: ", newer_here, &log_file);

        // sending names of files we want to receive
        int num_to_receive = out_of_date.size();
        write(sockfd, &num_to_receive, sizeof(int));
        for (auto& s: out_of_date) 
            sock_util->send_fname_and_modif_time(s, 0);
        // receiving those files
        for (int i = 0; i < num_to_receive; i++) 
            sock_util->receive_file_over_socket();

        // sending number of files we want to send
        int num_to_send = newer_here.size();
        write(sockfd, &num_to_send, sizeof(int));
        // sending those files
        for (int i = 0; i < num_to_send; i++) {
            sock_util->send_file_over_socket(newer_here[i]);
        }

        fsu->save_dir_content();
        log_file.clear();
        sleep(SYNC_PERIOD);
    }
}



void Comp1::signal_handler(int signal_num) 
{
    assert(instance != nullptr);
    cout << "Program interrupted by signal: "; 
    switch (signal_num)
    {
    case SIGPIPE:
        cout << "SIGPIPE" << endl;
        cout.flush();
        close(instance->sockfd);
        instance->connect_and_run();
        break;
    case SIGABRT:
        cout << "SIGABRT" << endl;
        cout.flush();
        cout.flush();
        break;
    case SIGTERM:
        cout << "SIGTERM" << endl;
        cout.flush();
        close(instance->sockfd);
        break;
    case SIGTSTP:
        cout << "SIGTSTP" << endl;
        close(instance->sockfd);
        break;
    default:
        cout << signal_num << endl;
        cout.flush();
        close(instance->sockfd);
        break;
    }

    // It terminates the  program 
    exit(1);
}

void Comp1::err_n_die(const char *fmt, ...) 
{
    cout << "err_n_die comp1" << endl;

    // Forward variadic arguments to the base class method
    va_list ap;
    va_start(ap, fmt);
    Comp::err_n_die(fmt, ap); // Call the base class method with variadic arguments
    va_end(ap);

    exit(1);
}