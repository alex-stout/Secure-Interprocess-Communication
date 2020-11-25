#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>

using namespace std;

// macro to help find the right system call for md5
string getOsName()
{
#ifdef _WIN32
    return "Windows 32-bit";
#elif _WIN64
    return "Windows 64-bit";
#elif __APPLE__ || __MACH__
    return "Mac OSX";
#elif __linux__
    return "Linux";
#elif __FreeBSD__
    return "FreeBSD";
#elif __unix || __unix__
    return "Unix";
#else
    return "Other";
#endif
}

// this union aids in the "packetizing" of the packet metadata required by the protocol
union packetCap
{
    unsigned long value;
    unsigned char bytes[sizeof(unsigned long)];
};

// intialize global setting for verbosity as 0, if enabled it is set to 1
int g_verbose = 0;

// run the md5 sommand based on the system executing the program (Linux & Mac Supported)
void run_md5(string filename)
{
    string call;
    string OS = getOsName();
    if (strcmp(OS.c_str(), "Mac OSX") == 0)
    {
        call = "md5 ";
    }
    else if (strcmp(OS.c_str(), "Linux") == 0)
    {
        call = "md5sum ";
    }

    call += filename;

    system(call.c_str());
}

// Future improvement: Multithread the encrypting and decrypting of the data. Most useful with large size packets.
// XOR encryption based on a string key applied to an array of characters
void xor_crypt(const string &key, char *data, int size)
{
    for (int i = 0; i < size; i++)
    {
        data[i] ^= key[i % key.size()];
    }
}

// function to help interacting with the user. Type is the client/server, default (client) is 0. Set 1 for server mode
void get_user_input(string &key, string &ip, int &port, string &file, unsigned long &pcktSize, int type = 0)
{
    cout << "Connect to IP address: ";
    cin >> ip;

    cout << "Port: ";
    cin >> port;

    // tailor the output to the type of program being executed (server or client)
    if (type)
    {
        cout << "Save file to (default stdout): ";
    }
    else
    {
        cout << "File to be sent: ";
    }
    cin >> file;

    if (file.empty() && type == 0)
    {
        cout << "You must include an input file in client mode.";
        exit(1);
    }

    if (!type)
    {
        // use a double here so if the input is less than 1 KB (decimal number) we can handle it. This will get rounded, but it'll be close
        double packetInput;
        cout << "Pkt size (KB): ";
        cin >> packetInput;
        // convert to KB
        pcktSize = packetInput * 1000;
    }

    cout << "Enter encryption key: ";
    cin >> key;
}

// helper function to print the packet details as hex. In verbose mode, it prints every item in the packet
void print_packet(char *buffer, long long size)
{
    // if verbose mode is set, then print out every item in the packet
    if (g_verbose)
    {
        for (int i = 0; i < size; i++)
        {
            cout << hex << (int)buffer[i] << dec;
        }
    }
    else if (size <= 4) // if the packets are small, just print them all out
    {
        for (int i = 0; i < size; i++)
        {
            cout << hex << (int)buffer[i] << dec;
        }
    }
    else
    {
        // just print the first 2 and last 2 items in the array
        cout << hex << (int)buffer[0] << (int)buffer[1] << dec << "..." << hex << (int)buffer[size - 2] << (int)buffer[size - 1] << dec;
    }
    cout << endl;
    return;
}

// handler for the client execution
int client()
{
    // create the necessary variables for the client
    int port;
    unsigned long pcktSize;
    string ip;
    string inFile;
    string key;

    // socket variables
    int sockfd;
    struct sockaddr_in serv_addr;

    // fill variables with user input
    get_user_input(key, ip, port, inFile, pcktSize);

    // get the file opeend and ready to send.
    FILE *inputFile;
    inputFile = fopen(inFile.c_str(), "r");
    if (inputFile == NULL)
    {
        cout << "\033[1;31mError opening the file: " << inFile << ". Make sure it exists and that you actually entered something.\033[1;0m" << endl;
        return 1;
    }
    // make sure our cursor is at the start of the file
    fseek(inputFile, 0, SEEK_SET);

    // configure the settings of the socket
    int sock = 0, valread;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // check that the IP address of the server is a valid format for IP. Note: this does not check if there's something at that IP, it's only syntactical
    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0)
    {
        cout << "Hmmm this ip doesn't looks right." << endl;
        exit(1);
    }

    // create the socket object with the settings
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cout << "There was a problem creating the socket." << endl;
        exit(1);
    }

    // reach out and connect to the socket of the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cout << "Connection failed" << endl;
        exit(1);
    }

    // get the stats of the file to see if we can optimize the packet size.
    struct stat sb;
    if (stat(inFile.c_str(), &sb) == -1)
    {
        cout << "Error running stats on file." << endl;
        exit(1);
    }
    else
    {
        cout << "File size: " << (long long)sb.st_size << endl;
    }

    // create the number for our buffer
    unsigned long buffer_size = pcktSize;
    // if the file is smaller than the packet size set by the user, resize the buffer size to be the size of the file. This will be more efficient than whtat the user input was
    if ((unsigned long)sb.st_size < buffer_size)
    {
        // let them know this is happening so they don't freak out
        cout << "It look like the packet size that you set (" << buffer_size << ") was bigger than the actual size (" << sb.st_size << ") of the file." << endl;
        buffer_size = (long)sb.st_size;
    }

    // create the buffer with the added space for the size of the size metadata attached to each packet
    char buff[buffer_size + sizeof(unsigned long)];

    // this is the totoal packets size that we will initially send to the server to make sure it's on the same page and ready to receive our payload
    unsigned long setPctSize = (unsigned long)sizeof(buff);
    // send to the server the size of the packets being sent
    send(sock, &setPctSize, sizeof(unsigned long), 0);
    long long packetNum = 0;
    unsigned long readSize;
    while ((readSize = (unsigned long)fread(buff, sizeof(char), sizeof(buff) - sizeof(long), inputFile)) > 0)
    {
        // run the encryption on the buffer (this only needs to happen on the actual data that's read in so we set the size to be the size fo the read data)
        xor_crypt(key, buff, readSize);

        // initialize the union for the metadata
        packetCap end;
        end.value = readSize;
        // inject the bytes of the metadata into the payload
        for (int i = 0; i < sizeof(unsigned long); i++)
        {
            buff[sizeof(buff) - i - 1] = end.bytes[i];
        }
        // send the data over the socket
        send(sock, buff, sizeof(buff), 0);
        // print out the sent packet
        cout << "Sent encrypted packet #" << packetNum << " - encrypted as ";
        print_packet(buff, readSize);

        // increment the packet number counter to keep track of how many have been sent
        packetNum++;
    }
    // clean up
    fclose(inputFile);
    cout << "Send Success!" << endl;
    run_md5(inFile);
    return 0;
}

int server()
{
    // intialize the variables to hold the input values
    int port;
    string ip;
    string outFile;
    string key;
    // this is a placeholder that does nothing because honestly I couldn't figure out optional parameters of objects...
    unsigned long placeholder;

    // socket variables
    int sockfd,
        new_socket;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr;

    // populate the settings from the user
    get_user_input(key, ip, port, outFile, placeholder, 1);

    // create the fd for the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // error check for the socket setup
    if (sockfd < 0)
    {
        cout << "Setting up the socket failed." << endl;
        exit(1);
    };

    // assign IP
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    // since it's a server, we listen to any incoming connection
    servaddr.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to the setting so that it can actually listen (I couldn't for the life of me get this to work with error checking)
    bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    // listen on the bound socket and accept a maxiumum of 5 concurrent connections
    if ((listen(sockfd, 5)) < 0)
    {
        cout << "Failed to start listening on port " << port << endl;
        exit(1);
    }

    // now we're cookin with an incoming connection
    len = sizeof(cliaddr);
    // transfer the incoming connection to a dedicated port
    new_socket = accept(sockfd, (struct sockaddr *)&cliaddr, &len);

    if (new_socket < 0)
    {
        cout << "Server accept failed." << endl;
        exit(1);
    }
    // this is a single element array that holds the initial setting send from the client that tells the server how big the incoming packets are going to be
    unsigned long packetSize[1] = {0};
    // get the packet size from the client
    read(new_socket, packetSize, sizeof(unsigned long));

    // setup the file pointer for the output (it may not be used but is initialized to help the code further down)
    FILE *fp;
    // open the file for writing
    if (outFile.length() != 0)
    {

        fp = fopen(outFile.c_str(), "w");
    }

    long long pcktNum = 0;      // counter for the number of incoming packets
    char buffer[packetSize[0]]; // buffer to hold the incoming data (set to the size from the transmission from the client)

    // continue reading until there's nothing left. What's tricky about this is that it will read in chunks the size of the packets,
    // at the end of a transmission, the packet might not be full since there might be a smaller amount of data left than the packet size.
    // This is why there is a unsigned long attached to the end that tells us how much real data is put into the packet. Looking back this isn't
    // the best way to do this. The better way would be to have the number at the beginning and then malloc the size of the buffer to be only the
    // size of the data, but since in this assignment we're using fixed packet sizes, this only happens at the end of the file (or once per transmission).
    while (read(new_socket, buffer, sizeof(buffer)))
    {
        // to hold the data count
        packetCap output;
        // populate the union with the data
        for (int i = 0; i < sizeof(unsigned long); i++)
        {
            // specifically access the end of the incoming packet for the metadata section
            output.bytes[i] = buffer[sizeof(buffer) - i - 1];
        }

        // print out the sent packet
        cout << "Rec packet #" << pcktNum << " as ";
        print_packet(buffer, output.value);

        // decrypt the buffer with the key
        xor_crypt(key, buffer, output.value);

        // only do this if there was a file set
        if (outFile.length() != 0)
        {
            // write the buffer to the file
            fwrite(&buffer, output.value, 1, fp);
        }

        // increment the packet counter
        pcktNum++;
    }

    // clean up
    cout << "Receive Success!" << endl;
    fclose(fp);
    close(sockfd);
    if (outFile.length() != 0)
    {
        run_md5(outFile);
    }
    else
    {
        cout << "No md5 sum since results were not saved..." << endl;
    }

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
            // this is a global variable because it is used a in a couple distributed areas
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