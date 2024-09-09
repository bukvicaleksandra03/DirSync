#include "comp1.hpp"


int main(int argc, char** argv)
{
    string dir_to_sync = "/home/aleksandra/sync2";
    string ip_addr = "127.0.0.0";

    Comp1* c1 = new Comp1(dir_to_sync, ip_addr);
    c1->connect_and_run();

    return 0;
}