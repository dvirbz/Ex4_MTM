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

typedef struct ThreadParams {
	SOCKET ClientSocket;
	int ThreadNumber;
	HANDLE lock_file;
}ThreadParams;

typedef struct Player
{
	char username[MAX_USERNAME_LEN];
	char first_letter;
	char second_letter;
	char third_letter;
	char forth_letter;
	int line_size;
}Player;

HANDLE ThreadHandles[NUM_OF_THREADS];
ThreadParams ThreadInputs[NUM_OF_THREADS];

void print_player(Player* other_player)
{
	printf("player name is: %s\n", other_player->username);
	printf("player letters are: %c%c%c%c\n", other_player->first_letter, other_player->second_letter, other_player->third_letter, other_player->forth_letter);
	printf("player line size: %d\n", other_player->line_size);
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

static void CleanupWorkerThreads()
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

void init_player(Player* player, char* username)
{
	printf("initializing player...\n");
	player->first_letter = '0';
	player->second_letter = '0';
	player->third_letter = '0';
	player->forth_letter = '0';
	snprintf(player->username, strlen(username) + 1, "%s", username);
	player->line_size = strlen(username) + 7;
	printf("usernameinit: %s\n", player->username);
	printf("succses!\n");
}

int recive_client_request(SOCKET s_communication, char* client_response, char* username)
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
	printf("got here2\n");
	BnC_Data* data = GET__BnC_Data(client_response);
	if (data == NULL)
	{
		return -1;
	}
	if (snprintf(username, MAX_USERNAME_LEN, "%s", data->username) == 0)
	{
		free(data);
		return -1;
	}
	free(data);
	printf("rcr username: %s\n", username);
	return 0;
}

int send_server_approved(SOCKET s_communication, char* server_massage, int server_message_size)
{
	if (GET__Server_Approved_PRO(server_massage) == -1)
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

int send_invite(SOCKET s_communication, char* server_massage, int server_message_size, char* other_username)
{
	printf("sending invite...\n");
	if (GET__Server_Invite_PRO(server_massage, other_username) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, server_message_size) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	printf("invite sent?\n");
	return 0;
}

int send_setup_request(SOCKET s_communication, char* server_massage, int server_message_size)
{
	printf("sending setup...\n");
	if (GET__Server_Setup_Request_PRO(server_massage) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, server_message_size) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	printf("setup sent!\n");
	return 0;
}

int write_to_file(HANDLE gameSession, Player* player)
{
	char line_to_write[MAX_LINE_LEN];
	int character_count = snprintf(line_to_write, player->line_size, "%s:%c%c%c%c\r\n\0",
		player->username, player->first_letter, player->second_letter, player->third_letter, player->forth_letter);
	if (character_count == 0)
	{
		printf("couldn't write string\n");
		free(line_to_write);
		return -1;
	}
	if (WriteFile(gameSession, line_to_write, player->line_size, NULL, NULL) == 0)
	{
		printf("failed to write to file\n");
		free(line_to_write);
		return -1;
	}
	return 0;
}

int read_first_line(HANDLE gameSession, Player* current_player, Player* other_player)
{
	char first_line[MAX_LINE_LEN];
	if (ReadFile(gameSession, first_line, MAX_LINE_LEN, NULL, NULL) == 0)
	{
		printf("couldn't read file\n");
		return -1;
	}
	int i = 0;
	while (first_line[i] != ':')
	{
		other_player->username[i] = first_line[i];
		i++;
	}
	other_player->first_letter = first_line[i + 1];
	other_player->second_letter = first_line[i + 2];
	other_player->third_letter = first_line[i + 3];
	other_player->forth_letter = first_line[i + 4];
	other_player->line_size = i + 7;
	return 0;
}

int read_second_line(HANDLE gameSession, Player* current_player, Player* other_player)
{
	if (SetFilePointer(gameSession, current_player->line_size, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		printf("couldn't set file pointer\n");
		return -1;
	}
	char second_line[MAX_LINE_LEN];
	if (ReadFile(gameSession, second_line, MAX_LINE_LEN, NULL, NULL) == 0)
	{
		printf("couldn't read file\n");
		return -1;
	}
	int i = 0;
	while (second_line[i] != ':')
	{
		other_player->username[i] = second_line[i];
		i++;
	}
	other_player->first_letter = second_line[i + 1];
	other_player->second_letter = second_line[i + 2];
	other_player->third_letter = second_line[i + 3];
	other_player->forth_letter = second_line[i + 4];
	other_player->line_size = i + 7;
	return 0;
}

int versus_or_disconnect(SOCKET s_communication, HANDLE* gameSession, char* client_response,
	char* server_massage, int server_message_size, int* error_code, Player* current_player, Player* other_player, BOOL* is_first_player)
{
	if (Recv_Socket(s_communication, client_response) == -1)
	{
		printf("Recv Failed\n");
		*(error_code) = -1;
		return EXIT_CODE;
	}
	switch (GET__Client_Response_ID(client_response))
	{
	case CLIENT_DISCONNECT_ID:
	{
		return EXIT_CODE;
		break;
	}
	case CLIENT_VERSUS_ID:
	{
		//MUTEX!
		gameSession = CreateFileA("GameSession.txt", GENERIC_ALL, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (gameSession == INVALID_HANDLE_VALUE)
		{	//ready to play MUTEX!!
			gameSession = CreateFileA("GameSession.txt", GENERIC_ALL, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (gameSession == INVALID_HANDLE_VALUE)
			{
				printf("can't open file\n");
				*(error_code) = -1;
				return EXIT_CODE;
			}
			is_first_player = FALSE;
			if (read_first_line(gameSession, current_player, other_player) != 0)
			{
				*(error_code) = -1;
				return EXIT_CODE;
			}
			if (SetFilePointer(gameSession, other_player->line_size, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
			{
				printf("couldn't set file pointer\n");
				return -1;
			}
			if (write_to_file(gameSession, current_player) != 0)
			{
				printf("failed to write line");
				*(error_code) = -1;
				return EXIT_CODE;
			}
			print_player(current_player);
			print_player(other_player);
			CloseHandle(gameSession);
		}
		else
		{
			*is_first_player = TRUE;
			if (write_to_file(gameSession, current_player) != 0)
			{
				printf("failed to write line");
				*(error_code) = -1;
				return EXIT_CODE;
			}
			CloseHandle(gameSession);
			//wait for other thread
			Sleep(40000);
			gameSession = CreateFileA("GameSession.txt", GENERIC_ALL, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (read_second_line(gameSession, current_player, other_player) != 0)
			{
				*(error_code) = -1;
				return EXIT_CODE;
			}
			print_player(current_player);
			print_player(other_player);
		}
		
		server_message_size = strlen(SERVER_INVITE) + strlen(PARTITION_MASSAGE_PARAMETERS) +
			strlen(other_player->username) + strlen(END_PROTOCOL);
		*(error_code) = send_invite(s_communication, server_massage, server_message_size, other_player->username);
		if (*(error_code) == -1)
		{
			printf("send invite failed!\n");
			return EXIT_CODE;
		}
		
		//send SetupReq:
		server_message_size = strlen(SERVER_SETUP_REQUEST) + strlen(END_PROTOCOL);
		*(error_code) = send_setup_request(s_communication, server_massage, server_message_size);
		if (*(error_code) == -1)
		{
			printf("send setupReq failed!\n");
			return EXIT_CODE;
		}
		break;
	}
	}
	return 0;
}

DWORD WINAPI StartThread(LPVOID lp_params)
{
	ThreadParams threadInput = *(ThreadParams*)lp_params;
	int error_code = 0;
	int server_message_size;
	char username[MAX_USERNAME_LEN];
	char* server_massage = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	char* client_response = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	SOCKET s_communication = threadInput.ClientSocket;
	HANDLE gameSession = NULL;
	Player* current_player = (Player*)calloc(1, sizeof(Player));
	Player* other_player = (Player*)calloc(1, sizeof(Player));;
	BOOL is_first_player = FALSE;

	error_code = recive_client_request(s_communication, client_response, username);
	if(error_code != 0)
		goto ExitSeq;
	printf("Your Username: %s\n", username);
	init_player(current_player, username);

	server_message_size = strlen(SERVER_APPROVED) + strlen(END_PROTOCOL);
	error_code = send_server_approved(s_communication, server_massage, server_message_size);
	if (error_code == -1)
		goto ExitSeq;

	server_message_size = strlen(SERVER_MAIN_MENU) + strlen(END_PROTOCOL);
	error_code = send_main_menu(s_communication, server_massage, server_message_size);
	if (error_code == -1)
		goto ExitSeq;
	
	if (versus_or_disconnect(s_communication, gameSession, client_response,
		server_massage, server_message_size, &(error_code), current_player, other_player, &(is_first_player)) != 0)
		goto ExitSeq;
	
	if (Recv_Socket(s_communication, client_response) == -1)
	{
		printf("Recv Failed\n");
		error_code = -1;
		goto ExitSeq;
	}
	return 0;

ExitSeq:
	if (closesocket(s_communication) == SOCKET_ERROR)
	{
		printf("Failed to close MainSocketThread, error %ld. Ending program\n", WSAGetLastError());
	}
	if (WSACleanup() == SOCKET_ERROR)
	{
		printf("Failed to close WinsocketThread, error %ld. Ending program.\n", WSAGetLastError());
	}
	free(server_massage);
	free(client_response);
	free(current_player);
	free(other_player);
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

	int Ind;
	SOCKET s_server = INVALID_SOCKET;
	int portNumber;
	unsigned long address;
	SOCKADDR_IN service;

	portNumber = stringToPositiveInt(argv[PORT_NUMBER_INDEX]);
	printf("your port number is: %d\n", portNumber);

	assert(InitializeWSA() == 0);
	s_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s_server == INVALID_SOCKET)
	{
		printf("Error at socket( ): %ld\n", WSAGetLastError());
		goto ServerCleanUp;
	}

	address = inet_addr(LOCAL_HOST_ADDRESS);
	if (address == INADDR_NONE)
	{
		printf("The string \"%s\" cannot be converted into an ip address. ending program.\n", LOCAL_HOST_ADDRESS);
		assert(FALSE);
	}

	service.sin_family = AF_INET;
	service.sin_addr.s_addr = address;
	service.sin_port = htons(portNumber);

	//Bind Socket
	if (bind(s_server, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR)
	{
		printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
		assert(FALSE);
	}

	//Listen on Socket
	if (listen(s_server, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		assert(FALSE);
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
			goto CleanThreads;
		}

		printf("Client Connected.\n");

		Ind = FindFirstUnusedThreadSlot();

		if (Ind == NUM_OF_THREADS) //two players already play
		{
			printf("No slots available for client, dropping the connection.\n");
			closesocket(AcceptSocket); //Server Denied Protocol
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
	return 0;

CleanThreads:
	CleanupWorkerThreads();
	if (closesocket(s_server) == SOCKET_ERROR)
	{
		printf("Failed to close MainSocketServer, error %ld. Ending program\n", WSAGetLastError());
	}
ServerCleanUp:
	if (WSACleanup() == SOCKET_ERROR)
	{
		printf("Failed to close WinsocketServer, error %ld. Ending program.\n", WSAGetLastError());
	}
}
#endif