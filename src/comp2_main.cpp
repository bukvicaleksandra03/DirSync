#include "comp2.hpp"

int main(int argc, char** argv)
{
    string dir_to_sync = "/home/aleksandra/sync1";

    Comp2* c2 = new Comp2(dir_to_sync);
    c2->listen_for_connection();
    c2->connect_and_run();

    return 0;
}