#pragma once
#ifndef FILE_FUNC_H
#define FILE_FUNC_H
#include "Commune.h"
#include "Server_send_recv.h"
#include "Player.h"
#include "HardCodedData.h"

FILE_FUNC_H BOOL opponentQuit[NUM_OF_THREADS];

int write_to_file(HANDLE gameSession, Player* current_player, Player* other_player, Lock* file_lock);

int read__line(HANDLE gameSession, Player* current_player, Player* other_player, Lock* file_lock, int threadNumber);

int write_and_read(HANDLE gameSession, Player* current_player, Player* other_player, Lock* file_lock, int threadNumber);
int write_and_read(HANDLE gameSession, Player* current_player, Player* other_player, Lock* file_lock, int threadNumber);
int update_data_from_file(Player* other_player, Player* current_player, char* player_info);
#endif