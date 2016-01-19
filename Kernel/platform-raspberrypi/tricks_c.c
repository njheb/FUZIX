#include <kernel.h>
#include <printf.h>
#include <kdata.h>
#include "raspberrypi.h"
#include "externs.h"

void set_tlb_entry(uint32_t virtual, uint32_t physical, uint32_t flags)
{
	int page = virtual / MEGABYTE;
	tlbtable[page] = physical | flags
		| (3<<10) /* AP = 3 (global access) */
		| 0x12    /* 1MB page */
		;
}

void unset_tlb_entry(uint32_t virtual)
{
	int page = virtual / MEGABYTE;
	tlbtable[page] = 0;
}

static void change_control_register(uint32_t set, uint32_t reset)
{
	uint32_t value = mrc(15, 0, 1, 0, 0);
	value |= set;
	value &= ~reset;
	mcr(15, 0, 1, 0, 0, value);
}

void enable_mmu(void)
{
	mcr(15, 0, 3, 0,  0, 0x3); /* domain */
	mcr(15, 0, 2, 0,  0, tlbtable); /* tlb base register 0 */
	mcr(15, 0, 2, 0,  1, tlbtable); /* tlb base register 1 */

	change_control_register(CR_A, CR_M|CR_C|CR_I);

	invalidate_data_cache();
	invalidate_insn_cache();
	invalidate_tlb();

	change_control_register(CR_M|CR_C|CR_I, 0);
}

void page_in_new_process(ptptr newp)
{
	if (newp->p_page != udata.u_page)
	{
		uint32_t basei = (uint32_t)&__progbase;

		clean_data_cache();
		invalidate_data_cache();
		set_tlb_entry(basei, newp->p_page*MEGABYTE, CACHED|BUFFERED);
		invalidate_tlb();
		flush_prefetch_buffer();
		invalidate_insn_cache();
	}

	newp->p_status = P_RUNNING;
}

/* Copies the currently mapped process into the new page, and leaves the new
 * page mapped. */
void copy_current_to_new_page(ptptr newp)
{
	int curpage = udata.u_page;
	int newpage = newp->p_page;
	uint32_t topi = (uint32_t)udata.u_top;

	/* Map both pages, with the new page at the real process's address. */

	uint32_t basei = (uint32_t)&__progbase;
	uint32_t udatai = (uint32_t)&udata;
	uint32_t limiti = basei + MEGABYTE;

	clean_data_cache();
	invalidate_data_cache();
	set_tlb_entry(basei,             newpage*MEGABYTE, CACHED|BUFFERED);
	set_tlb_entry(basei + MEGABYTE,  curpage*MEGABYTE, CACHED|BUFFERED);
	invalidate_tlb();

	/* Copy the actual data. */

	memcpy((void*)basei,  (void*)(basei + MEGABYTE),  topi - basei);
	memcpy((void*)udatai, (void*)(udatai + MEGABYTE), limiti - udatai);

	/* Unmap the old page. */

	clean_data_cache();
	invalidate_data_cache();
	unset_tlb_entry(basei + MEGABYTE);
	invalidate_tlb();

	/* Our new page is mapped. Make a new process table entry for it. */

	newproc(newp);

	/* Some final flushing and we're ready to go. */

	flush_prefetch_buffer();
	invalidate_insn_cache();
}

