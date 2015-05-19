/* fork.c
 *	Simple program to test whether fork and join works
 */

#include "syscall.h"

int i;
int work()
{
	for (i = 0; i < 100; i++);
}
int
main()
{
	char name[10];
	name[0] = 's';
	name[1] = 'o';
	name[2] = 'r';
	name[3] = 't';
	name[4] = '\0';
	Exec(name);
    /* not reached */
}
