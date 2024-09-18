#include <string>
#include <iostream>
using namespace std;
void PrintMap();

int main(int argc, char *argv[]) {
  if (argc < 2 || argv[1] == std::string("1")) {
    PrintMap();
    cout << "Print map test successfull!" << endl;
  }


}
