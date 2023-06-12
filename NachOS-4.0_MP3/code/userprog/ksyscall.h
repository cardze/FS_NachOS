/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__
#define __USERPROG_KSYSCALL_H__

#include "kernel.h"

#include "synchconsole.h"

void SysHalt()
{
	kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2)
{
	return op1 + op2;
}

int SysCreate(char *filename,int size)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->interrupt->CreateFile(filename,size);
}

void SysPrintInt(int val)
{
	int tmp = val;
	int digit[10] = {0}; // assume the length of val is not bigger than 10!
	int index = 0, str_index = 0;
	char converted_int[10] = {0};

	while (tmp)
	{
		digit[index] = tmp % 10;
		index++;
		tmp /= 10;
		if (index > 10)
			DEBUG(dbgTraCode, "length of val is bigger than 10\n");
	}

	while (index)
	{
		index--;
		converted_int[str_index++] = ('0' + digit[index]);
	}
	converted_int[str_index++] = ('\n');
	kernel->synchConsoleOut->PutInt(converted_int, str_index + 1);
}
// #ifdef FILESYS_STUB
OpenFileId SysOpen(char *name)
{
	return kernel->interrupt->OpenFile(name);
}

int SysWrite(char *buffer, int size, OpenFileId id)
{
	return kernel->interrupt->WriteFile(buffer, size, id);
}

int SysRead(char *buffer, int size, OpenFileId id)
{
	return kernel->interrupt->ReadFile(buffer, size, id);
}

int SysClose(OpenFileId id)
{
	return kernel->interrupt->CloseFile(id);
}
// #endif // FILESYS_STUB

#endif /* ! __USERPROG_KSYSCALL_H__ */
