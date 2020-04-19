/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

#define SETZERO 0
#define SETONE  1
#define TWOTEN  1024

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{
	STATWORD ps;
	//struct mblock *index;
	struct mblock *b,*a;
	//struct mblock *indexTres;
	//printf("VGETMEM To be implemented!\n");
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
	//index = indexDos->mnext;
	for(i=b->mnext; i!=(struct mblock *)NULL;i=i->mnext)
	//while (index != (struct mblock *)NULL) 
	{
		//int checkMlen = index->mlen;
                if(i->mlen > nbytes)
		{
                        //conv = (unsigned)i + nbytes;
                        a = (struct mblock *)((unsigned)i + nbytes);
                        b->mnext = a;
                        struct mblock *temp;
			a->mnext = i->mnext;
                        temp = a->mnext;
			// sub = i->mlen - nbytes;
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
		//index = i->mnext;
	}
	restore(ps);
	return((WORD*) -1 );
}
