#include "Commune.h"
#include "Server_send_recv.h"
#include "File_Func.h"
SERVER_SEND_RECV_H int num_of_writing = -1;
SERVER_SEND_RECV_H HANDLE ThreadHandles[NUM_OF_THREADS];
SERVER_SEND_RECV_H HANDLE first_want_to_invite = NULL;
SERVER_SEND_RECV_H HANDLE second_want_to_invite = NULL;
SERVER_SEND_RECV_H HANDLE readAndWriteEvent = NULL;

int first_player_versus(SOCKET s_communication, char* server_massage,
	HANDLE gameSession, Player* current_player, Player* other_player, Lock* file_lock, BOOL opponentQuit);
int second_player_versus(HANDLE* gameSession, Player* current_player,
	Player* other_player, Lock* file_lock, BOOL opponentQuit);
BOOL no_opponents();

/* Recv func */
int recive_client_request(SOCKET s_communication, char* client_response, Player* player)
{
	printf("got here\n");
	if (Recv_Socket(s_communication, client_response, FIFTEEN_SEC) == -1)
	{
		printf("Recv Failed\n");
		return -1;

	}
	if (GET__Client_Response_ID(client_response) != CLIENT_REQUEST_ID)
	{
		printf("Didn't CLIENT_REQUEST_ID\n");
		return -1;
	}
	printf("got here2 client_request: %s\n", client_response);
	BnC_Data* data = GET__BnC_Data(client_response);
	if (data == NULL)
	{
		printf("Data Failed\n");
		return -1;
	}
	if (snprintf(player->username, MAX_USERNAME_LEN, "%s", data->first_string) == 0)
	{
		free(data);
		printf("Copy Failed\n");
		return -1;
	}
	free(data);
	player->line_size = strlen(player->username) + strlen(player->move) +
		3 * strlen(PARTITION_MASSAGE_PARAMETERS) + 2 * strlen(END_PROTOCOL) + BULLS_AND_COWS_STR_LEN;
	printf("rcr username: %s\n", player->username);
	return 0;
}

/* Send func */
int send_approved(SOCKET s_communication, char* server_massage)
{
	if (GET__Server_Pro(server_massage, SERVER_APPROVED) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage), FIFTEEN_SEC) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}
int send_server_denied(SOCKET s_communication, char* server_massage)
{
	if (GET__Server_Pro(server_massage, SERVER_DENIED) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage), FIFTEEN_SEC) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}
int send_main_menu(SOCKET s_communication, char* server_massage)
{
	if (GET__Server_Pro(server_massage, SERVER_MAIN_MENU) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage), FIFTEEN_SEC) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}
int send_invite(SOCKET s_communication, char* server_massage, char* other_username)
{
	printf("sending invite...\n");
	if (GET__Server_Invite_Pro(server_massage, other_username) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage), FIFTEEN_SEC) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	printf("invite sent?\n");
	return 0;
}
int send_setup_request(SOCKET s_communication, char* server_massage)
{
	if (GET__Server_Pro(server_massage, SERVER_SETUP_REQUEST) == -1)
	{
		printf("Protocol failed\n");
		return ERROR_CODE;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage), FIFTEEN_SEC) == -1)
	{
		printf("Send Failed\n");
		return ERROR_CODE;
	}
	return 0;
}
int send_move_request(SOCKET s_communication, char* server_massage)
{
	if (GET__Server_Pro(server_massage, SERVER_PLAYER_MOVE_REQUEST) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage), FIFTEEN_SEC) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}
int send_game_results(SOCKET s_communication, char* server_massage,
	Player* other_player, Player* current_player)
{
	if (GET__Server_Game_Results_Pro(server_massage, current_player->bulls, current_player->cows,
		other_player->username, other_player->move) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage), FIFTEEN_SEC) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}
int send_game_won(SOCKET s_communication, char* server_massage,
	Player* other_player, Player* current_player)
{
	if (GET__Server_Won_Pro(server_massage, current_player->username, other_player->setup) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage), FIFTEEN_SEC) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}
int send_game_draw(SOCKET s_communication, char* server_massage,
	Player* other_player, Player* current_player)
{
	if (GET__Server_Pro(server_massage, SERVER_DRAW) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage), FIFTEEN_SEC) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}
int send_server_no_opponents(SOCKET s_communication, char* server_massage)
{
	if (GET__Server_Pro(server_massage, SERVER_NO_OPPONENTS) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage), FIFTEEN_SEC) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}
int send_opponent_quit(SOCKET s_communication, char* server_massage)
{
	if (GET__Server_Pro(server_massage, SERVER_OPPONENT_QUIT) == -1)
	{
		printf("Protocol failed\n");
		return -1;
	}
	if (Send_Socket(s_communication, server_massage, strlen(server_massage), FIFTEEN_SEC) == -1)
	{
		printf("Send Failed\n");
		return -1;
	}
	return 0;
}

/* Send and Recv func */
int Handle_Client_Request_Approved(SOCKET s_communication, char* client_response, char* server_massage,
	Player* current_player)
{
	int exit_code = 0;
	exit_code = recive_client_request(s_communication, client_response, current_player);
	if (exit_code != 0)
		return exit_code;
	exit_code = send_approved(s_communication, server_massage);
	if (exit_code != 0)
		return exit_code;
	return exit_code;
}
int Handle_Client_Request_Denied(SOCKET s_communication, char* client_response, char* server_massage,
	Player* current_player)
{
	int exit_code = 0;
	exit_code = recive_client_request(s_communication, client_response, current_player);
	if (exit_code != 0)
		return exit_code;
	exit_code = send_server_denied(s_communication, server_massage);
	if (exit_code != 0)
		return exit_code;
	return exit_code;
}
int Handle_setup(SOCKET s_communication, char* client_response, char* server_massage,
	Player* current_player, Player* other_player, HANDLE gameSession, Lock* file_lock, BOOL opponentQuit)
{
	int exit_code = 0;
	if (opponentQuit == TRUE)
	{
		if (send_server_no_opponents(s_communication, server_massage) == EXIT_CODE)
		{
			printf("send setupReq failed!\n");
			return EXIT_CODE;
		}
		return SERVER_NO_OPPONENTS_ID;
	}
	else
	{
		exit_code = send_setup_request(s_communication, server_massage);
		if (exit_code != 0)
		{
			printf("send setupReq failed!\n");
			return exit_code;
		}
	}
	exit_code = Recv_Socket(s_communication, client_response, TEN_MINUTES);
	if (exit_code != 0)
	{
		printf("Recv Failed\n");
		return exit_code;
	}

	int clResId = GET__Client_Response_ID(client_response);
	if (clResId == CLIENT_DISCONNECT_ID)
	{
		printf("got CLIENT_DISCONNECT_ID\n");
		return CLIENT_DISCONNECT_ID;
	}
	if (clResId != CLIENT_SETUP_ID)
	{
		printf("didn't get setup id, got: %s, id: %d\n", client_response, clResId);
		return ERROR_CODE;
	}

	BnC_Data* data = GET__BnC_Data(client_response);
	if (data == NULL)
	{
		printf("can't get BnCdata, got: %s\n", client_response);
		return ERROR_CODE;
	}

	if (snprintf(current_player->setup, NUM_DIGITIS_GUESS, "%s", data->first_string) == 0)
	{
		printf("can't snprintf\n");
		return ERROR_CODE;
	}
	free(data);
	if (snprintf(current_player->move, NUM_DIGITIS_GUESS, "%s", current_player->setup) == 0)
	{
		printf("can't snprintf\n");
		return ERROR_CODE;
	}
	if (write_and_read(gameSession, current_player, other_player, file_lock, opponentQuit) != 0)
	{
		return ERROR_CODE;
	}
	if (snprintf(other_player->setup, NUM_DIGITIS_GUESS, "%s", other_player->move) == 0)
	{
		printf("snprintf failed\n");
		return ERROR_CODE;
	}
	return 0;
}
int Handle_move(SOCKET s_communication, char* client_response, char* server_massage,
	Player* current_player, Player* other_player, HANDLE gameSession, Lock* file_lock, BOOL opponentQuit)
{
	if (opponentQuit == TRUE)
	{
		if (send_server_no_opponents(s_communication, server_massage) == EXIT_CODE)
		{
			printf("send setupReq failed!\n");
			return EXIT_CODE;
		}
		return SERVER_NO_OPPONENTS_ID;
	}
	else
	{
		if (send_move_request(s_communication, server_massage) == EXIT_CODE)
		{
			printf("send setupReq failed!\n");
			return EXIT_CODE;
		}
	}
	int exit_code = 0;
	exit_code = Recv_Socket(s_communication, client_response, TEN_MINUTES);
	if(exit_code != 0)
	{
		printf("Recv Failed\n");
		return exit_code;
	}
	int clResId = GET__Client_Response_ID(client_response);
	if (clResId == CLIENT_DISCONNECT_ID)
	{
		printf("got CLIENT_DISCONNECT_ID\n");
		return CLIENT_DISCONNECT_ID;
	}
	if (clResId != CLIENT_PLAYER_MOVE_ID)
	{
		printf("didn't get move id, got: %s, id: %d\n", client_response, clResId);
		return ERROR_CODE;
	}

	BnC_Data* data = GET__BnC_Data(client_response);
	if (data == NULL)
	{
		printf("can't get BnCdata, got: %s\n", client_response);
		return ERROR_CODE;
	}

	if (snprintf(current_player->move, NUM_DIGITIS_GUESS, "%s", data->first_string) == 0)
	{
		printf("can't snprintf\n");
		return ERROR_CODE;
	}
	free(data);
	if (write_and_read(gameSession, current_player, other_player, file_lock, opponentQuit) != 0)
	{
		return ERROR_CODE;
	}

	return 0;
}
int versus_or_disconnect(SOCKET s_communication, HANDLE* gameSession, char* client_response,
	char* server_massage, Player* current_player, Player* other_player, Lock* file_lock)
{
	if (Recv_Socket(s_communication, client_response, TEN_MINUTES) == -1)
	{
		printf("Recv Failed\n");
		return ERROR_CODE;
	}
	int response = GET__Client_Response_ID(client_response);
	if (response == CLIENT_DISCONNECT_ID)
	{
		printf("client disconnected, id: %d", CLIENT_DISCONNECT_ID);
		return CLIENT_DISCONNECT_ID;
	}
	if (response != CLIENT_VERSUS_ID)
	{
		printf("didnt get expacted response from versus or disconnect, ID = %d", response);
		return ERROR_CODE;
	}
	if (no_opponents() == TRUE)
	{
		if (send_server_no_opponents(s_communication, server_massage) == ERROR_CODE)
		{
			printf("couldn't send server no opponents\n");
			return ERROR_CODE;
		}
		return SERVER_NO_OPPONENTS_ID;
	}
	if (Write__First__Lock__Mutex(file_lock, 15000) == FALSE)
	{
		printf("Deadlock first write\n");
		return ERROR_CODE;
	}
	printf("trying to open gameSession.txt\n");
	*gameSession = CreateFileA(FILE_GAME_SESSION, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (*gameSession == INVALID_HANDLE_VALUE)
	{
		if (second_player_versus(gameSession, current_player, other_player, file_lock, FALSE) == ERROR_CODE)
		{
			return ERROR_CODE;
		}
	}
	else
	{
		// we are first
		if (first_player_versus(s_communication, server_massage,
			*gameSession, current_player, other_player, file_lock, FALSE) == ERROR_CODE)
		{
			return ERROR_CODE;
		}
		
	}
	//check if both players want to send invite
	DWORD retVal;
	if (current_player->is_first_player == TRUE)
	{
		SetEvent(first_want_to_invite);
		retVal = WaitForSingleObject(second_want_to_invite, FIFTEEN_SEC);
	}
	else
	{
		SetEvent(second_want_to_invite);
		retVal = WaitForSingleObject(first_want_to_invite, FIFTEEN_SEC);
	}
	switch (retVal)
	{
	case WAIT_OBJECT_0:
		break;
	case WAIT_FAILED:
		return ERROR_CODE;
		break;
	case WAIT_TIMEOUT:
		if (send_server_no_opponents(s_communication, server_massage) == ERROR_CODE)
		{
			printf("couldn't send server no opponents\n");
			return ERROR_CODE;
		}
		return SERVER_NO_OPPONENTS_ID;
	}

	if (send_invite(s_communication, server_massage, other_player->username) == ERROR_CODE)
	{
		printf("send invite failed!\n");
		return ERROR_CODE;
	}
	return 0;
}

int first_player_versus(SOCKET s_communication, char* server_massage, 
	HANDLE gameSession, Player* current_player, Player* other_player, Lock* file_lock , BOOL opponentQuit)
{
	current_player->is_first_player = TRUE;
	other_player->is_first_player = FALSE;
	printf("Im the first player\n");
	if (write_to_file(gameSession, current_player, other_player, file_lock) != 0)
	{
		printf("failed to write line\n");
		if (Write__First__Release__Mutex(file_lock) == FALSE)
		{
			printf("Coulden't release lock\n");
			goto ReleaseMutex;
		}
		return ERROR_CODE;
	}
	if (Write__First__Release__Mutex(file_lock) == FALSE)
	{
		printf("Coulden't release lock\n");
		return ERROR_CODE;
	}
	DWORD WaitResult;
	WaitResult = WaitForSingleObject(
		readAndWriteEvent,
		TEN_MINUTES);
	switch (WaitResult)
	{
	case WAIT_OBJECT_0:
		break;
	case WAIT_FAILED:
		return ERROR_CODE;
		break;
	case WAIT_TIMEOUT:
		if (send_server_no_opponents(s_communication, server_massage) == ERROR_CODE)
		{
			printf("couldn't send server no opponents\n");
			return ERROR_CODE;
		}
		printf("waited too long for other opponent to write\n");
		return SERVER_NO_OPPONENTS_ID;
	}
	if (read__line(gameSession, current_player, other_player, file_lock, opponentQuit) != 0)
	{
		printf("couldn't read line after sleep\n");
		return ERROR_CODE;
	}
	return 0;

ReleaseMutex:
	if (Write__First__Release__Mutex(file_lock) == FALSE)
	{
		printf("Coulden't release lock\n");
		return ERROR_CODE;
	}
	return ERROR_CODE;
}
int second_player_versus(HANDLE* gameSession, Player* current_player,
	Player* other_player, Lock* file_lock, BOOL opponentQuit)
{
	if (*gameSession == INVALID_HANDLE_VALUE || *gameSession == NULL)
	{
		*gameSession = CreateFileA(FILE_GAME_SESSION, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	if (*gameSession == INVALID_HANDLE_VALUE)
	{
		printf("can't open file, last error: %d\n", GetLastError());
		goto ReleaseMutex;
	}
	current_player->is_first_player = FALSE;
	other_player->is_first_player = TRUE;
	if (read__line(*gameSession, current_player, other_player, file_lock, opponentQuit) != 0)
	{
		printf("failed to read line\n");
		goto ReleaseMutex;
	}
	if (write_to_file(*gameSession, current_player, other_player, file_lock) != 0)
	{
		printf("failed to write line\n");
		goto ReleaseMutex;
	}
	if (Write__First__Release__Mutex(file_lock) == FALSE)
	{
		printf("Coulden't release lock\n");
		return ERROR_CODE;
	}
	printf("num_of_writings: %d\n", num_of_writing);
	num_of_writing++;
	if (!SetEvent(readAndWriteEvent))
	{
		printf("SetEvent failed (%d)\n", GetLastError());
		return ERROR_CODE;
	}
	return 0;

ReleaseMutex:
	if (Write__First__Release__Mutex(file_lock) == FALSE)
	{
		printf("Coulden't release lock\n");
		return ERROR_CODE;
	}
	return ERROR_CODE;
}
BOOL no_opponents()
{
	for (int i = 0; i < NUM_OF_THREADS; i++)
	{
		if (ThreadHandles[i] == NULL)
		{
			printf("we dont have oponents before waitforsingle\n");
			return TRUE;
		}
		DWORD Res = WaitForSingleObject(ThreadHandles[i], 0);
		if (Res == WAIT_OBJECT_0) // this thread finished running
		{
			printf("we dont have oponents after waitforsingle\n");
			CloseHandle(ThreadHandles[i]);
			ThreadHandles[i] = NULL;
			return TRUE;
		}
	}
	printf("returned false no_opponents\n");
	return FALSE;
}