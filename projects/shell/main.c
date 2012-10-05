/*
 * project 1 (shell) main.c template 
 *
 * Sean Bjornsson
 *
 * I took and modified some functions from the homeworks (linked list functions);
 */

/* you probably won't need any other header files for this project */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>



struct node {	//represents a process(job) in the list of jobs
        pid_t pid;
		char** command;
		char status[128];
        struct node *next; 
};
struct node* list_search(pid_t pid, struct node *list){	//search jobs for a process with given ID. retur that node
	while(list!=NULL){
		if(list->pid == pid)return list;
		list = list->next;
	}	
	return NULL;
}
    
void list_insert(pid_t pid,char** command, char* status, struct node **head) {	//insert new job at head
	struct node *newnode = malloc(sizeof(struct node));
	int i = 0;
	int count = 0;
	while(command[count]!=NULL)	{
		count++;
	}
	newnode->command = malloc(sizeof(char*)*(count+1));
	for(;i<count;i++){
		newnode->command[i] = strdup(command[i]);
	}
	newnode->command[i] = NULL;
	newnode->pid = pid;
	strncpy(newnode->status, status, 127);
    newnode->next = *head;
    *head = newnode;
	return;
}
void list_remove(pid_t p, struct node **head){	//remove job with given pid
	struct node* temp = *head;
	struct node* temp2;
	if(temp->pid==p){
		temp2 = temp;
		*head = temp->next;
		int i = 0;
		while(temp2->command[i]!=NULL){
			free(temp2->command[i]);
			i++;
		}
		free(temp2->command);
		free(temp2);
		return;
	}	
	while (temp->next!=NULL){
		if(temp->next->pid==p){
			temp2 = temp->next;			
			temp->next = temp->next->next;
			int i = 0;
			while(temp2->command[i]!=NULL){
				free(temp2->command[i]);
				i++;
			}
			free(temp2->command);
			free(temp2);
			return;
		}
		
		temp = temp->next;
		
	}
	return;
}

void print_node_death(struct node * n){	//print a notification that a job has terminated
	if(n == NULL)return;	
	int i = 0;
	printf("Process %d (",n->pid);
	while(n->command[i]!=NULL){
		printf("%s ",n->command[i]);
		i++;
	}
	printf(") has completed.\n");
}

void list_dump(struct node *list) {	//print all jobs
        int i = 0;
		int j = 0;
        printf("\nJOBS:\n");
        while (list != NULL) {
            printf("Job %d:\t%d\t%s\t", i++,list->pid, list->status);
			while(list->command[j]!=NULL){
				printf("%s ",list->command[j]);
				j++;
			}
			j = 0;
			printf("\n");
            list = list->next;
        }
}
void list_clear(struct node *list) {	//free jobs list. used to clean up memory at the end
    while (list != NULL) {
        struct node *tmp = list;
        list = list->next;
        free(tmp);
	}
}
void removeendspace(char * str){		
	int i = strlen(str)-1;
	for(;i>-1;i--){
		if(isspace(str[i]))str[i] = '\0';
	}	
}

void cleanmem(char** a1, char*** a2, int len){	//free malloc'd mem
	int i=0;
	for(;i<len;i++){
		free(a2[i]);
	}
	free(a1);free(a2);
}

char* removecomments(char* str){
	int i = 0;
	for(;i<strlen(str);i++){
		if(str[i] == '#'){
			str[i] = '\0';
			return str;
		}
	}
	return str;
}

char** tokenify(char* s, char* del){
	char* str = strdup(s);
	char* temp = strtok(str, del);
	if(temp == NULL){ 
		char** toks = malloc(sizeof(char*));
		toks[0] = NULL;
		return toks;
	}	
	int count =	0;
	while(temp!=NULL){
		count++;
		temp = strtok(NULL, del);
	}
	char** toks = malloc((count+1)*sizeof(char*));
	toks[0] = strtok(s, del);
	int i = 1;
	for(;i<=count;i++){
		toks[i] = strtok(NULL, del);
	}
	free(str);
	return toks;
}

int main(int argc, char **argv) {
	struct node* jobs = NULL;							//list of active parallel jobs
	int numjobs = 0;
	char fullpath[1024];									
	int sequentialnext = 1;								//switch mode  bit	
	int sequentialcurrent = 1;							//current mode bit
	int exitwait = 0;									//wait to exit in parallel mode bit
	struct rusage usage;							
	struct timeval utime,ktime;
	FILE * paths = NULL;								
	struct stat statreturn;
	if(!stat("shell-config",&statreturn)){				//if shell-config exists, open it
		paths = fopen ("shell-config", "rt");
	}
	struct stat statresult;
	int SIGCHLDhandler(int signum){						//signal handler for dead children
		if(!sequentialcurrent){
			int rstatus = 0;
    		pid_t childp =wait(&rstatus);
			print_node_death(list_search(childp,jobs));
			list_remove(childp, &jobs);
			fflush(stdout);
		}
		return 0;
	}
	signal(SIGCHLD, SIGCHLDhandler);
	char buffer[1024];
	char *prompt = "*_*_*_*_*> ";
    printf("%s", prompt);
    fflush(stdout);
    while (fgets(buffer, 1024, stdin) != NULL) {
		strcpy(buffer, removecomments(buffer));
		char* delimiter = ";";							//delimit command line input by ; to get command groupings. store in a1
		char**a1 = tokenify(buffer, delimiter);
		int a1length = 0;
		while(a1[a1length]!=NULL){
			a1length++;
		}
	
														
		char*** a2 = malloc(sizeof(char**)*a1length);	
		int i = 0;
		delimiter = " \n\t";
		for(;i<a1length;i++){								//delimit elts of a1 by whitespace and store in a2
			a2[i] = tokenify(a1[i],delimiter);
		}
	
		pid_t pid;
		i = 0;
		for(;i<a1length;i++){ 								//iterate through commands
			if(a2[i] == NULL||a2[i][0] == NULL) continue;	//if a command is null skip
			else if(!strcmp(a2[i][0],"mode")){				//check for mode command
				if(a2[i][1]==NULL) {
					if(sequentialcurrent)printf("%s", "mode: sequential\n");
					else printf("%s", "mode: parallel\n");		
				}
				else if(!strcmp(a2[i][1],"sequential") || !strcmp(a2[i][1], "s")){ 	
					sequentialnext = 1;					
				}
				else if(!strcmp(a2[i][1], "parallel") || !strcmp(a2[i][1], "p")){ 
					sequentialnext = 0;
				}
				else printf("Invalid mode argument\n");
				
			}
			else if(!strcmp(a2[i][0],"exit")){				//check for exit command
				if(!sequentialcurrent){						//execute later if in parallel
					exitwait = 1;
				}
				else{
					getrusage(RUSAGE_SELF,&usage);
					utime =usage.ru_utime;
					ktime = usage.ru_stime;
					printf("User CPU usage: %ld.%08ld\tKernel CPU usage: %ld.%08ld\n", utime.tv_sec, utime.tv_usec, ktime.tv_sec, ktime.tv_usec);
					cleanmem(a1,a2,a1length);
					exit(EXIT_SUCCESS); 					//exit if a2[i][0] == "exit" and in sequential mode
				}
			}
			else if(!strcmp(a2[i][0],"jobs")){				//check for jobs command
				list_dump(jobs);
			}	
			else if(!strcmp(a2[i][0],"pause")){				//check for pause command
				if(a2[i][1]==NULL){
					printf("Please enter a PID after the command.\n");
				}
				else{
					pid_t p = atoi(a2[i][1]);				//convert pid from string to int
					struct node * process = list_search(p,jobs);
					if(process==NULL){
						printf("Please enter the PID of a running process;\n");
					}
					else if (!strcmp(process->status,"paused")){
						printf("Process %d is already paused.\n",p);
					}
					else{
						int x = kill(p,SIGSTOP);			//pause child process			
						if(x<0);{
							printf("stderr, kill(pid,SIGSTOP) failed: %s\n", strerror(errno));
						}
						strncpy(process->status,"paused", 127);
						printf("Process %d paused.\n",pid);
					}
				}
			}
			else if(!strcmp(a2[i][0],"resume")){			//check for resume command
				if(a2[i][1]==NULL){
					printf("Please enter a PID after the command.\n");
				}
				else{
					pid_t p = atoi(a2[i][1]);				//convert pid from string to int
					struct node * process = list_search(p,jobs);
					if(process==NULL){
						printf("Please enter the PID of a running process;\n");
					}
					else if (!strcmp(process->status,"running")){
						printf("Process %d is already running.\n",p);
					}
					else{
						if(kill(p,SIGCONT)<0);{				//resume child process
							printf("stderr, kill(pid,SIGCONT) failed: %s\n", strerror(errno));
						}
						strncpy(process->status,"running", 127);
						printf("Process %d resumed.\n",pid);
					}
				}	
			}
			else{											//ready to execute commands. execute according to mode
				int rv = stat((a2[i][0]), &statresult);
				if(rv&&paths!=NULL){						//enter loop if command isnt stand alone and if shell-config exists
					char *path = malloc(1024);
					while(fgets(path, 1024, paths) != NULL&&rv)	//read directories out of file
   					{
						
						removeendspace(path);
						strcat(path,"/");
						strcat(path,a2[i][0]);
						rv = stat(path,&statresult);
						if(!rv){							//check if new path is an executable
							strncpy(fullpath,path,1024);
							a2[i][0] = &fullpath;			//replace old path with new
						}
   					}
   					fclose(paths);
					paths = fopen ("shell-config", "rt");
					free(path);	
				}		
					
				pid = fork();								//create child to execute command
				if(!sequentialcurrent&&pid>0){				//if in parallel, keep track of jobs
					numjobs++;
					list_insert(pid, a2[i],"running",&jobs);
				}	
				if(sequentialcurrent){						//in sequential mode
					if(pid == 0){
					/*in child*/
						if ( execv (a2[i][0] , a2[i]) < 0){
							fprintf (stderr , " execv failed : %s. Command was misspelled or does not exist.\n", strerror ( errno ));
							exit(EXIT_FAILURE);
						}
					}
					else if (pid>0){
				  		/* in parent */
            			int rstatus = 0;
            		    wait(&rstatus);
					}
					else {
           			/* fork had an error; bail out */
						fprintf(stderr, "fork failed: %s\n", strerror(errno));
       				 }
				}
				else{										//in parallel mode
					if(pid == 0){
					/*in child*/
						numjobs++;
						jobs = malloc(sizeof(char**)*numjobs);
						if ( execv (a2[i][0] , a2[i]) < 0){
							fprintf (stderr , " execv failed : %s. Command was misspelled or does not exist.\n", strerror ( errno ));
							exit(EXIT_FAILURE);
						}
					}
				}
			}
		}
		printf("%s", prompt);
        fflush(stdout);
		sequentialcurrent=sequentialnext;
		if(exitwait){										//exit if it was requested and no jobs are running
			if(jobs==NULL){
				getrusage(RUSAGE_SELF,&usage);
				utime =usage.ru_utime;
				ktime = usage.ru_stime;
				printf("User CPU usage: %ld.%08ld\tKernel CPU usage: %ld.%08ld\n", utime.tv_sec, utime.tv_usec, ktime.tv_sec, ktime.tv_usec);
				cleanmem(a1,a2,a1length);
				fclose(paths);
				exit(EXIT_SUCCESS);
			}
			else printf("Cannot exit while processes are still running.\n");
		}		
		cleanmem(a1,a2,a1length);
		
    }

    return 0;
}

