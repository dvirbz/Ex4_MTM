#pragma once
#ifndef COMMUNE_H
#define COMMUNE_H

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")

#include <stdio.h>

#define MAX_USERNAME_LEN 21
#define MAX_LINE_LEN 32
#define NUM_DIGITIS_GUESS 5
#define DECIMAL_BASE 10
#define ERROR_CODE -1
#define TEN_MINUTES 600000
#define FIFTEEN_SEC 15000
#define THIRTY_SEC 30000
#define FILE_GAME_SESSION "GameSession.txt"

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

#define CLIENT_REQUEST "CLIENT_REQUEST"
#define CLIENT_VERSUS "CLIENT_VERSUS"
#define CLIENT_DISCONNECT "CLIENT_DISCONNECT"
#define CLIENT_SETUP "CLIENT_SETUP"
#define CLIENT_PLAYER_MOVE "CLIENT_PLAYER_MOVE"

#define MAX_PRO_LEN 100 //should be updated to the true size
#define MAX_NAME_LEN 20
#define END_PROTOCOL "\n"
#define PARTITION_MASSAGE_PARAMETERS ":"
#define SHUTDOWN -2
#define NUM_OF_THREADS 2

typedef struct
{
	int bulls, cows;
	char first_string[MAX_USERNAME_LEN];
	char second_string[NUM_DIGITIS_GUESS];
}BnC_Data;

typedef enum
{
	SERVER_DENIED_ID,
	SERVER_APPROVED_ID,
	SERVER_MAIN_MENU_ID,
	SERVER_INVITE_ID,
	SERVER_NO_OPPONENTS_ID,
	SERVER_SETUP_REQUEST_ID,
	SERVER_PLAYER_MOVE_REQUEST_ID,
	SERVER_GAME_RESULTS_ID,
	SERVER_WIN_ID,
	SERVER_DRAW_ID,
	SERVER_OPPONENT_QUIT_ID,
	NUMBER_OF_SERVER_MES
}Server_Messages;

typedef enum
{
	CLIENT_REQUEST_ID,
	CLIENT_VERSUS_ID,
	CLIENT_DISCONNECT_ID,
	CLIENT_SETUP_ID,
	CLIENT_PLAYER_MOVE_ID,
	NUMBER_OF_CLIENT_MES
}Client_Messages;

/*Server Protocols*/
int GET__Server_Invite_Pro(char* protocol, char* username);
int GET__Server_Game_Results_Pro(char* protocol, int bulls, int cows,
	char* other_player_user, char* other_player_move);
int GET__Server_Won_Pro(char* protocol, char* wining_player_username, char* other_player_setup);
int GET__Server_Pro(char* protocol, const char* message_type);
/*Client Protocols*/
int GET__CLIENT_REQUEST_PRO(char* protocol, char* username);
int GET__CLIENT_VERSUS_PRO(char* protocol);
int GET__CLIENT_DISCONNECT_PRO(char* protocol);
int GET__CLIENT_SETUP_PRO(char* protocol, char* setup_seq);
int GET__CLIENT_PLAYER_MOVE_PRO(char* protocol, char* guess_seq);

/*Compare*/
int GET__Server_Response_ID(char* protocol);
int GET__Client_Response_ID(char* protocol);
char* GET__Message_Type(char* protocol);
BnC_Data* GET__BnC_Data(char* protocol);

/*Send and recive on socket*/
int Send_Socket(SOCKET s, const char* buffer, int len, int max_wait_time);
int ReceiveBuffer(SOCKET sd, char* OutputBuffer, int BytesToReceive, int max_wait_time);
int SendBuffer(SOCKET sd, const char* Buffer, int BytesToSend, int max_wait_time);
int Recv_Socket(SOCKET s, char* buffer, int max_wait_time);

#endif // ! COMMUNE_H