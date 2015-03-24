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
    default:
	printf("No test specified.\n");
	break;
    }
}

