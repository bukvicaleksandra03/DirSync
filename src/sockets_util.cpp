#include "sockets_util.hpp"

ssize_t SocketsUtil::readn(int fd, void* buf, size_t n) {
  ssize_t num_read = 0; // number of bytes fetched by last read
  ssize_t tot_read = 0; // total number of bytes read so far
  char* buffer = static_cast<char*>(buf);

  for (tot_read = 0; tot_read < n;) {
    num_read = read(fd, buffer, n - tot_read);

    if (num_read == 0) { // EOF
      throw SocketError();
    }
    if (num_read == -1) { // error
      if (errno == EINTR || errno == ECONNRESET || errno == EPIPE) {
        throw SocketError();
      } else {
        throw runtime_error("readn error, errno = " + std::to_string(errno));
      }
    }
    tot_read += num_read;
    buffer += num_read;
  }

  return tot_read;
}

ssize_t SocketsUtil::writen(int fd, const void* buf, size_t n) {
  ssize_t num_written; // number of bytes written in last write
  ssize_t tot_written; // total number of written read so far
  const char* buffer = static_cast<const char*>(buf);

  for (tot_written = 0; tot_written < n;) {
    num_written = write(fd, buffer, n - tot_written);

    if (num_written == -1) {
      if (errno == EINTR || errno == ECONNRESET || errno == EPIPE) {
        throw SocketError();
      } else {
        throw runtime_error("writen error");
      }
    }
    tot_written += num_written;
    buffer += num_written;
  }

  return tot_written;
}

void SocketsUtil::send_fname_and_modif_time(const string& file_name,
                                            uint64_t modif_time) {
  assert(socket_fd_initialized == true);

  const char* name = file_name.c_str();

  // Send name size
  size_t name_size = strlen(name);
  writen(socket_fd, &name_size, sizeof(size_t));
  // Send name
  writen(socket_fd, name, name_size);
  // Send modification time
  writen(socket_fd, &modif_time, sizeof(uint64_t));
}

void SocketsUtil::receive_fname_and_modif_time(
    unordered_map<string, uint64_t>& files) {
  assert(socket_fd_initialized == true);

  size_t name_size = 0;
  char* file_name = nullptr;
  uint64_t modif_time = 0;

  // Receiving name size and allocating memory
  readn(socket_fd, &name_size, sizeof(size_t));
  file_name = static_cast<char*>(malloc(name_size + 1));
  // Receiving file name
  readn(socket_fd, file_name, name_size);
  file_name[name_size] = '\0';
  // Receiving modification time
  readn(socket_fd, &modif_time, sizeof(uint64_t));
  string fname = file_name;
  files.insert({fname, modif_time});

  free(file_name);
}

void SocketsUtil::send_file_over_socket(const string& file_name) {
  assert(socket_fd_initialized == true);

  struct stat obj;

  string full_name = base_dir + '/' + file_name;
  stat(full_name.c_str(), &obj);

  // Send name size
  int name_size = file_name.length();
  writen(socket_fd, &name_size, sizeof(int));
  // Send name
  writen(socket_fd, file_name.c_str(), name_size);

  // Open file
  int file_desc = open(full_name.c_str(), O_RDONLY);
  if (file_desc < 0)
    throw runtime_error("File " + full_name + " could not be opened.");
  // Send file size
  int file_size = obj.st_size;
  writen(socket_fd, &file_size, sizeof(int));
  // Send file data in a loop
  off_t offset = 0;
  int remaining = file_size;
  while (remaining > 0) {
    int sent = sendfile(socket_fd, file_desc, &offset, remaining);
    if (sent <= 0) {
      throw SocketError();
    }
    remaining -= sent;
  }
}

void SocketsUtil::receive_file_over_socket() {
  assert(socket_fd_initialized);

  // Getting File
  int file_size;
  int name_size;
  char* file_name_c;
  char* data;
  // Receiving name size and allocating memory
  readn(socket_fd, &name_size, sizeof(int));

  // Receiving file name
  file_name_c = static_cast<char*>(malloc(name_size + 1));
  readn(socket_fd, file_name_c, name_size);
  file_name_c[name_size] = '\0';
  string file_name = file_name_c;
  // Recieving file size and allocating memory
  readn(socket_fd, &file_size, sizeof(int));
  data = static_cast<char*>(malloc(file_size + 1));
  // Creating a new file, receiving and storing data in the file.
  string full_name = base_dir + '/' + file_name;

  fsu->create_necessary_dirs(file_name);

  FILE* fp = fopen(full_name.c_str(), "w");

  readn(socket_fd, data, file_size);
  data[file_size] = '\0';
  fwrite(data, 1, file_size, fp);

  fclose(fp);

  free(file_name_c);
  free(data);
}