#include "Player.h"

int init_player(Player* player)
{
	if (snprintf(player->setup, NUM_DIGITIS_GUESS, "set!") == 0)
	{
		return ERROR_CODE;
	}
	if (snprintf(player->move, NUM_DIGITIS_GUESS, "mov!") == 0)
	{
		return ERROR_CODE;
	}
	if (snprintf(player->username, MAX_USERNAME_LEN, "UsernameInitilize!!!") == 0)
	{
		return ERROR_CODE;
	}
	player->line_size = strlen(player->username) + strlen(player->move) +
		3 * strlen(PARTITION_MASSAGE_PARAMETERS) + 2 * strlen(END_PROTOCOL) + BULLS_AND_COWS_STR_LEN;
	player->is_first_player = FALSE;
	player->bulls = 5;
	player->cows = 5;
	return 0;
}
int init_playeres(Player* current_player, Player* other_player)
{
	int exit_code = 0;
	exit_code = init_player(current_player);
	if (exit_code != 0)
		goto ExitSeq;
	exit_code = init_player(other_player);
	if (exit_code != 0)
		goto ExitSeq;
ExitSeq:
	return exit_code;
}
Game_Results GET__Game_Results(Player* current_player, Player* other_player)
{
	if (current_player->bulls == NUM_DIGITIS_GUESS - 1)
	{
		if (other_player->bulls == NUM_DIGITIS_GUESS - 1)
		{
			return DRAW;
		}
		return CURRENT_WON;
	}
	if (other_player->bulls == NUM_DIGITIS_GUESS - 1)
	{
		return OTHER_WON;
	}
	return CONTINUE;
}

BnC_Data* GET__Bulls_And_Cows(char* setup, char* guess)
{
	int bulls = 0, cows = 0;
	for (unsigned int i = 0; i < strlen(guess); i++)
	{
		for (unsigned int j = 0; j < strlen(setup); j++)
		{
			if (guess[i] == setup[j])
			{
				if (i == j)
				{
					bulls++;
				}
				else
				{
					cows++;
				}
			}
		}
	}
	BnC_Data* data = (BnC_Data*)calloc(1, sizeof(BnC_Data));
	if (data == NULL)
	{
		printf("MEM Allocation Fail\n");
		return NULL;
	}
	data->bulls = bulls;
	data->cows = cows;
	data->first_string[0] = '\0';
	data->second_string[0] = '\0';
	return data;
}