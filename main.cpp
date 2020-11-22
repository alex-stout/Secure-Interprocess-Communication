#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "encryption.hpp"
#include "settings.hpp"
#include "cstring"

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
    // intialize the variables to hold the input values
    int port;
    string ip;
    string outFile;
    string key;

    // socket variables
    int sockfd;
    sockaddr_in address;

    cout << "Running as server." << endl;
    cout << "------ SERVER SETUP -------" << endl;
    cout << "Connect to IP address: ";
    cin >> ip;
    cout << endl
         << "Port #: ";
    cin >> port;
    // if the port is outside the acceptable range, abort the program
    if (port<9000 | port> 9999)
    {
        cout << "Port " << port << " out of range. Please refer to the README for acceptable port ranges." << endl;
        exit(1);
    }
    cout << "Save file to (default stdout): ";
    cin >> outFile;
    cout << endl
         << endl
         << "Enter the encryption key: ";
    cin >> key;

    cout << "======= SETTINGS ======" << endl;
    cout << "IP address: " << ip << endl;
    cout << "Output: " << outFile << endl;
    cout << "Encryption key: " << key << endl;

    cout << "Setting up the socket..." << endl;

    // create the fd for the socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        cout << "Setting up the socket failed." << endl;
        exit(1);
    };

    address.sin_family = AF_INET;
    address.sin_port = port;
    address.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to the setting so that it can actually listen
    if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        cout << "There was a problem binding socket to port " << port << endl;
        exit(1);
    };

    // listen on the bound socket and accept a maxiumum of 5 concurrent connections
    listen(sockfd, 5);

    close(sockfd);
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
    }
    else
    {
        cout << "Unrecognized mode set. Please refer to the README for instructions on how to use this program." << endl;
        exit(1);
    }
    return 0;
}
