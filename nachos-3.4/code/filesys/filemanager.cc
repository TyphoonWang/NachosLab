//  filemanager.cc
#include "filemanager.h"
#include "openfile.h"


FileManager::FileManager()
{
	for (int i = 0; i < MAX_OPENFILE_NUM; ++i)
	{
		openedFileSector[i] = INVALID_FD;
		fileLocks[i] = NULL;
	}
	// INIT fd 0 fd 1 here as STDIN STDOUT if needed.....
}

 int 
 FileManager::MarkOpen(int sector)
 {
 	int fd = findFD(sector);
 	if (fd != INVALID_FD)
 	{
 		openedFileCount[fd] ++;
 	}
 	else
 	{
 		fd = allocateFD();
 		openedFileSector[fd] = sector;
 		openedFileCount[fd] = 1;
 	}
 	return fd;
 }

 bool 
 FileManager::MarkClose(int fd)
 {
 	if (openedFileSector[fd] != INVALID_FD)
 	{
 		openedFileCount[fd] --;
 		if (openedFileCount[fd] == 0)
 		{
 			openedFileSector[fd] = INVALID_FD;
 			if(fileLocks[fd] != NULL)
 			{
 				delete fileLocks[fd];
 				fileLocks[fd] = NULL;
 			}
 			return true;
 		}
 	}
 	return false;
 }

void 
FileManager::ReadStart(int fd)
{
 	if (openedFileSector[fd] != INVALID_FD)
 	{
 		if (fileLocks[fd] == NULL)
 		{
 			fileLocks[fd] = new RWLock("File lock");
 		}
 		fileLocks[fd] -> ReadBegin();
 	}
}

void 
FileManager::ReadEnd(int fd)
{
 	if (openedFileSector[fd] != INVALID_FD)
 	{
 		ASSERT(fileLocks[fd] != NULL);
 		fileLocks[fd] -> ReadEnd();
 	}
}

void 
FileManager::WriteStart(int fd)
{
 	if (openedFileSector[fd] != INVALID_FD)
 	{
 		if (fileLocks[fd] == NULL)
 		{
 			fileLocks[fd] = new RWLock("File lock");
 		}
 		fileLocks[fd] -> WriteBegin();
 	}
}

void 
FileManager::WriteEnd(int fd)
{
 	if (openedFileSector[fd] != INVALID_FD)
 	{
 		ASSERT(fileLocks[fd] != NULL);
 		fileLocks[fd] -> WriteEnd();
 	}
}


 // === PRIVATE
 int 
 FileManager::allocateFD()
 {
 	for (int i = 0; i < MAX_OPENFILE_NUM; ++i)
	{
		if(openedFileSector[i] == INVALID_FD)
			return i;
	}
	return INVALID_FD;
 }

int 
FileManager::findFD(int sector)
{
 	for (int i = 0; i < MAX_OPENFILE_NUM; ++i)
	{
		if(openedFileSector[i] == sector){
			return i;
		}


	}
	return INVALID_FD;
}