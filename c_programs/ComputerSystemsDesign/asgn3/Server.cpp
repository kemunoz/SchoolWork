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
#include <deque>
#include <iostream>




using namespace std;
struct page{
	char *filecontents;
	char *filename;
	int dirty_bit;
};
deque <page> q;
char *creatresp = "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\n";
char *synerr = "HTTP/1.1 400 Bad Request\r\n";
char *permerr = "HTTP/1.1 403 Forbidden\r\n";
char * nferr = "HTTP/1.1 404 Not Found\r\n";
char *insererr = "HTTP/1.1 500 Internal Server Error\r\n";
char *okresp = "HTTP/1.1 200 OK\r\n";

//checks if page is in cache
//if yes or if array elem in empty then returns position in array
//returns -1 if we just need to push
//else -2 if we have to pop
int write_to_hd(char* filename, char* file_contents){

	int fd;
	fd = open(filename, O_RDWR | O_CREAT, 0666);
	//If the file was opened/created succesfully
	if(fd > 0)
	 {
	 	write(fd, file_contents,strlen(file_contents));
		close(fd);
		return 1;
	}
	else if(errno = 2)
	{
		return -2;
	}
}
// void print_deque(){
// 	printf("THIS IS THE SIZE: %d\n", q.size());
// 	for(int i = 0; i<q.size(); i++)
// 	{
// 		printf("Filename of %d in deque: %s\n",i, q[i].filename);
// 	}
// }

//checks to see if file is in the cache
int in_cache(char *test_file)
{
	volatile int strcmp_check;
	for(int i = 0; i< q.size(); i++)
	{
		strcmp_check = strncmp(test_file,q[i].filename,27);
		if(strcmp_check == 0)
		{
			
			return i;
		}
	}
	if(q.size()<4){
		
		return -1;
	}
	//pop front off cache
	else return -2;
}

int write_to_cache(char*filename,int length,int flag,int new_socket){


	//if flag >= 0
	//then replace whatever is in that array element
	//else pop off the next element 
	//if the dirty bit is 1 then call writetofile() then update cache
	//else if db == 0 then just update cache and call writetofile()
	int errchk;
	int valreadfile;
	char* bufferfile = (char *) calloc(length,sizeof(char));
	valreadfile = read(new_socket,bufferfile,length);
	//write(fd, bufferfile,valreadfile);
	//close(fd);
	if(flag >=0)
	{
		q[flag].filecontents = bufferfile;//whatever they are
		q[flag].filename = filename;
		q[flag].dirty_bit = 1;
		send(new_socket, okresp, strlen(okresp), 0);
		close(new_socket);
		return 1;
	}
	if(flag == -1)
	{
		errchk = write_to_hd(filename,bufferfile);
		if(errchk == -2)
		{
			send(new_socket, permerr, strlen(permerr), 0);
			close(new_socket);
			return 1;
		}
		struct page push_page;
		push_page.filecontents = bufferfile;
		push_page.dirty_bit = 0;
		push_page.filename = filename;
		q.push_back(push_page);
		send(new_socket, creatresp, strlen(creatresp), 0);
		close(new_socket);
		return 1;
	}
	else if(flag == -2)
	{
		if(q[0].dirty_bit == 1){
			write_to_hd(q[0].filename,q[0].filecontents);
		}
		q.pop_front();
		struct page push_page;
		push_page.filecontents = bufferfile;
		push_page.filename = filename;
		push_page.dirty_bit = 0;
		q.push_back(push_page);
		send(new_socket, creatresp, strlen(creatresp), 0);
		close(new_socket);
	}
	return 1;
}
int filename_check(char* filename)
{
	
	for(int i =0; i<strlen(filename);i++)
		{
			//				48 <= x <= 122									x == 45					 				48 <= x <= 57
			if( !(((int)filename[i] >= 97 || (int)filename[i] <= 122) || ((int)filename[i] == 45) || ((int)filename[i] >= 48 || (int)filename[i] <= 57)
			|| ( (int)filename[i] >= 65 || (int)filename[i] <=90 ) || ((int)filename[i] == 95) || ((int)filename[i] >= 0 || (int)filename[i] <= 9) ) )
			{
				return -1;
			}
		}
	if(strlen(filename) != 27)
	{	
		return -1;
	}
	return 1;

}



//This function reads from cache and returns 1 on success
//this fcuntion also takes care of closing the socket and sending the appropriate messages

int read_cache(int cache_position,int new_socket)
{
	char* bufferfile = q[cache_position].filecontents;
	char*contlength = (char *) calloc(100,sizeof(char));
	snprintf(contlength,100,"Content-Length: %d\r\n\r\n",strlen(bufferfile));
	send(new_socket,okresp,strlen(okresp),0);
	send(new_socket,contlength,strlen(contlength),0);
	send(new_socket,bufferfile,strlen(bufferfile),0);
	close(new_socket);
	return 1;
}

//this function reads form the disk whenever the file is not in the cache

int readfromfile(char* filename,int new_socket)
{
	int length, scerr,valreadfile,fd;
	struct stat sb;
	char *bufferfile;
	char* contlength;
	char* errnomsg;

	fd = open(filename,O_RDONLY);
		//if file was opened succesfully
		if(fd > 0)
		{
			//Send the header response to the client
			if(fstat(fd,&sb) != -1)
			{
				bufferfile = (char *) calloc(sb.st_size,sizeof(char));
				contlength = (char *) calloc(100,sizeof(char));
		 		snprintf(contlength,100,"Content-Length: %ld\r\n\r\n", sb.st_size);
				valreadfile = read(fd,bufferfile,sb.st_size);
				//Send the OK reponse
				send(new_socket, okresp, strlen(okresp), 0);
				//Send the content length followinf the repsonse
				send(new_socket,contlength,strlen(contlength),0);
				//send the data from the file
				send(new_socket,bufferfile,valreadfile,0);
				close(new_socket);
			}
			return 1;
		}
		else
		{
			errnomsg = strerror(errno);
			scerr = strcmp(errnomsg,"No such file or directory");
			if(scerr == 0)
			{
				send(new_socket,nferr,strlen(nferr),0);
				close(new_socket);
				return -2;
			}
			else
			{
				send(new_socket,permerr,strlen(permerr), 0);
				close(new_socket);
				return -3;
			}
		}
				
}



int main(int argc, char *argv[]){
	int server_fd, new_socket,portnum = 0;
	int errchk,opt,cache_flag = 0,logging_flag = 0,sum = 0;
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
	char rqst[10];
	char* filename = (char *) calloc(100,sizeof(char));
	char * hostname;
	char* log_file;
	if(argc == 1)
	{
		perror("Invalid number of command line args");
		exit(EXIT_FAILURE);
	}
	while((opt = getopt(argc,argv,":c"))!=-1)
	{
		switch(opt)
		{
			case 'c':
				cache_flag = 1;
				break;
			case 'l':
				logging_flag = 1;
				log_file = optarg;
				break;

		}

	}
	for(;optind<argc; optind++){
		if(sum == 0)
		{
			hostname = argv[optind];
			hent = gethostbyname(hostname);
		}
		if(sum == 1)
		{
			portnum = atoi(argv[optind]);
		}
		sum++;
	}

	if(portnum == 0)
	{
		portnum = 8080;
	}



	//creating the file scoket descriptor
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}


	address.sin_family = AF_INET;
	//address.sin_addr.s_addr = INADDR_ANY;
	//address.sin_addr.s_addr = hent->h_addr;
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
		if(cache_flag == 1)
		{
			if(scp == 0)
			{
				char* filename = (char *) calloc(100,sizeof(char));

				sscanf(bufferhead,"%*s %*[/]%s %*s %*s %*s %*s %*s %*s %*s %*s %d %*s %*s",filename,&length);
				errchk = filename_check(filename);
				//then check if file is in the cache
				if(errchk == -1)
				{
					send(new_socket,synerr,strlen(synerr),0);
					close(new_socket);
					continue;
				}
				else if(errchk != -1)
				{
					errchk = in_cache(filename);
					write_to_cache(filename,length,errchk,new_socket);
					//print_deque();
					continue;
				}
			}
			else if(scg == 0)
			{
				char* filename = (char *) calloc(100,sizeof(char));
				sscanf(bufferhead,"%*s %*[/]%s" ,filename);
				errchk = filename_check(filename);
				if(errchk == -1)
				{
					send(new_socket,synerr,strlen(synerr),0);
					close(new_socket);
					continue;
				}
			//errchk = readfromfile(filename,new_socket);
				errchk = in_cache(filename);
				if(errchk >= 0)
				{
					read_cache(errchk,new_socket);
				}
				else
				{
					errchk = readfromfile(filename,new_socket);
				}
			
			} 
			else
			{
				//Was neither a GET or PUT request so sond a 500 internal server erro
				send(new_socket,insererr,strlen(insererr),0);
				close(new_socket);
			}
		}
		else
		{
			if(scp == 0)
			{

				sscanf(bufferhead,"%*s %*[/]%s %*s %*s %*s %*s %*s %*s %*s %*s %d %*s %*s",filename,&length);
				errchk = filename_check(filename);
				if(errchk == -1)
				{
					send(new_socket,synerr,strlen(synerr),0);
					close(new_socket);
					continue;
				}
				else if(errchk != -1)
				{
					char* bufferfile = (char *) calloc(length,sizeof(char));
					valreadfile = read(new_socket,bufferfile,length);
					errchk = write_to_hd(filename,bufferfile);
				}
				if(errchk == 1)
				{
					send(new_socket,creatresp,strlen(creatresp), 0);
					close(new_socket);
					continue;
				}
				else if(errchk == -2)
				{
					send(new_socket,permerr,strlen(permerr), 0);
					close(new_socket);
					continue;
				}
			}
			else if(scg == 0)
			{
				sscanf(bufferhead,"%*s %*[/]%s" ,filename);
				errchk = filename_check(filename);
				if(errchk == -1)
				{
					send(new_socket,synerr,strlen(synerr),0);
					close(new_socket);
					continue;
				}
				errchk = readfromfile(filename,new_socket);
			
			} 
			else
			{
				//Was neither a GET or PUT request so sond a 500 internal server erro
				send(new_socket,insererr,strlen(insererr),0);
				close(new_socket);
			}
		}
		
	 }


	return 0;
}


