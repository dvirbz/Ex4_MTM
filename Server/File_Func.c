#include "File_Func.h"
#include <HardCodedData.h>
FILE_FUNC_H BOOL opponentQuit[NUM_OF_THREADS] = { TRUE, TRUE };
/*Write the player data line to the appropriate line in the file */
int write_to_file(HANDLE gameSession, Player* current_player, Player* other_player, Lock* file_lock)
{
	if (Write__Lock__Mutex(file_lock, WRITE_TO_FILE_TIME) == FALSE)
	{
		return ERROR_CODE;
	}
	if (Write__Lock(file_lock, WRITE_TO_FILE_TIME, NUM_OF_THREADS) == FALSE)
	{
		Write__Release__Mutex(file_lock);
		return ERROR_CODE;
	}
	int distance_to_move = 0;
	if (current_player->is_first_player == FALSE)
	{
		distance_to_move = other_player->line_size;
	}
	if (SetFilePointer(gameSession, distance_to_move, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		Write__Release(file_lock, NUM_OF_THREADS);
		Write__Release__Mutex(file_lock);
		return ERROR_CODE;
	}
	char line_to_write[MAX_LINE_LEN];
	int character_count = snprintf(line_to_write, MAX_LINE_LEN, "%s:%s:%d:%d\r\n",
		current_player->username, current_player->move, other_player->bulls, other_player->cows);
	if (character_count == 0)
	{
		Write__Release(file_lock, NUM_OF_THREADS);
		Write__Release__Mutex(file_lock);
		return ERROR_CODE;
	}
	if (WriteFile(gameSession, line_to_write, strlen(line_to_write), NULL, NULL) == 0)
	{
		Write__Release(file_lock, NUM_OF_THREADS);
		Write__Release__Mutex(file_lock);
		printf("failed to write to file\n");
		return ERROR_CODE;
	}
	num_of_writing++;
	if (Write__Release(file_lock, NUM_OF_THREADS) == FALSE)
	{
		Write__Release__Mutex(file_lock);
		return ERROR_CODE;
	}
	if (Write__Release__Mutex(file_lock) == FALSE)
	{
		return ERROR_CODE;
	}

	return 0;
}
/*Read the player data line from the appropriate line in the file */
int read__line(HANDLE gameSession, Player* current_player, Player* other_player, Lock* file_lock, int threadNumber)
{
	while ((num_of_writing % 2) != 0 && opponentQuit[1 - threadNumber] == FALSE);
	if (Read__Lock(file_lock, WRITE_TO_FILE_TIME) == FALSE)
	{
		return ERROR_CODE;
	}
	int distance_to_move = current_player->line_size;
	if (current_player->is_first_player == FALSE)
	{
		distance_to_move = 0;
	}
	if (SetFilePointer(gameSession, distance_to_move, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		return ERROR_CODE;
	}
	char line[MAX_LINE_LEN];
	if (ReadFile(gameSession, line, other_player->line_size, NULL, NULL) == 0)
	{
		return ERROR_CODE;
	}
	line[other_player->line_size] = '\0';
	if (Read__Release(file_lock) == FALSE)
	{
		return ERROR_CODE;
	}
	if (update_data_from_file(other_player, current_player, line) == -1)
	{
		return ERROR_CODE;
	}
	return 0;
}

int write_and_read(HANDLE gameSession, Player* current_player, Player* other_player, Lock* file_lock, int threadNumber)
{
	if (write_to_file(gameSession, current_player, other_player, file_lock) == -1)
	{
		printf("can't write to file\n");
		return ERROR_CODE;
	}

	if (read__line(gameSession, current_player, other_player, file_lock, threadNumber) == -1)
	{
		printf("can't read from file\n");
		return ERROR_CODE;
	}
	return 0;
}
/*Analyze the data from the line read, update the player structets*/
int update_data_from_file(Player* other_player, Player* current_player, char* player_info)
{
	char* username, * move, * bulls, * cows;
	char* next = NULL;
	username = strtok_s(player_info, ":\r", &next);
	move = strtok_s(NULL, ":\r", &next);
	bulls = strtok_s(NULL, ":\r", &next);
	cows = strtok_s(NULL, ":\r", &next);
	if (username == NULL || move == NULL || bulls == NULL || cows == NULL)
	{
		return ERROR_CODE;
	}
	if (snprintf(other_player->username, MAX_USERNAME_LEN, "%s", username) == 0)
	{
		return ERROR_CODE;
	}
	if (snprintf(other_player->move, NUM_DIGITIS_GUESS, "%s", move) == 0)
	{
		return ERROR_CODE;
	}
	current_player->bulls = (int)strtol(bulls, NULL, DECIMAL_BASE);
	if (current_player->bulls == 0 && bulls[0] != '0')
	{
		return ERROR_CODE;
	}
	current_player->cows = (int)strtol(cows, NULL, DECIMAL_BASE);
	if (current_player->cows == 0 && cows[0] != '0')
	{
		return ERROR_CODE;
	}
	other_player->line_size = strlen(username) + strlen(move) + strlen(bulls)
		+ strlen(cows) + strlen(PARTITION_MASSAGE_PARAMETERS) * 3 + 2 * strlen(END_PROTOCOL);
	return 0;
}