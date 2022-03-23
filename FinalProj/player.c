//player.c
//written by Riley Freels
//for CS375 UNIX system programming

#include <sys/socket.h> //included for sockets
#include <stdio.h>      //included for perror, printf, ...
#include <netinet/in.h>
#include <arpa/inet.h>  //inet_addr()
#include <unistd.h>     //included for read, write, close
#include <stdlib.h>     //included for exit, ...
#include <string.h>     //included for strncmp

int main(int argc, char *argv[])
{
  int sockfd;                  //socket id
  int len;                     //store size of socket address
  struct sockaddr_in address;  //socket address structure
  int result;                  //stores result of connection: 0=success: <0 fail
  //next two vars store port number entered by user and convert to integer
  long temp  = strtol(argv[1], NULL, 10);
  int port = temp;
  
  //check that a port argument was entered
  if(argc != 2)
    {
      printf("error: enter port number\n");
      exit(1);
    }

  int rps = 1;              //rock paper scissors variable
  char status[5];           //status variable (for reading TURN and GO)
  char player;              //player number
  char winner;              //winner variable
  char choices[2];          //stores player1 and two choices
  char round;               //stores round number
  char wins[2] = {0, 0};    //stores player wins

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr("127.0.0.1");
  address.sin_port = htons(port);
  len = sizeof(address);

  result = connect(sockfd, (struct sockaddr *)&address, len);

  //error connecting to server socket
  if(result == -1){
    perror("error player");
    exit(1);
  }

  write(sockfd, "CONN", 4); //write connection status to server
  read(sockfd, &status, 4); //read turn
  read(sockfd, &player, 1); //get player number
  printf("You are player %d.\n", player);
  while(!(strncmp(status, "STOP", 4) == 0))
    {

      if((strncmp(status, "TURN", 4) == 0))
	{
	  write(sockfd, "READY", 5); //send READY status
	  memset(status, 0, 5);      //clear status variable
	  read(sockfd, &status, 2);  //get GO status
	  
	  if(strncmp(status, "GO", 2) == 0)
	    {
	      //display choices and prompt user
	      printf("0. Exit\n1. Rock\n2. Paper\n3.Scissors\n");
	      printf("Enter Choice: ");
	      scanf(" %d", &rps);
	      //check for correct input: loop until correct input recv'd
	      while(rps != 0 && rps != 1 && rps != 2 && rps != 3)
		{
		  printf("ERROR: incorrect input\n");
		  printf("Enter Choice: ");
		  scanf(" %d", &rps);
		}
	      //if rps == 0: send the STOP signal
	      if(rps == 0)
		{
		  write(sockfd, "STOP", 4);
		  read(sockfd, &status, 4);
		}
	      else
		{
		  //send read status
		  write(sockfd, "READ", 4);
		  //send choice to referee
		  write(sockfd, &rps, 1);
		  printf("waiting on other player input\n");

		  //clear status variable
		  memset(status, 0, 5);
		  //read turn
		  read(sockfd, &status, 4);
		}
		  if(!(strncmp(status, "STOP", 4) == 0))
		    {
		      //read results
		      read(sockfd, &winner, 1);
		      read(sockfd, &choices, 2);
		      read(sockfd, &round, 1);

		      //display round number
		      printf("Round %d:\n", round+1);

		      //display player choices
		      if(choices[0] == 1)
			printf("Player 1 chose: Rock.\n");
		      else if(choices[0] == 2)
			printf("Player 1 chose: Paper.\n");
		      else if(choices[0] == 3)
			printf("Player 1 chose: Scissors.\n");
		      
		      if(choices[1] == 1)
			printf("Player 2 chose: Rock.\n");
		      else if(choices[1] == 2)
			printf("Player 2 chose: Paper.\n");
		      else if(choices[1] == 3)
			printf("Player 2 chose: Scissors.\n");

		      //display winner
		      if(winner == 0)
			printf("DRAW\n");
		      else
			printf("Player %d wins.\n", winner);

		      //clear status variable
		      memset(status, 0, 5);
		      //read turn
		      read(sockfd, &status, 4);

		      //This if statement is for the player
		      //that is waiting on the other player.
		      //if status == STOP the round is over: display game stats
		      if((strncmp(status, "STOP", 4) == 0))
			{
			  read(sockfd, &round, 1);
			  read(sockfd, &wins, 2);
			  printf("\nGAME OVER\n");
			  printf("Rounds Played: %d\n", round);
			  printf("Player 1 score: %d\n", wins[0]);
			  printf("Player 2 score: %d\n", wins[1]);
			}
		    }
		  else
		    {
		      //This is for the player that made the choice. 
		      //status == STOP: the round is over: display game stats
		      read(sockfd, &round, 1);
		      read(sockfd, &wins, 2);
		      printf("\nGAME OVER\n");
		      printf("Rounds Played: %d\n", round);
		      printf("Player 1 score: %d\n", wins[0]);
		      printf("Player 2 score: %d\n", wins[1]);
		    }	
	    }
	}
      
    }
  //close the socket and exit program
  close(sockfd);
  exit(0);
}
