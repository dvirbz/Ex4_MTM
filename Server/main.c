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
#include "HardCodedData.h"
#include <stdbool.h>


/*Thread structer and global param*/
typedef struct {
	SOCKET ClientSocket;
	int ThreadNumber;
	Lock* file_lock;
	HANDLE readAndWriteEvent;
}ThreadParams;

HANDLE ThreadToBeClosed;
ThreadParams ThreadInputs[NUM_OF_THREADS];
ThreadParams ServerDeniedThreadInputs;

int can_I_delete_file = 0;

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
				return;
			}
		}
	}
	if (ThreadToBeClosed != NULL)
	{
		DWORD Res = WaitForSingleObject(ThreadToBeClosed, INFINITE);

		if (Res == WAIT_OBJECT_0)
		{
			closesocket(ServerDeniedThreadInputs.ClientSocket);
			CloseHandle(ThreadToBeClosed);
			ThreadToBeClosed = NULL;
		}
		else
		{
			printf("Waiting for thread failed. Ending program\n");
			return;
		}
	}
	return;
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
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 0);

			if (Res == WAIT_OBJECT_0) 
			{
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				break;
			}
		}
	}

	return Ind;
}

int Game(SOCKET s_communication, char* client_response, char* server_massage, Player * current_player,
	Player* other_player, HANDLE gameSession, Lock * file_lock, int threadNumber)
{
	int play_status = 0;
	while (play_status == 0)
	{
		if (opponentQuit[1 - threadNumber] == TRUE)
		{
			if (send_opponent_quit(s_communication, server_massage) == ERROR_CODE)
			{
				return ERROR_CODE;
			}
			return SERVER_OPPONENT_QUIT_ID;
		}
		int exit_code = 0;
		exit_code = Handle_move(s_communication, client_response, server_massage,
			current_player, other_player, gameSession, file_lock, threadNumber);
		if(exit_code != 0)
		{
			return exit_code;
		}
		/* check for BnC, send to client*/
		BnC_Data* data = GET__Bulls_And_Cows(current_player->setup, other_player->move);
		if (data == NULL)
		{
			return ERROR_CODE;
		}
		other_player->bulls = data->bulls;
		other_player->cows = data->cows;
		if (write_to_file(gameSession, current_player, other_player, file_lock) != 0)
		{
			printf("can't write to file\n");
			return ERROR_CODE;
		}
		if (read__line(gameSession, current_player, other_player, file_lock, threadNumber) != 0)
		{
			printf("can't read from file\n");
			return ERROR_CODE;
		}
		switch (GET__Game_Results(current_player, other_player))
		{
		case CURRENT_WON:
			send_game_won(s_communication, server_massage, other_player, current_player);
			play_status = -1;
			break;
		case OTHER_WON:
			send_game_won(s_communication, server_massage, other_player, other_player);
			play_status = -1;
			break;
		case DRAW:
			send_game_draw(s_communication, server_massage, other_player, current_player);
			play_status = -1;
			break;
		case CONTINUE:
			if (send_game_results(s_communication, server_massage, other_player, current_player) != 0)
			{
				return ERROR_CODE;
			}
			break;		
		}
	}
	return 0;
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
		printf("cant malloc client response\n");
		exit_code = -1;
		goto ExitFreeServer;
	}

	Player* current_player = (Player*)calloc(1, sizeof(Player));
	if (current_player == NULL)
	{
		printf("cant malloc current player\n");
		exit_code = -1;
		goto ExitFreeServerClient;
	}
	Player* other_player = (Player*)calloc(1, sizeof(Player));
	if (other_player == NULL)
	{
		printf("cant malloc other player\n");
		exit_code = -1;
		goto ExitFreeServerClientPlayer;
	}
	exit_code = init_playeres(current_player, other_player);
	if (exit_code != 0)
		goto ExitSeq;

	exit_code = Handle_Client_Request_Approved(s_communication, client_response, server_massage, current_player);
	if (exit_code != 0)
		goto ExitSeq;
	int pre_game_code = 0;
	while (TRUE)
	{
	MainMenu:
		exit_code = send_main_menu(s_communication, server_massage);
		if (exit_code == ERROR_CODE)
			goto ExitSeq;
		switch (versus_or_disconnect(s_communication, &gameSession, client_response,
			server_massage, current_player, other_player, file_lock, threadInput.ThreadNumber))
		{
		case ERROR_CODE:
			exit_code = ERROR_CODE;
			goto ResetGame;
		case CLIENT_DISCONNECT_ID:
			exit_code = CLIENT_DISCONNECT_ID;
			goto ExitSeq;
			break;
		case SERVER_NO_OPPONENTS_ID:
			exit_code = 0;
			goto ResetGame;
			break;
		case SHUTDOWN:
			exit_code = ERROR_CODE;
			goto ResetGame;
		}

		exit_code = Handle_setup(s_communication, client_response, server_massage, current_player,
			other_player, gameSession, file_lock, opponentQuit[1 - threadInput.ThreadNumber]);
		if (exit_code != 0)
		{
			printf("can't handle setup, exitcode = %d\n", exit_code);
			goto ResetGame;
		}
		exit_code = Game(s_communication, client_response, server_massage, current_player,
			other_player, gameSession, file_lock, threadInput.ThreadNumber);
	ResetGame:
		if (exit_code != 0)
		{
			opponentQuit[threadInput.ThreadNumber] = TRUE;
			num_of_writing++;
		}
		if (gameSession != NULL)
		{
			CloseHandle(gameSession);
			can_I_delete_file++;
		}
		gameSession = NULL;
		if (current_player->is_first_player == TRUE)//CloseFile
		{
			while (can_I_delete_file % 2 != 0 );
			if (can_I_delete_file % 2 != 0)
			{
				can_I_delete_file++;
			}
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
		if (exit_code == ERROR_CODE || exit_code == SHUTDOWN)
		{
			goto ExitSeq;
		}
		if (exit_code == SERVER_OPPONENT_QUIT_ID)
		{
			exit_code = 0;
			goto MainMenu;
		}
	}

ExitSeq:	
	if (gameSession != NULL)
	{
		CloseHandle(gameSession);		
		can_I_delete_file++;
	}	
	opponentQuit[threadInput.ThreadNumber] = TRUE;
	while (can_I_delete_file % 2 != 0 && opponentQuit[1 - threadInput.ThreadNumber] == FALSE);
	free(other_player);
ExitFreeServerClientPlayer:
	free(current_player);
	shutdown(s_communication, SD_SEND);
	while (exit_code != SHUTDOWN)
	{
		exit_code = Recv_Socket(s_communication, client_response, FIFTEEN_SEC);
		if (exit_code != 0)
		{
			break;
		}
	}
ExitFreeServerClient:
	free(client_response);
ExitFreeServer:
	free(server_massage);
Exit_No_Free:
	if (closesocket(s_communication) == SOCKET_ERROR)
	{
		printf("Failed to close MainSocketThread, error %ld. Ending program\n", WSAGetLastError());
	}
	return exit_code;
}

DWORD WINAPI ServerDeniedThread(LPVOID lp_params)
{
	ThreadParams threadInput = *(ThreadParams*)lp_params;
	SOCKET s_communication = threadInput.ClientSocket;
	int exit_code = 0;

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
		printf("cant malloc client response\n");
		exit_code = -1;
		goto ExitFreeServer;
	}

	Player* current_player = (Player*)calloc(1, sizeof(Player));
	if (current_player == NULL)
	{
		printf("cant malloc current player\n");
		exit_code = -1;
		goto ExitFreeServerClient;
	}

	exit_code = Handle_Client_Request_Denied(s_communication, client_response, server_massage, current_player);

ExitFreeServerClient:
	free(client_response);
ExitFreeServer:
	free(server_massage);
Exit_No_Free:
	if (closesocket(s_communication) == SOCKET_ERROR)
	{
		printf("Failed to close MainSocketThread, error %ld. Ending program\n", WSAGetLastError());
	}
	return exit_code;
}

DWORD WINAPI ExitMainThread(char* exit_string)
{
	while (strcmp(exit_string, "exit") != 0)
	{
		scanf_s("%s", exit_string, EXIT_STRING_LEN);
	}
	return 0;
}

int main(int argc, char* argv[])
{
	assert(argc == ARGUMENT_NUMBER_SERVER);

	int Ind, exit_code = 0, portNumber;
	int max_sd, activity;
	SOCKET s_server = INVALID_SOCKET;
	SOCKADDR_IN service;
	fd_set readfds;
	Lock* file_lock = New__Lock(NUM_OF_THREADS);
	if (file_lock == NULL)
	{
		return ERROR_CODE;
	}
	readAndWriteEvent = CreateEvent(
		NULL,               
		TRUE,               
		FALSE,              
		TEXT("WriteEvent")  
	);
	first_want_to_invite = CreateEvent(
		NULL,               
		TRUE,               
		FALSE,              
		TEXT("FirstInvite")  
	);
	second_want_to_invite = CreateEvent(
		NULL,               
		TRUE,               
		FALSE,              
		TEXT("SecondInvite")  
	);
	if (readAndWriteEvent == NULL || first_want_to_invite == NULL || second_want_to_invite == NULL)
	{
		return ERROR_CODE;
	}
	portNumber = (int)strtol(argv[PORT_NUMBER_INDEX], NULL, DECIMAL_BASE);
	if (portNumber == 0)
	{
		Destroy__lock(file_lock);
		return ERROR_CODE;
	}
	assert(InitializeWSA() == 0);
	s_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s_server == INVALID_SOCKET)
	{
		printf("Error at socket( ): %ld\n", WSAGetLastError());
		exit_code = -1;
		goto ServerCleanUp;
	}
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = INADDR_ANY;
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
	char* exit_message = (char*)calloc(EXIT_STRING_LEN, sizeof(char));
	if (exit_message == NULL)
	{
		printf("MEM allocation failed\n");
		exit_code = ERROR_CODE;
		goto CloseSocket;
	}
	if (snprintf(exit_message, EXIT_STRING_LEN, "game") == 0)
	{
		printf("snprintf failed\n");
		exit_code = ERROR_CODE;
		goto CloseSocket;
	}
	HANDLE thread_exit = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)ExitMainThread,
		exit_message,
		0,
		NULL
	);
	if (thread_exit == NULL)
	{
		printf("thread creation failed, last error: %d\n", GetLastError());
		exit_code = ERROR_CODE;
		goto CloseSocket;
	}
	DWORD exit_retval = WaitForSingleObject(thread_exit, 0);
	TIMEVAL wait_time;
	wait_time.tv_sec = 1;

	while (exit_retval != WAIT_OBJECT_0)
	{
		FD_ZERO(&readfds);
		FD_SET(s_server, &readfds);
		max_sd = s_server;
		activity = select(max_sd + 1, &readfds, NULL, NULL, &wait_time);

		if ((activity < 0) && (errno != EINTR))
		{
			printf("select failed\n");
		}
		if (activity > 0)
		{

			if (FD_ISSET(s_server, &readfds))
			{
				SOCKET AcceptSocket = accept(s_server, NULL, NULL);
				if (AcceptSocket == INVALID_SOCKET)
				{
					printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
					exit_code = -1;
					goto CloseSocket;
				}
				Ind = FindFirstUnusedThreadSlot();

				if (Ind == NUM_OF_THREADS) //two players already play
				{
					ServerDeniedThreadInputs.ClientSocket = AcceptSocket;
					ThreadToBeClosed = CreateThread(NULL,
						0,
						(LPTHREAD_START_ROUTINE)ServerDeniedThread,
						&(ServerDeniedThreadInputs),
						0,
						NULL);
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
		}
		exit_retval = WaitForSingleObject(thread_exit, 0);
		if (exit_retval == WAIT_OBJECT_0)
		{
			break;
		}

	}

	CleanupWorkerThreads();
CloseSocket:
	if (closesocket(s_server) == SOCKET_ERROR)
	{
		printf("Failed to close MainSocketServer, error %ld. Ending program\n", WSAGetLastError());
	}
ServerCleanUp:
	if (readAndWriteEvent != NULL)
	{
		CloseHandle(readAndWriteEvent);
	}
	Destroy__lock(file_lock);
	if (WSACleanup() == SOCKET_ERROR)
	{
		printf("Failed to close WinsocketServer, error %ld. Ending program.\n", WSAGetLastError());
	}
	return exit_code;
}
#endif