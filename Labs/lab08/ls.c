#include <sys/types.h>
#include <dirent.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>       
#include <unistd.h>
#include <time.h>


int main(int argc, char **argv) {
	char* dirname = argv[1];
	DIR *dir = opendir(dirname);
    struct dirent *buffer;
	int dirorfile;
	char* type;
	char* name;
	struct stat fileinfo;
	while ((buffer=readdir(dir))!=NULL){
		dirorfile = buffer->d_type;
		name = buffer->d_name;
		if(dirorfile==4)type="Directory";
		else{
			type = "File";
			stat(name,&fileinfo);
		}
		printf("Name: %s\tType: %s\t",name, type);
		if(dirorfile==8){
			printf("Size: %d\t",fileinfo.st_size);
			char* time_last_accessed=asctime(localtime(&fileinfo.st_atime));
			printf("Last accessed: %s",time_last_accessed);
		}
		
		printf("\n");
	}
    int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result);
	return 0;
}


