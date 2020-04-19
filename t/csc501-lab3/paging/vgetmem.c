/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

//#define SETZERO 0
//#define SETONE  1
//#define TWOTEN  1024

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{
	STATWORD ps;

	struct mblock *b,*a;


	int conv,sub;
	int checkList = proctab[currpid].vmemlist->mnext;

	if(proctab[currpid].vmemlist->mnext == (struct mblock*)NULL)
	{
		restore(ps);
		return -1;
	}
	if (nbytes == 0) 	
	{		
		restore(ps);
		return -1;
	}

	int listFor;
	nbytes = (unsigned int) roundmb(nbytes);
	listFor = &proctab[currpid].vmemlist;
	b = listFor;
	struct mblock *i;

	for(i=b->mnext; i!=(struct mblock *)NULL;i=i->mnext) 
	{
                if(i->mlen > nbytes)
		{       
                        a = (struct mblock *)((unsigned)i + nbytes);
                        b->mnext = a;
                 
		        struct mblock *temp;
			a->mnext = i->mnext;
                        temp = a->mnext;
		
                        a->mlen = i->mlen - nbytes;
                        restore(ps);
                        return ((WORD*)i);
                }	
		else if (i->mlen == nbytes) 
		{
			b->mnext = i->mnext;
			restore (ps);
			return ((WORD*)i);
		}
		b = i;
	}
	restore(ps);
	return((WORD*) -1 );
}
