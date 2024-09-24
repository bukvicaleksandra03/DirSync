#ifndef _LOG_FILE_HPP
#define _LOG_FILE_HPP

extern "C" {
#include <assert.h>
}

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

class LogFile {
private:
  fstream log_file;
  string filename;

public:
  LogFile(const string& filename) { this->filename = filename; }

  // Deleting copy and move constructors
  LogFile(const LogFile&) = delete;
  LogFile& operator=(const LogFile&) = delete;
  LogFile(LogFile&&) = delete;
  LogFile& operator=(LogFile&&) = delete;

  ~LogFile() { this->close(); }

  void write(const string& str) {
    assert(log_file.is_open());
    if (log_file.is_open()) {
      log_file << str;
    }
  }

  template <typename T> LogFile& operator<<(const T& value) {
    assert(log_file.is_open());
    if (log_file.is_open()) {
      log_file << value;
    }
    return *this;
  }

  void open() {
    if (!log_file.is_open()) {
      log_file.open(filename, ios_base::out | ios_base::trunc);
    }
  }

  void close() {
    if (log_file.is_open()) {
      log_file.close();
    }
  }

  void write_out_map(unordered_map<string, uint64_t>& um) {
    for (auto& u : um) {
      *this << u.first << " - " << u.second << '\n';
    }
  }

  void write_out_vector(vector<string>& vec) {
    for (auto& u : vec) {
      *this << u << '\n';
    }
  }
};

#endif // _LOG_FILE_HPP