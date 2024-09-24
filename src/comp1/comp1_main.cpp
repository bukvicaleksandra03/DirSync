
#include "comp1.hpp"

int main(int argc, char** argv) {
  // Check if the correct number of arguments are provided
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <directory_to_sync> <ip_address>\n";
    return 1;
  }

  const std::string dir_to_sync = argv[1];
  const std::string ip_addr = argv[2];

  Comp1* c1 = new Comp1(dir_to_sync, ip_addr);
  c1->connect_and_run();

  return 0;
}