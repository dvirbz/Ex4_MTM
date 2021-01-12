#ifndef LOCK_H
#define LOCK_H
#include <Windows.h>

typedef struct Lock {
	HANDLE read_lock;//Semaphore To lock reading in case of other thread is writing
	HANDLE write_lock;//Semaphore To lock writing in case of other thread is reading
	HANDLE write_lock_mutex;//Prevents writing in case of other thread is writing	
}Lock;
/*========================================================================*/
/* Initialize Lock, Allocate the required memory and returns the new Lock.
   If it fails returns NULL, it's on the user to check.
   */
Lock* New__Lock(int number_of_threads);
/*========================================================================*/
/* Input: Pointer to Lock, Number of threads
   Ups the write_lock semaphore number of threads times
   Output: return if the Release Semaphore succeeded -> TRUE else FALSE  
   */
BOOL Write__Release(Lock* my_Lock, int number_of_threads);
/*========================================================================*/
/* Input: Pointer to Lock
   Ups the read_lock semaphore and write_lock one time
   Output: return if the Releases Semaphore succeeded -> TRUE else FALSE
   */
BOOL Read__Release(Lock* my_Lock);
/*========================================================================*/
/* Input: Pointer to Lock
   Signals the write_lock_mutex
   Output: return if the Releases Mutex succeeded -> TRUE else FALSE
   */
BOOL Write__Release__Mutex(Lock* my_Lock);

/*========================================================================*/
/* Input: Pointer to Lock, wait time
   Downs the read_lock and write_lock semaphore
   Output: return if the Down succeeded -> TRUE else FALSE
   */
BOOL Read__Lock(Lock* my_Lock, int wait_time);
/*========================================================================*/
/* Input: Pointer to Lock, wait time, number of threads
   Tries to Down the write_lock semaphore number_of_threads times
   Output: return if the All Downs succeeded -> TRUE else FALSE
   */
BOOL Write__Lock(Lock* my_Lock, int wait_time, int number_of_threads);
/*========================================================================*/
/* Input: Pointer to Lock, wait time
   Down the write_lock_mutex
   Output: return if the Down succeeded -> TRUE else FALSE
   */
BOOL Write__Lock__Mutex(Lock* my_Lock, int wait_time);
/*========================================================================*/

/*  Input: Pointer to Lock
    Frees all resources related to the Lock
   */
BOOL Destroy__lock(Lock* my_Lock);
#endif 
