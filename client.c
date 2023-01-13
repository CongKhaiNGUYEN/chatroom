#include <stdio.h>
#include <string.h> 
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#define BUFFER_SIZE 512


int date_flag = 0;

void drop_left(char *s, int n)
{
   char* s2 = s + n;
   while ( *s2 )
   {
      *s = *s2;
      ++s;
      ++s2;
   }
   *s = '\0';
}



//******************************************************************************
//*                              File                                          *
//******************************************************************************

void write_file(int sockfd){
  int n;
  FILE *fp;
  char *filename = "recv.txt";
  fp = fopen(filename, "w+");
  char buffer[1024];
  bzero(buffer, 1024);
  while (1) {
    n = recv(sockfd, buffer, 1024, 0);
    if (strncmp(buffer,"END_OF_FILE",strlen("END_OF_FILE"))==0){
        printf("\nend-of-file\n");
        fclose(fp);
        return;
    }
    printf("%s", buffer);
    fprintf(fp, "%s", buffer);
    bzero(buffer, 1024);
  }
}


 void send_file(int sockfd){
    FILE *fp;
    char *filename = "send_file.txt";
    fp = fopen(filename, "r");
    if (fp == NULL) {
    perror("[-]Error in reading file.");
    return;
    }
    char data[1024];

  while(fgets(data, 1024, fp) != NULL) {
      //printf("%s", data);
    if (send(sockfd, data, sizeof(data), 0) == -1) {
      perror("[-]Error in sending file.");
      return;
    }
    //printf("[+]File data sent successfully.\n");
    bzero(data, 1024);
  }
  send(sockfd, "END_OF_FILE", 11 , 0);
  printf("[+]File data sent successfully.\n");
  fclose(fp);
}

//******************************************************************************
//*                          end File                                          *
//******************************************************************************




int main(int argc, char *argv[])
{
    int socket_desc;
    struct sockaddr_in server;
    char buf[BUFFER_SIZE];
    char temp_buf[BUFFER_SIZE+10];
    int port;

    int fd;
    int nchar;

    time_t now;
    int hours, mins,seconds;
    struct tm *local;

    if (argc < 3){
        printf("%s <server's ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }

    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;
    if (sscanf(argv[2], "%d", &port) != 1)
    {
        puts("Erreur: Le paramètre NOMBRE doit être un nombre entier !");
        return EXIT_FAILURE;
    }
    server.sin_port = htons(port);

    if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        puts("connect error");
        return 1;
    }

    puts("Connected\n");
    puts("Enter nickname :\n");
    bzero(buf, BUFFER_SIZE);
    scanf("%s", buf);
    strcat(buf,"\n"); 
    if (send(socket_desc, buf, strlen(buf), 0) < 0)
    {
        puts("send failed");
    }
    

    fcntl(socket_desc, F_SETFL, fcntl(socket_desc, F_GETFL) | O_NONBLOCK);
    fcntl(STDIN_FILENO, F_SETFL, fcntl(socket_desc, F_GETFL) | O_NONBLOCK);

    while (1)
    {
        bzero(buf, BUFFER_SIZE);
        if (recv(socket_desc, buf, BUFFER_SIZE - 1, 0) < 0)
        {
            if (errno != EAGAIN)
                puts("recv failed");
        }
        else if (strncmp(buf,"/exit",strlen("/exit")) == 0 || strncmp(buf,"/Exit",strlen("/Exit")) == 0 || strncmp(buf,"/EXIT",strlen("/EXIT"))==0){
            break;
        }
        else if(strncmp(buf,"/send",strlen("/send")) == 0){
            send_file(socket_desc);
            continue;
        }
        else if(strncmp(buf,"/date",strlen("/date")) == 0){
            if (date_flag){
                date_flag = 0;
            }else{
                date_flag = 1;
            }
            continue;
        }
        else if(strncmp(buf,"/file",strlen("/file")) == 0){
            write_file(socket_desc);
            continue;
        }
        else
        {
            if (date_flag){
                time(&now);
                local=localtime(&now);
                hours = local->tm_hour;          
                mins = local->tm_min;         
                seconds = local->tm_sec; 
                if (hours < 12)    
                    printf("%02d:%02d:%02dam: ", hours, mins, seconds);         
                else    
                    printf("%02d:%02d:%02dpm: ", hours - 12, mins, seconds);
            }
            printf("%s",buf);
            printf("\033[0m\n");
        }
        bzero(buf, BUFFER_SIZE);
        if (read(STDIN_FILENO, buf, BUFFER_SIZE - 1) < 0)
        {   
            if (errno != EAGAIN)
                puts("recv failed");
        }
        else
        {   
            send(socket_desc, buf, strlen(buf), 0);
        }
        usleep(100000);
    }

    close(socket_desc);
    return 0;
}