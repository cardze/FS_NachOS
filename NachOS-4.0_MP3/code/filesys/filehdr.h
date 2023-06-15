// filehdr.h
//	Data structures for managing a disk file header.
//
//	A file header describes where on disk to find the data in a file,
//	along with other information about the file (for instance, its
//	length, owner, etc.)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "pbitmap.h"

#define NumDirect ((SectorSize - 4 * sizeof(int)) / sizeof(int))
#define NumIndirect ((SectorSize - 1 * sizeof(int)) / sizeof(int))
#define MaxFileSize (NumDirect * SectorSize)

// The following class defines the Nachos "file header" (in UNIX terms,
// the "i-node"), describing where on disk to find all of the data in the file.
// The file header is organized as a simple table of pointers to
// data blocks.
//
// The file header data structure can be stored in memory or on disk.
// When it is on disk, it is stored in a single sector -- this means
// that we assume the size of this data structure to be the same
// as one disk sector.  Without indirect addressing, this
// limits the maximum file length to just under 4K bytes.
//
// There is no constructor; rather the file header can be initialized
// by allocating blocks for the file (if it is a new file), or by
// reading it from disk.

class FileHeader
{
public:
  bool Allocate(PersistentBitmap *bitMap, int fileSize); // Initialize a file header,
                                                         //  including allocating space
                                                         //  on disk for the file data

  bool AllocateSingleIndirect(PersistentBitmap *freeMap);
  bool AllocateDoubleIndirect(PersistentBitmap *freeMap);
  void Deallocate(PersistentBitmap *bitMap); // De-allocate this file's
                                             //  data blocks

  void FetchFrom(int sectorNumber); // Initialize file header from disk
  void WriteBack(int sectorNumber); // Write modifications to file header
                                    //  back to disk

  int ByteToSector(int offset); // Convert a byte offset into the file
                                // to the disk sector containing
                                // the byte
  
  int FileLength();             // Return the length of the file
                                // in bytes

  void Print(); // Print the contents of the file.

private:
  int numBytes;               // Number of bytes in the file
  int numSectors;             // Number of data sectors in the file
  int dataSectors[NumDirect]; // Disk sector numbers for each data
                              // block in the file
  int SingleIndirectSector;   // if the data is bigger than max file size,
                              // go to this data structure to continue.
                              // this give another 128 * 31 Byte
                              // so finally single indirect will support to
                              // 128 *(28+31) Bytes (about 7KB)
  int DoubleIndirectSector;   // to support 32KB , we need more indirect pointer
                              // this attemp will support
                              // 128 *(28+31+31*31) Bytes (about 130KB)
};

class SingleIndirectPointer
{
public:
  int numsSector;               // Number of data sectors in the single indirect pointer
  int dataSectors[NumIndirect]; // extented data sector
};

class DoubleIndirectPointer
{
public:
  int numsSector;            // Number of activated indirect pointer in the double indirect pointer
  int pointers[NumIndirect]; // extented data sector
};

#endif // FILEHDR_H
