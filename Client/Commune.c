#include "Commune.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int GET__Server_Denied_PRO(char * protocol[])
{
	if (snprintf((*protocol), MAX_PRO_LEN, "%s", SERVER_DENIED) == 0)
		return -1;
	return 0;
}
int GET__Server_Approved_PRO(char* protocol[])
{
	if (snprintf((*protocol), MAX_PRO_LEN, "%s", SERVER_APPROVED) == 0)
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
		exit_code = SERVER_DENIED_R;
	}
	if (strncmp(approved, protocol, sizeof(protocol)) == 0)
	{
		exit_code = SERVER_APPROVED_R;
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

int GET__CLIENT_REQUEST_PRO(char * protocol[],char * username)
{
	if (snprintf((*protocol), MAX_PRO_LEN, "%s:%s%s\0", CLIENT_REQUEST, username, END_PROTOCOL) == 0)
		return -1;
	return 0;
}

int GET__PRO_WITH_EOP(char* protocol)
{
	if (snprintf(protocol, MAX_PRO_LEN, "%s%s\0", protocol, END_PROTOCOL) == 0)
		return -1;
	return 0;
}