//Includes
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdlib.h>

//Defines
#define __NR_sys_cs1550_sem_init 325
#define __NR_sys_cs1550_down 326
#define __NR_sys_cs1550_up 327

//TypeDef
typedef struct{
    struct task_struct *task;
    struct cs1550_node *next;
}cs1550_node;

typedef struct{ 
    int value; 
    cs1550_node *list;
}cs1550_sem;


//Function Prototypes
void sem_init(cs1550_sem *sem, int val);
void up(cs1550_sem *sem);
void down(cs1550_sem *sem);


int main( int argc, const char* argv[] ){

	int num_p, num_c, size_buf, size_mem;
	int pid,i;
	int	*buffer;
	int *send;
	int *receive;
	cs1550_sem *mutex,*empty,*full;
	cs1550_sem *shared;

	if(argc == 4){
		//Get number of Producers and Consumers
		num_p = strtol(argv[1], NULL, 10);
		num_c = strtol(argv[2], NULL, 10);
		
		//Get size of the Buffer
		size_buf = strtol(argv[3], NULL, 10);
		
		size_mem = (size_buf+2)*sizeof(int) + 4*sizeof(cs1550_sem);
		
		//Allocate memory for shared data
		shared = (cs1550_sem *)mmap(NULL, size_mem, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
		mutex = shared;
		empty = mutex + 1;
		full = empty + 1;
		
		send = (int *)(full + 1);
		receive = send+1;
		buffer = receive + 1;
		*send = 0;
		*receive = 0;
		
		
		//Initialize Semaphores with Syscall
		sem_init(mutex,1);
		sem_init(empty,size_buf);
		sem_init(full,0);
	}
	
	else{
		perror("invalid argument error ");
		return -1;
	}
	
	pid = fork();
	
	if(pid == 0){
		
		int item;
		int chef = 65;
		
		if(num_p > 1){
		
			pid = fork();
			
			if(pid == 0){
				chef++;
			}
			
			for(i = 2; i < num_p; i++){
				if(pid == 0){
					pid = fork();
					chef++;
				}
			}
		}
	
		//---------------PRODUCER---------------
    	while (1){
    		down(empty);
    		down(mutex);
    		item = *send;
    		buffer[item] = item;
    		*send = (*send + 1) % size_buf;
    		sleep(3);
    		up(mutex);
    		up(full);
    		printf("Chef %c produced: Pancake%d\n",chef,item);
   		}	
		//---------------PRODUCER---------------
	
	}
	
	else{
	
		int item;
		char customer = 65;
	
		if(num_c > 1){
		
			pid = fork();
			
			if(pid == 0){
				customer++;
			}
		
			for(i = 2; i < num_c; i++){
				if(pid == 0){
					pid = fork();
					customer++;
				}
			}
		}
		
		//---------------CONSUMER---------------
    	while (1){
    		down(full);
    		down(mutex);
    		item = buffer[*receive];
    		*receive = (*receive + 1) % size_buf;
    		sleep(3);
    		up(mutex);
    		up(empty);
    		printf("Customer %c consumed: Pancake%d \n",customer,item);
   		}	
		//---------------CONSUMER---------------
	}
	
}


void sem_init(cs1550_sem *sem, int val){
syscall(__NR_sys_cs1550_sem_init, sem, val);
}

void up(cs1550_sem *sem){
syscall(__NR_sys_cs1550_up, sem);
}

void down(cs1550_sem *sem){
syscall(__NR_sys_cs1550_down, sem);
}
