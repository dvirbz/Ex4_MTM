#pragma once
#ifndef PLAYER_H
#define PLAYER_H
#include "Commune.h"
#define BULLS_AND_COWS_STR_LEN 2

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
	CURRENT_WON = 1,
	OTHER_WON,
	DRAW,
	CONTINUE
}Game_Results;

void print_player(Player* player);
int init_player(Player* player);
Game_Results GET__Game_Results(Player* current_player, Player* other_player);
BnC_Data* GET__Bulls_And_Cows(char* setup, char* guess);
#endif