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
#define MAX_USERNAME_LEN 20
#define MAX_IP_LEN 15
#define IP_ARG_NUM 1
#define PORT_ARG_NUM 2
#define USER_ARG_NUM 3
#define DECIMAL_BASE 10

enum Player_Status
{
	CONNECT,
	EXIT,
	PLAY
};

int InitializeWSA();
int Connect_Socket(SOCKET s, SOCKADDR* cs);

void Connect_MENU(char server_ip[], int server_port);
void Denied_MENU(char server_ip[], int server_port);
void Choose_Next_Connect();
void Choose_Next_Play();
int GET__Connect_Player_Decision();
int GET__Play_Player_Decision();
int Handle_Client_Request(SOCKET s_client, char* username,char * client_request);
int GET__Server_Response(SOCKET s_client, char* server_response);

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

	/* Connecting */
	int connect_status = CONNECT, play_status = TRUE;
	char* client_message = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	char* server_response = (char*)calloc(MAX_PRO_LEN, sizeof(char));

	while (connect_status == CONNECT)
	{
		if (Connect_Socket(s_client, (SOCKADDR*)&clientService) != 0)
		{
			Connect_MENU(server_ip_address, server_port_number);
			connect_status = GET__Connect_Player_Decision();
			if (connect_status != CONNECT)
			{
				exit_code = -1;
				goto ExitSeq;
			}
			else
			{
				continue;
			}
		}
		printf("Connected to server on %s:%d\n", server_ip_address, server_port_number);
	/*===============================================================================*/
		/* Client Request */
		if (Handle_Client_Request(s_client, username,client_message) == -1)
		{
			exit_code = -1;
			goto ExitSeq;
		}		
		/* Get Server Response */		
		switch (GET__Server_Response(s_client, server_response))
		{
		case SERVER_DENIED_ID:
			Denied_MENU(server_ip_address, server_port_number);
			connect_status = GET__Connect_Player_Decision();
			if (connect_status != CONNECT)
			{
				exit_code = -1;
				goto ExitSeq;
			}					
			if (closesocket(s_client) == SOCKET_ERROR)
			{
				exit_code = -1;
				goto ExitSeq;
			}
			s_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (s_client == INVALID_SOCKET)
			{
				exit_code = -1;
				goto Exit_WO_CLOSE;
			}
			break;
		case SERVER_APPROVED_ID:
			connect_status = PLAY;
			break;
		default:printf("pro: %s size: %d\n", server_response, sizeof(server_response));
			exit_code = -1;
			goto ExitSeq;
			break;
		}
	}
	play_status = connect_status;
	while (play_status == PLAY)
	{
		Choose_Next_Play();
		play_status = GET__Play_Player_Decision();
		if (play_status != CONNECT)
		{
			exit_code = -1;
			goto ExitSeq;
		}
		else
		{
			continue;
		}
		if (play_status != PLAY)
		{
			exit_code = -1;
			goto ExitSeq;
		}
	}
	printf("All will be well %s, PortNumber: %d\n", username, server_port_number);

ExitSeq:
	free(client_message);
	free(server_response);
	if (closesocket(s_client) == SOCKET_ERROR)
	{
		printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
	}
Exit_WO_CLOSE:
	if (WSACleanup() == SOCKET_ERROR)
	{
		printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
	}
	return exit_code;
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
int GET__Connect_Player_Decision()
{
	int connect_status = 0;
	if (scanf_s("%d", &connect_status) == 0)
	{
		return -1;
	}
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
	switch (play_status)
	{
	case 1:return CONNECT;
		break;
	case 2:return EXIT;
		break;
	}
	return -1;
}
int Connect_to_Server()
{
	return 0;
}
int Handle_Client_Request(SOCKET s_client, char* username, char * client_request)
{
	int client_req_size = strlen(CLIENT_REQUEST) + strlen(END_PROTOCOL) + strlen(username) + 1;	
	if (GET__CLIENT_REQUEST_PRO(client_request, username) == -1)
	{
		printf("Protocol failed\n");		
		return -1;
	}
	if (Send_Socket(s_client, client_request, client_req_size) == -1)
	{
		printf("Send Failed\n");		
		return -1;
	}	
	return 0;
}
int GET__Server_Response(SOCKET s_client, char * server_response)
{	
	if (Recv_Socket(s_client, server_response) == -1)
	{
		printf("Recv Failed\n");
		return -1;
	}
	return GET__Response_ID(server_response);
}

/*time_t x = time(NULL);
while (time(NULL) > x + 7);*/