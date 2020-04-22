#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) 
{
	int tmp_n_pages=0;
	STATWORD ps;
	disable(ps);
  	
	/* requests a new mapping of npages with ID map_id */
    	
  	if (bs_id >= 16 || bs_id < 0)
	{
		restore(ps);
		return SYSERR;
	}
	if ( npages > 128 || npages <= 0 || bsm_tab[bs_id].bs_sem == 1) 
      	{
		restore(ps);
      		return SYSERR;
	}
       
	if(bsm_tab[bs_id].bs_status== BSM_MAPPED)
	{
    		if (bsm_tab[bs_id].bs_private == 1) 	
		{
      			restore(ps);
      			return SYSERR;
    		}
		else
		{
			//if(npages > bsm_tab[bs_id].max_npages)
			//{
			bsm_tab[bs_id].bs_max_npages = npages;
			int val = bsm_tab[bs_id].bs_npages;
			restore(ps);
			return val;
			//}			
		}
	}
    	if ( bsm_tab[bs_id].bs_status==BSM_UNMAPPED ) 
	{
    		bsm_tab[bs_id].bs_npages = npages;
    		bsm_tab[bs_id].bs_pid[currpid] = 1;
    		restore(ps);
    		return npages;
	}
}
