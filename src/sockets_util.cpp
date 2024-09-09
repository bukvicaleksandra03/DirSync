#include "sockets_util.hpp"
#include "file_sys_util.hpp"
#include "comp.hpp"

void SocketsUtil::send_fname_and_modif_time(const string &file_name, uint64_t modif_time)
{
    assert(socket_fd_initialized == true);
    
    const char* name = file_name.c_str();

    // Send name size
    size_t name_size = strlen(name);
    write(socket_fd, &name_size, sizeof(size_t));
    // Send name
    write(socket_fd, name, name_size);
    // Send modification time
    write(socket_fd, &modif_time, sizeof(uint64_t));
    // Logging
    log_file->seekp(0, std::ios::end);
    *log_file << "Sending file: " << file_name << " with mod time: " << modif_time  << " name size: " << name_size << endl;
}

void SocketsUtil::receive_fname_and_modif_time(unordered_map<string, uint64_t>& files)
{
    assert(socket_fd_initialized == true);

    size_t name_size;
    char *file_name;
    uint64_t modif_time;

    // Receiving name size and allocating memory
    recv(socket_fd, &name_size, sizeof(size_t), 0);
    file_name = (char*)malloc(name_size+1);
    // Receiving file name
    recv(socket_fd, file_name, name_size, 0);
    file_name[name_size] = '\0';
    // Receiving modification time
    recv(socket_fd, &modif_time, sizeof(uint64_t), 0);
    string fname = file_name;
    files.insert({fname, modif_time});
    // Logging
    log_file->seekp(0, std::ios::end);
    *log_file << "Recieving file: " << fname << " with mod time: " << modif_time << " name size: " << name_size << endl;
}

void SocketsUtil::send_file_over_socket(const string& file_name)
{
    assert(socket_fd_initialized == true);

	struct stat	obj;

    string full_name = base_dir + '/' + file_name;
	stat(full_name.c_str(), &obj);

    // Send name size
    int name_size = file_name.length();
    write(socket_fd, &name_size, sizeof(int));
    // Send name
    write(socket_fd, file_name.c_str(), name_size);

    // Open file 
	int file_desc = open(full_name.c_str(), O_RDONLY);
    if (file_desc < 0) comp->err_n_die("File %s could not be opened.\n", full_name.c_str());
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

void SocketsUtil::receive_file_over_socket() {
    assert(socket_fd_initialized == true);

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
    string full_name = base_dir + '/' + file_name;

    fsu->create_necessary_dirs(file_name);

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