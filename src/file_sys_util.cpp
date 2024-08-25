#include "file_sys_util.hpp"

/**
 * @param dir_name Name of the directory.
 * @param file_name FULL name of a file inside of that directory.
 * @return The name of the file without the directory prefix.
 *
 * @example
 * string result = cut_out_dir_name("/home/user", "/home/user/Desktop/fff.txt");
 * result will be "Desktop/fff.txt"
 */
string cut_out_dir_name(const string& dir_name, const string& file_name) {
    // Check if dir_name is a prefix of file_name
    if (file_name.compare(0, dir_name.length(), dir_name) != 0 || dir_name.length() >= file_name.length()) {
        err_n_die("cut_out_dir_name: Invalid input\nDirectory: %s\nFile: %s\n", dir_name, file_name);
    }

    // Return the substring after the directory prefix
    string result = file_name.substr(dir_name.length());

    // If the next character is '/', skip it
    if (!result.empty() && result[0] == '/') {
        result.erase(0, 1);
    }

    return result;
}

/**
 * @param original_dir_name In the beginning original_dir_name and dir_name are the same, but with each recursive call dir_name is changed.
 * and original_dir_name stays the same.
 * @param dir_name Name of the directory from which we are getting files.
 * @param files An unordered map in which files are being placed. File name and unix timestamp are being placed.
 * 
 * This function is recursive, it will also return files from directories inside of this directory.
 * 
 * @return 0 on success
 */
int get_all_files_in_directory(const string& original_dir_name, const string& dir_name, unordered_map<string, uint64_t>& files) 
{
    DIR* dir;
    struct dirent *entry;
    struct stat fileStat;

    dir = opendir(dir_name.c_str());
    if (dir == NULL) err_n_die("Unable to open directory");

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
            string s = cut_out_dir_name(original_dir_name, filePath);
            files.insert({s, fileStat.st_mtime});
        }
        else if (S_ISDIR(fileStat.st_mode)) 
            get_all_files_in_directory(original_dir_name, filePath, files); // recursive function call
        else 
            printf("%s is neither a file nor a directory (could be a symbolic link, etc.)\n", entry->d_name);
        
    }

    // Close the directory
    closedir(dir);
    return 0;
}

/**
 * This function compares two maps (mapping file name to modification time) and returns the list of files that are present in 
 * prev_file_condition and not in curr_file_condition.
 */
vector<string> find_deleted_files(unordered_map<string, uint64_t>& prev_file_condition, unordered_map<string, uint64_t>& curr_file_condition) {
    vector<string> deleted_files;
    for (auto& pfc: prev_file_condition) {
        if (curr_file_condition.find(pfc.first) == curr_file_condition.end()) 
            deleted_files.push_back(pfc.first);
    }
    return deleted_files;
}

/**
 * @param base_path Name of the directory in which to create all necessary directories.
 * @param relative_path Name of the file we want to create
 *
 * example: create_necessary_directories(/home/user, f1/f2/fff.txt)
 * As a result directories "/home/user/f1" and "/home/user/f1/f2" will be created.
 */
void create_necessary_directories(const string& base_path, const string& relative_path) 
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

    string dir_to_be_created = base_path;
    for (auto& token: tokens) {
        dir_to_be_created += '/';
        dir_to_be_created += token;

        // Check if the directory exists, if not create it
        struct stat st;
        if (stat(dir_to_be_created.c_str(), &st) != 0) { // Directory does not exist
            if (mkdir(dir_to_be_created.c_str(), 0755) != 0)  // Create the directory with 0755 permissions
                err_n_die("mkdir");
        } else if (!S_ISDIR(st.st_mode)) { // If it's not a directory
            err_n_die("%s exists but is not a directory\n", dir_to_be_created);
        }
    }
}

/**
 * Deletes all the files in directory(dir_name) from to_be_deleted map.
 * It also updates the files map by removing elements when a file is deleted.
 */
void delete_files(const string& dir_name, unordered_map<string, uint64_t> &to_be_deleted, unordered_map<string, uint64_t> &files)
{
    for (auto& tbd: to_be_deleted) 
    {
        string full_path = dir_name + '/' + tbd.first;
        int ret = remove(full_path.c_str());
        if (ret == -1) { // file didn't exist or another error occured
            if (errno != ENOENT) 
                err_n_die("delete_file, remove: ");
        }
        else if (ret == 0) { // file was successfully deleted
            files.erase(tbd.first);
        }
    }
}

/**
 * For each of the files in "to_be_checked" this functions adds that file to the list
 * if there is no file with that name in dir_name or a file like that exists but is out of date.
 * Returns a list of files we need from the server in order to synchronize.
 */
vector<string> out_of_date_files(const string &dir_name, unordered_map<string, uint64_t> &to_check)
{
    vector<string> ret;
    for (auto& tc: to_check) {
        string full_path = dir_name + '/' + tc.first;
        
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
vector<string> newer_files(const string &dir_name, unordered_map<string, uint64_t>& curr_files, unordered_map<string, uint64_t> &to_check)
{
    vector<string> ret;
    for (auto& cf: curr_files) {
        if (to_check.find(cf.first) == to_check.end()) {
            ret.push_back(cf.first);
        }
    }
    for (auto& tc: to_check) {
        string full_path = dir_name + '/' + tc.first;
        
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