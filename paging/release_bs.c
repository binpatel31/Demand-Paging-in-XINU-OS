#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


SYSCALL release_bs(bsd_t bs_id) 
{

	//kprintf("RELEASE_BSTo be implemented!\n");
    	STATWORD ps;
    	disable(ps);
    	
	if(bs_id<0 || bs_id>=16)
	{
		restore(ps);
		return SYSERR;
	}	

	if(bsm_tab[bs_id].bs_status == BSM_UNMAPPED)
	{
		restore(ps);
		return SYSERR;
	}
	bsm_tab[bs_id].bs_pid[currpid]  = 0;
    	bsm_tab[bs_id].bs_vpno[currpid] = 4096;

    	//int checkMapping = bsm_tab[bs_id].bs_mapping;
    	if (bsm_tab[bs_id].bs_mapping == 0) 
	{
      		bsm_tab[bs_id].bs_status = BSM_UNMAPPED;
      		bsm_tab[bs_id].bs_sem    = 0;
      		
		if(bsm_tab[bs_id].bs_pid[currpid] == 1)
		{
			bsm_tab[bs_id].bs_reference_cnt-=1;
		}	
	
		bsm_tab[bs_id].bs_npages = 0;
      		bsm_tab[bs_id].bs_private= 0;
    	}
   	restore(ps);
   	return OK;
}
