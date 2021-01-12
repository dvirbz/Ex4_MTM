#include <Windows.h>
#include <stdio.h>
#include "Lock.h"

Lock* New__Lock(int number_of_threads)
{
	Lock* my_lock = (Lock*)malloc(sizeof(Lock));
	if (my_lock == NULL) {
		printf("MEMORY_ALLOCATION_FAILED\n");
		return NULL;
	}
	my_lock->read_lock = CreateSemaphoreA(NULL, number_of_threads, number_of_threads, NULL);
	if (my_lock->read_lock == NULL){
		return NULL;
	}
	my_lock->write_lock = CreateSemaphoreA(NULL, number_of_threads, number_of_threads, NULL);
	if (my_lock->write_lock == NULL) {
		return NULL;
	}
	my_lock->write_lock_mutex = CreateMutexA(NULL, FALSE, NULL);
	if (my_lock->write_lock_mutex == NULL) {
		return NULL;
	}	
	return my_lock;
}

BOOL Write__Release(Lock* my_Lock, int number_of_threads) {	
	return ReleaseSemaphore(my_Lock->write_lock, number_of_threads, NULL);
}

BOOL Read__Release(Lock* my_Lock){	
	return ReleaseSemaphore(my_Lock->read_lock, 1, NULL) && ReleaseSemaphore(my_Lock->write_lock, 1, NULL);
}

BOOL Read__Lock(Lock* my_Lock, int wait_time) {
	DWORD wait_code;	
	wait_code = WaitForSingleObject(my_Lock->write_lock, wait_time);
	if (WAIT_OBJECT_0 != wait_code) {
		printf("write_locked\n");
		return  FALSE;
	}	
	wait_code = WaitForSingleObject(my_Lock->read_lock, wait_time);
	if (WAIT_OBJECT_0 != wait_code) {
		printf("read_locked\n");
		return  FALSE;
	}		
	return TRUE;
}

BOOL Write__Lock(Lock* my_Lock, int wait_time, int number_of_threads) {
	DWORD wait_code;
	for (int i = 0; i < number_of_threads; i++)
	{		
		wait_code = WaitForSingleObject(my_Lock->write_lock, wait_time);		
		if (WAIT_OBJECT_0 != wait_code) {
			return  FALSE;
		}
	}		
	return TRUE;
}
BOOL Write__Lock__Mutex(Lock* my_Lock, int wait_time) {
	DWORD wait_code;
	wait_code = WaitForSingleObject(my_Lock->write_lock_mutex, wait_time);	
	if (WAIT_OBJECT_0 != wait_code) {		
		return  FALSE;
	}		
	return TRUE;
}
BOOL Write__Release__Mutex(Lock* my_Lock) {
	return ReleaseMutex(my_Lock->write_lock_mutex);
}

BOOL Destroy__lock(Lock* my_Lock)
{
	BOOL succeded = TRUE;
	if (CloseHandle(my_Lock->read_lock) == FALSE)
	{
		succeded = FALSE;
	}
	if (CloseHandle(my_Lock->write_lock) == FALSE)
	{
		succeded = FALSE;
	}
	if (CloseHandle(my_Lock->write_lock_mutex) == FALSE)
	{
		succeded = FALSE;
	}
	free(my_Lock);
	return succeded;
}



