//referee.c
//written by Riley Freels
//for CS375 UNIX system programming

#include <sys/socket.h> //included for sockets
#include <stdio.h>      //included for perror, printf, ...
#include <netinet/in.h> //included for sockets
#include <sys/ioctl.h>  //included for ioctl
#include <unistd.h>     //included for read, write, close
#include <stdlib.h>     //included for exit, ...
#include <string.h>     //included for strncmp

int winner(int rps1, int rps2);

int main()
{
  int server_sockfd, client_sockfd;
  unsigned int server_len, client_len;
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  int result;
  fd_set readfds, testfds;
  int connected = 0;         //number of clients connected
  int chosen = 0;            //tracks if both players have had their turn
  char win;                  //variable to store and send back winner
  int fd_stored[2];          //fds of sockets
  int turn = 0;              //player1 turn = 0; player2 turn = 1;
  int rounds = 0;            //tracks rounds played
  char wins[2] = {0, 0};     //tracks wins of players

  //create server socket
  server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = htons(9734);
  server_len = sizeof(server_address);

  bind(server_sockfd, (struct sockaddr *)&server_address, server_len);

  //listen for connections: maximum of two
  listen(server_sockfd, 2);

  FD_ZERO(&readfds);
  FD_SET(server_sockfd, &readfds);

  //check for error getting socket name
  if(getsockname(server_sockfd, (struct sockaddr *)&sin, &len) == -1)
    perror("getsockname");
  else
    printf("Referee is using port %d.\n", ntohs(sin.sin_port));

  printf("Referee is waiting for players.\n");

  while(1) {

    char status[5];  //stores status sent from players
    int fd;          //file descriptor of players socket
    char rps[2];     //players choices
    int nread;

    testfds = readfds;

    result = select(FD_SETSIZE, &testfds, (fd_set *) 0,
		    (fd_set *) 0, (struct timeval *) 0);

    //error getting file descriptor
    if(result < 1){
      perror("referee");
      exit(1);
    }
    
    for(fd = 0; fd < FD_SETSIZE; fd++){
      if(FD_ISSET(fd, &testfds)){
	if(fd == server_sockfd){
	  //get size of client address
	  client_len = sizeof(client_address);
	  //accept client connection
	  client_sockfd = accept(server_sockfd,
				 (struct sockaddr *)&client_address,
				 &client_len);
	  //increment number of players connected
	  connected++;
	  FD_SET(client_sockfd, &readfds);
	  fd_stored[connected-1] = client_sockfd;
	  //display player connected
	  printf("Player %d has connected.\n", connected);
	}
	else{
	  //player disconnects
	  ioctl(fd, FIONREAD, &nread);
	  
	  if(nread == 0){
	    close(fd);
	    FD_CLR(fd, &readfds);
            connected--;
	    printf("Player has disconnected.\n");
	  }
	  else
	    {
	      //activity on players socket
	      //connection status
	      if(rounds < 1)
		{
		  //read connection status only on first round
		  read(fd, &status, 4);
		}
	      if(turn == 0)  //player1 turn
		{
		  if(rounds < 1)
		    {
		      write(fd_stored[0], "TURN", 4);
		      //send player number
		      write(fd_stored[0], &connected, 1);
		    }
		  //read ready status
		  read(fd_stored[0], &status, 5);
		}
	      else //player2 turn
		{
		  if(rounds < 1)
		    {
		      write(fd_stored[1], "TURN", 4);
		      //send player number
		      write(fd_stored[1], &connected, 1);
		    }
		  //read ready status
		  read(fd_stored[1], &status, 5);
		}
	      //printf("status recv: %s\n", status);
	      if((strncmp(status, "READY", 5) == 0))
		{
		  //write go status to player
		  write(fd_stored[turn], "GO", 2);

		  //check for stop status
		  read(fd, &status, 4);
		  
		  
		  //check if STOP status received
		  if((strncmp(status, "STOP", 4) == 0))
		    {
		      printf("GAME ENDED\n");
		      //write STOP status to both players
		      write(fd_stored[0], "STOP", 4);
		      write(fd_stored[1], "STOP", 4);
		      
		      //write rounds played
		      write(fd_stored[0], &rounds, 1);
		      write(fd_stored[1], &rounds, 1);
		      //write wins
		      write(fd_stored[0], &wins, 2);
		      write(fd_stored[1], &wins, 2);

		      //reset variables
		      chosen = 0;
		      rounds = 0;
		      turn = 0;
		      rps[0] = 0; rps[1] = 0;
		      wins[0] = 0; wins[1] = 0;
		    }
		  else
		    {
		      if(turn == 0)
			{
			  //read player1 choice
			  read(fd_stored[0], &rps[chosen], 1);
			  printf("Player 1 chose: %d\n", rps[chosen]);
			  //send TURN status to player 2
			  if(connected == 2)
			    write(fd_stored[1], "TURN", 4);
			  //increment turn var
			  turn++;
			  
			}
		      else
			{
			  //read player two choice
			  read(fd_stored[1], &rps[chosen], 1);
			  printf("Player 2 chose: %d\n", rps[chosen]);
			  //decrement turn var
			  turn--;
			  
			}
		      //increment chosen variable
		      chosen++;
		    }
		}
	      else
		{
		  printf("error: READY\n");
		}
	      
	      //check for winner: chosen == 2: both players have had their turn
	      if(chosen == 2)
		{
		  //check for winner
		  win = winner(rps[0], rps[1]);

		  //increment player1 or player2 score
		  if(win == 1)
		    wins[0]++;
		  else if(win == 2)
		    wins[1]++;

		  //write continue status to both players
		  write(fd_stored[0], "CONT", 4);
		  write(fd_stored[1], "CONT", 4);
		  
		  //write winner to both players
		  write(fd_stored[0], &win, 1);
		  write(fd_stored[1], &win, 1);
		  //write player choices to both players
		  write(fd_stored[0], &rps, 2);
		  write(fd_stored[1], &rps, 2);
		  //write round number to players
		  write(fd_stored[0], &rounds, 1);
		  write(fd_stored[1], &rounds, 1);
		  //set player choices to zero
		  rps[0] = 0; rps[1] = 0;
		  //increment round
		  rounds++;
		  //write turn status
		  write(fd_stored[turn], "TURN", 4);
		  //reset chosen varable to 0: no one has had their turn
		  chosen = 0;
		}
	      //reset status variable
	      memset(status, 0, 5);
	    }//end socket code
	  
	}//end socket disconnect
      }//end socket create     
    }//end for loop         
  }//end while loop
}//end main

int winner(int rps1, int rps2)
{
  //calculate winner
  if(rps1 == rps2)
    {
      //tie
      //p1 rock p2 scissors: p1 wins
      printf("DRAW.\n");
      return 0;
    }
  else if(rps1 == 1 && rps2 == 3)
    {
      //p1 rock p2 scissors: p1 wins
      printf("Player 1 wins.\n");
      return 1;
    }
  else if(rps1 == 1 && rps2 == 2)
    {
      //p1 rock p2 paper: p2 wins
      printf("Player 2 wins.\n");
      return 2;
    }
  else if(rps1 == 2 && rps2 == 1)
    {
      //p1 paper p2 rock: p1 wins
      printf("Player 1 wins.\n");
      return 1;
    }
  else if(rps1 == 2 && rps2 == 3)
    {
      //p1 paper p2 scissors: p2 wins
      printf("Player 2 wins.\n");
      return 2;
    }
  else if(rps1 == 3 && rps2 == sizeof(int))
    {
      //p1 scissors p2 rock: p2 wins
      printf("Player 2 wins.\n");
      return 2;
    }
  else if(rps1 == 3 && rps2 == 2)
    {
      //p1 scissors p2 paper: p1 wins
      printf("Player 1 wins.\n");
      return 1;
    }
  return 0;
}

