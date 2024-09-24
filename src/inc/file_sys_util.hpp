#ifndef _FILE_SYS_UTIL_HPP
#define _FILE_SYS_UTIL_HPP

extern "C" {
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
}

#include <filesystem>
#include <unordered_set>

#include "file_state_file.hpp"
#include "log_file.hpp"
class Comp;

class FileSysUtil {
private:
  std::shared_ptr<LogFile> log_file;
  std::shared_ptr<FileStateFile> last_sync_state;

  std::unordered_set<std::string> exclude_files;

  std::string base_dir;
  std::unordered_map<std::string, uint64_t>* files;
  std::unordered_map<std::string, uint64_t>* dirs;

  void delete_dir_recursively(const std::string& dir_path);

  void init_last_sync_state();

public:
  FileSysUtil(const std::string& dir_name,
              const std::shared_ptr<LogFile>& log_file,
              const std::shared_ptr<FileStateFile>& lss,
              std::unordered_map<std::string, uint64_t>* files,
              std::unordered_map<std::string, uint64_t>* dirs,
              std::unordered_set<std::string> ef) {
    this->base_dir = dir_name;
    this->log_file = log_file;
    this->last_sync_state = lss;
    init_last_sync_state();
    this->files = files;
    this->dirs = dirs;
    this->exclude_files = ef;
  }

  std::string cut_out_dir_name(const std::string& file_name);

  int get_dir_content();

  int get_dir_content_recursive(const std::string& dir_name);

  void save_dir_content();

  void get_last_sync_content(
      std::unordered_map<std::string, uint64_t>* prev_file_condition,
      std::unordered_map<std::string, uint64_t>* prev_dir_condition);

  void create_necessary_dirs(const std::string& relative_path);

  void create_dirs(std::unordered_map<std::string, uint64_t>* to_be_created);

  void delete_files(std::unordered_map<std::string, uint64_t>* to_be_deleted);

  void delete_dirs(std::unordered_map<std::string, uint64_t>* to_be_deleted);

  std::vector<std::string>
  out_of_date_files(std::unordered_map<std::string, uint64_t>* to_check);

  std::vector<std::string>
  newer_files(std::unordered_map<std::string, uint64_t>* to_check);

  static std::vector<std::string>
  find_deleted(std::unordered_map<std::string, uint64_t>* prev_condition,
               std::unordered_map<std::string, uint64_t>* curr_condition);
};

#endif