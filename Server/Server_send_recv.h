#pragma once
#ifndef SERVER_SEND_RECV_H
#define SERVER_SEND_RECV_H
#include "Commune.h"
#include "Player.h"
#include "Lock.h"
#include "HardCodedData.h"

SERVER_SEND_RECV_H int num_of_writing;
SERVER_SEND_RECV_H HANDLE ThreadHandles[NUM_OF_THREADS];
SERVER_SEND_RECV_H HANDLE first_want_to_invite;
SERVER_SEND_RECV_H HANDLE second_want_to_invite;
SERVER_SEND_RECV_H HANDLE readAndWriteEvent;
/* Recv func */
int recive_client_request(SOCKET s_communication, char* client_response, Player* player);

/* Send func */
int send_approved(SOCKET s_communication, char* server_massage);
int send_main_menu(SOCKET s_communication, char* server_massage);
int send_invite(SOCKET s_communication, char* server_massage, char* other_username);
int send_setup_request(SOCKET s_communication, char* server_massage);
int send_move_request(SOCKET s_communication, char* server_massage);
int send_game_results(SOCKET s_communication, char* server_massage,
	Player* other_player, Player* current_player);
int send_game_won(SOCKET s_communication, char* server_massage,
	Player* other_player, Player* current_player);
int send_game_draw(SOCKET s_communication, char* server_massage,
	Player* other_player, Player* current_player);
int send_server_no_opponents(SOCKET s_communication, char* server_massage);
/* Send and Recv func */
int Handle_setup(SOCKET s_communication, char* client_response, char* server_massage,
	Player* current_player, Player* other_player, HANDLE gameSession, Lock* file_lock);
int Handle_move(SOCKET s_communication, char* client_response, char* server_massage,
	Player* current_player, Player* other_player, HANDLE gameSession, Lock* file_lock);
int versus_or_disconnect(SOCKET s_communication, HANDLE* gameSession, char* client_response,
	char* server_massage, Player* current_player, Player* other_player, Lock* file_lock);

BOOL no_opponents();
#endif