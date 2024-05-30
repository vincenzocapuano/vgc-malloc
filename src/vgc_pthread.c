//
// Copyright (C) 2013-2024 by Vincenzo Capuano
//
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "vgc_common.h"
#include "vgc_message.h"
#include "vgc_pthread.h"


static const char *moduleName = "PTHREAD";


ATTR_PUBLIC bool PTHREAD_mutexLock(pthread_mutex_t *mutex)
{
	switch(pthread_mutex_lock(mutex)) {
		case 0:			return true;

		case EOWNERDEAD:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutex_lock", "Warning", "status EOWNERDEAD", 0);
					return PTHREAD_mutexConsistent(mutex);

		case ENOTRECOVERABLE:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutex_lock", "Error", "status ENOTRECOVERABLE", 0);
					if (!PTHREAD_mutexDestroy(mutex)) return false;
					return false;

		case EAGAIN:		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutex_lock", "Error", "status EAGAIN", 0);
					return false;

		case EDEADLK:		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutex_lock", "Error", "status EDEADLK", 0);
					return false;

		case ENOMEM:		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutex_lock", "Error", "status ENOMEM", 0);
					return false;

		default:		vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutex_lock", "Error", "unknown error", 0);
					return false;
	}
}


ATTR_PUBLIC bool PTHREAD_mutexUnlock(pthread_mutex_t *mutex)
{
	switch(pthread_mutex_unlock(mutex)) {
		case 0:		return true;

		case EPERM:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutex_unlock", "Error", "status EPERM", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutex_unlock", "Error", "unknown error", 0);
				return false;
	}
}	


ATTR_PUBLIC bool PTHREAD_mutexInit(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr)
{
	memset(mutex, 0, sizeof(*mutex));
	switch(pthread_mutex_init(mutex, attr)) {
		case 0:		return true;

		case EBUSY:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutex_init", "Error", "status EBUSY", 0);
				return false;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutex_init", "Error", "status EINVAL", 0);
				return false;

		case EFAULT:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutex_init", "Error", "status FAULT", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutex_init", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_mutexDestroy(pthread_mutex_t *mutex)
{
	switch(pthread_mutex_destroy(mutex)) {
		case 0:		return true;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutex_destroy", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutex_destroy", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_mutexConsistent(pthread_mutex_t *mutex)
{
	switch(pthread_mutex_consistent(mutex)) {
		case 0:		return true;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutex_consistent", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutex_consistent", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_mutexattrInit(pthread_mutexattr_t *attr)
{
	bool ret;

	switch(pthread_mutexattr_init(attr)) {
		case 0:		ret = PTHREAD_mutexattrSetrobust(attr, PTHREAD_MUTEX_ROBUST);
				ret = ret && PTHREAD_mutexattrSetpshared(attr, PTHREAD_PROCESS_SHARED);
				ret = ret && PTHREAD_mutexattrSettype(attr, PTHREAD_MUTEX_ERRORCHECK);
				return ret;

		case ENOMEM:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutexattr_init", "Error", "status ENOMEM", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutexattr_init", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_mutexattrDestroy(pthread_mutexattr_t *attr)
{
	switch(pthread_mutexattr_destroy(attr)) {
		case 0:		return true;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutexattr_destroy", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutexattr_destroy", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_mutexattrGetrobust(const pthread_mutexattr_t *restrict attr, int *restrict robust)
{
	switch(pthread_mutexattr_getrobust(attr, robust)) {
		case 0:		return true;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutexattr_getrobust", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutexattr_getrobust", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_mutexattrSetrobust(pthread_mutexattr_t *attr, int robust)
{
	switch(pthread_mutexattr_setrobust(attr, robust)) {
		case 0:		return true;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutexattr_setrobust", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutexattr_setrobust", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_mutexattrSetpshared(pthread_mutexattr_t *attr, int pshared)
{
	switch(pthread_mutexattr_setpshared(attr, pshared)) {
		case 0:		return true;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutexattr_setpshared", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutexattr_setpshared", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_mutexattrGetpshared(const pthread_mutexattr_t *restrict attr, int *restrict pshared)
{
	switch(pthread_mutexattr_getpshared(attr, pshared)) {
		case 0:		return true;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutexattr_getpshared", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutexattr_getpshared", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_mutexattrGettype(const pthread_mutexattr_t *restrict attr, int *restrict type)
{
	switch(pthread_mutexattr_gettype(attr, type)) {
		case 0:		return true;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutexattr_gettype", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutexattr_gettype", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_mutexattrSettype(pthread_mutexattr_t *attr, int type)
{
	switch(pthread_mutexattr_settype(attr, type)) {
		case 0:		return true;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutexattr_settype", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_mutexattr_settype", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_rwlockInit(pthread_rwlock_t *restrict rwlock, const pthread_rwlockattr_t *restrict attr)
{
	switch(pthread_rwlock_init(rwlock, attr)) {
		case 0:		return true;

		case EAGAIN:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_init", "Error", "status EAGAIN", 0);
				return false;

		case ENOMEM:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_init", "Error", "status ENOMEM", 0);
				return false;

		case EPERM:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_init", "Error", "status EPERM", 0);
				return false;

		case EBUSY:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_init", "Error", "status EBUSY", 0);
				return false;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_init", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_init", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_rwlockDestroy(pthread_rwlock_t *rwlock)
{
	switch(pthread_rwlock_destroy(rwlock)) {
		case 0:		return true;

		case EBUSY:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_destroy", "Error", "status EBUSY", 0);
				return false;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_destroy", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_destroy", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_rwlockRdLock(pthread_rwlock_t *rwlock)
{
	switch(pthread_rwlock_rdlock(rwlock)) {
		case 0:		return true;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_rdlock", "Error", "status EINVAL", 0);
				return false;

		case EAGAIN:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_rdlock", "Error", "status EAGAIN", 0);
				return false;

		case EDEADLK:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_rdlock", "Error", "status EDEADLK", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_rdlock", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_rwlockTryRdLock(pthread_rwlock_t *rwlock)
{
	switch(pthread_rwlock_tryrdlock(rwlock)) {
		case 0:		return true;

		case EBUSY:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_tryrdlock", "Error", "status EBUSY", 0);
				return false;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_tryrdlock", "Error", "status EINVAL", 0);
				return false;

		case EAGAIN:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_tryrdlock", "Error", "status EAGAIN", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_tryrdlock", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_rwlockWrLock(pthread_rwlock_t *rwlock)
{
	switch(pthread_rwlock_wrlock(rwlock)) {
		case 0:		return true;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_wrlock", "Error", "status EINVAL", 0);
				return false;

		case EDEADLK:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_wrlock", "Error", "status EDEADLK", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_wrlock", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_rwlockTryWrLock(pthread_rwlock_t *rwlock)
{
	switch(pthread_rwlock_trywrlock(rwlock)) {
		case 0:		return true;

		case EBUSY:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_trywrlock", "Error", "status EBUSY", 0);
				return false;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_trywrlock", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_trywrlock", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_rwlockTimedRdLock(pthread_rwlock_t *restrict rwlock, const struct timespec *restrict abs_timeout)
{
	switch(pthread_rwlock_timedrdlock(rwlock, abs_timeout)) {
		case 0:		return true;

		case ETIMEDOUT:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_timedrdlock", "Error", "status ETIMEDOUT", 0);
				return false;

		case EAGAIN:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_timedrdlock", "Error", "status EAGAIN", 0);
				return false;

		case EDEADLK:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_timedrdlock", "Error", "status EDEADLK", 0);
				return false;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_timedrdlock", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_timedrdlock", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_rwlockTimedWrLock(pthread_rwlock_t *restrict rwlock, const struct timespec *restrict abs_timeout)
{
	switch(pthread_rwlock_timedwrlock(rwlock, abs_timeout)) {
		case 0:		return true;

		case ETIMEDOUT:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_timedwrlock", "Error", "status ETIMEDOUT", 0);
				return false;

		case EDEADLK:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_timedwrlock", "Error", "status EDEADLK", 0);
				return false;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_timedwrlock", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_timedwrlock", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_rwlockUnlock(pthread_rwlock_t *rwlock)
{
	switch(pthread_rwlock_unlock(rwlock)) {
		case 0:		return true;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_unlock", "Error", "status EINVAL", 0);
				return false;

		case EPERM:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_unlock", "Error", "status EPERM", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlock_unlock", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_rwlockattrInit(pthread_rwlockattr_t *attr)
{
	switch(pthread_rwlockattr_init(attr)) {
		case 0:		return PTHREAD_rwlockattrSetpshared(attr, PTHREAD_PROCESS_SHARED);

		case ENOMEM:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlockattr_init", "Error", "status ENOMEM", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlockattr_init", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_rwlockattrDestroy(pthread_rwlockattr_t *attr)
{
	switch(pthread_rwlockattr_destroy(attr)) {
		case 0:		return true;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlockattr_destroy", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlockattr_destroy", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_rwlockattrSetpshared(pthread_rwlockattr_t *attr, int pshared)
{
	switch(pthread_rwlockattr_setpshared(attr, pshared)) {
		case 0:		return true;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlockattr_setpshared", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlockattr_setpshared", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_rwlockattrGetpshared(const pthread_rwlockattr_t *restrict attr, int *restrict pshared)
{
	switch(pthread_rwlockattr_getpshared(attr, pshared)) {
		case 0:		return true;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlockattr_getpshared", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_rwlockattr_getpshared", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *args)
{
	switch (pthread_create(thread, attr, start_routine, args)) {
		case 0:		return true;

		case EAGAIN:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_create", "Error", "status EAGAIN", 0);
				return false;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_create", "Error", "status EINVAL", 0);
				return false;

		case EPERM:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_create", "Error", "status EPERM", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_create", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_kill(pthread_t thread, int sig)
{
	switch (pthread_kill(thread, sig)) {
		case 0:		return true;

		case ESRCH:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_kill", "Error", "status ESRCH", 0);
				return false;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_kill", "Error", "status EINVAL", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_kill", "Error", "unknown error", 0);
				return false;
	}
}

ATTR_PUBLIC bool PTHREAD_join(pthread_t thread, void **retval)
{
	switch (pthread_join(thread, retval)) {
		case 0:		return true;

		case EDEADLK:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_join", "Error", "status EDEADLK", 0);
				return false;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_join", "Error", "status EINVAL", 0);
				return false;

		case ESRCH:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_join", "Error", "status ESRCH", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_join", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_detach(pthread_t thread)
{
	switch (pthread_detach(thread)) {
		case 0:		return true;

		case EINVAL:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_detach", "Error", "status EINVAL", 0);
				return false;

		case ESRCH:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_detach", "Error", "status ESRCH", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_detach", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC bool PTHREAD_cancel(pthread_t thread)
{
	switch (pthread_cancel(thread)) {
		case 0:		return true;

		case ESRCH:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_cancel", "Error", "status ESRCH", 0);
				return false;

		default:	vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "pthread_cancel", "Error", "unknown error", 0);
				return false;
	}
}


ATTR_PUBLIC pthread_t PTHREAD_self(void)
{
	return pthread_self();
}
