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

typedef struct {
	SOCKET ClientSocket;
	int ThreadNumber;
	HANDLE lock_file;
}ThreadParams;

typedef struct
{
	char username[MAX_USERNAME_LEN];
	char setup[NUM_DIGITIS_GUESS];
	char move[NUM_DIGITIS_GUESS];
	int line_size;
	BOOL is_first_player;
}Player;

HANDLE ThreadHandles[NUM_OF_THREADS];
ThreadParams ThreadInputs[NUM_OF_THREADS];

void print_player(Player* player)
{
	printf("player name is: %s\n", player->username);
	printf("player setup is: %s\n", player->setup);
	printf("player move is: %s\n", player->move);
	printf("player line size: %d\n", player->line_size);
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

int init_player(Player* player, char* username)
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
	player->line_size = strlen(username) + 7;
	player->is_first_player = FALSE;
	printf("usernameinit: %s\n", player->username);
	printf("succses!\n");
	return 0;
}

int update_player(Player* player, char* player_info)
{
	char* username, * move_or_setup;
	char* next = NULL;
	username = strtok_s(player_info, ":\n", &next);
	move_or_setup = strtok_s(NULL, ":\n", &next);	
	if (username == NULL || move_or_setup == NULL)
	{
		return -1;
	}	
	if (strcmp(player->username, username) != 0)
	{
		if (snprintf(player->setup, NUM_DIGITIS_GUESS, "%s", move_or_setup) == 0)
		{
			return -1;
		}
		if (snprintf(player->username, MAX_USERNAME_LEN, "%s", username) == 0)
		{
			return -1;
		}
	}
	else
	{
		if (snprintf(player->move, NUM_DIGITIS_GUESS, "%s", move_or_setup) == 0)
		{
			return -1;
		}
	}
	player->line_size = strlen(username) + strlen(move_or_setup) + strlen(PARTITION_MASSAGE_PARAMETERS) + 2;
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

int send_main_menu(SOCKET s_communication, char* server_massage, int server_message_size)
{
	if (GET__Server_Main_Menu_PRO(server_massage) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, server_message_size) == -1)
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
	printf("sending setup...\n");
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
	printf("setup sent!\n");
	return 0;
}

int write_to_file(HANDLE gameSession, Player* player, int distance_to_move)
{
	if (SetFilePointer(gameSession, distance_to_move, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		printf("couldn't set file pointer\n");
		return -1;
	}
	char line_to_write[MAX_LINE_LEN];
	int character_count = snprintf(line_to_write, MAX_LINE_LEN, "%s:%s\r\n\0",
		player->username, player->setup);
	if (character_count == 0)
	{
		printf("couldn't write string\n");		
		return -1;
	}
	if (WriteFile(gameSession, line_to_write, strlen(line_to_write), NULL, NULL) == 0)
	{
		printf("failed to write to file\n");		
		return -1;
	}
	return 0;
}

int read__line(HANDLE gameSession, Player* current_player, Player* other_player,int distance_to_move)
{
	if (SetFilePointer(gameSession, distance_to_move, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		printf("couldn't set file pointer\n");
		return -1;
	}
	char line[MAX_LINE_LEN];
	if (ReadFile(gameSession, line, MAX_LINE_LEN, NULL, NULL) == 0)
	{
		printf("couldn't read file\n");
		return -1;
	}	
	if (update_player(other_player,line) == -1)
	{
		return -1;
	}
	return 0;
}

int versus_or_disconnect(SOCKET s_communication, HANDLE* gameSession, char* client_response,
	char* server_massage, Player* current_player, Player* other_player)
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
		//MUTEX!		
		current_player->is_first_player = TRUE;
		gameSession = CreateFileA("GameSession.txt", GENERIC_ALL, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (gameSession == INVALID_HANDLE_VALUE)
		{	//ready to play MUTEX!!
			current_player->is_first_player = FALSE;
		}
		if (current_player->is_first_player == FALSE)
		{
			gameSession = CreateFileA("GameSession.txt", GENERIC_ALL, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (gameSession == INVALID_HANDLE_VALUE)
			{
				printf("can't open file\n");
				return EXIT_CODE;
			}
			if (read__line(gameSession, current_player, other_player, 0) != 0)
			{
				printf("failed to read line");
				return EXIT_CODE;
			}
			if (write_to_file(gameSession, current_player, other_player->line_size) != 0)
			{
				printf("failed to write line");
				return EXIT_CODE;
			}

			CloseHandle(gameSession);
		}
		else
		{
			print_player(current_player);
			if (write_to_file(gameSession, current_player, 0) != 0)
			{
				printf("failed to write line");
				return EXIT_CODE;
			}
			CloseHandle(gameSession);
			//wait for other thread
			Sleep(40000);
			gameSession = CreateFileA("GameSession.txt", GENERIC_ALL, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (read__line(gameSession, current_player, other_player, current_player->line_size) != 0)
			{
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
		//send SetupReq:		
		if (send_setup_request(s_communication, server_massage) == EXIT_CODE)
		{
			printf("send setupReq failed!\n");
			return EXIT_CODE;
		}
		break;

	}
	return 0;
}

DWORD WINAPI StartThread(LPVOID lp_params)
{
	ThreadParams threadInput = *(ThreadParams*)lp_params;
	int error_code = 0;
	int server_message_size;
	char* server_massage = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	char* client_response = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	SOCKET s_communication = threadInput.ClientSocket;
	HANDLE gameSession = NULL;
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
	error_code = recive_client_request(s_communication, client_response, current_player);
	if(error_code != 0)
		goto ExitSeq;

	printf("Your Username: %s\n", current_player->username);
	error_code = init_player(current_player, current_player->username);
	if (error_code != 0)
		goto ExitSeq;

	server_message_size = strlen(SERVER_APPROVED) + strlen(END_PROTOCOL);
	error_code = send_approved(s_communication, server_massage);
	if (error_code == -1)
		goto ExitSeq;

	server_message_size = strlen(SERVER_MAIN_MENU) + strlen(END_PROTOCOL);
	error_code = send_main_menu(s_communication, server_massage, server_message_size);
	if (error_code == -1)
		goto ExitSeq;
	
	switch (versus_or_disconnect(s_communication, gameSession, client_response,
		server_massage, current_player, other_player))
	{
	case EXIT_CODE:
		error_code = EXIT_CODE;
		goto ExitSeq;
	case CLIENT_DISCONNECT_ID:
		error_code = 0;
		goto ExitSeq;
	}	
	
	if (Recv_Socket(s_communication, client_response) == -1)
	{
		printf("Recv Failed\n");
		error_code = -1;
		goto ExitSeq;
	}	

ExitSeq:
	if (current_player->is_first_player == TRUE);//CloseFile
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
	Lock* file_lock = (Lock*)calloc(1, sizeof(Lock));
	if (file_lock == NULL)
	{
		return -1;
	}
	portNumber = (int)strtol(argv[PORT_NUMBER_INDEX], NULL, DECIMAL_BASE);
	if (portNumber == 0)
	{
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

	address = inet_addr(LOCAL_HOST_ADDRESS);
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
CleanThreads:
	CleanupWorkerThreads();
CloseSocket:
	if (closesocket(s_server) == SOCKET_ERROR)
	{
		printf("Failed to close MainSocketServer, error %ld. Ending program\n", WSAGetLastError());
	}
ServerCleanUp:
	if (WSACleanup() == SOCKET_ERROR)
	{
		printf("Failed to close WinsocketServer, error %ld. Ending program.\n", WSAGetLastError());
	}
	return exit_code;
}
#endif