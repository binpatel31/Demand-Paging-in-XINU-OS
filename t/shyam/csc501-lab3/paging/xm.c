/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
	STATWORD ps;
	disable(ps);
	// called when get bs is called
	if(bsm_tab[source].bs_status== BSM_UNMAPPED){
		kprintf("\nbs is unmapped");
		restore(ps);
		return SYSERR;
	}
	else{ 
		if(bsm_tab[source].bs_private==1){
			restore(ps);
			return SYSERR;
		}
		else{
			if(npages<=bsm_tab[source].max_npages){
				bsm_tab[source].bs_pid[currpid]=1;
				bsm_tab[source].bs_vpno[currpid]=virtpage;
				bsm_tab[source].bs_npages[currpid]=npages;
				restore(ps);
				return OK;
			}
			else{
				restore(ps);
				return SYSERR;
			}
		}
	}
	
	
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
  if(bsm_unmap(currpid, (virtpage), 0)!=OK){
	return SYSERR;
  }
  return OK;
}
