/*
** Copyright 2019, Dario Casalinuovo. All rights reserved.
** Distributed under the terms of the LGPL License.
**
** Copyright 2004, Bill Hayden. All rights reserved.
** Copyright 2002-2004, The OpenBeOS Team. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

/* Threading routines */

#include <OS.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"


#define TRACE_THREAD 1
#ifdef TRACE_THREAD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

/* TODO: Threads that die normally do not remove their own entries from */
/*        the thread table.  They remain forever...                      */

/* TODO: Compute based on the amount of available memory. */
#define MAX_THREADS 4096

#define FREE_SLOT 0xFFFFFFFF

typedef void* (*pthread_entry) (void*);

thread_info *thread_table = NULL;
static int thread_shm = -1;

static __thread thread_id sCurThreadID = B_NAME_NOT_FOUND;

/* TODO: table access is not protected by a semaphore */

void
init_thread(void)
{
	key_t table_key;
	bool created = true;
	int size = sizeof(thread_info) * MAX_THREADS;

	if (thread_table)
		return;

	/* grab a (hopefully) unique key for our table */
	table_key = ftok("/usr/local/bin/", (int)'T');

	/* create and initialize a new semaphore table in shared memory */
	thread_shm = shmget(table_key, size, IPC_CREAT | IPC_EXCL | 0700);
	if (thread_shm == -1 && errno == EEXIST) {
		/* grab the existing shared memory thread table */
		thread_shm = shmget(table_key, size, IPC_CREAT | 0700);
		created = false;
	}

	if (thread_shm < 0) {
		printf("FATAL: Couldn't setup thread table: %s\n", strerror(errno));
		return;
	}

	/* point our local table at the master table */
	thread_table = shmat(thread_shm, NULL, 0);
	if (thread_table == (void *) -1) {
		printf("FATAL: Couldn't load thread table: %s\n", strerror(errno));
		return;
	}

	if (created) {
		/* POTENTIAL RACE: table exists but is uninitialized until here */
		for (uint32 i = 0; i < MAX_THREADS; i++) {
			thread_table[i].thread = FREE_SLOT;
		}
	}
}


thread_id
_kern_spawn_thread(thread_func func, const char *name, int32 priority, void *data)
{
	for (uint32 i = 0; i < MAX_THREADS; i++) {
		if (thread_table[i].thread == FREE_SLOT) {
			if (!name)
				name = "no-name thread";

			thread_table[i].pth = -1; //not in the POSIX system yet
			thread_table[i].thread = i;
			thread_table[i].team = getpid();
			thread_table[i].priority = priority;
			thread_table[i].state = B_THREAD_SPAWNED;
			strncpy(thread_table[i].name, name, B_OS_NAME_LENGTH);
			thread_table[i].name[B_OS_NAME_LENGTH - 1] = '\0';
			thread_table[i].func = func;
			thread_table[i].data = data;
			thread_table[i].code = 0;
			thread_table[i].sender = 0;
			thread_table[i].buffer = NULL;

			sCurThreadID = i;

			return i;
		}
	}

	return B_ERROR;
}


status_t
_kern_kill_thread(thread_id thread)
{
	for (uint32 i = 0; i < MAX_THREADS; i++) {
		if (thread_table[i].thread == thread) {
			if (pthread_kill(thread_table[i].pth, SIGKILL) == 0) {
				thread_table[i].thread = FREE_SLOT;
				thread_table[i].team = 0;
				if (thread_table[i].buffer)
					free(thread_table[i].buffer);

				return B_OK;
			}
			break;
		}
	}

	return B_BAD_THREAD_ID;
}


status_t
_kern_rename_thread(thread_id thread, const char *newName)
{
	for (uint32 i = 0; i < MAX_THREADS; i++) {
		if (thread_table[i].thread == thread) {
			strncpy(thread_table[i].name, newName, B_OS_NAME_LENGTH);
			thread_table[i].name[B_OS_NAME_LENGTH - 1] = '\0';

			return B_OK;
		}
	}

	return B_BAD_THREAD_ID;
}


void
_kern_exit_thread(status_t status)
{
	pthread_t this_thread = pthread_self();

	for (uint32 i = 0; i < MAX_THREADS; i++) {
		if (thread_table[i].pth == this_thread) {
			thread_table[i].thread = FREE_SLOT;
			thread_table[i].team = 0;
			if (thread_table[i].buffer)
				free(thread_table[i].buffer);
			break;
		}
	}
	
	pthread_exit((void *) &status);
}


status_t
_kern_on_exit_thread(void (*callback)(void *), void *data)
{
    return B_NO_MEMORY;
}


status_t
_kern_send_data(thread_id thread, int32 code, const void *buffer, size_t buffer_size)
{
	for (uint32 i = 0; i < MAX_THREADS; i++) {
		if (thread_table[i].thread == thread) {
			thread_table[i].code = code;
			if (buffer) {
				if (!thread_table[i].buffer)
					thread_table[i].buffer = malloc(buffer_size);
				memcpy(thread_table[i].buffer, buffer, buffer_size);
			}

			return B_OK;
		}
	}

	return B_BAD_THREAD_ID;
}


status_t
_kern_receive_data(thread_id *sender, void *buffer, size_t bufferSize)
{
	for (uint32 i = 0; i < MAX_THREADS; i++) {
		if (thread_table[i].thread != FREE_SLOT) {
			if (*sender)
				thread_table[i].sender = *sender;

			while (!thread_table[i].buffer)
				continue;

			if (thread_table[i].buffer) {
				memcpy(buffer, thread_table[i].buffer, bufferSize);
				free(thread_table[i].buffer);
			}

			return B_OK;
		}
	}

	return B_BAD_THREAD_ID;
}


bool
_kern_has_data(thread_id thread)
{
	for (uint32 i = 0; i < MAX_THREADS; i++) {
		if (thread_table[i].thread == thread)
			return (thread_table[i].buffer != NULL);
	}

	return false;
}


void teardown_threads()
{
	int32 count = 0;
	// Free thread table entries created by our process
	for (uint32 i = 0; i < MAX_THREADS; i++) {
		if (thread_table[i].team == getpid()) {
			thread_table[i].thread = FREE_SLOT;
			thread_table[i].team = 0;
			count++;
		}
	}
	
	printf("teardown_threads(): %d threads deleted\n", count);
}


status_t
_kern_get_thread_info(thread_id id, thread_info *info)
{
	if (info == NULL || id < B_OK)
		return B_BAD_VALUE;

	for (uint32 i = 0; i < MAX_THREADS; i++) {
		if (thread_table[i].thread == id) {
			info->thread = id;
			strncpy (info->name, thread_table[i].name, B_OS_NAME_LENGTH);
			info->name[B_OS_NAME_LENGTH - 1] = '\0';
			info->state = thread_table[i].state;
			info->priority = thread_table[i].priority;
			info->team = thread_table[i].team;
			return B_OK;
		}
	}

	return B_BAD_VALUE;
}


status_t
_kern_get_next_thread_info(team_id team, int32 *_cookie, thread_info *info)
{
	if (info == NULL || *_cookie < 0)
		return B_BAD_VALUE;

	for (uint32 i = *_cookie; i < MAX_THREADS; i++) {
		if (thread_table[i].team == team) {
			*_cookie = i + 1;

			return _kern_get_thread_info(thread_table[i].thread, info);
		}
	}

	return B_BAD_VALUE;
}


thread_id
find_thread(const char *name)
{
	pthread_t pth = 0;

	// TODO: We need to make more use of thread local storage!
	if (name == NULL)
		return sCurThreadID;

	for (uint32 i = 0; i < MAX_THREADS; i++) {
		if (thread_table[i].thread != FREE_SLOT) {
			if (!pth) {
				if (strcmp(thread_table[i].name, name) == 0)
					return i;
			}
		}
	}

	return B_NAME_NOT_FOUND;
}


status_t
_kern_set_thread_priority(thread_id id, int32 priority)
{

	for (uint32 i = 0; i < MAX_THREADS; i++) {
		if (thread_table[i].thread == id) {
			thread_table[i].priority = priority;
			return B_OK;
		}
	}

	return B_BAD_THREAD_ID;
}


status_t
_kern_wait_for_thread(thread_id id, status_t *_returnCode)
{
	if (_returnCode == NULL)
		return B_BAD_VALUE;

	for (uint32 i = 0; i < MAX_THREADS; i++) {
		if (thread_table[i].thread == id) {
			// It seems the thread was spawned
			// but never resumed.
			if (thread_table[i].pth == -1)
				return B_OK;

			if (pthread_join(thread_table[i].pth, (void**)_returnCode) == 0)
				return B_OK;
			break;
		}
	}

	return B_BAD_THREAD_ID;
}


status_t
_kern_suspend_thread(thread_id id)
{
	for (uint32 i = 0; i < MAX_THREADS; i++) {
		if (thread_table[i].thread == id) {
			pthread_kill(thread_table[i].pth, SIGSTOP);
			thread_table[i].state = B_THREAD_SUSPENDED;

			return B_OK;
		}
	}

	return B_BAD_THREAD_ID;
}


status_t
_kern_resume_thread(thread_id id)
{
	for (uint32 i = 0; i < MAX_THREADS; i++) {
		if (thread_table[i].thread == id) {
			switch (thread_table[i].state) {
				case B_THREAD_SPAWNED:
				{
					pthread_t tid;

					if (pthread_create(&tid, NULL, (pthread_entry)thread_table[i].func,
										thread_table[i].data) == 0)
					{
						thread_table[i].pth = tid;
						thread_table[i].state = B_THREAD_RUNNING;
						return B_OK;
					}

					return B_ERROR;
				}

				case B_THREAD_SUSPENDED:
					pthread_kill(thread_table[i].pth, SIGCONT);
					thread_table[i].state = B_THREAD_RUNNING;
					return B_OK;

				default:
					return B_BAD_THREAD_STATE;
			}
		}
	}

	return B_BAD_THREAD_ID;
}


status_t
_kern_block_thread(uint32 flags, bigtime_t timeout)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
_kern_unblock_thread(thread_id thread, status_t status)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
_kern_unblock_threads(thread_id* threads, uint32 count,
	status_t status)
{
	UNIMPLEMENTED();
	return B_ERROR;
}
