#include "Commune.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")

int GET__Server_Invite_Pro(char* protocol, char* username)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s%s%s%s", SERVER_INVITE, PARTITION_MASSAGE_PARAMETERS, username, END_PROTOCOL) == 0)
		return -1;
	return 0;
}
int GET__Server_Game_Results_Pro(char* protocol, int bulls, int cows,
	char* other_player_user, char* other_player_move)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s:%d;%d;%s;%s%s", SERVER_GAME_RESULTS, bulls, cows,
		other_player_user, other_player_move, END_PROTOCOL) == 0)
		return -1;
	return 0;
}
int GET__Server_Won_Pro(char* protocol, char* wining_player_username, char* other_player_setup)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s:%s;%s%s", SERVER_WIN, wining_player_username, other_player_setup, END_PROTOCOL) == 0)
		return -1;
	return 0;
}
int GET__Server_Pro(char* protocol, const char* message_type)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s%s", message_type, END_PROTOCOL) == 0)
		return -1;
	return 0;
}

int GET__Server_Response_ID(char* protocol)
{
	int response_id = -1;
	char* message_type = GET__Message_Type(protocol);
	if (message_type == NULL)
	{
		response_id = -1;
		goto Exit;
	}
	if (strcmp(message_type, SERVER_APPROVED) == 0)
	{
		response_id = SERVER_APPROVED_ID;
		goto Exit;
	}
	if (strcmp(message_type, SERVER_DENIED) == 0)
	{
		response_id = SERVER_DENIED_ID;
		goto Exit;
	}
	if (strcmp(message_type, SERVER_MAIN_MENU) == 0)
	{
		response_id = SERVER_MAIN_MENU_ID;
		goto Exit;
	}
	if (strcmp(message_type, SERVER_INVITE) == 0)
	{
		response_id = SERVER_INVITE_ID;
		goto Exit;
	}
	if (strcmp(message_type, SERVER_NO_OPPONENTS) == 0)
	{
		response_id = SERVER_NO_OPPONENTS_ID;
		goto Exit;
	}
	if (strcmp(message_type, SERVER_SETUP_REQUEST) == 0)
	{
		response_id = SERVER_SETUP_REQUEST_ID;
		goto Exit;
	}
	if (strcmp(message_type, SERVER_PLAYER_MOVE_REQUEST) == 0)
	{
		response_id = SERVER_PLAYER_MOVE_REQUEST_ID;
		goto Exit;
	}
	if (strcmp(message_type, SERVER_GAME_RESULTS) == 0)
	{
		response_id = SERVER_GAME_RESULTS_ID;
		goto Exit;
	}
	if (strcmp(message_type, SERVER_WIN) == 0)
	{
		response_id = SERVER_WIN_ID;
		goto Exit;
	}
	if (strcmp(message_type, SERVER_DRAW) == 0)
	{
		response_id = SERVER_DRAW_ID;
		goto Exit;
	}
	if (strcmp(message_type, SERVER_OPPONENT_QUIT) == 0)
	{
		response_id = SERVER_OPPONENT_QUIT_ID;
		goto Exit;
	}
Exit:
	free(message_type);
	return response_id;
}
int GET__Client_Response_ID(char* protocol)
{
	int response_id = -1;
	char* message_type = GET__Message_Type(protocol);
	if (message_type == NULL)
	{
		response_id = -1;
		goto Exit;
	}
	if (strcmp(message_type, CLIENT_REQUEST) == 0)
	{
		response_id = CLIENT_REQUEST_ID;
		goto Exit;
	}
	if (strcmp(message_type, CLIENT_VERSUS) == 0)
	{
		response_id = CLIENT_VERSUS_ID;
		goto Exit;
	}
	if (strcmp(message_type, CLIENT_DISCONNECT) == 0)
	{
		response_id = CLIENT_DISCONNECT_ID;
		goto Exit;
	}
	if (strcmp(message_type, CLIENT_SETUP) == 0)
	{
		response_id = CLIENT_SETUP_ID;
		goto Exit;
	}
	if (strcmp(message_type, CLIENT_PLAYER_MOVE) == 0)
	{
		response_id = CLIENT_PLAYER_MOVE_ID;
		goto Exit;
	}

Exit:
	free(message_type);
	return response_id;
}

int GET__CLIENT_REQUEST_PRO(char* protocol, char* username)
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
	char* message_type, * token1, * token2, * token3, * token4;
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
	message_type = strtok_s(temp, ":;\n", &next);
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
		data->bulls = -1;
		data->cows = -1;
		if (snprintf(data->first_string, MAX_USERNAME_LEN, "%s", token1) == 0)
		{
			free(data);
			free(temp);
			return NULL;
		}
		if (snprintf(data->second_string, NUM_DIGITIS_GUESS, " \0") == 0)
		{
			free(data);
			free(temp);
			return NULL;
		}
		break;
	case SERVER_WIN_ID:
		data->bulls = -1;
		data->cows = -1;
		if (snprintf(data->first_string, MAX_USERNAME_LEN, "%s", token1) == 0)
		{
			free(data);
			free(temp);
			return NULL;
		}
		if (snprintf(data->second_string, NUM_DIGITIS_GUESS, "%s", token2) == 0)
		{
			free(data);
			free(temp);
			return NULL;
		}
		break;
	case SERVER_GAME_RESULTS_ID:
		data->bulls = (int)strtol(token1, NULL, DECIMAL_BASE);
		data->cows = (int)strtol(token2, NULL, DECIMAL_BASE);
		if (snprintf(data->first_string, MAX_USERNAME_LEN, "%s", token3) == 0)
		{
			free(data);
			free(temp);
			return NULL;
		}
		if (snprintf(data->second_string, NUM_DIGITIS_GUESS, "%s", token4) == 0)
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

int SendBuffer(SOCKET sd, const char* Buffer, int BytesToSend, int max_wait_time)
{
	const char* CurPlacePtr = Buffer;
	int BytesTransferred;
	int RemainingBytesToSend = BytesToSend;
	if (setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, (char*)&max_wait_time, sizeof(max_wait_time)) == SOCKET_ERROR)
	{
		printf("Couldn't set socket send time out, Last error: %d", GetLastError());
		return ERROR_CODE;
	}
	while (RemainingBytesToSend > 0)
	{
		/* send does not guarantee that the entire message is sent */
		BytesTransferred = send(sd, CurPlacePtr, RemainingBytesToSend, 0);
		if (BytesTransferred == SOCKET_ERROR)
		{
			printf("send() failed, error %d\n", WSAGetLastError());
			return ERROR_CODE;
		}
		printf("Number of bytes send: %d out of total %d\n", BytesTransferred, BytesToSend);
		RemainingBytesToSend -= BytesTransferred;
		CurPlacePtr += BytesTransferred;
	}
	return 0;
}
int Send_Socket(SOCKET s, const char* buffer, int len, int max_wait_time)
{
	return SendBuffer(s, buffer, len,max_wait_time);//?+1 for terminating zero
}
int ReceiveBuffer(SOCKET sd, char* OutputBuffer, int BytesToReceive, int max_wait_time)
{
	char* CurPlacePtr = OutputBuffer;
	int BytesJustTransferred;
	int RemainingBytesToReceive = BytesToReceive;
	if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char*)&max_wait_time, sizeof(max_wait_time)) == SOCKET_ERROR)
	{
		printf("Couldn't set socket recv time out, Last error: %d", GetLastError());
		return ERROR_CODE;
	}
	while (RemainingBytesToReceive > 0)
	{
		/* send does not guarantee that the entire message is sent */
		BytesJustTransferred = recv(sd, CurPlacePtr, 1, 0);
		if (BytesJustTransferred == SOCKET_ERROR)
		{
			return -1;
		}
		else if (BytesJustTransferred == 0)
		{
			return SHUTDOWN; // recv() returns zero if connection was gracefully disconnected.
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
int Recv_Socket(SOCKET s, char* buffer, int max_wait_time)
{
	return ReceiveBuffer(s, buffer, MAX_PRO_LEN,max_wait_time);
}
