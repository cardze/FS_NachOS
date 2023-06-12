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
#include "debug.h"
#include "synchdisk.h"
#include "main.h"

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

bool FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize)
{
    numBytes = fileSize;
    numSectors = divRoundUp(fileSize, SectorSize);
    // Init single and Double
    SingleIndirectSector = -1;
    DoubleIndirectSector = -1;
    if (freeMap->NumClear() < numSectors)
        return FALSE; // not enough space

    int sectorCounter;

    for (sectorCounter = 0; (sectorCounter < NumDirect) && (sectorCounter < numSectors); sectorCounter++)
    {
        dataSectors[sectorCounter] = freeMap->FindAndSet();
        // since we checked that there was enough free space,
        // we expect this to succeed
        ASSERT(dataSectors[sectorCounter] >= 0);
    }
    if (sectorCounter == numSectors)
    {
        DEBUG(dbgFile, "Direct pointer is enough for " << fileSize << " size file.\n\n");
        return TRUE;
    }
    int rested_size = numBytes - (NumDirect * SectorSize);
    // Let's go single indirect
    // 4KB limit (5KB -> 7KB)
    if (this->AllocateSingleIndirect(freeMap))
    {
        return TRUE;
    }
    else
    { // The single indirect is full...
        rested_size -= (NumIndirect * SectorSize);
        return AllocateDoubleIndirect(freeMap);
    }
    return TRUE;
}

bool FileHeader::AllocateSingleIndirect(PersistentBitmap *freeMap)
{
    SingleIndirectPointer *single_indirect = new SingleIndirectPointer;
    // create it.
    SingleIndirectSector = freeMap->FindAndSet();
    single_indirect->numsSector = 0;
    for (single_indirect->numsSector = 0;
         (single_indirect->numsSector < NumIndirect) && ((single_indirect->numsSector + NumDirect) < this->numSectors);
         single_indirect->numsSector++)
    {
        single_indirect->dataSectors[single_indirect->numsSector] = freeMap->FindAndSet();
        // since we checked that there was enough free space,
        // we expect this to succeed
        ASSERT(single_indirect->dataSectors[single_indirect->numsSector] >= 0);
    }
    kernel->synchDisk->WriteSector(SingleIndirectSector, (char *)single_indirect);
    if (single_indirect->numsSector == NumIndirect)
    {
        return FALSE;
    }
    // delete single_indirect;
    return TRUE;
}

bool FileHeader::AllocateDoubleIndirect(PersistentBitmap *freeMap)
{
    int sector_counter = NumIndirect + NumDirect;
    DoubleIndirectPointer *double_indirect = new DoubleIndirectPointer;
    DoubleIndirectSector = freeMap->FindAndSet();
    double_indirect->numsSector = 0;

    while (sector_counter <= this->numSectors)
    {
        SingleIndirectPointer *single = new SingleIndirectPointer;
        single->numsSector = 0;
        double_indirect->pointers[double_indirect->numsSector] = freeMap->FindAndSet();
        for (single->numsSector = 0;
             (single->numsSector < NumIndirect) && (single->numsSector + sector_counter < this->numSectors);
             single->numsSector++)
        {
            single->dataSectors[single->numsSector] = freeMap->FindAndSet();
            // since we checked that there was enough free space,
            // we expect this to succeed
            ASSERT(single->dataSectors[single->numsSector] >= 0);
        }
        sector_counter += NumIndirect;
        double_indirect->numsSector++;
        if (double_indirect->numsSector >= NumIndirect)
            return FALSE;
        kernel->synchDisk->WriteSector(double_indirect->pointers[double_indirect->numsSector], (char *)single);
        DEBUG(dbgFile, "Open Double indirect data table "<<double_indirect->numsSector<<" \n\n")
    }

    kernel->synchDisk->WriteSector(DoubleIndirectSector, (char *)double_indirect);
    // delete double_indirect;
    return TRUE;
}
//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(PersistentBitmap *freeMap)
{
    SingleIndirectPointer *single_temp = new SingleIndirectPointer;
    DoubleIndirectPointer *double_temp = new DoubleIndirectPointer;
    for (int i = 0; i < numSectors && i < NumDirect; i++)
    {
        ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
        freeMap->Clear((int)dataSectors[i]);
    }
    if (SingleIndirectSector != -1)
    {
        kernel->synchDisk->ReadSector(SingleIndirectSector, (char *)single_temp);
        for (int i = single_temp->numsSector - 1; i >= 0; i--)
        {
            ASSERT(freeMap->Test((int)single_temp->dataSectors[i])); // ought to be marked!
            freeMap->Clear((int)single_temp->dataSectors[i]);
        }
        ASSERT(freeMap->Test((int)SingleIndirectSector)); // ought to be marked!
        freeMap->Clear((int)SingleIndirectSector);
        SingleIndirectSector = -1;
    }
    if (DoubleIndirectSector != -1)
    {
        kernel->synchDisk->ReadSector(DoubleIndirectSector, (char *)double_temp);
        for (int i = double_temp->numsSector - 1; i >= 0; i--)
        {
            single_temp = new SingleIndirectPointer;
            kernel->synchDisk->ReadSector(double_temp->pointers[i], (char *)single_temp);
            for (int j = single_temp->numsSector - 1; j >= 0; j--)
            {
                ASSERT(freeMap->Test((int)single_temp->dataSectors[j])); // ought to be marked!
                freeMap->Clear((int)single_temp->dataSectors[j]);
            }
            ASSERT(freeMap->Test((int)double_temp->pointers[i])); // ought to be marked!
            freeMap->Clear((int)double_temp->pointers[i]);
        }
        ASSERT(freeMap->Test((int)DoubleIndirectSector)); // ought to be marked!
        freeMap->Clear((int)DoubleIndirectSector);
        DoubleIndirectSector = -1;
    }
    // delete single_temp;
    // delete double_temp;
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void FileHeader::FetchFrom(int sector)
{
    kernel->synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void FileHeader::WriteBack(int sector)
{
    kernel->synchDisk->WriteSector(sector, (char *)this);
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

int FileHeader::ByteToSector(int offset)
{
    SingleIndirectPointer *single_temp = new SingleIndirectPointer;
    DoubleIndirectPointer *double_temp = new DoubleIndirectPointer;
    int offset_sector = offset / SectorSize;
    if (offset_sector < NumDirect)
    {
        return (dataSectors[offset_sector]);
    }
    else
    {
        if (offset_sector < NumIndirect + NumDirect)
        {
            kernel->synchDisk->ReadSector(SingleIndirectSector, (char *)single_temp);
            return (single_temp->dataSectors[offset_sector - NumDirect]);
        }
        else
        {
            DEBUG(dbgFile, "Double indirect ByteToSector offset = "<<(offset)<<" \n\n")
            
            single_temp = new SingleIndirectPointer;
            kernel->synchDisk->ReadSector(DoubleIndirectSector, (char *)double_temp);
            DEBUG(dbgFile, "Double indirect ByteToSector table index = "<<(offset_sector - NumDirect - NumIndirect)<<" \n\n")

            int index = double_temp->pointers[(offset_sector - NumDirect - NumIndirect) / NumIndirect];
            DEBUG(dbgFile, "Double indirect ByteToSector index "<<(index)<<" \n\n")
            
            kernel->synchDisk->ReadSector(index, (char *)single_temp);
            DEBUG(dbgFile, "Double indirect ByteToSector sector = "<<single_temp->dataSectors[((offset_sector - NumDirect - NumIndirect)%NumIndirect)]<<" \n\n")
            return (single_temp->dataSectors[(offset_sector - NumDirect - NumIndirect) % NumIndirect]);
        }
    }
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
        printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++)
    {
        kernel->synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
        {
            if ('\040' <= data[j] && data[j] <= '\176') // isprint(data[j])
                printf("%c", data[j]);
            else
                printf("\\%x", (unsigned char)data[j]);
        }
        printf("\n");
    }
    delete[] data;
}
