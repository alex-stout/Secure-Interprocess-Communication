#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "encryption.hpp"
#include "settings.hpp"
#include "cstring"

using namespace std;

// intialize global setting for verbosity as 0, if enabled it is set to 1
int g_verbose = 0;

int client()
{
    // get the file opeend and ready to send.
    FILE *inputFile;
    char buff[1];
    inputFile = fopen("./test.txt", "r");
    // make sure our cursor is at the start of the file
    fseek(inputFile, 0, SEEK_SET);

    // hardcode the port for now... will set this later in the settings
    int port = 9500;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char data[1] = {'L'};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cout << "There was a problem creating the socket." << endl;
        exit(1);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        cout << "Hmmm this ip doesn't looks right." << endl;
        exit(1);
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cout << "Connection failed" << endl;
        exit(1);
    }
    while (fread(buff, sizeof(buff), 1, inputFile) > 0)
    {
        send(sock, buff, sizeof(buff), 0);
    }

    fclose(inputFile);

    // apply XOR cypher encryption to the data
    data[0] = data[0] ^ 'w';
    cout << sizeof(data) << endl;
    cout << "Cyphered data: " << hex << (int)data[0] << endl;
    send(sock, data, sizeof(data), 0);
    cout << "Running as client." << endl;
    return 0;
}

int server()
{
    // intialize the variables to hold the input values
    int port = 9500;
    string ip;
    string outFile;
    string key;
    char buffer[10];
    cout << "charsize: " << sizeof(buffer) << endl;

    // socket variables
    int sockfd, new_socket;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr, valread;

    cout << "Running as server." << endl;
    cout << "------ SERVER SETUP -------" << endl;
    cout << "Connect to IP address: ";
    //cin >> ip;
    cout << endl
         << "Port #: ";
    //cin >> port;
    // if the port is outside the acceptable range, abort the program
    if (port<9000 | port> 9999)
    {
        cout << "Port " << port << " out of range. Please refer to the README for acceptable port ranges." << endl;
        exit(1);
    }
    cout << "Save file to (default stdout): ";
    //cin >> outFile;
    cout << endl
         << endl
         << "Enter the encryption key: ";
    //cin >> key;

    cout << "======= SETTINGS ======" << endl;
    cout << "IP address: " << ip << endl;
    cout << "Output: " << outFile << endl;
    cout << "Encryption key: " << key << endl;

    cout << "Setting up the socket..." << endl;

    // create the fd for the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        cout << "Setting up the socket failed." << endl;
        exit(1);
    };

    // assign IP
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to the setting so that it can actually listen
    bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    // if (bindI != 0)
    // {
    //     cout << "There was a problem binding socket to port " << port << endl;
    //     exit(1);
    // };

    // listen on the bound socket and accept a maxiumum of 5 concurrent connections
    if ((listen(sockfd, 5)) < 0)
    {
        cout << "Failed to start listening on port " << port << endl;
        exit(1);
    }

    len = sizeof(cliaddr);
    new_socket = accept(sockfd, (struct sockaddr *)&cliaddr, &len);

    if (new_socket < 0)
    {
        cout << "Server accept failed." << endl;
        exit(1);
    }
    while (read(new_socket, buffer, 1))
    {
        cout << buffer[0] << endl;
    }

    close(sockfd);

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
    }
    else
    {
        cout << "Unrecognized mode set. Please refer to the README for instructions on how to use this program." << endl;
        exit(1);
    }
    return 0;
}
