#pragma once
#ifndef PLAYER_H
#define PLAYER_H
#include "Commune.h"
#include "HardCodedData.h"

typedef struct
{
	char username[MAX_USERNAME_LEN];
	char setup[NUM_DIGITIS_GUESS];
	char move[NUM_DIGITIS_GUESS];
	int line_size;
	BOOL is_first_player;
	int bulls, cows;
}Player;

typedef enum
{
	CURRENT_WON = 3,
	OTHER_WON,
	DRAW,
	CONTINUE
}Game_Results;

int init_playeres(Player* current_player, Player* other_player);
Game_Results GET__Game_Results(Player* current_player, Player* other_player);
BnC_Data* GET__Bulls_And_Cows(char* setup, char* guess);
#endif