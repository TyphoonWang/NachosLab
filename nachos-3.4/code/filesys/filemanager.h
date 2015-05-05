//  filemanager.h
//  

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "copyright.h"
#include "synch.h"

#define MAX_OPENFILE_NUM 10
#define INVALID_FD -1

class RWLock;

// call by openfile class
class FileManager {
  public:
  	FileManager();
    int MarkOpen(int sector);  // openedFileCount + 1, return fd
    bool MarkClose(int fd); // openedFileCount - 1. return if openedFileCount == 0 (can delete)

    void ReadStart(int fd);
    void ReadEnd(int fd);
    void WriteStart(int fd);
    void WriteEnd(int fd);

  private:
    int openedFile[MAX_OPENFILE_NUM]; 		// saved inode sector
    int openedFileCount[MAX_OPENFILE_NUM];  // how many thread is using a file
    RWLock* fileLocks[MAX_OPENFILE_NUM];    // locks handle read / write

    int allocateFD();
    int findFD(int sector);
};

#endif