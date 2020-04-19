/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
 extern int page_replace_policy;
fr_map_t frm_tab[NFRAMES];
SYSCALL init_frm()
{
	STATWORD ps;
	disable(ps);
  	 /* frm_tab is the inverted page table */
  int i;
  for(i=0; i<NFRAMES; i++){
	  initialize_frm(i);
  }
  restore(ps);
  return OK;
}

void initialize_frm(int i){
	pt_t *page_table_pointer = (pt_t *)((i+FRAME0)*NBPG);
	frm_tab[i].fr_status = FRM_UNMAPPED;
	frm_tab[i].fr_type = FR_PAGE;
	frm_tab[i].fr_refcnt = 0;
	frm_tab[i].fr_dirty = 0;
	int k=0;
	for(k=1; k<NFRAMES ; k++){
		page_table_pointer->pt_pres = 0;
		page_table_pointer->pt_write = 0;
		page_table_pointer->pt_user= 0;
		page_table_pointer->pt_pwt = 0;
		page_table_pointer->pt_pcd = 0;
		page_table_pointer->pt_acc = 0;
		page_table_pointer->pt_dirty = 0;
		page_table_pointer->pt_mbz = 0;
		page_table_pointer->pt_global = 0;
		page_table_pointer->pt_avail = 0;
		page_table_pointer->pt_base = 0;
		page_table_pointer++;
	}
	for(k=0; k<50; k++){
		frm_tab[i].fr_pid[k]=0;
		frm_tab[i].fr_vpno[k] = 4096;
	}
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
	STATWORD ps;
	disable(ps);	
	/* iterate through inverted page table and find free frame or replacable frame and save it in *avail */
	*avail = -1;
	int i;
	for(i=0; i<NFRAMES; i++){
		if(frm_tab[i].fr_status == FRM_UNMAPPED){
			*avail = i;
			sc_flag=1;
			/* insert_sc_queue(i); should not be called here, as the page directories and page tables would be inserted */
			return OK;
		}
	}
	/* if no free frame found look into the page scheduling policy and find a replacable frame */
	if(page_replace_policy==SC)
	{
	int ret =  frame_replace_sc_queue();
	kprintf("Replacing Frame :%d\n",ret+FRAME0);
	*avail = ret;
	restore(ps);
	return OK;
	}
	else if(page_replace_policy==LFU)
	{
		int ret = frame_replace_LFU();
		kprintf("Replacing Frame :%d\n",ret+FRAME0);
		free_frm(ret);
		*avail = ret;
		restore(ps);
		return OK;
	}
  restore(ps);
  return SYSERR;
}
int frame_replace_LFU()
{
	int m;
	int min=0x7fffffff;
	int frame = -1;
	for(m=0;m<1024;m++)
	{
		if(frm_tab[m].fr_type == FR_PAGE)
		{
			if(frm_tab[m].fr_cnt < min)
			{
				min = frm_tab[m].fr_cnt;
				frame = m;
			}
			else if(frm_tab[m].fr_cnt == min)
			{
				if(frm_tab[m].fr_vpno > frm_tab[frame].fr_vpno)
				{
					frame = m;
				}
			}
		}
	}
	return frame;
}
void insert_sc_queue(int i){
	
	if(sc_tail==-1){
		/* implies queue is empty update the tail, current */
		sc_tail=i;
		sc_current = i;
		sc_q[i].next = i;
		sc_q[i].prev = i;
	}
	else{
		sc_q[i].next = sc_q[sc_tail].next;
		sc_q[sc_tail].next = i;
		
		sc_q[i].prev = sc_tail;
		sc_q[sc_q[i].next].prev = i;
	}
	sc_tail = i;
	
}
void init_sc_q(){
	int i;
	for(i=0; i<FRAME0; i++){
		sc_q[i].next=-1;
		sc_q[i].prev=-1;
	}
}
/* 
void print_queue(){
	int temp = sc_q[sc_current].next;
	kprintf("\t current %d, temp is %d, temp.next %d", sc_q[sc_current], temp, sc_q[temp].next);
	while(temp !=sc_current && sc_tail!=-1 ){
		// above condition might lead to infinite loop incase of single value present in queue, modify it
		temp = sc_q[temp].next;
	}
	kprintf("\n\n\tdone queue\t\n");
} */

int frame_replace_sc_queue(){
	int k, pid=currpid, found=0;
	sc_flag = 0;
	
	while(found==0){
	for(k=0; k<50; k++){
		if(frm_tab[sc_current].fr_vpno[k]>4096){
			pid = k;
			break;
		}
	}
	int vpno = frm_tab[sc_current].fr_vpno[pid];
	int pd_offset = ((vpno>>10)&0x000003ff);
	int pt_offset = ((vpno&0x000003ff));
	pd_t *paged_offset;
	pt_t *paget_offset;
	paged_offset = proctab[k].pdbr+(pd_offset * sizeof(pd_t));
	paget_offset = (paged_offset->pd_base * 4096) + (pt_offset* sizeof(pt_t));
	sc_current = sc_q[sc_current].next;
	if(paget_offset->pt_acc ==1){
		paget_offset->pt_acc =0;
		}
	else{
		found = 1;
		free_frm(paget_offset->pt_base - FRAME0);
		/* should not call delete_from_sc_q here, the frame gets replaced, entry should remain in q. hence, write a frame just before free frame is called for page, except here */
		break; 
		}
	}
	return sc_q[sc_current].prev;
}

void delete_from_sc_q(int i){
	if(sc_q[i].next == i){
		init_sc_q();
		sc_tail=-1;
		sc_current = -1;
		sc_q[i].next = -1;
		sc_q[i].prev = -1;
	}
	else{
		sc_q[sc_q[i].prev].next = sc_q[i].next;
		sc_q[sc_q[i].next].prev = sc_q[i].prev;
	}
}
/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
	STATWORD ps;
	disable(ps);
	/* if the frame is unmapped or frame type is not frame page return error */
	if(frm_tab[i].fr_status == FRM_UNMAPPED ){
		restore(ps);
		return SYSERR;
	}
	if(frm_tab[i].fr_type!=FR_PAGE ){
		if(i>3){
			initialize_frm(i);
		}
		restore(ps);
		return OK;
	}
	/* if the frame is of page table or page directory type we do not execute the below code */
	int store=-1, pageth=-1;
	int fr_pid = currpid;
	int fr_vpno = frm_tab[i].fr_vpno[currpid];
	if(bsm_lookup(fr_pid, fr_vpno*4096, &store, &pageth)==SYSERR){
		restore(ps);
		return SYSERR;
	}
	write_bs((i+FRAME0)*NBPG, store, pageth);	/* write the frame back to backing store */
	
	/* get the page directory and page directory offset */
	pd_t *pdbr ;
	virt_addr_t *virtual_address;
	virtual_address->pd_offset = fr_vpno/1024; 
	virtual_address->pt_offset = fr_vpno;
	pdbr = proctab[fr_pid].pdbr;
	pt_t *paget_entry;
	pd_t *paged_entry;
	paged_entry = (pd_t *)((proctab[fr_pid].pdbr +( virtual_address->pd_offset * sizeof(pd_t))));
	paget_entry = (pt_t *)((paged_entry->pd_base*NBPG)+((virtual_address->pt_offset) * sizeof(pt_t)));
	if(paget_entry->pt_pres ==1){
	paget_entry->pt_pres = 0;		/* update in page table unmapped frame is not present */
	frm_tab[paged_entry->pd_base - FRAME0].fr_refcnt -=1; 		/*  update the frame ref count of page table frame */
	initialize_frm(paget_entry->pt_base - FRAME0);
	if(frm_tab[paged_entry->pd_base - FRAME0].fr_refcnt==0){
		paged_entry->pd_pres=0;
		initialize_frm(paged_entry->pd_base - FRAME0);
	}
	}
	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 * set the page directory - for a process
 *-------------------------------------------------------------------------
 */
void setPageDirectory(int pid){
	STATWORD ps;
	disable(ps);
	int frame_number =0;
	pd_t *page_directory;
	get_frm(&frame_number);
	proctab[pid].pdbr = (FRAME0+frame_number)*NBPG;	
	frm_tab[frame_number].fr_status = FRM_MAPPED;
	frm_tab[frame_number].fr_pid[pid] = 1;
	frm_tab[frame_number].fr_type = FR_DIR;
	int i;
	page_directory = (pd_t *)proctab[pid].pdbr;
	for(i=0; i<NFRAMES ; i++){
		page_directory->pd_pres= 0;
		page_directory->pd_write = 0;
		page_directory->pd_user=0;
		page_directory->pd_pwt=0;
		page_directory->pd_pcd=0;
		page_directory->pd_acc=0;
		page_directory->pd_mbz=0;
		page_directory->pd_fmb=0;
		page_directory->pd_global=0;
		page_directory->pd_avail=0;
		page_directory->pd_base= 0;
		
		if(i<4){
			page_directory->pd_pres= 1;
			page_directory->pd_write = 1;
			page_directory->pd_base= ((FRAME0+i));
		}
		page_directory++;
	}
	restore(ps);
	return OK;
}
/*	called only for freeing page directory	*/
int reset_frame(int i){
	if(i<6){
		return OK;
	}
	frm_tab[i].fr_type = FR_PAGE;
	frm_tab[i].fr_refcnt = 0;
	frm_tab[i].fr_status = FRM_UNMAPPED;
	return OK;
}


/*-------------------------------------------------------------------------
 * create a page table for the process
 *-------------------------------------------------------------------------
 */
int create_page_table(){
	int frame_number = -1;
	if(get_frm(&frame_number)==-1){
		return -1;
	}
	pt_t *page_table_pointer = (pt_t *)((frame_number+FRAME0)*NBPG);
	
	frm_tab[frame_number].fr_status = FRM_MAPPED;
	frm_tab[frame_number].fr_type = FR_TBL;
	frm_tab[frame_number].fr_vpno[currpid] = 4096;
	frm_tab[frame_number].fr_refcnt = 0;
	frm_tab[frame_number].fr_dirty = 0;
	int k;
	for(k=0; k<50; k++){
		frm_tab[frame_number].fr_pid[k]=0;
		frm_tab[frame_number].fr_vpno[k] = 4096;
	}
	page_table_pointer->pt_base = (FRAME0+frame_number);
	page_table_pointer++;
	k=0;
	for(k=1; k<NFRAMES ; k++){
		page_table_pointer->pt_pres = 0;
		page_table_pointer->pt_write = 1;
		page_table_pointer->pt_user= 1;
		page_table_pointer->pt_pwt = 1;
		page_table_pointer->pt_pcd = 0;
		page_table_pointer->pt_acc = 0;
		page_table_pointer->pt_dirty = 0;
		page_table_pointer->pt_mbz = 0;
		page_table_pointer->pt_global = 0;
		page_table_pointer->pt_avail = 0;
		page_table_pointer->pt_base = 0;
		page_table_pointer++;
	}
	return frame_number;
}

