/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <dirent.h>
#include <regex.h>

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  


  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

  //Variables I added
  //while loop exit clause
  int keeprunning = 1;
  //new buffer to send back to the client
  char newbuf[BUFSIZE];
  //regular expression variable init
  regex_t regexGet;
  regex_t regexPut;
  regex_t regexDelete;
  int retiGet;
  int retiPut;
  int retiDelete;
  FILE *fp;
	
  	/* 
   	* check command line arguments 
   	*/
  	if (argc != 2) {
    		fprintf(stderr, "usage: %s <port>\n", argv[0]);
    		exit(1);
  	}
  	portno = atoi(argv[1]);

  	/* 
   	* socket: create the parent socket 
   	*/
  	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  	if (sockfd < 0) 
    	error("ERROR opening socket");

  	/* setsockopt: Handy debugging trick that lets 
   	* us rerun the server immediately after we kill it; 
   	* otherwise we have to wait about 20 secs. 
   	* Eliminates "ERROR on binding: Address already in use" error. 
   	*/
  	optval = 1;
  	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     	(const void *)&optval , sizeof(int));

  	/*
   	* build the server's Internet address
   	*/
  	bzero((char *) &serveraddr, sizeof(serveraddr));
  	serveraddr.sin_family = AF_INET;
  	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  	serveraddr.sin_port = htons((unsigned short)portno);

  	/* 
   	* bind: associate the parent socket with a port 
   	*/
  	if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   	sizeof(serveraddr)) < 0) 
    	error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (keeprunning) {

    	/*
     	* recvfrom: receive a UDP datagram from a client
     	*/
    	bzero(buf, BUFSIZE);
	bzero(newbuf, BUFSIZE);
    	n = recvfrom(sockfd, buf, BUFSIZE, 0,
		(struct sockaddr *) &clientaddr, &clientlen);
    	if (n < 0)
      		error("ERROR in recvfrom");

    	/* 
     	* gethostbyaddr: determine who sent the datagram
     	*/

	hostaddrp = inet_ntoa(clientaddr.sin_addr);
	if (hostaddrp == NULL)
	      	error("ERROR on inet_ntoa\n"); 
	/*if(strcmp(hostaddrp,"127.0.0.1")){ 
  		hostp = gethostbyaddr(hostaddrp, 
			sizeof(clientaddr.sin_addr), AF_INET);
	}
	else{
	    	hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			sizeof(clientaddr.sin_addr.s_addr), AF_INET);
	}*/
	hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
		sizeof(clientaddr.sin_addr.s_addr), AF_INET);
	if (hostp == NULL)
		herror("Error on gethostbyaddr" );
	

    	printf("server received datagram from %s (%s)\n", 
		hostp->h_name, hostaddrp);
    	printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
		
	//regular expression initialization
	//get command
	retiGet = regcomp(&regexGet, "get .*", 0);
	retiGet = regexec(&regexGet, buf, 0, NULL, 0);
	//put command
	retiPut = regcomp(&regexPut, "put .*", 0);
	retiPut = regexec(&regexPut, buf, 0, NULL, 0);
	//delete command
	retiDelete = regcomp(&regexDelete, "delete .*", 0);
	retiDelete = regexec(&regexDelete, buf, 0, NULL, 0);

	//match get command
	if (!retiGet){
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
			//reset buffer
			bzero(buf, BUFSIZE);
			//find the size of the file
			fseek(fp, 0L, SEEK_END);
			sz = ftell(fp);
			//convert to string and send to client
			sprintf(buf,"%d",sz);
			n = sendto(sockfd, buf, strlen(buf), 0, &clientaddr, clientlen);
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
				n = sendto(sockfd, buf, BUFSIZE, 0, &clientaddr, clientlen);
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
    		/* print the client's reply */
    		n = recvfrom(sockfd, buf, BUFSIZE, 0, &clientaddr, &clientlen);
    		printf("Client Response: %s", buf);
	}
	//match put command
	else if (!retiPut){
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
		/*open the file, this program assumes that the server has write permissions to every
		file in the local directory, if it doesn't there may be issues encountered since there
		is no check to make sure of success due to it being in write mode instead of read */
		fp = fopen(filename, "wb");
		//reset the buffer
		bzero(buf,BUFSIZE);
		//receive a packet from the client, expecting the file size to be this packet
		n = recvfrom(sockfd, buf, BUFSIZE, 0,
			(struct sockaddr *) &clientaddr, &clientlen);
		//convert the received packet into an integer for later
		sz = atoi(buf);
		size = 0;
		//printf("Total Size: %d\n",sz);
		//while the file still has portions unwritten
		while(size < sz){
			//reset the buffer then wait to receive a packet
			bzero(buf,BUFSIZE);
			n = recvfrom(sockfd, buf, BUFSIZE, 0,
				(struct sockaddr *) &clientaddr, &clientlen);
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
			//printf("size: %d\n",size);
		}
		//close the file
		fclose(fp);
		//reset buffer and inform client the file was copied succesfully
		bzero(buf,BUFSIZE);
		strcpy(buf, "File copied successfully.\n");
		n = sendto(sockfd, buf, strlen(buf), 0, 
			(struct sockaddr *) &clientaddr, clientlen);
	}
	//match delete command
	else if (!retiDelete){
		//initialize variables
		int i;
		int filedeleted;
		char filename[strlen(buf)-7];
		//copy filename into new string, being careful not to copy the newline character and make sure the string is terminated properly
		for(i=7;i<strlen(buf)-1;i++){
			if (buf[i] != '\n'){
				filename[i-7] = buf[i];
			}
			else{
				filename[i-7] = '\0';
			}
		}
		//delete the file, as well as store a value to determine if the file was properly deleted or not
		// 0 on success, 1 on failure
		filedeleted = remove(filename);
		//check if file was deleted and output status to the new buffer to send to client.
		if (!filedeleted){
			strcpy(newbuf, "successfully deleted file\n");
		}
		else {
			strcpy(newbuf,"error deleting file\n");
		}
		//send the file deletion status to the client.
		n = sendto(sockfd, newbuf, strlen(newbuf), 0, 
			(struct sockaddr *) &clientaddr, clientlen);
	}
	//match exit command
    	else if (!strcmp(&buf,"exit\n")){
		//copy string to the new buffer to let client know the server was closed
       		strcpy(newbuf, "server exited.\n");
		//output that the server is closing to the server console
       		printf("exiting server...\n");
		//change value to exit while loop
       		keeprunning = 0;
		//send confirmation that the server is exiting to the client
       		n = sendto(sockfd, newbuf, strlen(newbuf), 0, 
			(struct sockaddr *) &clientaddr, clientlen);
	}
	//match ls command
	else if (!strcmp(&buf,"ls\n")){
		//declare directory navigation variables
		DIR *wd;
		struct dirent *directory;
	     	wd = opendir(".");
		//declare string copy variables
		int pos = 0;
		int i;
		//check for working directory
	     	if (wd) {
			//loop between each file in the directory
	        	while ((directory = readdir(wd))) {
				//copy the filename to the new buffer without the newline character			
				for (i=0; i<=strlen(directory->d_name)-1; i++) {
					newbuf[pos] = directory->d_name[i];
					pos++;
				}
				//add a space between filenames
				newbuf[pos] = ' ';
				pos++;
	       		}
			//end the buffer with a newline character
			newbuf[pos] = '\n';
		}
		//close the directory
		closedir(wd);
		//send the output to the client
	       	n = sendto(sockfd, newbuf, strlen(newbuf), 0, 
			(struct sockaddr *) &clientaddr, clientlen);
		//check for error when sending back to the client
		if (n < 0) 
			error("ERROR in sendto");
	}
	//if the command is not matched, echo the command back to the client
	else{
		printf("unknown command, echoing message...\n");
		n = sendto(sockfd, buf, strlen(buf), 0, 
			(struct sockaddr *) &clientaddr, clientlen);
	}
  }

exit(0);
}
