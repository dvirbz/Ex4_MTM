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
#include "Player.h"
#include "File_Func.h"
#include "Server_send_recv.h"
#include <stdbool.h>

#define ARGUMENT_NUMBER_SERVER 2
#define PORT_NUMBER_INDEX 1
#define LOCAL_HOST_ADDRESS "127.0.0.1"
#define FILE_GAME_SESSION "GameSession.txt"

/*Thread structer and global param*/
typedef struct {
	SOCKET ClientSocket;
	int ThreadNumber;
	Lock* file_lock;
	HANDLE readAndWriteEvent;
}ThreadParams;

ThreadParams ThreadInputs[NUM_OF_THREADS];
int can_I_close_file = 0;

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

/*DWORD WINAPI StartThread(LPVOID lp_params)
{
	ThreadParams threadInput = *(ThreadParams*)lp_params;
	Lock* file_lock = threadInput.file_lock;
	SOCKET s_communication = threadInput.ClientSocket;
	HANDLE readAndWriteEvent = threadInput.readAndWriteEvent;
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
	while (TRUE)
	{		
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
			break;
		}
		/*if (gameSession == INVALID_HANDLE_VALUE || gameSession == NULL)
		{
			printf("can't open file\n");
			goto ExitSeq;
		}
		printf("Yay\n");
		if (Handle_setup(s_communication, client_response, server_massage, current_player,
			other_player, gameSession, file_lock) == -1)
		{
			printf("can't handle setup\n");
			error_code = -1;
			goto ExitSeq;
		}
		print_player(other_player);
		if (snprintf(other_player->setup, NUM_DIGITIS_GUESS, "%s", other_player->move) == 0)
		{
			printf("snprintf failed\n");
			error_code = -1;
			goto ExitSeq;
		}
		print_player(other_player);
		int play_status = 0;
		while (play_status == 0)
		{
			if (Handle_move(s_communication, client_response, server_massage,
				current_player, other_player, gameSession, file_lock) == -1)
			{
				printf("can't handle move\n");
				error_code = -1;
				goto ExitSeq;
			}

			/* check for BnC, send to client
			BnC_Data* data = GET__Bulls_And_Cows(current_player->setup, other_player->move);
			if (data == NULL)
			{
				printf("can't handle data\n");
				error_code = -1;
				goto ExitSeq;
			}
			other_player->bulls = data->bulls;
			other_player->cows = data->cows;
			if (write_to_file(gameSession, current_player, other_player, file_lock) != 0)
			{
				printf("can't write to file\n");
				error_code = -1;
				goto ExitSeq;
			}
			if (read__line(gameSession, current_player, other_player, file_lock) != 0)
			{
				printf("can't read from file\n");
				error_code = -1;
				goto ExitSeq;
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
					error_code = -1;
					goto ExitSeq;
				}
				break;
			}
		}
		/*
		if (gameSession != NULL)
		{
			printf("closed file");
			CloseHandle(gameSession);
		}
		if (current_player->is_first_player == TRUE)//CloseFile
		{
			gameSession = CreateFileA("GameSession.txt", GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (DeleteFileA("GameSession.txt") == 0)
			{
				printf("error: %d\n", GetLastError());
			}
		}
		ResetEvent(readAndWriteEvent);
		num_of_writing = -1;
		if (gameSession != NULL)
		{
			CloseHandle(gameSession);
			can_I_close_file++;
		}
		if (current_player->is_first_player == TRUE)//CloseFile
		{
			while (can_I_close_file % 2 != 0);
			if (DeleteFileA("GameSession.txt") == 0)
			{
				printf("error: %d\n", GetLastError());
			}
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
	return error_code;
}*/

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
	if (exit_code != 0)
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
			num_of_writing = -1;
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
	if (gameSession != NULL)
	{
		CloseHandle(gameSession);
		can_I_close_file++;
	}
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
	if (readAndWriteEvent == NULL || first_want_to_invite == NULL || second_want_to_invite == NULL)
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