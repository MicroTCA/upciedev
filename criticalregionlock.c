/*
 *
 *  criticalregionlock.c
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
#include "criticalregionlock.h"

#ifdef USE_SEMAPHORE

static inline int LockCritRegionPrivate(struct SCriticalRegionLock* a_pLock, enum _SEM_LOCK_TYPE_ a_lockType);
static inline int UnlockCritRegionPrivate(struct SCriticalRegionLock* a_pLock, enum _SEM_LOCK_TYPE_ a_nUnlockType);

void InitCritRegionLock(struct SCriticalRegionLock* a_pLock, long a_lnTimeoutMs)
{
	memset(a_pLock, 0, sizeof(struct SCriticalRegionLock));

	a_pLock->m_lnPID = -1;
	a_pLock->m_nInCriticalRegion = 0;
	a_pLock->m_ulnJiffiesTmOut = msecs_to_jiffies(a_lnTimeoutMs);

	sema_init(&a_pLock->m_semaphore, 1);
	spin_lock_init(&a_pLock->m_spinlock);
}
EXPORT_SYMBOL(InitCritRegionLock);



void DestroyCritRegionLock(struct SCriticalRegionLock* lock)
{
}
EXPORT_SYMBOL(DestroyCritRegionLock);



int EnterCritRegion(struct SCriticalRegionLock* a_pLock)
{
	//long lnPID = (long)current->group_leader->pid;
	long lnPID = (long)current->pid;

	spin_lock(&a_pLock->m_spinlock);
	if (unlikely(lnPID == a_pLock->m_lnPID))
	{
		a_pLock->m_nInCriticalRegion = 1;
		spin_unlock(&a_pLock->m_spinlock);
		return 0;
	} // if (unlikely(lnPID == a_pLock->m_lnPID))
	else
	{
		spin_unlock(&a_pLock->m_spinlock);
		return LockCritRegionPrivate(a_pLock, _SHORT_LOCK_UNLOCK_);
	} // else of if (unlikely(lnPID == a_pLock->m_lnPID))

	return 0;
}
EXPORT_SYMBOL(EnterCritRegion);



int LeaveCritRegion(struct SCriticalRegionLock* a_pLock)
{
	return UnlockCritRegionPrivate(a_pLock, _SHORT_LOCK_UNLOCK_);
}
EXPORT_SYMBOL(LeaveCritRegion);



int LongLockOfCriticalRegion(struct SCriticalRegionLock* a_pLock)
{

	//long lnPID = (long)current->group_leader->pid;
	long lnPID = (long)current->pid;

	spin_lock(&a_pLock->m_spinlock);
	if (unlikely(lnPID == a_pLock->m_lnPID))
	{
		spin_unlock(&a_pLock->m_spinlock);
		return _ALREADY_LOCKED_;
	} // if (unlikely(lnPID == a_pLock->m_lnPID))
	else
	{
		int nRet;
		spin_unlock(&a_pLock->m_spinlock);
		nRet = LockCritRegionPrivate(a_pLock, _LONG_LOCK_UNLOCK_);//(long)current->group_leader->pid
	} // else of if (unlikely(lnPID == a_pLock->m_lnPID))

	return 0;
}
EXPORT_SYMBOL(LongLockOfCriticalRegion);



int UnlockLongCritRegion(struct SCriticalRegionLock* a_pLock)
{
	return UnlockCritRegionPrivate(a_pLock, _LONG_LOCK_UNLOCK_);
}
EXPORT_SYMBOL(UnlockLongCritRegion);




//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
static inline int UnlockCritRegionPrivate(struct SCriticalRegionLock* a_pLock, enum _SEM_LOCK_TYPE_ a_nUnlockType)
{
	//long lnPID = (long)current->group_leader->pid;
	long lnPID = (long)current->pid;
	int nReturn = 0;

	spin_lock(&a_pLock->m_spinlock);
	switch (a_nUnlockType)
	{
	case _SHORT_LOCK_UNLOCK_:
		if (likely(a_pLock->m_nLockingType == _SHORT_LOCK_UNLOCK_)){ goto unlockSemaphore; }
		else if (a_pLock->m_nShouldBeFreed)
		{
			nReturn = _TIMEOUT_;
			goto unlockSemaphore;
		}
		else
		{
			goto simpleLeavingReg;
		}
		break;

	case _LONG_LOCK_UNLOCK_:
		if (likely(a_pLock->m_lnPID == lnPID)){ goto unlockSemaphore; }
		else
		{
			spin_unlock(&a_pLock->m_spinlock);
			return _NOT_LOCK_OWNER__;
		}
		break;

	default:
		spin_unlock(&a_pLock->m_spinlock);
		return _NOT_IMPLEMENTED_;
	}

	return _NOT_IMPLEMENTED_;

unlockSemaphore:
	up(&a_pLock->m_semaphore);
	a_pLock->m_lnPID = -1;
	a_pLock->m_nShouldBeFreed = 0;
	a_pLock->m_nLockingType = _NO_LOCK_;
simpleLeavingReg:
	a_pLock->m_nInCriticalRegion = 0;
	spin_unlock(&a_pLock->m_spinlock);

	return nReturn;

}



static inline int LockCritRegionPrivate(struct SCriticalRegionLock* a_pLock, enum _SEM_LOCK_TYPE_ a_lockType)
{
	int nIter = 0;
	pid_t lnPidToKill = 0;
	//long lnPID = (long)current->group_leader->pid;
	long lnPID = (long)current->pid;

	for (; nIter < 100; ++nIter)
	{
		if (unlikely(down_timeout(&a_pLock->m_semaphore, a_pLock->m_ulnJiffiesTmOut)))
		{
			spin_lock(&a_pLock->m_spinlock);
			if ((a_pLock->m_nLockingType == _LONG_LOCK_UNLOCK_) &&
				(get_jiffies_64() - a_pLock->m_ullnStartJiffi>a_pLock->m_ulnJiffiesTmOut))
			{
				if (a_pLock->m_nInCriticalRegion)
				{
					a_pLock->m_nShouldBeFreed = 1;
				}
				else
				{
					up(&a_pLock->m_semaphore); // before upping m_nInCriticalregion flag was checked
					a_pLock->m_nLockingType = _NO_LOCK_;
					lnPidToKill = a_pLock->m_lnPID;
					a_pLock->m_lnPID = -1;
				}
			}
			spin_unlock(&a_pLock->m_spinlock);

			if (lnPidToKill){ struct siginfo aSigInfo; KillPidWithInfo(lnPidToKill, SIGALRM, &aSigInfo); lnPidToKill = 0; }

		} // if (unlikely(down_timeout(&a_pLock->m_semaphore, a_pLock->m_ulnJiffiesTmOut)))
		else
		{
			spin_lock(&a_pLock->m_spinlock);
			a_pLock->m_nLockingType = a_lockType;
			a_pLock->m_nInCriticalRegion = 1;
			a_pLock->m_nShouldBeFreed = 0;
			a_pLock->m_lnPID = lnPID;
			a_pLock->m_ullnStartJiffi = get_jiffies_64();
			spin_unlock(&a_pLock->m_spinlock);
			return 0;

		} // else  of  if (unlikely(down_timeout(&a_pLock->m_semaphore, a_pLock->m_ulnJiffiesTmOut)))

	} // for (; nIter < 10; ++nIter)

	return -ETIME;

}


#endif  /* #ifdef USE_SEMAPHORE */



int KillPidWithInfo(pid_t a_unPID, int a_nSignal, struct siginfo* a_pInfo)
{
	struct pid* pPidStr = find_vpid(a_unPID);
	struct task_struct * pTask = pPidStr ? pid_task(pPidStr, PIDTYPE_PID) : NULL;
	const struct cred* pCred = pTask ? pTask->cred : NULL;

	if (pCred)
	{
		a_pInfo->si_signo = a_nSignal;
		a_pInfo->si_code = SI_QUEUE;
		return kill_pid_info_as_cred(a_nSignal, a_pInfo, pPidStr, pCred, 0);
	}

	return -1;
}
EXPORT_SYMBOL(KillPidWithInfo);

