// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
//
void PCIncrease()
{
    int PC = machine->ReadRegister(PCReg);
    int NextPC = machine->ReadRegister(NextPCReg); 
    machine->WriteRegister(PrevPCReg, PC); 
    machine->WriteRegister(PCReg, NextPC); 
    machine->WriteRegister(NextPCReg, NextPC + sizeof(int));
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
		DEBUG('a', "Shutdown, initiated by user program.\n");
   		interrupt->Halt();
    } 
    else if((which == SyscallException) && (type == SC_Exit)) {
        int arg1 = machine->ReadRegister(4);
        printf("[User program] Exit: %d\n", arg1);
        currentThread->Finish();
    }
    else if ((which == SyscallException) && (type == SC_Create))
    {
        int arg1 = machine->ReadRegister(4);
        char fileName[20];
        int val = 0;
        int i = 0;
        //Read name
        do{
            while(!machine->ReadMem(arg1, 1, & val));
            arg1 ++;
            fileName[i++] = (char)val;
        }while(val != 0);
        printf("creat file:%s\n", fileName);
        fileSystem->Create(fileName, 16);    
        PCIncrease();
    }
    else if ((which == SyscallException) && (type == SC_Open))
    {
        int arg1 = machine->ReadRegister(4);
        char fileName[20];
        int val = 0;
        int i = 0;
        //Read name
        do{
            while(!machine->ReadMem(arg1, 1, & val));
            arg1 ++;
            fileName[i++] = (char)val;
        }while(val != 0);
        printf("open file:%s\n", fileName);
        int s = fileSystem->FindSector(fileName);
        int retVal = fileManager->MarkOpen(s);
        machine->WriteRegister(2,retVal);
        PCIncrease();
    }
    else if ((which == SyscallException) && (type == SC_Close))
    {
        int arg1 = machine->ReadRegister(4);
        int retVal = fileManager->MarkClose(arg1);
        PCIncrease();
    }
    else if ((which == SyscallException) && (type == SC_Read))
    {
        int buffer = machine->ReadRegister(4);
        int size = machine->ReadRegister(5);
        int fd = machine->ReadRegister(6);
        OpenFile *f = new OpenFile(fileManager->getSector(fd));
        char buf[32];
        int readed = 0;
        int addr = buffer;
        while (size > 0)
        {
            int r = f->Read(buf,32);
            for (int i = 0; i < r; ++i)
            {
                int val = buf[i];
                while(!machine->WriteMem(addr, 1, val));
                addr ++;
                size -= 1;
            }  
            readed += r;
        }
        delete f;
        printf("read file:%d\n", fd);
        machine->WriteRegister(2,readed);
        PCIncrease();
    }
    else if ((which == SyscallException) && (type == SC_Write))
    {
        int buffer = machine->ReadRegister(4);
        int size = machine->ReadRegister(5);
        int fd = machine->ReadRegister(6);
        OpenFile *f = new OpenFile(fileManager->getSector(fd));

        char data[256];
        int val = 0;
        int i = 0;
        do{
            while(!machine->ReadMem(buffer, 1, & val));
            buffer ++;
            data[i++] = (char)val;
        }while(val != 0);
        f->Write(data,size);
        delete f;
        printf("write file %d\n",fd);
        PCIncrease();
    }
    else if(which == PageFaultException) {
    	int addr = machine->ReadRegister(BadVAddrReg);
    	pageManager->handlePageFault(addr);
	}
    else {
        printf("OverflowException %d, IllegalInstrException %d\n",OverflowException,IllegalInstrException);
		printf("Unexpected user mode exception %d %d\n", which, type);
		ASSERT(FALSE);
    }
}
