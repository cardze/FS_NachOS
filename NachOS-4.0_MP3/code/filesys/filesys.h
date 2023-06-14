// filesys.h
//	Data structures to represent the Nachos file system.
//
//	A file system is a set of files stored on disk, organized
//	into directories.  Operations on the file system have to
//	do with "naming" -- creating, opening, and deleting files,
//	given a textual file name.  Operations on an individual
//	"open" file (read, write, close) are to be found in the OpenFile
//	class (openfile.h).
//
//	We define two separate implementations of the file system.
//	The "STUB" version just re-defines the Nachos file system
//	operations as operations on the native UNIX file system on the machine
//	running the Nachos simulation.
//
//	The other version is a "real" file system, built on top of
//	a disk simulator.  The disk is simulated using the native UNIX
//	file system (in a file named "DISK").
//
//	In the "real" implementation, there are two key data structures used
//	in the file system.  There is a single "root" directory, listing
//	all of the files in the file system; unlike UNIX, the baseline
//	system does not provide a hierarchical directory structure.
//	In addition, there is a bitmap for allocating
//	disk sectors.  Both the root directory and the bitmap are themselves
//	stored as files in the Nachos file system -- this causes an interesting
//	bootstrap problem when the simulated disk is initialized.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef FS_H
#define FS_H

#include "copyright.h"
#include "sysdep.h"
#include "openfile.h"
#include <map>

#ifdef FILESYS_STUB // Temporarily implement file system calls as
// calls to UNIX, until the real file system
// implementation is available

typedef int OpenFileId;
class FileSystem
{
public:
	FileSystem()
	{
		table_len = 0;
		for (int i = 0; i < 20; i++)
			fileDescriptorTable[i] = NULL;
	}
	FileSystem(bool format);

	bool Create(char *name)
	{
		int fileDescriptor = OpenForWrite(name);

		if (fileDescriptor == -1)
			return FALSE;
		Close(fileDescriptor);
		return TRUE;
	}

	OpenFile *Open(char *name)
	{
		int fileDescriptor = OpenForReadWrite(name, FALSE);

		if (fileDescriptor == -1)
			return NULL;
		return new OpenFile(fileDescriptor);
	}

	OpenFileId OpenAFile(char *name)
	{
		if (Open(name) == NULL)
			return -1;
		OpenFile *File_object = Open(name);
		// add the OpenFile object into the table.
		fileDescriptorTable[table_len] = File_object;
		// and record the actual number of OpenFile object in the table.
		table_len++;

		return File_object->get_FD();
		// return OpenForReadWrite(name,FALSE);
	}

	int WriteAFile(char *buffer, int size, OpenFileId id)
	{
		for (int i = 0; i < table_len; i++)
		{
			if (fileDescriptorTable[i]->get_FD() == id)
			{
				// find out the right file descriptor in the table.
				fileDescriptorTable[i]->Write(buffer, size);
				return size;
			}
		}
		// WriteFile(id, buffer, size);
		return size;
	}

	int ReadAFile(char *buffer, int size, OpenFileId id)
	{
		for (int i = 0; i < table_len; i++)
		{
			if (fileDescriptorTable[i]->get_FD() == id)
			{
				// find out the right file descriptor in the table.
				fileDescriptorTable[i]->Read(buffer, size);
				return size;
			}
		}
		// WriteFile(id, buffer, size);
		return size;
	}

	int CloseAFile(OpenFileId id)
	{
		for (int i = 0; i < table_len; i++)
		{
			if (fileDescriptorTable[i]->get_FD() == id)
			{
				// remove OpedFile object from table
				fileDescriptorTable[i] = NULL;
				// if the i iter to the last item in the table.
				if (i == table_len - 1)
					table_len--;
				else
				{
					for (int j = i; j < table_len - 1; j++)
					{
						fileDescriptorTable[j] = fileDescriptorTable[j + 1];
					}
					table_len--;
				}
				return 1;
			}
		}
		return -1;
		// int ret = Close(id);
		// return ret >= 0 ? 1 : -1;
	}

	bool Remove(char *name) { return Unlink(name) == 0; }

private:
	int table_len;
	OpenFile *fileDescriptorTable[20];

	OpenFile *freeMapFile;	 // Bit map of free disk blocks,
							 // represented as a file
	OpenFile *directoryFile; // "Root" directory -- list of
							 // file names, represented as a file
};

#else // FILESYS
typedef int OpenFileId;
class FileSystem
{
public:
	FileSystem(bool format); // Initialize the file system.
							 // Must be called *after* "synchDisk"
							 // has been initialized.
							 // If "format", there is nothing on
							 // the disk, so initialize the directory
							 // and the bitmap of free blocks.
	~FileSystem();
	bool Create(char *name, int initialSize);
	// Create a file (UNIX creat)

	OpenFile *Open(char *name);

	OpenFileId OpenAFile(char *name);

	int WriteAFile(char *buffer, int size, OpenFileId id);

	int ReadAFile(char *buffer, int size, OpenFileId id);

	int CloseAFile(OpenFileId id);

	bool Remove(char *name); // Delete a file (UNIX unlink)

	void List(); // List all the files in the file system

	void Print(); // List all the files and their contents

private:
	map<OpenFileId, OpenFile*> openedTable;
	OpenFile *freeMapFile;	 // Bit map of free disk blocks,
							 // represented as a file
	OpenFile *directoryFile; // "Root" directory -- list of
							 // file names, represented as a file
	// for recording the present working dir
	OpenFile *currentDirectoryFile; 
	Directory *currentDirectory;

};

#endif // FILESYS

#endif // FS_H
