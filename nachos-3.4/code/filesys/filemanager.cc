//  filemanager.cc
#include "filemanager.h"
#include "openfile.h"


FileManager::FileManager()
{
	for (int i = 0; i < MAX_OPENFILE_NUM; ++i)
	{
		openedFile[i] = INVALID_FD;
		fileLocks[i] = NULL;
	}
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
 		openedFile[fd] = sector;
 		openedFileCount[fd] = 1;
 	}
 	return fd;
 }

 bool 
 FileManager::MarkClose(int fd)
 {
 	if (openedFile[fd] != INVALID_FD)
 	{
 		openedFileCount[fd] --;
 		if (openedFileCount[fd] == 0)
 		{
 			openedFile[fd] = INVALID_FD;
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
 	if (openedFile[fd] != INVALID_FD)
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
 	if (openedFile[fd] != INVALID_FD)
 	{
 		ASSERT(fileLocks[fd] != NULL);
 		fileLocks[fd] -> ReadEnd();
 	}
}

void 
FileManager::WriteStart(int fd)
{
 	if (openedFile[fd] != INVALID_FD)
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
 	if (openedFile[fd] != INVALID_FD)
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
		if(openedFile[i] == INVALID_FD)
			return i;
	}
	return INVALID_FD;
 }

int 
FileManager::findFD(int sector)
{
 	for (int i = 0; i < MAX_OPENFILE_NUM; ++i)
	{
		if(openedFile[i] == sector)
			return i;
	}
	return INVALID_FD;
}