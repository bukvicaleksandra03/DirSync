#include "sockets_util.hpp"
#include "file_sys_util.hpp"

/**
 * @param socket_fd Socket file descriptor.
 * @param name Name of the file relative to the directory being synchronized.
 * @param modif_time Last modified time of that file. (in UNIX time)
 */
void send_fname_and_modif_time(int socket_fd, const string& file_name, uint64_t modif_time)
{
    const char* name = file_name.c_str();
    // Send name size
    int name_size = strlen(name);
    //printf("Sending name: %s with size %d\n", name, name_size);
    write(socket_fd, &name_size, sizeof(int));
    // Send name
    write(socket_fd, name, name_size);
    // Send modification time
    write(socket_fd, &modif_time, sizeof(uint64_t));
}

/**
 * @param socket_fd Socket file descriptor.
 * @param files An unordered map in which files that are received are being placed. 
 * File name and last modification time of that file (as a UNIX timestamp) are being placed.
 */
void receive_fname_and_modif_time(int socket_fd, unordered_map<string, uint64_t>& files)
{
    int name_size;
    char *file_name;
    uint64_t modif_time;
    // Receiving name size and allocating memory
    recv(socket_fd, &name_size, sizeof(int), 0);
    file_name = (char*)malloc(name_size+1);
    // Receiving file name
    recv(socket_fd, file_name, name_size, 0);
    file_name[name_size] = '\0';
    // Receiving modification time
    recv(socket_fd, &modif_time, sizeof(uint64_t), 0);

    string fname = file_name;
    files.insert({fname, modif_time});
}

/**
 * @param socket_fd Socket file descriptor.
 * @param dir_name Name of the directory we are synchronizing from.
 * @param file_name Name of the file relative to the directory with name dir_name.
 *
 * @example
 * send_file_over_socket(sockedfd, /home/user, Desktop/fff.txt); 
 *      - through the socket first send the name size (sizeof("Desktop/fff.txt")),
 *      - than send the name ("Desktop/fff.txt"), next send the size of the file,
 *      - finally send the entire file
 */
void send_file_over_socket(int socket_fd, const string& dir_name, const string& file_name)
{
	struct stat	obj;

    string full_name = dir_name + '/' + file_name;
	stat(full_name.c_str(), &obj);

    // Send name size
    int name_size = file_name.length();
    write(socket_fd, &name_size, sizeof(int));
    // Send name
    write(socket_fd, file_name.c_str(), name_size);

    // Open file
	int file_desc = open(full_name.c_str(), O_RDONLY);
	// Send file size
	int file_size = obj.st_size;
	write(socket_fd, &file_size, sizeof(int));
    // Send file data in a loop
    off_t offset = 0;
    int remaining = file_size;
    while (remaining > 0) {
        int sent = sendfile(socket_fd, file_desc, &offset, remaining);
        if (sent <= 0) {
            perror("Error sending file");
            close(file_desc);
            return;
        }
        remaining -= sent;
    }
}

/**
 * Receive a file being sent with function send_file_over_socket and place
 * it in a directory dir_name.
 * 
 * @param socket_fd Socket file descriptor.
 * @param dir_name Name of the directory we are synchronizing to.
 */
void receive_file_over_socket(int socket_fd, const string& dir_name) {
    // Getting File 
	int file_size;
    int name_size;
    char *file_name_c;
	char *data;
    // Receiving name size and allocating memory
    recv(socket_fd, &name_size, sizeof(int), 0);
    file_name_c = (char*)malloc(name_size+1);
    // Receiving file name
    recv(socket_fd, file_name_c, name_size, 0);
    file_name_c[name_size] = '\0';
    string file_name = file_name_c;
    free(file_name_c);
	// Recieving file size and allocating memory
	recv(socket_fd, &file_size, sizeof(int), 0);
	data = (char*)malloc(file_size+1);
	// Creating a new file, receiving and storing data in the file.
    string full_name = dir_name + '/' + file_name;

    create_necessary_directories(dir_name, file_name);

	FILE *fp = fopen(full_name.c_str(), "w");

    int remaining = file_size;
    size_t total_received = 0;
    while (remaining > 0) {
        size_t r = recv(socket_fd, data + total_received, remaining, 0);
        if (r <= 0) {
            perror("recv");
            fclose(fp);
            free(data);
            exit(EXIT_FAILURE);
        }
        total_received += r;
        remaining -= r;
    }
    data[total_received] = '\0'; 
    fwrite(data, 1, total_received, fp);

	fclose(fp);
}