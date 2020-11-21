#include <iostream>
#include "encryption.hpp"
#include "settings.hpp"

using namespace std;

// intialize global setting for verbosity as 0, if enabled it is set to 1
int g_verbose = 0;

int client()
{
    cout << "Running as client." << endl;
    return 0;
}

int server()
{
    cout << "Running as server." << endl;
    int serve_sock, client_sock, client_addr_size;
    return 0;
}

// handle the intial setup
int main(int argc, char *argv[])
{
    cout << "number of args " << argc << endl;
    // check to see the number of args. It should be over 2 or 3
    if (argc <= 1)
    {
        cout << "No arguements were given. Please refer to the README for instructions on how to use this program." << endl;
        exit(1);
    }
    else if (argc > 3)
    {
        cout << "Too many arguements were given. Please refer to the README for instructions on how to use this program." << endl;
    }
    else if (argc == 3) // if there's three arguments it means they are including a verbose mode
    {
        if (strcmp(argv[2], "-v") == 0 | strcmp(argv[2], "--verbose") == 0)
        {
            g_verbose = 1;
            exit(1);
        }
        else
        {
            cout << "Unrecognized arguement given. Please refer to the README for instructions on how to use this program." << endl;
            exit(1);
        }
    }

    // verify the vailidty of the mode setting then set the mode
    if (strcmp(argv[1], "client") == 0)
    {
        client();
    }
    else if (strcmp(argv[1], "server") == 0)
    {
        server();
        system("md5 test.txt");
    }
    else
    {
        cout << "Unrecognized mode set. Please refer to the README for instructions on how to use this program." << endl;
        exit(1);
    }
    return 0;
}
