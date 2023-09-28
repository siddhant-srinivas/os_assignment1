#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#define PERMS 0666

struct mesg_buffer{
    long mesg_type;
    char mesg_text[100];
};
int main(int argc,char* argv[]){
   
    struct mesg_buffer buf;
    int i;
    key_t key;
    int len;
    int msgid;
    if((key=ftok("server.c",'B'))==-1){
        perror("ftok");
        exit(1);
    }
    if((msgid= msgget(key,PERMS | IPC_CREAT))==-1){
        perror("msgget");
        exit(1);
    }
    printf("Ready to receive messages\n");
    while(1){
        if(msgrcv(msgid,&buf,sizeof(buf.mesg_text),0,0)==-1){
            printf("%s", buf.mesg_text);
            perror("msgrcv");
            exit(1);
        }
        switch(buf.mesg_type){
        case 1:
            pid_t pid1 = fork();
            if(pid1 == -1){
                perror("fork");
                exit(EXIT_FAILURE);
            }
            if(pid1 == 0){
                struct mesg_buffer buf2;
                buf2.mesg_type= 1;
                strcpy(buf2.mesg_text,"Hello\0");       //Client
                if(msgsnd(msgid,&buf2,strlen(buf2.mesg_text)+1,0)==-1){
                    perror("msgsnd");
                    exit(1);
                }
                exit(0);
            }
            break;
        case 2:
            //printf("inside 2");
            pid_t pid2 = fork();
            if(pid2 == -1){
                perror("fork");
                exit(EXIT_FAILURE);
            }
            if(pid2 == 0){
                int fd1[2];
                if(pipe(fd1)==-1){
                    perror("pipe");
                    exit(1);
                }
                struct mesg_buffer buf3;
                pid_t cpid2 = fork();
                if(cpid2 == -1){
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                //Grandchild
                if(cpid2 == 0){
                    close(fd1[0]);      //Closing read end
                    dup2(fd1[1],STDOUT_FILENO);     //Redirecting stdout to write end
                    execlp("find","find",".","-name",buf.mesg_text,NULL);
                    close(fd1[1]);
                    perror("execlp");
                    exit(-1);
                }
                //Child
                else{
                    wait(NULL);     //Ensures that file search finishes fully
                    char r2[20];
                    int x = 0;
                    close(fd1[1]);
                    if(read(fd1[0],&r2,sizeof(r2)) == 0){   //Read end is empty
                        x = 1;
                    }
                    close(fd1[0]);
                    for(int i=0;i<strlen(r2);i++){
                        if(r2[i]=='\n') 
                            r2[i] = '\0';   
                        if((int)r2[i] > 128){
                            x = 1;
                            break;
                        }
                    }
                    if(x == 0){
                        char f[50] = "File found: ";
                        strcat(f, r2);
                        strcpy(buf3.mesg_text,f);                       
                    }
                    else{
                        char nf[20] = "File not found";
                        strcpy(buf3.mesg_text, nf);
                    }
                    buf3.mesg_type = 2;
                    if(msgsnd(msgid,&buf3,strlen(buf3.mesg_text)+1,0)==-1){
                        perror("msgsnd");
                        exit(1);
                    }
                    exit(0);
                }
            }
            break; 
        case 3:
            pid_t pid3 = fork();
            if(pid3 == -1){
                perror("fork");
                exit(EXIT_FAILURE);
            }
            if(pid3 == 0){
                int fd[2];
                if(pipe(fd)==-1){
                    perror("pipe");
                    exit(1);
                }
                struct mesg_buffer buf4;
                int a;
                pid_t cpid3 = fork();
                if(cpid3 == -1){
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                if(cpid3 == 0){
                    //Insert code for detecting file not found
                    dup2(fd[1],STDOUT_FILENO);
                    execlp("wc","wc","-w",buf.mesg_text,NULL);
                    close(fd[1]);
                    perror("execlp");
                    exit(-1);
                }
                else{
                    wait(NULL);
                    char r[20];
                    close(fd[1]);
                    read(fd[0],&r,sizeof(r));
                    close(fd[0]);
                    for(int i=0;i<strlen(r);i++){
                        if(r[i]==' ') r[i] = '\0';
                    }
                    printf("%s\n",r);
                    strcpy(buf4.mesg_text,r);
                    buf4.mesg_type = 3;
                    
                    if(msgsnd(msgid,&buf4,strlen(buf4.mesg_text)+1,0)==-1){
                        perror("msgsnd");
                        exit(1);
                    }
                    exit(0);
                }
            }
            break;
        case 4:
            while(wait(NULL) > 0){};
            if(msgctl(msgid, IPC_RMID, NULL) == -1){
                perror("msgctl");
                exit(1);
            }
            printf("Main server terminated\n");
            exit(0);
        } 
    }
}