//This is a program that mimmicks the cat command line in linux
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int fd, sz;
	int loopflag = 1;
	char *c = (char *) calloc(100, sizeof(char));
	//if there is nothing given then just read from stdin
	if(argc == 1){
		//read until \0 from stdin then go to next file if available
		while(loopflag){
			sz = read(0,c,32);
			if(sz == 0)
				loopflag=0;
			else{
				c[sz] = '\0';
				printf("%s", c);
			}
		}
	}
	else {
	 	for(int i = 1 ; i<argc; i++){
		//This is where all the code will go because I have to iterate through everything in the statement
			//if(- is given then copy from stdin)
			if(*argv[i] == '-'){
				while(loopflag){
					sz = read(0,c,32);
					if(sz == 0)
						loopflag=0;
					else{
						c[sz] = '\0';
						printf("%s", c);
					}
				}
			}
			//Open the first file in the command line
			fd = open(argv[i], O_RDONLY);
			if(fd != -1) {
				sz = read(fd,c,32);
				if(sz == -1){
					warn("%s",argv[i]);
				}
				else{
					while(sz != 0){
						c[sz] = '\0';
						printf("%s", c);
						sz = read(fd,c,32);
					}
				}
				
				close(fd);
			}
			else{
				warn("%s", argv[i]);
				return 0;

			}
		}
	}
	

	return 0;

}
