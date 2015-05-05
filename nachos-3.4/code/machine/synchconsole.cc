//  synchconsole.cc
//  

#include "synchconsole.h"

static void
ConsoleWriteDone (int arg)
{
    SynchConsole* synCon = (SynchConsole*)arg;
    synCon->WriteDone();
}

static void
ConsoleReadDone (int arg)
{
    SynchConsole* synCon = (SynchConsole*)arg;
    synCon->ReadDone();
}

SynchConsole::SynchConsole(char *readFile, char *writeFile)
{
	rSem = new Semaphore("synch console read", 0);
	wSem = new Semaphore("synch console write", 0);
    rlock = new Lock("synch disk read lock");
    wlock = new Lock("synch disk write lock");
	console = new Console(readFile,writeFile,ConsoleReadDone,ConsoleWriteDone,(int)this);
}

void
SynchConsole::PutChar(char ch)
{
	wlock->Acquire();			
	console->PutChar(ch);
    wSem->P();			// wait for interrupt
    wlock->Release();
}

char
SynchConsole::GetChar()
{
	rlock->Acquire();			
	char c = console->GetChar();
    rSem->P();			// wait for interrupt
    rlock->Release();
    return c;
}

void 
SynchConsole::ReadDone()
{
	rSem->V();
}

void 
SynchConsole::WriteDone()
{
	wSem->V();
}