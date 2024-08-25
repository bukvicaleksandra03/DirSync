#include "common.hpp"
#include "file_sys_util.hpp"
#include "sockets_util.hpp"

void write_out_map(string s, unordered_map<string, uint64_t>& um) {
    cout << s << endl;
    for (auto& u: um) {
        cout << u.first << " - " << u.second << endl;
    }
}

int main(int argc, char** argv) 
{
    // Establishing connection
    int listenfd, connfd;
    struct sockaddr_in servaddr;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_n_die("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    if ((bind(listenfd, (SA*)&servaddr, sizeof(servaddr))) < 0) 
        err_n_die("bind error");

    if ((listen(listenfd, 10)) < 0)
        err_n_die("listen error");
    
    
    struct sockaddr_in addr;

    printf("waiting for a connection on port %d\n", SERVER_PORT);
    fflush(stdout);
    connfd = accept(listenfd, (SA*) NULL, NULL);

    string dir_sync_from = "/home/aleksandra/sync1";

    // we use this to check which files have been deleted since the last time we synced
    unordered_map<string, uint64_t> prev_file_condition;

    // Connection has been established, now we synchronize every 5 minutes
    while (true) 
    {
        // maps the name of the file to the time of modification (in Unix time, seconds from 1970)
        unordered_map<string, uint64_t> all_curr_files;
        get_all_files_in_directory(dir_sync_from, dir_sync_from, all_curr_files);

        // synchronizing file deletion
        vector<string> my_deleted_files = find_deleted_files(prev_file_condition, all_curr_files);
        int num_del_files = my_deleted_files.size();
        write(connfd, &num_del_files, sizeof(int));
        for (string& df: my_deleted_files) {
            send_fname_and_modif_time(connfd, df, 0);
        }
        int client_num_del_files;
        recv(connfd, &client_num_del_files, sizeof(int), 0);
        unordered_map<string, uint64_t> client_del_files;
        for (int i = 0; i < client_num_del_files; i++) {
            receive_fname_and_modif_time(connfd, client_del_files);
        }
        delete_files(dir_sync_from, client_del_files, all_curr_files);

        // sending file list and modification times
        int num_files = all_curr_files.size();
        write(connfd, &num_files, sizeof(int));
        for (auto &um: all_curr_files) 
            send_fname_and_modif_time(connfd, um.first.c_str(), um.second);

        // receiving names of files we need to send
        int num_to_send;
        recv(connfd, &num_to_send, sizeof(int), 0);
        unordered_map<string, uint64_t> files_to_send;
        for (int i = 0; i < num_to_send; i++) {
            receive_fname_and_modif_time(connfd, files_to_send);
        }
        // sending those files
        for (auto& fs: files_to_send) {
            send_file_over_socket(connfd, dir_sync_from, fs.first);
        }

        // receiving number of files we need to receive
        int num_to_receive;
        recv(connfd, &num_to_receive, sizeof(int), 0);
        for (int i = 0; i < num_to_receive; i++) {
            receive_file_over_socket(connfd, dir_sync_from);
        }

        get_all_files_in_directory(dir_sync_from, dir_sync_from, prev_file_condition);
        sleep(60);
    }

    close(connfd);
    close(listenfd);
}