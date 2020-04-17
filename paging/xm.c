/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

#define TWOTEN 1024
#define SETONE 1
#define SETZERO 0
/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
        STATWORD ps;
        disable(ps);

	if(source<0 || source>=16)
	{
                kprintf("2");
                restore(ps);
                return SYSERR;		
	}
        if(npages<1 || npages>128)
        {
                kprintf("3");
                restore(ps);
                return SYSERR;
        }

	if(virtpage<(TWOTEN * 4))
	{
                kprintf("4");
  		restore(ps);
		return SYSERR;
	}

  	if(bsm_tab[source].bs_private==1)
	{
    		kprintf("5"); 
  		restore(ps);
		return SYSERR;
  	}
  	if(npages> bsm_tab[source].bs_npages)
	{
		    kprintf("6");  
  		restore(ps);
		return SYSERR;
  	}

  	bsm_map(currpid,virtpage,source,npages);

  restore(ps);
  return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
  	//kprintf("To be implemented!");
 	STATWORD ps;
  	disable(ps);

  	if(virtpage<4096)
	{
  		restore(ps);
		return SYSERR;
  	}
  	int a = bsm_unmap(currpid,virtpage,0);
        if (a!=OK)
	{
		restore(ps);
		return SYSERR;
	}
  	restore(ps);
  	return OK;
}
