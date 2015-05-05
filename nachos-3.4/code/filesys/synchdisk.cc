// synchdisk.cc 
//	Routines to synchronously access the disk.  The physical disk 
//	is an asynchronous device (disk requests return immediately, and
//	an interrupt happens later on).  This is a layer on top of
//	the disk providing a synchronous interface (requests wait until
//	the request completes).
//
//	Use a semaphore to synchronize the interrupt handlers with the
//	pending requests.  And, because the physical disk can only
//	handle one operation at a time, use a lock to enforce mutual
//	exclusion.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synchdisk.h"

//----------------------------------------------------------------------
// DiskRequestDone
// 	Disk interrupt handler.  Need this to be a C routine, because 
//	C++ can't handle pointers to member functions.
//----------------------------------------------------------------------

static void
DiskRequestDone (int arg)
{
    SynchDisk* disk = (SynchDisk *)arg;

    disk->RequestDone();
}

//----------------------------------------------------------------------
// SynchDisk::SynchDisk
// 	Initialize the synchronous interface to the physical disk, in turn
//	initializing the physical disk.
//
//	"name" -- UNIX file name to be used as storage for the disk data
//	   (usually, "DISK")
//----------------------------------------------------------------------

SynchDisk::SynchDisk(char* name)
{
    semaphore = new Semaphore("synch disk", 0);
    lock = new Lock("synch disk lock");
    disk = new Disk(name, DiskRequestDone, (int) this);
    diskBuffer = new DiskBuffer(this);
}

//----------------------------------------------------------------------
// SynchDisk::~SynchDisk
// 	De-allocate data structures needed for the synchronous disk
//	abstraction.
//----------------------------------------------------------------------

SynchDisk::~SynchDisk()
{
    delete diskBuffer;
    delete disk;
    delete lock;
    delete semaphore;
}

//----------------------------------------------------------------------
// SynchDisk::ReadSector
// 	Read the contents of a disk sector into a buffer.  Return only
//	after the data has been read.
//
//	"sectorNumber" -- the disk sector to read
//	"data" -- the buffer to hold the contents of the disk sector
//----------------------------------------------------------------------

void
SynchDisk::ReadSector(int sectorNumber, char* data)
{
    lock->Acquire();			// only one disk I/O at a time
    disk->ReadRequest(sectorNumber, data);
    semaphore->P();			// wait for interrupt
    lock->Release();
}

void
SynchDisk::ReadSectorFast(int sectorNumber, char* data)
{
    char *buf = diskBuffer->GetSectorContent(sectorNumber,true);
    bcopy(buf, data, SectorSize); //copy buf to data
}

//----------------------------------------------------------------------
// SynchDisk::WriteSector
// 	Write the contents of a buffer into a disk sector.  Return only
//	after the data has been written.
//
//	"sectorNumber" -- the disk sector to be written
//	"data" -- the new contents of the disk sector
//----------------------------------------------------------------------

void
SynchDisk::WriteSector(int sectorNumber, char* data)
{
    lock->Acquire();			// only one disk I/O at a time
    disk->WriteRequest(sectorNumber, data);
    semaphore->P();			// wait for interrupt
    lock->Release();
}

void
SynchDisk::WriteSectorFast(int sectorNumber, char* data)
{
    char *buf = diskBuffer->GetSectorContent(sectorNumber,false);
    bcopy(data, buf, SectorSize);
    
    WriteSector(sectorNumber,data); // write back
}

//----------------------------------------------------------------------
// SynchDisk::RequestDone
// 	Disk interrupt handler.  Wake up any thread waiting for the disk
//	request to finish.
//----------------------------------------------------------------------

void
SynchDisk::RequestDone()
{ 
    semaphore->V();
}

DiskBufferBlock::~DiskBufferBlock(){}

int DiskBuffer::SwapDown()
{
    int minHit = 999999;
    int find;
    for (int i = 0; i < DISK_BUFFER_NUM; ++i)
    {
        if (buffers[i]->sector == DISK_BUFFER_UNUSED)
        {
            return i;
        }
        if (buffers[i]->hit < minHit)
        {
            minHit = buffers[i]->hit;
            find = i;
        }
    }
    if (buffers[find]->dirty)
    {
        synchDisk->WriteSector(buffers[find]->sector,buffers[find]->content);
    }
    return find;
}

char* DiskBuffer::GetSectorContent(int sector,bool readOnly)
{
    for (int i = 0; i < DISK_BUFFER_NUM; ++i)
    {
        if (buffers[i]->sector == sector)
        {
            buffers[i]->hit ++;
            if (!readOnly)
            {
                buffers[i]->dirty = 1;
            }
            return buffers[i]->content;
        }
    }
    // swap a sector down
    int find = SwapDown();
    buffers[find]->sector = sector;
    buffers[find]->hit = 0;
    synchDisk->ReadSector(sector,buffers[find]->content);
    if (readOnly)
    {
        buffers[find]->dirty = 0;
    } else {
        buffers[find]->dirty = 1;
    }
    return buffers[find]->content;
}

DiskBuffer::~DiskBuffer()
{
    for (int i = 0; i < DISK_BUFFER_NUM; ++i)
    {
        delete buffers[i];
    }
}