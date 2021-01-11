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


#define NUM_OF_THREADS 2
#define ARGUMENT_NUMBER_SERVER 2
#define PORT_NUMBER_INDEX 1
#define LOCAL_HOST_ADDRESS "127.0.0.1"

/*SOCKET socket(
	int af, // address family specification
	int type, // type specification for the new socket
	int protocol // protocol to be used with the socket
	//that is specific to the indicated address family
);*/

typedef struct ThreadParams {
	SOCKET* ClientSocket;
}ThreadParams;

HANDLE ThreadHandles[NUM_OF_THREADS];
ThreadParams ThreadInputs[NUM_OF_THREADS];

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
	for (i; i < strlen(portNumberStr); i++)
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

DWORD StartThread(ThreadParams threadInput)
{
	int error_code = 0;
	int server_message_size;
	char* server_massage = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	char* client_response = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	char* username = (char*)calloc(MAX_NAME_LEN, sizeof(char));
	SOCKET s_communication = *(threadInput.ClientSocket);
	//wait for client to send client_request
	printf("opened thread and trying to recive now\n");
	if (Recv_Socket(s_communication, client_response) == -1)
	{
		printf("Recv Failed\n");
		error_code = -1;
		goto ExitSeq;
	}
	printf("trying to get username\n");
	username = GET_username_from_massage(client_response);
	printf("your username is: %s\n", username);
	//Open file to write username and get other username / create file

	int response_id = GET__Response_ID(client_response);
	//Server Approved send
	server_message_size = strlen(SERVER_APPROVED) + strlen(END_PROTOCOL);
	if (GET__Server_Approved_PRO(server_massage) == -1)
	{
		printf("Protocol failed\n");
		error_code = -1;
		goto ExitSeq;
	}
	if (Send_Socket(s_communication, server_massage, server_message_size) == -1)
	{
		printf("Send Failed\n");
		error_code = -1;
		goto ExitSeq;
	}
	//Server main menu
	server_message_size = strlen(SERVER_MAIN_MENU) + strlen(END_PROTOCOL);
	if (GET_Server_Main_Menu_PRO(server_massage) == -1)
	{
		printf("Protocol failed\n");
		error_code = -1;
		goto ExitSeq;
	}
	if (Send_Socket(s_communication, server_massage, server_message_size) == -1)
	{
		printf("Send Failed\n");
		error_code = -1;
		goto ExitSeq;
	}

	//if recv: DISCONNECT_CLIENT -> close socket and thread.
	//if recv: VERSUS_CLIENT -> send: INVITE_SERVER
	if (Recv_Socket(s_communication, client_response) == -1)
	{
		printf("Recv Failed\n");
		error_code = -1;
		goto ExitSeq;
	}
	response_id = GET__Response_ID(client_response);//change
	
ExitSeq:
	if (closesocket(s_communication) == SOCKET_ERROR)
	{
		printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
	}
	if (WSACleanup() == SOCKET_ERROR)
	{
		printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
	}
	free(server_massage);
	free(client_response);
	free(username);
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

	for(Ind = 0; Ind< NUM_OF_THREADS; Ind ++)
	{
		ThreadHandles[Ind] = NULL;
	}
	printf("Waiting for a client to connect...\n");

	while(TRUE)
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

CloseSocket:
	if (closesocket(s_server) == SOCKET_ERROR)
		printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
ServerCleanUp:
	if (WSACleanup() == SOCKET_ERROR)
	{
		printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
	}
}
#endif