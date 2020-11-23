#include <iostream>
#include <sys/socket.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <algorithm>
#include "encryption.hpp"
#include "settings.hpp"
#include "cstring"
#include <chrono>
#include <math.h>
using namespace std::chrono;

using namespace std;

// TO-DO: Multithread the encrypting and decrypting of the data. Most useful with large data files.

// intialize global setting for verbosity as 0, if enabled it is set to 1
int g_verbose = 0;
const string encKey = "encryption";

void thread_func(string const &message)
{
    cout << message << " from the thread!" << endl;
}

void xor_crypt(const string &key, char *data, int data_len)
{
    for (int i = 0; i < data_len; i++)
    {
        data[i] ^= key[i % key.size()];
    }
}

int client()
{
    // start the timing
    auto start = high_resolution_clock::now();
    // get the file opeend and ready to send.
    FILE *inputFile;
    inputFile = fopen("./testfile", "r");
    // make sure our cursor is at the start of the file
    fseek(inputFile, 0, SEEK_SET);

    // hardcode the port for now... will set this later in the settings
    int port = 9500;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;

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

    // let's see if we can get the size of the file so that we can see if the buffer is set larger than the actual file
    struct stat sb;
    if (stat("./testfile", &sb) == -1)
    {
        cout << "Error running stats on file." << endl;
        exit(1);
    }
    else
    {
        cout << "File size: " << (long long)sb.st_size << endl;
    }

    // add one to the the input to account for our "end of packet" symbol which is a -1
    long buffer_size = 10;
    // now resize the buffer size to be the size of the file instead of what the user set
    if ((int)sb.st_size < buffer_size)
    {
        cout << "It look like the packet size that you set (" << buffer_size << ") was bigger than the actual size (" << sb.st_size << ") of the file." << endl;
        buffer_size = (long)sb.st_size;
    }

    char buff[buffer_size];

    cout << "Size of buffer: " << sizeof(buff) << endl;

    // send to the server the size of the packets being sent
    send(sock, &buffer_size, sizeof(long), 0);
    cout << "I'm gonna send " << sb.st_size / buffer_size << " packets over." << endl;
    if (buffer_size > 65535)
    {
        cout << "\033[1;33mWarning: You set a very large packet size. You might see a different number of packets on the server size because of TCP packet size limitation. You'll still get all the data though.\033[1;0m" << endl;
    }
    uint32_t packetNum = 0;
    cout << "Buffer size: " << sizeof(buff) << endl;
    memset(buff, -1, sizeof(buff));
    int readSize;
    while ((readSize = fread(buff, sizeof(char), sizeof(buff), inputFile)) > 0)
    {
        // run the encryption on the buffer (opportunity for multithreading here)
        // for (int i = 0; i < sizeof(buff); i++)
        // {
        //     buff[i] = buff[i] ^ '5';
        // }
        xor_crypt(encKey, buff, readSize);
        send(sock, buff, sizeof(buff), 0);

        // cout << "Sent encrypted packet #" << packetNum << " - encrypted as ";
        cout << "Sent encrypted packet #" << packetNum << endl;

        // for (int i = 0; i < sizeof(buff); i++)
        // {
        //     if (sizeof(buff) == 2 && i == 2)
        //     {
        //         cout << "...";
        //         int backstep = 2;
        //         if (sizeof(buff) == 3)
        //         {
        //             backstep = 1;
        //         }
        //         i = sizeof(buff) - backstep;
        //     }
        //     cout << buff[i];
        // }
        // cout << endl;
        packetNum++;
        memset(buff, -1, sizeof(buff));
    }
    fclose(inputFile);

    cout << "Send Success!" << endl;
    cout << "MD5:" << endl;
    system("md5 ./testfile");

    auto stop = high_resolution_clock::now();

    // Get duration. Substart timepoints to
    // get durarion. To cast it to proper unit
    // use duration cast method
    auto duration = duration_cast<microseconds>(stop - start);

    float fDuration = (float)duration.count();
    cout << "Time taken by function: "
         << (float)fDuration / 1000000 << " seconds" << endl;
    return 0;
}

int server()
{
    // intialize the variables to hold the input values
    int port = 9500;
    string ip;
    string outFile;
    string key;

    // socket variables
    int sockfd, new_socket;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr, valread;

    cout << "Running as server." << endl;
    cout << "------ SERVER SETUP -------" << endl;
    cout << "Connect to IP address: ";
    cin >> ip;
    // check the ip to see if it's a valid IPv4 ip
    if (inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr.s_addr) <= 0)
    {
        cout << "Hmmm this ip doesn't looks right." << endl;
        exit(1);
    }

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
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        cout << "Setting up the socket failed." << endl;
        exit(1);
    };

    // assign IP
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    cout << "binding on port: " << port << endl;

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
    long packetSize[1] = {0};
    // get the packet size from the client
    read(new_socket, packetSize, sizeof(long));
    cout << "Setting the size of the packets to: " << (long)packetSize[0] << endl;
    FILE *fp;
    fp = fopen(outFile.c_str(), "w");
    u_int32_t pcktNum = 0;
    char buffer[packetSize[0]];
    memset(buffer, -1, sizeof(buffer));

    while (read(new_socket, buffer, sizeof(buffer)))
    {
        cout << "Rec packet #" << pcktNum << endl;
        int bufferSize = 0;
        for (int i = 0; i < sizeof(buffer); i++)
        {
            if (pcktNum == 437118)
            {
                cout << (int)buffer[i] << " ";
            }
            // if we get here, we know that the packet is incomplete so we set the termination point
            if ((int)buffer[i] == -1)
            {
                cout << "I've reached the end of the line." << endl;
                break;
            }
            bufferSize++;
        }
        cout << endl;
        if (pcktNum == 437118)
        {
            cout << "Buff size: " << bufferSize << endl;
        }
        xor_crypt(key, buffer, bufferSize);
        fwrite(&buffer, bufferSize, 1, fp);
        //memset(buffer, 0, sizeof(buffer));
        pcktNum++;
        memset(buffer, -1, sizeof(buffer));
    }
    fclose(fp);

    close(sockfd);

    system("md5 ./output.txt");

    return 0;
}

// handle the intial setup
int main(int argc, char *argv[])
{

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