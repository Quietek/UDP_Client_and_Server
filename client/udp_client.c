/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <regex.h> 

#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}



int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    
    //Variables I added
    //second buffer for receiving messages
    char newbuf[BUFSIZE];
    //regular expression variable init
    regex_t regexGet;
    regex_t regexPut;
    regex_t regexDelete;
    int retiGet;
    int retiPut;
    int retiDelete;
    FILE *fp = NULL;

    	/* check command line arguments */
    	if (argc != 3) {
       		fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       		exit(0);
    	}
    	hostname = argv[1];
    	portno = atoi(argv[2]);

    	/* socket: create the socket */
    	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    	if (sockfd < 0) 
        	error("ERROR opening socket");

    	/* gethostbyname: get the server's DNS entry */
    	server = gethostbyname(hostname);
    	if (server == NULL) {
        	fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        	exit(0);
    	}

    	/* build the server's Internet address */
    	bzero((char *) &serveraddr, sizeof(serveraddr));
    	serveraddr.sin_family = AF_INET;
    	bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    	serveraddr.sin_port = htons(portno);
	while(1){
    	//zero out buffer
	bzero(buf, BUFSIZE);
	//prompt user for a command
    	printf("Use one of the following commands: \n");
	printf("please note: this client is case sensitive. \n");
    	printf("get [file-name] 	- get file from server\n");
    	printf("put [file-name] 	- send file to server\n");
    	printf("delete [file-name] 	- delete file from server\n");
    	printf("ls 			- list files server has in its local directory\n");
    	printf("exit 			- close the server\n");
	//read user command into buffer
    	fgets(buf, BUFSIZE, stdin);
	
	//regular expression initilization
	//get command
	retiGet = regcomp(&regexGet, "get .*", 0);
	retiGet = regexec(&regexGet, buf, 0, NULL, 0);
	//put command
	retiPut = regcomp(&regexPut, "put .*", 0);
	retiPut = regexec(&regexPut, buf, 0, NULL, 0);
	//delete command
	retiDelete = regcomp(&regexDelete, "delete .*", 0);
	retiDelete = regexec(&regexDelete, buf, 0, NULL, 0);
	
	//get server length for use on sent message
	serverlen = sizeof(serveraddr);
	//match get command
	if (!retiGet){
		//variable declarations
		int i, size, sz;
		char filename[strlen(buf)-4], ch[1];
		//find filename in command
		for(i=4;i<strlen(buf);i++){
			if (buf[i] != '\n'){
				filename[i-4] = buf[i];
			}
			else{
				filename[i-4] = '\0';
			}
		}
		//send the command to the server
		n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
		//reset buffer
		bzero(buf, BUFSIZE);
		/*open the file, this program assumes that the server has write permissions to every
		file in the local directory, if it doesn't there may be issues encountered since there
		is no check to make sure of success due to it being in write mode instead of read */
		fp = fopen(filename, "wb");
		//reset the buffer
		bzero(buf,BUFSIZE);
		//receive a packet from the client, expecting the file size to be this packet
		n = recvfrom(sockfd, buf, BUFSIZE, 0,
			(struct sockaddr *) &serveraddr, &serverlen);
		//convert the received packet into an integer for later
		sz = atoi(buf);
		size = 0;
		//while the file still has portions unwritten
		while(size < sz){
			//reset the buffer then wait to receive a packet
			bzero(buf,BUFSIZE);
			n = recvfrom(sockfd, buf, BUFSIZE, 0,
				(struct sockaddr *) &serveraddr, &serverlen);
			//if the remainder of the file is greater than an entire packet, write the whole packet to the file
			if (sz-size > BUFSIZE){

				for(i=0;i<BUFSIZE;i++){
					ch[0] = buf[i];
					fwrite(ch, 1, 1, fp);
					size++;
				}
			}
			//if the remainder of the file is smaller than a full packet, write the remaining bytes to the file
			else{
				for (i=0;i<(sz-size);i++){
					ch[0] = buf[i];
					fwrite(ch, 1, 1, fp);
					size++;				
				}
			}
		}
		//close the file
		fclose(fp);
		//reset buffer and inform client the file was copied succesfully
		bzero(buf,BUFSIZE);
		strcpy(buf, "File copied successfully.\n");
		printf("File copied successfully.\n");
		n = sendto(sockfd, buf, strlen(buf), 0, 
			(struct sockaddr *) &serveraddr, serverlen);
	}
	else if (!retiPut){
		//variable delcarations
		int i,size,sz;
		char filename[strlen(buf)-4], ch[1];
		//pull filename from the buffer
		for(i=4;i<strlen(buf);i++){
			if (buf[i] != '\n'){
				filename[i-4] = buf[i];
			}
			else{
				filename[i-4] = '\0';
			}
		}
		//open the file
		fp = fopen(filename, "rb");

		//check if file opened properly
		if(fp) {
			//send the command to the server
			n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
			//reset buffer
			bzero(buf, BUFSIZE);
			//find the size of the file
			fseek(fp, 0L, SEEK_END);
			sz = ftell(fp);
			//convert to string and send to server
			sprintf(buf,"%d",sz);
			n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
			//reset buffer
			bzero(buf,BUFSIZE);
			//reset file seek to beginning of file
			fseek(fp, 0L, SEEK_SET);
			//declare local variable to keep track of how much of the file has been sent
			size = 0;
			//while there is still part of the file left to be sent
			while (size < sz){
				//if the remaining part of the file is larger than one full packet, fill the buffer completely
				if (sz-size > BUFSIZE){
					for(i=0;i<BUFSIZE;i++){
						fread(ch,1,1,fp);
						buf[i] = ch[0];
						size++;
					}
				}
				//if the remaining part of the file is smaller than one full packet, only write the necessary amount to the buffer
				else{
					for(i=0;i<(sz-size);i++){
						fread(ch,1,1,fp);
						buf[i] = ch[0];
						size++;
					}

				}
				//send packet to server and reset buffer
				n = sendto(sockfd, buf, BUFSIZE, 0, &serveraddr, serverlen);
				bzero(buf,BUFSIZE);
			}
		}
		//if the file could not be found, exit
		else{
			printf("File Does Not Exist.\n");
			exit(EXIT_FAILURE);
		}
		//close the file
		fclose(fp);
		//reset the buffer
    		bzero(buf, BUFSIZE);
    		/* print the server's reply */
    		n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
    		printf("Server Response: %s", buf);
	}
	else if (!retiDelete){
		n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
    		bzero(newbuf, BUFSIZE);
    		/* print the server's reply */
    		n = recvfrom(sockfd, newbuf, BUFSIZE, 0, &serveraddr, &serverlen);
    		printf("Server Response: %s", newbuf);
	}
	//no match with get, put, or delete (likely means ls or exit)
	else{

    		//send buffer to the server
    		n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
		//error check sendto
    		if (n < 0) 
      			error("ERROR in sendto");
		//zero out newbuffer for the response
    		bzero(newbuf, BUFSIZE);
    		//get server response
    		n = recvfrom(sockfd, newbuf, BUFSIZE, 0, &serveraddr, &serverlen);
		//error check recvfrom
    		if (n < 0) 
      			error("ERROR in recvfrom");
		//output server response to command line
    		printf("Server Response: %s", newbuf);
	}
}
    	return 0;
}
