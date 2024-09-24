#ifndef _FILE_STATE_FILE_HPP
#define _FILE_STATE_FILE_HPP

extern "C" {
#include <assert.h>
}

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

class FileStateFile {
private:
  std::fstream file;
  std::string filename;

public:
  FileStateFile(const std::string& filename) { this->filename = filename; }

  void init() {
    file.open(filename, std::ios_base::out | std::ios_base::trunc);
    file << "DIRS:\n";
    file << "FILES:\n";
    file.close();
  }

  void write_content(const std::unordered_map<std::string, uint64_t>& dirs,
                     const std::unordered_map<std::string, uint64_t>& files) {
    // Open the file in truncation mode to clear its contents
    file.open(filename, std::ios_base::out | std::ios_base::trunc);

    file << "DIRS:\n";
    for (auto& dir : dirs) {
      file << dir.first << ' ' << dir.second << '\n';
    }
    file << "FILES:\n";
    for (auto& f : files) {
      file << f.first << ' ' << f.second << '\n';
    }

    file.close();
  }

  void get_content(std::unordered_map<std::string, uint64_t>& dirs,
                   std::unordered_map<std::string, uint64_t>& files) {
    dirs.clear();
    files.clear();

    // Open the file in input mode to read its contents
    file.open(filename, std::ios::in);

    std::string line;
    bool is_parsing_dirs = false;
    bool is_parsing_files = false;

    while (getline(file, line)) {
      if (line == "DIRS:") {
        is_parsing_dirs = true;
        is_parsing_files = false;
      } else if (line == "FILES:") {
        is_parsing_dirs = false;
        is_parsing_files = true;
      } else if (is_parsing_dirs) {
        // Assume directories are saved as "dir_name dir_timestamp" format
        std::istringstream iss(line);
        std::string dir_name;
        uint64_t dir_timestamp;
        iss >> dir_name >> dir_timestamp;
        dirs[dir_name] = dir_timestamp;
      } else if (is_parsing_files) {
        // Assume files are saved as "file_name file_timestamp" format
        std::istringstream iss(line);
        std::string file_name;
        uint64_t file_timestamp;
        iss >> file_name >> file_timestamp;
        files[file_name] = file_timestamp;
      }
    }

    file.close();
  }

  // Deleting copy and move constructors
  FileStateFile(const FileStateFile&) = delete;
  FileStateFile& operator=(const FileStateFile&) = delete;
  FileStateFile(FileStateFile&&) = delete;
  FileStateFile& operator=(FileStateFile&&) = delete;

  ~FileStateFile() {
    if (file.is_open()) file.close();
  }
};

#endif // _FILE_STATE_FILE_HPP