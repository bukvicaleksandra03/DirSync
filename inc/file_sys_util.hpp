#ifndef _FILE_SYS_UTIL_HPP
#define _FILE_SYS_UTIL_HPP

#include "common.hpp"
class Comp;
class FileSysUtil {
private:
    ofstream* log_file;
    fstream* last_sync_state;
    unordered_set<string> exclude_files;
    Comp* comp;

    string base_dir;
    unordered_map<string, uint64_t>* files;
    unordered_map<string, uint64_t>* dirs;

    void delete_dir_recursively(const string& dir_path);

    void init_last_sync_state();

public:
    FileSysUtil(const string& dir_name, ofstream* log_file,
                unordered_map<string, uint64_t>* files,
                unordered_map<string, uint64_t>* dirs,
                fstream* last_sync_state, Comp* comp, unordered_set<string> ef) 
    {
        this->base_dir = dir_name;
        this->log_file = log_file;
        this->last_sync_state = last_sync_state;
        init_last_sync_state();
        this->files = files;
        this->dirs = dirs;
        this->comp = comp;
        this->exclude_files = ef;
    }

    string cut_out_dir_name(const string& file_name);

    int get_dir_content();

    int get_dir_content_recursive(const string& dir_name);

    void save_dir_content();

    void get_last_sync_content(unordered_map<string, uint64_t> *prev_file_condition,
                                unordered_map<string, uint64_t> *prev_dir_condition);

    void create_necessary_dirs(const string& relative_path);

    void create_dirs(unordered_map<string, uint64_t> *to_be_created);

    void delete_files(unordered_map<string, uint64_t> *to_be_deleted);
                    
    void delete_dirs(unordered_map<string, uint64_t> *to_be_deleted);

    vector<string> out_of_date_files(unordered_map<string, uint64_t> *to_check);

    vector<string> newer_files(unordered_map<string, uint64_t> *to_check);

    static vector<string> find_deleted(unordered_map<string, uint64_t>* prev_condition, 
                                    unordered_map<string, uint64_t>* curr_condition);

};

#endif