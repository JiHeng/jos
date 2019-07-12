// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	pte_t *pte = (pte_t *)((PDX(UVPT) << 22) | (PDX(addr) << 12));
	pte = pte + PTX(addr);

	if ((err & PTE_W) == 0){
		panic("[%x]: read fault: %x\n",sys_getenvid(), addr);
	}

	// check if the page is copy-on-write
	if ((*pte & PTE_COW) == 0) {
		panic("writing into a non-writable page. The page is not copy-on-write. addr: %x, pte: %x\n",
		addr, *pte);
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	addr = (void *)ROUNDDOWN(addr, PGSIZE);
	// do not use thisenv->env_id, this page fault can happen before jumping back to fork,
	// where thisenv will be fixed.
	envid_t envid = sys_getenvid();
	if (sys_page_alloc(sys_getenvid(), PFTEMP, PTE_U | PTE_W | PTE_P) < 0) {
		panic("cannot alloc page\n");
	}
	pde_t *pde = (pde_t *)(PDX(UVPT) << 22) + PDX(PFTEMP);

	memcpy(PFTEMP, addr, PGSIZE);
	sys_page_map(envid, PFTEMP, envid, addr, PTE_U | PTE_W | PTE_P);
	sys_page_unmap(envid, PFTEMP);
	return;
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	void *addr = (void *)(pn * PGSIZE);
	if ((uint32_t)addr >= UTOP) {
		panic("cannot map kernel space\n");
	}

	unsigned pt_num = pn / 1024; // page table index
	pte_t *pte = (pte_t *)((PDX(UVPT) << 22) | (pt_num << 12)) + (pn % 1024);

	// map
	int perm = *pte & (PTE_SYSCALL);
	if (perm & PTE_SHARE) {
		perm = perm | PTE_SHARE;
	} else if ((perm & PTE_COW) || (perm & PTE_W)) {
		perm = PTE_COW;
	}

	if (sys_page_map(thisenv->env_id, addr, envid, addr, perm) < 0) {
		panic("cannot map page");
	}

	// map the parent page as COW
	if (sys_page_map(thisenv->env_id, addr, thisenv->env_id, addr, perm) < 0) {
		sys_page_unmap(envid, addr);
		panic("cannot map parent page");
	}

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	set_pgfault_handler(pgfault);
	envid_t envid = sys_exofork();
	if (envid < 0) {
		panic("cannot sys fork\n");
	}

	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	pde_t *pde = (pde_t *)((PDX(UVPT) << 22) | (PDX(UVPT) << 12));
	pte_t *pte;
	unsigned pn = PDX(UVPT) * 1024;
	for (int i = 0 ; i < PDX(UTOP); i++) {
		if ((*pde & PTE_P) > 0) {
			// walk the pages in the table
			pte = (pte_t *)((PDX(UVPT) << 22) | (i << 12));
			for (int j = 0; j < 1024; j++) {
				if (((*pte) & (PTE_P | PTE_U)) > 0) {
					if (i == PDX(UXSTACKTOP - PGSIZE) && j == PTX(UXSTACKTOP - PGSIZE)) {
						sys_page_alloc(envid, (void *)UXSTACKTOP - PGSIZE, PTE_U | PTE_W | PTE_P);
					} else {
						duppage(envid, i * 1024 + j);
					}
				}
				pte++;
			}
		}

		pde++;
	}

	if (sys_env_set_status(envid, ENV_RUNNABLE) < 0) {
		panic("cannot set env status");
	}

	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
