//  synchconsole.h 
//	Data structures to simulate the behavior of a terminal
//	I/O device.  A terminal has two parts -- a keyboard input,
//	and a display output, each of which produces/accepts 
//	characters sequentially.
//
//	The console hardware device is synchronous. 

#ifndef SYNCHCONSOLE_H
#define SYNCHCONSOLE_H

#include "copyright.h"
#include "utility.h"
#include "console.h"
#include "synch.h"

class SynchConsole {
  public:
    SynchConsole(char *readFile, char *writeFile);
				// initialize the hardware console device
    ~SynchConsole();			// clean up console emulation

// external interface -- Nachos kernel code can call these
    void PutChar(char ch);	// Write "ch" to the console display,

    char GetChar();	   	// Poll the console input.  If a char is 
				// available, return it.  Otherwise, return EOF.
    				// "readHandler" is called whenever there is 
				// a char to be gotten
    void WriteDone();
    void ReadDone();
  private:
    Console *console;
    Lock *rlock;
    Semaphore *rSem;
    Lock *wlock;
    Semaphore *wSem;
};

#endif // SYNCHCONSOLE_H
