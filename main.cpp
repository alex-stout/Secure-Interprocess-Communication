#include <iostream>
#include "encryption.hpp"
#include "settings.hpp"

using namespace std;

int main(int argc, char *argv[])
{
    cout << "number of args " << argc << endl;
    // check to see the number of args. It should be over 2 or 3
    if (argc <= 1)
    {
        cout << "No arguements were given. Please refer to the README for instructions on how to use this program.";
        exit(1);
    }
    int type = getType();
    int results = encrypt(5, 5);
    cout << results << endl;
}

int client()
{
    cout << "Running as client.";
}

int server()
{
    cout << "Running as server.";
}