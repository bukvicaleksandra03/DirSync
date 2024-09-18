#include "file_sys_util.hpp"
#include "comp.hpp"

/**
 * @param file_name FULL name of a file inside of base_dir directory.
 *
 * example:
 * string base_dir = "/home/user";
 * cut_out_dir_name("/home/user/Desktop/fff.txt") = "Desktop/fff.txt"
 */
string FileSysUtil::cut_out_dir_name(const string& file_name) 
{
    // Check if base_dir is a prefix of file_name
    if (file_name.compare(0, base_dir.length(), base_dir) != 0 || base_dir.length() >= file_name.length()) {
        *log_file << "cut_out_dir_name: Invalid input\nDirectory: " + base_dir + "\nFile: " + file_name + "\n";
        throw new runtime_error("cut_out_dir_name: Invalid input\nDirectory: " + base_dir + "\nFile: " + file_name + "\n");
    }

    // Return the substring after the directory prefix
    string result = file_name.substr(base_dir.length());

    // If the next character is '/', skip it
    if (!result.empty() && result[0] == '/') {
        result.erase(0, 1);
    }

    return result;
}

/**
 * Places the content of the entire base_dir into files and dirs maps.
 * @return 0 on success
 */
int FileSysUtil::get_dir_content()
{
    dirs->clear();
    files->clear();
    get_dir_content_recursive(base_dir);
    return 0;
}

int FileSysUtil::get_dir_content_recursive(const string& dir_name) 
{
    DIR* dir;
    struct dirent *entry;
    struct stat fileStat;

    dir = opendir(dir_name.c_str());
    if (dir == NULL) throw runtime_error("Unable to open directory " + dir_name);
    

    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        string fname = entry->d_name;
        string filePath = dir_name + '/' + fname;

        // Get file statistics
        if (stat(filePath.c_str(), &fileStat) == -1) {
            perror("stat");
            continue;
        }
        
        // Check if the entry is a regular file or a directory
        if (S_ISREG(fileStat.st_mode)) {
            string s = cut_out_dir_name(filePath);
            if (exclude_files.find(s) == exclude_files.end()) 
                files->insert({s, fileStat.st_mtime});
        }
        else if (S_ISDIR(fileStat.st_mode)) {
            string s = cut_out_dir_name(filePath);
            dirs->insert({s, fileStat.st_mtime});

            // recursive function call
            get_dir_content_recursive(filePath); 
        }

    }

    // Close the directory
    closedir(dir);
    return 0;
}

void FileSysUtil::init_last_sync_state()
{
    last_sync_state->init();
}

/**
 * Gets the current content of the base_dir and writes it to a file last_sync_state.
 * It first clears last_sync_state file, and closes it in the end.
 */
void FileSysUtil::save_dir_content()
{
    get_dir_content();
    last_sync_state->write_content(*dirs, *files);
}

void FileSysUtil::get_last_sync_content(unordered_map<string, uint64_t> *prev_file_condition, 
                                        unordered_map<string, uint64_t> *prev_dir_condition)
{
    last_sync_state->get_content(*prev_dir_condition, *prev_file_condition);
}

/**
 * This function compares two maps (mapping file name to modification time) and returns the list of files that are present in 
 * prev_file_condition and not in curr_file_condition.
 */
vector<string> FileSysUtil::find_deleted(unordered_map<string, uint64_t>* prev_condition, 
                            unordered_map<string, uint64_t>* curr_condition) 
{
    vector<string> deleted_files;
    for (auto& pfc: *prev_condition) {
        if (curr_condition->find(pfc.first) == curr_condition->end()) 
            deleted_files.push_back(pfc.first);
    }
    return deleted_files;
}

/**
 * @param relative_path Name of the file we want to create
 *
 * example: create_necessary_dirs(/home/user, f1/f2/fff.txt)
 * As a result directories "/home/user/f1" and "/home/user/f1/f2" will be created.
 */
void FileSysUtil::create_necessary_dirs(const string& relative_path) 
{
    vector<string> tokens;
    string token;
    for (int i = 0; i < relative_path.size(); i++) {
        if (relative_path[i] != '/') token += relative_path[i];
        else {
            tokens.push_back(token);
            token.clear();
        }
    }

    string dir_to_be_created = base_dir;
    for (auto& token: tokens) {
        dir_to_be_created += '/';
        dir_to_be_created += token;

        // Check if the directory exists, if not create it
        struct stat st;
        if (stat(dir_to_be_created.c_str(), &st) != 0) { // Directory does not exist
            if (mkdir(dir_to_be_created.c_str(), 0755) != 0)  // Create the directory with 0755 permissions
                throw runtime_error("mkdir");
        } else if (!S_ISDIR(st.st_mode)) { // If it's not a directory
            throw runtime_error(dir_to_be_created + " exists but is not a directory\n");
        }  
    }
}

void FileSysUtil::create_dirs(unordered_map<string, uint64_t> *to_be_created)
{
    for (auto& tbc: *to_be_created) 
    {
        string full_path = base_dir + '/' + tbc.first;
        
        create_necessary_dirs(tbc.first);

        // Check if the directory exists, if not create it
        struct stat st;
        if (stat(full_path.c_str(), &st) != 0) { // Directory does not exist
            if (mkdir(full_path.c_str(), 0755) != 0)  // Create the directory with 0755 permissions
                throw runtime_error("mkdir");
        }
    }
}

/**
 * Deletes all the files in directory(dir_name) from to_be_deleted map.
 * It also updates the files map by removing elements when a file is deleted.
 */
void FileSysUtil::delete_files(unordered_map<string, uint64_t> *to_be_deleted)
{
    for (auto& tbd: *to_be_deleted) 
    {
        string full_path = base_dir + '/' + tbd.first;
        int ret = remove(full_path.c_str());
        if (ret == -1) { // file didn't exist or another error occured
            if (errno != ENOENT) 
                throw runtime_error("delete_file, remove: ");
        }
        else if (ret == 0) { // file was successfully deleted
            files->erase(tbd.first);
        }
    }
}

/**
 * Recursively deletes the contents of a directory and the directory itself.
 */
void FileSysUtil::delete_dir_recursively(const string& dir_path) 
{
    DIR* dir = opendir(dir_path.c_str());
    if (!dir) {
        throw runtime_error("opendir failed: ");
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        string entry_name = entry->d_name;

        // Skip "." and ".." entries
        if (entry_name == "." || entry_name == "..") {
            continue;
        }

        string full_path = dir_path + '/' + entry_name;

        struct stat statbuf;
        if (stat(full_path.c_str(), &statbuf) == -1) {
            throw runtime_error("stat failed: ");
        }

        if (S_ISDIR(statbuf.st_mode)) {
            // Entry is a directory, recurse into it
            delete_dir_recursively(full_path);
        } else {
            // Entry is a file, delete it
            if (remove(full_path.c_str()) == -1) {
                throw runtime_error("remove failed: ");
            }
            string file_name = cut_out_dir_name(full_path);
            files->erase(file_name);
        }
    }

    closedir(dir);

    // Finally, remove the empty directory
    if (rmdir(dir_path.c_str()) == -1) {
        throw runtime_error("rmdir failed: ");
    }
    if (base_dir != dir_path) {
        string dir_name = cut_out_dir_name(dir_path);
        dirs->erase(dir_name);
    }
}

/**
 * Deletes all the directories in directory(dir_name) from to_be_deleted map.
 * It also updates the dirs and files map by removing elements when a directory is deleted.
 */
void FileSysUtil::delete_dirs(unordered_map<string, uint64_t> *to_be_deleted)
{
    for (auto& tbd: *to_be_deleted) 
    {
        string full_path = base_dir + '/' + tbd.first;
        
        struct stat statbuf;
        if (stat(full_path.c_str(), &statbuf) == -1) {
            if (errno != ENOENT) {
                throw runtime_error("stat failed: ");
            }
            continue; // Skip if directory doesn't exist
        }

        if (S_ISDIR(statbuf.st_mode)) {
            // Entry is a directory, delete it recursively
            delete_dir_recursively(full_path);
        }
        dirs->erase(tbd.first);
    }
}

/**
 * Returns a list of files we need from the server in order to synchronize.
 */
vector<string> FileSysUtil::out_of_date_files(unordered_map<string, uint64_t> *to_check)
{
    vector<string> ret;
    for (auto& tc: *to_check) {
        string full_path = base_dir + '/' + tc.first;
        
        struct stat file_stat;
        if (stat(full_path.c_str(), &file_stat) == 0) {
            uint64_t current_mod_time = file_stat.st_mtime;
            if (current_mod_time < tc.second) {
                ret.push_back(tc.first);
            }
        } else {
            ret.push_back(tc.first);
        }
    }

    return ret;
}

/**
 * Returns a list of files that are newer on the client than on the server.
 * (That have been changed or created more recently)
 */
vector<string> FileSysUtil::newer_files(unordered_map<string, uint64_t> *to_check)
{
    vector<string> ret;
    for (auto& cf: *files) {
        if (to_check->find(cf.first) == to_check->end()) {
            ret.push_back(cf.first);
        }
    }
    for (auto& tc: *to_check) {
        string full_path = base_dir + '/' + tc.first;
        
        struct stat file_stat;
        if (stat(full_path.c_str(), &file_stat) == 0) {
            uint64_t current_mod_time = file_stat.st_mtime;
            if (current_mod_time >= tc.second) {
                ret.push_back(tc.first);
            }
        } 
    }

    return ret;
}