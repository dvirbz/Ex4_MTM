#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int InitializeWSA();
int Connect_Socket(SOCKET s, SOCKADDR* cs);

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
	int reconnect = TRUE;
	while (Connect_Socket(s_client, (SOCKADDR*)&clientService) != 0)
	{
		printf("Failed connecting to server on %s:%d\n", server_ip_address, server_port_number);
		printf("Choose what to do next:\n");
		printf("1. Try to reconnect\n");
		printf("2. Exit\n");
		if (scanf_s("%d", &reconnect) == 0)
		{
			error_code = -1;
			goto ExitSeq;
		}
		if(reconnect != TRUE)
		{
			error_code = -1;
			goto ExitSeq;
		}
	}	
	printf("Connected to server on %s:%d\n", server_ip_address, server_port_number);
	/*===============================================================================*/

	printf("All will be well %s, PortNumber: %d", username, server_port_number);
	

ExitSeq:
	closesocket(s_client);//check for error
	WSACleanup();//check for error
	return error_code;
	
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