//  invertedPage.h 

#ifndef InvertedPageEntry_H
#define InvertedPageEntry_H

#include "openfile.h"

class InvertedPageEntry {
  public:
    // int physicalPage;   // The page number in real memory (relative to the
            //  start of "mainMemory"
            
    int virtualPage;  	// The page number in virtual memory.

    bool valid;         // If this bit is set, the translation is ignored.
			// (In other words, the entry hasn't been initialized.)
             
    bool readOnly;	// If this bit is set, the user program is not allowed
			// to modify the contents of the page.
            
    bool use;           // This bit is set by the hardware every time the
			// page is referenced or modified.
             
    bool dirty;         // This bit is set by the hardware every time the
			          // page is modified.
             
    int  pid;  // thread id

    int  hit; // save hit time, hit min = most not used page
};

#define SWAPPages (NumPhysPages * 10)

class PageManager {
public:

    InvertedPageEntry invertedPageTable[NumPhysPages]; // inverted Page Table

    InvertedPageEntry swapPageTable[SWAPPages]; // Swap area often few times of Physical MEM; Now for test ! 

    PageManager();
    ~PageManager();

    int allocatePage(int virtAddr,bool isReadOnly);
    void deallocPage(int pid);
    void handlePageFault(int virtAddress);

    void clearTLB();

private:
    OpenFile *swapFile;
    int findEmptyPage(int from);
    int findPage(int from,int vpn,int pid);
    int findPageInSwap(int from,int vpn,int pid);

    int pageToBeSwapDown();
    int swapDownPage();
    void swapUpPage(int vpn,int pid,int swapPage);

    int hashVPN2PPN(unsigned int vpn,unsigned int pid);

    TranslationEntry *tlbToBeReplace();
    void updateTLB(int findedppn,int vpn); // findedppn != physicalPage

    void updateHitFromTLB();
    
    unsigned int getVPN(int addr);
    unsigned int getPID();

    unsigned int currentVPN;

    void printTable(); //For debug
};

#endif
