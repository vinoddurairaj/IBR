// async.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define BUFFER_SIZE			(64*1024)
#define	NUMBER_OF_BUFFERS	MAXIMUM_WAIT_OBJECTS


int main(int argc, char* argv[])
{
	HANDLE		hFile;   
	DWORD		NumberOfBytesWrote, dwSizeOfBuffer = BUFFER_SIZE;
	OVERLAPPED	osWriteOperation[NUMBER_OF_BUFFERS];
	HANDLE		Events[NUMBER_OF_BUFFERS];
	DWORD		err;
	char		*pDataBuf;
    ULARGE_INTEGER WritePointer; 
	int			i;

	hFile = CreateFile(argv[1],
		GENERIC_WRITE, 0,
		NULL, OPEN_ALWAYS,
		FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
		NULL);   
	
	if (hFile == INVALID_HANDLE_VALUE) {
		return(1);
	}
		
	pDataBuf = (char *) VirtualAlloc(NULL, BUFFER_SIZE * NUMBER_OF_BUFFERS, MEM_COMMIT, PAGE_READWRITE);
	
	for (i = 0; i < NUMBER_OF_BUFFERS; i++) {
		memset(&osWriteOperation[i], 0, sizeof(OVERLAPPED));
		osWriteOperation[i].hEvent = Events[i] = CreateEvent(NULL, TRUE, TRUE, NULL);
	}

	do {
		WritePointer.QuadPart = 0;

		do {
			i = WaitForMultipleObjects(
				NUMBER_OF_BUFFERS,             // number of handles in the handle array
				Events,							// pointer to the object-handle array
				FALSE,							// wait flag
				INFINITE						// time-out interval in milliseconds
				);

			ResetEvent(Events[i]);
			printf("%d\n", i);

			osWriteOperation[i].Offset = WritePointer.LowPart;
			osWriteOperation[i].OffsetHigh = WritePointer.HighPart;

			if (!WriteFile(hFile,
				pDataBuf + (i * BUFFER_SIZE),
				dwSizeOfBuffer,
				&NumberOfBytesWrote,
				&osWriteOperation[i] ) )
			{
				err = GetLastError();
				if (err != ERROR_IO_PENDING)
				{
					VirtualFree(pDataBuf,BUFFER_SIZE, MEM_DECOMMIT);
					
					return 1;
				}
			}

			WritePointer.QuadPart += BUFFER_SIZE;
		} while (WritePointer.QuadPart < 0x5000000 && !_kbhit() );
	} while ( !_kbhit() );

	CloseHandle(hFile);

	for (i = 0; i < NUMBER_OF_BUFFERS; i++) {
		CloseHandle(Events[i]);
	}

	VirtualFree(pDataBuf,BUFFER_SIZE * NUMBER_OF_BUFFERS, MEM_DECOMMIT);

	return 0;
}

