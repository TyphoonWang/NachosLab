// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "filehdr.h"
#include "indfilehdr.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 

    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors) {
        DEBUG('f', "Allocate: not enough space\n");
	   return FALSE;		// not enough space
    }

    if (numSectors <= NumDirect)
    {
        DEBUG('f', "Allocate: small file!\n");
        for (int i = 0; i < numSectors; i++)
            dataSectors[i] = freeMap->Find();
    }
    else
    {
        DEBUG('f', "Allocate: large file! Sectors count: %d\n",numSectors);
        // need use indirect block
        for (int i = 0; i < NumDirect; i++)
            dataSectors[i] = freeMap->Find();
        int remainSectors = numSectors - NumDirect;
        int idx = 0; 
        while (remainSectors > 0)
        {
            int findSector = freeMap->Find(); // For Indirect Secotor
            indirectSectors[idx] = findSector;

            IndFileHeader *ihd = new IndFileHeader();
            for (int i = 0; i < NumDirectIN && remainSectors > 0; ++i, --remainSectors)
            {
                DEBUG('f', "Allocate: Sectors %d\n",i);
                ihd -> SetSector(i,freeMap->Find()); // For Actruall Data
            }
            ihd->WriteBack(findSector);
            idx++;
            delete ihd;
        }
    }

    createTime = time(0);
    return TRUE;
}

bool FileHeader::ReAllocate(BitMap *freeMap, int newSize)
{
    int newNumSectors  = divRoundUp(newSize, SectorSize);
    if (newNumSectors < numSectors) // do not need ReAllocate
    {
         return true;
    }
    else
    {
        int remainSectorsIdx = numSectors - NumDirect;

        int indirectIdx = remainSectorsIdx / NumDirectIN;
        int indirectOffset = remainSectorsIdx - indirectIdx * NumDirectIN;

        int remainSectors = newNumSectors - numSectors;

        if (freeMap->NumClear() < remainSectors) {
            DEBUG('f', "REAllocate: not enough space\n");
            return FALSE;        // not enough space
        }
        
        IndFileHeader *ihd = new IndFileHeader();
        if (indirectSectors[indirectIdx] <= 0)
        {
            int findSector = freeMap->Find(); // For Indirect Secotor
            indirectSectors[indirectIdx] = findSector;
            ihd->WriteBack(findSector);
        }
        ihd->FetchFrom(indirectSectors[indirectIdx]);
        for (int i = indirectOffset + 1; i < NumDirectIN && remainSectors > 0; ++i, --remainSectors)
        {
            DEBUG('f', "ReAllocate: Sectors %d\n",i);
            ihd -> SetSector(i,freeMap->Find()); // For Actruall Data
        }

        // new indirect block!
        int idx = indirectIdx + 1;
        while(remainSectors > 0)
        {
            int findSector = freeMap->Find(); // For Indirect Secotor
            indirectSectors[idx] = findSector;

             IndFileHeader *ihd = new IndFileHeader();
            for (int i = 0; i < NumDirectIN && remainSectors > 0; ++i, --remainSectors)
            {
                DEBUG('f', "ReAllocate(new indrect block): Sectors %d\n",i);
                ihd -> SetSector(i,freeMap->Find()); // For Actruall Data
            }
            ihd->WriteBack(findSector);
            idx++;
            delete ihd;
        }
        return true;
    }
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    if (numSectors <= NumDirect)
    {    
        for (int i = 0; i < numSectors; i++) {
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
            freeMap->Clear((int) dataSectors[i]);
        }
    }
    else
    {
        // need use indirect block
        for (int i = 0; i < NumDirect; i++)
        {
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
            freeMap->Clear((int) dataSectors[i]);
        }
        int remainSectors = numSectors - NumDirect;
        int idx = 0; 
        while (remainSectors > 0)
        {
            int findSector = indirectSectors[idx];

            IndFileHeader *ihd = new IndFileHeader();
            ihd->FetchFrom(findSector);
            for (int i = 0; i < NumDirectIN && remainSectors > 0; ++i, --remainSectors)
            {
                int s = ihd -> GetSector(i); 
                ASSERT(freeMap->Test(s));  // ought to be marked!
                freeMap->Clear(s);
            }
            idx++;
            delete ihd;
        }
    }

}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    DEBUG('f', "FileHeader::FetchFrom. Sector %d\n", sector);
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    int sectorIdx = offset / SectorSize;
    if (sectorIdx < NumDirect)
    {    
        return(dataSectors[sectorIdx]);
    }
    else
    {
        int remainSectorsIdx = sectorIdx - NumDirect;
        int indirectIdx = remainSectorsIdx / NumDirectIN;
        int indirectOffset = remainSectorsIdx - indirectIdx * NumDirectIN;

        IndFileHeader *ihd = new IndFileHeader();
        ihd->FetchFrom(indirectSectors[indirectIdx]);
        int s = ihd->GetSector(indirectOffset);
        delete ihd;
        DEBUG('f', "ByteToSector: sectorIdx: %d, remainSectorsIdx: %d, indirectIdx: %d, indirectOffset:%d, Sectors %d\n",
            sectorIdx,remainSectorsIdx,indirectIdx,indirectOffset,s);
        return s;
    }
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];



    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    if (numSectors <= NumDirect)
    {    
        for (int i = 0; i < numSectors; i++) {
             printf("%d ", dataSectors[i]);
        }
    }
    else
    {
        for (int i = 0; i < NumDirect; i++)
        {
             printf("%d ", dataSectors[i]);
        }
        int remainSectors = numSectors - NumDirect;
        int idx = 0; 
        while (remainSectors > 0)
        {
            int findSector = indirectSectors[idx];

            IndFileHeader *ihd = new IndFileHeader();
            ihd->FetchFrom(findSector);
            for (int i = 0; i < NumDirectIN && remainSectors > 0; ++i, --remainSectors)
            {
                int s = ihd -> GetSector(i); 
                 printf("%d ", s);
            }
            idx++;
            delete ihd;
        }
    }
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	   synchDisk->ReadSector(ByteToSector(i * SectorSize), data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	       if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		      printf("%c", data[j]);
            else
		      printf("\\%x", (unsigned char)data[j]);
	   }
        printf("\n"); 
    }
    delete [] data;
}
