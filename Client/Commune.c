#include "Commune.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int IS_Denied_or_Approved(char* protocol)
{
	int exit_code = 0;
	char* denied = (char*)calloc(MAX_PRO_LEN, 1);
	if (denied == NULL)
	{
		exit_code = -1;
		goto Exit;
	}
	if (GET__Server_Denied_PRO(&denied) == -1)
	{
		exit_code = -1;
		goto Fail;
	}
	char* approved = (char*)calloc(MAX_PRO_LEN, 1);
	if (approved == NULL)
	{
		exit_code = -1;
		goto Fail;
	}
	if (GET__Server_Approved_PRO(&approved) == -1)
	{
		exit_code = -1;
		goto FailAfterApproved;
	}
	if (strncmp(denied, protocol,sizeof(protocol)) == 0)
	{
		exit_code = SERVER_DENIED_ID;
	}
	if (strncmp(approved, protocol, sizeof(protocol)) == 0)
	{
		exit_code = SERVER_APPROVED_ID;
	}	
	//Debug
	printf("denied: %s size: %d\napproved: %s size: %d\nprotocol: %s size: %d\n",
		denied, sizeof(denied), approved, sizeof(approved), protocol, sizeof(protocol));
FailAfterApproved:
	free(approved);
Fail:	
	free(denied);
Exit:
	return exit_code;
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