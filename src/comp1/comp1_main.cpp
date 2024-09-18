#include "comp1.hpp"

int main(int argc, char** argv)
{
    // Check if the correct number of arguments are provided
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <directory_to_sync> <ip_address>" << endl;
        return 1;
    }

    string dir_to_sync = argv[1];
    string ip_addr = argv[2];

    Comp1* c1 = new Comp1(dir_to_sync, ip_addr);
    c1->connect_and_run();

    return 0;
}