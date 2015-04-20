//  invertedPage.cc
//  hash VA to PA 

#include "machine.h"
#include "invertedPage.h"
#include "system.h"
// ------------------------------- PUBLIC ---------------------------------------------
PageManager::PageManager()
{
	//Open swap file
	swapFile = fileSystem->Open("../vm/swap5");

	for (int i = 0; i < NumPhysPages; ++i)
	{
		invertedPageTable[i].valid = FALSE;
	}
}

PageManager::~PageManager()
{
	delete swapFile;
}

int PageManager::allocatePage(int virtAddr,bool isReadOnly)
{
	unsigned int vpn = getVPN(virtAddr);
	unsigned int pid = getPID();
	int ppnFrom = hashVPN2PPN(vpn, pid);
	int emptyppn = findEmptyPage(ppnFrom);
	invertedPageTable[emptyppn].pid = pid;
	invertedPageTable[emptyppn].virtualPage = vpn;
	invertedPageTable[emptyppn].use   = FALSE;
	invertedPageTable[emptyppn].dirty = FALSE;
	invertedPageTable[emptyppn].readOnly = isReadOnly;
}

void PageManager::handlePageFault(int virtAddress)
{
	DEBUG('a', "### PageManager: handlePageFault,virtualAddr = %d \n", 
                    virtAddress);
	unsigned int vpn = getVPN(virtAddress);
	unsigned int pid = getPID();
	int ppnFrom = hashVPN2PPN(vpn, pid);
	
	int finded = findPage(ppnFrom,vpn,pid);
	DEBUG('a', "### PageManager: page finded = %d \n", 
                    finded);
	if (finded >= 0) // PageFault case 1: in memory but not in TLB
	{
		// load page entry to tlb 
		updateTLB(finded,vpn);
	} 
	else 
	{   // PageFault case 2: not in memory

		finded = findPageInSwap(ppnFrom,vpn,pid); // find page in swap
		
		if (finded >= 0)
		{
			// swap from disk !
			DEBUG('a', "### PageManager: swap from disk, finded PPN = %d \n", 
                    finded);
			swapUpPage(vpn,pid,finded);
		}
		else
		{
			finded = findEmptyPage(ppnFrom);
			AddrSpace *space = currentThread->space;
			DEBUG('a', "### PageManager: initSpace from file, finded PPN = %d \n", 
                    finded);
			bool readOnly = space->initSpace(vpn,finded);
			invertedPageTable[finded].virtualPage = vpn;
			invertedPageTable[finded].pid = pid;
			invertedPageTable[finded].use   = FALSE;
			invertedPageTable[finded].dirty = FALSE;
			invertedPageTable[finded].readOnly = readOnly; // read only decided by initSpace
			invertedPageTable[finded].valid = TRUE;
		}
	}

	//printTable();
}

// ------------------------------- PRIVATE ---------------------------------------------
// 
TranslationEntry * PageManager::tlbToBeReplace()
{
	// random swap ...
	static int i = 0;
	i = i + 3;
	i %= TLBSize;
	//printf("Find TLB TO BE REPLACE %d\n", i);
	return &(machine->tlb[i]);
}

void PageManager::updateTLB(int findedppn,int vpn)
{
	DEBUG('a', "### PageManager::updateTLB,findedppn = %d,\t vpn = %d \n", 
                    findedppn, vpn);
	TranslationEntry *oldTLB; // tlb enrty to be replaced
	oldTLB = tlbToBeReplace();

	oldTLB->valid = TRUE;
	oldTLB->virtualPage= vpn;
	oldTLB->physicalPage = findedppn;
	oldTLB->use = 0;
	oldTLB->dirty = 0;
	oldTLB->readOnly = invertedPageTable[findedppn].readOnly;
}

int PageManager::findPage(int from,int vpn,int pid)
{
	for (int i = from; i < NumPhysPages; ++i)
	{
		if (invertedPageTable[i].virtualPage == vpn && 
			invertedPageTable[i].valid && 
			invertedPageTable[i].pid == pid)
		{
			return i;
		}
	}
	for (int i = 0; i < from; ++i)
	{
		if (invertedPageTable[i].virtualPage == vpn && 
			invertedPageTable[i].valid && 
			invertedPageTable[i].pid == pid)
		{
			return i;
		}
	}
	return -1; //not found!
}

int PageManager::findPageInSwap(int from,int vpn,int pid)
{
	for (int i = from; i < SWAPPages; ++i)
	{
		if (swapPageTable[i].virtualPage == vpn && 
			swapPageTable[i].valid && 
			swapPageTable[i].pid == pid)
		{
			return i;
		}
	}
	for (int i = 0; i < from; ++i)
	{
		if (swapPageTable[i].virtualPage == vpn && 
			swapPageTable[i].valid && 
			swapPageTable[i].pid == pid)
		{
			return i;
		}
	}
	return -1; //not found!
}


int PageManager::findEmptyPage(int from)
{
	DEBUG('a', "### PageManager finding EmptyPage ...\n");
	for (int i = from; i < NumPhysPages; ++i)
	{
		if (invertedPageTable[i].valid == FALSE)
		{
			return i;
		}
	}
	for (int i = 0; i < from; ++i)
	{
		if (invertedPageTable[i].valid == FALSE)
		{
			return i;
		}
	}
	return swapDownPage(); //not found swap a page down and return a valid page
}

int PageManager::pageToBeSwapDown()
{
	static int i = 0;
	i = i + 3;
	i %= NumPhysPages;
	return i;
}

int PageManager::swapDownPage()
{
	DEBUG('a', "================================PageManage swap Down Page ... \n");
	int ppn = pageToBeSwapDown();
	int physicalAddr = ppn * PageSize;
	for (int i = 0; i < SWAPPages; ++i) // find a swap area
	{
		if (swapPageTable[i].valid == FALSE)
		{
			int inSwapAddr = i * PageSize;
			swapFile->WriteAt(&(machine->mainMemory[physicalAddr]),
			PageSize, inSwapAddr);
			swapPageTable[i].valid = TRUE;
			swapPageTable[i].pid = invertedPageTable[ppn].pid;
			swapPageTable[i].virtualPage = invertedPageTable[ppn].virtualPage;
			swapPageTable[i].readOnly = invertedPageTable[ppn].readOnly;
		}
		return ppn;
	}
	// should never reach here!
	ASSERT(FALSE);
	return -1;
}

void PageManager::swapUpPage(int vpn, int pid, int swapPage)
{
	DEBUG('a', "==================================PageManage swap UP Page ... \n");
	int ppnFrom = hashVPN2PPN(vpn, pid);
	int finded = findEmptyPage(ppnFrom); // find an empty page in memory
	int physicalAddr = finded * PageSize;
	int inSwapAddr = swapPage * PageSize;
	swapFile->ReadAt(&(machine->mainMemory[physicalAddr]),PageSize, inSwapAddr);
	swapPageTable[swapPage].valid = FALSE;
	invertedPageTable[finded].readOnly = swapPageTable[swapPage].readOnly;
	invertedPageTable[finded].virtualPage = vpn;
	invertedPageTable[finded].pid = pid;
	invertedPageTable[finded].use = FALSE;
	invertedPageTable[finded].dirty = FALSE;
	return;
}

int PageManager::hashVPN2PPN(unsigned int vpn,unsigned int pid) // hash based on visual addredd and thread id
{
	return (vpn + pid * 314) % NumPhysPages;
}

unsigned int PageManager::getVPN(int addr)
{
	return (unsigned) addr / PageSize;
}

unsigned int PageManager::getPID()
{
	return (unsigned)currentThread->getPid();
}

void printPageEntry(int ppn)
{
	InvertedPageEntry e = pageManager->invertedPageTable[ppn];
	printf("PPN   : %d \t   VPN : %d \t",ppn,e.virtualPage);
	printf("Valid : %d \t   PID : %d \n",e.valid,e.pid);
}

void PageManager::printTable()
{
	printf("----------------------Inverted Page Table : ----------------------------------\n");
	for (int i = 0; i < NumPhysPages; ++i)
	{
		printPageEntry(i);
	}
	printf("--------------------------------------------------------\n");
}

