#ifndef _FILE_SYS_UTIL_HPP
#define _FILE_SYS_UTIL_HPP

#include "common.hpp"

string cut_out_dir_name(const string& dir_name, const string& file_name);

int get_all_files_in_directory(const string& original_dir_name, const string& dir_name, unordered_map<string, uint64_t>& files);

vector<string> find_deleted_files(unordered_map<string, uint64_t>& prev_file_condition, unordered_map<string, uint64_t>& curr_file_condition);

void create_necessary_directories(const string& base_path, const string& relative_path);

void delete_files(const string& dir_name, unordered_map<string, uint64_t>& to_be_deleted, unordered_map<string, uint64_t>& files);

vector<string> out_of_date_files(const string& dir_name, unordered_map<string, uint64_t>& to_check);

vector<string> newer_files(const string &dir_name, unordered_map<string, uint64_t>& curr_files, unordered_map<string, uint64_t> &to_check);

#endif