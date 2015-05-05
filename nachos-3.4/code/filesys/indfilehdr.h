// indfilehdr.h
// Sector hold indirect file 
#ifndef INDIRFILEHDR_H
#define INDIRFILEHDR_H

#include "filehdr.h"

#define NumDirectIN 	(( SectorSize ) / sizeof(int))

class IndFileHeader {
  public:

    void FetchFrom(int sector) {
         synchDisk->ReadSector(sector, (char *)this);
    } 	// Initialize file header from disk

    void WriteBack(int sector){ 
        synchDisk->WriteSector(sector, (char *)this); 
    } 	// Write modifications to file header back to disk

    void SetSector(int sectorIdx,int physicalSector)
    {
        dataSectors[sectorIdx] = physicalSector;
    }

    int GetSector(int sectorIdx)
    {
        return dataSectors[sectorIdx];
    }

  private:
    int dataSectors[NumDirectIN];		// Disk sector numbers for each data 
};

#endif // INDIRFILEHDR_H
