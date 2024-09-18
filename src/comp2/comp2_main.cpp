#include "comp2.hpp"


int main(int argc, char** argv)
{
    // Check if the directory argument is provided
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <directory_to_sync>" << endl;
        return 1;
    }

    string dir_to_sync = argv[1];

    Comp2* c2 = new Comp2(dir_to_sync);
    c2->listen_for_connection();
    c2->connect_and_run();

    return 0;
}