#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

//args variables
int N;
int P;
int C;
int X;
int PTime;
int CTime;
//extra consumer variables
int CX;
int extra;
int distribution;
int overconsumed;
//arrays
int *buffer;
int *pArray;
int *cArray;
//semaphores
sem_t full;
sem_t empty;
//mutex
pthread_mutex_t lock;
pthread_mutex_t newLock;
//trackers
int in = 0;
int out = 0;
int pIndex = 0;
int cIndex = 0;
//items added
int items = 0;
//Threads
struct ThreadArgs{
	pthread_t tid;
	int id;
};
/* 
 * Function to remove item.
 * Item removed is returned
 */
int dequeue_item()
{
    int item = buffer[out];
    out = (out+1)%N;
    return item;
}
/* 
 * Function to add item.
 * Item added is returned.
 * It is up to you to determine
 * how to use the ruturn value.
 * If you decide to not use it, then ignore
 * the return value, do not change the
 * return type to void. 
 */
int enqueue_item(int item)
{
	//taken from ch5 slide
	buffer[in] = item;
	in = (in+1)%N;
	return item;
}

void *producer(void *arg){
	struct ThreadArgs* args = (struct ThreadArgs*) arg;
	for(int i = 0; i < X; i++){
		sem_wait(&empty);
		pthread_mutex_lock(&lock);	
		items++;
		pArray[pIndex++] = items;
		printf("%d was produced by producer-> %d\n", items, args->id);
			
		if(enqueue_item(items) == 0){
			printf("Something went wrong...\n");
			exit(0);
		}
		pthread_mutex_unlock(&lock);
		sem_post(&full);
		sleep(PTime);
	}
}

void *consumer(void *arg){
	struct ThreadArgs* args = (struct ThreadArgs*) arg;
	//Deal with the case of overflow and set the first consumer thread to handle the overconsumption
	if(distribution != 0 && args->id == 1){
		for(int i = 0; i < overconsumed; i++){
			sem_wait(&full);
			pthread_mutex_lock(&lock);
			int temp = dequeue_item();
			//extra lock to stop consumer array from manipulating global temp
			pthread_mutex_lock(&newLock);
			cArray[cIndex++] = temp;
			pthread_mutex_unlock(&newLock);
			printf("%d was consumed by consumer-> %d\n", temp, args->id);
			pthread_mutex_unlock(&lock);
			sem_post(&empty);
			sleep(CTime);		
		}
	}else{
		for(int i = 0; i < CX; i++){
			sem_wait(&full);
			pthread_mutex_lock(&lock);
			int temp = dequeue_item();
			//extra lock to stop consumer array from manipulating global temp
			pthread_mutex_unlock(&lock);
			sem_post(&empty);
			pthread_mutex_lock(&newLock);
			cArray[cIndex++] = temp;
			pthread_mutex_unlock(&newLock);
			printf("%d was consumed by consumer-> %d\n", temp, args->id);
			sleep(CTime);		
		}
	}
}

int compare(const void *a, const void *b){
	return (*(int*)a - *(int*)b );
}

void print(){
	int size = P*X;
	int equal = 1;
	//for printing
	printf("%-15s%-4s%-4s\n","Producer Array", "|", "Consumer Array");
	for(int i = 0; i < size; i++){
		printf("%-15d%-4s%-4d\n",pArray[i], "|", cArray[i]);
	}
	//for comparison
	for(int i = 0; i < size; i++){
		if(pArray[i] != cArray[i]){
			equal = 0;
			break;
		}
	}
	if(equal){
		printf("\nConsumer and Producer Arrays Match!\n");
	}else{
		printf("\nConsumer and Producer Arrays Do Not Match!\n");
	}
}

int main(int argc, char** argv) {
	//get time and date
	time_t start, stop;
	start = time(NULL);
	printf("Current Time: %s", ctime(&start));
	//check for valid arguments
	if(argc != 7){
		printf("Insufficient arguments\n");
		return -1;
	}
	//args variables declaration
	N = atoi(argv[1]);
	P = atoi(argv[2]);
	C = atoi(argv[3]);
	X = atoi(argv[4]);
	PTime = atoi(argv[5]);
	CTime = atoi(argv[6]);
	//Check for invalid arguments
	if(N <= 0 || P <= 0 || C <= 0 || X <= 0){
		printf("Arguments must be greater than zero\n");
		return -1;
	}
	//extra consumer variables declaration
	CX = (X*P)/C;
	extra = 0;
	distribution = (X*P)%C;
	//There's always 1 extra thread handling the overconsumption
	if(distribution != 0){
		extra = 1;
		overconsumed = CX + distribution;
	}else{
		overconsumed = 0;
	}
	//print based on sample output
	printf("\n");
	printf("%42s %4d\n","Number of Buffers:", N);
	printf("%42s %4d\n","Number of Producers:", P);
	printf("%42s %4d\n","Number of Consumers:", C);
	printf("%42s %4d\n","Number of items Produced by each producer:", X);
	printf("%42s %4d\n","Number of items consumed by each consumer:", CX);
	printf("%42s %4d\n","Over consume on?:", extra);
	printf("%42s %4d\n","Over consume amount:", overconsumed);
	printf("%42s %4d\n","Time each Producer Sleeps (seconds):", PTime);
	printf("%42s %4d\n","Time each Consumer Sleeps (seconds):", CTime);
	printf("\n");
	//initialize arrays
	buffer = (int*)malloc(sizeof(int)*N);
	pArray = (int*)malloc(sizeof(int)*P*X);
	cArray = (int*)malloc(sizeof(int)*P*X);
	//initialize semaphores
	if(sem_init(&full, 0, 0) < 0){
		printf("Error initializing semaphore\n");
	}
	if(sem_init(&empty, 0, N) < 0){
		printf("Error initializing semaphore\n");
	}
	//spawn threads
	struct ThreadArgs pro[P];
	struct ThreadArgs con[C];
	//initialize mutex
	if(pthread_mutex_init(&lock, NULL) != 0){
		printf("Cannot initialize mutex\n");
	}
	if(pthread_mutex_init(&newLock, NULL) != 0){
		printf("Cannot initialize mutex\n");
	}
	for(int i = 0; i < P; i++){
		pro[i].id = i+1;
		if(pthread_create(&pro[i].tid, NULL, producer, (void*)&pro[i]) < 0){
			printf("Error Creating Thread..\n");
		}		
	}
	for(int i = 0; i < C; i++){
		con[i].id = i+1;	
		if(pthread_create(&con[i].tid, NULL, consumer, (void*)&con[i]) < 0){
			printf("Error Creating Thread..\n");
		}
	}
	for(int i = 0; i < P; i++){
		if(pthread_join(pro[i].tid, NULL) != 0){
			printf("Error joining Threads\n");
			exit(0);
		}	
		printf("Producer Thread joined: %d\n", i+1);
	}
	for(int i = 0; i < C; i++){
		if(pthread_join(con[i].tid, NULL) != 0){
			printf("Error joining Threads\n");
			exit(0);
		}	
		printf("Consumer Thread joined: %d\n", i+1);
	}
	stop = time(NULL);
	printf("Current Time: %s", ctime(&stop));
	//array sorting
	int size = P*X;
	qsort(pArray, size, sizeof(int), compare);
	qsort(cArray, size, sizeof(int), compare);
	printf("\n");
	print();
	printf("\n");
	printf("Total Runtime: %f secs.\n", difftime(stop,start));
	//free data structures to prevent memory leak
	pthread_mutex_destroy(&lock);
	free(buffer);
	free(pArray);
	free(cArray);
	sem_destroy(&full);
	sem_destroy(&empty);
}