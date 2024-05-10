#define _XOPEN_SOURCE 500  // Required for FTW_DEPTH on some systems
#include <ftw.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
 
int client_connections=0;
//char* root_dir="/home/garikim/ASP";
char* root_dir;
int count=0;
int size1,size2;
char filenames[2000][500];
char file[100];
char command[100];
char creation_times[1000][25];
char* tar_path;
char current_path[100];
int file_found=0;
char extension1[10],extension2[10],extension3[10];
char user_date[10];
char month[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun","Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
char* arr[4];  

//Utility Function/
const char* extractfilename(const char *filepath)  // extracts filename from given path
{
    const char *filename = strrchr(filepath, '/'); // extracts the string from last slash to end
    if (filename == NULL) {  // if filename is null then there is no slash in the string so returns path
        filename = filepath;   
    } else {
        filename++;  // removes the '/' and sends only filename
    }
    return filename;
}


int isDirectoryvalid(const char *path) // returns 0 if directory is valid
{
    struct stat stats;
    stat(path, &stats); //loads the path information into stats
    if (S_ISDIR(stats.st_mode))  //S_ISDIR returns whether directory or not based on st_mode
        return 0;

    return -1;
}

void create_tar_file(const char* filepath) //creates tar file and removes the test_directory 
{
    char* path=malloc(100);
    strcpy(path,"");
    strcpy(path,filepath);
    printf("path - %s \n", path); //file path
    if(fork()==0)
    {
        char *const args[] = {"tar", "-rf","temp.tar",path, NULL};   // append the file to the tar
        execvp(args[0],args);  // executing execvp
        perror("execlp");
        exit(1);
    }
    else
    {
        wait(NULL);  // waiting for the child execution to be completed
    }
    printf("Appending file to tar file\n"); 
}

void generateGunZip()  //creating .gz file
{
    if(fork()==0)
    {
        char *const args[] = {"gzip","temp.tar", NULL};  
        execvp(args[0],args);  // executes the above command and create .gz file
        perror("execlp");
        exit(1);
    }
    else
    {
        wait(NULL);  // wait for the child execution to be completed
    }
}

int remove_directory() //removes the directory and removes files and subfolders in that directory
{
    if (fork() == 0) {
        execlp("rm", "rm", "-r", "tar_dump", NULL); // removes the 
        perror("execlp"); 
        exit(1);
    } else {
        // Parent process
        wait(NULL);
    }
    return 0;
}

char* extractdirectory( char *fpath)  // extract directory path removing the string from last slash
{
    char *lastSlash = strrchr(fpath, '/'); //returns the string from last slash to end

    if (lastSlash != NULL) {
        // Calculate the length of the path
        int pathLength = strlen(fpath) - strlen(lastSlash) ; //gets the size of the string removing the string after last slash 
        char *directory_path=(char *)malloc(pathLength);
        strncpy(directory_path, fpath, pathLength); //copying necessary string to directory_path
        directory_path[pathLength] = '\0'; // making last character as null in directory_path
	return directory_path;
    } 
    return fpath;
}

char* create_directory(  char* path) //creates the given directory path recursively
{
    if(isDirectoryvalid(path)==0) //if path is valid then returns the path
    {
        return path;
    }
      char* directory=extractdirectory(path); //extract the directory path removing last folder from the path
    create_directory(directory);
    mkdir(path,0744);
    return path;
}




void send_message_to_client(int sd)   // sends message to the client
{
    int fd=open("dirlist_a.txt",O_CREAT|O_APPEND|O_WRONLY,0777);  //creates and opens the file dirlist_a.txt
    for(int i=0;i<count;i++)
    {
        long n=write(fd,filenames[i],strlen(filenames[i])); //writes the data in filenames array
        if(i < count - 1) { // if not the last item of filenames array
            write(fd, "\n", 1); // include newline in the file
        }
        
    }
    close(fd);  //closes the file descriptor
    fd=open("dirlist_a.txt",O_RDONLY);  // opens a file dirlist_a.txt 
    struct stat st;
    stat("dirlist_a.txt",&st); 
    long int size= st.st_size;  //gets the size of the file
    char snum[10];
    sprintf(snum,"%d",size);
    send(sd,snum, sizeof(snum), 0); //sends the size of the file
    char* buffer[size];
    memset(buffer,0,size); //inialize every cell in buffer with 0
    //printf(" size - %ld\n",size);  
    long n=read(fd,buffer,size);  // read the content of the file
    send(sd, buffer, n, 0);  // sends the content of the file
    unlink("dirlist_a.txt"); //deletes the dirlist_a.txt file
}

void send_tar_to_client(int sd)
{
    int fd=open("temp.tar.gz",O_RDONLY);  //opens the temp.tar.gz file
    struct stat st;
    stat("temp.tar.gz", &st); 
    off_t size= st.st_size;
     //reads the file content and store it into buffer
    char snum[10];
    sprintf(snum,"%lld",size);
    send(sd,snum, sizeof(snum), 0);  //sends the file size to the client
    //printf(" size - %ld\n",size);  
    char buffer[1024];
    ssize_t n;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) 
    {
        send(sd, buffer, n, 0);  //sends the file content to the client
    }
    
    close(fd);
    remove("temp.tar.gz");
}



//dirlist_a start/
void sort_last_item() // sort the array 
{
    char* temp=malloc(100);
    int n=count;
    while(n>0) //iterates for every string
    {
        for(int i = 0; filenames[n][i] != '\0' && filenames[n-1][i] != '\0'; i++) //iterator for every character in the string
        {
            int y= tolower(filenames[n-1][i]) - tolower(filenames[n][i]); //stores the difference between the two characters
            if (y > 0) // if first character is greater that second character then swap of 2 strings and break
            {
                strcpy(temp,filenames[n]);
                strcpy(filenames[n],filenames[n-1]);
                strcpy(filenames[n-1],temp);
                break;
            }
            else if( y < 0) // else return
            {
                return;

            }
        }
        n--; // decrementing the counter
    }
}

int dirlist_all(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {  
    if (typeflag == FTW_D && strcmp(root_dir,fpath)!=0) {
        const char *filename = extractfilename(fpath); //extracts file name 
        //printf("path = %s\n",filename); 
        //printf("count = %d \n", count);
        strcpy(filenames[count], filename);  // copies the filename into filenames array
        sort_last_item();  // sorts the array
        count++;  // increments the counts
        file_found=1; //file_found flag is set
    }
    return 0;  // Continue traversal
}

void dirlist_a()
{
    if (nftw(root_dir,dirlist_all, 20, FTW_PHYS) == -1) {     // calls the dirlist_all function for every file in the root_dir
        perror("nftw");
        return;
    }
    if(file_found==0) 
    {
        printf("Sending 'File not found' to client - %d \n",client_connections+1);
        strcpy(filenames[count++],"File not found");  // stores the string " No file found" into the filenames array
    } 
    else
    {
        printf("Below Directory list is sent to client - %d \n ",client_connections+1);
        for (int i = 0; i < count; i++) { 
            printf("path = %s\n", filenames[i]);  //prints all the data in filenames array
        }
        printf("Count of directory names send to client - %d is  %d \n",client_connections+1, count);
    }
    
}
//dirlist_a end/

//dirlist_t start/
void sort_last_timestamp()
{
    char* temp=malloc(500);
    int n=count;
    while(n>0) // iterates for every string
    {
        if (strcmp(creation_times[n-1],creation_times[n])>0) // if timestamps are in decreasing order then it swaps the timestamps and string in the filenames array
        {
            strcpy(temp,filenames[n]);
            strcpy(filenames[n],filenames[n-1]);
            strcpy(filenames[n-1],temp);

            strcpy(temp,creation_times[n]);
            strcpy(creation_times[n],creation_times[n-1]);
            strcpy(creation_times[n-1],temp);
        }
        else
        {
            return;

        }
        n--; 
    }
}

int dirlist_timestamp(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (typeflag == FTW_D  && strcmp(root_dir,fpath)!=0) {
        const char *filename = extractfilename(fpath); 
        strftime(creation_times[count], sizeof(creation_times[count]), "%Y-%m-%d %H:%M:%S", localtime(&sb->st_ctime)); // if stores the creation time stamp in the creation_times array
        printf("path = %s\n",filename);
        printf("count = %d \n", count);
        strcpy(filenames[count], fpath); 
        sort_last_timestamp(); // sorts the arrays based on the timestamp of the files
        count++;
        file_found=1; //file_found flag is set
    }
    return 0;  
}

void dirlist_t()
{
    if (nftw(root_dir,dirlist_timestamp, 20, FTW_PHYS) == -1) {     // calls the dirlist_timestamp function for every file the root_dir
        perror("nftw");
        return;
    }
    if(file_found==0)
    {
        printf("Sending 'File not found' to client - %d \n",client_connections+1);
        strcpy(filenames[count++],"File not found");  // stores the string " No file found" into the filenames array
    } 
    else{
        printf("Below Directory list is sent to client - %d \n ",client_connections+1);
        for (int i = 0; i < count; i++) {
            printf("path = %s\n", filenames[i]);
        }
        printf("Count of directory names send to client -%d is  %d \n",client_connections+1, count);
    }
    
    
}
//dirlist_t end/


//w24fn filename start/
int file_details(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) { 
    if (typeflag == FTW_F) { //traversing only files
        const char *filename = extractfilename(fpath); //extracting filename from filepath
        printf("path = %s\n",fpath);
        if(strcmp(filename,file)==0) //If the client's input matches the file stored on server's filesystem
        {
            file_found=1; //flag which verifies if a file exists or not
            char message[100];

            sprintf(message, "Filename: %s", filename);
            strcpy(filenames[count++], message); 
            sprintf(message, "Size (bytes): %ld", sb->st_size);
            strcpy(filenames[count++], message); 
            sprintf(message, "Date created: %s", ctime(&sb->st_ctime));
            message[strlen(message)-1]='\0';
            printf("message - %s \n",message);
            strcpy(filenames[count++], message); 

            //Storing the file permissions input message buffer
            sprintf(message, "%c%c%c%c%c%c%c%c%c%c",
                                    (S_ISDIR(sb->st_mode)) ? 'd' : '-',
                                    (sb->st_mode & S_IRUSR) ? 'r' : '-',
                                    (sb->st_mode & S_IWUSR) ? 'w' : '-',
                                    (sb->st_mode & S_IXUSR) ? 'x' : '-',
                                    (sb->st_mode & S_IRGRP) ? 'r' : '-',
                                    (sb->st_mode & S_IWGRP) ? 'w' : '-',
                                    (sb->st_mode & S_IXGRP) ? 'x' : '-',
                                    (sb->st_mode & S_IROTH) ? 'r' : '-',
                                    (sb->st_mode & S_IWOTH) ? 'w' : '-',
                                    (sb->st_mode & S_IXOTH) ? 'x' : '-');

            // Add the permissions string to filenames array
            sprintf(filenames[count++], "File permissions: %s", message);
            return 1;
        }
        
    }
    return 0;  
}

void w24fn()
{
    char *token = strtok(command, " ");
    strcpy(file, strtok(NULL, " "));
    if (nftw(root_dir,file_details, 20, FTW_PHYS) == -1) { //Traversing file_details function for every file in root_dir 
        perror("nftw");
        return;
    }
    if(file_found==0)
    { //If the client's search returns no files
        printf("Sending 'File not found' to client - %d \n",client_connections+1);
        strcpy(filenames[count++],"File not found");
    } 
    else
    {
        printf("Below File Details are send to Client - %d \n ",client_connections+1);
        for (int i = 0; i < count; i++) {
            printf("%s\n", filenames[i]);
        }
    }
    
}
//w24fn filename end/


//w24fz filename start/
int file_size(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    //printf("hello wo\n");
    if (typeflag == FTW_F) { //Traversing file_details function for every file in root_dir 
        const char *filename = extractfilename(fpath); 
        long long size=sb->st_size; //Storing the size of the file
        if(size>=size1 && size<=size2) //fetching files within size1 and size2 contraints
        {
            strcpy(filenames[count],fpath);
            create_tar_file(fpath); //Calling function to generate tar archive
            file_found=1; //updating flag
            count++;
        }
    }
    return 0; 
}

void w24fz()
{
    char *token = strtok(command, " ");
    size1 = atoi(strtok(NULL, " "));
    size2 = atoi(strtok(NULL, " "));
    //printf("size 1 = %d , size 2 = %d \n",size1,size2);
    if (nftw(root_dir,file_size, 20, FTW_PHYS) == -1) { //Iterating file_size function for every file in root_dir
        perror("nftw");
        return;
    }
    if(file_found==0)
    { //if file doesn't exist
        printf("Sending 'File not found' to client - %d \n",client_connections+1);
        strcpy(filenames[count++],"File not found");
    }
    else
    {
        printf(" Below list of files are being zipped and sent to client - %d\n",client_connections+1);
        for (int i = 0; i < count; i++) {
            printf("path = %s\n", filenames[i]);
        }
        printf("Count of file names sent to client - %d is  %d \n",client_connections+1, count);
        generateGunZip();
    }
}
//w24fz filename start/


//w24ft extension start/
int extension_files(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (typeflag == FTW_F) {
        const char *filename = extractfilename(fpath); 
        const char *fileextension = strrchr(filename, '.'); // extracts the string from last slash to end
        if (fileextension == NULL) {  // if filename is null then there is no slash in the string so returns path
            return 0;
        } else {
            fileextension++;  // removes the '/' and sends only filename
        }
        //printf("path = %s\n",fpath);

        //Checking if fileextensions match on server's filesystem
        if((extension1!=NULL && strcmp(fileextension,extension1)==0) || (extension2!=NULL && strcmp(fileextension,extension2)==0) || (extension3!=NULL && strcmp(fileextension,extension3)==0))
        {
            //printf("path = %s\n",fpath);
            //printf("file extension - %s\n",fileextension);
            strcpy(filenames[count],fpath);
            long long size=sb->st_size; //storing the size of the file
            create_tar_file(fpath); //calling function to create a tar archive
            file_found=1; //updating flag
            count++;
        }
    }
    return 0; 
}

void w24ft()
{

    char *token = malloc(100);
    token=strtok(command, " ");
    int m=0;

    while(token!=NULL)
    {
        arr[m] = malloc(strlen(token) + 1); // Allocate memory for the token
        strcpy(arr[m], token);
        m++;
        token=strtok(NULL, " ");
    }

    for(int j=m;j<4;j++)
    {
        arr[j] = malloc(1);
        arr[j][0]='\0';
    }
    //copying file extensions
    strcpy(extension1,arr[1]);
    strcpy(extension2,arr[2]);
    strcpy(extension3,arr[3]);
    //printf("extensions - %s %s %s \n", extension1,extension2,extension3);
    if (nftw(root_dir,extension_files, 20, FTW_PHYS) == -1) { //Calling extension_files function for every file in the root_dir  
        perror("nftw");
        return;
    }
    if(file_found==0)
    { //If file doesn't exist
        printf("Sending 'File not found' to client - %d \n",client_connections+1);
        strcpy(filenames[count++],"File not found");
    }
    else
    {
        printf("Below list of files are being zipped and sent to client - %d\n",client_connections+1);
        for (int i = 0; i < count; i++) {
            printf("path = %s\n", filenames[i]);
        }
        printf("Count of file names sent to client - %d is  %d \n",client_connections+1, count);
        generateGunZip();
    }
    
}
//w24ft extension end/

//w24fdb extension start/
int lessthan_dates_files(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (typeflag == FTW_F) { //checking if the current filepath contains a file or not
        const char *filename = extractfilename(fpath); 
        char creation_time[20]; // Allocate a buffer for the date
        //printf("path - %s \n",fpath);
        // Extract month, date, and year from ctime
        char message[100];
        sprintf(message, "%s", ctime(&sb->st_ctime));
        char* token = strtok(message, " ");
        char* m = strtok(NULL, " ");
        char* date = strtok(NULL, " ");
        strtok(NULL, " "); // Skip the time part
        char* year = strtok(NULL, " ");
        year[strlen(year)-1] = '\0'; // Remove the newline character from the year

        // Find the index of the month in the month array
        int i;
        for(i = 0; i < 12; i++) {
            if (strcmp(m, month[i]) == 0) {
                break;
            }
        }
        // Convert month index to string format with leading zero if needed
        char mon[3];
        sprintf(mon, "%02d", i + 1);
        sprintf(date, "%02d", atoi(date)); //appending 0 in day where day <=9
        // Combine year, month, and date to form the creation time string
        sprintf(creation_time, "%s-%s-%s",year,mon,date); //Converting dateformat to required format
       // printf("creation_time - %s , user_date - %s , strcmp(creation_time,user_date) - %d \n",creation_time,user_date,strcmp(creation_time,user_date));
        if(strcmp(creation_time,user_date)<=0)
        {   //Checking for files with date of creation < user_date
            //printf("creation_time - %s , user_date - %s , strcmp(creation_time,user_date) - %d \n",creation_time,user_date,strcmp(creation_time,user_date));
            strcpy(filenames[count],fpath);
            create_tar_file(fpath); //appending the files to a tar archive
            file_found=1; //updating flag
            count++;
        }
    }
    return 0; 
}


void w24fdb()
{
    char *token = strtok(command, " ");
    strcpy(user_date, strtok(NULL, " "));
    if (nftw(root_dir,lessthan_dates_files, 20, FTW_PHYS) == -1) { //Iterating lessthan_dates_files function for every file in the root_dir 
        perror("nftw");
        return;
    }
    if(file_found==0)
    { //File doesn't exist
        printf("Sending 'File not found' to client - %d \n",client_connections+1);
        strcpy(filenames[count++],"File not found");
    }
    else
    {
        printf("Below list of files are being zipped and sent to client - %d\n",client_connections+1);
        for (int i = 0; i < count; i++) {
            printf("path = %s\n", filenames[i]);
        }
        printf("Count of file names sent to client - %d is  %d \n",client_connections+1, count);
        generateGunZip(); //generating a tar.gz archive
    }
}
//w24fdb extension end/


//w24fda extension start/
int greaterthan_dates_files(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (typeflag == FTW_F) {
        const char *filename = extractfilename(fpath); 

        // Print creation time using strftime
        char creation_time[20]; // Allocate a buffer for the date

        // Extract month, date, and year from ctime
        char message[100];
        sprintf(message, "%s", ctime(&sb->st_ctime));
        char* token = strtok(message, " ");
        char* m = strtok(NULL, " ");
        char* date = strtok(NULL, " ");
        strtok(NULL, " "); // Skip the time part
        char* year = strtok(NULL, " ");
        year[strlen(year)-1] = '\0'; // Remove the newline character from the year

        // Find the index of the month in the month array
        int i;
        for(i = 0; i < 12; i++) {
            if (strcmp(m, month[i]) == 0) {
                break;
            }
        }

        // Convert month index to string format with leading zero if needed
        char mon[3];
        sprintf(mon, "%02d", i + 1);
        sprintf(date, "%02d", atoi(date)); //appending 0 in day where day <=9
        // Combine year, month, and date to form the creation time string
        sprintf(creation_time, "%s-%s-%s", year, mon, date);

        // Compare creation time with user provided date
        if(strcmp(creation_time, user_date) >= 0) {
            strcpy(filenames[count], filename);
            create_tar_file(fpath);
            file_found = 1;
            count++;
        }
    }
    return 0;
}


void w24fda()
{
    char *token = strtok(command, " ");
    strcpy(user_date, strtok(NULL, " "));
    if (nftw(root_dir,greaterthan_dates_files, 20, FTW_PHYS) == -1) {  //Calling greaterthan_dates_files function for every file in root_dir 
        perror("nftw");
        return;
    }
    if(file_found==0)
    { //File doesn't exist
        printf("Returning 'File not found' to client - %d \n",client_connections+1);
        strcpy(filenames[count++],"File not found");
    }
    else
    {
        printf("Below list of files are being zipped and sent to client - %d\n",client_connections+1);
        for (int i = 0; i < count; i++) {
            printf("path = %s\n", filenames[i]);
        }
        printf("Count of file names sent to client - %d is  %d \n",client_connections+1, count);
        generateGunZip(); //creating a tar.gz archive
    }
    
}
//w24fda extension end/


//Processes client's request
void child(int sd){
    while(1){
        count=0;
        file_found=0;
        memset(command,0,sizeof(command));
        if(recv(sd, command, sizeof(command), 0)<0){
            close(sd);
            fprintf(stderr,"Client is dead, wait for a new client\n");
            exit(0);
        }
        //fprintf(stderr, "Server - Client sent back this message: %s\n", command);
        command[strlen(command)] = '\0';
        printf("Command from client - %d : %s\n", client_connections+1,command);
        if(strcmp(command, "dirlist -a") == 0) { //for dirlist -a
            dirlist_a();
            send_message_to_client(sd);
        } 
        else if(strcmp(command,"dirlist -t") == 0) { //for dirlist -t
            printf("working dirlist -t\n");
            dirlist_t();
            send_message_to_client(sd);
        }
        else if(strstr(command, "w24fn")) //for w24fn
        {
            printf("working w24fn\n");
            w24fn();
            send_message_to_client(sd);
        }
        else if(strstr(command, "w24ft")) //for w24ft
        {
            printf("working w24ft\n");
            w24ft();
            if(file_found==1)
            {
                send_tar_to_client(sd); //Passing the tar archive to client
            }
            else{
                send_message_to_client(sd); //passing no file found to client
            }
                   
        }
        else if(strstr(command, "w24fz")) //for w24fz
        {
            printf("working w24fz\n");
            w24fz();
            if(file_found==1)
            {
                send_tar_to_client(sd);  //Passing the tar archive to client
            }
            else
            {
                send_message_to_client(sd); //passing no file found to client
            }
                       
        }
        else if(strstr(command, "w24fdb")) //for w24fdb
        {
            printf("working w24fdb\n");
            w24fdb();
            if(file_found==1)
            {
                send_tar_to_client(sd); //Passing the tar archive to client
            }
            else
            {
                send_message_to_client(sd); //passing no file found to client
            }      
        }
        else if(strstr(command, "w24fda")) //for w24fda
        {
            printf("working w24fda\n");
            w24fda();
            if(file_found==1)
            {
                send_tar_to_client(sd); //Passing the tar archive to client
            }
            else
            {
                send_message_to_client(sd); //passing no file found to client
            }
        }
        else if(strcmp(command,"quitc") == 0) { //for quit c
            close(sd);
            exit(0);
        }
    }
}

int main(int argc, char *argv[]){
    int sd, client_fd, portNumber;
    socklen_t len;
    struct sockaddr_in servAdd;
    char mirror1_fd_str[10],mirror2_fd_str[10];
    char message[255];
    getcwd(current_path,100);

    //port numbers for mirror1 and mirror2
    int mirror1_fd=9191,mirror2_fd=9192;
    root_dir=getenv("HOME");

    //Formatting mirror port numbers
    sprintf(mirror1_fd_str, "%d", mirror1_fd);
    sprintf(mirror2_fd_str, "%d", mirror2_fd);

    //validating arg count
    if(argc != 2){
        printf("Call model: %s <Port #>\n", argv[0]);
        exit(0);
    }

    if ((sd=socket(AF_INET,SOCK_STREAM,0))<0){
    fprintf(stderr, "Cannot create socket\n");
    exit(1);
    }

    servAdd.sin_family = AF_INET;
    servAdd.sin_addr.s_addr = htonl(INADDR_ANY);
    sscanf(argv[1], "%d", &portNumber);
    servAdd.sin_port = htons((uint16_t)portNumber);
    
    bind(sd,(struct sockaddr*)&servAdd,sizeof(servAdd));
    
    listen(sd, 5);
    
    printf("Server is running at port number - %d \n",portNumber);
    while(1){
    
        //Accepting the client's request
        if ((client_fd = accept(sd, (struct sockaddr *)NULL, NULL)) < 0) {
            fprintf(stderr, "Accept failed\n");
            exit(1);
        }
        
        //Logic for port switching between server, mirror1 and mirror2 to handle multiple client requests
        if(client_connections < 3 || (client_connections >= 9 && client_connections % 3 == 0)) { //server
            
            printf("connected client - %d \n", client_connections+1);
            memset(message, 0, sizeof(message));
            strcpy(message, "");
            strcpy(message,"server");
            int sdfg = send(client_fd, message, sizeof(message), 0);
                if(fork()==0) {
                    close(sd);
                    child(client_fd);
                    exit(0);   
                }
        } else if(client_connections >= 3 && client_connections < 6 || (client_connections >= 9 && client_connections % 3 == 1)) { //mirror1
            printf("redirecting client - %d to mirror_1\n", client_connections+1);
            if(fork() == 0) {
                //close(sd);
                strcpy(message,mirror1_fd_str);
                send(client_fd, message, strlen(message)+1, 0);
                close(client_fd);
                exit(0); 
            }
        } else if(client_connections >= 6 && client_connections < 9 || (client_connections >= 9 && client_connections % 3 == 2)) { //mirror2
            printf("redirecting client - %d to mirror_2\n", client_connections+1);
            if(fork() == 0) {
                //close(sd);
                strcpy(message,mirror2_fd_str);
                send(client_fd, message, strlen(message)+1, 0);
                close(client_fd);
                exit(0); 
            }
        }

        //terminating socket connection of client
        close(client_fd);
        client_connections++;
    }

}