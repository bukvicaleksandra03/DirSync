#include "common.hpp"
#include "file_sys_util.hpp"
#include "sockets_util.hpp"

int main(int argc, char** argv) 
{
    // Establishing connection
    int sockfd;
    struct sockaddr_in servaddr;

    if (argc != 2) 
        err_n_die("usage: %s <server address>", argv[0]);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_n_die("Error while creating the socket!");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        err_n_die("inet_pton error for %s", argv[1]);

    if (connect(sockfd, (SA*)& servaddr, sizeof(servaddr)) < 0)
        err_n_die("connect failed!");

    string dir_sync_to = "/home/aleksandra/sync2";

    // we use this to check which files have been deleted since the last time we synced
    unordered_map<string, uint64_t> prev_file_condition;

    // Connection has been established, now we synchronize every 5 minutes
    while (true)
    {
        // maps the name of the file to the time of modification (in Unix time, seconds from 1970)
        unordered_map<string, uint64_t> all_curr_files;
        get_all_files_in_directory(dir_sync_to, dir_sync_to, all_curr_files);

        // synchronizing file deletion
        vector<string> my_deleted_files = find_deleted_files(prev_file_condition, all_curr_files);
        int server_num_del_files;
        recv(sockfd, &server_num_del_files, sizeof(int), 0);
        unordered_map<string, uint64_t> serv_del_files;
        for (int i = 0; i < server_num_del_files; i++) {
            receive_fname_and_modif_time(sockfd, serv_del_files);
        }
        delete_files(dir_sync_to, serv_del_files, all_curr_files);
        int num_del_files = my_deleted_files.size();
        write(sockfd, &num_del_files, sizeof(int));
        for (string& df: my_deleted_files) {
            send_fname_and_modif_time(sockfd, df, 0);
        }
        
        // receiving file list and modification times
        int num_files;
        recv(sockfd, &num_files, sizeof(int), 0);
        unordered_map<string, uint64_t> received_files;
        for (int i = 0; i < num_files; i++) 
            receive_fname_and_modif_time(sockfd, received_files);

        vector<string> out_of_date = out_of_date_files(dir_sync_to, received_files);
        vector<string> newer_here = newer_files(dir_sync_to, all_curr_files, received_files);

        // sending names of files we want to receive
        int num_to_receive = out_of_date.size();
        write(sockfd, &num_to_receive, sizeof(int));
        for (auto& s: out_of_date) 
            send_fname_and_modif_time(sockfd, s, 0);
        // receiving those files
        for (int i = 0; i < num_to_receive; i++) 
            receive_file_over_socket(sockfd, dir_sync_to);
        

        // sending number of files we want to send
        int num_to_send = newer_here.size();
        write(sockfd, &num_to_send, sizeof(int));
        // sending those files
        for (int i = 0; i < num_to_send; i++) {
            send_file_over_socket(sockfd, dir_sync_to, newer_here[i]);
        }

        get_all_files_in_directory(dir_sync_to, dir_sync_to, prev_file_condition);
        sleep(60);
    }

    close(sockfd);
    exit(0);
}