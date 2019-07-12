#include <inc/lib.h>

int
pageref(void *v)
{
	pte_t *pte = (pte_t *)((PDX(UVPT) << 22) | (PDX(v) << 12)) + PTX(v);
	if (!(uvpd[PDX(v)] & PTE_P)) {
		return 0;
	}

	if (!(*pte & PTE_P)) {
		return 0;
	}

	int ref =  pages[PGNUM(*pte)].pp_ref;

	return ref;
}
