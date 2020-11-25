# Secure Inter-process Communication (IPC)

Implementation of secure (encrypted) communication of file transfers between a client and server using TCP socket streams.

## Instructions

To compile run: `make all`

To run: `./run`

The options are:

- server: this runs the program as the server and listens for incoming connections - `./run server`
- client: this runs the program as the client and sends a file to a specific IP address and port - `./run client`

Options:

- `-v` or `--verbose`: this will print each byte in the data packet in hex
