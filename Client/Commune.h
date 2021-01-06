#pragma once
#ifndef  COMMUNE_H
#define COMMUNE_H


#define SERVER_DENIED "SERVER_DENIED"
#define SERVER_APPROVED "SERVER_APPROVED"


#define CLIENT_REQUEST "CLIENT_REQUEST"

#define MAX_PRO_LEN 100 //should be updated to the true size
#define END_PROTOCOL "\n"

int GET__CLIENT_REQUEST_PRO(char* protocol[], char* username);

#endif // ! COMMUNE_H