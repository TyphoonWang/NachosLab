// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "noff.h"

#define UserStackSize		1024 	// increase this as necessary!

class OpenFile;

class AddrSpace {
  public:
    AddrSpace(char* filename);	// Create an address space,
					// initializing it with the program
					// stored in the file "fileName"
    ~AddrSpace();			// De-allocate an address space

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch 
   
    bool isFinishInit() { return finishInit; } // if all AddrSpace is initialized in Memory or Swap area
    bool initSpace(int virtualPageNum, int physicalPageNum);

  private:
    NoffHeader noffH;
    OpenFile *executable;

    int numPages;		// Number of pages in the virtual 
					// address space
    int   codePages; // Number of code pages (always save in file, load when necessary)
    int   dataPages; // Number of data pages (first load from file, save in swap area)

    

    bool  finishInit; // if all AddrSpace is initialized in Memory or Swap area
};

#endif // ADDRSPACE_H
