// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

Lock::Lock(char* debugName) 
{
    name = debugName;
    owner = currentThread;
    lockSem = new Semaphore("lock semaphore", 1);
}

Lock::~Lock() 
{
    delete lockSem;
}

void Lock::Acquire() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    lockSem->P();
    owner = currentThread;
    DEBUG('t', "Lock %s - Change owner to %s\n", name, currentThread->getName());
    (void) interrupt->SetLevel(oldLevel);
}

void Lock::Release() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(isHeldByCurrentThread())
    lockSem->V();
    (void) interrupt->SetLevel(oldLevel);
}

bool Lock::isHeldByCurrentThread()
{
    return currentThread == owner;
}

Condition::Condition(char* debugName) 
{
    name = debugName;
    conSem = new Semaphore("Condition semaphore", 0);
    waiting = 0;
}

Condition::~Condition() 
{
    delete conSem; 
}
                   
void Condition::Wait(Lock* conditionLock) 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(conditionLock->isHeldByCurrentThread());
    conditionLock->Release();     //releasing the lock and
    waiting ++;
    conSem->P();                  // going to sleep until csignal / broadcast
    conditionLock -> Acquire();   // then re-acquire the lock
    (void) interrupt->SetLevel(oldLevel);
}

void Condition::Signal(Lock* conditionLock) 
{ 
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    if (waiting > 0)
    {
        conSem -> V();
        waiting --;
    }
    (void) interrupt->SetLevel(oldLevel);
}

void Condition::Broadcast(Lock* conditionLock) 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    while (waiting > 0)
    {
        conSem -> V();
        waiting --;
    }
    (void) interrupt->SetLevel(oldLevel);
}

SynchBarrier::SynchBarrier(char* debugName, int maxThreadCount) 
{
    name = debugName;
    barrierSem  = new Semaphore("Barrier semaphore", 0);
    maxThread   = maxThreadCount;
    threadCount = 0;
}

SynchBarrier::~SynchBarrier()
{
    delete barrierSem;
}

// need call when a thread finish working
void SynchBarrier::Enter()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    threadCount ++;
    if (threadCount == maxThread)
    {
        for (int i = 0; i < threadCount - 1; ++i)
        {
            barrierSem -> V(); 
        }
    }
    else
    {
        barrierSem -> P();
    }
    (void) interrupt->SetLevel(oldLevel);
}

RWLock::RWLock(char* debugName)
{
    name = debugName;
    writeLock = new Lock("Write Lock");
    readLock  = new Lock("Read Lock");
    readerCount = 0;
}

RWLock::~RWLock()
{
    delete readLock;
    delete writeLock;
}

void RWLock::ReadBegin()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    DEBUG('t', "%s Acquiring WriteLock...\n", currentThread->getName());
    writeLock->Acquire(); // test if anyone is writing, or try writing
    DEBUG('t', "%s Acquired WriteLock!!!\n", currentThread->getName());
    writeLock->Release();
    DEBUG('t', "%s Give up WriteLock!!!\n", currentThread->getName());
    if (readerCount == 0)
    {
       DEBUG('t', "%s Acquiring ReadLock...\n", currentThread->getName());
       readLock->Acquire(); // mark someone is reading
       DEBUG('t', "%s Acquired ReadLock\n", currentThread->getName());
    }
    readerCount ++;
    (void) interrupt->SetLevel(oldLevel);
}

void RWLock::ReadEnd()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    readerCount --;
    if (readerCount == 0)
    {
        DEBUG('t', "%s Give up ReadLock\n", currentThread->getName());
        readLock->setOwner(currentThread);
        readLock->Release();
    }
    (void) interrupt->SetLevel(oldLevel);
}

void RWLock::WriteBegin()
{
    DEBUG('t', "%s Acquiring WriteLock...\n", currentThread->getName());
    writeLock ->Acquire(); // try writing
    DEBUG('t', "%s Acquired WriteLock!!\n", currentThread->getName());
    DEBUG('t', "%s Acquiring ReadLock...\n", currentThread->getName());
    readLock->Acquire();   // and no one reading
    DEBUG('t', "%s Acquired ReadLock!!\n", currentThread->getName());
}

void RWLock::WriteEnd()
{
    readLock->Release();
    DEBUG('t', "%s Give up ReadLock\n", currentThread->getName());
    writeLock ->Release();
    DEBUG('t', "%s Give up WriteLock\n", currentThread->getName());
}