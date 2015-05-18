/* file.c
 *	Simple program to test whether create works
 */

#include "syscall.h"

int
main()
{
	char name[10];
	char orignData[10];
	char readdata[10];
	OpenFileId fd;
	int c;
	
	name[0] = 'R';
	name[1] = '\0';

	orignData[0] = 'T';
	orignData[1] = 'E';
	orignData[2] = '\0';
	
	fd = Open(name);
	Write(orignData, 3, fd);
	Close(fd);
    fd = Open(name);
    
    c = Read(readdata, 10, fd);
    Close(fd);
    Exit(readdata[1]);
}
