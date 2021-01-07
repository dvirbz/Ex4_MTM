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
int SendBuffer(SOCKET sd, const char* Buffer, int BytesToSend);
int Send_Socket(SOCKET s, const char* buffer, int len);
int ReceiveBuffer(SOCKET sd, char* OutputBuffer, int BytesToReceive);
int Recv_Socket(SOCKET s, char* buffer);
void Connect_MENU(char server_ip[], int server_port);
void Denied_MENU(char server_ip[], int server_port);
void Choose_Next_Connect();
void Choose_Next_Play();

int main(int argc, char* argv[])
{
	int error_code = 0;
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
	while (connect_status == CONNECT)
	{
		if (Connect_Socket(s_client, (SOCKADDR*)&clientService) != 0)
		{
			Connect_MENU(server_ip_address, server_port_number);
			if (scanf_s("%d", &connect_status) == 0)
			{
				error_code = -1;
				goto ExitSeq;
			}
			switch (connect_status)
			{
			case 1:connect_status = CONNECT;
				break;
			case 2:connect_status = EXIT;
				break;
			}
			if (connect_status == EXIT)
			{
				error_code = -1;
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
		int client_req_size = strlen(CLIENT_REQUEST) + strlen(END_PROTOCOL) + strlen(username) + 2;
		char* client_request = (char*)calloc(client_req_size, sizeof(char));
		if (GET__CLIENT_REQUEST_PRO(client_request, username) == -1)
		{
			printf("Protocol failed\n");
			error_code = -1;
			free(client_request);
			goto ExitSeq;
		}
		if (Send_Socket(s_client, client_request, client_req_size - 1) == -1)
		{
			printf("Send Failed\n");
			free(client_request);
			goto ExitSeq;
		}
		free(client_request);
		
		/* Get Server Response */
		char* server_response = (char*)calloc(MAX_PRO_LEN, sizeof(char));
		if (Recv_Socket(s_client, server_response) == -1)
		{
			printf("Recv Failed\n");
			error_code = -1;
			goto ExitSeq;
		}
		int response_id = GET__Response_ID(server_response);//change
		printf("Response id: %d\n", response_id);
		switch (response_id)
		{
		case SERVER_DENIED_ID:
			Denied_MENU(server_ip_address, server_port_number);
			if (scanf_s("%d", &connect_status) == 0)
			{
				error_code = -1;
				goto ExitSeq;
			}
			switch (connect_status)
			{
			case 1:connect_status = CONNECT;
				break;
			case 2:connect_status = EXIT;
				break;
			}
			if (connect_status == EXIT)
			{
				error_code = -1;
				goto ExitSeq;
			}
			if (closesocket(s_client) == SOCKET_ERROR)
			{
				error_code = -1;
				goto ExitSeq;
			}
			s_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (s_client == INVALID_SOCKET)
			{
				error_code = -1;
				goto Exit_WO_CLOSE;
			}
			break;
		case SERVER_APPROVED_ID:
			connect_status = PLAY;
			break;
		default:printf("pro: %s size: %d\n", server_response, sizeof(server_response));
			break;
		}
	}
	play_status = connect_status;
	while (play_status == PLAY)
	{
		Choose_Next_Play();
		if (scanf_s("%d", &play_status) == 0)
		{
			error_code = -1;
			goto ExitSeq;
		}
		switch (play_status)
		{
		case 1:play_status = PLAY;
			break;
		case 2:play_status = EXIT;
			break;
		}
		if (play_status != PLAY)
		{
			error_code = -1;
			goto ExitSeq;
		}
	}

	printf("All will be well %s, PortNumber: %d\n", username, server_port_number);

ExitSeq:
	if (closesocket(s_client) == SOCKET_ERROR)
	{
		printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
	}
Exit_WO_CLOSE:
	if (WSACleanup() == SOCKET_ERROR)
	{
		printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
	}
	return error_code;
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
int SendBuffer(SOCKET sd,const char* Buffer, int BytesToSend)
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
int Send_Socket(SOCKET s, const char * buffer, int len)
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
int Connect_to_Server()
{
	return 0;
}

/*time_t x = time(NULL);
while (time(NULL) > x + 7);*/