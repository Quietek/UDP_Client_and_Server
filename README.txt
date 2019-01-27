Basic UDP client and server for basic file transfer.

To use the server:
navigate to directory udp_server.c and its makefile are located in in a terminal
make to compile the server
./server [port number] to run the server on the specified port.
make clean to delete the executable


To use the client:
navigate to directory udp_client.c and its makefile are located in in a terminal
make to compile the client
./client [address/hostname] [port number] to run the client and connect to the specified address on the specified port.
make clean to delete the executable


Implemented commands (from client to server):
ls - list files located in the server executable's directory
exit - tell the server to close
put [filename] - transfer the specified file from the client to the server
get [filename] - transfer the specified file from the server to the client
delete [filename] - delete the specified file from the server

close the client using ctrl-c.
