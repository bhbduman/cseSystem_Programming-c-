#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <getopt.h>
#include<arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <sys/types.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/stat.h>




#ifndef BECOME_DAEMON_H
#define BECOME_DAEMON_H

#define BD_NO_CHDIR 01
#define BD_NO_CLOSE_FILES 02
#define BD_NO_REOPEN_STD_FDS 04
#define BD_NO_UMASK0 010
#define BD_MAX_CLOSE 8192
int becomeDaemon();
#endif
sem_t *doubleInstt_sss;

sig_atomic_t sigusr1_count=0;
int finish_thread=0;
void handler (int signal_number){
	++sigusr1_count;
    ++finish_thread;
}
time_t sec;
#define SERVER_IP "74.125.235.20"
struct entry{
    char *entr;
    int cell_s;
};

    int row_s=0;
    int column_s=0;

struct entry **dataset;

//_________________________________________________//
//QUEUE
struct data {
	int cli;
	struct data *next;
};

struct thread{
    pthread_t id_t;
    struct data node;
    int id;
    int cli_sock;
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
void add_queue(struct queue *queue, int cli){

	struct data *temp;
	temp = malloc(sizeof(struct data));
    /*temp->data = (char*)malloc(sizeof(char)*size);
    fprintf(fp_logFile,"size: %d\n",size);
    int i;
    for ( i = 0; i < size; i++)
        temp->data[i]=data[i];
    temp->data[i]='\0';
    */
   temp->cli = cli;
    temp->next = NULL;

	if(queue->rear == NULL){
		queue->front = temp;
		queue->rear = temp;
	}else{
		queue->rear->next = temp;
		queue->rear = temp;
	}
}
void delete_queue(struct queue * queue) {
	if(queue->front == NULL )
		return;
	
	struct data *temp = queue->front;
	queue->front = queue->front->next; 

	if(queue->front == NULL)
		queue->rear = NULL;

	free(temp);
}
struct queue * qGlob;
pthread_mutex_t queue_mutex= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thread_mutex= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rw_mutex= PTHREAD_MUTEX_INITIALIZER;
int numberOfConnection_glob=0;
int numberOfThread_glob=0;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t thread_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t r_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t w_cond = PTHREAD_COND_INITIALIZER;
int AW=0;
int WW=0;
int WR=0;
int AR=0;
//_________________________________________________//

struct thread *threads;

FILE *fp_logFile;


int measure_dataset(const char *datasetPath);
int aloc_cell_dataset(const char *datasetPath);
int fill_dataset(const char *datasetPath);
int read_dataset(char* datasetPath);
void free_dataset();

void parse_execute(char* request, int socket, int id);
void select_operation(char* request,  int socket);
void select_update(char* request, int socket,int id);
void selectDisct_operation(char* request,  int socket);

void *server_poolthreads(void *args);


int main(int argc, char *argv[]) {
    if((doubleInstt_sss=sem_open("doubleInstt_sss", O_CREAT|O_RDWR|O_EXCL, 0666, 0)) == SEM_FAILED ){
        fprintf(stderr,"It's Already Created");
        exit(1);
    }
    becomeDaemon();
    struct sigaction sa;
    memset (&sa, 0, sizeof(sa));
    sa.sa_handler = &handler;
    if(sigaction(SIGINT, &sa, NULL) == -1){
        //free_dataset(dataset);
        perror("sigaction error");
       exit(1);
    }
    int opt;
    int port;
    char *pathToLogFile;
    int poolSize;
    char *datasetPath;
    qGlob = queue_init();
    while((opt = getopt(argc, argv, "p:o:l:d:")) != -1){
        switch(opt){
            case 'p': 
                port= atoi(optarg);
                break;

            case 'o': 
                pathToLogFile= optarg;
                break;

            case 'l': 
                poolSize= atoi(optarg);
                if (poolSize < 2)
                {
                    fprintf(fp_logFile,"Error!! :POOL SIZE MUST BIGGER THAN OR EQUAL TO 2\n");
                    exit(2);
                }
                break;
            
            case 'd': 
                datasetPath= optarg;
                break;
            default :

                sem_close(doubleInstt_sss);
                sem_unlink("doubleInstt_sss");
                fprintf(fp_logFile,"USAGE: ./server -p PORT -o pathToLogFile –l poolSize –d datasetPath");
                return 1;
        }
    }
    fp_logFile = fopen(pathToLogFile, "w");
    if (fp_logFile == NULL){
        perror("Couldn't open log file ");
        exit(1);
    }
    fprintf(fp_logFile,"Executing with parameters:\n-p %d\n-o %s\n-l %d\n-d %s\n", port, pathToLogFile, poolSize, datasetPath);

    read_dataset(datasetPath);
    //free_dataset();
    
    
     int socket_fd, new_socket_fd ;
   // const char* const socket_name;//??
    
    struct sockaddr_in serv_addr, cli_addr;
    //int addrlen = sizeof(serv_addr);
    
    if (-1 ==(socket_fd= socket(AF_INET, SOCK_STREAM, 0))){
        perror("SOCKET CREATION !! ");
        exit(EXIT_FAILURE);
    }
    int opti = 1;
    if( setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opti, sizeof(opti)) ) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons ( port);

    if (bind (socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1){
        perror("BIND ERROR!! ");
        exit(EXIT_FAILURE);
    }
    
    if(listen(socket_fd, 100)<0){
        perror("LISTEN");
        exit(EXIT_FAILURE);
    }
    
    socklen_t size_sockadr= sizeof(struct sockaddr_in);    
    

    threads = (struct thread* )malloc(sizeof(struct thread)* poolSize);
    for (int i = 0; i < poolSize; i++)
        threads[i].id=i;

    for (int i = 0; i < poolSize; i++) {
        int error2;
		if((error2 = pthread_create(&(threads[i].id_t),NULL,server_poolthreads,&threads[i]))){
			fprintf(stderr,"Failed to create thread:%s\n",strerror(error2));
			exit(EXIT_FAILURE);
		}
        
    }
    numberOfThread_glob = poolSize;
   
    while (1){
        if(sigusr1_count){
            //free_dataset(dataset);
            fprintf(fp_logFile,"Termination signal received, waiting for ongoing threads to complete.\n");
            break;          
        }
        pthread_mutex_lock(&thread_mutex);
            while(numberOfThread_glob<=0){
                fprintf(fp_logFile,"No thread is available! Waiting...\n");
                pthread_cond_wait(&thread_cond, &thread_mutex);
            }
            --numberOfThread_glob;
        pthread_mutex_unlock(&thread_mutex);
        if(sigusr1_count){
            //free_dataset(dataset);
            fprintf(fp_logFile,"Termination signal received, waiting for ongoing threads to complete.\n");
            break;          
        }
        new_socket_fd = accept(socket_fd, (struct sockaddr*)&cli_addr, &size_sockadr);
        if(sigusr1_count){
            //free_dataset(dataset);
            fprintf(fp_logFile,"Termination signal received, waiting for ongoing threads to complete.\n");
            break;          
        }

     
        pthread_mutex_lock(&queue_mutex);
            add_queue(qGlob,new_socket_fd);
        pthread_cond_signal(&queue_cond);
        pthread_mutex_unlock(&queue_mutex);
        if(sigusr1_count){
            //free_dataset(dataset);
            fprintf(fp_logFile,"Termination signal received, waiting for ongoing threads to complete.\n");
            break;          
        }
        
        
    }
    for (int i = 0; i < poolSize; i++)
    {
        
        pthread_detach(threads[i].id_t);
    }

    if(sigusr1_count){
        fprintf(fp_logFile,"All threads have terminated, server shutting down.\n");
        fclose(fp_logFile);
        free_dataset(dataset);
     
    }
    
    
    close(socket_fd);
    sem_close(doubleInstt_sss);
    sem_unlink("doubleInstt_sss");








    return 0;
}

void *server_poolthreads(void *args){
    struct thread *p = (struct thread*) args;
    int new_socket_fd;
    char buffer[1024] = {0};
    while (1)
    {   
        if(finish_thread)
            break;
        fprintf(fp_logFile,"Thread #%d: waiting for connection\n",p->id);
        pthread_mutex_lock(&queue_mutex);
        pthread_cond_wait(&queue_cond, &queue_mutex);
        
        fprintf(fp_logFile,"A connection has been delegated to thread id #%d\n",p->id);
        
            new_socket_fd= qGlob->front->cli;
            delete_queue(qGlob);
            
        pthread_mutex_unlock(&queue_mutex);
        if(finish_thread)
            break;
        bzero(buffer, 1024);
        read(new_socket_fd, buffer, 1024);
        while (strcmp(buffer,"DONE")) {
            fprintf(fp_logFile,"Thread #%d: received query '%s'\n",p->id,buffer);
            //fprintf(fp_logFile,"%s",buffer);
            parse_execute(buffer,new_socket_fd,p->id);
            ///operations
           
            bzero(buffer, 1024);
            read(new_socket_fd, buffer, 1024);
            fflush(fp_logFile);
        }
        
        usleep(500000);
        if(finish_thread)
            break;
        close(new_socket_fd);
        
        pthread_mutex_lock(&thread_mutex);
        
            ++numberOfThread_glob;
            pthread_cond_signal(&thread_cond);
        
        pthread_mutex_unlock(&thread_mutex);
        if(finish_thread)
            break;
       

    }
    


   return NULL; 
}
void parse_execute(char* request, int socket,int id){
   char * piece;
    if((piece = strtok_r(request, " ", &request))){
        if (!strcmp(piece, "SELECT")){
            pthread_mutex_lock(&rw_mutex);
                while((AW+WW) > 0){
                    WR++;
                    pthread_cond_wait(&r_cond,&rw_mutex);
                    WR--;
                }
            AR++;
            pthread_mutex_unlock(&rw_mutex);
            select_operation(request,socket);
             fprintf(fp_logFile,"Thread #%d: query completed, %d records have been returned\n",id,row_s);
            pthread_mutex_lock(&rw_mutex);
            AR--;
            if(AR ==0 && WW>0)
                pthread_cond_signal(&w_cond);
            pthread_mutex_unlock(&rw_mutex);
        }
        else if (!strcmp(piece, "UPDATE")){
            pthread_mutex_lock(&rw_mutex);
                while((AW+AR) > 0){
                    WW++;
                    pthread_cond_wait(&w_cond,&rw_mutex);
                    WW--;
                }
            AW++;
            pthread_mutex_unlock(&rw_mutex);
            select_update(request,socket,id);
            pthread_mutex_lock(&rw_mutex);
            AW--;
            if(WW >0)
                pthread_cond_signal(&r_cond);
            else if(WR >0)
                pthread_cond_broadcast(&r_cond);
            pthread_mutex_unlock(&rw_mutex);
        } 
        else{
           // perror("UNDETERMINED OPERATION");
        }
    }
    return;
}
void select_operation(char* request, int socket){
    char * piece;
    char whole[100];
    char cat[1024]={0};
    int columns[100]= {-1};
    char cmpstr[100];
    int k=0;
    while(request[k] != ';'){
        whole[k]=request[k];
        ++k; 
    }
    whole[k]='\0';
    if((piece = strtok_r(request, " ", &request))){
        if (!strcmp(piece, "DISTINCT"))
            selectDisct_operation(request,socket);
    }
    if (!strcmp(piece,"*") ){
        bzero(cat,1024);
        sprintf(cat,"%d",row_s-1);
        write(socket, cat, sizeof(cat) );
        bzero(cat,1024);
        for (int i = 0; i < row_s-1; i++){
            sec=time(NULL);
            sprintf(cat,"(%ld) ",sec);
            for (int j = 0; j < column_s; j++){
                strcat(cat,dataset[i][j].entr);
                strcat(cat," ");
            }
            strcat(cat,"\n");
            write(socket, cat, sizeof(cat) );        
            //fprintf(fp_logFile,"%s",cat);
            bzero(cat,1024);            
        }
    }
    else{
        
        strcpy(cmpstr,"EMPTY");
        fflush(fp_logFile);
        int i = 0;
        k=0;
        while(strcmp(cmpstr,"FROM")){            
            bzero(cmpstr,0);
            int j=0;
            if(whole[i] == ',' ||whole[i] ==' ')
                ++i;
            while(1){
                if(whole[i] == ',' ||whole[i] ==' ')
                    break;
                //fprintf(fp_logFile,">%c<",whole[i]);
                cmpstr[j]=whole[i];
                ++j;
                ++i;
            }
            cmpstr[j]='\0';  
                     
                for (int i = 0; i < column_s; i++){
                   // fprintf(fp_logFile,"---%s==%s==%d--\n",cmpstr,dataset[0][i].entr,strcmp(dataset[0][i].entr,cmpstr));   
                    if(!strcmp(dataset[0][i].entr,cmpstr)){
                        columns[k]=i;
                        ++k;
                        columns[k]=-1;
                       // fprintf(fp_logFile,"===%s===",dataset[0][i].entr);
                    }                   
                }
                
            
        }
        bzero(cat,1024);
        sprintf(cat,"%d",row_s-1);
        write(socket, cat, sizeof(cat) );
        bzero(cat,1024);
        for (int i = 0; i < row_s-1; i++)
        {
            int j=0;
            sec=time(NULL);
            sprintf(cat,"(%ld) ",sec);
            while(columns[j] != -1){
                strcat(cat,dataset[i][columns[j]].entr);
                strcat(cat," ");
                j++;
            }
            strcat(cat,"\n");
            write(socket, cat, sizeof(cat) );
            bzero(cat,1024);        
        }
    }
}
void selectDisct_operation(char* request, int socket){
    
    //write(socket, finish, strlen(finish) );
   // write(socket, finish, strlen(finish) );
}
void select_update(char* request, int socket,int id){
    
    int chanCol=-1;
    char whole[100];
    char wcolName[100];
    char wcolRes[150];
    int wcolPlace=-1;
    char word[150]={0};
    char buffer[1024]={0};
    int iter=0;
    int flag =0;
    char cat[1024]={0};
    int *rows= (int*)malloc(sizeof(int)*row_s);
    char * piece;
    int k=0;
    sprintf(buffer,"%d",row_s-1);
    write(socket, buffer, sizeof(buffer));
    bzero(buffer,1024);

    for (int i = 0; i < row_s; i++)
        rows[i]=-1;
    
    piece = strtok_r(request, " ", &request);
    piece = strtok_r(request, " ", &request);
    while(request[k] != ';'){
        whole[k]=request[k];
        ++k; 
    }
    whole[k]='\0';
    while (strcmp(piece, "WHERE"))
        piece = strtok_r(request, " ", &request);
    piece = strtok_r(request, " ", &request);
    k=0;
    while(piece[k]!='='){
        wcolName[k]=piece[k];
        ++k;
    }
    wcolName[k]='\0';
    for (int i = 0; i < column_s; i++){
        if(!strcmp(dataset[0][i].entr,wcolName)){
            wcolPlace=i;
        }                   
    }
    ++k;++k;
    int l=0;
    while(piece[k]!= 39){
        wcolRes[l]=piece[k];
        ++k;++l;
    }
    wcolRes[l]='\0';
    int m=0;
    for (int i = 0; i <row_s ; i++){
        if (!strcmp(dataset[i][wcolPlace].entr,wcolRes)) {
            if(flag ==0)
                ++iter;
            rows[m]=i;
            ++m;
        }     
    }
    flag=1;
    rows[m]=-1;
    m=0;
    l=0;
    while(strcmp(word,"WHERE")){
        l=0;
        while(whole[m] != '=' && whole[m] != ' '){
            word[l]=whole[m];
            ++l;++m;
        }
        word[l]='\0';
        if(!strcmp(word,"WHERE"))
            break;
        for (int i = 0; i <column_s ; i++)
            if (!strcmp(dataset[0][i].entr,word)){
                chanCol=i;
            }    

        bzero(word,150);
        ++m;++m;
        l=0;
        while(whole[m] != 39){
            word[l]=whole[m];
            ++l;++m;
        }
        word[l]='\0';
        k=0;
        
        while(rows[k] != -1){
            dataset[k][chanCol].entr = (char*)realloc(dataset[k][chanCol].entr,sizeof(word));
            strcpy(dataset[rows[k]][chanCol].entr,word);
        ++k;
        }
        
        ++m;
        
        while (whole[m] == ',' ||whole[m]==' ' )
        {
            ++m;   
        }
        
        bzero(word,150);
        
    }
   free(rows);


    sprintf(buffer,"%d",iter);
    write(socket, buffer, sizeof(buffer));
    bzero(buffer,1024);

    for (int i = 0; i < row_s-1; i++){
        sec=time(NULL);
        sprintf(cat,"(%ld) ",sec);
        for (int j = 0; j < column_s; j++){
            strcat(cat,dataset[i][j].entr);
            strcat(cat," ");
        }
        strcat(cat,"\n");
        write(socket, cat, sizeof(cat) );        
        //fprintf(fp_logFile,"%s",cat);
        bzero(cat,1024);            
    }
    fprintf(fp_logFile,"Thread #%d: query completed, %d records updated and have been returned\n",id,iter);
    
    

}

int read_dataset(char* datasetPath) {

    fprintf(fp_logFile,"Loading dataset...\n");
    clock_t t;
    t = clock();

    measure_dataset(datasetPath);
    column_s+=1;
    row_s+=1;
    dataset= (struct entry **)malloc(row_s* sizeof(struct entry *));
    for(int i =0; i<row_s; i++)
        dataset[i] = (struct entry *)malloc(column_s *sizeof(struct entry ));
    aloc_cell_dataset(datasetPath);
    for (int i = 0; i < row_s; i++)
        for (int j = 0; j < column_s; j++)
            dataset[i][j].entr = (char*)malloc(dataset[i][j].cell_s * sizeof(char));

    fill_dataset(datasetPath);

    t = clock() - t;
    double time_taken = ((double)t)/CLOCKS_PER_SEC;
    
    fprintf(fp_logFile,"Dataset loaded in %.3f seconds with %d records.\n",time_taken,row_s);
    fflush(fp_logFile);
    return 0;
}
int fill_dataset(const char *datasetPath){
    FILE *fp = fopen(datasetPath, "r");
    if (fp == NULL){
        perror("!!ERROR!! Couldn't open dataset file");
        exit(1);
    }
    char ch;
    int i=0;
    int j=0;
    int k=0;
    int counter=0;
    int flag=0;
    ch = fgetc(fp);
    if(ch == '"'){
        if(flag == 0) flag=1;
        else flag=0;
    }
    while(ch !=EOF){
        if ( ch>=32 && ch<=126 ) {
            while((ch != '\n' && ch != EOF) || flag == 1){
                if(ch == 44){
                    ch = fgetc(fp);
                    if(ch == '"'){
                        if(flag == 0) flag=1;
                        else flag=0;
                    }
                    j++;    
                }
                
                k=0;
                while(((ch != 44 && ch != '\n') && ch != EOF) || flag == 1) {
                    dataset[i][j].entr[k]=ch;
                    ch = fgetc(fp);
                    if(ch == '"'){
                        if(flag == 0) flag=1;
                        else flag=0;
                    }

                    
                    
                    k++;
                }
                dataset[i][j].entr[k]='\0';
                k=0;
            }  
            counter++;
        }
        if (ch == '\n') {
            if(flag == 0){
                i++;
                j=0;

                ch = fgetc(fp);
                if(ch == '"'){
                    if(flag == 0) flag=1;
                    else flag=0;
                }
            }
        }
        
         
    }

    fclose(fp);


    return 0;
}
int aloc_cell_dataset(const char *datasetPath){
    FILE *fp = fopen(datasetPath, "r");
    if (fp == NULL){
        perror("!!ERROR!! Couldn't open dataset file");
        exit(1);
    }
    char ch;
    int i=0;
    int j=0;
    int k=0;
    int counter=0;
    int flag=0;
    ch = fgetc(fp);
    if(ch == '"'){
        if(flag == 0) flag=1;
        else flag=0;
    }
    while(ch !=EOF){
        if ( ch>=32 && ch<=126 ) {
            while((ch != '\n' && ch != EOF) || flag == 1){
                if(ch == 44){
                    ch = fgetc(fp);
                    if(ch == '"'){
                        if(flag == 0) flag=1;
                        else flag=0;
                    }
                    j++;
                       
                }   
                dataset[i][j].cell_s=0; 
                k=0;
                while(((ch != 44 && ch != '\n') && ch != EOF) || flag == 1) {
                    ++dataset[i][j].cell_s;
                    ch = fgetc(fp);
                    if(ch == '"'){
                        if(flag == 0) flag=1;
                        else flag=0;
                    }

                    
                    
                    k++;
                }
                ++dataset[i][j].cell_s;
                k=0;
            } 
            counter++;
        }
        if (ch == '\n') {
            if(flag == 0){
                i++;
                j=0;

                ch = fgetc(fp);
                if(ch == '"'){
                    if(flag == 0) flag=1;
                    else flag=0;
                }
            }
        }
        
         
    }

    fclose(fp);


    return 0;
}
int measure_dataset(const char *datasetPath){
    FILE *fp = fopen(datasetPath, "r");
    if (fp == NULL){
        perror("!!ERROR!! Couldn't open dataset file");
        exit(1);
    }
    char ch;
    int i=0;
    int j=0;
    int k=0;
    int counter=0;
    int flag=0;
    int flag2=0;
    ch = fgetc(fp);
    if(ch == '"'){
        if(flag == 0) flag=1;
        else flag=0;
    }
    while(ch !=EOF){
        if ( ch>=32 && ch<=126 ) {
            while((ch != '\n' && ch != EOF) || flag == 1){
                if(ch == 44){
                    ch = fgetc(fp);
                    if(ch == '"'){
                        if(flag == 0) flag=1;
                        else flag=0;
                    }
                    if(flag2==0)
                        ++column_s;
                    j++;    
                }    
                k=0;
                while(((ch != 44 && ch != '\n') && ch != EOF) || flag == 1) {
                    ch = fgetc(fp);
                    if(ch == '"'){
                        if(flag == 0) flag=1;
                        else flag=0;
                    }

                    
                    
                    k++;
                }
                k=0;
            }
            counter++;
        }
        if (ch == '\n') {
            if(flag == 0){
                ++row_s;
                i++;
                j=0;
                flag2=1;

                ch = fgetc(fp);
                if(ch == '"'){
                    if(flag == 0) flag=1;
                    else flag=0;
                }
            }
        }
        
         
    }

    fclose(fp);

    return 0;
}
void free_dataset(){

    fflush(fp_logFile);
    for (int i = 0; i < row_s-1; i++){
        for (int j = 0; j < column_s; j++){
            free(dataset[i][j].entr);;
        }
    }
    for (int i = 0; i < row_s-1; i++){
        free(dataset[i]);;   
    }
    free(dataset);
    while(qGlob->front != NULL)
        delete_queue(qGlob);
    free(qGlob->front);
    free(threads);
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&thread_mutex);

}

int becomeDaemon()
{
    int maxfd, fd;
    switch (fork()) {
        case -1: 
            return -1;
        case 0: 
            break;
        default:
            _exit(EXIT_SUCCESS);
    }
    if (setsid() == -1)
        return -1;
    switch (fork()) {
        case -1:
            return -1;
        case 0:
            break;
        default:
            _exit(EXIT_SUCCESS);
    }
    umask(0);

    maxfd = sysconf(_SC_OPEN_MAX);
    if (maxfd == -1)
    maxfd = BD_MAX_CLOSE;
    for (fd = 0; fd < maxfd; fd++)
        close(fd);

    

    return 0;
}
