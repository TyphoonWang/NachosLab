/* create.c
 *	Simple program to test whether create works
 */

#include "syscall.h"

int
main()
{
	char name[10];
	name[0] = 'R';
	name[1] = '\0';
	Create(name);
    /* not reached */
}
