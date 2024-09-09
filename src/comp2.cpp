#include "comp2.hpp"

Comp2::Comp2(const string& dir_to_sync) : Comp(dir_to_sync) {

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT_NUM); 

    signal(SIGPIPE, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGTSTP, signal_handler);

}

void Comp2::listen_for_connection()
{
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_n_die("socket error");
    
    while ((bind(listenfd, (SA*)&servaddr, sizeof(servaddr))) < 0) 
        err_n_die("bind error");

    std::cout << "Waiting for connection on port: " << PORT_NUM << std::endl;
    fflush(stdout);

    if ((listen(listenfd, 10)) < 0) 
        err_n_die("listen error");
}

void Comp2::connect_and_run()
{
    cout << "waiting for connection\n" << endl;
    sockfd = accept(listenfd, (SA*) NULL, NULL);
    if (sockfd == -1) 
        err_n_die("accept error");
    run();
}

void Comp2::run() 
{
    sock_util->set_socket_fd(sockfd);
    // Connection has been established, now we synchronize every SYNC_PERIOD seconds
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
        int num_del_dirs = my_deleted_dirs.size();
        write(sockfd, &num_del_dirs, sizeof(int));
        for (string& df: my_deleted_dirs) {
            sock_util->send_fname_and_modif_time(df, 0);
        }
        int rcv_num_del_dirs;
        recv(sockfd, &rcv_num_del_dirs, sizeof(int), 0);
        unordered_map<string, uint64_t> rcv_del_dirs;
        for (int i = 0; i < rcv_num_del_dirs; i++) {
            sock_util->receive_fname_and_modif_time(rcv_del_dirs);
        }
        write_out_map("Rcv_del_dirs: ", rcv_del_dirs, &log_file);
        fsu->delete_dirs(&rcv_del_dirs);

        // synchronizing file deletion
        vector<string> my_deleted_files = fsu->find_deleted(&prev_file_condition, &curr_files);
        int num_del_files = my_deleted_files.size();
        write(sockfd, &num_del_files, sizeof(int));
        for (string& df: my_deleted_files) {
            sock_util->send_fname_and_modif_time(df, 0);
        }
        write_out_vector("my_deleted_files: ", my_deleted_files, &log_file);

        int client_num_del_files;
        recv(sockfd, &client_num_del_files, sizeof(int), 0);
        unordered_map<string, uint64_t> client_del_files;
        for (int i = 0; i < client_num_del_files; i++) {
            sock_util->receive_fname_and_modif_time(client_del_files);
        }
        fsu->delete_files(&client_del_files);
        write_out_map("client_del_files: ", client_del_files, &log_file);

        // sending directories list
        int num_dirs = curr_dirs.size();
        log_file << "Num dirs: " << num_dirs << endl;
        write_out_map("Curr dirs:", curr_dirs, &log_file);
        write(sockfd, &num_dirs, sizeof(int));
        for (auto &um: curr_dirs) {
            //log_file << um.first << " " << um.second << endl;
            sock_util->send_fname_and_modif_time(um.first, um.second);
        }
            
        
        write_out_map("sent_dirs", curr_dirs, &log_file);
        
        // receiving directories list
        int recv_num_dirs;
        recv(sockfd, &recv_num_dirs, sizeof(int), 0);
        unordered_map<string, uint64_t> received_dirs;
        for (int i = 0; i < recv_num_dirs; i++) 
            sock_util->receive_fname_and_modif_time(received_dirs);
        fsu->create_dirs(&received_dirs);
        write_out_map("received_dirs", received_dirs, &log_file);

        // sending file list and modification times
        int num_files = curr_files.size();
        write(sockfd, &num_files, sizeof(int));
        for (auto &um: curr_files) 
            sock_util->send_fname_and_modif_time(um.first, um.second);

        write_out_map("sent_files", curr_files, &log_file);

        // receiving names of files we need to send
        int num_to_send;
        recv(sockfd, &num_to_send, sizeof(int), 0);
        unordered_map<string, uint64_t> files_to_send;
        for (int i = 0; i < num_to_send; i++) {
            sock_util->receive_fname_and_modif_time(files_to_send);
        }
        write_out_map("files we need to send:", files_to_send, &log_file);
        // sending those files
        for (auto& fs: files_to_send) {
            sock_util->send_file_over_socket(fs.first);
        }

        // receiving number of files we need to receive
        int num_to_receive;
        recv(sockfd, &num_to_receive, sizeof(int), 0);
        for (int i = 0; i < num_to_receive; i++) {
            sock_util->receive_file_over_socket();
        }

        fsu->save_dir_content();
        sleep(SYNC_PERIOD);
    }
}

void Comp2::signal_handler(int signal_num) 
{ 
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
        break;
    case SIGTERM:
        cout << "SIGTERM" << endl;
        cout.flush();
        close(instance->listenfd);
        close(instance->sockfd);
        break;
    case SIGTSTP:
        cout << "SIGTSTP" << endl;
        cout.flush();
        close(instance->listenfd);
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

void Comp2::err_n_die(const char *fmt, ...) 
{
    cout << "err_n_die comp2" << endl;
   
    // Forward variadic arguments to the base class method
    va_list ap;
    va_start(ap, fmt);
    Comp::err_n_die(fmt, ap); // Call the base class method with variadic arguments
    va_end(ap);

    close(listenfd);
    exit(1);
}