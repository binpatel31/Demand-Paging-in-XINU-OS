/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{
	STATWORD ps;    
	struct	mblock	*p, *q, *leftover;

	disable(ps);
	struct mblock *vpointer;
	vpointer = proctab[currpid].vmemlist;
	if (nbytes==0 || vpointer->mnext== (struct mblock *) NULL) {
		restore(ps);
		return( (WORD *)SYSERR);
	}
	nbytes = (unsigned int) roundmb(nbytes);
	for (q= vpointer,p=vpointer->mnext ;
	     p != (struct mblock *) NULL ;
	     q=p,p=p->mnext)
		if ( q->mlen == nbytes) {
			q->mnext = (struct mblock*)((unsigned)p + nbytes);
			q->mlen =0;
			restore(ps);
			return( (WORD *)p );
		} else if ( q->mlen > nbytes ) {
			leftover = (struct mblock *)( (unsigned)p + nbytes );
			q->mnext = leftover;
			//leftover->mnext = p->mnext;
			q->mlen = q->mlen - nbytes;
			restore(ps);
			return( (WORD *)p );
		}
		//kprintf("Error allocating memory to heap");
	restore(ps);
	return( SYSERR );
	//return( (WORD *)SYSERR );
	//kprintf("To be implemented!\n");
	
	
}


