// write.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "stdlib.h"
#include <conio.h>

int main(int argc, char* argv[])
{
	FILE *stream;
	int	bytes;
	char	buffer[65536];

	do {
		stream = fopen(argv[1] , "wb");
		if (stream == NULL)
			return 1;

		bytes = 0;

		do {
			bytes += fwrite(buffer, 1, 65536, stream);
			
			if (argc > 2)
				Sleep(atoi(argv[2]));

			fprintf(stdout, "%d\n", bytes);
		} while ( !_kbhit() && bytes < 0x1000000);

		fclose(stream);

	} while (!_kbhit());

	return 0;
}

