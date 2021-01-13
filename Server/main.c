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
#define EXIT_CODE -1

/*SOCKET socket(
	int af, // address family specification
	int type, // type specification for the new socket
	int protocol // protocol to be used with the socket
	//that is specific to the indicated address family
);*/

BOOL second_thread_wrote = FALSE;
int num_of_writing = 0;

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

HANDLE ThreadHandles[NUM_OF_THREADS];
ThreadParams ThreadInputs[NUM_OF_THREADS];

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
	player->line_size = strlen(player->username) + 11;
	player->is_first_player = FALSE;
	player->bulls = 5;
	player->cows = 5;
	printf("usernameinit: %s\n", player->username);
	printf("succses!\n");
	return 0;
}

int update_player(Player* player, char* player_info)
{
	char* username, * move, * bulls, * cows;
	char* next = NULL;
	username = strtok_s(player_info, ":\r", &next);
	move = strtok_s(NULL, ":\r", &next);
	bulls = strtok_s(NULL, ":\r", &next);
	cows = strtok_s(NULL, ":\r", &next);
	if (username == NULL || move == NULL || bulls == NULL || cows == NULL)
	{
		printf("username or moveset == NULL\n");
		return -1;
	}
	if (snprintf(player->move, NUM_DIGITIS_GUESS, "%s", move) == 0)
	{
		printf("snprintf failed 3\n");
		return -1;
	}
	player->bulls = (int)strtol(bulls, NULL, DECIMAL_BASE);
	if (player->bulls == 0 && bulls[0] == '0')
	{
		printf("strtol failed 3\n");
		return -1;
	}
	player->cows = (int)strtol(cows, NULL, DECIMAL_BASE);
	if (player->cows == 0 && cows[0] == '0')
	{
		printf("strtol failed 3\n");
		return -1;
	}
	player->line_size = strlen(username) + strlen(move) + strlen(PARTITION_MASSAGE_PARAMETERS) * 3 + 4;
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
	player->line_size = strlen(player->username) + 7;
	printf("rcr username: %s\n", player->username);
	return 0;
}

int send_approved(SOCKET s_communication, char* server_massage)
{
	if (GET__Server_Approved_PRO(server_massage) == -1)
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
	if (GET__Server_Main_Menu_PRO(server_massage) == -1)
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
	if (GET__Server_Invite_PRO(server_massage, other_username) == -1)
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
	if (GET__Server_Setup_Request_PRO(server_massage) == -1)
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
	if (GET__Server_Move_Request_PRO(server_massage) == -1)
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

int write_to_file(HANDLE gameSession, Player* player, int distance_to_move, Lock* file_lock)
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
	if (SetFilePointer(gameSession, distance_to_move, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		Write__Release(file_lock, NUM_OF_THREADS);
		Write__Release__Mutex(file_lock);
		printf("couldn't set file pointer, error: %d\n", GetLastError());
		return -1;
	}
	char line_to_write[MAX_LINE_LEN];
	int character_count = snprintf(line_to_write, MAX_LINE_LEN, "%s:%s:%d:%d\r\n",
		player->username, player->move, player->bulls, player->cows);
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

int read__line(HANDLE gameSession, Player* current_player, Player* other_player, int distance_to_move, Lock* file_lock)
{
	if (Read__Lock(file_lock, 5000) == FALSE)
	{
		printf("Could't lock in 5 sec\n");
		return -1;
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
	if (Read__Release(file_lock) == FALSE)
	{
		printf("couldn't release\n");
		return -1;
	}
	if (update_player(other_player,line) == -1)
	{
		printf("couldn't update_player");
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
		return EXIT_CODE;
	}
	switch (GET__Client_Response_ID(client_response))
	{
	case CLIENT_DISCONNECT_ID:
		return CLIENT_DISCONNECT_ID;
	case CLIENT_VERSUS_ID:
		Write__First__Lock__Mutex(file_lock, 5000);
		current_player->is_first_player = TRUE;
		*gameSession = CreateFileA("GameSession.txt", GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (*gameSession == INVALID_HANDLE_VALUE)
		{	//ready to play MUTEX!!
			current_player->is_first_player = FALSE;
		}
		else
		{
			current_player->is_first_player = TRUE;
		}
		if (current_player->is_first_player == FALSE)
		{
			Write__First__Release__Mutex(file_lock);
			*gameSession = CreateFileA("GameSession.txt", GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (*gameSession == INVALID_HANDLE_VALUE)
			{
				printf("can't open file, last error: %d\n", GetLastError());
				return EXIT_CODE;
			}
			if (read__line(*gameSession, current_player, other_player, 0, file_lock) != 0)
			{
				printf("failed to read line\n");
				return EXIT_CODE;
			}
			if (write_to_file(*gameSession, current_player, other_player->line_size, file_lock) != 0)
			{
				printf("failed to write line\n");
				return EXIT_CODE;
			}
			second_thread_wrote = TRUE;

		}
		else
		{
			printf("Im the first player\n");
			if (write_to_file(*gameSession, current_player, 0, file_lock) != 0)
			{
				Write__Release__Mutex(file_lock);
				printf("failed to write line\n");
				return EXIT_CODE;
			}
			Write__Release__Mutex(file_lock);
			//wait for other thread
			while (second_thread_wrote == FALSE);
			if (read__line(*gameSession, current_player, other_player, current_player->line_size, file_lock) != 0)
			{
				printf("couldn't read line after sleep\n");
				return EXIT_CODE;
			}
		}
		print_player(current_player);
		print_player(other_player);
		if (send_invite(s_communication, server_massage, other_player->username) == EXIT_CODE)
		{
			printf("send invite failed!\n");
			return EXIT_CODE;
		}
		break;
	}
	return 0;
}

int Handle_setup(SOCKET s_communication, char* client_response,
	char* server_massage, Player* current_player)
{
	if (send_setup_request(s_communication, server_massage) == EXIT_CODE)
	{
		printf("send setupReq failed!\n");
		return EXIT_CODE;
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
	return 0;
}

int Handle_move(SOCKET s_communication, char* client_response,
	char* server_massage, Player* current_player, Player* other_player, HANDLE gameSession, Lock* file_lock)
{
	if (send_move_request(s_communication, server_massage) == EXIT_CODE)
	{
		printf("send setupReq failed!\n");
		return EXIT_CODE;
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

	int distance_to_move_write = 0, distance_to_move_read = current_player->line_size;
	if (current_player->is_first_player == FALSE)
	{
		distance_to_move_write = other_player->line_size;
		distance_to_move_read = 0;
	}
	printf("distance_to_move_write: %d\ndistance_to_move_read: %d\n",
		distance_to_move_write, distance_to_move_read);	
	if (write_to_file(gameSession, current_player, distance_to_move_write, file_lock) == -1)
	{
		printf("can't write to file\n");
		return -1;
	}
	num_of_writing++;
	while ((num_of_writing % 2) != 0);
	
	if (read__line(gameSession, current_player, other_player, distance_to_move_read, file_lock) == -1)
	{
		printf("can't read from file\n");
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
			if (guess[i] == setup[i])
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

DWORD WINAPI StartThread(LPVOID lp_params)
{
	ThreadParams threadInput = *(ThreadParams*)lp_params;
	Lock* file_lock = threadInput.file_lock;
	SOCKET s_communication = threadInput.ClientSocket;
	HANDLE gameSession = NULL;
	int error_code = 0, distance_to_move = 0; 
	char* server_massage = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	if (server_massage == NULL)
	{
		printf("cant malloc server massage\n");
		error_code = -1;
		goto Exit_No_Free;
	}
	char* client_response = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	if (client_response == NULL)
	{
		free(server_massage);
		printf("cant malloc client response\n");
		error_code = -1;
		goto Exit_No_Free;
	}
	
	Player* current_player = (Player*)calloc(1, sizeof(Player));
	if (current_player == NULL)
	{
		error_code = -1;
		goto Exit_No_Free;
	}
	Player* other_player = (Player*)calloc(1, sizeof(Player));	
	if (other_player == NULL)
	{
		error_code = -1;
		goto Exit_No_Free;
	}
	error_code = init_player(current_player);
	if (error_code != 0)
		goto ExitSeq;
	error_code = init_player(other_player);
	if (error_code != 0)
		goto ExitSeq;
	error_code = recive_client_request(s_communication, client_response, current_player);
	if(error_code != 0)
		goto ExitSeq;

	printf("Your Username: %s\n", current_player->username);

	error_code = send_approved(s_communication, server_massage);
	if (error_code == -1)
		goto ExitSeq;
	
	error_code = send_main_menu(s_communication, server_massage);
	if (error_code == -1)
		goto ExitSeq;
	
	switch (versus_or_disconnect(s_communication, &gameSession, client_response,
		server_massage, current_player, other_player, file_lock))
	{
	case EXIT_CODE:
		error_code = EXIT_CODE;
		goto ExitSeq;
	case CLIENT_DISCONNECT_ID:
		error_code = 0;
		goto ExitSeq;
	}	
	if (gameSession == INVALID_HANDLE_VALUE || gameSession == NULL)
	{
		printf("can't open file\n");
		goto ExitSeq;
	}
	printf("Yay\n");
	if (Handle_setup(s_communication, client_response, server_massage, current_player) == -1)
	{
		printf("can't handle setup\n");
		error_code = -1;
		goto ExitSeq;
	}
	while (1)
	{
		if (Handle_move(s_communication, client_response, server_massage,
			current_player, other_player, gameSession, file_lock) == -1)
		{
			printf("can't handle move\n");
			error_code = -1;
			goto ExitSeq;
		}
		/* check for BnC, send to client*/
		BnC_Data* data = GET__Bulls_And_Cows(current_player->setup, other_player->move);
		if (data == NULL)
		{
			printf("can't handle data\n");
			error_code = -1;
			goto ExitSeq;
		}

	}

ExitSeq:
	printf("entered ExitSeq\n");
	print_player(current_player);
	Write__Lock__Mutex(file_lock, 5000);
	if (current_player->is_first_player == TRUE)//CloseFile
	{
		if (DeleteFileA("GameSession.txt") == 0)
		{
			printf("error: %d", GetLastError());
		}
	}
	Write__Release__Mutex(file_lock);
	free(server_massage);
	free(client_response);
	free(current_player);
	free(other_player);
Exit_No_Free:	
	if (closesocket(s_communication) == SOCKET_ERROR)
	{
		printf("Failed to close MainSocketThread, error %ld. Ending program\n", WSAGetLastError());
	}	
	if (gameSession != NULL)
	{
		CloseHandle(gameSession);
	}
	printf("player quit\n");
	return error_code;
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
	return exit_code;
}
#endif