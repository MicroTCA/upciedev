/*
 *
 *  criticalregionlock.h
 *
 *  Created on: Jul 06, 2015
 *	Author: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *	Source file for implementation of new syncronization object.
 *
 *  This synchronization object makes possible to lock and
 *  unlock it from different contexes. This makes possible to lock
 *  the object confirm several driver accesses with intermediate
 *  calculations in user space.
 *  In the case of gilty program kernel driver is available to unlock the sync. object.
 *
 *  This synchronization object is candidate to change mutex
 *
 */
#ifndef __criticalregionlock_h__
#define __criticalregionlock_h__

//#define USE_SEMAPHORE	// by defining this all drivers will use new 
						// locking mechanism

#define	_DEFAULT_TIMEOUT_			500 //ms
#define	_TIMEOUT_					-3
#define	_ALREADY_LOCKED_			-4
#define	_NOT_LOCK_OWNER__			-5
#define	_NOT_IMPLEMENTED_			-6

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/pid.h>

#ifdef USE_SEMAPHORE
#include <linux/semaphore.h>
#else
#include <linux/mutex.h>
#endif

enum _SEM_LOCK_TYPE_{_NO_LOCK_,_SHORT_LOCK_UNLOCK_,_LONG_LOCK_UNLOCK_};

extern int KillPidWithInfo(pid_t pid, int sigNum, struct siginfo* info);
//extern int KillPidWithInfo(pid_t a_unPID, int a_nSignal, struct siginfo* a_pInfo);

#ifdef USE_SEMAPHORE
typedef struct SCriticalRegionLock
{
	struct semaphore	m_semaphore;
	spinlock_t			m_spinlock;

	long				m_lnPID;
	int					m_nInCriticalRegion : 1;
	int					m_nShouldBeFreed : 1;
	int					m_nLockingType : 4;
	u64					m_ulnJiffiesTmOut;
	u64					m_ullnStartJiffi;

}SCriticalRegionLock;
#else
#define	SCriticalRegionLock mutex
#endif


#ifdef USE_SEMAPHORE
extern void	InitCritRegionLock(struct SCriticalRegionLock* lock, long timeoutMs);
extern void	DestroyCritRegionLock(struct SCriticalRegionLock* lock);
extern int EnterCritRegion(struct SCriticalRegionLock* lock);
extern int LeaveCritRegion(struct SCriticalRegionLock* lock);
extern int LongLockOfCriticalRegion(struct SCriticalRegionLock* lock);
extern int UnlockLongCritRegion(struct SCriticalRegionLock* lock);
#else
#define	InitCritRegionLock(lock,timeout) mutex_init((lock))
#define	DestroyCritRegionLock(lock) do{}while(0)
#define	EnterCritRegion(lock) mutex_lock_interruptible((lock))
//#define	EnterCritRegion(lock) mutex_lock((lock))
static inline int LeaveCritRegion(struct SCriticalRegionLock* lock){mutex_unlock((lock));return 0;}
static inline int LongLockOfCriticalRegion(struct SCriticalRegionLock* lock){return _NOT_IMPLEMENTED_;}
static inline int UnlockLongCritRegion(struct SCriticalRegionLock* lock){return _NOT_IMPLEMENTED_;}
#endif

extern int KillPidWithInfo(pid_t a_unPID, int a_nSignal, struct siginfo* a_pInfo);

#endif  /* #ifndef __criticalregionlock_h__ */