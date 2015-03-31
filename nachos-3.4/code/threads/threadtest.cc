// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "synch.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
    // when a simple thread finish, call TS
    currentThread->TS();
}

//=============================================================================
// SimpleThread Do nothing but run and finish
//=============================================================================
void 
SimpleThreadDoNothing(int which)
{
    printf("*** thread %d run\n", which);
}

//=============================================================================
// SimpleThread loop ticks
//=============================================================================
void 
SimpleThreadLoop(int which)
{
    for (int i = 0; i < 50; ++i)
    {
        printf("*** thread %d Loop %d times\n", which,i);
        interrupt->OneTick();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

//=============================================================================
// ThreadTest2
// Test max Thread limit
//=============================================================================
void
ThreadTest2()
{
    DEBUG('t', "Entering ThreadTest2");
    for (int num = 0; num < 128; num++) {
        Thread *t = new Thread("forked thread");
        t->Fork(SimpleThread, num);   
        // when num = 127, add the main thread,  will be over max thread limit
    }
    currentThread->Yield();
}

//=============================================================================
// ThreadTest3
// Test max Thread limit & pid free function, which will run normally
//=============================================================================
void
ThreadTest3()
{
    DEBUG('t', "Entering ThreadTest3");
    for (int num = 0; num < 127; num++) {
        Thread *t = new Thread(" thread");
        t->Fork(SimpleThreadDoNothing, num);   
    }
    currentThread->Yield();
    // all 127 forked thread should run and finish normally (FIFO)
    for (int num = 0; num < 127; num++) {
        Thread *t = new Thread("forked thread");
        t->Fork(SimpleThreadDoNothing, num);   
    }
    DEBUG('t', "Leaving ThreadTest3 normally");
    // all 127 forked thread should run and finish normally
}

//=============================================================================
// ThreadTest4
// Test priority
//=============================================================================
void
ThreadTest4()
{
    DEBUG('t', "Entering ThreadTest4");

    Thread *tLow = new Thread("forked thread low");
    tLow->setPriority(2); // set priority before fork
    tLow->Fork(SimpleThread, 1);

    Thread *tHigh = new Thread("forked thread high");
    tHigh->setPriority(0);
    tHigh->Fork(SimpleThread, 2);

    SimpleThread(0);
}

//=============================================================================
// ThreadTest5
// Test timer
//=============================================================================
void
ThreadTest5()
{
    DEBUG('t', "Entering ThreadTest5");    
    Thread *tHigh = new Thread("forked thread high");
    tHigh->setPriority(0);
    tHigh->Fork(SimpleThreadLoop, 1);
    Thread *tNormal = new Thread("forked thread Normal");
    tNormal->setPriority(1);
    tNormal->Fork(SimpleThreadLoop, 2);
    SimpleThreadLoop(3);
    //printf("LEAVE!!!!\n");
}

Lock* tLock6 = new Lock("ThreadTest6 Lock");

//=============================================================================
// SimpleThread lock Acquire and Release
//=============================================================================
void 
SimpleThreadLock(int which)
{
    printf("%d try to acquire lock %s\n", which, tLock6->getName());
    tLock6 -> Acquire();
    printf("%d Acquired lock %s\n RUNNING ... \n", which, tLock6->getName());
    for (int i = 0; i < 50; ++i) // Do some work!
    {
        interrupt->OneTick();
    }
    tLock6 -> Release();
    printf("%d Release lock %s\n", which, tLock6->getName());
}

//=============================================================================
// ThreadTest6
// Test lock
//=============================================================================

void
ThreadTest6()
{
    DEBUG('t', "Entering ThreadTest6");    
    Thread *t0 = new Thread("forked thread 0");
    t0->Fork(SimpleThreadLock, 0);
    Thread *t1 = new Thread("forked thread 1");
    t1->Fork(SimpleThreadLock, 1);
    Thread *t2 = new Thread("forked thread 2");
    t2->Fork(SimpleThreadLock, 2);
    Thread *t3 = new Thread("forked thread 3");
    t3->Fork(SimpleThreadLock, 3);
    currentThread -> Yield();
}

RWLock* rwlock;
int rwContent;
void reader(int which)
{
    printf("\t\t\t -%d- Try read...\n", which);
    rwlock->ReadBegin();
    for (int i = 0; i < 50; ++i) // Do some work!
    {
        interrupt->OneTick();
        printf("-%d- is Reading %d : %d/%d \n", which, rwContent, i, 50);
    }
    printf("\t\t\t -%d- Read : %d\n", which, rwContent);
    rwlock->ReadEnd();
    printf("\t\t\t -%d- Finish read...\n", which);
}

void writer(int which)
{
    printf("\t\t\t -%d- Try write...\n", which);
    rwlock->WriteBegin();
    for (int i = 0; i < 50; ++i) // Do some work!
    {
        interrupt->OneTick();
        printf("-%d- is Writing %d : %d/%d \n", which, which, i, 50);
    }
    rwContent = which;
    printf("\t\t\t -%d- Write %d into rwContent \n", which, which);
    rwlock->WriteEnd();
    printf("\t\t\t -%d- Finish write...\n", which);
}
//=============================================================================
// ThreadTest7
// Test RWLock with reader writer problem
//=============================================================================
void
ThreadTest7()
{
    DEBUG('t', "Entering ThreadTest7");    
    rwlock = new RWLock("ThreadTest7");
    rwContent = 0;
    Thread *t0 = new Thread("writer 0");
    t0->Fork(writer, 0);

    Thread *t1 = new Thread("reader 1");
    t1->Fork(reader, 1);

    Thread *t2 = new Thread("reader 2");
    t2->Fork(reader, 2);

    Thread *t3 = new Thread("writer 3");
    t3->Fork(writer, 3);

    Thread *t4 = new Thread("writer 4");
    t4->Fork(writer, 4);

    Thread *t5 = new Thread("reader 5");
    t5->Fork(reader, 5);
    currentThread -> Yield();
}

SynchBarrier *barrier;
void doSomeThingSynch(int which)
{
    printf("\t\t\t -%d- Do SomeThing Sync...\n", which);
    for (int i = 0; i < which * 10 ; ++i) // Do some work!
    {
        interrupt->OneTick();
        printf("-%d- is working.... : %d/%d \n", which, i, which * 10);
    }
    printf("\t\t\t -%d- Finish working and waiting for synch...\n", which);
    barrier->Enter();
    printf("\t\t\t -%d- Finish Sync !!!\n", which);
}
//=============================================================================
// ThreadTest8
// Test Barrier
//=============================================================================
void
ThreadTest8()
{
    DEBUG('t', "Entering ThreadTest8"); 
    barrier = new SynchBarrier("barrier", 3);
    Thread *t1 = new Thread("worker 1");
    t1->Fork(doSomeThingSynch, 1);

    Thread *t2 = new Thread("worker 2");
    t2->Fork(doSomeThingSynch, 2);

    Thread *t3 = new Thread("worker 3");
    t3->Fork(doSomeThingSynch, 3);
}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
    ThreadTest1();
    break;
    case 2:
    ThreadTest2();
    break;
    case 3:
    ThreadTest3();
    break;
    case 4:
    ThreadTest4();
    break;
    case 5:
    ThreadTest5();
    break;
    case 6:
    ThreadTest6();
    break;
    case 7:
    ThreadTest7(); // reader / writer problem
    break;
    case 8:
    ThreadTest8(); // barrier
    break;
    default:
	printf("No test specified.\n");
	break;
    }
}

