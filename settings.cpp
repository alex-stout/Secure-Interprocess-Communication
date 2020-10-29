#include <iostream>

using namespace std;

int getType()
{
    int type;
    std::cout << "Please choose the type of program you want to start. (1 or 2)" << std::endl;
    std::cout << "1. Server (recieve files)" << std::endl;
    std::cout << "2. Client (send files)" << std::endl;
    std::cout << "Input: ";
    std::cin >> type;
    return type;
}