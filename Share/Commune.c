#include "Commune.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")

int GET__Server_Denied_PRO(char* protocol)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s%s", SERVER_DENIED, END_PROTOCOL) == 0)
		return -1;
	return 0;
}
int GET__Server_Approved_PRO(char* protocol)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s%s", SERVER_APPROVED, END_PROTOCOL) == 0)
		return -1;
	return 0;
}

int GET__Response_ID(char* protocol)
{
	char* message_type = GET__Message_Type(protocol);	
	if (message_type == NULL)
	{
		return -1;
	}	
	if (strcmp(message_type, SERVER_APPROVED) == 0)
	{
		return SERVER_APPROVED_ID;
	}
	if (strcmp(message_type, SERVER_DENIED) == 0)
	{
		return SERVER_DENIED_ID;
	}
	if (strcmp(message_type, SERVER_MAIN_MENU) == 0)
	{
		return SERVER_MAIN_MENU_ID;
	}
	if (strcmp(message_type, SERVER_INVITE) == 0)
	{
		return SERVER_INVITE_ID;
	}
	if (strcmp(message_type, SERVER_NO_OPPONENTS) == 0)
	{
		return SERVER_NO_OPPONENTS_ID;
	}
	if (strcmp(message_type, SERVER_SETUP_REQUEST) == 0)
	{
		return SERVER_SETUP_REQUEST_ID;
	}
	if (strcmp(message_type, SERVER_PLAYER_MOVE_REQUEST) == 0)
	{
		return SERVER_PLAYER_MOVE_REQUEST_ID;
	}
	if (strcmp(message_type, SERVER_GAME_RESULTS) == 0)
	{
		return SERVER_GAME_RESULTS_ID;
	}	
	if (strcmp(message_type, SERVER_WIN) == 0)
	{
		return SERVER_WIN_ID;
	}
	if (strcmp(message_type, SERVER_DRAW) == 0)
	{
		return SERVER_DRAW_ID;
	}
	if (strcmp(message_type, SERVER_OPPONENT_QUIT) == 0)
	{
		return SERVER_OPPONENT_QUIT_ID;
	}
	return -1;
}

int GET__CLIENT_REQUEST_PRO(char * protocol,char * username)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s:%s%s\0", CLIENT_REQUEST, username, END_PROTOCOL) == 0)
	{
		return -1;
	}
	return 0;
}
int GET__CLIENT_SETUP_PRO(char* protocol, char* setup_seq)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s:%s%s\0", CLIENT_SETUP, setup_seq, END_PROTOCOL) == 0)
	{
		return -1;
	}
	return 0;
}
int GET__CLIENT_PLAYER_MOVE_PRO(char* protocol, char* guess_seq)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s:%s%s\0", CLIENT_PLAYER_MOVE, guess_seq, END_PROTOCOL) == 0)
	{
		return -1;
	}
	return 0;
}
int GET__CLIENT_VERSUS_PRO(char* protocol)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s%s\0", CLIENT_VERSUS, END_PROTOCOL) == 0)
	{
		return -1;
	}
	return 0;
}
int GET__CLIENT_DISCONNECT_PRO(char* protocol)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s%s\0", CLIENT_DISCONNECT, END_PROTOCOL) == 0)
	{
		return -1;
	}
	return 0;
}

int GET__PRO_WITH_EOP(char* protocol)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s%s\0", protocol, END_PROTOCOL) == 0)
		return -1;
	return 0;
}
char* GET__Message_Type(char* protocol)
{	
	char* exit_char = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	if (exit_char == NULL)
	{
		goto Exit;
	}
	char* message_type = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	if (message_type == NULL)
	{
		exit_char = NULL;
		goto Exit;
	}
	if (snprintf(message_type, MAX_PRO_LEN, "%s", protocol) == 0)
	{
		exit_char = NULL;
		goto Free;
	}	
	char* next = NULL;
	if (snprintf(exit_char, MAX_PRO_LEN, "%s", strtok_s(message_type, ":\n", &next)) == 0)
	{
		free(exit_char);
		exit_char = NULL;
		goto Free;
	}
Free:
	free(message_type);
Exit:
	return exit_char;
}
BnC_Data* GET__BnC_Data(char* protocol)
{
	Server_Messages from_which_protocol = -1;
	BnC_Data* data = (BnC_Data*)calloc(1, sizeof(BnC_Data));
	if (data == NULL)
	{
		return NULL;
	}
	char* message_type, *token1, *token2, *token3, *token4;
	char* temp = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	if (temp == NULL)
	{
		return NULL;
	}	
	if (snprintf(temp, MAX_PRO_LEN, "%s", protocol) == 0)
	{
		return NULL;
	}
	char* next = NULL;
	message_type = strtok_s(temp, ":;\n",&next);
	token1 = strtok_s(NULL, ":;\n", &next);
	token2 = strtok_s(NULL, ":;\n", &next);
	token3 = strtok_s(NULL, ":;\n", &next);
	token4 = strtok_s(NULL, ":;\n", &next);
	if (token3 == NULL || token4 == NULL)
	{
		if (token2 == NULL)
		{
			if (token1 == NULL)
			{
				return NULL;
			}
			from_which_protocol = SERVER_INVITE_ID;
		}
		else
		{
			from_which_protocol = SERVER_WIN_ID;
		}	
	}
	else
	{
		from_which_protocol = SERVER_GAME_RESULTS_ID;
	}
	switch (from_which_protocol)
	{
	case SERVER_INVITE_ID:
		if (snprintf(data->opp_username, MAX_USERNAME_LEN, "%s", token1) == 0)
		{
			free(data);
			free(temp);
			return NULL;
		}
		break;
	case SERVER_WIN_ID:
		if (snprintf(data->opp_username, MAX_USERNAME_LEN, "%s", token1) == 0)
		{
			free(data);
			free(temp);
			return NULL;
		}
		if (snprintf(data->opp_move, NUM_DIGITIS_GUESS, "%s", token2) == 0)
		{
			free(data);
			free(temp);
			return NULL;
		}
		break;
	case SERVER_GAME_RESULTS_ID:
		data->bulls = (int)strtol(token1, NULL, DECIMAL_BASE);
		data->cows = (int)strtol(token2, NULL, DECIMAL_BASE);
		if (snprintf(data->opp_username, MAX_USERNAME_LEN, "%s", token3) == 0)
		{
			free(data);
			free(temp);
			return NULL;
		}
		if (snprintf(data->opp_move, NUM_DIGITIS_GUESS, "%s", token4) == 0)
		{
			free(data);
			free(temp);
			return NULL;
		}
		break;
	default:
		return NULL;
		break;
	}
	free(temp);
	return data;
}

int SendBuffer(SOCKET sd, const char* Buffer, int BytesToSend)
{
	const char* CurPlacePtr = Buffer;
	int BytesTransferred;
	int RemainingBytesToSend = BytesToSend;

	while (RemainingBytesToSend > 0)
	{
		/* send does not guarantee that the entire message is sent */
		BytesTransferred = send(sd, CurPlacePtr, RemainingBytesToSend, 0);
		if (BytesTransferred == SOCKET_ERROR)
		{
			printf("send() failed, error %d\n", WSAGetLastError());
			return -1;
		}
		printf("Number of bytes send: %d out of total %d\n", BytesTransferred, BytesToSend);
		RemainingBytesToSend -= BytesTransferred;
		CurPlacePtr += BytesTransferred;
	}
	return 0;
}
int Send_Socket(SOCKET s, const char* buffer, int len)
{
	return SendBuffer(s, buffer, len);
}
int ReceiveBuffer(SOCKET sd, char* OutputBuffer, int BytesToReceive)
{
	char* CurPlacePtr = OutputBuffer;
	int BytesJustTransferred;
	int RemainingBytesToReceive = BytesToReceive;

	while (RemainingBytesToReceive > 0)
	{
		/* send does not guarantee that the entire message is sent */
		BytesJustTransferred = recv(sd, CurPlacePtr, 1, 0);
		if (BytesJustTransferred == SOCKET_ERROR)
		{
			printf("recv() failed, error %d\n", WSAGetLastError());
			return -1;
		}
		else if (BytesJustTransferred == 0)
		{
			return 1; // recv() returns zero if connection was gracefully disconnected.
		}		
		RemainingBytesToReceive -= BytesJustTransferred;
		if (*CurPlacePtr == '\n')
		{
			*CurPlacePtr = '\0';
			printf("Remaining of 100 : %d\n", RemainingBytesToReceive);
			break;
		}
		CurPlacePtr += BytesJustTransferred; // <ISP> pointer arithmetic
	}

	return 0;
}
int Recv_Socket(SOCKET s, char* buffer)
{	
	return ReceiveBuffer(s, buffer, MAX_PRO_LEN);
}