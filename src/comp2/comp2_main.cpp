
#include "comp2.hpp"

int main(int argc, char** argv) {
  // Check if the directory argument is provided
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <directory_to_sync> <port_num>\n";
    return 1;
  }

  const std::string dir_to_sync = argv[1];
  int port_num = atoi(argv[2]);

  Comp2* c2 = new Comp2(dir_to_sync, port_num);
  c2->listen_for_connection();
  c2->connect_and_run();

  return 0;
}