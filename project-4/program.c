#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#define STUDENT_NUMBER 100

sig_atomic_t sigusr1_count=0;

void handler (int signal_number){
	++sigusr1_count;
}

sem_t mutex;
sem_t mutex2;
sem_t mutex3;
sem_t thrd_ready;
sem_t mutex5;
sem_t s_st_choic;
sem_t s_halt;
sem_t s_snum;
sem_t s_stdentrd;
sem_t s_choic;
sem_t s_hws;

int student_money;
int studentNumber;
int homeworks;
int minCost;
struct queue * qGlob;



struct data {
	char data;
	struct data *next;
};

struct queue {
	struct data* front;
	struct data* rear;
};

struct queue * queue_init() {
	struct queue *que = (struct queue*)malloc(sizeof(struct queue));
	que->front = que->rear = NULL;
	return que;
}

void add_queue(struct queue *queue, char data){

	struct data *temp;
	temp = malloc(sizeof(struct data));
	temp->data = data;
	temp->next = NULL;


	if(queue->rear == NULL){
		queue->front = temp;
		queue->rear = temp;
	}else{
		queue->rear->next = temp;
		queue->rear = temp;
	}
}

struct read_hw_params {
	char*homework_file_path;
	pthread_t *h_detach;	
};

struct studentHired {
	pthread_t student_for_hire;
	char name[20];
	int quality;
	int cost;
	int speed;
	sem_t free;
	int hiredMoney;
	int quantity;
	int busy;
	char hw;
};

struct student_hired_params {
	struct studentHired *student_hired;
	int index;
	char homework_type;
};



void delete_queue(struct queue * queue) {
	if(queue->front == NULL )
		return;
	
	struct data *temp = queue->front;
	queue->front = queue->front->next; 

	if(queue->front == NULL)
		queue->rear = NULL;

	free(temp);
}

struct studentHired students[STUDENT_NUMBER];
int finishThreads=0;

void* read_homework(void *parameters);
void read_students(struct studentHired *students, char *students_file_path);
void* student_for_hired(void*arg);

int main(int argc, char *argv[]) {

	struct sigaction sa;
	memset (&sa, 0, sizeof(sa));
	sa.sa_handler = &handler;

	if(sigaction(SIGINT, &sa, NULL) == -1){
		perror("sigaction error");
		exit(-1);
	}

	char *homework_file_path;
	char *students_file_path;

	sem_init(&mutex,0,0);
	sem_init(&mutex2,0,1);
	sem_init(&mutex3,0,0);
	sem_init(&thrd_ready,0,0);
	sem_init(&mutex5,0,0);
	sem_init(&s_st_choic,0,1);
	sem_init(&s_halt,0,0);
	sem_init(&s_snum,0,0);
	sem_init(&s_stdentrd,0,1);
	sem_init(&s_choic,0,1);
	sem_init(&s_hws,0,0);
	
	if (argc !=4) {
		fprintf(stderr, "Argument number is missing!!! Usage: ./program homeworkFilePath studentsFilePath money \n");
		return 1;
	}
	if(!atoi(argv[3])) {
		fprintf(stderr, "Insufficient or invalid money\n");
		return 1;
	}
	
	qGlob = queue_init();
	student_money = atoi(argv[3]);
	students_file_path = argv[2]; 
	pthread_t h;
	struct read_hw_params threadH_args;
	homework_file_path=argv[1];

	student_money = atoi(argv[3]);


	read_students(students,students_file_path);

	for (int i = 0; i < studentNumber; i++)
	{
		
		int error2;
		if((error2 = pthread_create(&(students[i].student_for_hire),NULL,student_for_hired,&students[i]))){
			fprintf(stderr,"Failed to create thread:%s\n",strerror(error2));
			exit(EXIT_FAILURE);
		}

	}

	sem_wait(&mutex3);

	threadH_args.homework_file_path= homework_file_path;
	threadH_args.h_detach=&h;
	int error;

	if((error = pthread_create(&h,NULL,read_homework,&threadH_args))){
		fprintf(stderr,"Failed to create thread:%s\n",strerror(error));
	}

	

	
	while (1) {

		sem_wait(&s_snum);
		sem_wait(&mutex);
		char hw;
		if(qGlob->front != NULL){
			hw = qGlob->front->data;
			delete_queue(qGlob);
		}
		if(sigusr1_count){
			fprintf(stdout,"Termination signal received, closing.\n");
			sem_destroy(&mutex);
			sem_destroy(&mutex2);
			sem_destroy(&mutex3);
			sem_destroy(&thrd_ready);
			sem_destroy(&mutex5);
			sem_destroy(&s_st_choic);
			sem_destroy(&s_halt);
			sem_destroy(&s_snum);
			sem_destroy(&s_stdentrd);
			sem_destroy(&s_choic);	
			while(qGlob->front != NULL)
				delete_queue(qGlob);
			free(qGlob->front);
			return 1;            
    	}


		struct studentHired temp;
		int index;
		for (int i = 0; i < studentNumber; i++)
		{
			if(students[i].busy != 1){
				index=i;
				temp = students[i];
				break;
			}
		}
		
		
		for (int i = 0; i < studentNumber; i++){
			if (hw == 'Q'){
				if(students[i].quality >= temp.quality && students[i].busy !=1 && (student_money - students[i].cost >= 0 )){
					index=i;
					temp = students[i];
				}
			}else if (hw == 'C'){	
				if(students[i].cost <= temp.cost && students[i].busy !=1 && (student_money - students[i].cost >= 0 )){
					index=i;
					temp = students[i];
				}
				
			}else if (hw == 'S'){
				if(students[i].speed >= temp.speed && students[i].busy !=1 && (student_money - students[i].cost >= 0 )){
					index=i;
					temp = students[i];
				}
				
			}else{
				perror("Error invalid char");
				exit(1);
			}			
		}
		if(student_money- students[index].cost < 0 || homeworks == 0 ){
			
			if(student_money- students[index].cost < 0)
				fprintf(stdout, "H has no more money for homeworks, terminating.\n");
			else
				fprintf(stdout, "No more homeworks left or coming in, closing.\n");
			
			sem_wait(&s_choic);
			finishThreads=1;
			sem_post(&s_choic);

			/*for (int i = 0; i < studentNumber; i++){
				sem_post(&students[i].free);
			}*/
			for (int i = 0; i < studentNumber; i++)
			{
				pthread_kill(students[i].student_for_hire, SIGINT);
			}
			

			break;
		}
		sem_wait(&s_st_choic);
			students[index].busy=1;
			students[index].quantity += 1;
			students[index].hiredMoney += students[index].cost;
			homeworks--;
			student_money -= students[index].cost;
			students[index].hw=hw;
			sem_post(&students[index].free);
		sem_post(&s_st_choic);
		
			sem_wait(&mutex5);

		if( homeworks == 0 ){
			
			if(student_money- students[index].cost < 0)
				fprintf(stdout, "H has no more money for homeworks, terminating.\n");
			else
				fprintf(stdout, "No more homeworks left or coming in, closing.\n");
			
			sem_wait(&s_choic);
			finishThreads=1;
			sem_post(&s_choic);

			/*for (int i = 0; i < studentNumber; i++){
				sem_post(&students[i].free);
			}*/
			for (int i = 0; i < studentNumber; i++)
			{
				pthread_kill(students[i].student_for_hire, SIGINT);
			}
			break;
		}
		
	}
	
	
	for (int i = 0; i < studentNumber; i++)
	{
		pthread_join(students[i].student_for_hire,NULL);
	}
	int cost=0;
	int hnum=0;
	fprintf(stdout,"\n\nHomeworks solved and money made by students:\n");
	for (int i = 0; i < studentNumber; i++)
	{	
		fprintf(stdout,"%s %d %d\n",students[i].name, students[i].quantity, students[i].hiredMoney);
		cost+=students[i].hiredMoney;
		hnum+=students[i].quantity;
	}
	fprintf(stdout,"Total cost for %d homeworks %dTL\n",hnum,cost);
	fprintf(stdout,"Money left at G's account: %dTL\n",student_money);

	sem_destroy(&mutex);
	sem_destroy(&mutex2);
	sem_destroy(&mutex3);
	sem_destroy(&thrd_ready);
	sem_destroy(&mutex5);
	sem_destroy(&s_st_choic);
	sem_destroy(&s_halt);
	sem_destroy(&s_snum);
	sem_destroy(&s_stdentrd);
	sem_destroy(&s_choic);	
	while(qGlob->front != NULL)
		delete_queue(qGlob);
	free(qGlob);

	return 0;

}

void* student_for_hired(void *arg) {
	struct studentHired *p = (struct studentHired*) arg;
	while(1){

		fprintf(stdout,"%s is waiting for a homework\n",p->name);
		//sem_wait(&thrd_ready);
		
		sem_wait(&(p->free));
		
		if(finishThreads==1){
			return NULL;
		}
		
		int time_to_sleep=6-p->speed;
		sem_wait(&s_st_choic);
		fprintf(stdout,"%s is solving homework %c for %d, H has %d left.\n",p->name, p->hw, p->cost,student_money);
		sem_post(&s_st_choic);
		sem_post(&mutex5);
	
		sleep(time_to_sleep);
		sem_wait(&s_st_choic);
		p->busy=0;
		sem_post(&s_st_choic);
		sem_post(&s_snum);

		if(finishThreads==1 || homeworks ==0){
			return NULL;
		}

	}
	return NULL;


}
void* read_homework(void* arg) {
	struct read_hw_params *p = (struct read_hw_params*)arg;
	
    char ch;
	FILE * fileH = fopen(p->homework_file_path, "r");
    if (!fileH) {
        fprintf(stderr, "File Error!!");
		//sem_post(&mutex);
        exit(EXIT_FAILURE);
    }
	while(1) {
		do{
			ch = fgetc(fileH);
		}
		while(ch!=EOF && (ch != 'C' && ch != 'S' && ch != 'Q' ));

		if(ch == EOF){
			fprintf(stdout, "H has no other homeworks, terminating.\n");
			sem_post(&mutex);
			fclose(fileH);
			if(pthread_detach(*(p->h_detach)))
				fprintf(stderr,"COULDnt detached thread h");
			return NULL;
		}
		if(student_money <= 0) {
			fprintf(stdout, "H has no more money for homeworks, terminating.\n");
			fclose(fileH);
			sem_post(&mutex);
			if(pthread_detach(*(p->h_detach)))
				fprintf(stderr,"COULDnt detached thread h");
			return NULL;
		}
	
		add_queue(qGlob,ch);
		homeworks++;
		sem_post(&mutex);
		fprintf(stdout, "H has a new homework %c; remaining money is %dTL\n", ch, student_money);
		
	}
	
	
	return NULL;
}
void read_students(struct studentHired *students, char *students_file_path) {

	FILE *file = fopen(students_file_path, "r");
	if (!file) {
        fprintf(stderr, "File Error!!");
        exit(EXIT_FAILURE);
    }
	char line[100];
	int i=0;
	while(fgets(line, 100, file) !=NULL){
		sscanf(line, "%s %d %d %d", students[i].name, &students[i].quality, &students[i].speed, &students[i].cost);
		
		//herrr
		sem_init(&(students[i].free), 0, 0);
		students[i].hiredMoney=0;
		studentNumber++;
		sem_post(&s_snum);
		i++;
		
	}
	fclose(file);
	fprintf(stdout,"%d students-for-hire threads have been created.\nName Q S C\n",studentNumber);
	for (int i = 0; i < studentNumber; i++) {
		fprintf(stdout,"%s %d %d %d\n",students[i].name,students[i].quality,students[i].speed, students[i].cost);
	}
	sem_post(&mutex3);

	return;
}
