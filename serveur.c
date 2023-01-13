/**
	Handle multiple socket connections with select and fd_set on Linux
*/

#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>    //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <ctype.h>  //toupper()

#define TRUE 1
#define FALSE 0
#define PORT 8888

//color code
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

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

int count_letter(char* string){
    if (string == NULL) return 0;
    int l = sizeof(string);
    char ch;
    for (int i = 0; i < l; i++){
        ch = string[i];
        if (ch == ' ') return i;
        if (ch == '\n') return i;
    }
    return l;
}

void upper_case(char * temp) {
    char * name;
    name = strtok(temp,":");

    // Convert to upper case
    char *s = name;
    while (*s) {
      *s = toupper((unsigned char) *s);
      s++;
    }

}



//******************************************************************************
//*                              File                                          *
//******************************************************************************


void redirect(int sock_src,int sock_dest ){
    int n;
    char buffer[1024];
    bzero(buffer, 1024);
    while (1)
    {   
        n = recv(sock_src, buffer, 1024, 0);
        if (strncmp(buffer,"END_OF_FILE",strlen("END_OF_FILE"))==0){
            n = send(sock_dest, "END_OF_FILE",11, 0);
            return;
        }
        n = send(sock_dest, buffer, sizeof(buffer), 0);
    } 
}


//******************************************************************************
//*                              File                                          *
//******************************************************************************


typedef struct ClientUser{
    char nickname[15];
    char color[30];
    char password[200];
    int salon_id;
    int is_mod;
}ClientUser;

//*****************************Create Salon*************************************
typedef struct Salon{
    char moderateur[30];
    int members[30];
    int memeber_socket_id[30];
    char member_list[30][30];
    char salon_name[30];
    char password[30];
}Salon;
//******************************************************************************

int main(int argc, char *argv[])
{
    int opt = TRUE;
    int master_socket, addrlen, new_socket, client_socket[30], max_clients = 30, activity, i, j, valread,temp_count,temp_count_2, sd;
    int max_sd;
    int temp_dest;
    Salon room[10];
    ClientUser cliUser[30];
    for (int i = 0; i < 30; i ++){
        strcpy(cliUser[i].nickname, "");
        strcpy(cliUser[i].password, "");
        strcpy(cliUser[i].color, "");
        cliUser[i].salon_id = -1;
        cliUser[i].is_mod = FALSE;
    }
    //char nickname[30][8];
    //char password[30][200];
    char temp_nickname[1025];
    struct sockaddr_in address;
    int port = PORT;

    char buffer[1025]; //data buffer of 1K
    char temp_buffer[1035];
    char command[1025];
    //char color[30][1025];
    if (argc == 2){
        port = atoi(argv[1]);
    }
    //init color list
    for (i = 0; i < max_clients; i++)
    {
        strcpy(cliUser[i].color,ANSI_COLOR_CYAN);
    }
    //set of socket descriptors
    fd_set readfds;

    char *message;
    char *message_welcome = "Welcome : "; 

    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }
    for (int r = 0; r < 10; r ++){
        strcpy(room[r].moderateur,"");
        for (int ij = 0; ij < 30; ij++){
            room[r].members[i] = 0;
        }
    }
    //create a master socket
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    //bind the socket to localhost port 8888
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", port);

    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    while (TRUE)
    {
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        //add child sockets to set
        for (i = 0; i < max_clients; i++)
        {
            //socket descriptor
            sd = client_socket[i];

            //if valid socket descriptor then add to read list
            if (sd > 0)
                FD_SET(sd, &readfds);

            //highest file descriptor number, need it for the select function
            if (sd > max_sd)
                max_sd = sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR))
        {
            printf("select error");
        }

        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(master_socket, &readfds))
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            
            //check unique nickname
            i=0;
            char tempor[30];
            while(i!=max_clients)
            {
                bzero(buffer, 1024);
                read(new_socket, buffer, 1024);
                strcpy(tempor, buffer);
                for (int co = 0; co < strlen(buffer); co ++){
                    if (buffer[co] == '\n'){
                        tempor[co] = ' ';
                        tempor[co + 1] = '\0';
                        break;
                    }
                }
                for (i=0; i < max_clients; i++)
                {
                    if (strcmp(tempor,cliUser[i].nickname)==0)
                        {   
                            if (send(new_socket, "\x1b[31mExisted nickname!\n",23, 0) != 23)
                            {
                                perror("send");
                            }
                            else
                                break;
                        }
                }

            }
            printf("%s\n", buffer);

            

            //send new connection greeting message
            message = calloc(strlen(message_welcome) + strlen(buffer) + 11, sizeof(char));
            bzero(message, strlen(message_welcome) + strlen(buffer) + 2);
            strncpy(message, message_welcome, strlen(message_welcome));
            strcat(message,"\x1b[36m");
            strncat(message, buffer, strlen(message_welcome) + strlen(buffer)+5);
            strcat(message,"\x1b[0m");
            strncat(message, "\n", strlen(message_welcome) + strlen(buffer)+9 + 1);
            if (send(new_socket, message, strlen(message), 0) != strlen(message))
            {
                perror("send");
            }

            puts("Welcome message sent successfully");

            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++)
            {
                //if position is empty
                if (client_socket[i] == 0)
                {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n", i);
                    bzero(cliUser[i].nickname, 8);
                    strncpy(cliUser[i].nickname, tempor, 7);

                    break;
                }
            }
        }

        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds))
            {
                bzero(buffer, 1024);
                //Check if it was for closing , and also read the incoming message
                if ((valread = read(sd, buffer, 1024)) <= 0)
                {
                    //Somebody disconnected , get his details and print
                    getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    close(sd);
                    client_socket[i] = 0;
                }
                else
                {   
                    //set the string terminating NULL byte on the end of the data read
                    buffer[valread] = '\0';
                    printf("message from %d : %s \n", sd, buffer);

                    strcpy(command,"/msg "); //set command as /msg

                    if(strncmp(buffer,command,strlen(command))==0) //check /msg command this help send private message 
                    {   
                        //read the name of the user to send message
                        temp_count=0;
                        while(buffer[temp_count+5]!=' ')
                        {   
                            temp_nickname[temp_count]=buffer[temp_count+5];
                            temp_count++;
                        }
                        temp_nickname[temp_count]='\0';
                        strcat(temp_nickname,"\n\0");

                        //find the user in nickname list
                        temp_count=0;
                        while(temp_count<max_clients)
                        {
                            if(strcmp(temp_nickname,cliUser[temp_count].nickname)==0) // this one 
                                break;
                            
                            temp_count++;
                        }
                        // printf("%d",temp_count);
                        if (temp_count==max_clients)
                        {
                            
                            send(client_socket[i],"\x1b[31mIncorrect nickname! \n",26,0);
                        }
                        else
                        {
                            //delete the command and nickname in buffer
                            drop_left(buffer,strlen(command)+strlen(temp_nickname));
                            
                            //this part to send private message
                            strcpy(temp_nickname,cliUser[i].nickname);
                            strcat(temp_nickname,cliUser[i].nickname);
                            temp_nickname[strlen(temp_nickname)-1]='\0';
                            
                            send(client_socket[temp_count], "Private message from : ", 23, 0);
                            if(send(client_socket[temp_count], temp_nickname, strlen(temp_nickname), 0) <0)
                                printf("erron send ");
                            send(client_socket[temp_count], "\x1b[0m : ", 7, 0);
                            send(client_socket[temp_count], buffer, strlen(buffer), 0);
                        }
                    }

                    //some thing wrong with this one
                    else if (strncmp(buffer,"/nickname ",strlen("/nickname "))==0) //check /nick command this help change the nickname
                    {
                        drop_left(buffer,strlen("/nickname "));
                        char temp_name[30], temps_password[30], check_valid[30];
                        int count = count_letter(buffer) + 1;
                        if (count < strlen(buffer)){
                            strncpy(temp_name, buffer, count);
                            temp_name[count] = '\0';
                            drop_left(buffer,count+1);
                            strcpy(temps_password, buffer);
                            strncpy(check_valid, buffer, strlen(temp_name));
                            int check_flag = 0;
                            if (strncmp(check_valid, temp_name, strlen(temp_name))==0){
                                for (int k = 0; k < max_clients; k++){
                                    if (strcmp(cliUser[k].nickname, temp_name) == 0){
                                        send(client_socket[i], "\x1b[31m nickname existed!!!\n",26, 0);
                                        check_flag = 1;
                                        break;
                                    }
                                }
                                if (check_flag == 0){

                                    strcpy(cliUser[i].nickname,temp_name);
                                    send(client_socket[i], "\x1b[31mNickname is changed!\n",26, 0);
                                    continue;
                                }

                            }else{
                                send(client_socket[i], "\x1b[31mSomething Wrong\n",21, 0);
                            }

                        }else{
                            strncpy(temp_name, buffer, count);
                            temp_name[count-1] = ' ';
                            temp_name[count] = '\0';
                            int check_flag = 0;
                            for (int k = 0; k < max_clients; k++){
                                printf("%s", cliUser[k].nickname);
                                    if (strcmp(cliUser[k].nickname, temp_name) == 0){
                                        send(client_socket[i], "\x1b[31m nickname existed!!!\n",26, 0);
                                        check_flag = 1;
                                        break;
                                    }
                            }
                            if (check_flag == 0){
                                strcpy(cliUser[i].nickname,buffer);
                                send(client_socket[i], "\x1b[31mNickname is changed!\n",26, 0);
                                continue;
                            }else{
                                //
                            }
                        } 
                        for (int k = 0; k < max_clients; k++){
                            if (strcmp(cliUser[k].nickname, temp_name) == 0)
                                if (strlen(cliUser[k].password) != 0)
                                        if (strcmp(cliUser[k].password, temps_password)!=0){
                                            send(client_socket[i], "\x1b[31mPassword is not correct!\n",30, 0);
                                            break;
                                        }
                                        else{
                                                strcpy(cliUser[i].nickname,temp_name);
                                                strcpy(cliUser[k].nickname, "anonymous ");
                                                send(client_socket[i], "\x1b[31mNickname is changed!\n",26, 0);
                                                char msssg[] = "\x1b[31mSomeone has logged in your account with password\n"
                                                                "Now you are <<anonymous>>. You have to change your nickname now\n"
                                                                "/nickname <new nick-name> or /register <new nickname>  <password>\n";
                                                send(client_socket[k], msssg,strlen(msssg), 0);
                                                break;
                                        }
                        }
                    }else if (strncmp(buffer,"/register ",strlen("/register "))==0){
                        drop_left(buffer,strlen("/register "));
                        char temp_name[30], temps_password[30];
                        int count = count_letter(buffer) + 1;
                        if (count < sizeof(buffer)){
                            strncpy(temp_name, buffer, count);
                            temp_name[count] = '\0';
                            drop_left(buffer,count+1);
                            strcpy(temps_password, buffer);
                            for (int k = 0; k < max_clients; k++){
                                if (strcmp(cliUser[k].nickname, temp_name) == 0){
                                    if (strcmp(cliUser[k].password,"")==0){
                                        strcpy(cliUser[k].nickname,temp_name);
                                        strcpy(cliUser[k].password,temps_password);
                                        break;
                                    }
                                    send(client_socket[i], "\x1b[31mNickname has been existed with password!\n",46, 0);
                                    break;
                                }
                                if (strcmp(cliUser[k].nickname, "") == 0){
                                    strcpy(cliUser[k].nickname,temp_name);
                                    strcpy(cliUser[k].password,temps_password);
                                    break;
                                }
                            }
                        }
                        else{
                            send(client_socket[i], "\x1b[31mInvalid command!\n",22, 0);
                            continue;
                        }

                    }else if (strncmp(buffer,"/unregister ",strlen("/unregister "))==0){
                        drop_left(buffer,strlen("/unregister "));
                        char temp_name[30], temps_password[30], check_valid[30];
                        int count = count_letter(buffer) + 1;
                        if (count < sizeof(buffer)){
                            strncpy(temp_name, buffer, count);
                            temp_name[count] = '\0';
                            drop_left(buffer,count+1);
                            strcpy(temps_password, buffer);
                            strncpy(check_valid, buffer, strlen(temp_name));
                            for (int k = 0; k < max_clients; k++){
                            if (strcmp(cliUser[k].nickname, temp_name) == 0)
                                if (strlen(cliUser[k].password) != 0)
                                        if (strcmp(cliUser[k].password, temps_password)!=0){
                                            send(client_socket[i], "\x1b[31mPassword is not correct!\n",30, 0);
                                            break;
                                        }
                                        else{
                                            strcpy(cliUser[k].nickname,"");
                                            strcpy(cliUser[k].password,"");
                                            break; 
                                        }
                            }
                            continue;
                        }else{
                            send(client_socket[i], "\x1b[31mInvalid nickname or password\n",34, 0);
                        }
                    }else if (strncmp(buffer,"/join ",strlen("/join "))==0){
                        drop_left(buffer,strlen("/join "));
                        char room_name[30];
                        strcpy(room_name, buffer);
                        room_name[strlen(buffer)] = '\0';
                        for (int co = 0; co < 10; co++){
                            if (strncmp(room_name, room[co].salon_name, strlen(room_name))==0){
                                //printf("\nroom name\n");
                                for (int k = 0; k<30; k++){
                                    if (room[co].members[k] == 0){
                                        //printf("\n|in if|\n");
                                        room[co].members[k]=client_socket[i];
                                        room[co].memeber_socket_id[k] = i;
                                        strcpy(room[co].member_list[k], cliUser[i].nickname);
                                        cliUser[i].salon_id = co;
                                        break;
                                    }
                                }
                                break;
                            }
                            if (strcmp(room[co].moderateur,"")==0){
                                room[co].members[0] = client_socket[i];
                                room[co].memeber_socket_id[0] = i;
                                strcpy(room[co].member_list[0], cliUser[i].nickname);
                                strcpy(room[co].moderateur, cliUser[i].nickname);
                                strcpy(room[co].salon_name, room_name);
                                cliUser[i].salon_id = co;
                                cliUser[i].is_mod = TRUE;
                                break;
                            }
                        }
                        for (int co = 0; co < 10; co++){
                            if (strcmp(room[co].moderateur, "")!=0){
                                printf("-------------------------------------------------------------\n");
                                printf("moderateur:  %s\n",room[0].moderateur);
                                printf("salon_name:  %s\n", room[0].salon_name);
                                for (int kk = 0 ; kk<30; kk++){
                                    if (strcmp(room[co].member_list[kk],"") != 0){
                                        printf("memeber %d:  %s\n", kk, room[co].member_list[kk]);
                                    }
                                }
                                printf("-------------------------------------------------------------\n");
                            }
                        }
                    }else if (strncmp(buffer,"/kick ",strlen("/kick "))==0){
                        if (!(cliUser[i].is_mod)){
                            send(client_socket[i], "\x1b[31mMust be a moderater\n",25, 0);
                            continue;
                        }
                        drop_left(buffer,strlen("/join "));
                        char mem_name[30];
                        strcpy(mem_name, buffer);
                        for (int ji =0; ji<30;ji++){
                            if (strncmp(room[cliUser[i].salon_id].member_list[ji], mem_name, strlen(mem_name)-1)==0){
                                int client_sock = room[cliUser[i].salon_id].members[ji];
                                int client_id = room[cliUser[i].salon_id].memeber_socket_id[ji];
                                cliUser[client_id].salon_id = -1;
                                cliUser[client_id].is_mod = FALSE;
                                strcpy(room[cliUser[i].salon_id].member_list[ji],"");
                                room[cliUser[i].salon_id].members[ji] = 0;
                                send(client_sock, "\x1b[31mYou have been kicked by mod\n",33,0); 
                                send(client_socket[i], "\x1b[33mSuccesfully\n", 17,0);
                                continue;
                            }
                        }
                    }
                    else if (strncmp(buffer,"/check ",strlen("/check "))==0 || strncmp(buffer,"/check",strlen("/check"))==0){
                        int check_fl = 0;
                        for (int co = 0; co < 10; co++){
                            if (strcmp(room[co].moderateur, "")!=0){
                                for (int mem = 0; mem < 30; mem++){
                                    if (strcmp(room[co].member_list[mem], cliUser[i].nickname)==0){
                                        char msg[50];
                                        strcpy(msg, "\x1b[36mMember of group:  ");
                                        strcat(msg, room[co].salon_name);
                                        if (strncmp(room[co].moderateur, cliUser[i].nickname, strlen(cliUser[i].nickname))==0){
                                            strcat(msg, "----------moderateur----------\n");
                                        }
                                        send(client_socket[i], msg,strlen(msg), 0);
                                        check_fl = 1;
                                        break;
                                    }
                                }
                                if (check_fl == 1)  break;
                            }
                        }
                        if (check_fl == 0) send(client_socket[i], "\x1b[31m NAH\n",10, 0);
                    }
                    else if (strstr(buffer, "/exit") != NULL || strstr(buffer, "/Exit") != NULL || strstr(buffer, "/EXIT") != NULL){
                        //Somebody disconnected , get his details and print
                        getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                        printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                        //Close the socket and mark as 0 in list for reuse
                        send(client_socket[i], "/EXIT", 5, 0);
                        close(sd);
                        client_socket[i] = 0;

                    }

                    else if (strncmp(buffer,"/help ",strlen("/help "))==0 ||strncmp(buffer,"/help",strlen("/help"))==0 ) //check /help command 
                    {
                        char * help_command = "\x1b[36m\n/nickname <nickname> <password>\n"
                                                "/register <nickname> <password>\n"
                                                "/msg <nickname> <msg>                       (private message)\n"
                                                "/send <nickname>                            (data is stored in send_file.txt)\n"
                                                "/alert <nickname> <message>                 (/alert <message>  will alert all client!)\n"
                                                "/color  <color>                             (red, blue, green, yellow, cyan, magenta)\n"
                                                "/join <room's name>\n"
                                                "/kick <member's nickname>                   (for mod only)\n"
                                                "/date\n"
                                                "/check\n"
                                                "/exit\n";
                        send(client_socket[i], help_command , strlen(help_command), 0);
                    }
                    else if (strncmp(buffer,"/date",strlen("/date")) == 0){
                        send(client_socket[i], "/date", 5, 0);
                    }
                    else if (strncmp(buffer,"/color",strlen("/color"))==0 ){
                        if (strstr(buffer,"red") != NULL )
                        {
                            printf("red");
                            strcpy(cliUser[i].color,ANSI_COLOR_RED);
                            send(client_socket[i], "\x1b[31mColor changed", 18, 0);
                        }
                        else if (strstr(buffer,"blue") != NULL )
                        {
                            printf("blue");
                            strcpy(cliUser[i].color,ANSI_COLOR_BLUE);
                            send(client_socket[i], "\x1b[34mColor changed", 18, 0);
                        }
                        else if (strstr(buffer,"green") != NULL )
                        {
                            printf("green");
                            strcpy(cliUser[i].color,ANSI_COLOR_GREEN);
                            send(client_socket[i], "\x1b[32mColor changed", 18, 0);
                        }
                        else if (strstr(buffer,"cyan") != NULL)
                        {
                            printf("cyan");
                            strcpy(cliUser[i].color,ANSI_COLOR_CYAN);
                            send(client_socket[i], "\x1b[36mColor changed", 18, 0);
                        }
                        else if (strstr(buffer,"yellow") != NULL)
                        {
                            printf("yellow");
                            strcpy(cliUser[i].color,ANSI_COLOR_YELLOW);
                            send(client_socket[i], "\x1b[33mColor changed", 18, 0);
                        }
                        else if (strstr(buffer,"magenta") != NULL)
                        {
                            printf("magenta");
                            strcpy(cliUser[i].color,ANSI_COLOR_MAGENTA);
                            send(client_socket[i], "\x1b[35mColor changed", 18, 0);
                        }
                    }
                    //send file's syntax:    /send <to someone>  
                    //where is the file?   It can be loaded from the client's side
                    else if ((strncmp(buffer,"/send ",strlen("/send "))==0 ))
                    {
                        
                        drop_left(buffer,strlen("/send "));
                        temp_count=0;
                        while( (buffer[temp_count]!=' ') && (temp_count<=10) )
                        {   
                            temp_nickname[temp_count]=buffer[temp_count];
                            temp_count++;
                        }
                        temp_nickname[temp_count]='\0';

                        printf("*%s*",temp_nickname);

                        for (temp_count_2=0; temp_count_2 < max_clients; temp_count_2++)
                        {
                            if (strncmp(temp_nickname,cliUser[temp_count_2].nickname, strlen(temp_nickname)-1)==0)
                                {   
                                    break;
                                }
                        }
                        send(client_socket[i], "/send", 5, 0);
                        send(client_socket[temp_count_2], "/file", 5, 0);
                        redirect(client_socket[i],client_socket[temp_count_2]);

                    }
                    //alert's syntax:   /alert <nickname> <message>
                    else if (strncmp(buffer,"/alert ",strlen("/alert "))==0)
                    {
                        drop_left(buffer,strlen("/alert "));
                        //get the nickname
                        temp_count=0;
                        while( (buffer[temp_count]!=' ') && (temp_count<=10) )
                        {   
                            temp_nickname[temp_count]=buffer[temp_count];
                            temp_count++;
                        }
                        temp_nickname[temp_count]='\0';
                        strcat(temp_nickname,"\n\0");

                        printf("*%s*",temp_nickname);

                        //check the nickname is available to use
                        for (temp_count_2=0; temp_count_2 < max_clients; temp_count_2++)
                        {
                            printf("$%s$",cliUser[temp_count_2].nickname);
                            if (strcmp(temp_nickname,cliUser[temp_count_2].nickname)==0)
                                {   
                                    break;
                                }
                        }
                        
                        printf("%d",temp_count_2);

                        if(temp_count_2==max_clients)
                        {   
                            //send to all client the alert
                            for (j = 0; j < max_clients; j++)
                            {
                                if (i != j && client_socket[j] != 0)
                                {   
                                    upper_case(buffer);
                                    strcpy(temp_nickname,cliUser[i].color);
                                    strcat(temp_nickname,cliUser[i].nickname);
                                    temp_nickname[strlen(temp_nickname)-1]='\0';

                                    send(client_socket[j], temp_nickname, strlen(temp_nickname), 0);
                                    strcpy(temp_buffer,"\x1b[31m (ALERTE ALL): ");
                                    strcat(temp_buffer,buffer);
                                    send(client_socket[j], temp_buffer, strlen(temp_buffer), 0);
                                }
                                
                            }
                        }
                        else
                        {
                            //find that client
                            //send message to only that client
                            drop_left(buffer,strlen(temp_nickname));

                            upper_case(buffer);

                            strcpy(temp_nickname,cliUser[i].color);
                            strcat(temp_nickname,cliUser[i].nickname);
                            temp_nickname[strlen(temp_nickname)-1]='\0';

                            send(client_socket[temp_count_2], temp_nickname, strlen(temp_nickname), 0);
                            strcpy(temp_buffer,"\x1b[31m (ALERTE YOUR): ");
                            strcat(temp_buffer,buffer);
                            send(client_socket[temp_count_2], temp_buffer, strlen(temp_buffer), 0);
                        }

                    }
                    else if (cliUser[i].salon_id == -1)
                    {
                        for (j = 0; j < max_clients; j++)
                        {
                            if (i != j && client_socket[j] != 0)
                            {   
                                if (cliUser[j].salon_id != -1){
                                    continue;
                                }
                                strcpy(temp_nickname,cliUser[i].color);
                                strcat(temp_nickname,cliUser[i].nickname);
                                temp_nickname[strlen(temp_nickname)-1]='\0';
                                send(client_socket[j], temp_nickname, strlen(temp_nickname), 0);
                                send(client_socket[j], "\x1b[0m : ", 7, 0);
                                send(client_socket[j], buffer, strlen(buffer), 0);
                            }
                        }
                    }
                    else{
                        int salon_id = cliUser[i].salon_id;
                        for (int sl = 0; sl < 30; sl++){
                            if (room[salon_id].members[sl] != client_socket[i] && room[salon_id].members[sl] != 0){
                                // printf("+%d\n", client_socket[i]);
                                // printf("-%d\n", room[salon_id].members[sl]);
                                strcpy(temp_nickname,cliUser[i].color);
                                strcat(temp_nickname,cliUser[i].nickname);
                                temp_nickname[strlen(temp_nickname)-1]='\0';
                                send(room[salon_id].members[sl], temp_nickname, strlen(temp_nickname), 0);
                                send(room[salon_id].members[sl], "\x1b[0m : ", 7, 0);
                                send(room[salon_id].members[sl], buffer, strlen(buffer), 0);
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}