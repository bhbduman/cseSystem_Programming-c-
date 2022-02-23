
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
#include <math.h>
struct datum
{
    float x ,y;
};

float interpolate(struct datum data[], float xi, int n);
void manuplate_file(char *);
double first_round(FILE *fptr, char* line, double last[],int count);
double second_round(FILE *fptr, char * line,double arr[7], int counter, double coef[]);
int main(int argc, char* argv[]) {
    
    if(argc != 2){
        fprintf(stderr, "Usage Error!!\nUsage: processM <pathToFile>\n");
        return -1;
    }
    manuplate_file(argv[1]);

    return 0;
}
void manuplate_file(char* path) {
    
    FILE *fptr = fopen(path, "r+");
    
    if( fptr == NULL ){
        fprintf(stderr,"File could not open or Wrong file path!!!\n");
    }

    /*int c_0=fork();
    int pid[8];
    if(c_0 != 0){
        wait(NULL);
        printf("here: %d\n",c_0);
    }else{
        //pid[0]=getpid();
        //printf("pid: %d--------------------------\n",getpid());
       // printf("ppid: %d--------------------------\n",getppid());
        char line[100];
        for(int i=0;i<8;i++) // loop will run n times (n=5)
        {   
            perror("x");
            if(fork() == 0)
            {
                printf("[son] pid %d from [parent] pid %d\n",getpid(),getppid());
                exit(0);
            }
        }
        for(int i=0;i<8;i++) // loop will run n times (n=5)
            wait(NULL);
            int counter=1;
            while (fgets(line, sizeof(line), fptr) != NULL && counter<9) {
                
                first_round(fptr,line);
                
                counter++;
            }
    }*/
    char line[100];
    int counter=1;
    double arr[7];
    double fifth[8];
    double sixth[8];
    double last[8];
    double error5[8];
    double error6[8];
    double coef[8][7];
    while (fgets(line, sizeof(line), fptr) != NULL && counter<9) {           
        fifth[counter-1] = first_round(fptr,line,last,counter);
        counter++;
    }
    rewind(fptr);
    counter=1;
    fclose(fptr);
    fptr = fopen(path, "r+");

    while (fgets(line, sizeof(line), fptr) != NULL && counter<9) {
        sixth[counter-1]= second_round(fptr,line,arr, counter,coef[counter-1]);   
        counter++;
    }
    for (int k=0; k<8; k++){
        error5[k]=fabs((fifth[k]-last[k]));
        error6[k]=fabs((sixth[k]-last[k]));
        //printf("\n");
    }
    double fift;
    double six;
    for(int k = 0; k<8; k++){
        fift +=error5[k];
        six +=error6[k];
    }
    wait(NULL);
    printf("Error of polynomial of degree 5: %.1f\n",fift/8.0);
    printf("Error of polynomial of degree 6: %.1f\n",six/8.0);
    for(int k=0; k<8; k++){
        printf("Polynomial %d: ",k);
        for(int l=0; l<7; l++){
            printf("%.1f,",coef[k][l]);
        }
        printf("\n");

    }




}
double first_round(FILE *fptr, char * line,double last[], int count){
    int counter;
    struct datum data[6];
    float d_number;
    float data_splitted[16];
    counter=0;
    //printf("%s", line);
    

    char *number= strtok(line,",");
    d_number = atof(number);
    data_splitted[counter] = d_number;
    counter++;
    while(number != NULL && counter !=16){
        
        number = strtok(NULL, ",");
        d_number = atof(number);
        data_splitted[counter] = d_number;

        counter++;
    }
    counter=0;
    for(int i = 0; i<6; i++){
        data[i].x=data_splitted[counter];
        counter++;
        data[i].y=data_splitted[counter];
        counter++;

    // printf("x:%f y:%f\n",data[i].x,data[i].y);
    }
    
    double result= interpolate(data,data_splitted[14],6);
    last[count-1]= data_splitted[15];
    //printf("%.1f==> result\n",result);
    return result;

}
void coefficent(double arr[], struct datum data[7], double coef[]){
    
    double newC[7];
    for(int i=0; i<7; i++){
        arr[i]=0;
    }
    for (int i=0; i<7; i++) {
        for (int nc=0; nc<7; nc++) 
            newC[nc]=0;
        if (i>0) {
            newC[0]=-data[0].x/(data[i].x-data[0].x);
            newC[1]=1/(data[i].x-data[0].x);
        } else {
            newC[0]=-data[1].x/(data[i].x-data[1].x);
            newC[1]=1/(data[i].x-data[1].x);
        }
        int startindex=1;
        if (i == 0)
            startindex=2;
        for(int n=startindex; n<7; n++){
            if(i==n)
                continue;
            for(int nc=6; nc>=1; nc--){
                newC[nc]= newC[nc]*(-data[n].x/(data[i].x-data[n].x))+newC[nc-1]/(data[i].x-data[n].x);
            }
            newC[0]=newC[0]*(-data[n].x/(data[i].x-data[n].x));
        }
        for (int nc=0; nc<7; nc++) arr[nc]+=data[i].y*newC[nc];
    }
    
    
    for(int k=0; k<7; k++){
        coef[k]=arr[k];
    }


}
double second_round(FILE *fptr, char * line, double arr[7], int count,double coef[]){
    
    int counter;
    struct datum data[7];
    float d_number;
    float data_splitted[16];
    counter=0;
    

    char *number= strtok(line,",");
    d_number = atof(number);
    data_splitted[counter] = d_number;
    counter++;
    while(number != NULL && counter !=16){
            
        number = strtok(NULL, ",");
        
        d_number = atof(number);
        data_splitted[counter] = d_number;

        counter++;
    }
    
    counter=0;
    for(int i = 0; i<7; i++){
        data[i].x=data_splitted[counter];
        counter++;
        data[i].y=data_splitted[counter];
        counter++;

     //printf("x:%f y:%f\n",data[i].x,data[i].y);
    }
    
    float result= interpolate(data,data_splitted[14],7);
    
    coefficent(arr,data,coef);
    return result;


}

float interpolate(struct datum data[], float xi, int n) {
    float result = 0; // Initialize result
  
    for (int i=0; i<n; i++)
    {
        // Compute individual terms of above formula
        float term = data[i].y;
        for (int j=0;j<n;j++)
        {
            if (j!=i)
                term = term*(xi - data[j].x)/((float)(data[i].x - data[j].x));
        }
  
        // Add current term to result
        result += term;
    }
  
    return result;
}


