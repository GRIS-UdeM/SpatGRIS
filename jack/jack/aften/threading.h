/********************************************************************************
 * Copyright (C) 2005-2007 by Prakash Punnoor                                   *
 * prakash@punnoor.de                                                           *
 *                                                                              *
 * This library is free software; you can redistribute it and/or                *
 * modify it under the terms of the GNU Lesser General Public                   *
 * License as published by the Free Software Foundation; either                 *
 * version 2 of the License                                                     *
 *                                                                              *
 * This library is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            *
 * Lesser General Public License for more details.                              *
 *                                                                              *
 * You should have received a copy of the GNU Lesser General Public             *
 * License along with this library; if not, write to the Free Software          *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA *
 ********************************************************************************/
#ifndef THREADING_H
#define THREADING_H

#include "common.h"

typedef enum
{
    START,
    WORK,
    END,
    ABORT
} ThreadState;

#ifdef HAVE_POSIX_THREADS
#include <pthread.h>

typedef pthread_t       THREAD;
typedef pthread_mutex_t MUTEX;
typedef pthread_cond_t  COND;

typedef struct A52GlobalThreadSync
{
    int current_thread_num;
    int threads_to_abort;
    volatile int samples_thread_num;
    MUTEX samples_mutex;
} A52GlobalThreadSync;

typedef struct A52ThreadSync
{
    THREAD thread;
    MUTEX enter_mutex;
    MUTEX confirm_mutex;
    COND  enter_cond;
    COND  confirm_cond;

    COND samples_cond;
    COND* next_samples_cond;
} A52ThreadSync;

#define thread_create(threadid, threadfunc, threadparam) \
    pthread_create(threadid, NULL, (void *(*) (void *))threadfunc, threadparam)
#define thread_join(x)         pthread_join(x, NULL)

#define posix_mutex_init(x)          pthread_mutex_init(x, NULL)
#define posix_mutex_destroy(x)       pthread_mutex_destroy(x)
#define posix_mutex_lock(x)          pthread_mutex_lock(x)
#define posix_mutex_unlock(x)        pthread_mutex_unlock(x)

#define posix_cond_init(x)           pthread_cond_init(x, NULL)
#define posix_cond_destroy(x)        pthread_cond_destroy(x)
#define posix_cond_wait(cond, mutex) pthread_cond_wait(cond, mutex)
#define posix_cond_signal(x)         pthread_cond_signal(x)
#define posix_cond_broadcast(x)      pthread_cond_broadcast(x)


#ifdef HAVE_GET_NPROCS
#include <sys/sysinfo.h>

static inline int
get_ncpus()
{
    return get_nprocs();
}
#elif defined(SYS_DARWIN)
#include <sys/sysctl.h>

static inline int
get_ncpus()
{
    int mib[2] = { CTL_HW, HW_NCPU };
    int numCPUs;
    size_t len = sizeof(numCPUs);

    return sysctl(mib, 2, &numCPUs, &len, NULL, 0) ? 1 : numCPUs;
}
#else

static inline int
get_ncpus()
{
    return NUM_THREADS;
}
#endif

#else /* HAVE_POSIX_THREADS */
#ifdef HAVE_WINDOWS_THREADS
#include <windows.h>

typedef HANDLE THREAD;
typedef HANDLE EVENT;
typedef CRITICAL_SECTION CS;

typedef struct A52GlobalThreadSync
{
    int current_thread_num;
    int threads_to_abort;
    volatile int samples_thread_num;
    CS samples_cs;
} A52GlobalThreadSync;

typedef struct A52ThreadSync
{
    THREAD thread;
    EVENT ready_event;
    EVENT enter_event;
    EVENT samples_event;
    EVENT* next_samples_event;
} A52ThreadSync;

static inline void
thread_create(HANDLE *thread, int (*threadfunc)(void*), LPVOID threadparam)
{
    DWORD thread_id;
    *thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadfunc,
                           threadparam, 0, &thread_id);
}

static inline void
thread_join(HANDLE thread)
{
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

static inline void
windows_event_init(EVENT *event)
{
    *event = CreateEvent(NULL, FALSE, FALSE, NULL);
}

static inline void
windows_event_destroy(EVENT *event)
{
    CloseHandle(*event);
}

static inline void
windows_event_set(EVENT *event)
{
    SetEvent(*event);
}

static inline void
windows_event_reset(EVENT *event)
{
    ResetEvent(*event);
}

static inline void
windows_event_wait(EVENT *event)
{
    WaitForSingleObject(*event, INFINITE);
}

static inline void
windows_cs_init(CS *cs)
{
    InitializeCriticalSection(cs);
}

static inline void
windows_cs_destroy(CS *cs)
{
    DeleteCriticalSection(cs);
}

static inline void
windows_cs_enter(CS *cs)
{
    EnterCriticalSection(cs);
}

static inline void
windows_cs_leave(CS *cs)
{
    LeaveCriticalSection(cs);
}


static inline int
get_ncpus()
{
    SYSTEM_INFO sys_info;

    GetSystemInfo(&sys_info);
    return sys_info.dwNumberOfProcessors;
}
#else /* HAVE_WINDOWS_THREADS */

#define NO_THREADS

static inline int
get_ncpus()
{
    return 1;
}

#define thread_create(X, Y, Z)
#define thread_join(X)

#endif /* HAVE_WINDOWS_THREADS */
#endif /* HAVE_POSIX_THREADS */

#ifndef HAVE_POSIX_THREADS
#define posix_mutex_init(x)
#define posix_mutex_destroy(x)
#define posix_mutex_lock(x)
#define posix_mutex_unlock(x)

#define posix_cond_init(x)
#define posix_cond_destroy(x)
#define posix_cond_wait(cond, mutex)
#define posix_cond_signal(x)
#define posix_cond_broadcast(x)
#endif /* HAVE_POSIX_THREADS */

#ifndef HAVE_WINDOWS_THREADS
#define windows_event_init(x)
#define windows_event_destroy(x)
#define windows_event_set(x)
#define windows_event_reset(x)
#define windows_event_wait(x)

#define windows_cs_init(x)
#define windows_cs_destroy(x)
#define windows_cs_enter(x)
#define windows_cs_leave(x)
#endif /* HAVE_WINDOWS_THREADS */

#endif /* THREADING_H */
