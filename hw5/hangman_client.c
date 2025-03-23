#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h> 

struct server_message{

    char msg_flag;
    char data[50];

};
struct server_game_data{

    char msg_flag;
    char word_length;
    char num_incorrect;
    char data[50];

};

struct client_message{

    char msg_length;
    char data[50];

};


/* All of the basic socket programming syntax was derived from the linuxhowtos.org 
link at the end of the socket programming video posted on Canvas.
*/

//https://www.linuxhowtos.org/data/6/client.c

#define h_addr h_addr_list[0] 
//first ip address if there is a list of ips for the host

int main(int argc, char *argv[])
{
    int sockfd, portno;// sockets are just file descriptors, sockfd will be used to call read() and write() on the file descriptor

    struct sockaddr_in serv_addr;//struct used to store information about server such as IP, IP version, and the port to be used
    struct hostent *server;//used the get the address of an input hostname, later the IP will be transferred to 'server'

    char buffer[1024];

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);//gets port number from input

    sockfd = socket(AF_INET, SOCK_STREAM, 0);// AF_INET indicates the IPv4 convention will be used for internet addresses
    //SOCK_STREAM indicates that sock will be a stream socket
    //the third parameter says that the protocol used will be the default protocol for that type of socket. For datagram, UDP, for stream TCP.
    
   
    server = gethostbyname(argv[1]);//again, uses DNS to get IP of host if a domain name is entered rather than an IP
    

    bzero((char *) &serv_addr, sizeof(serv_addr));//clears out the server struct before placing data into it

    serv_addr.sin_family = AF_INET;//specifies the address of the server will be IPv4

    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);// copies the IP address from the hostent server struct to the serv_addr struct
    serv_addr.sin_port = htons(portno);//specifies port number

    connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
    
    
    //citation https://pubs.opengroup.org/onlinepubs/009695399/functions/connect.html
    // attempts to connect sockfd, the client socket, with serv_addr the proposed server socket.

  
    bzero(buffer,1024);// clears out buffer
    read(sockfd,buffer,1024);//reads bytes from server from socket

    
    if((int)buffer[0] > 1){
        struct server_message server_info;
        memcpy(&server_info, buffer, (int)buffer[0]);
        char message_from_server[(int)buffer[0]+1];
        memcpy(message_from_server, server_info.data,(int)buffer[0]);
        message_from_server[(int)buffer[0]] = '\0';
        printf(">>>%s\n",message_from_server);//prints out the buffer
        close(sockfd);
        return;

      
    }

       
    char inString[128];//where input will be read from command line
    struct client_message game_start;
    printf(">>>Ready to start game? (y/n): ");
    fgets(inString,128,stdin);
    int flag= 0;

               
    if( inString[0] == 'y'){
        game_start.msg_length = 0;
           
    }else{
        close(sockfd);
        return 0;

    }
    
    memcpy(buffer, &game_start, sizeof(struct client_message));
    write(sockfd, buffer, sizeof(struct client_message));
    
    
    while(1){
        bzero(inString,128);
        bzero(buffer,1024);// clears out buffer
        read(sockfd,buffer,1024);//reads bytes from server from socket
        struct server_message server_info;
        struct server_game_data server_game_info;
        struct client_message client_info;


        if((int)buffer[0]== 0){
            
           
            //receiving game data
            //word length, num incorrect, and data array
            memcpy(&server_game_info, buffer, sizeof(struct server_game_data));
           
            char output[(int)server_game_info.word_length +1];
            memcpy(output, server_game_info.data,(int)server_game_info.word_length);
            output[(int)server_game_info.word_length] = '\0';

            char guessList[ (int)server_game_info.num_incorrect+1];
            memcpy(guessList,server_game_info.data +(int)server_game_info.word_length, (int)server_game_info.num_incorrect);
            guessList[(int)server_game_info.num_incorrect] = '\0';

            printf(">>>");
            for(int m = 0; m < (int)server_game_info.word_length;m++){
                if(m == (int)server_game_info.word_length-1){
                    printf("%c", output[m]);
                }else{
                    printf("%c ", output[m]);
                }
            }
            printf("\n");

            printf(">>>Incorrect Guesses: ");
            for(int m = 0; m <(int)server_game_info.num_incorrect ;m++){
                if(m ==(int)server_game_info.num_incorrect-1){
                    printf("%c", guessList[m]);
                }else{
                    printf("%c ", guessList[m]);
                }
            }
            printf("\n");
            printf(">>>\n");

            printf(">>>Letter to guess: ");
            void * val = fgets(inString, 128,stdin);

            if(val == NULL){
                printf("\n");
                close(sockfd);
                
                return;
            }
            
            while(strlen(inString) > 2 || ( ((int)inString[0] < 65 || (int)inString[0] > 90) && ((int)inString[0] < 97 || (int)inString[0] > 122)) ){
                
                bzero(inString,128);
                printf(">>>Error! Please guess one letter.\n");
                printf(">>>Letter to guess: ");
                void * val = fgets( inString, 128,stdin);

                
                if(val == NULL){
                    printf("\n");
                    close(sockfd);
                    return 0;
                }
                
            }

            if((int)inString[0] >= 65 && (int)inString[0] <=90){
                inString[0] += 32;
            }

            client_info.msg_length = 1;
            

        }else{
            
            //messages
            memcpy(&server_info, buffer, (int)buffer[0]);
            
            char message_from_server[(int)buffer[0]+1];
            memcpy(message_from_server, server_info.data,(int)buffer[0]);
            message_from_server[(int)buffer[0]] = '\0';
            
            if((int)server_info.msg_flag == 60 ){//only used for win loss conditions
                printf(">>>%s\n",message_from_server);//prints out the buffer
                break;
            }
            
            printf(">>>%s",message_from_server);//prints out the buffer

            scanf("%s", inString);

            if((int)inString[0] == 0){
               
                break;
            }

            
            
            
        }

        //client_info.msg_length = strlen(inString);
        
        memcpy(client_info.data, inString, strlen(inString));
        memcpy(buffer, &client_info, sizeof(struct client_message));

        write(sockfd, buffer, sizeof(struct client_message));

        
        
    }
    
    close(sockfd);

    return 0;
}