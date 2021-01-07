#include "Commune.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")

int GET__Server_Denied_PRO(char * protocol)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s", SERVER_DENIED) == 0)
		return -1;
	return 0;
}
int GET__Server_Approved_PRO(char* protocol)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s", SERVER_APPROVED) == 0)
		return -1;
	return 0;
}

int GET__Response_ID(char* protocol)
{
	char* message_type = GET__Message_Type(protocol);	
	if (message_type == NULL)
	{
		return -1;
	}
	printf("message type: %s\n", message_type);
	if (strcmp(message_type, SERVER_APPROVED) == 0)
	{
		return SERVER_APPROVED_ID;
	}
	if (strcmp(message_type, SERVER_DENIED) == 0)
	{
		return SERVER_DENIED_ID;
	}
	return -1;
}

int GET__CLIENT_REQUEST_PRO(char * protocol,char * username)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s:%s%s\0", CLIENT_REQUEST, username, END_PROTOCOL) == 0)
		return -1;
	return 0;
}

int GET__PRO_WITH_EOP(char* protocol)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s%s\0", protocol, END_PROTOCOL) == 0)
		return -1;
	return 0;
}
char* GET__Message_Type(char* protocol)
{
	printf("Protocol: %s\n", protocol);
	char* exit_char = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	if (exit_char == NULL)
	{
		goto Exit;
	}
	char* message_type = (char*)calloc(MAX_PRO_LEN, sizeof(char));
	if (message_type == NULL)
	{
		exit_char = NULL;
		goto Exit;
	}
	if (snprintf(message_type, MAX_PRO_LEN, "%s", protocol) == 0)
	{
		exit_char = NULL;
		goto Free;
	}
	printf("INNER message type: %s size: %d strlen: %d\n", message_type,
		sizeof(message_type), strlen(message_type));
	char* next = NULL;
	if (snprintf(exit_char, MAX_PRO_LEN, "%s", strtok_s(message_type, ":\n", &next)) == 0)
	{
		free(exit_char);
		exit_char = NULL;
		goto Free;
	}
	printf("EXIT message type: %s\n", exit_char);

Free:
	free(message_type);
Exit:
	return exit_char;
}

int SendBuffer(SOCKET sd, const char* Buffer, int BytesToSend)
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
int Send_Socket(SOCKET s, const char* buffer, int len)
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