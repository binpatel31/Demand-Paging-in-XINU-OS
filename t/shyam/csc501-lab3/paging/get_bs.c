#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */
	STATWORD ps;
	disable(ps);
	if(isbad_npage(npages) || isbad_bsid(bs_id) ){
		restore(ps);
		return SYSERR;
	}
	/* if bsm is unmapped allocate it directly */
	if(bsm_tab[bs_id].bs_status == BSM_UNMAPPED){
		/* map this using the bsm_map */
		if(bsm_map(currpid, 4096, bs_id, npages)==SYSERR){
			restore(ps);
			return SYSERR;
		}
		bsm_tab[bs_id].max_npages = npages;
		restore(ps);
		return npages;
	}
	else{
		// if private or if the , return sys error
		if(bsm_tab[bs_id].bs_status == BSM_MAPPED && bsm_tab[bs_id].bs_private){ /*		*/
			restore(ps);
			return SYSERR;
		}
		if(npages >bsm_tab[bs_id].max_npages){
			restore(ps);
			return bsm_tab[bs_id].max_npages;
		}
		/* else add process to shared backing store add process to the backing store  */
		proctab[currpid].backing_store_map[bs_id]=1;
		bsm_tab[bs_id].bs_pid[currpid] = 1;
		bsm_tab[bs_id].bs_npages[currpid] = npages; 
		bsm_tab[bs_id].bs_vpno[currpid] = 4096;
		bsm_tab[bs_id].ref_count++;
		
		restore(ps);
		return bsm_tab[bs_id].max_npages;
	}

}


