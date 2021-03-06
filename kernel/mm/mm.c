/*
 * Copyright (c) 2020 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * OS-Lab-2020 (i.e., ChCore) is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *   http://license.coscl.org.cn/MulanPSL
 *   THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 *   PURPOSE.
 *   See the Mulan PSL v1 for more details.
 */

#include <common/mm.h>
#include <common/kprint.h>
#include <common/macro.h>

#include "buddy.h"
#include "slab.h"

extern unsigned long *img_end;
struct global_mem global_mem;

#define PHYSICAL_MEM_START (24 * 1024 * 1024) //24M

#define START_VADDR phys_to_virt(PHYSICAL_MEM_START) //24M
#define NPAGES      (128 * 1000)

#define PHYSICAL_MEM_END (PHYSICAL_MEM_START + NPAGES * BUDDY_PAGE_SIZE)

extern void parse_mem_map(void *);
extern void arch_mm_init(void);

int physmem_map_num;
u64 physmem_map[8][2];

/*
 * Layout:
 *
 * | metadata (npages * sizeof(struct page)) | start_vaddr ... (npages * PAGE_SIZE) |
 *
 */

unsigned long get_ttbr1()
{
	unsigned long pgd;
	__asm__("mrs %0,ttbr1_el1" : "=r"(pgd));
	return pgd;
}

/*
 * map_kernel_space: map the kernel virtual address 
 * [va:va+size] to physical addres [pa:pa+size].
 * 1. get the kernel pgd address
 * 2. fill the block entry with corresponding attribution bit
 * 
 */
void map_kernel_space(vaddr_t va, paddr_t pa, size_t len)
{
	// copied from boot/mmu.c
#define MASK(n) (BIT(n) - 1)

#define ARM_1GB_BLOCK_BITS 30
#define ARM_2MB_BLOCK_BITS 21

#define PGDE_SIZE_BITS 3
#define PGD_BITS       9
#define PGD_SIZE_BITS  (PGD_BITS + PGDE_SIZE_BITS)

#define PUDE_SIZE_BITS 3
#define PUD_BITS       9
#define PUD_SIZE_BITS  (PUD_BITS + PUDE_SIZE_BITS)

#define PMDE_SIZE_BITS 3
#define PMD_BITS       9
#define PMD_SIZE_BITS  (PMD_BITS + PMDE_SIZE_BITS)

#define GET_PGD_INDEX(x) \
	(((x) >> (ARM_2MB_BLOCK_BITS + PMD_BITS + PUD_BITS)) & MASK(PGD_BITS))
#define GET_PUD_INDEX(x) \
	(((x) >> (ARM_2MB_BLOCK_BITS + PMD_BITS)) & MASK(PUD_BITS))

	//lab2
	u64 *pgd = (u64 *)get_ttbr1();
	u64 *pud = (u64 *)(pgd[GET_PGD_INDEX(KBASE)] &
			   ~3); // clear the lowest 2 bits
	u64 *pmd = (u64 *)(pud[GET_PUD_INDEX(KBASE)] &
			   ~3); // clear the lowest 2 bits
	//128M~256M
	for (u64 i = 64; i < 128; ++i) {
		// same as boot/mmu.c#init_boot_pt
		pmd[i] = (i << 21) | BIT(54) | BIT(10) /* access flag */
			 | (3 << 8) /* shareability */
			 | (4 << 2) /* MT_NORMAL memory */
			 | BIT(0); /* 2M block */
	}
}

void kernel_space_check()
{
	unsigned long kernel_val;
	for (unsigned long i = 64; i < 128; i++) {
		kernel_val = *(unsigned long *)(KBASE + (i << 21));
		kinfo("kernel_val: %lx\n", kernel_val);
	}
	kinfo("kernel space check pass\n");
}

void mm_init(void *info)
{
	vaddr_t free_mem_start = 0;
	vaddr_t free_mem_end = 0;
	struct page *page_meta_start = NULL;
	u64 npages = 0;
	u64 start_vaddr = 0;

	physmem_map_num = 0;

	/* only use the last entry (biggest free chunk) */
	parse_mem_map(info);

	if (physmem_map_num == 1) {
		free_mem_start = phys_to_virt(physmem_map[0][0]);
		free_mem_end = phys_to_virt(physmem_map[0][1]);

		npages = (free_mem_end - free_mem_start) /
			 (PAGE_SIZE + sizeof(struct page));
		start_vaddr = (free_mem_start + npages * sizeof(struct page));
		start_vaddr = ROUND_UP(start_vaddr, PAGE_SIZE);
		kdebug("[CHCORE] mm: free_mem_start is 0x%lx, free_mem_end is 0x%lx\n",
		       free_mem_start, free_mem_end);
	} else if (physmem_map_num == 0) {
		free_mem_start =
			phys_to_virt(ROUND_UP((vaddr_t)(&img_end), PAGE_SIZE));
		// free_mem_end = phys_to_virt(PHYSICAL_MEM_END);
		npages = NPAGES;
		start_vaddr = START_VADDR;
		kdebug("[CHCORE] mm: free_mem_start is 0x%lx, free_mem_end is 0x%lx\n",
		       free_mem_start, phys_to_virt(PHYSICAL_MEM_END));
	} else {
		BUG("Unsupport physmem_map_num\n");
	}

	if ((free_mem_start + npages * sizeof(struct page)) > start_vaddr) {
		BUG("kernel panic: init_mm metadata is too large!\n");
	}

	page_meta_start = (struct page *)free_mem_start;
	kdebug("page_meta_start: 0x%lx, real_start_vadd: 0x%lx,"
	       "npages: 0x%lx, meta_page_size: 0x%lx\n",
	       page_meta_start, start_vaddr, npages, sizeof(struct page));

	/* buddy alloctor for managing physical memory */
	init_buddy(&global_mem, page_meta_start, start_vaddr, npages);

	/* slab alloctor for allocating small memory regions */
	init_slab();

	/* init PCID */
	arch_mm_init();

	map_kernel_space(KBASE + (64UL << 21), 64UL << 21, 64UL << 21);
	//check whether kernel space [KABSE + 128 : KBASE + 256] is mapped
	// kernel_space_check();
}
