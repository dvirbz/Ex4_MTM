#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "Ws2_32.lib")
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Commune.h"
#include "Lock.h"
#include <stdbool.h>

#define NUM_OF_THREADS 2
#define ARGUMENT_NUMBER_SERVER 2
#define PORT_NUMBER_INDEX 1
#define LOCAL_HOST_ADDRESS "127.0.0.1"
#define FILE_GAME_SESSION "GameSession.txt"
#define ERROR_CODE -1
#define BULLS_AND_COWS_STR_LEN 2
#define TEN_MINUTES 600000
/*SOCKET socket(
	int af, // address family specification
	int type, // type specification for the new socket
	int protocol // protocol to be used with the socket
	//that is specific to the indicated address family
);*/

HANDLE first_want_to_invite = NULL;
HANDLE second_want_to_invite = NULL;
HANDLE readAndWriteEvent = NULL;
int num_of_writing = -1;
int can_I_close_file = 0;


typedef struct {
	SOCKET ClientSocket;
	int ThreadNumber;
	Lock* file_lock;
}ThreadParams;

typedef struct
{
	char username[MAX_USERNAME_LEN];
	char setup[NUM_DIGITIS_GUESS];
	char move[NUM_DIGITIS_GUESS];
	int line_size;
	BOOL is_first_player;
	int bulls, cows;
}Player;

typedef enum
{
	CURRENT_WON = 1,
	OTHER_WON,
	DRAW,
	CONTINUE
}Game_Results;

HANDLE ThreadHandles[NUM_OF_THREADS];
ThreadParams ThreadInputs[NUM_OF_THREADS];

BOOL no_opponents()
{
	for (int i = 0; i < NUM_OF_THREADS; i++)
	{
		if (ThreadHandles[i] == NULL)
		{
			return TRUE;
		}
	}
	return FALSE;
}

void print_player(Player* player)
{
	printf("player name is: %s\n", player->username);
	printf("player setup is: %s\n", player->setup);
	printf("player move is: %s\n", player->move);
	printf("player line size: %d\n", player->line_size);
	printf("player is the first player? %d\n", player->is_first_player);
	printf("player bulls: %d\n", player->bulls);
	printf("player cows: %d\n", player->cows);
}

int InitializeWSA()
{
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
	{
		printf("Error at WSAStartup() code: %d\n",WSAGetLastError());
		return -1;
	}
	return 0;
}

int stringToPositiveInt(char* portNumberStr)
{
	int sum = 0;
	int i = 0;
	int currentDigit = 0;
	for (i; i < (int)strlen(portNumberStr); i++)
	{
		if (portNumberStr[i] <= '9' && portNumberStr[i] >= '0')
		{
			currentDigit = portNumberStr[i] - '0';
			sum = sum * 10;
			sum += currentDigit;
		}
		else
		{
			return -1;
		}
	}
	return sum;
}

void CleanupWorkerThreads()
{
	int Ind;

	for (Ind = 0; Ind < NUM_OF_THREADS; Ind++)
	{
		if (ThreadHandles[Ind] != NULL)
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], INFINITE);

			if (Res == WAIT_OBJECT_0)
			{
				closesocket(ThreadInputs[Ind].ClientSocket);
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				break;
			}
			else
			{
				printf("Waiting for thread failed. Ending program\n");
				return;
			}
		}
	}
}

int init_player(Player* player)
{
	printf("initializing player...\n");	
	if (snprintf(player->setup, NUM_DIGITIS_GUESS, "aaaa") == 0)
	{
		return -1;
	}
	if (snprintf(player->move, NUM_DIGITIS_GUESS, "aaaa") == 0)
	{
		return -1;
	}
	if (snprintf(player->username, MAX_USERNAME_LEN, "!!!!!!!!!!!!!!!!!!!!") == 0)
	{
		return -1;
	}
	player->line_size = strlen(player->username) + strlen(player->move) +
		3 * strlen(PARTITION_MASSAGE_PARAMETERS) + 2 * strlen(END_PROTOCOL) + BULLS_AND_COWS_STR_LEN;
	player->is_first_player = FALSE;
	player->bulls = 5;
	player->cows = 5;
	printf("usernameinit: %s\n", player->username);
	printf("succses!\n");
	return 0;
}

int update_data_from_file(Player* other_player, Player* current_player, char* player_info)
{
	char* username, * move, * bulls, * cows;
	char* next = NULL;
	username = strtok_s(player_info, ":\r", &next);
	move = strtok_s(NULL, ":\r", &next);
	bulls = strtok_s(NULL, ":\r", &next);
	cows = strtok_s(NULL, ":\r", &next);
	printf("Before Line size: %d\n", other_player->line_size);
	printf("username: %s\nmove: %s\nBulls: %s\nCows: %s\n", username, move, bulls, cows);
	if (username == NULL || move == NULL || bulls == NULL || cows == NULL)
	{
		printf("username or moveset == NULL\n");
		return -1;
	}
	if (snprintf(other_player->username, MAX_USERNAME_LEN, "%s", username) == 0)
	{
		printf("snprintf failed 3\n");
		return -1;
	}
	if (snprintf(other_player->move, NUM_DIGITIS_GUESS, "%s", move) == 0)
	{
		printf("snprintf failed 3\n");
		return -1;
	}
	current_player->bulls = (int)strtol(bulls, NULL, DECIMAL_BASE);
	if (current_player->bulls == 0 && bulls[0] != '0')
	{
		printf("strtol failed bulls: %s\n",bulls);
		return -1;
	}
	current_player->cows = (int)strtol(cows, NULL, DECIMAL_BASE);
	if (current_player->cows == 0 && cows[0] != '0')
	{
		printf("strtol failed cows: %s\n",cows);
		return -1;
	}
	other_player->line_size = strlen(username) + strlen(move) + strlen(bulls)
		+ strlen(cows) + strlen(PARTITION_MASSAGE_PARAMETERS) * 3 + 2 * strlen(END_PROTOCOL);
	printf("After Line size: %d\n", other_player->line_size);
	return 0;
}

int recive_client_request(SOCKET s_communication, char* client_response, Player* player)
{
	printf("got here\n");
	if (Recv_Socket(s_communication, client_response) == -1)
	{
		printf("Recv Failed\n");
		return -1;
		
	}
	if (GET__Client_Response_ID(client_response) != CLIENT_REQUEST_ID)
	{
		printf("Didn't CLIENT_REQUEST_ID\n");
		return -1;
	}
	printf("got here2 client_request: %s\n", client_response);
	BnC_Data* data = GET__BnC_Data(client_response);
	if (data == NULL)
	{
		printf("Data Failed\n");
		return -1;
	}
	if (snprintf(player->username, MAX_USERNAME_LEN, "%s", data->first_string) == 0)
	{
		free(data);
		printf("Copy Failed\n");
		return -1;
	}
	free(data);
	player->line_size = strlen(player->username) + strlen(player->move) +
		3 * strlen(PARTITION_MASSAGE_PARAMETERS) + 2 * strlen(END_PROTOCOL) + BULLS_AND_COWS_STR_LEN;
	printf("rcr username: %s\n", player->username);
	return 0;
}

int send_approved(SOCKET s_communication, char* server_massage)
{
	if (GET__Server_Pro(server_massage,SERVER_APPROVED) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage)) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}

int send_main_menu(SOCKET s_communication, char* server_massage)
{
	if (GET__Server_Pro(server_massage,SERVER_MAIN_MENU) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage)) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}

int send_server_no_opponents(SOCKET s_communication, char* server_massage)
{
	if (GET__Server_Pro(server_massage, SERVER_NO_OPPONENTS) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage)) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}

int send_invite(SOCKET s_communication, char* server_massage, char* other_username)
{	
	printf("sending invite...\n");
	if (GET__Server_Invite_Pro(server_massage, other_username) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage)) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	printf("invite sent?\n");
	return 0;
}

int send_setup_request(SOCKET s_communication, char* server_massage)
{
	if (GET__Server_Pro(server_massage,SERVER_SETUP_REQUEST) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage)) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}

int send_move_request(SOCKET s_communication, char* server_massage)
{
	if (GET__Server_Pro(server_massage,SERVER_PLAYER_MOVE_REQUEST) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage)) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}

int send_game_results(SOCKET s_communication, char* server_massage,
	Player* other_player, Player* current_player)
{
	if (GET__Server_Game_Results_Pro(server_massage,current_player->bulls,current_player->cows,
	other_player->username,other_player->move) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage)) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}

int send_game_won(SOCKET s_communication, char* server_massage,
	Player* other_player, Player* current_player)
{
	if (GET__Server_Won_Pro(server_massage, current_player->username, other_player->setup) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage)) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}

int send_game_draw(SOCKET s_communication, char* server_massage,
	Player* other_player, Player* current_player)
{
	if (GET__Server_Pro(server_massage,SERVER_DRAW) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage)) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}

int write_to_file(HANDLE gameSession, Player* current_player, Player* other_player, Lock* file_lock)
{	
	if (Write__Lock__Mutex(file_lock, 5000) == FALSE)
	{
		printf("write lock mutex\n");
		return -1;
	}
	if (Write__Lock(file_lock, 5000, NUM_OF_THREADS) == FALSE)
	{
		Write__Release__Mutex(file_lock);
		printf("write lock\n");
		return -1;
	}
	int distance_to_move = 0;
	if (current_player->is_first_player == FALSE)
	{
		distance_to_move = other_player->line_size;
	}
	if (SetFilePointer(gameSession, distance_to_move, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		Write__Release(file_lock, NUM_OF_THREADS);
		Write__Release__Mutex(file_lock);
		printf("couldn't set file pointer, error: %d\n", GetLastError());
		return -1;
	}
	char line_to_write[MAX_LINE_LEN];
	int character_count = snprintf(line_to_write, MAX_LINE_LEN, "%s:%s:%d:%d\r\n",
		current_player->username, current_player->move, other_player->bulls, other_player->cows);
	if (character_count == 0)
	{
		Write__Release(file_lock, NUM_OF_THREADS);
		Write__Release__Mutex(file_lock);
		printf("couldn't write string\n");		
		return -1;
	}
	if (WriteFile(gameSession, line_to_write, strlen(line_to_write), NULL, NULL) == 0)
	{
		Write__Release(file_lock, NUM_OF_THREADS);
		Write__Release__Mutex(file_lock);
		printf("failed to write to file\n");		
		return -1;
	}
	num_of_writing++;
	if (Write__Release(file_lock, NUM_OF_THREADS) == FALSE)
	{
		Write__Release__Mutex(file_lock);
		printf("write release\n");
		return -1;
	}
	if (Write__Release__Mutex(file_lock) == FALSE)
	{
		printf("write release mutex\n");
		return -1;
	}
	
	return 0;
}

int read__line(HANDLE gameSession, Player* current_player, Player* other_player, Lock* file_lock)
{
	while ((num_of_writing % 2) != 0);
	if (Read__Lock(file_lock, 5000) == FALSE)
	{
		printf("Could't lock in 5 sec\n");
		return -1;
	}
	int distance_to_move = current_player->line_size;
	if (current_player->is_first_player == FALSE)
	{
		distance_to_move = 0;
	}
	if (SetFilePointer(gameSession, distance_to_move, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		printf("couldn't set file pointer\n");
		return -1;
	}
	char line[MAX_LINE_LEN];
	if (ReadFile(gameSession, line, other_player->line_size, NULL, NULL) == 0)
	{
		printf("couldn't read file\n");
		return -1;
	}	
	line[other_player->line_size] = '\0';	
	if (Read__Release(file_lock) == FALSE)
	{
		printf("couldn't release\n");
		return -1;
	}
	if (update_data_from_file(other_player,current_player,line) == -1)
	{
		printf("couldn't update_player\n");
		return -1;
	}
	printf("User: %s other user: %s line: %s\n", current_player->username, other_player->username, line);
	return 0;
}

int write_and_read(HANDLE gameSession, Player* current_player, Player* other_player, Lock* file_lock)
{
	if (write_to_file(gameSession, current_player, other_player, file_lock) == -1)
	{
		printf("can't write to file\n");
		return -1;
	}

	if (read__line(gameSession, current_player, other_player, file_lock) == -1)
	{
		printf("can't read from file\n");
		return -1;
	}
	return 0;
}

int versus_or_disconnect(SOCKET s_communication, HANDLE* gameSession, char* client_response,
	char* server_massage, Player* current_player, Player* other_player, Lock* file_lock)
{
	if (Recv_Socket(s_communication, client_response) == -1)
	{
		printf("Recv Failed\n");
		return ERROR_CODE;
	}
	int response = GET__Client_Response_ID(client_response);
	if (response == CLIENT_DISCONNECT_ID)
	{
		printf("client disconnected, id: %d", CLIENT_DISCONNECT_ID);
		return CLIENT_DISCONNECT_ID;
	}
	//versus
	//try to open file:
	if (response != CLIENT_VERSUS_ID)
	{
		printf("didnt get expacted response from versus or disconnect, ID = %d", response);
		return ERROR_CODE;
	}
	//IS there another client Wait lesemaphore haim yesh od sahkan;
	if (no_opponents() == TRUE)
	{
		if (send_server_no_opponents(s_communication, server_massage) == ERROR_CODE)
		{
			printf("couldn't send server no opponents\n");
			return ERROR_CODE;
		}
		return SERVER_NO_OPPONENTS_ID;
	}
	if (Write__First__Lock__Mutex(file_lock, 15000) == FALSE)
	{
		printf("Deadlock first write\n");
		return ERROR_CODE;
	}
	*gameSession = CreateFileA(FILE_GAME_SESSION, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (*gameSession == INVALID_HANDLE_VALUE)
	{
		if (*gameSession == INVALID_HANDLE_VALUE || *gameSession == NULL)
		{
			*gameSession = CreateFileA(FILE_GAME_SESSION, GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		}
		if (*gameSession == INVALID_HANDLE_VALUE)
		{
			printf("can't open file, last error: %d\n", GetLastError());
			return ERROR_CODE;
		}
		current_player->is_first_player = FALSE;
		other_player->is_first_player = TRUE;
		if (read__line(*gameSession, current_player, other_player, file_lock) != 0)
		{
			printf("failed to read line\n");
			return ERROR_CODE;
		}
		if (write_to_file(*gameSession, current_player, other_player, file_lock) != 0)
		{
			printf("failed to write line\n");
			return ERROR_CODE;
		}
		if (Write__First__Release__Mutex(file_lock) == FALSE)
		{
			printf("Coulden't release lock\n");
			return ERROR_CODE;
		}
		num_of_writing++;
		if (!SetEvent(readAndWriteEvent))
		{
			printf("SetEvent failed (%d)\n", GetLastError());
			return ERROR_CODE;
		}
	}
	else
	{
		// we are first
		current_player->is_first_player = TRUE;
		other_player->is_first_player = FALSE;
		printf("Im the first player\n");
		if (write_to_file(*gameSession, current_player, other_player, file_lock) != 0)
		{
			printf("failed to write line\n");
			if (Write__First__Release__Mutex(file_lock) == FALSE)
			{
				printf("Coulden't release lock\n");
				return ERROR_CODE;
			}
			return ERROR_CODE;
		}
		if (Write__First__Release__Mutex(file_lock) == FALSE)
		{
			printf("Coulden't release lock\n");
			return ERROR_CODE;
		}
		DWORD WaitResult;
		WaitResult = WaitForSingleObject(
			readAndWriteEvent,
			TEN_MINUTES);
		switch (WaitResult)
		{
		case WAIT_OBJECT_0:
			break;
		case WAIT_FAILED:
			return ERROR_CODE;
			break;
		case WAIT_TIMEOUT:
			if (send_server_no_opponents(s_communication, server_massage) == ERROR_CODE)
			{
				printf("couldn't send server no opponents\n");
				return ERROR_CODE;
			}
			printf("waited too long for other opponent to write\n");
			return SERVER_NO_OPPONENTS_ID;
		}
		if (read__line(*gameSession, current_player, other_player, file_lock) != 0)
		{
			printf("couldn't read line after sleep\n");
			return ERROR_CODE;
		}
	}
	print_player(current_player);
	print_player(other_player);
	//check if both players want to send invite
	DWORD retVal;
	if (current_player->is_first_player == TRUE)
	{
		SetEvent(first_want_to_invite);
		retVal = WaitForSingleObject(second_want_to_invite, TEN_MINUTES);
	}
	else
	{
		SetEvent(second_want_to_invite);
		retVal = WaitForSingleObject(first_want_to_invite, TEN_MINUTES);
	}
	switch (retVal)
	{
	case WAIT_OBJECT_0:
		break;
	case WAIT_FAILED:
		return ERROR_CODE;
		break;
	case WAIT_TIMEOUT:
		if (send_server_no_opponents(s_communication, server_massage) == ERROR_CODE)
		{
			printf("couldn't send server no opponents\n");
			return ERROR_CODE;
		}
		return SERVER_NO_OPPONENTS_ID;
	}

	if (send_invite(s_communication, server_massage, other_player->username) == ERROR_CODE)
	{
		printf("send invite failed!\n");
		return ERROR_CODE;
	}
	return 0;
}

int Handle_setup(SOCKET s_communication, char* client_response, char* server_massage,
	Player* current_player, Player* other_player, HANDLE gameSession, Lock* file_lock)
{
	if (send_setup_request(s_communication, server_massage) == ERROR_CODE)
	{
		printf("send setupReq failed!\n");
		return ERROR_CODE;
	}

	if (Recv_Socket(s_communication, client_response) == -1)
	{
		printf("Recv Failed\n");
		return -1;
	}

	if (GET__Client_Response_ID(client_response) != CLIENT_SETUP_ID)
	{
		printf("didn't get setup id, got: %s\n", client_response);
		return -1;
	}

	BnC_Data* data = GET__BnC_Data(client_response);
	if (data == NULL)
	{
		printf("can't get BnCdata, got: %s\n", client_response);
		return -1;
	}

	if (snprintf(current_player->setup, NUM_DIGITIS_GUESS, "%s", data->first_string) == 0)
	{
		printf("can't snprintf\n");
		return -1;
	}
	free(data);
	if (snprintf(current_player->move, NUM_DIGITIS_GUESS, "%s", current_player->setup) == 0)
	{
		printf("can't snprintf\n");
		return -1;
	}
	if (write_and_read(gameSession, current_player, other_player, file_lock) != 0)
	{
		return -1;
	}
	return 0;
}

int Handle_move(SOCKET s_communication, char* client_response,char* server_massage,
	Player* current_player, Player* other_player,HANDLE gameSession, Lock* file_lock)
{
	if (send_move_request(s_communication, server_massage) == ERROR_CODE)
	{
		printf("send setupReq failed!\n");
		return ERROR_CODE;
	}

	if (Recv_Socket(s_communication, client_response) == -1)
	{
		printf("Recv Failed\n");
		return -1;
	}

	if (GET__Client_Response_ID(client_response) != CLIENT_PLAYER_MOVE_ID)
	{
		printf("didn't get player move id, got: %s\n", client_response);
		return -1;
	}

	BnC_Data* data = GET__BnC_Data(client_response);
	if (data == NULL)
	{
		printf("can't get BnCdata, got: %s\n", client_response);
		return -1;
	}

	if (snprintf(current_player->move, NUM_DIGITIS_GUESS, "%s", data->first_string) == 0)
	{
		printf("can't snprintf\n");
		return -1;
	}
	free(data);
	if (write_and_read(gameSession, current_player, other_player, file_lock) != 0)
	{
		return -1;
	}

	return 0;
}

BnC_Data* GET__Bulls_And_Cows(char* setup, char* guess)
{
	int bulls = 0, cows = 0;
	for (unsigned int i = 0; i < strlen(guess); i++)
	{
		for (unsigned int j = 0; j < strlen(setup); j++)
		{
			if (guess[i] == setup[j])
			{
				if (i == j)
				{
					bulls++;
				}
				else
				{
					cows++;
				}
			}
		}
	}
	BnC_Data* data = (BnC_Data*)calloc(1, sizeof(BnC_Data));
	if (data == NULL)
	{
		printf("MEM Allocation Fail\n");
		return NULL;
	}
	data->bulls = bulls;
	data->cows = cows;
	data->first_string[0] = '\0';
	data->second_string[0] = '\0';
	return data;
}

Game_Results GET__Game_Results(Player* current_player, Player* other_player)
{
	if (current_player->bulls == NUM_DIGITIS_GUESS - 1)
	{
		if (other_player->bulls == NUM_DIGITIS_GUESS - 1)
		{
			return DRAW;
		}
		return CURRENT_WON;
	}
	if (other_player->bulls == NUM_DIGITIS_GUESS - 1)
	{
		return OTHER_WON;
	}
	return CONTINUE;
}

DWORD WINAPI StartThread(LPVOID lp_params)
{
	ThreadParams threadInput = *(ThreadParams*)lp_params;
	Lock* file_lock = threadInput.file_lock;
	SOCKET s_communication = threadInput.ClientSocket;
	HANDLE gameSession = NULL;

	int exit_code = 0, distance_to_move = 0; 
	char* server_massage = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	if (server_massage == NULL)
	{
		printf("cant malloc server massage\n");
		exit_code = -1;
		goto Exit_No_Free;
	}
	char* client_response = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	if (client_response == NULL)
	{
		free(server_massage);
		printf("cant malloc client response\n");
		exit_code = -1;
		goto Exit_No_Free;
	}
	
	Player* current_player = (Player*)calloc(1, sizeof(Player));
	if (current_player == NULL)
	{
		exit_code = -1;
		goto Exit_No_Free;
	}
	Player* other_player = (Player*)calloc(1, sizeof(Player));	
	if (other_player == NULL)
	{
		exit_code = -1;
		goto Exit_No_Free;
	}
	exit_code = init_player(current_player);
	if (exit_code != 0)
		goto ExitSeq;
	exit_code = init_player(other_player);
	if (exit_code != 0)
		goto ExitSeq;
	exit_code = recive_client_request(s_communication, client_response, current_player);
	if(exit_code != 0)
		goto ExitSeq;

	printf("Your Username: %s\n", current_player->username);
	exit_code = send_approved(s_communication, server_massage);
	if (exit_code == -1)
		goto ExitSeq;
	while (TRUE)
	{	
		MainMenu:
		exit_code = send_main_menu(s_communication, server_massage);
		if (exit_code == -1)
			goto ExitSeq;
		switch (versus_or_disconnect(s_communication, &gameSession, client_response,
			server_massage, current_player, other_player, file_lock))
		{
		case ERROR_CODE:
			exit_code = ERROR_CODE;
			goto ResetGame;
		case CLIENT_DISCONNECT_ID:
			exit_code = 0;			
			goto ExitSeq;
			break;
		case SERVER_NO_OPPONENTS_ID:
			exit_code = 0;
			goto MainMenu;
			break;
		}
		/*if (gameSession == INVALID_HANDLE_VALUE || gameSession == NULL)
		{
			printf("can't open file\n");
			goto ExitSeq;
		}*/
		printf("Yay\n");
		if (Handle_setup(s_communication, client_response, server_massage, current_player,
			other_player, gameSession, file_lock) == -1)
		{
			printf("can't handle setup\n");
			exit_code = -1;
			goto ResetGame;
		}
		print_player(other_player);
		if (snprintf(other_player->setup, NUM_DIGITIS_GUESS, "%s", other_player->move) == 0)
		{
			printf("snprintf failed\n");
			exit_code = -1;
			goto ResetGame;
		}
		print_player(other_player);
		int play_status = 0;
		while (play_status == 0)
		{
			if (Handle_move(s_communication, client_response, server_massage,
				current_player, other_player, gameSession, file_lock) == -1)
			{
				printf("can't handle move\n");
				exit_code = -1;
				goto ResetGame;
			}
			/* check for BnC, send to client*/
			BnC_Data* data = GET__Bulls_And_Cows(current_player->setup, other_player->move);
			if (data == NULL)
			{
				printf("can't handle data\n");
				exit_code = -1;
				goto ResetGame;
			}
			other_player->bulls = data->bulls;
			other_player->cows = data->cows;
			if (write_to_file(gameSession, current_player, other_player, file_lock) != 0)
			{
				printf("can't write to file\n");
				exit_code = -1;
				goto ResetGame;
			}
			if (read__line(gameSession, current_player, other_player, file_lock) != 0)
			{
				printf("can't read from file\n");
				exit_code = -1;
				goto ResetGame;
			}
			switch (GET__Game_Results(current_player, other_player))
			{
			case CURRENT_WON:
				send_game_won(s_communication, server_massage, other_player, current_player);
				printf("I Won\n");
				play_status = -1;
				break;
			case OTHER_WON:
				send_game_won(s_communication, server_massage, other_player, other_player);
				printf("Other Won\n");
				play_status = -1;
				break;
			case DRAW:
				send_game_draw(s_communication, server_massage, other_player, current_player);
				printf("It's a tie\n");
				play_status = -1;
				break;
			case CONTINUE:
				if (send_game_results(s_communication, server_massage, other_player, current_player) != 0)
				{
					printf("can't send game results\n");
					exit_code = -1;
					goto ResetGame;
				}
				break;
			}
		}

	ResetGame:
		if (gameSession != NULL)
		{
			CloseHandle(gameSession);
			can_I_close_file++;
		}
		gameSession = NULL;
		if (current_player->is_first_player == TRUE)//CloseFile
		{
			while (can_I_close_file % 2 != 0);
			if (DeleteFileA(FILE_GAME_SESSION) == 0)
			{
				printf("error: %d\n", GetLastError());
			}
		}
		ResetEvent(readAndWriteEvent);
		if (current_player->is_first_player == TRUE)
		{
			ResetEvent(first_want_to_invite);
		}
		else
		{
			ResetEvent(second_want_to_invite);
		}
		if (exit_code == ERROR_CODE)
		{
			goto ExitSeq;
		}
	}
	

ExitSeq:
	printf("entered ExitSeq\n");
	print_player(current_player);			
	free(server_massage);
	free(client_response);
	free(current_player);
	free(other_player);
Exit_No_Free:	
	if (closesocket(s_communication) == SOCKET_ERROR)
	{
		printf("Failed to close MainSocketThread, error %ld. Ending program\n", WSAGetLastError());
	}	
	printf("player quit\n");
	return exit_code;
}

int FindFirstUnusedThreadSlot()
{
	int Ind;

	for (Ind = 0; Ind < NUM_OF_THREADS; Ind++)
	{
		if (ThreadHandles[Ind] == NULL)
			break;
		else
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 0);

			if (Res == WAIT_OBJECT_0) // this thread finished running
			{
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				break;
			}
		}
	}

	return Ind;
}

int main(int argc, char* argv[])
{
	assert(argc == ARGUMENT_NUMBER_SERVER);

	int Ind, exit_code = 0;
	SOCKET s_server = INVALID_SOCKET;
	int portNumber;
	unsigned long address;
	SOCKADDR_IN service;
	Lock* file_lock = New__Lock(NUM_OF_THREADS);
	if (file_lock == NULL)
	{
		return -1;
	}
	readAndWriteEvent = CreateEvent(
		NULL,               // default security attributes
		TRUE,               // manual-reset event
		FALSE,              // initial state is nonsignaled
		TEXT("WriteEvent")  // object name
	);
	first_want_to_invite = CreateEvent(
		NULL,               // default security attributes
		TRUE,               // manual-reset event
		FALSE,              // initial state is nonsignaled
		TEXT("FirstInvite")  // object name
	);
	second_want_to_invite = CreateEvent(
		NULL,               // default security attributes
		TRUE,               // manual-reset event
		FALSE,              // initial state is nonsignaled
		TEXT("SecondInvite")  // object name
	);
	if (readAndWriteEvent == NULL)
	{
		printf("couldn't create event");
		return -1;
	}

	portNumber = (int)strtol(argv[PORT_NUMBER_INDEX], NULL, DECIMAL_BASE);
	if (portNumber == 0)
	{
		Destroy__lock(file_lock);
		return -1;
	}
	printf("your port number is: %d\n", portNumber);

	assert(InitializeWSA() == 0);
	s_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s_server == INVALID_SOCKET)
	{
		printf("Error at socket( ): %ld\n", WSAGetLastError());
		exit_code = -1;
		goto ServerCleanUp;
	}

	address = INADDR_ANY;
	if (address == INADDR_NONE)
	{
		printf("The string \"%s\" cannot be converted into an ip address. ending program.\n", LOCAL_HOST_ADDRESS);
		exit_code = -1;
		goto CloseSocket;
	}

	service.sin_family = AF_INET;
	service.sin_addr.s_addr = address;
	service.sin_port = htons(portNumber);

	//Bind Socket
	if (bind(s_server, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR)
	{
		printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
		exit_code = -1;
		goto CloseSocket;
	}

	//Listen on Socket
	if (listen(s_server, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		exit_code = -1;
		goto CloseSocket;
	}

	for (Ind = 0; Ind < NUM_OF_THREADS; Ind++)
	{
		ThreadHandles[Ind] = NULL;
	}
	printf("Waiting for a client to connect...\n");

	while (TRUE)
	{
		SOCKET AcceptSocket = accept(s_server, NULL, NULL);
		if (AcceptSocket == INVALID_SOCKET)
		{
			printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
			exit_code = -1;
			goto CloseSocket;
		}

		printf("Client Connected.\n");

		Ind = FindFirstUnusedThreadSlot();

		if (Ind == NUM_OF_THREADS) //two players already play
		{
			printf("No slots available for client, dropping the connection.\n");
			closesocket(AcceptSocket); //Server Denied Protocol also check for failure
		}
		else
		{
			// need to start thread
			ThreadInputs[Ind].ClientSocket = AcceptSocket;
			ThreadInputs[Ind].ThreadNumber = Ind;
			ThreadInputs[Ind].file_lock = file_lock;
			ThreadHandles[Ind] = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)StartThread,
				&(ThreadInputs[Ind]),
				0,
				NULL
			);
		}

	}

	printf("end");
	CleanupWorkerThreads();
CloseSocket:
	if (closesocket(s_server) == SOCKET_ERROR)
	{
		printf("Failed to close MainSocketServer, error %ld. Ending program\n", WSAGetLastError());
	}
ServerCleanUp:
	Destroy__lock(file_lock);
	if (WSACleanup() == SOCKET_ERROR)
	{
		printf("Failed to close WinsocketServer, error %ld. Ending program.\n", WSAGetLastError());
	}
	if (readAndWriteEvent != NULL)
	{
		CloseHandle(readAndWriteEvent);
	}
	return exit_code;
}
#endif