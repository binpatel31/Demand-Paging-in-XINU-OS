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


	int pid,store;

	if (get_bsm(&store) == SYSERR) 
	{
		restore (ps);
		return SYSERR;
	}
	pid = create(procaddr, ssize, priority, name, nargs, args);
	
	//=================================
	bsm_tab[store].bs_status = BSM_MAPPED;
    	bsm_tab[store].bs_npages = hsize;
  
    	bsm_tab[store].bs_pid[pid] = 1;
    	bsm_tab[store].bs_sem      = 0;
	bsm_tab[store].bs_vpno[pid]= 4096;
	bsm_tab[store].bs_reference_cnt=1;
	bsm_tab[store].bs_mapping = 1;
	proctab[currpid].vhpno = 4096;
	proctab[currpid].store = store;
	proctab[currpid].bs[store]=1;

	//==================
	bsm_tab[store].bs_private = 1;
	int next = (struct mblock *)(4096 * 4096);
	proctab[pid].vhpnpages = hsize;

	proctab[pid].vmemlist = getmem(sizeof(struct mblock *));
	
	proctab[pid].vmemlist->mnext = next;
	proctab[pid].vmemlist->mlen = 0;

	struct mblock *strt;

	strt = BACKING_STORE_BASE + (store * BACKING_STORE_UNIT_SIZE);

	strt->mnext = NULL;

	int len_v = 4096*hsize;
	strt->mlen = len_v;

	fr_pid_track[store][pid]=1;

	restore(ps);
	return pid;
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
