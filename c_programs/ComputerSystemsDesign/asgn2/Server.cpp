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
#include <pthread.h>
#include <vector>
#include <iostream>

using namespace std;
//struct for each thread
struct args{
	char *bufferdata;
	char *filename;
	int length;
	int fd;
	int rqst_socket;
	int rqst_flag;
};
std::vector<args> v;
//this will be the global condition variable
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t syncheckread = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t syncheckwrite = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t writing = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rwmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t read_response = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t err_mutex = PTHREAD_MUTEX_INITIALIZER;

volatile int count = 0;
int ptr_flag = 1;
volatile int nreaders = 0;
char *creatresp = "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\n";
char *synerr = "HTTP/1.1 400 Bad Request\r\n";
char *permerr = "HTTP/1.1 403 Forbidden\r\n";
char *nferr = "HTTP/1.1 404 Not Found\r\n";
char *insererr = "HTTP/1.1 500 Internal Server Error\r\n";
char *okresp = "HTTP/1.1 200 OK\r\n";
//initialize the vector for the requests
struct args* ptr = NULL;

//GET
void readfromfile(void *i)
{
	int length, scerr,valreadfile,file_name_length;
	struct stat sb;
	char* bufferfile;
	char* contlength;
	char* errnomsg;
	char* filenamecheck;




	pthread_mutex_lock(&syncheckread);
	file_name_length = strlen(((struct args*)i)->filename);
	filenamecheck = ((struct args*)i)->filename;
	printf("%s\n",filenamecheck);
	for(int x = 0; x<file_name_length;x++)
	{
		if( !(((int)filenamecheck[x] >= 97 || (int)filenamecheck[x] <= 122) || ((int)filenamecheck[x] == 45) || ((int)filenamecheck[x] >= 48 || (int)filenamecheck[x] <= 57)
		|| ( (int)filenamecheck[x] >= 65 || (int)filenamecheck[x] <=90 ) || ((int)filenamecheck[x] == 95) || ((int)filenamecheck[x] >= 0 || (int)filenamecheck[x] <= 9) ) )
		{
			send(((struct args*)i)->rqst_socket,synerr,strlen(synerr),0);
			close(((struct args*)i)->rqst_socket);
			return ;
		}
	}
	pthread_mutex_unlock(&syncheckread);
	printf("the filename length is:%d\n",strlen(((struct args*)i)->filename));
	if(strlen(((struct args*)i)->filename) != 27)
	{	
		send(((struct args*)i)->rqst_socket,synerr,strlen(synerr),0);
		close(((struct args*)i)->rqst_socket);
		cout<<"in the filename length check"<<endl;
		return;
	}




	pthread_mutex_lock(&rwmutex);
	nreaders+=1;
	if(nreaders == 1)
	{
		pthread_mutex_lock(&writing);
	}
	pthread_mutex_unlock(&rwmutex);

	(((struct args*)i)->fd) = open(((struct args*)i)->filename,O_RDONLY);
		//if file was opened succesfully
		if((((struct args*)i)->fd) > 0)
		{
			pthread_mutex_lock(&read_response);
			if(fstat(((struct args*)i)->fd,&sb) != -1)
			{
				bufferfile = (char *) calloc(sb.st_size,sizeof(char));
				contlength = (char *) calloc(100,sizeof(char));
		 		snprintf(contlength,100,"Content-Length: %ld\r\n\r\n", sb.st_size);
				valreadfile = read(((struct args*)i)->fd,bufferfile,sb.st_size);
				send(((struct args*)i)->rqst_socket, okresp, strlen(okresp), 0);
				//Send the content length followinf the repsonse
				send(((struct args*)i)->rqst_socket,contlength,strlen(contlength),0);
				//send the data from the file
				send(((struct args*)i)->rqst_socket,bufferfile,valreadfile,0);
				pthread_mutex_unlock(&read_response);
			}
			pthread_mutex_lock(&rwmutex);
			nreaders -=1;
			if(nreaders == 0)
			{
				pthread_mutex_unlock(&writing);
			}
			pthread_mutex_unlock(&rwmutex);
			return ;
		}
		else
		{
			pthread_mutex_lock(&err_mutex);
			errnomsg = strerror(errno);
			scerr = strcmp(errnomsg,"No such file or directory");
			if(scerr == 0)
			{
				//mutex.up()
				pthread_mutex_unlock(&err_mutex);
				send(((struct args*)i)->rqst_socket,nferr,strlen(nferr),0);
				close(((struct args*)i)->rqst_socket);
				return ;

			}
			else
			{
				pthread_mutex_unlock(&err_mutex);
				send(((struct args*)i)->rqst_socket,permerr,strlen(permerr), 0);
				close(((struct args*)i)->rqst_socket);
				return ;
			}
		}
				
}
//PUT
void writetofile(void *i)
{
	int valreadfile;
	int file_name_length;

	pthread_mutex_lock(&syncheckwrite);
	file_name_length = strlen(((struct args*)i)->filename);
	char* filenamecheck = ((struct args*)i)->filename;
	printf("%s\n",filenamecheck);
	for(int x = 0; x<file_name_length;x++)
	{
		cout<<filenamecheck[x]<<endl;
		if( !(((int)filenamecheck[x] >= 97 || (int)filenamecheck[x] <= 122) || ((int)filenamecheck[x] == 45) || ((int)filenamecheck[x] >= 48 || (int)filenamecheck[x] <= 57)
		|| ( (int)filenamecheck[x] >= 65 || (int)filenamecheck[x] <=90 ) || ((int)filenamecheck[x] == 95) || ((int)filenamecheck[x] >= 0 || (int)filenamecheck[x] <= 9) ) )
		{
			send(((struct args*)i)->rqst_socket,synerr,strlen(synerr),0);
			close(((struct args*)i)->rqst_socket);
			return;
		}
	}
	pthread_mutex_unlock(&syncheckwrite);


	if(strlen(((struct args*)i)->filename) != 27)
	{	
		send(((struct args*)i)->rqst_socket,synerr,strlen(synerr),0);
		close(((struct args*)i)->rqst_socket);
		return;
	}
	((struct args*)i)->fd = open(((struct args*)i)->filename, O_RDWR | O_CREAT, 0666);
	//check if the file has been opened
	//use an array and see if the fd is in the array already
	//mutex.down(array check)
	//this will be a chr array. each cell will hold the name of the file which is 27chars long
	//mutex.up(arraycheck)
	//If the file was opened/created succesfully
	pthread_mutex_lock(&writing);
	if((((struct args*)i)->fd) > 0)
	 {
	 	char* bufferfile = (char *) calloc(((struct args*)i)->length,sizeof(char));
	 	//valreadfile = snprintf(bufferfile,((struct args*)i)->length,"%s",((struct args*)i)->bufferdata);
	 	//printf("%s\n",((struct args*)i)->bufferdata);
	 	//printf("%s\n",bufferfile);
	 	valreadfile=read(((struct args*)i)->rqst_socket,bufferfile,((struct args*)i)->length);
	 	printf("%d\n",valreadfile);
	 	write(((struct args*)i)->fd, bufferfile,valreadfile);
		close(((struct args*)i)->fd);
		send(((struct args*)i)->rqst_socket,creatresp,strlen(creatresp), 0);
		close(((struct args*)i)->rqst_socket);
		pthread_mutex_unlock(&writing);
		return;
	}
	else if(errno == 2)
	{
		pthread_mutex_unlock(&writing);
		send(((struct args*)i)->rqst_socket,permerr,strlen(permerr), 0);
		close(((struct args*)i)->rqst_socket);
		return;

	}

}

void *dispatch(void *i)
{
	while(1){
		while(count == 0){
		}
		pthread_cond_signal(&cv);
		
	}
	return NULL;
}

void *entry_point(void *i)
{
	while(1)
	{
		pthread_mutex_lock(&condition_mutex);
		pthread_cond_wait(&cv,&condition_mutex);


		pthread_mutex_lock(&count_mutex);
		printf("THREAD IS AWAKE\n");
		--count;
		if(ptr_flag){
			ptr = v.data();
			ptr_flag = 0;
		}
		pthread_mutex_unlock(&count_mutex);
		i = ptr;
		if(count>1){
			printf("before the seg");
			ptr++;
			printf("after the seg");
		}
		pthread_mutex_unlock(&condition_mutex);
		if(((struct args*)i)->rqst_flag == 1)
		{
			writetofile(i);
		}
		else
		{
			readfromfile(i);
		}
	}
	return NULL;
}


int main(int argc, char *argv[]){
	int server_fd,portnum,new_socket;
	int errchk;
	pthread_t thread,dispatcht;
	int valreadbuff,valreadfile;
	struct sockaddr_in address;
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
	int numth = 4;
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
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	memcpy(&address.sin_addr.s_addr,hent->h_addr,hent->h_length);
	address.sin_port = htons(portnum);
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
	//initialize the threads
	errchk = pthread_create(&dispatcht,NULL,&dispatch,NULL);
	if(errchk == 0)
		printf("DISPATCH CREATED SUCCESFULLY\n");
	for(int i =0; i<numth; i++)
	{
		struct args *comps = (struct args *)malloc(sizeof(struct args)); 
		errchk = pthread_create(&thread,NULL,&entry_point,(void*)comps);
		if(errchk ==0)
			printf("threads created\n");
	}
	while(1)
	{
		if((new_socket = accept(server_fd, (struct sockaddr*)&address,(socklen_t*)&addrlen)) < 0)
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}
		printf("request recieved\n");
		//this is the header
		valreadbuff = read(new_socket, bufferhead, 1024);
		//This gets the reqst from the header
		sscanf(bufferhead, "%s",rqst);
		scp = strcmp(rqst,"PUT");
		scg = strcmp(rqst,"GET");

		//if PUT
		if(scp == 0)
		{
			printf("PUT REQUEST RECIEVED\n");
			sscanf(bufferhead,"%*s %*[/]%s %*s %*s %*s %*s %*s %*s %*s %*s %d %*s %*s",filename,&length);
			printf("new_socket:%d\nlength:%d\n",new_socket,length);
			//valreadbuff = read(new_socket, bufferhead,length);
			v.push_back({"0",filename,length,0,new_socket,1});
			pthread_mutex_lock(&count_mutex);
			count +=1;
			printf("COUNT IS NOW:%d\n",count);
			pthread_mutex_unlock(&count_mutex);
		}
		else if(scg == 0)
		{
			sscanf(bufferhead,"%*s %*[/]%s" ,filename);
			count++;
			v.push_back({bufferhead,filename,0,0,new_socket,0});
			
		} 
		else
		{
			//Was neither a GET or PUT request so sond a 500 internal server erro
			send(new_socket,insererr,strlen(insererr),0);
			close(new_socket);
		}
	 }


	return 0;
}


