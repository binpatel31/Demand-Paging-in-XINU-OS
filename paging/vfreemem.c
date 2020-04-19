/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
	struct	mblock	*block;
	unsigned size;
{

	STATWORD ps;
	struct mblock *a,*b;
	unsigned top;

	if (size == 0) 
	{
		return SYSERR;
	}

	disable(ps);

	
	int c,h;
	int list = &proctab[currpid].vmemlist;
	b = list;

	for(a=b->mnext; a != (struct mblock *)NULL && a< block; a=a->mnext)
	{
		b = a;
	}


	int addr_e = &proctab[currpid].vmemlist;
	int at = (unsigned) roundmb(size) + (unsigned)block;
	if((b->mlen + (unsigned)b) > (unsigned)block)
    	{
		if (b != &proctab[currpid].vmemlist)
		{
                	restore(ps);
                	return SYSERR;
		}
    	}
	if(a != NULL && (at > (unsigned)a ))
	{
		restore(ps);
		return SYSERR;
	}
    	h = b->mlen + (unsigned)b;
	int ed = (unsigned) block;
	top=h;
	
	int temp = b->mlen;
	if (b != &memlist) 
	{
		if((b->mlen + (unsigned)b) == ed)
		{
			int add = b->mlen;
			b->mlen =  add + (unsigned) roundmb(size);
		}
	} 
	else 
	{
		//int s = size;
		block->mlen = (unsigned) roundmb(size);
		
		temp = temp+(1024);
		block->mnext = a;
		b->mnext = block;
		b = block;
	}
	int val1 = b->mlen + (unsigned)(b);
	int val2 = (unsigned)a;
	
	if (val1 == val2 ) 
	{
		int addDos = b->mlen;
		temp = temp+1024;
		b->mlen = addDos + a->mlen;
		b->mnext = a->mnext;
	}

	restore(ps);
	return OK;
}
