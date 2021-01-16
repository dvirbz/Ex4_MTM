#pragma once

//Server protocols
#define SERVER_DENIED "SERVER_DENIED"
#define SERVER_APPROVED "SERVER_APPROVED"
#define SERVER_MAIN_MENU "SERVER_MAIN_MENU"
#define SERVER_INVITE "SERVER_INVITE"
#define SERVER_NO_OPPONENTS "SERVER_NO_OPPONENTS"
#define SERVER_SETUP_REQUEST "SERVER_SETUP_REQUEST"
#define SERVER_PLAYER_MOVE_REQUEST "SERVER_PLAYER_MOVE_REQUEST"
#define SERVER_GAME_RESULTS "SERVER_GAME_RESULTS"
#define SERVER_WIN "SERVER_WIN"
#define SERVER_DRAW "SERVER_DRAW"
#define SERVER_OPPONENT_QUIT "SERVER_OPPONENT_QUIT" 

//Client protocols
#define CLIENT_REQUEST "CLIENT_REQUEST"
#define CLIENT_VERSUS "CLIENT_VERSUS"
#define CLIENT_DISCONNECT "CLIENT_DISCONNECT"
#define CLIENT_SETUP "CLIENT_SETUP"
#define CLIENT_PLAYER_MOVE "CLIENT_PLAYER_MOVE"

//regular numbers
#define MAX_USERNAME_LEN 21
#define MAX_LINE_LEN 32
#define NUM_DIGITIS_GUESS 5
#define DECIMAL_BASE 10
#define MAX_PRO_LEN 100 //should be updated to the true size
#define MAX_NAME_LEN 20
#define END_PROTOCOL "\n"
#define PARTITION_MASSAGE_PARAMETERS ":"
#define SHUTDOWN -2
#define EXIT_CODE -1
#define NUM_OF_THREADS 2
#define ARGUMENT_NUMBER_SERVER 2
#define PORT_NUMBER_INDEX 1
#define LOCAL_HOST_ADDRESS "127.0.0.1"
#define FILE_GAME_SESSION "GameSession.txt"
#define ERROR_CODE -1
#define BULLS_AND_COWS_STR_LEN 2
#define TEN_MINUTES 600000
#define FIFTEEN_SEC 15000
#define THIRTY_SEC 30000
#define BULLS_AND_COWS_STR_LEN 2

