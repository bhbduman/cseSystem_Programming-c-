
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <getopt.h>
#include <signal.h>

sig_atomic_t sigusr1_count= 0;
void handler (int signal_number) {
    ++sigusr1_count;
}

//#include "vector.h"

int isValid(char *original, char *check){
    char prev= '\0';
    char cprev= '\0';
    
    for(int i = 0; i<strlen(original); i++){
        if(original[i]>='A' && original[i]<='Z') 
            original[i]=original[i]+32;
        else
            continue;
    }
    for(int i = 0; i<strlen(check); i++){
        if(check[i]>='A' && check[i]<='Z') 
            check[i]=check[i]+32;
        else
            continue;        
    }
    if(strcmp(original,check)==0){
        return 1;
    }
    else if(strchr(original, '+') != NULL){
        while(((*original == *check || *original == '+') && !(*original == '\0' || *check == '\0'))|| (cprev == *original)){

            //printf("BIG%c==%c\n",*original, *check);
            if(*original == '+'){
                original--;
                prev = *original;
                original++;
                while(*check == prev){
                    check++;
                }
                check--;
            }
            cprev= *check;
            original++;
            if ((cprev != *original))
            {
                check++;
            }
            
        }

        if(*original == '\0' && *check == '\0'){
            return 1;
        }
        return 0;
    }
    else{
        return 0;
    }
}

char *checkPerm(struct stat statbuf){    
    char  *create = malloc(10*sizeof(char));

    if(create == NULL){
        fprintf(stderr,"Could not allocate memory");
        return NULL;
    }
    
    (statbuf.st_mode & S_IRUSR) ? (create[0]= 'r') : (create[0]= '-');
    (statbuf.st_mode & S_IWUSR) ? (create[1]= 'w') : (create[1]= '-');
    (statbuf.st_mode & S_IXUSR) ? (create[2]= 'x') : (create[2]= '-');
    (statbuf.st_mode & S_IRGRP) ? (create[3]= 'r') : (create[3]= '-');
    (statbuf.st_mode & S_IWGRP) ? (create[4]= 'w') : (create[4]= '-');
    (statbuf.st_mode & S_IXGRP) ? (create[5]= 'x') : (create[5]= '-');
    (statbuf.st_mode & S_IROTH) ? (create[6]= 'r') : (create[6]= '-');
    (statbuf.st_mode & S_IWOTH) ? (create[7]= 'w') : (create[7]= '-');
    (statbuf.st_mode & S_IXOTH) ? (create[8]= 'x') : (create[8]= '-');
    create[9] = '\0';
    return create;

}

int check(const char *path,char *filename, char * fInDir,const char *filesize,const char *filetype,const char *fileperm,const char *linknum){
   
    struct stat statbuf;

    if(lstat(path,&statbuf)) {
        perror("lstat");
        return -1;
    }


    if(filename && !isValid(filename, fInDir)) {
        return 0;
    }
    
    if(filesize != NULL){
        int fsize = atoi(filesize);
        if ( fsize != statbuf.st_size)
            return 0;
    }

    if (filetype != NULL)
    {
        switch (statbuf.st_mode & S_IFMT )
        {
        case S_IFREG:
            if (strcmp(filetype,"f") != 0){
                return 0;
            }
            break;
        case S_IFDIR:
            if (strcmp(filetype,"d") != 0){
                return 0;
            }
            break;
        case S_IFCHR:
            if (strcmp(filetype,"c") != 0){
                return 0;
            }
            break;
        case S_IFBLK:
            if (strcmp(filetype,"b") != 0){
                return 0;
            }
            break;
        case S_IFLNK:
            if (strcmp(filetype,"l") != 0){
                return 0;
            }
            break;
        case S_IFIFO:
            if (strcmp(filetype,"p") != 0){
                return 0;
            }
            break;
        case S_IFSOCK:
            if (strcmp(filetype,"s") != 0){
                return 0;
            }
            break;
        default:
            fprintf(stderr,"Unknown file type: %s\n", filetype);
            return 0;
        }
        
    }
    
    if (fileperm != NULL)
    {
        char *perm = checkPerm(statbuf);
        if(perm==NULL)
            return -1;
        else if(strcmp(fileperm, perm) !=0){
            free(perm);
            return 0;
        }
        free(perm);
    }

    if(linknum != NULL){
        int lNum = atoi(linknum);
        if(lNum != statbuf.st_nlink){
            return 0;
        }
    }

    return 1;
}
void printPath( char* name) {
    if(name != NULL){
        int counter=0;
        for(int i=0; i<strlen(name); i++){
            if(name[i] == '/'){
                fprintf(stdout,"\n");
                counter+=2;
                fprintf(stdout,"|");
                for(int i =0; i<counter;i++){
                    fprintf(stdout,"-");
                }
                    continue;
            }            
            
            fprintf(stdout,"%c",name[i]);
        }
        fprintf(stdout,"\n");
    }
    return;
}

void traversedir(char *name, int indnt, char *search,const char *filesize,const char *filetype,const char *fileperm,const char *linknum,const char *pathname, int *isFound) {
    
    char *path=NULL;
    struct dirent *dp=NULL;
    
    DIR *dir = opendir(name);
    if (!dir) {
        perror("traversedir");
        return;
    }

    while ((dp = readdir(dir)) != NULL)
    {
        if(sigusr1_count){
            if(closedir(dir))
                perror("traversedir");
            return;            
        }
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            path = malloc (1+ strlen(name)+strlen("/") +strlen(dp->d_name));
            if(path==NULL){
                fprintf(stderr,"Could not allocate memory\n");
                if(closedir(dir))
                    perror("traversedir");
                return;
            }
            
            strcpy(path, name);
            strcat(path, "/");
            strcat(path, dp->d_name);

            
            int status = check(path,search,dp->d_name,filesize,filetype,fileperm,linknum);

            if(status == -1) {
                free(path);
                if(closedir(dir))
                    perror("traversedir");
                return;
            }    
            else if(status == 1) {
                printPath(path);
                (*isFound)++;
                
                
            }
            
            if(dp->d_type == DT_DIR) {
                traversedir(path, indnt + 1,search,filesize,filetype,fileperm,linknum,pathname,isFound);
            }
            free(path);
        }
    }
    
    if(closedir(dir)) {
        perror("traversedir");
        return;
    }
}

void myfind(char *filename,char *filesize,char *filetype,char *fileperm,char *linknum,char *pathname, int *isFound) {
    traversedir(pathname,1,filename,filesize,filetype,fileperm,linknum,pathname,isFound);
}

int main(int argc, char *argv[]){
    struct sigaction sa;
    memset (&sa, 0, sizeof(sa));
    sa.sa_handler = &handler;
    sigaction (SIGINT, &sa, NULL);


    int opt,i=0;
    char *filename= NULL;
    char *filesize= NULL;
    char *filetype= NULL;
    char *fileperm= NULL;
    char *linknum = NULL;
    char *pathname= NULL;

    while ((opt = getopt(argc, argv, "f:b:t:p:l:w:")) !=-1){
        switch (opt){
            case 'f':
               // printf("-f : filename (case insensitive), supporting the following regular expression: +\n");
                filename =  optarg;
                break;
            case 'b':
                //printf("-b : file size (in bytes)\n");
                filesize = optarg;
                break;
            case 't':
                //printf("-t : file type (d: directory, s: socket, b: block device, c: character device f: regular file,p:pipe, l: symbolic link\n");
                filetype = optarg;// & S_IFMT;//S_IFREG
                break;
            case 'p':
                //printf("-p : permissions, as 9 characters (e.g. ‘rwxr-xr--’)\n");
                fileperm = optarg;
                break;
            case 'l':
                //printf("-l: number of links\n");
                linknum = optarg;
                break;
            case 'w':
               // printf("mandatory\n");
                pathname = optarg;
                break;
            default :
                fprintf(stderr, "Usage: %s -w [targetfilepath] && ([-f filename] || [-t filetype] || [-p filepermission] || [-l numberoflinks])\n", argv[0]);
                return 1;
        }
        
    }
    
    if (pathname == NULL) {
        fprintf(stderr, "Usage: %s -w [targetfilepath] && ([-f filename] || [-t filetype] || [-p filepermission] || [-l numberoflinks])\n", argv[0]);
        return 1;
    }
    
    if(filesize != NULL) {
        for (i = 0 ; i < strlen(filesize) ; i++) 
        {
            if(!isdigit(filesize[i])) 
            { 
                fprintf(stderr,"Invalid file size: %s\n", filesize);
                fprintf(stderr, "Usage: %s -w [targetfilepath] && ([-f filename] || [-t filetype] || [-p filepermission] || [-l numberoflinks])\n", argv[0]);
                return 1;
            } 
        }        
    }

    if(filetype != NULL) {
        if(strlen(filetype) > 1 || (filetype[0] != 'd' &&( filetype[0] != 's' &&( filetype[0] != 'b' &&( filetype[0] != 'c' &&( filetype[0] != 'p' &&( filetype[0] != 'f' &&( filetype[0] != 'l')))))))){
            fprintf(stderr,"Invalid file type: %s\n", filetype);
            fprintf(stderr, "Usage: %s -w [targetfilepath] && ([-f filename] || [-t filetype] || [-p filepermission] || [-l numberoflinks])\n", argv[0]);
                return 1;
        }
    }

    if(fileperm != NULL) {
        if(strlen(fileperm)!= 9){
            fprintf(stderr,"Invalid file permission : %s\n",fileperm);
            fprintf(stderr, "Usage: %s -w [targetfilepath] && ([-f filename] || [-t filetype] || [-p filepermission] || [-l numberoflinks])\n", argv[0]);
            return 1;
        }
        if((fileperm[0] != 'r'&& fileperm[0] != '-') || (fileperm[3] != 'r'&& fileperm[3] != '-') || (fileperm[6] != 'r' && fileperm[6] != '-') ){
            fprintf(stderr,"Invalid file permission : %s\n",fileperm);
            fprintf(stderr, "Usage: %s -w [targetfilepath] && ([-f filename] || [-t filetype] || [-p filepermission] || [-l numberoflinks])\n", argv[0]);
            return 1;
        }
        if((fileperm[1] != 'w'&& fileperm[1] != '-') || (fileperm[4] != 'w'&& fileperm[4] != '-') || (fileperm[7] != 'w'&& fileperm[7] != '-') ){
            fprintf(stderr,"Invalid file permission : %s\n",fileperm);
            fprintf(stderr, "Usage: %s -w [targetfilepath] && ([-f filename] || [-t filetype] || [-p filepermission] || [-l numberoflinks])\n", argv[0]);
            return 1;
        }
        if((fileperm[2] != 'x'&& fileperm[2] != '-') || (fileperm[5] != 'x'&& fileperm[5] != '-') || (fileperm[8] != 'x'&& fileperm[8] != '-') ){
            fprintf(stderr,"Invalid file permission : %s\n",fileperm);
            fprintf(stderr, "Usage: %s -w [targetfilepath] && ([-f filename] || [-t filetype] || [-p filepermission] || [-l numberoflinks])\n", argv[0]);
            return 1;
        }
    }

    if(linknum !=NULL) {
        for (i = 0 ; i < strlen(linknum) ; i++) 
        {
            if(!isdigit(linknum[i])) 
            { 
                fprintf(stderr,"Invalid number of link: %s\n",linknum);
                fprintf(stderr, "Usage: %s -w [targetfilepath] && ([-f filename] || [-t filetype] || [-p filepermission] || [-l numberoflinks])\n", argv[0]);
                return 1;
            } 
        }
        
    }

    if (argc<=4) {
       fprintf( stderr, "Invalid number of argument\n");
       fprintf(stderr, "Usage: %s -w [targetfilepath] && ([-f filename] || [-t filetype] || [-p filepermission] || [-l numberoflinks])\n", argv[0]);
       return 1;
    }
    
    //fprintf(stdout, "%s\n", pathname);
    int isFound=0;
    myfind(filename,filesize,filetype,fileperm,linknum,pathname,&isFound);
    if(sigusr1_count){
        fprintf(stderr,"CTRL^C SIGNAL HAS BEEN HANDLED\n");
        return 1;            
    }
    if(!isFound){
        fprintf(stderr, "No file found\n");
    }
    
    return 0;
}
