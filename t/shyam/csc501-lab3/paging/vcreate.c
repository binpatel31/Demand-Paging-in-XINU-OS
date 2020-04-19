/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	
	STATWORD ps;
	disable(ps);
	
	// get a backing store with private access.
	if( isbad_npage(hsize)){
		restore(ps);
		//kprintf("heap size cannot be grater than 128 pages\n");
		return SYSERR;
	}
	int bs_id=0;
	if(get_bsm(&bs_id)==SYSERR){
		restore(ps);
		return SYSERR;
	}
	/* though get _bs does the bsm_map for BSM_UNMAPPED, should not be used as this might map to shared OR can be used as we bring an unmapped bs_id from get_bsm */
	
	
	int p_id = create(procaddr,ssize,priority,name,nargs,args);
	if(bsm_map(p_id, 4096, bs_id, hsize)==SYSERR){
		restore(ps);
		return SYSERR;
	}
	bsm_tab[bs_id].bs_private = 1;
	
	proctab[p_id].private_heap = 1;
	proctab[p_id].backing_store_map[bs_id] =1;
	proctab[p_id].store = bs_id;
	proctab[p_id].vhpnpages = hsize;
	proctab[p_id].vhpno = 4096;	//?
	struct mblock *vheap;
	
	vheap = BACKING_STORE_BASE + (bs_id * BACKING_STORE_UNIT_SIZE);
	vheap->mlen = hsize*NBPG;
	vheap->mnext = NULL;
	proctab[p_id].vmemlist->mlen = hsize*4096;
	proctab[p_id].vmemlist->mnext = 4096 * 4096;
	
	restore(ps);
	return p_id;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
