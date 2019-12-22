#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
/**
 * THESE DEFINE VALUES CANNOT BE CHANGED.
 * DOING SO WILL CAUSE POINTS TO BE DEDUCTED
 * FROM YOUR GRADE
 */
 /** BEGIN VALUES THAT CANNOT BE CHANGED */
#define MAX_THREADS 16
#define MAX_ITERATIONS 40
/** END VALUES THAT CANNOT BE CHANGED */


/**
 * use this struct as a parameter for the function
 * nanosleep. 
 * For exmaple : nanosleep(&ts, NULL);
 */
struct timespec ts = {0, 123456 };
int global = 0;
pthread_mutex_t lock;

struct ThreadArgs{
	pthread_t tid;
	int id;
	int count;
};

void* Adder(void* arg){
	struct ThreadArgs* args = (struct ThreadArgs*) arg;
	for(int i = 0; i < MAX_ITERATIONS; i++){
		//placing nanosleep outside the loop iteration somehow causes many 0s
		pthread_mutex_lock(&lock);
		int temp = global;
		nanosleep(&ts,NULL);
		temp+=3;
		global = temp;
		pthread_mutex_unlock(&lock);
		printf("Current Value written to Global Variables by ADDER thread id: %d is %d\n", args->id, temp);
	}
	return NULL;
}

void* Subtracter(void* arg){
	struct ThreadArgs* args = (struct ThreadArgs*) arg;
	for(int i = 0; i < MAX_ITERATIONS; i++){
		//placing nanosleep outside the loop iteration somehow causes many 0s
		pthread_mutex_lock(&lock);
		int temp = global;
		nanosleep(&ts,NULL);
		temp-=3;
		global = temp;
		pthread_mutex_unlock(&lock);
		printf("Current Value written to Global Variables by SUBTRACTER thread id: %d is %d\n", args->id, temp);
	}
	return NULL;
}

int main(int argc, char** argv)
{
	struct ThreadArgs thread_info[MAX_THREADS];
	int ret_val;
	if(pthread_mutex_init(&lock, NULL)){
		printf("cannot initialize mutex\n");
		return -1;
	}
	//Even = add, Odd = subtract
	for(int i = 0; i < MAX_THREADS; i++){
		thread_info[i].id = i+1;
		thread_info[i].count = 10-i;
		//spawns adder method when iteration is even
		if(i%2 == 0){
			ret_val = pthread_create(&thread_info[i].tid, NULL, Adder, (void*)&thread_info[i]);
		}else if(i%2 != 0){
			ret_val = pthread_create(&thread_info[i].tid, NULL, Subtracter, (void*)&thread_info[i]);
		}
		if(ret_val < 0){
			perror("Error Creating Thread..");
			return -2;
		}
	}

	for(int j = 0; j < MAX_THREADS; j++){
		ret_val = pthread_join(thread_info[j].tid, NULL);
		if(ret_val){
			perror("Error joining Thread..");
			return -3;
		}
	}
	pthread_mutex_destroy(&lock);
	printf("Final Value of Shared Variable : %d\n", global);
    return 0;
}
