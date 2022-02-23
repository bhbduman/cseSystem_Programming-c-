#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>

sig_atomic_t sigusr1_count= 0;
void handler (int signal_number) {
    ++sigusr1_count;
}


sem_t *empty;
sem_t *mutex;
sem_t *vac1;
sem_t *vac2;


int vac1_val, vac2_val;
int number_citizens;
int dose;
int fd;
int fn;
int *addrCitizen;
int *addrCitizen_dose;


void read_intobuf(FILE *file, int nurse_order, pid_t nurse_pid);
void vaccinate(int i,pid_t vaccinator_pid,int number_citizens,FILE * file);

int main(int argc, char *argv[]) {
    struct sigaction sa;
    memset (&sa, 0, sizeof(sa));
    sa.sa_handler = &handler;
    sigaction (SIGINT, &sa, NULL);
            

    int opt;

    int nurses;
    int vaccinators;
    int citizens;
    int buffer_size;
    int number_of_dose;
    char* inputfile=NULL;
    FILE *file=NULL;
    while((opt = getopt(argc, argv, "n:v:c:b:t:i:")) != -1){
        switch(opt){
            case 'n': 
                nurses= atoi(optarg);
                if(nurses==0){
                    fprintf(stderr,"Invalid number of Nurse\n");
                    fprintf(stdout,"n >= 2: the number of nurses (integer)v >= 2: the number of vaccinators (integer) c >= 3: the number of citizens (integer)b >= tc+1: size of the buffer (integer), t >= 1: how many times each citizen must receive the 2 shots (integer), i: pathname of the input file\n");
                    return 1;
                }
                break;

            case 'v': 
                vaccinators= atoi(optarg);
                if(vaccinators==0){
                    fprintf(stderr,"Invalid number of Vaccinator\n");
                    fprintf(stdout,"n >= 2: the number of nurses (integer)v >= 2: the number of vaccinators (integer) c >= 3: the number of citizens (integer)b >= tc+1: size of the buffer (integer), t >= 1: how many times each citizen must receive the 2 shots (integer), i: pathname of the input file\n");
                    return 1;
                }
                break;

            case 'c': 
                citizens= atoi(optarg);
                if(citizens==0){
                    fprintf(stderr,"Invalid number of Citizen\n");
                    fprintf(stdout,"n >= 2: the number of nurses (integer)v >= 2: the number of vaccinators (integer) c >= 3: the number of citizens (integer)b >= tc+1: size of the buffer (integer), t >= 1: how many times each citizen must receive the 2 shots (integer), i: pathname of the input file\n");
                    return 1;
                }
                break;
            
            case 'b': 
                buffer_size= atoi(optarg);
                if(buffer_size==0){
                    fprintf(stderr,"Invalid number of Buffer size\n");
                    fprintf(stdout,"n >= 2: the number of nurses (integer)v >= 2: the number of vaccinators (integer) c >= 3: the number of citizens (integer)b >= tc+1: size of the buffer (integer), t >= 1: how many times each citizen must receive the 2 shots (integer), i: pathname of the input file\n");
                    return 1;
                }
                break;
            case 't': 
                number_of_dose= atoi(optarg);
                if(number_of_dose==0){
                    fprintf(stderr,"Invalid number of Number of Dose\n");
                    fprintf(stdout,"n >= 2: the number of nurses (integer)v >= 2: the number of vaccinators (integer) c >= 3: the number of citizens (integer)b >= tc+1: size of the buffer (integer), t >= 1: how many times each citizen must receive the 2 shots (integer), i: pathname of the input file\n");
                    return 1;
                }
                break;
            case 'i': 
                inputfile= optarg;
                break;
            default :
                break;
        }
    }

    number_citizens= citizens;
    dose = number_of_dose;

    file = fopen(inputfile, "r");
    if (!file) {
        fprintf(stderr, "File Error!!");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout,"Welcome to the GTU344 clinic. Number of citizens to vaccinate c=%d with t=%d doses.\n", citizens, number_of_dose);
    empty=sem_open("empty", O_CREAT|O_RDWR, 0666, buffer_size);
    mutex=sem_open("mutex", O_CREAT|O_RDWR, 0666, 1);
    vac1 =sem_open("vac1", O_CREAT|O_RDWR, 0666, 0);
    vac2 =sem_open("vac2", O_CREAT|O_RDWR, 0666, 0);
    
    fd= shm_open("citizen", (O_CREAT | O_RDWR), (S_IRUSR | S_IWUSR));
    if (ftruncate(fd, (sizeof(int)*citizens)+1) == -1)
        exit(EXIT_FAILURE);

    addrCitizen = (int*)mmap(NULL, (sizeof(int)*citizens)+1, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);


    fn = shm_open("dose", (O_CREAT | O_RDWR), (S_IRUSR | S_IWUSR));
    if (ftruncate(fn, sizeof(int)*citizens) == -1)
        exit(EXIT_FAILURE);

    addrCitizen_dose = (int*)mmap(NULL, sizeof(int)*citizens, PROT_READ | PROT_WRITE, MAP_SHARED, fn, 0);
    
    for (int i = 0; i < citizens; i++)
    {   
        int temp=fork();
        
        addrCitizen_dose[i]=0;
        
        switch (temp)
        {
        case 0:
            sem_close(empty);
    
    sem_close(mutex);
    
    sem_close(vac1);
    
    sem_close(vac2);
            fclose(file);
            exit(1);
            break;
        case -1:
            perror("Fork Error: ");
            exit(EXIT_FAILURE);
            break;
        default:
            addrCitizen[i]= temp;
            break;
        }



    }
    addrCitizen[citizens]=citizens;
    



    for (int i = 0; i < nurses; i++)
    {
        pid_t nurse_pi=0;
        
        switch (fork())
        {
        case 0:
                
                
            read_intobuf(file, i,nurse_pi);
            break;
        case -1:
            perror("Fork Error: ");
            exit(EXIT_FAILURE);
            break;
        default:
            break;
        }

    }

    
    for (int i = 0; i < vaccinators; i++)
    {
        
        pid_t vaccinator_pid;
        //vaccinatorsa[i] = fork();
        switch (fork())
        {
        case 0:
            vaccinator_pid=getpid();
            vaccinate(i,vaccinator_pid,citizens,file);       
            
            break;
        case -1:
            perror("Fork Error: ");
            exit(EXIT_FAILURE);
            break;
        default:
            break;
        }

    }
    
    
    while( wait(NULL) != -1);
    

    fprintf(stdout," The clinic is now closed. Stay healthy.\n");
    sem_close(empty);
    
    sem_close(mutex);
    
    sem_close(vac1);
    
    sem_close(vac2);
    sem_unlink("empty");
    sem_unlink("mutex");
    sem_unlink("vac1");
    sem_unlink("vac2");
    fclose(file); 


    return 0;
} 
void read_intobuf(FILE *file,  int nurse_order, pid_t nurse_pid){

    while(1){
        sem_wait(empty);
        
        //(ch = fgetc(file)) != EOF
        char ch;
        do{
            ch = fgetc(file);
        }
        while(ch!=EOF && (ch != '1' && ch != '2' ));
        if(ch == EOF){
            sem_post(empty);
            sem_close(empty);
    
            sem_close(mutex);
            
            sem_close(vac1);
            
            sem_close(vac2);
            fclose(file);
            exit(EXIT_SUCCESS);
        }
        if (ch == '1') {
            fprintf(stdout,"Nurse %d (pid=%d) has brought vaccine 1",nurse_order, getpid());
            sem_post(vac1);
        }
        if (ch == '2') {
            fprintf(stdout,"Nurse %d (pid=%d) has brought vaccine 2",nurse_order, getpid());
            sem_post(vac2);
        }
        sem_getvalue(vac1, &vac1_val);
        sem_getvalue(vac2, &vac2_val);
        fprintf(stdout,": the clinic has %d vaccine1 and %d vaccine2.\n", vac1_val, vac2_val);
        
    }
    
}
void vaccinate(int i,pid_t vaccinator_pid,int number_citizens,FILE * file){
        int counter=0;
    while(1){
        sem_wait(mutex);
        if(  addrCitizen[number_citizens]== 0){
            fprintf(stdout,"Vaccinator %d (pid=%d) vaccinated %d doses. ",i,getpid(),counter);
            sem_post(mutex);
            
            sem_close(empty);
    
            sem_close(mutex);
            
            sem_close(vac1);
            
            sem_close(vac2);
            fclose(file);
            exit(1);
        }
        sem_wait(vac1);
        sem_wait(vac2);


        

        addrCitizen[number_citizens]-=1;
        fprintf(stdout,"Vaccinator %d (pid=%d) is inviting citizen pid=%d to the clinic\n",i,getpid(),addrCitizen[addrCitizen[number_citizens]]);
        fprintf(stdout,"Citizen %d (pid=%d) is vaccinated for the %dth time",addrCitizen[number_citizens],addrCitizen[addrCitizen[number_citizens]],addrCitizen_dose[addrCitizen[number_citizens]]);
        
        counter+=1;
        addrCitizen_dose[addrCitizen[number_citizens]]+=1;

        if(addrCitizen_dose[addrCitizen[number_citizens]] != dose) addrCitizen[number_citizens]+=1;
        
        sem_getvalue(vac1, &vac1_val);
        sem_getvalue(vac2, &vac2_val);
        
        fprintf(stdout,": the clinic has %d vaccine1 and %d vaccine2.\n", vac1_val, vac2_val);
        if(addrCitizen[number_citizens]==0)
            fprintf(stdout,"All citizens have been vaccinated .\n");
        sem_post(empty);
        sem_post(empty);
        sem_post(mutex);





    }

}

