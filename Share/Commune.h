#pragma once
#ifndef COMMUNE_H
#define COMMUNE_H

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")

#include <stdio.h>
#include "HardCodedData.h"

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
int Recv_Socket(SOCKET sd, char* OutputBuffer, int max_wait_time);

#endif // ! COMMUNE_H