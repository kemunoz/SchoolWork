#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]){
	int server_fd, new_socket,portnum;
	int fileval = 0;
	int valreadbuff,valreadfile;
	struct sockaddr_in address;
	struct stat sb;
	struct hostent *hent;
	//scp is string compare value to PUT
	//scg is string compare value to GET
	int scp, scg,scerr;
	int addrlen = sizeof(address);
	int length,fd;
	char bufferhead[1024] = {0};
	//char bufferfile[1024] = {0};
	char rqst[10];
	char* filename = (char *) calloc(100,sizeof(char));
	char *hello = "Hello from server";
	//This response will be sent after a GET request
	char *okresp = "HTTP/1.1 200 OK\r\n";
	//This response will be sent after a PUT request
	char *creatresp = "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\n";
	//400 Error code response
	char *synerr = "HTTP/1.1 400 Bad Request\r\n";
	char *permerr = "HTTP/1.1 403 Forbidden\r\n";
	char * nferr = "HTTP/1.1 404 Not Found\r\n";
	char *insererr = "HTTP/1.1 500 Internal Server Error\r\n";
	char *errnomsg;
	char * hostname;
	//Checking command line argument
	if(argc == 1)
	{
		perror("Invalid number of command line args");
		exit(EXIT_FAILURE);
	}
	else if(argc == 2)
	{
		//the second input is hostname/ip
		hostname = argv[1];
		hent = gethostbyname(hostname);
		//if no port is specified use the default port
		portnum = 8080;

	}
	else if(argc == 3)
	{
		//the 3rd input is PORT #
		hostname = argv[1];
		hent = gethostbyname(hostname);
		portnum = atoi(argv[2]);
	}
	else if(argc > 3)
	{
		//Throw an error
		perror("Invalid number of command line args");
		exit(EXIT_FAILURE);

	}



	//creating the file scoket descriptor
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}


	address.sin_family = AF_INET;
	//address.sin_addr.s_addr = INADDR_ANY;
//	address.sin_addr.s_addr = hent->h_addr;
	memcpy(&address.sin_addr.s_addr,hent->h_addr,hent->h_length);
	//This is specifying the port we are using and we can leave 
	//it as a int but the data format is diff so we call htons() to convert it to the correct data
	address.sin_port = htons(portnum);

	//here we are forcefully attching the socket to the Port 8080
	//the 2nd parameter is to tell us where the connetion is coming from
	if(bind(server_fd, (struct sockaddr * )&address,sizeof(address))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if(listen(server_fd, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	//This next portion is the ACCEPT() part of the server
	//So how this works is we are accepting the information for the CLIENT socket. 
	//That way we can write to it when we get requests
	while(1)
	{
		if((new_socket = accept(server_fd, (struct sockaddr*)&address,(socklen_t*)&addrlen)) < 0)
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}
		//This is the whole header
		valreadbuff = read(new_socket, bufferhead, 1024);
		//This gets the reqst from the header
		sscanf(bufferhead, "%s",rqst);
		scp = strcmp(rqst,"PUT");
		scg = strcmp(rqst,"GET");
		//if PUT
		if(scp == 0)
		{
			//Do this for the PUT header
			//1. Open the file or Create it if it DNE
			sscanf(bufferhead,"%*s %*[/]%s %*s %*s %*s %*s %*s %*s %*s %*s %d %*s %*s",filename,&length);


			////////////////////////////////////////////////////////THIS IS 400 Bad Request/////////////////////////////////////
			for(int i =0; i<strlen(filename);i++)
			{
				//				48 <= x <= 122									x == 45					 				48 <= x <= 57
				if( !(((int)filename[i] >= 97 || (int)filename[i] <= 122) || ((int)filename[i] == 45) || ((int)filename[i] >= 48 || (int)filename[i] <= 57)
				|| ( (int)filename[i] >= 65 || (int)filename[i] <=90 ) || ((int)filename[i] == 95) || ((int)filename[i] >= 0 || (int)filename[i] <= 9) ) )
				{
					fileval = 1;
					send(new_socket,synerr,strlen(synerr),0);
					close(new_socket);
					break;
				}
			}
			if(fileval)
			{
				fileval = 0;
				continue;
			}
			else if(strlen(filename) != 27)
			{
				send(new_socket,synerr,strlen(synerr),0);
				close(new_socket);
				continue;
			}
			////////////////////////////////////////////////////////THIS IS 400 Bad Request/////////////////////////////////////



			fd = open(filename, O_RDWR | O_CREAT, 0666);
			//If the file was opened/created succesfully
			if(fd > 0)
			 {
			 	char* bufferfile = (char *) calloc(length,sizeof(char));
			 	valreadfile = read(new_socket,bufferfile,length);
			 	write(fd, bufferfile,valreadfile);
				close(fd);
				printf("Sending message\n");
				send(new_socket,creatresp,strlen(creatresp), 0);
			}
			else
			{
				if(errno == 2)
				{
					send(new_socket,permerr,strlen(permerr), 0);
					close(new_socket);
					continue;
				}
				
			}
			
		}
		else if(scg == 0)
		{
			//Do this for the GET Header
			sscanf(bufferhead,"%*s %*[/]%s" ,filename);

			////////////////////////////////////////////////////////THIS IS 400 Bad Request/////////////////////////////////////
			//check if it is a valid ascii character
			for(int i =0; i<strlen(filename);i++)
			{
				//				48 <= x <= 122									x == 45					 				48 <= x <= 57
				if( !(((int)filename[i] >= 97 || (int)filename[i] <= 122) || ((int)filename[i] == 45) || ((int)filename[i] >= 48 || (int)filename[i] <= 57)
				|| ( (int)filename[i] >= 65 || (int)filename[i] <=90 ) || ((int)filename[i] == 95) || ((int)filename[i] >= 0 || (int)filename[i] <= 9)) )
				{
					fileval = 1;
					send(new_socket,synerr,strlen(synerr),0);
					close(new_socket);
					break;
				}
			}
			if(fileval)
			{
				fileval = 0;
				continue;
			}
			else if(strlen(filename) != 27)//check if it is correct length
			{
				send(new_socket,synerr,strlen(synerr),0);
				close(new_socket);
				continue;
			}

			////////////////////////////////////////////////////////////This is 400 Bad Request//////////////////////////////////////


			//Here we will open the file so we can read from it
			fd = open(filename,O_RDONLY);
			//if file was opened succesfully
			if(fd > 0)
			{

				//Send the header response to the client
				if(fstat(fd,&sb) != -1)
				{
					char* bufferfile = (char *) calloc(sb.st_size,sizeof(char));

					char *contlength = (char *) calloc(100,sizeof(char));
			 		snprintf(contlength,100,"Content-Length: %ld\r\n\r\n", sb.st_size);

					valreadfile = read(fd,bufferfile,sb.st_size);
					//Send the OK reponse
					send(new_socket, okresp, strlen(okresp), 0);
					//Send the content length followinf the repsonse
					send(new_socket,contlength,strlen(contlength),0);
					//send the data from the file
					send(new_socket,bufferfile,valreadfile,0);
				}
				
			}
			else
			{

				errnomsg = strerror(errno);
				scerr = strcmp(errnomsg,"No such file or directory");
				if(scerr == 0)
				{
					send(new_socket,nferr,strlen(nferr),0);
					close(new_socket);
					continue;
				}
				else
				{
					send(new_socket,permerr,strlen(permerr), 0);
					close(new_socket);
					continue;
				}
				
			}
			
		} 
		else
		{
			//Was neither a GET or PUT request so sond a 500 internal server erro
			send(new_socket,insererr,strlen(insererr),0);
			close(new_socket);
		}
		send(new_socket, hello, strlen(hello), 0);
		close(new_socket);
	 }


	return 0;
}
