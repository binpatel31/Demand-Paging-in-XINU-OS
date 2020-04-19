/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>

#define SETZERO 0
#define SETONE  1
#define TWOTEN  1024

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
	struct	mblock	*block;
	unsigned size;
{
	//kprintf("VFREEMEM To be implemented!\n");
	STATWORD ps;
	struct mblock *a,*b;//index;
	unsigned top;
	//struct mblock *indexDos;
	if (size == 0) 
	{
		return SYSERR;
	}

	disable(ps);
	//size = (unsigned) roundmb(size);
	
	int c,h;
	int list = &proctab[currpid].vmemlist;
	b = list;

	//index = indexDos->mnext;
	for(a=b->mnext; a != (struct mblock *)NULL && a< block; a=a->mnext)
	//while (index != (struct mblock *)NULL && index< block) 
	{
		b = a;
		//index = index->mnext;
	}


//	int c = b->mlen;
//	int h = c + (unsigned)b;
//	top = h;
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
	//int a00 = b->mlen;
	//int a01 = (unsigned)(b);
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
