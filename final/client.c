#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <getopt.h>


int read_query(char *queryFile);

int main(int argc, char *argv[]) {

    int opt;
    int id;
    char*idc;
    char *ipv4;
    int port;
    char *pathToQueryFile;

    while((opt = getopt(argc, argv, "i:a:p:o:")) != -1){
        switch(opt){
            case 'i': 
                idc=optarg;
                id= atoi(idc);
                if (id < 1)
                {
                    printf("Error!! :ID MUST BIGGER THAN OR EQUAL TO 1\n");
                    exit(2);
                }
                break;

            case 'a': 
                ipv4= optarg;
                break;

            case 'p': 
                port= atoi(optarg);
                break;
            
            case 'o': 
                pathToQueryFile= optarg;
                break;
            default :
                printf("USAGE: ./client –i id -a 127.0.0.1 -p PORT -o pathToQueryFile");
                return 1;
        }
    }
    int socket_fd;
    struct sockaddr_in serv_addr;
    char buffer[1024]= {0};

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ipv4);
    serv_addr.sin_port = htons ( port);


    FILE *fp = fopen(pathToQueryFile, "r");
    if (fp == NULL){
        perror("!!ERROR!! Couldn't open dataset file");
        exit(1);
    }
    char line[200];
    char num[10];
    size_t length= 200;
    int i=0; 
    char message[1024];
    char* finish="DONE" ;
    char *piece;
    char cat[1024];
    char *charptr;
    int querCount=0;
    clock_t t;
    

    if (-1 ==(socket_fd= socket(AF_INET, SOCK_STREAM, 0))){
    perror("SOCKET CREATION !! ");
    exit(EXIT_FAILURE);
    }
        
    if (connect(socket_fd , (struct sockaddr *)&serv_addr , sizeof(serv_addr)) == -1)
    {
        perror("CONNECT ERROR!! ");
        return 1;
    }
    fprintf(stdout,"Client-%d connecting to %s:%d\n",id,ipv4,port);
    fflush(stdout);
    while (fgets(line, length, fp) != NULL) {
//////////////////////
        bzero(message,1024);
        i=0;
        for ( i = 0; i < length; i++){
            num[i] = line[i];
            if (line[i] == ' ') {
                break;
            }
        }
        num[i]='\0';
        int k=0;
        i++;
        for (int j = i; j < length; j++)
        {
            message[k]=line[j];
            if (line[j] == '\0' || line[j] == '\n')
                break;
            k++; 
            
        }
        message[k]='\0';
        //
        int counter=0;
        while(message[counter] != '\0')
            ++counter;;
        if(!strcmp(num,idc)){
            ++querCount;
            t = clock();
            fprintf(stdout,"Client-%d connected and sending query '%s'\n",id,message);
            fflush(stdout);
            write(socket_fd, message, sizeof(message));    
            bzero(cat,1024);
            strcpy(cat,message);
            charptr=message;
            //write(socket_fd, cat, sizeof(cat));
            piece = strtok_r(charptr, " ", &charptr);
            if (!strcmp(piece, "SELECT")) {
                piece = strtok_r(charptr, " ", &charptr);
                if (!strcmp(piece, "*")){
                    t = clock() - t;
                    double time_taken = ((double)t)/CLOCKS_PER_SEC;
                    read(socket_fd, buffer, 1024);
                    int iter = atoi(buffer);
                    fprintf(stdout,"Server’s response to Client-%d is %s records, and arrived in %.1f seconds.\n",id,buffer,time_taken);
                    fflush(stdout);
                    bzero(buffer,1024);
              
                    
                    for(int i=0; i<iter;i++){
                    //printf("==%d==",iter);
                        fflush(stdout); 
                 
                        bzero(buffer,1024);
                        read(socket_fd, buffer, 1024); 
                        printf("%s",buffer);
                        fflush(stdout); 
                 
                    }
                 
                }else if (!strcmp(piece, "DISTINCT")){
                    printf("DISTINCT MESSAGE\n");
                    fflush(stdout);
                    //read(socket_fd, buffer, 1024);
                }else{
                    bzero(buffer,1024);
                    read(socket_fd, buffer, 1024);
                    t = clock() - t;
                    double time_taken = ((double)t)/CLOCKS_PER_SEC;
                    fprintf(stdout,"Server’s response to Client-%d is %s records, and arrived in %.1f seconds.\n",id,buffer,time_taken);
                    fflush(stdout);
                    int iter = atoi(buffer);
                    
                    for (int i = 0; i < iter; i++)
                    {
                        bzero(buffer,1024);
                        read(socket_fd, buffer, 1024);
                        printf("%s",buffer);
                    }
                }
                
            }else if (!strcmp(piece, "UPDATE")) {
                bzero(buffer,1024);
                read(socket_fd, buffer, 1024);
                int iter = atoi(buffer);
                bzero(buffer,1024);
                read(socket_fd, buffer, 1024);
                int record = atoi(buffer);
                for (int i = 0; i < iter; i++)
                {
                    bzero(buffer,1024);
                    read(socket_fd, buffer, 1024);
                    printf("%s",buffer);
                }

                t = clock() - t;
                double time_taken = ((double)t)/CLOCKS_PER_SEC;

                
                fprintf(stdout,"Server’s response to Client-%d is %d records updated, and arrived in %.1f seconds.\n",id,record,time_taken);
                fflush(stdout);
                bzero(buffer,1024);
            }else{
                printf("UNDETERMINED\n");
            }
            //bzero(buffer,1024);
            //read(socket_fd, buffer, 1024);
            //printf("%s", buffer);

        } 

    }

        write(socket_fd, finish, sizeof(finish));
        fprintf(stdout,"A total of %d queries were executed, client is terminating.\n",querCount);
        fflush(stdout);
        close(socket_fd);

    return 0;
}
int read_query(char *queryFile);