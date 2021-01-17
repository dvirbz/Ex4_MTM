#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Commune.h"
#include <time.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")

#define ArgumetnNumber 4
#define MAX_IP_LEN 15
#define IP_ARG_NUM 1
#define PORT_ARG_NUM 2
#define USER_ARG_NUM 3

typedef enum 
{
	CONNECT,
	EXIT,
	PLAY,
	MENU
}Player_Status;
typedef enum 
{
	SETUP,
	GUESS,
	END
}Game_Status;

int InitializeWSA();
int Connect_Socket(SOCKET s, SOCKADDR* cs);

/* Player Related Func*/
int GET__Connect_Player_Decision();
int GET__Play_Player_Decision();
void Choose_Next_Connect();
void Choose_Next_Play();
void Connect_MENU(char server_ip[], int server_port);
void Denied_MENU(char server_ip[], int server_port);

int Game_Guess(Game_Status guess_status, SOCKET s_client,
	char* server_response, char* guess_seq, char* client_message);
int Game_Setup(Game_Status guess_status, SOCKET s_client, char* server_response,
	char* guess_seq, char* client_message);
int Game(Player_Status play_status, Game_Status guess_status, SOCKET s_client, char* server_response,
	char* guess_seq, char* client_message);
int Connect(SOCKET s_client, SOCKADDR_IN clientService, char* server_response, int server_port_number,
	char* server_ip_address, char* first_string, char* guess_seq,
	char* client_message, Player_Status connect_status);

/* Server Related Func*/
int GET__Server_Response(SOCKET s_client, char* server_response, int max_wait_time);
int Handle_Server_Main_Menu(SOCKET s_client, char* server_response,
	Player_Status play_status, char* client_message);

/* Client Related Func*/
int Handle_Client_Request(SOCKET s_client, char* username, char* client_request);
int Handle_Client_Versus(SOCKET s_client, char* client_versus);
int Handle_Client_Disconnect(SOCKET s_client, char* client_disconnect);
int Handle_Client_Setup(SOCKET s_client, char* client_message, char* setup_seq);
int Handle_Client_Player_Move(SOCKET s_client, char* client_message, char* guess_seq);

int main(int argc, char* argv[])
{
	int exit_code = 0;
	assert(argc == ArgumetnNumber);
	/* Init Connection Params */
	char server_ip_address[MAX_IP_LEN], username[MAX_USERNAME_LEN];
	int server_port_number;

	assert(snprintf(server_ip_address, MAX_IP_LEN, "%s", argv[IP_ARG_NUM]) != 0);
	assert(snprintf(username, MAX_USERNAME_LEN, "%s", argv[USER_ARG_NUM]) != 0);
	server_port_number = (int)strtol(argv[PORT_ARG_NUM], NULL, DECIMAL_BASE);
	assert(!(server_port_number == 0 && strcmp(argv[PORT_ARG_NUM], "0")));
	/*===============================================================================*/

	/* Init WSA and Socket Params */
	assert(InitializeWSA() == 0);

	SOCKET s_client;
	s_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	assert(s_client != INVALID_SOCKET);

	SOCKADDR_IN clientService;
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(server_ip_address);
	clientService.sin_port = htons(server_port_number);
	/*===============================================================================*/
	
	/* Init Commune Params */	
	Player_Status connect_status = CONNECT, play_status = PLAY;
	Game_Status guess_status = END;
	char* client_message = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	if (client_message == NULL)
	{
		exit_code = -1;
		goto Exit_WO_FREE;
	}
	char* server_response = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	if (server_response == NULL)
	{
		free(client_message);
		exit_code = -1;
		goto Exit_WO_FREE;
	}
	char* guess_seq = (char*)calloc(NUM_DIGITIS_GUESS, sizeof(char));
	if (guess_seq == NULL)
	{
		free(server_response);
		exit_code = -1;
		goto Exit_WO_FREE;
	}
	/*===============================================================================*/

	/* Connecting */
	exit_code = Connect(s_client, clientService, server_response, server_port_number, server_ip_address,
		username, guess_seq, client_message, connect_status);
	if (exit_code == -1 || exit_code == SHUTDOWN)
	{	
		goto ExitSeq;
	}	
	connect_status = exit_code;
	exit_code = 0;
	/*===============================================================================*/

	play_status = connect_status;
	/* SERVER_MAIN_MENU*/
	exit_code = Handle_Server_Main_Menu(s_client, server_response, play_status, client_message);
	if (exit_code == -1 || exit_code == SHUTDOWN)
	{		
		goto ExitSeq;
	}
	/*===============================================================================*/

	/* Let's Play */
	exit_code = Game(play_status, guess_status, s_client, server_response, guess_seq, client_message);
	if (exit_code == -1 || exit_code == SHUTDOWN)
	{
		goto ExitSeq;
	}
	/*===============================================================================*/

ExitSeq:
	Handle_Client_Disconnect(s_client, client_message);
	/* wait until server support this gracefull disconnect too*/
	shutdown(s_client, SD_SEND);
	while (exit_code != SHUTDOWN)
	{
		exit_code = Recv_Socket(s_client, server_response, FIFTEEN_SEC);
	}
	free(guess_seq);
	free(client_message);
	free(server_response);
Exit_WO_FREE:
	if (closesocket(s_client) == SOCKET_ERROR)
	{
		printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
	}
	if (WSACleanup() == SOCKET_ERROR)
	{
		printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
	}
	return exit_code;
}

int InitializeWSA()
{
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
	{
		printf("Error at WSAStartup()\n");
		return -1;
	}
	return 0;
}
int Connect_Socket(SOCKET s, SOCKADDR* cs)
{
	if (connect(s, cs, sizeof(*cs)) == SOCKET_ERROR)
	{
		printf("%d\n", WSAGetLastError());
		return -1;
	}
	return 0;
}

void Connect_MENU(char server_ip[],int server_port) {
	printf("Failed connecting to server on %s:%d\n", server_ip, server_port);
	Choose_Next_Connect();
}
void Denied_MENU(char server_ip[], int server_port) {
	printf("Server on %s:%d denied the connection request.\n", server_ip, server_port);
	Choose_Next_Connect();
}
void Choose_Next_Connect()
{
	printf("Choose what to do next:\n");
	printf("1. Try to reconnect\n");
	printf("2. Exit\n");
}
void Choose_Next_Play()
{
	printf("Choose what to do next:\n");
	printf("1. Play against another client\n");
	printf("2. Quit\n");
}
int GET__Connect_Player_Decision()
{
	int connect_status = 0;
	if (scanf_s("%d", &connect_status) == 0)
	{
		return -1;
	}
	fflush(stdin);
	switch (connect_status)
	{
	case 1:return CONNECT;
		break;
	case 2:return EXIT;
		break;
	}
	return -1;
}
int GET__Play_Player_Decision()
{
	int play_status = 0;
	if (scanf_s("%d", &play_status) == 0)
	{
		return -1;
	}
	fflush(stdin);
	switch (play_status)
	{
	case 1:return PLAY;
		break;
	case 2:return EXIT;
		break;
	}
	return -1;
}

int Game_Guess(Game_Status guess_status,SOCKET s_client, char* server_response,
	char* guess_seq, char* client_message)
{
	while (guess_status == GUESS)
	{
		int server_res_id = GET__Server_Response(s_client, server_response, TEN_MINUTES);
		if (server_res_id == SERVER_PLAYER_MOVE_REQUEST_ID)
		{
			printf("Choose your guess:\n");
			if (scanf_s("%s", guess_seq, NUM_DIGITIS_GUESS) == 0)
			{
				printf("oof");
				return -1;
				
			}
			fflush(stdin);
			printf("guess: %s\n", guess_seq);
			if (Handle_Client_Player_Move(s_client, client_message, guess_seq) == -1)
			{
				return -1;
			}
		}
		else
		{
			if (server_res_id == SHUTDOWN)
			{
				return SHUTDOWN;
			}
			printf("SERVER_PLAYER_MOVE_REQUEST_ID pro: %s server_res_id: %d\n",server_response, server_res_id);
			return -1;
		}
		server_res_id = GET__Server_Response(s_client, server_response, TEN_MINUTES);
		BnC_Data* data = GET__BnC_Data(server_response);
		switch (server_res_id)
		{
		case SERVER_GAME_RESULTS_ID:
			printf("Bulls: %d\nCows: %d\n%s played: %s\n",
				data->bulls, data->cows, data->first_string, data->second_string);
			free(data);			
			break;
		case SERVER_WIN_ID:
			printf("%s won!\nOpponent's number was %s\n",
				data->first_string, data->second_string);
			free(data);
			return END;
			break;
		case SERVER_DRAW_ID:
			printf("It's a tie\n");
			free(data);
			return END;
			break;
		case SERVER_OPPONENT_QUIT_ID:
			printf("Opponent quit.\n");
			free(data);
			return END;
			break;
		case SHUTDOWN:
			free(data);
			return SHUTDOWN;
		default:printf("SERVER_GAME_RESULTS_ID pro: %s id: %d\n", server_response, server_res_id);
			free(data); 
			return -1;
			break;
		}
	}
	return guess_status;
}
int Game_Setup(Game_Status guess_status, SOCKET s_client, char* server_response,
	char* guess_seq, char* client_message)
{
	int server_res_id = GET__Server_Response(s_client, server_response, TEN_MINUTES);
	if (server_res_id == SERVER_SETUP_REQUEST_ID)
	{
		printf("Choose your 4 digits:\n");
		if (scanf_s("%s", guess_seq, NUM_DIGITIS_GUESS) == 0)
		{
			printf("oof");
			return -1;
		}
		fflush(stdin);
		printf("setup: %s\n", guess_seq);
		if (Handle_Client_Setup(s_client, client_message, guess_seq) == -1)
		{
			return -1;
		}
	}
	else
	{
		if (server_res_id == SHUTDOWN)
		{
			return SHUTDOWN;
		}
		printf("SERVER_SETUP_REQUEST_ID pro: %s server_res_id: %d\n",server_response, server_res_id);
		return -1;
	}
	return 0;
}
int Game(Player_Status play_status, Game_Status guess_status, SOCKET s_client, char* server_response,
	char* guess_seq, char* client_message)
{
	while (play_status == PLAY)
	{
		int server_res_id = GET__Server_Response(s_client, server_response, THIRTY_SEC);
		switch (server_res_id)
		{
		case SERVER_INVITE_ID:
			printf("Game is on!\n");
			guess_status = SETUP;
			play_status = PLAY;
			break;
		case SERVER_NO_OPPONENTS_ID:
			play_status = MENU;
			break;
		case SHUTDOWN:
			printf("SERVER_NO_OPPONENTS_ID SERVER_INVITE_ID pro: %s id: %d\n", server_response, server_res_id);
			return SHUTDOWN;
		default:printf("SERVER_NO_OPPONENTS_ID SERVER_INVITE_ID pro: %s id: %d\n", server_response, server_res_id);
			return -1;
			break;
		}
		if (play_status != MENU)
		{
			if (guess_status == SETUP)
			{
				int retval = Game_Setup(guess_status, s_client,
					server_response, guess_seq, client_message);
				if (retval == -1)
				{
					return -1;
				}
				if (retval == SHUTDOWN)
				{
					return SHUTDOWN;
				}
				guess_status = GUESS;
			}

			guess_status = Game_Guess(guess_status, s_client, server_response,
				guess_seq, client_message);
			if (guess_status == -1)
				return -1;
			if (guess_status == SHUTDOWN)
			{
				return SHUTDOWN;
			}
			if (guess_status == END)
			{
				if (Handle_Server_Main_Menu(s_client, server_response, play_status, client_message) == -1)
				{
					return -1;
				}
				play_status = PLAY;
			}

		}
		else
		{
			if (Handle_Server_Main_Menu(s_client, server_response, play_status, client_message) == -1)
			{
				return -1;
			}
			play_status = PLAY;
		}		
	}
	return 0;
}
int Connect(SOCKET s_client, SOCKADDR_IN clientService, char* server_response,int server_port_number,
	char * server_ip_address, char *username, char* guess_seq,
	char* client_message, Player_Status connect_status )
{
	while (connect_status == CONNECT)
	{
		if (Connect_Socket(s_client, (SOCKADDR*)&clientService) != 0)
		{
			Connect_MENU(server_ip_address, server_port_number);
			connect_status = GET__Connect_Player_Decision();
			if (connect_status != CONNECT)
			{
				return -1;
			}
			else
			{
				continue;
			}
		}
		printf("Connected to server on %s:%d\n", server_ip_address, server_port_number);
		/*===============================================================================*/
			/* Client Request */
		int retval_client_request = Handle_Client_Request(s_client, username, client_message);
		if (retval_client_request == -1 || retval_client_request == SHUTDOWN)
		{
			printf("retval_client_request: %d\n", retval_client_request);
			return retval_client_request;
		}
		
		/* Get Server Response */
		int server_res_id = GET__Server_Response(s_client, server_response, FIFTEEN_SEC);
		switch (server_res_id)
		{
		case SERVER_DENIED_ID:
			Denied_MENU(server_ip_address, server_port_number);
			connect_status = GET__Connect_Player_Decision();
			if (connect_status != CONNECT)
			{			
				return -1;
			}
			/*shutdown(s_client, SD_SEND);
			while (Recv_Socket(s_client, server_response, FIFTEEN_SEC) != SHUTDOWN);*/
			if (closesocket(s_client) == SOCKET_ERROR)
			{				
				return -1;
			}
			s_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (s_client == INVALID_SOCKET)
			{				
				return -1;
			}
			break;
		case SERVER_APPROVED_ID:
			return PLAY;
			break;
		case SHUTDOWN:
			printf("SERVER_APPROVED_ID expected pro: %s id: %d\n", server_response, server_res_id);
			return SHUTDOWN;
		default:printf("SERVER_APPROVED_ID pro: %s id: %d\n", server_response, server_res_id);
			return -1;
			break;
		}
	}
	return 0;
}



int Handle_Client_Request(SOCKET s_client, char* username, char * client_message)
{
	int client_mes_size = strlen(CLIENT_REQUEST) + strlen(END_PROTOCOL) + strlen(username) + 1;
	if (GET__CLIENT_REQUEST_PRO(client_message, username) == -1)
	{
		printf("Protocol failed\n");		
		return -1;
	}
	if (Send_Socket(s_client, client_message, client_mes_size, FIFTEEN_SEC) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}
int Handle_Client_Disconnect(SOCKET s_client, char* client_message) {
	int client_mes_size = strlen(CLIENT_DISCONNECT) + strlen(END_PROTOCOL);
	if (GET__CLIENT_DISCONNECT_PRO(client_message) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_client, client_message, client_mes_size, FIFTEEN_SEC) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}
int Handle_Client_Versus(SOCKET s_client, char* client_message) {
	int client_mes_size = strlen(CLIENT_VERSUS) + strlen(END_PROTOCOL);
	if (GET__CLIENT_VERSUS_PRO(client_message) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_client, client_message, client_mes_size, FIFTEEN_SEC) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}
int Handle_Client_Setup(SOCKET s_client, char* client_message, char* setup_seq) {
	int client_mes_size = strlen(CLIENT_SETUP) + strlen(END_PROTOCOL) + strlen(setup_seq) +1;
	if (GET__CLIENT_SETUP_PRO(client_message, setup_seq) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_client, client_message, client_mes_size, FIFTEEN_SEC) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}
int Handle_Client_Player_Move(SOCKET s_client, char* client_message, char* guess_seq)
{
	int client_mes_size = strlen(CLIENT_PLAYER_MOVE) + strlen(END_PROTOCOL) + strlen(guess_seq) + 1;
	if (GET__CLIENT_PLAYER_MOVE_PRO(client_message, guess_seq) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_client, client_message, client_mes_size, FIFTEEN_SEC) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}
int Handle_Server_Main_Menu(SOCKET s_client, char* server_response,
	Player_Status play_status, char* client_message)
{
	int server_res_id = GET__Server_Response(s_client, server_response, FIFTEEN_SEC);
	if (server_res_id == SERVER_MAIN_MENU_ID)
	{
		Choose_Next_Play();
		play_status = GET__Play_Player_Decision();
		if (play_status != PLAY)
		{			
			return -1;
		}
		if (Handle_Client_Versus(s_client, client_message) == -1)
		{
			return -1;
		}
	}
	else
	{
		printf("SERVER_MAIN_MENU_ID pro: %s id: %d\n", server_response, server_res_id);
		if (server_res_id == SHUTDOWN)
		{			
			return SHUTDOWN;
		}
		return -1;
	}
	return 0;
}

int GET__Server_Response(SOCKET s_client, char * server_response, int max_wait_time)
{	
	snprintf(server_response, MAX_PRO_LEN, "\0");
	int retval = Recv_Socket(s_client, server_response, max_wait_time);
	if (retval == -1)
	{
		printf("Recv Failed\n");
		return -1;
	}
	if (retval == SHUTDOWN)
	{
		printf("Gracefull disconnect started\n");
		return SHUTDOWN;
	}
	return GET__Server_Response_ID(server_response);
}

