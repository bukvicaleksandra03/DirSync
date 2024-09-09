#include "comp.hpp"

Comp *Comp::instance = nullptr;

Comp::Comp(const string &dir_to_sync)
{
    this->dir_to_sync = dir_to_sync;
    log_file.open(dir_to_sync + '/' + "sync_log.txt", ios::out | ios::app);

    instance = this;

    unordered_set<string> exclude_files;
    exclude_files.insert({"sync_log.txt"});
    exclude_files.insert({"last_sync_state.txt"});


    fsu = new FileSysUtil(dir_to_sync, &log_file, &curr_files, &curr_dirs, &last_sync_state, this, exclude_files);
    sock_util = new SocketsUtil(dir_to_sync, fsu, &log_file, this);
}

Comp::~Comp()  {
    log_file.close();
    delete fsu;
    delete sock_util;
}

void Comp::err_n_die(const char *fmt, ...)
{
    cout << "err_n_die comp" << endl;
    int errno_save;
    va_list ap;

    errno_save = errno;

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    fflush(stdout);

    if (errno_save != 0)
    {
        fprintf(stdout, "(errno = %d) : %s\n", errno_save, strerror(errno_save));
        fprintf(stdout,"\n");
        fflush(stdout);
    }
    va_end(ap);

    if (log_file.is_open()) {
        log_file.close();
    }
    if (last_sync_state.is_open()) {
        last_sync_state.close();
    }

    close(sockfd);
}
