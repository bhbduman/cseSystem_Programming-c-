#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

sig_atomic_t SIGUSR1_count=0;
sig_atomic_t SIGUSR2_count=0;
sig_atomic_t SIGINT_count=0;

void handler_usr1(int signal_number){
    ++SIGUSR1_count;
}
void handler_usr2(int signal_number){
    ++SIGUSR2_count;
}
void handler_int(int signal_number){
    ++SIGINT_count;
}

int main(int argc, char*argv[]){
    int pid_arr[8];
    int ppid,check;
    sigset_t signals;
    sigset_t signalsb;

    struct sigaction SIGUSR1_sa;
    memset(&SIGUSR1_sa, 0, sizeof(SIGUSR1_sa));
    SIGUSR1_sa.sa_handler = &handler_usr1;
    sigaction(SIGUSR1, &SIGUSR1_sa, NULL);

    struct sigaction SIGUSR2_sa;
    memset(&SIGUSR2_sa, 0, sizeof(SIGUSR2_sa));
    SIGUSR2_sa.sa_handler = &handler_usr2;
    sigaction(SIGUSR2, &SIGUSR2_sa, NULL);

    struct sigaction SIGINT_sa;
    memset(&SIGINT_sa, 0, sizeof(SIGINT_sa));
    SIGINT_sa.sa_handler = &handler_int;
    sigaction(SIGINT, &SIGINT_sa, NULL);

    if(sigfillset(&signalsb)){
        perror("Error filling set");
        exit(-1);
    }
    if(sigfillset(&signals)){
        perror("Error filling set");
        exit(-1);
    }
    if(sigdelset(&signals, SIGUSR1)){
        perror("Error removing sigusr1");
        exit(-1);
    }
    if(sigdelset(&signals, SIGUSR2)){
        perror("Error removing sigusr2");
        exit(-1);
    }
    if(sigdelset(&signals, SIGINT)){
        perror("Error removing sigint");
        exit(-1);
    }

    int err= sigprocmask(SIG_BLOCK, &signalsb, NULL);
   if(err){
     perror("Could not block the signals");
     exit(-1);
    }
    

    for (int i = 0; i < 8; i++){
        pid_arr[i] = fork();
        if(pid_arr[i]==0){
            printf("Child[%d] copleted First Round son of=> %d\n",getpid(),getppid());
            if(sigsuspend(&signals) &&errno== EFAULT)
                perror("Could not suspend the signals");

            //ppid=getppid();
            if(kill(getppid(),SIGUSR1))
                perror("Could not send the signal to his parent");
            if( sigsuspend(&signals) &&errno== EFAULT)
                perror("Could not suspend the signals");
                
            printf("Child[%d] copleted Second Round son of=> %d\n",getpid(),getppid());

            exit(0);
        }
        else if (pid_arr[i]<0){
            fprintf(stderr,"Fork Error!!!\n");
            exit(-1);
        }
        else {continue;}
    }


    for(int i=0; i<8; i++){
        if(kill(pid_arr[i], SIGUSR1))
            perror("Could not send the signal to his child");
        if( sigsuspend(&signals) &&errno== EFAULT)
            perror("Could not suspend the signals");
            
    }
    printf("\nParent will calculate the error for first round\n\n");
    for(int i=0; i<8; i++){
        if(kill(pid_arr[i], SIGUSR1))
            perror("Could not send the signal to his child");
        if(sigsuspend(&signals) &&errno== EFAULT)
            perror("Could not suspend the signals");
    }
    for(int i=0; i<8; i++)
       wait(NULL);
    //while((check= waitpid(-1,NULL,0))!=-1){}
    
    printf("\nParent will calculate the error for second round\n");





    return 0;
}
