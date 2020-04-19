#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {

  /* release the backing store with ID bs_id */
	STATWORD ps;
	disable(ps);
	if(isbad_bsid(bs_id) || bsm_tab[bs_id].bs_status == BSM_UNMAPPED){
		restore(ps);
		return  SYSERR;
	}
	if(bsm_tab[bs_id].bs_private == 1){			/*	if bs is private release it		*/
		free_bsm(bs_id);
		restore(ps);
		return OK;
	}
	else{
		bsm_tab[bs_id].bs_pid[currpid] = 0;
		bsm_tab[bs_id].bs_vpno[currpid] = 4096;
		bsm_tab[bs_id].bs_npages[currpid] = 0;
		if(proctab[currpid].backing_store_map[bs_id]==1){
			proctab[currpid].backing_store_map[bs_id]=0;
			bsm_tab[bs_id].ref_count--;
		}
		if(bsm_tab[bs_id].ref_count ==0){
			free_bsm(bs_id);
		}
		restore(ps);
		return OK;
	}
}

