#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


int main(int argc, char **argv) {
	char* filename = argv[1];
	int fd = creat(filename, S_IRWXU);
	if(fd<0){
		fprintf(stderr, "Error: %d\n", strerror(errno));
	}
	else close(fd);
	
}
