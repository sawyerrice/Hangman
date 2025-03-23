#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <poll.h>
#include <time.h>

/* All of the basic socket programming syntax was derived from the linuxhowtos.org 
link at the end of the socket programming video posted on Canvas.
*/

//https://www.linuxhowtos.org/data/6/server.c


struct server_message{

    char msg_flag;
    char data[60];

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

struct game_data{

    int num_guesses;
    int word_length;
    int num_incorrect;
    char guesses[26];
    char * word_to_guess;
    char * word_state;
};




int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[1024];
    struct sockaddr_in serv_addr, cli_addr;
    struct pollfd fds[5];
    struct game_data * game_arr[3];


    int openSpace = 3;

    for(int i =0; i < 3; i++){
        game_arr[i] = NULL;
    }

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);// declares the socket, specifies stream for TCP. Also specifies IPv4

    bzero((char *) &serv_addr, sizeof(serv_addr));//clears out serv_addr struct 

    portno = atoi(argv[1]);//gets port number to listen on from command line

    serv_addr.sin_family = AF_INET;//specifies that the server will use IPv4 addresses
    serv_addr.sin_addr.s_addr = INADDR_ANY;//allows this socket to listen on all possible IP addresses for my machine

    serv_addr.sin_port = htons(portno);//assigns port number to server struct

    bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));// binds the server struct to 'sockfd' file descriptor
   //citation https://pubs.opengroup.org/onlinepubs/009695399/functions/bind.html
             
    listen(sockfd,4); 
    //https://pubs.opengroup.org/onlinepubs/009695099/functions/listen.html
    //marks sockfd as open to connections
    //this specifies that 3 connections can be waiting in queue for this server at a time

    fds[0].fd = sockfd;//fd[0] is used for listening for new connections
    fds[0].events = POLLIN;

    for (int i = 1; i <= 4; i++) {
        fds[i].fd = -1;
    }

    clilen = sizeof(cli_addr);

    while(1){

        // https://github.com/weboutin/simple-socket-server/blob/master/poll-server.c
        //I took some inspiration from this example of a server using polling online
        // my code is not very similar, but some of the basic structuring and formatting was learned from this example


        bzero(buffer,1024);// clears out buffer
        poll(fds, 5, -1); // Wait indefinitely for activity
        //https://man7.org/linux/man-pages/man2/poll.2.html
        //citation for poll
        
        if(fds[0].revents & POLLIN ) {//checks for activity on socket creating new connections

            printf("%d\n", openSpace);
            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

            if(openSpace == 0) {
                printf("No more space\n");
                struct server_message noSpace;
                char error[30] = "server-overloaded";
                memcpy(noSpace.data, error, 30);
                noSpace.msg_flag = 30;
                memcpy(buffer, &noSpace, sizeof(struct server_message));
                write(newsockfd, buffer, sizeof(struct server_message));
                close(newsockfd);
                continue;
            }else{
                printf("Space!\n");
                struct server_message accepted;
                bzero(&accepted, sizeof(struct server_message));
                accepted.msg_flag = 1;
                memcpy(buffer, &accepted, sizeof(struct server_message));
                write(newsockfd, buffer, sizeof(struct server_message));

            }
            


            for (int i = 1; i <= 3; i++) {
                if (fds[i].fd == -1) {
                    //chooses lowest empty slot to add a new connection
                    fds[i].fd = newsockfd;
                    fds[i].events = POLLIN;
                   
                    openSpace--;
                    break;
                }
            }
        }
      

        for (int i = 1; i <= 3; i++) {
            bzero(buffer,1024);// clears out buffer
            if (fds[i].fd > 0 && (fds[i].revents & POLLIN)) {
                
                if (read(fds[i].fd, buffer, 1024) == 0) {
                    // Client disconnected
                    
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    free(game_arr[i-1]->word_state);
                    free(game_arr[i-1]);
                    game_arr[i-1] = NULL;
                    openSpace++;
                } else {
                    int send_message = 0;
                    // Handle client request
                    struct client_message client_info;
                    
                    memcpy(&client_info, buffer, sizeof(struct client_message));//reads all bytes out of the buffer in a struct

                    char * word_spaces = malloc(9*sizeof(char));
                    if(client_info.msg_length == 0){
                        //signalling to start game
                        //pick word and send first control packet
                        char* words[15];
                        for(int i = 0 ; i < 15; i++){
                            words[i] = malloc(12);
                        }

                        FILE *file = fopen("hangman_words.txt", "r");
                        int count = 0;
                        char arr[12];
                        while ((fgets(arr, 12, file)!= NULL) && count < 15) {
                            
                            
                           
                            

                            //int idx = strcspn(arr, "\n");
                            
                            for(int c = 0;  c < 9; c++){
                                
                                if(arr[c] == '\n' || arr[c] == '\0' || (int)arr[c] == 13){
        
                                    arr[c] = '\0';
                                    break;
                                }
                                if(c == 8){
                                    
                                    arr[8] = '\0';
                                }
                            }

                            
                            memcpy(words[count], arr, strlen(arr)+1);

                            bzero(arr, 12);
                            count++;
                        }
                        
                        fclose(file);

                       
                        

                        int random_num = rand() % count;
                        
                        struct game_data* new_game = malloc(sizeof(struct game_data));
                        new_game->num_guesses = 0;
                        new_game->num_incorrect = 0;

                        new_game->word_to_guess = words[random_num];
                        
                        for(int m = 0; m < 26; m++){
                            new_game->guesses[m] = 0;
                        }

                        int length = strlen(new_game->word_to_guess);
                        new_game->word_length = length;
                       
                        

                        
                        for(int k = 0; k < length; k++){
                            word_spaces[k] = '_';
                        }
                        
                        new_game->word_state = word_spaces;
                        game_arr[i-1] = new_game;
                        
                    }else if(client_info.msg_length == 1){
                        
                        struct game_data* curr_game = game_arr[i-1];
                        char user_guess = *client_info.data;
                         
                        int correct_guess = 0;
                        int all_correct = 1;
                        for(int i=0; i < curr_game->word_length; i++){

                            
                            if(curr_game->word_to_guess[i] == user_guess){
                                correct_guess = 1;
                                curr_game->word_state[i] = user_guess;
                            }

                            if(curr_game->word_state[i] == '_'){
                                all_correct = 0;
                            }
                        }
                        if(!correct_guess){
                            curr_game->guesses[curr_game->num_incorrect] = user_guess;
                            curr_game->num_incorrect++;
                        }
                        if(all_correct){
                            send_message = 1;
                        }
                        if(curr_game->num_incorrect == 6){
                            send_message = 2;
                        }
                        
                    }

                    if(send_message >= 1){
                        struct server_message server_resp;
                        
                        char win[60]; 
                        sprintf(win, "The word was %s\n>>>You Win!\n>>>Game Over!", game_arr[i-1]->word_to_guess);
                        char lose[60];
                        sprintf(lose, "The word was %s\n>>>You Lose!\n>>>Game Over!", game_arr[i-1]->word_to_guess);
                        
                        if(send_message == 1){
                           memcpy(server_resp.data, win,60);
                           server_resp.msg_flag = 60;
                        }else if(send_message == 2){
                            memcpy(server_resp.data, lose, 60);
                            server_resp.msg_flag = 60;
                        }

                        memcpy(buffer, &server_resp, sizeof(struct server_game_data));
                        write(fds[i].fd, buffer, 1024);

                    }else{

                        struct server_game_data server_resp;

                        server_resp.msg_flag = 0;
                    
                        
                        server_resp.num_incorrect = game_arr[i-1]->num_incorrect;
                        server_resp.word_length = game_arr[i-1]->word_length;

                        memcpy(server_resp.data, game_arr[i-1]->word_state, game_arr[i-1]->word_length);
                        memcpy(server_resp.data +game_arr[i-1]->word_length, game_arr[i-1]->guesses, game_arr[i-1]->num_incorrect);
                        
                        memcpy(buffer, &server_resp, sizeof(struct server_game_data));
                        
                        write(fds[i].fd, buffer, 1024);
                    }
                }
            }
        }
        
        
        
    }
    
   

    return 0; 
}

