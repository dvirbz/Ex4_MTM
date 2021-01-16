#include "File_Func.h"
/*Write the player data line to the appropriate line in the file */
int write_to_file(HANDLE gameSession, Player* current_player, Player* other_player, Lock* file_lock)
{
	if (Write__Lock__Mutex(file_lock, 5000) == FALSE)
	{
		printf("write lock mutex\n");
		return -1;
	}
	if (Write__Lock(file_lock, 5000, NUM_OF_THREADS) == FALSE)
	{
		Write__Release__Mutex(file_lock);
		printf("write lock\n");
		return -1;
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
		printf("couldn't set file pointer, error: %d\n", GetLastError());
		return -1;
	}
	char line_to_write[MAX_LINE_LEN];
	int character_count = snprintf(line_to_write, MAX_LINE_LEN, "%s:%s:%d:%d\r\n",
		current_player->username, current_player->move, other_player->bulls, other_player->cows);
	if (character_count == 0)
	{
		Write__Release(file_lock, NUM_OF_THREADS);
		Write__Release__Mutex(file_lock);
		printf("couldn't write string\n");
		return -1;
	}
	if (WriteFile(gameSession, line_to_write, strlen(line_to_write), NULL, NULL) == 0)
	{
		Write__Release(file_lock, NUM_OF_THREADS);
		Write__Release__Mutex(file_lock);
		printf("failed to write to file\n");
		return -1;
	}
	num_of_writing++;
	if (Write__Release(file_lock, NUM_OF_THREADS) == FALSE)
	{
		Write__Release__Mutex(file_lock);
		printf("write release\n");
		return -1;
	}
	if (Write__Release__Mutex(file_lock) == FALSE)
	{
		printf("write release mutex\n");
		return -1;
	}

	return 0;
}
/*Read the player data line from the appropriate line in the file */
int read__line(HANDLE gameSession, Player* current_player, Player* other_player, Lock* file_lock)
{
	while ((num_of_writing % 2) != 0);
	if (Read__Lock(file_lock, 5000) == FALSE)
	{
		printf("Could't lock in 5 sec\n");
		return -1;
	}
	int distance_to_move = current_player->line_size;
	if (current_player->is_first_player == FALSE)
	{
		distance_to_move = 0;
	}
	if (SetFilePointer(gameSession, distance_to_move, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		printf("couldn't set file pointer\n");
		return -1;
	}
	char line[MAX_LINE_LEN];
	if (ReadFile(gameSession, line, other_player->line_size, NULL, NULL) == 0)
	{
		printf("couldn't read file\n");
		return -1;
	}
	line[other_player->line_size] = '\0';
	if (Read__Release(file_lock) == FALSE)
	{
		printf("couldn't release\n");
		return -1;
	}
	if (update_data_from_file(other_player, current_player, line) == -1)
	{
		printf("couldn't update_player\n");
		return -1;
	}
	printf("User: %s other user: %s line: %s\n", current_player->username, other_player->username, line);
	return 0;
}

int write_and_read(HANDLE gameSession, Player* current_player, Player* other_player, Lock* file_lock)
{
	if (write_to_file(gameSession, current_player, other_player, file_lock) == -1)
	{
		printf("can't write to file\n");
		return -1;
	}

	if (read__line(gameSession, current_player, other_player, file_lock) == -1)
	{
		printf("can't read from file\n");
		return -1;
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
	printf("Before Line size: %d\n", other_player->line_size);
	printf("username: %s\nmove: %s\nBulls: %s\nCows: %s\n", username, move, bulls, cows);
	if (username == NULL || move == NULL || bulls == NULL || cows == NULL)
	{
		printf("username or moveset == NULL\n");
		return -1;
	}
	if (snprintf(other_player->username, MAX_USERNAME_LEN, "%s", username) == 0)
	{
		printf("snprintf failed 3\n");
		return -1;
	}
	if (snprintf(other_player->move, NUM_DIGITIS_GUESS, "%s", move) == 0)
	{
		printf("snprintf failed 3\n");
		return -1;
	}
	current_player->bulls = (int)strtol(bulls, NULL, DECIMAL_BASE);
	if (current_player->bulls == 0 && bulls[0] != '0')
	{
		printf("strtol failed bulls: %s\n", bulls);
		return -1;
	}
	current_player->cows = (int)strtol(cows, NULL, DECIMAL_BASE);
	if (current_player->cows == 0 && cows[0] != '0')
	{
		printf("strtol failed cows: %s\n", cows);
		return -1;
	}
	other_player->line_size = strlen(username) + strlen(move) + strlen(bulls)
		+ strlen(cows) + strlen(PARTITION_MASSAGE_PARAMETERS) * 3 + 2 * strlen(END_PROTOCOL);
	printf("After Line size: %d\n", other_player->line_size);
	return 0;
}