// filesys.cc
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "disk.h"
#include "pbitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"

#ifdef FILESYS_STUB

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known
// sectors, so that they can be located on boot-up.
#define FreeMapSector 0
#define DirectorySector 1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number
// of files that can be loaded onto the disk.
#define FreeMapFileSize (NumSectors / BitsInByte)
#define NumDirEntries 10
#define DirectoryFileSize (sizeof(DirectoryEntry) * NumDirEntries)

FileSystem::FileSystem(bool format)
{
    DEBUG(dbgFile, "Initializing the file system.BRuh");
    if (format)
    {
        PersistentBitmap *freeMap = new PersistentBitmap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);
        FileHeader *mapHdr = new FileHeader;
        FileHeader *dirHdr = new FileHeader;

        DEBUG(dbgFile, "Formatting the file system.");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FreeMapSector);
        freeMap->Mark(DirectorySector);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
        ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));

        // Flush the bitmap and directory FileHeaders back to disk
        // We need to do this before we can "Open" the file, since open
        // reads the file header off of disk (and currently the disk has garbage
        // on it!).

        DEBUG(dbgFile, "Writing headers back to disk.");
        mapHdr->WriteBack(FreeMapSector);
        dirHdr->WriteBack(DirectorySector);

        // OK to open the bitmap and directory files now
        // The file system operations assume these two files are left open
        // while Nachos is running.

        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);

        // Once we have the files "open", we can write the initial version
        // of each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG(dbgFile, "Writing bitmap and directory back to disk.");
        freeMap->WriteBack(freeMapFile); // flush changes to disk
        directory->WriteBack(directoryFile);

        if (debug->IsEnabled('f'))
        {
            freeMap->Print();
            directory->Print();
        }
        delete freeMap;
        delete directory;
        delete mapHdr;
        delete dirHdr;
    }
    else
    {
        // if we are not formatting the disk, just open the files representing
        // the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
    }
    DEBUG(dbgFile, "Finish initializing the file system.");
}
#else

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known
// sectors, so that they can be located on boot-up.
#define FreeMapSector 0
#define DirectorySector 1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number
// of files that can be loaded onto the disk.
#define FreeMapFileSize (NumSectors / BitsInByte)
#define NumDirEntries 64
#define DirectoryFileSize (sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{
    DEBUG(dbgFile, "Initializing the file system.");
    if (format)
    {
        PersistentBitmap *freeMap = new PersistentBitmap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);
        FileHeader *mapHdr = new FileHeader;
        FileHeader *dirHdr = new FileHeader;

        DEBUG(dbgFile, "Formatting the file system.");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FreeMapSector);
        freeMap->Mark(DirectorySector);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
        ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));

        // Flush the bitmap and directory FileHeaders back to disk
        // We need to do this before we can "Open" the file, since open
        // reads the file header off of disk (and currently the disk has garbage
        // on it!).

        DEBUG(dbgFile, "Writing headers back to disk.");
        mapHdr->WriteBack(FreeMapSector);
        dirHdr->WriteBack(DirectorySector);

        // OK to open the bitmap and directory files now
        // The file system operations assume these two files are left open
        // while Nachos is running.

        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);

        // pwd implement
        currentDirectoryFile = new OpenFile(DirectorySector); // root dir
        currentDirectory = new Directory(NumDirEntries);
        currentDirectory->FetchFrom(currentDirectoryFile);

        // Once we have the files "open", we can write the initial version
        // of each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG(dbgFile, "Writing bitmap and directory back to disk.");
        freeMap->WriteBack(freeMapFile); // flush changes to disk
        directory->WriteBack(directoryFile);

        if (debug->IsEnabled('f'))
        {
            freeMap->Print();
            directory->Print();
        }
        delete freeMap;
        delete mapHdr;
        delete dirHdr;
    }
    else
    {
        // if we are not formatting the disk, just open the files representing
        // the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);

        // pwd implement
        currentDirectoryFile = new OpenFile(DirectorySector); // root dir
        currentDirectory = new Directory(NumDirEntries);
        currentDirectory->FetchFrom(currentDirectoryFile);
    }
    DEBUG(dbgFile, "Finish initializing the file system.");
}

// destructor of filesystem class
FileSystem::~FileSystem()
{
    if (currentDirectoryFile != NULL)
        delete currentDirectoryFile;
    if (currentDirectory != NULL)
        delete currentDirectory;
    delete freeMapFile;
    delete directoryFile;
}

// split the path name
int FileSystem::splitPath(char **arr, char *path)
{
    char *tmp_path = (char *)calloc(30, sizeof(char));
    strncpy(tmp_path, path, 30);
    int ret = 0;
    char *tmp;
    tmp = strtok(tmp_path, "/");
    while (tmp != NULL)
    {
        arr[ret++] = tmp;
        tmp = strtok(NULL, "/");
    }
    return ret;
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------

bool FileSystem::Create(char *name, int initialSize)
{
    PersistentBitmap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;
    char *file_name;
    char *dir_arr[10];
    int count = splitPath(dir_arr, name);
    file_name = dir_arr[count - 1];
    ASSERT(changeToRightDir(dir_arr, count - 1) == TRUE) // sus

    DEBUG(dbgFile, "Creating file " << file_name << " size " << initialSize);

    currentDirectory->FetchFrom(currentDirectoryFile);

    if (currentDirectory->Find(file_name) != -1)
    {
        DEBUG(dbgFile, "File " << file_name << "is already in directory.");
        success = FALSE; // file is already in directory
    }
    else
    {
        freeMap = new PersistentBitmap(freeMapFile, NumSectors);
        sector = freeMap->FindAndSet(); // find a sector to hold the file header
        if (sector == -1)
        {
            DEBUG(dbgFile, " creating File " << file_name << " : no free block for file header.");
            success = FALSE; // no free block for file header
        }
        else if (!currentDirectory->Add(file_name, sector, IS_FILE))
        {
            DEBUG(dbgFile, " creating File " << file_name << " : no space in directory.");
            success = FALSE; // no space in directory
        }
        else
        {
            hdr = new FileHeader;
            if (!hdr->Allocate(freeMap, initialSize))
            {
                DEBUG(dbgFile, " creating File " << file_name << " : no space on disk for data.");
                success = FALSE; // no space on disk for data
            }
            else
            {
                success = TRUE;
                // everthing worked, flush all changes back to disk
                hdr->WriteBack(sector);
                currentDirectory->WriteBack(currentDirectoryFile);
                freeMap->WriteBack(freeMapFile);
            }
            delete hdr;
        }
        delete freeMap;
    }
    resetRootDir();
    return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.
//	To open a file:
//	  Find the location of the file's header, using the directory
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFile *
FileSystem::Open(char *name)
{
    OpenFile *openFile = NULL;
    int sector;
    char *file_name;
    char *dir_arr[10];
    int count = splitPath(dir_arr, name);
    file_name = dir_arr[count - 1];
    DEBUG(dbgFile, "Opening file " << file_name << "Path : "<<name);
    ASSERT(changeToRightDir(dir_arr, count - 1) == TRUE) // sus

    currentDirectory->FetchFrom(currentDirectoryFile);
    sector = currentDirectory->Find(file_name);
    if (sector >= 0)
        openFile = new OpenFile(sector); // name was found in directory
    resetRootDir();
    return openFile; // return NULL if not found
}

OpenFileId
FileSystem::OpenAFile(char *name)
{
    OpenFile *one_file = Open(name);
    OpenFileId id;
    // choose a file discriptor which is not occupied by another opened file.
    do
    {
        id = rand();
    } while (openedTable.find(id) != openedTable.end());
    // assign id to file on map
    openedTable[id] = one_file;
    return id;
}

int FileSystem::WriteAFile(char *buffer, int size, OpenFileId id)
{
    if (openedTable.find(id) != openedTable.end())
        return openedTable[id]->Write(buffer, size);
    return 0; // failed to write.
}

int FileSystem::ReadAFile(char *buffer, int size, OpenFileId id)
{
    if (openedTable.find(id) != openedTable.end())
        return openedTable[id]->Read(buffer, size);
    return 0; // failed to read.
}

int FileSystem::CloseAFile(OpenFileId id)
{
    if (openedTable.find(id) != openedTable.end())
    {
        delete openedTable[id];
        openedTable.erase(id);
        return 1;
    }
    return 0; // failed to close.
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------

bool FileSystem::Remove(char *name)
{
    PersistentBitmap *freeMap;
    FileHeader *fileHdr;
    int sector;
    char *file_name;
    char *dir_arr[10];
    int count = splitPath(dir_arr, name);
    file_name = dir_arr[count - 1];
    ASSERT(changeToRightDir(dir_arr, count - 1) == TRUE) // sus

    currentDirectory->FetchFrom(currentDirectoryFile);
    sector = currentDirectory->Find(name);
    if (sector == -1)
    {
        return FALSE; // file not found
    }
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);

    freeMap = new PersistentBitmap(freeMapFile, NumSectors);

    fileHdr->Deallocate(freeMap); // remove data blocks
    freeMap->Clear(sector);       // remove header block
    currentDirectory->Remove(name);

    freeMap->WriteBack(freeMapFile);                   // flush to disk
    currentDirectory->WriteBack(currentDirectoryFile); // flush to disk
    delete fileHdr;
    delete freeMap;
    resetRootDir();
    return TRUE;
}

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void FileSystem::List(char *path)
{
    char *dir_arr[10];
    int count = splitPath(dir_arr, path);
    changeToRightDir(dir_arr, count);
    currentDirectory->FetchFrom(currentDirectoryFile);
    currentDirectory->List();
    resetRootDir();
}

bool FileSystem::createDir(char *name)
{
    PersistentBitmap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;

    DEBUG(dbgFile, "Creating Dir " << name);

    currentDirectory->FetchFrom(currentDirectoryFile);

    if (currentDirectory->Find(name) != -1)
        success = FALSE; // file is already in directory
    else
    {
        freeMap = new PersistentBitmap(freeMapFile, NumSectors);
        sector = freeMap->FindAndSet(); // find a sector to hold the file header
        if (sector == -1)
            success = FALSE; // no free block for file header
        else if (!currentDirectory->Add(name, sector, IS_DIR))
            success = FALSE; // no space in directory
        else
        {
            hdr = new FileHeader;
            if (!hdr->Allocate(freeMap, DirectoryFileSize))
                success = FALSE; // no space on disk for data
            else
            {
                success = TRUE;
                // everthing worked, flush all changes back to disk
                hdr->WriteBack(sector);
                OpenFile *newDirFile = new OpenFile(sector);
                Directory *newDir = new Directory(NumDirEntries);
                newDir->WriteBack(newDirFile);
                currentDirectory->WriteBack(currentDirectoryFile);
                freeMap->WriteBack(freeMapFile);
            }
            delete hdr;
        }
        delete freeMap;
    }
    return success;
}

bool FileSystem::changeToRightDir(char **arr, int len)
{
    if (len < 0)
        return true; // Already in right folder
    DEBUG(dbgFile, " changeToRightDir : len = " << len);
    int sector_num = -1;
    for (int i = 0; i < len; i++)
    {
        DEBUG(dbgFile, "Try switch to " << arr[i]);
        // currentDirectory->List();
        sector_num = currentDirectory->Find(arr[i]);
        DEBUG(dbgFile, " changeToRightDir : sector_num = " << sector_num);

        if (sector_num == -1)
        {
            std::cout << "dir not found..\n";
            return false;
        }
        else
        {
            currentDirectory->WriteBack(currentDirectoryFile);
            delete currentDirectoryFile;
            delete currentDirectory;
            currentDirectoryFile = new OpenFile(sector_num);
            currentDirectory = new Directory(NumDirEntries);
            currentDirectory->FetchFrom(currentDirectoryFile);
            DEBUG(dbgFile, "Success on switching to " << arr[i]);
            // currentDirectory->List();
        }
    }
    return true;
}
void FileSystem::resetRootDir()
{
    // cout<<" resetRootDir : resetting pwd to root"<<endl;
    if (currentDirectoryFile != NULL)
        delete currentDirectoryFile;
    if (currentDirectory != NULL)
        delete currentDirectory;
    currentDirectoryFile = new OpenFile(DirectorySector); // root dir
    currentDirectory = new Directory(NumDirEntries);
    currentDirectory->FetchFrom(currentDirectoryFile);
}
bool FileSystem::MakeNewDir(char *name)
{
    char *dir_arr[10];
    int dir_count = 0;
    char *new_dir_name;
    if (name[0] != '/')
    {
        cout << "The mkdir should start with '/' \n ";
        return false;
    }
    else
    {
        dir_count = splitPath(dir_arr, name);
        // cout << " MakeNewDir : dir_count = " << dir_count << endl;
        new_dir_name = dir_arr[dir_count - 1];
        // cout << " MakeNewDir : new_dir_name = " << new_dir_name << endl;

        // move the currDir to right place
        if (changeToRightDir(dir_arr, dir_count - 1) == false)
            return FALSE;
        if (createDir(new_dir_name) == false)
            return FALSE;
    }

    // back to root dir
    resetRootDir();

    return TRUE;
}
//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    PersistentBitmap *freeMap = new PersistentBitmap(freeMapFile, NumSectors);

    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("Directory file header:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    freeMap->Print();

    currentDirectory->FetchFrom(currentDirectoryFile);
    currentDirectory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
}
#endif // FILESYS_STUB
