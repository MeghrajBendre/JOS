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
	//extern volatile pte_t uvpt[];     // VA of "virtual page table"
	//extern volatile pde_t uvpd[];     // VA of current page directory	

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	addr = ROUNDDOWN(addr, PGSIZE);

	if(!(err & FEC_WR))
		panic("pgfault: faulting access write: %e", err);

	if(!((uvpt[PGNUM(addr)]) & PTE_P))
		panic("pgfault: page not is not present: %e", err);

	if(!(uvpt[PGNUM(addr)] & PTE_COW))
		panic("pgfault: faulting access cow: %e", err);

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.


	//allocate page
	r = sys_page_alloc(0, (void*)PFTEMP, (PTE_P|PTE_U|PTE_W));
	if(r < 0)
		panic("pgfault: sys_page_alloc returned %e", r);
	//copy old page to new page
	memcpy((void*)PFTEMP, (const void*)addr, PGSIZE);

	r = sys_page_map(0, (void*)PFTEMP, 0, addr, (PTE_P|PTE_U|PTE_W));
	if(r < 0)
		panic("pgfault: sys_page_map returned %e", r);

	r = sys_page_unmap(0, (void*)PFTEMP);
	if(r < 0)
		panic("pgfault: sys_page_map returned %e", r);
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
	// LAB 4: Your code here.
	//panic("duppage not implemented");
	int r=0;
	void *va = (void*)(pn << PGSHIFT);
	//extern volatile pte_t uvpt[];     // VA of "virtual page table"
	//extern volatile pde_t uvpd[];     // VA of current page directory

	//page sharing enabled
        if(uvpt[pn] & PTE_SHARE) {
                r = sys_page_map(0, va, envid, va, uvpt[pn] & PTE_SYSCALL);
                if(r < 0)
                        panic("duppage: sys_page_map returned %e", r);
		return 0;
	}


	//If the page is writable or copy-on-write
	if((uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW)) {
		r = sys_page_map(0, va, envid, va, (PTE_P | PTE_U | PTE_COW));
		if(r < 0)
			panic("duppage: sys_page_map returned %e", r);
		
		//for parent
		r = sys_page_map(0, va, 0, va, (PTE_P | PTE_U | PTE_COW));
		if(r < 0)
			panic("duppage: sys_page_map returned %e", r);
	}
	else if (uvpt[pn] & (PTE_P)){
		int req_perm = (uvpt[pn] & PTE_SYSCALL);
		r = sys_page_map(0, va, envid, va, req_perm);
		if(r < 0)
			panic("duppage: sys_page_map returned %e", r);
	}
	else
		panic("Page Not Found");

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
	//panic("fork not implemented");
	envid_t env_id;
	uint32_t i, j, pg_num;
	int r;

	//extern volatile pte_t uvpt[];     // VA of "virtual page table"
	//extern volatile pde_t uvpd[];     // VA of current page directory


	// Set up our page fault handler appropriately.
	set_pgfault_handler(pgfault);
	
	// Create a child.
	env_id = sys_exofork();
	if(env_id < 0) {	//error
		panic("fork: sys_exofork returned with error: %e", env_id);
	}
	else if(env_id == 0) {	//child
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	else {	//parent
		uintptr_t pg_addr;

	for(pg_addr = 0; pg_addr < UTOP; pg_addr += PGSIZE) {
		if(pg_addr == (uintptr_t)(UXSTACKTOP - PGSIZE)) {
			//cprintf("Exception stack page so continue \n");
			continue;
		}
		if((uvpd[PDX(pg_addr)] & (PTE_P)) && (uvpt[PGNUM(pg_addr)] & (PTE_P | PTE_U)))
				duppage(env_id,PGNUM(pg_addr));
			
		
	}

		// Then mark the child as runnable and return.
		r = sys_page_alloc(env_id, (void*)(UXSTACKTOP - PGSIZE), (PTE_U | PTE_W | PTE_P));
		if(r < 0)
			panic("fork: sys_page_alloc :: %e", r);
		
		sys_env_set_pgfault_upcall(env_id, thisenv->env_pgfault_upcall);
		sys_env_set_status(env_id, ENV_RUNNABLE);
		return env_id;
	}
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
