//
// Copyright (C) 2013-2024 by Vincenzo Capuano
//
#pragma once

#include <pthread.h>
#include <stdbool.h>


bool PTHREAD_mutexInit(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);
bool PTHREAD_mutexDestroy(pthread_mutex_t *mutex);
bool PTHREAD_mutexLock(pthread_mutex_t *mutex);
bool PTHREAD_mutexUnlock(pthread_mutex_t *mutex);

bool PTHREAD_mutexConsistent(pthread_mutex_t *mutex);

bool PTHREAD_mutexattrInit(pthread_mutexattr_t *attr);
bool PTHREAD_mutexattrDestroy(pthread_mutexattr_t *attr);
bool PTHREAD_mutexattrSetpshared(pthread_mutexattr_t *attr, int pshared);
bool PTHREAD_mutexattrGetpshared(const pthread_mutexattr_t *restrict attr, int *restrict pshared);
bool PTHREAD_mutexattrGetrobust(const pthread_mutexattr_t *restrict attr, int *restrict robust);
bool PTHREAD_mutexattrSetrobust(pthread_mutexattr_t *attr, int robust);
bool PTHREAD_mutexattrGettype(const pthread_mutexattr_t *restrict attr, int *restrict type);
bool PTHREAD_mutexattrSettype(pthread_mutexattr_t *attr, int type);

bool PTHREAD_rwlockInit(pthread_rwlock_t *restrict rwlock, const pthread_rwlockattr_t *restrict attr);
bool PTHREAD_rwlockDestroy(pthread_rwlock_t *rwlock);
bool PTHREAD_rwlockRdLock(pthread_rwlock_t *rwlock);
bool PTHREAD_rwlockTryRdLock(pthread_rwlock_t *rwlock);
bool PTHREAD_rwlockWrLock(pthread_rwlock_t *rwlock);
bool PTHREAD_rwlockTryWrLock(pthread_rwlock_t *rwlock);
bool PTHREAD_rwlockTimedRdLock(pthread_rwlock_t *restrict rwlock, const struct timespec *restrict abs_timeout);
bool PTHREAD_rwlockTimedWrLock(pthread_rwlock_t *restrict rwlock, const struct timespec *restrict abs_timeout);
bool PTHREAD_rwlockUnlock(pthread_rwlock_t *rwlock);

bool PTHREAD_rwlockattrInit(pthread_rwlockattr_t *attr);
bool PTHREAD_rwlockattrDestroy(pthread_rwlockattr_t *attr);
bool PTHREAD_rwlockattrSetpshared(pthread_rwlockattr_t *attr, int pshared);
bool PTHREAD_rwlockattrGetpshared(const pthread_rwlockattr_t *restrict attr, int *restrict pshared);

bool PTHREAD_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *args);
bool PTHREAD_kill(pthread_t thread, int sig);
bool PTHREAD_join(pthread_t thread, void **retval);
bool PTHREAD_detach(pthread_t thread);
bool PTHREAD_cancel(pthread_t thread);

pthread_t PTHREAD_self(void);
