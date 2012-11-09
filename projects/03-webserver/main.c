#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "network.h"


// global variable; can't be avoided because
// of asynchronous signal interaction
int still_running = TRUE;
void signal_handler(int sig) {
    still_running = FALSE;
}
//each node is essentially the socket that is being passed to the socket pool, with its client address.
struct node{
	int socket;
	char *IP;
	struct node * next;
};

//the socket pool is simply a container for socket nodes that is shared by all of the threads (therefore also contains the mutexes and cond variable)
typedef struct {
	struct node* head;
	int empty;
	pthread_mutex_t *mutex;
	pthread_mutex_t *loglock;
	pthread_cond_t *condv;
}socket_pool;

struct thread_args{
	socket_pool* socks;
	FILE * log;
};

//takes as paramaters the sockpool (to lock), the client ip address, the file to write to, the request info, the code (200/404) and the total response size. it composes all of these things and writes them to the log file given, with a time stamp.
void log_data(socket_pool *sp,char*IP, FILE* log, char* req_info, int code, int resp_size){
	//time stamp calculation
  	struct timeval tv;
 	time_t curtime;
  	gettimeofday(&tv, NULL); 
  	curtime=tv.tv_sec;
	char* current_time = asctime(localtime(&curtime));
	current_time[strlen(current_time)-1]='\0';
	//compose string to write
	char temp[4096];
	sprintf(temp, "%s\t%s\t%s\t%d\t%d\n",IP,current_time,req_info,code,resp_size);
	//write string atomically	
	pthread_mutex_lock(sp->loglock)	;	
	fputs(temp, log);
	pthread_mutex_unlock(sp->loglock);	
}

//initializes and mallocs the socket pool components, including the mutexes and condition variable
void socket_pool_init(socket_pool *sp){
	sp->head  = NULL;
	sp->empty = 1;
	sp->mutex = malloc(sizeof(pthread_mutex_t));
	sp->loglock = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(sp->mutex,NULL);
	pthread_mutex_init(sp->loglock,NULL);
	sp->condv = malloc(sizeof(pthread_cond_t));
	pthread_cond_init(sp->condv,NULL);
}

//creates a socket node and adds it to the front of the linked list that is the socket pool (threadsafe)
void add_socket(socket_pool *sp, int socket, char* IPaddr){
	pthread_mutex_lock(sp->mutex);
	struct node *newnode = malloc(sizeof(struct node));
	newnode->socket = socket;
	newnode->next = sp->head;
	newnode->IP = IPaddr;
    sp->head = newnode;
	sp->empty = 0;
	pthread_cond_signal(sp->condv);
	pthread_mutex_unlock(sp->mutex);
}

//get next socket fills the given socket and IPaddress pointers with the information of the next socket available, and then removes that socket from the socket pool. this is not locked with mutexes because this function is called inside of a locked critical section.
int get_next_socket(socket_pool *sp, int* sock, char**IP){
	if(sp==NULL || sp->empty){
		return -1;
	}
	*sock = sp->head->socket;
	*IP = sp->head->IP;
	struct node *n = sp->head;
	sp->head = sp->head->next;
	free(n);
	if(sp->head==NULL){
		sp->empty = 1;
	}
	return 0;
}

//destroys mutexes and condition variables. nodes are already freed by get_next_socket()
void clear_socket_pool(socket_pool *sp){
	pthread_mutex_destroy(sp->mutex);
	free(sp->mutex);
	pthread_mutex_destroy(sp->loglock);
	free(sp->loglock);
	pthread_cond_destroy(sp->condv);
	free(sp->condv);
	free(sp);
}

void usage(const char *progname) {
    fprintf(stderr, "usage: %s [-p port] [-t numthreads]\n", progname);
    fprintf(stderr, "\tport number defaults to 3000 if not specified.\n");
    fprintf(stderr, "\tnumber of threads is 1 by default.\n");
    exit(0);
}

//waits for sockets to be added to the pool, then handles them. the only critical section is attaining the socket from the thread pool because once it has it, actions on that socket do not affect other sockets. 
//once a thread attains a socket, it obtains the request and checks if the requested files is present. if it is, it composes a return string consisting of the appropriate header (HTTP_200) and the file contents and sends the data back to the client
void *worker_thread(void * threadargs) {
	//initialize args to local variables	
	struct thread_args* args = threadargs;	
	socket_pool *sp =  args->socks;
	FILE* log = args->log;
	int sock;
	int code;
	char *IP;
	char *buffer;
	char *buffer2;
	char *returnstr;
	char *req_info;
	struct stat fileinfo;
	while(still_running){
		pthread_mutex_lock(sp->mutex);
		while(sp->empty){	//wait on condition variable if the socket pool is empty
			pthread_cond_wait(sp->condv, sp->mutex);
			if(!still_running){	//if thread is woken up and the server is done, unlock and return
				pthread_mutex_unlock(sp->mutex);
				return NULL;
			}
		}
		buffer= malloc(sizeof(char)*1024);
		buffer2= malloc(sizeof(char)*4024);
		get_next_socket(sp, &sock,&IP); //fill sock and IP local variables from the next available socket in the pool
		pthread_mutex_unlock(sp->mutex);
		if(0>getrequest(sock,buffer,1024)){ //if getrequest() fails, return and log a 404
			returnstr =malloc(sizeof(HTTP_404));
			sprintf(returnstr, HTTP_404);
			code = 404;
		}
		
		else {
			sprintf(buffer2,"./%s",buffer);
			if(0>stat(buffer2,&fileinfo)){	//if file does not exist, log a 404
				returnstr =malloc(sizeof(HTTP_404));
				sprintf(returnstr, HTTP_404);
				code = 404;
			}	
			else{	//if file does exist, compose return data
				int filesize = (int) fileinfo.st_size;
				char filecontents[filesize];
				int fd = open(buffer, O_RDWR);
				read (fd, filecontents, filesize);
				returnstr =malloc(sizeof(HTTP_200)+filesize);
				code = 200;
				sprintf(returnstr, HTTP_200, filesize);
				sprintf(returnstr, "%s%s", returnstr, filecontents);
				close(fd);
			}
		
			if(0>senddata(sock,returnstr,sizeof(returnstr))){	//send the data back to client//
				printf("Error: data could not be sent ");
			}	
			//log request
			req_info = malloc(1024+sizeof("GET "));
			sprintf(req_info, "\"GET %s\"",buffer);
			log_data(sp,IP, log,req_info,code, sizeof(returnstr));
			free(req_info);
			free(IP);
		}
		free(returnstr);
		free(buffer);
		free(buffer2);
		close(sock);
	}
	return NULL;
}



void runserver(int numthreads, unsigned short serverport) {
	
	//create socket pool 
    socket_pool *sockpool= malloc(sizeof(socket_pool));;
	socket_pool_init(sockpool);

    // create pool of threads
    pthread_t threads[numthreads];
    
	//create args to pass to threads
	FILE* log = fopen("weblog.txt", "a");
	struct thread_args* args = malloc(sizeof(struct thread_args));
	args->socks = sockpool;
	args->log = log;
	
	int i = 0;
    // start up the threads;
    for (i = 0; i < numthreads; i++) {
        if (0 > pthread_create(&threads[i], NULL, worker_thread, args)) {
            fprintf(stderr, "Error creating thread: %s\n", strerror(errno));
        }
    }
	//////////server code by Prof. Sommers  
    int main_socket = prepare_server_socket(serverport);
    if (main_socket < 0) {
        exit(-1);
    }
    signal(SIGINT, signal_handler);

    struct sockaddr_in client_address;
    socklen_t addr_len;

    fprintf(stderr, "Server listening on port %d.  Going into request loop.\n", serverport);
    while (still_running) {
        struct pollfd pfd = {main_socket, POLLIN};
        int prv = poll(&pfd, 1, 10000);

        if (prv == 0) {
            continue;
        } else if (prv < 0) {
            PRINT_ERROR("poll");
            still_running = FALSE;
            continue;
        }
        
        addr_len = sizeof(client_address);
        memset(&client_address, 0, addr_len);
        int new_sock = accept(main_socket, (struct sockaddr *)&client_address, &addr_len);
        if (new_sock > 0) {
            fprintf(stderr, "Got connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port)
			
			//pass socket and client address to socket pool for threads to process
			char * IPaddr= malloc(100);
			sprintf(IPaddr, "%s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
			IPaddr[strlen(IPaddr)-1] = '\0';
			add_socket(sockpool,new_sock,IPaddr);
			
        }
    }
    fprintf(stderr, "Server shutting down.\n");
	fclose(log);
	//join threads. broadcast to get them out of waiting for the condition variable
	pthread_cond_broadcast(sockpool->condv);
	for (i = 0; i < numthreads; i++) {
		pthread_join(threads[i], NULL);
	}
    clear_socket_pool(sockpool);
    close(main_socket);
	free(args);
}


int main(int argc, char **argv) {
    unsigned short port = 3000;
    int num_threads = 1;

    int c;
    while (-1 != (c = getopt(argc, argv, "p:t:h"))) {
        switch(c) {
            case 'p':
                port = atoi(optarg);
                if (port < 1024) {
                    usage(argv[0]);
                }
                break;

            case 't':
                num_threads = atoi(optarg);
                if (num_threads < 1) {
                    usage(argv[0]);
                }
                break;
            case 'h':
            default:
                usage(argv[0]);
                break;
        }
    }

    runserver(num_threads, port);
    
    fprintf(stderr, "Server done.\n");
    exit(0);
}
