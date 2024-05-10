#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h> 
#include <fcntl.h>
#include <sys/stat.h>

void ctrl_c_handler() {
    exit(0);
    return;
}

//Global Variables
char command[200];
char current_directory[100]; 
char home_directory[100];


//Function which receives buffer from server for dirlist -a and dirlist -t
void fetchMessage(int server_fd) {
    //Array stores buffer size from server
    char bufferLength[50];
    long int size = 0;

    //printf("Fetch Message\n");
    //printf("strlen - %d \n",strlen(command));

    send(server_fd, command,strlen(command),0);
    memset(bufferLength, 0, sizeof(bufferLength));

    //Receiving the buffer size from server
    if(recv(server_fd, bufferLength, sizeof(bufferLength),0) < 0){
        fprintf(stderr, "read() error\n");
        exit(3);
    } else {
        //Copying server's buffer size
        size = atoi(bufferLength);
        //printf("n = %d \n", size);
    }

    //Array which stores the buffer contents sent from server
    char dirList[size];

    memset(dirList, 0, sizeof(dirList));

    //Receiving the buffer data from server
    if(recv(server_fd, dirList, sizeof(dirList),0) < 0){
        fprintf(stderr, "read() error\n");
        exit(3);
    }
    dirList[size] = '\0';

    //Displaying the results of dirlist -a and dirlist -t from server on client
    printf("%s \n", dirList);
}

//Function which receives buffer data from server and creates a tar.gz archive out of it on client
void fetchTarFile(int server_fd) {
    
    //printf("strlen - %d \n",strlen(command));

    //Array stores the size of the tarFile created at server end
    char fileLength[50];
    off_t size = 0;
    send(server_fd, command,strlen(command),0);

    memset(fileLength, 0, sizeof(fileLength));

    //Array which receives the size of the tarFile created at server end
    if(recv(server_fd, fileLength, sizeof(fileLength),0) < 0){
        fprintf(stderr, "read() error\n");
        exit(3);
    } else {
        size = atoi(fileLength); //storing the size of tarFile from server end
        //printf("size = %d \n", size);
    }

    //Array which stores the contents of the tarFile created at server end
    char tarFile[1024];

    memset(tarFile, 0, sizeof(tarFile));
    //printf("tar file before\n",tarFile);

    //Receiving the tarFile created at server end
    if(recv(server_fd, tarFile, sizeof(tarFile),0) < 0){
        fprintf(stderr, "read() error\n");
        exit(3);
    } 
    if(strcmp(tarFile,"File not found")==0)
    {
        printf("File not found\n");
        return;
    }
    else 
    {
        //Getting the home directory of client
        char client_directory[100];
        //printf("Receiving File\n");
        //printf("tar file - %s \n",tarFile);
        //printf("size of tar: %lld \n", strlen(tarFile));

        char fileName[12];
        strcpy(fileName,"temp.tar.gz");
        //printf("filename - %s \n",fileName);

        //changing pwd to clients home directory
        chdir(home_directory);

        //creating w24project directory to store the tarFiles files

        //Building an absolute path on the client
        strcpy(client_directory,home_directory);
        strcat(client_directory, "/");
        strcat(client_directory, "w24project");
        mkdir(client_directory, 0777);
        strcat(client_directory, "/");
        strcat(client_directory, fileName);

        //creating a new tar.gz archive
        int fd3 = open(client_directory, O_CREAT|O_TRUNC|O_RDWR, 0777);
        ssize_t n;
        int sizeoftar = sizeof(tarFile);
        ssize_t total = write(fd3, tarFile, sizeoftar);
        //writing buffer data received from server
        while ((n = recv(server_fd, tarFile, sizeoftar, 0)) >0) {
            total = total + write(fd3, tarFile, n);
            if(n < sizeoftar || total>=size)
            {
                break;
            }
        } 

        printf("Received %s file and stored at %s\n",fileName,client_directory);
        chdir(current_directory);
    }
}

int validations(int server_fd)
{
    char message[13500];
    int count=0;
    
    //printf("command - %s \n",command);
    command[strlen(command)-1]='\0';
    
    char *cmd = malloc(strlen(command));
    strcpy(cmd, command);

    int n = 0;
    char *cmdArr[4];
    int isValid = 0;

    char *token = strtok(cmd, " ");

    while(token != NULL) {
        cmdArr[n] = token;
        n++;
        token = strtok(NULL, " ");
    }

    if(strcmp(cmdArr[0], "dirlist") == 0) { //Validation for dirlist
        if(n == 2 && (strcmp(cmdArr[1], "-a") == 0)) { //Validation for dirlist -a
            fetchMessage(server_fd);
        } else if(n == 2 && (strcmp(cmdArr[1], "-t") == 0)) { //Validation for dirlist -t
            fetchMessage(server_fd);
        } else {
            printf("Please provide valid input. Usage: <dirlist -a> | <dirlist -t>\n");
        } 
    } else if(strcmp(cmdArr[0], "w24fn") == 0) { //validation for w24fn
        if(n == 2) {
            fetchMessage(server_fd);
        } else {
            printf("Please provide valid input. Usage: <w24fn filename>\n");
        }
    } else if(strcmp(cmdArr[0], "w24fz") == 0) { //validation for w24fz
        if(n == 3) { //max possible arguments == 3
            //checking if size1 arg is an integer or not
            for(int i = 0; i < strlen(cmdArr[1]); i++) {
                int isValidSize = cmdArr[1][i];
                //Comparing each character in size1 to match an integer 0 to 1
                if(isValidSize >= '0' && isValidSize <= '9') {
                    isValid = 1;
                } else {
                    isValid = 0;
                    printf("Please provide valid input. Usage: <w24fz size1(int) size2(int)>\n");
                    return 0;
                }
            }

            //checking if size2 arg is an integer or not
            for(int i = 0; i < strlen(cmdArr[2]); i++) {
                int isValidSize = cmdArr[2][i];
                //Comparing each character in size2 to match an integer 0 to 1
                if(isValidSize >= '0' && isValidSize <= '9') {
                    isValid = 1;
                } else {
                    isValid = 0;
                    printf("Please provide valid input. Usage: <w24fz size1(int) size2(int)>\n");
                    return 0;
                }
            }

            if(strcmp(cmdArr[1], cmdArr[2]) <= 0) {
                isValid = 1;
            } else {
                isValid = 0;
                printf("Please provide valid input. Usage: <w24fz size1(int) size2(int)>\n");
                return 0;
            }

            if(isValid) {
                fetchTarFile(server_fd);
            } else {
                printf("Please provide valid input. Usage: <w24fz size1(int) size2(int)>\n");
            }
        } else {
            printf("Please provide valid input. Usage: <w24fz size1 size2>\n");
        }
    } else if(strcmp(cmdArr[0], "w24ft") == 0) { //validation for w24ft
        if(n > 1 && n < 5) { //max possible args == 5
            fetchTarFile(server_fd);
        } else {
            printf("Please provide valid input. Usage: <w24ft <extensionList upto 3 extensions separated by space>>\n");
        }
    } else if(strcmp(cmdArr[0], "w24fda") == 0 || strcmp(cmdArr[0], "w24fdb") == 0)  { //validation for 24fda
        if(n == 2) 
        { //max possible args == 2
            //Array stores the values of date after tokenizing with '-'
            char *isValidDate[3];

            //copy of user entered command
            char *dateTokenArr = malloc(100);

            strcpy(dateTokenArr, "");
            strcpy(dateTokenArr, cmdArr[1]);
            
            int j = 0;

            //tokenizing the date with '-' to get the year, month and day values
            char *dateToken = strtok(dateTokenArr, "-");
            
            while(dateToken != NULL) {
                isValidDate[j] = dateToken;
                j++;
                dateToken = strtok(NULL, "-");
            }
            isValid=1;
            if(j == 3)
            {
                //checking if the year is valid or not
                for(int i = 0; i < strlen(isValidDate[0]) && isValid!=0 ; i++) {
                    int isValidYear = isValidDate[0][i];
                    if(isValidYear >= '0' && isValidYear <= '9') {
                        isValid = 1;
                    } else {
                        isValid = 0;
                    }
                }

                //Valid month format check
                if( isValid!=0 && strlen(isValidDate[1]) == 2) {
                    if(isValidDate[1][0] == '0') {
                        if(  isValidDate[1][1] >= '1' && isValidDate[1][1] <= '9') {
                            isValid = 1;
                        } else {
                            isValid = 0;
                        }
                    } else if(isValidDate[1][0] == '1') {
                        if(isValidDate[1][1] >= '0' && isValidDate[1][1] <= '2') {
                            isValid = 1;
                        } else {
                            isValid = 0;
                        }
                    }
                } else {
                    isValid = 0;
                }

                //Valid day format check
                if(isValid!=0 && strlen(isValidDate[2]) == 2) {
                    if(isValidDate[2][0] == '0' || isValidDate[2][0] == '1' || isValidDate[2][0] == '2') {
                        if(isValidDate[2][1] >= '1' && isValidDate[2][1] <= '9') {
                            isValid = 1;
                        } else {
                            isValid = 0;
                        }
                    } else if(isValidDate[2][0] == '3') {
                        if( isValid!=0 && isValidDate[2][1] == '0' || isValidDate[2][1] == '1') {
                            isValid = 1;
                        } else {
                            isValid = 0;
                        }
                    }
                } else {
                    isValid = 0;
                }

                int month = atoi(isValidDate[1]);

                //validation for month
                if(isValid!=0 && month >= 1 && month <= 12) {
                    int year = atoi(isValidDate[0]);
                    int day = atoi(isValidDate[2]);
                    if(month == 1 || month == 3 || month == 5 || month == 7 || 
                        month == 8 || month == 10 || month == 12) { //for months with 31 days
                        if(day >= 1 && day <= 31) {
                            isValid = 1;
                        } else {
                            isValid = 0;
                        }
                    } else if(month == 2) {
                        if(((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) { //leap year check for february
                            if(day >= 1 && day <= 29) { //for leap year
                                isValid = 1;
                            } else {
                                isValid = 0;
                            }
                        } else {
                            if(day >= 1 && day <= 28) { //for a non-leap year
                                isValid = 1;
                            } else {
                                isValid = 0;
                            }
                        }                       
                    } else if(month == 4 || month == 6 || month == 9 || month == 11) {
                        if(day >= 1 && day <= 30) { //for months with 30 days
                            isValid = 1;
                        } else {
                            isValid = 0;
                        }
                    }
                } else {
                    isValid = 0;
                }

                if(isValid) {
                    fetchTarFile(server_fd);
                } else {
                     if(strcmp(cmdArr[0], "w24fda") == 0)
                    {
                        printf("Please provide valid input. Usage: <w24fda date>\n");
                    }
                    else if(strcmp(cmdArr[0], "w24fdb") == 0)
                    {
                        printf("Please provide valid input. Usage: <w24fdb date>\n");
                    } 
                }
            }    
            else
            {
                printf("entered else\n");
                if(strcmp(cmdArr[0], "w24fda") == 0)
                {
                    printf("Please provide valid input. Usage: <w24fda date>\n");
                }
                else if(strcmp(cmdArr[0], "w24fdb") == 0)
                {
                    printf("Please provide valid input. Usage: <w24fdb date>\n");
                } 
            }        
        } else {
            printf("entered else\n");
            if(strcmp(cmdArr[0], "w24fda") == 0)
            {
                printf("Please provide valid input. Usage: <w24fda date>\n");
            }
            else if(strcmp(cmdArr[0], "w24fdb") == 0)
            {
                printf("Please provide valid input. Usage: <w24fdb date>\n");
            } 
        }
    } else if(strcmp(cmdArr[0], "quitc") == 0) { //Validation for quitc
        if(n == 1) { //max possible args == 1
            send(server_fd, command, strlen(command), 0);
            close(server_fd);
            exit(0);
        } else {
            printf("Please provide valid input. Usage: <quitc>\n");
        }
    } else { //Invalid user input
        printf("Please enter a valid command\n");
    }  
}


int main(int argc, char *argv[]){
    signal(SIGINT, ctrl_c_handler);
    char message[255];
    
    int ssd, portNumber;
    socklen_t len;
    struct sockaddr_in servAdd;
    strcpy(home_directory,getenv("HOME"));
    //Storing client's present working directory
    printf("client started \n");
    getcwd(current_directory, 100);

    //Validation for starting client's socket
    if(argc != 3) {
        printf("Call model:%s <IP> <Port#>\n",argv[0]);
        exit(0);
    }

    //creating a socket for the client
    if((ssd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Cannot create socket\n");
        exit(1);
    }

    servAdd.sin_family = AF_INET;
    sscanf(argv[2], "%d", &portNumber);
    servAdd.sin_port = htons((uint16_t)portNumber);//convert host bytestream into network bytestream

    if(inet_pton(AF_INET, argv[1], &servAdd.sin_addr) < 0) {
        fprintf(stderr, " inet_pton() has failed\n");
        exit(2);
    }

    //Connecting to server's socket
    if(connect(ssd, (struct sockaddr *)&servAdd, sizeof(servAdd)) < 0) {
        fprintf(stderr, "connect() has failed, exiting\n");
        exit(3);
    }

    //handshake message
    recv(ssd, message, sizeof(message), 0);
    printf(" message - %s \n", message);

    //if handshake with mirrors
    if(strcmp(message,"server") != 0) {
        printf("mirror connection started \n");
        close(ssd);
        if((ssd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            fprintf(stderr, "Cannot create socket\n");
            exit(1);
        }
        servAdd.sin_family = AF_INET;
        sscanf(message, "%d", &portNumber);
        servAdd.sin_port = htons((uint16_t)portNumber);
        if(inet_pton(AF_INET, argv[1], &servAdd.sin_addr) < 0) {
            fprintf(stderr, " inet_pton() has failed\n");
            exit(2);
        }

        //connecting to mirror's socket
        if(connect(ssd,(struct sockaddr *)&servAdd,sizeof(servAdd)) < 0) {
            fprintf(stderr, "connect() has failed, exiting\n");
            exit(3);
        }
        printf("mirror connection successsfull \n");
    }

    //handling client's input
    while(1) {
        fprintf(stderr, "client24$ ");
        fgets(command, sizeof(command), stdin);
        validations(ssd);
    }
}