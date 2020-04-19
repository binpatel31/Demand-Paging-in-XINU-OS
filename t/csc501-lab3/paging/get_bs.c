#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

#define SETONE  1
#define SETZERO 0
int get_bs(bsd_t bs_id, unsigned int npages) 
{
	STATWORD ps;
	disable(ps);
  	/* requests a new mapping of npages with ID map_id */
    	
    	int checkSemValue     = bsm_tab[bs_id].bs_sem;
   	//int checkPrivateValue = bsm_tab[bs_id].bs_private;
 	//int checkStatus       = bsm_tab[bs_id].bs_status;
    	int pagesValue        = bsm_tab[bs_id].bs_npages;
   	if (bs_id >= 16 || bs_id < SETZERO)
	{
		restore(ps);
		return SYSERR;
	}
	if ( npages > 128 || npages <= SETZERO || checkSemValue == 1) 
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