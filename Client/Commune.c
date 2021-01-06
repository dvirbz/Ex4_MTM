#include "Commune.h"
#include <stdio.h>

char* GET__Server_Denied_PRO()
{
	return SERVER_DENIED;
}
int GET__CLIENT_REQUEST_PRO(char * protocol[],char * username)
{
	if (snprintf((*protocol), MAX_PRO_LEN, "%s:%s%s\0", CLIENT_REQUEST, username, END_PROTOCOL) == 0)
		return -1;
	return 0;
}