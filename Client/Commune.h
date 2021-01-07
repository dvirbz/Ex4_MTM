#pragma once
#ifndef  COMMUNE_H
#define COMMUNE_H


#define SERVER_DENIED "SERVER_DENIED"
#define SERVER_APPROVED "SERVER_APPROVED"


#define CLIENT_REQUEST "CLIENT_REQUEST"

#define MAX_PRO_LEN 100 //should be updated to the true size
#define END_PROTOCOL "\n"

enum Server_Responses
{
	SERVER_DENIED_ID,
	SERVER_APPROVED_ID,
	NUMBER_OF_SERVER_RES
};
/*Server Protocols*/
int GET__Server_Approved_PRO(char* protocol);
int GET__Server_Denied_PRO(char* protocol);

/*Client Protocols*/
int GET__CLIENT_REQUEST_PRO(char* protocol, char* username);

/*Compare*/
int IS_Denied_or_Approved(char* protocol);
int GET__Response_ID(char* protocol);
char* GET__Message_Type(char* protocol);

#endif // ! COMMUNE_H