#include "network.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
int main(int argc, char **argv){
	struct stat statrv;
	if(0>stat("./abc", &statrv)){
		printf("stat failure\n");
	}
	else{
		printf("stat success\n");
	}
	int filesize = (int) statrv.st_size;
	printf("File size: %d bytes\n",filesize );
	char buffer[filesize];
	int fd = open("./abc", O_RDWR);
	read (fd, buffer, filesize);
	char * temp=malloc(sizeof(HTTP_200)+filesize);
	sprintf(temp, HTTP_200, filesize);
	sprintf(temp, "%s%s", temp, buffer);
	printf("\n\n%s\n",temp);
	close(fd);
	printf("abc");
	return 0;
}
