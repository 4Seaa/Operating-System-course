﻿#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

static struct {
	uint32_t proc;	// ID of process currently uses this page
	int index;	// Index of the page in the list of pages allocated
			// to the process.
	int next;	// The next page in the list. -1 if it is the last page.
} _mem_stat[NUM_PAGES];

static pthread_mutex_t mem_lock;

void init_mem(void) {
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* get offset of the virtual address */
static addr_t get_offset(addr_t addr) {
	return addr & ~((~0U) << OFFSET_LEN);
}

/* get the first layer index */
static addr_t get_first_lv(addr_t addr) {
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* get the second layer index */
static addr_t get_second_lv(addr_t addr) {
	return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
}

/* Search for page table table from the a segment table */
static struct page_table_t* get_page_table(
	addr_t index, 	// Segment level index
	struct seg_table_t* seg_table) { // first level table

/*
 * TODO: Given the Segment index [index], you must go through each
 * row of the segment table [seg_table] and check if the v_index
 * field of the row is equal to the index
 *
 * */

 //Author Ng.Trong.Trung

	int i;
	for (i = 0; i < seg_table->size; i++) {
		if (index == seg_table->table[i].v_index)
			return seg_table->table[i].pages;
	}
	return NULL;

}

/* Translate virtual address to physical address. If [virtual_addr] is valid,
 * return 1 and write its physical counterpart to [physical_addr].
 * Otherwise, return 0 */
static int translate(
	addr_t virtual_addr, 	// Given virtual address
	addr_t* physical_addr, // Physical address to be returned
	struct pcb_t* proc) {  // Process uses given virtual address

/* Offset of the virtual address */
	addr_t offset = get_offset(virtual_addr);
	/* The first layer index */
	addr_t first_lv = get_first_lv(virtual_addr);
	/* The second layer index */
	addr_t second_lv = get_second_lv(virtual_addr);

	/* Search in the first level */
	struct page_table_t* page_table = NULL;
	page_table = get_page_table(first_lv, proc->seg_table);
	if (page_table == NULL) {
		return 0;
	}

	int i;
	for (i = 0; i < page_table->size; i++) {
		if (page_table->table[i].v_index == second_lv) {
			/* TODO: Concatenate the offset of the virtual addess
			 * to [p_index] field of page_table->table[i] to
			 * produce the correct physical address and save it to
			 * [*physical_addr]  */
			*physical_addr = ((page_table->table[i].p_index) << OFFSET_LEN) + offset;
			return 1;
		}
	}
	return 0;
}

addr_t alloc_mem(uint32_t size, struct pcb_t* proc) {
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;
	/* TODO: Allocate [size] byte in the memory for the
	 * process [proc] and save the address of the first
	 * byte in the allocated memory region to [ret_mem].
	 * */

	uint32_t num_pages = ((size % PAGE_SIZE) == 0) ? size / PAGE_SIZE : size / PAGE_SIZE + 1; // Number of pages we will use
	int mem_avail = 0; // We could allocate new memory region or not?

	/* First we must check if the amount of free memory in
	 * virtual address space and physical address space is
	 * large enough to represent the amount of required
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */

	int checkvir = 0;//Check sl free pages trong virtual space đủ để allocate ko
	int checkphy = 0;//Check sl free pages trong physical space đủ để allocate ko
	uint32_t count = 0;//Số lượng free pages
	//Check Physical
	for (int i = 0; i < NUM_PAGES; i++) {
		if (_mem_stat[i].proc == 0) {//Check số lượng free pages trong physical space
			count += 1;
		}
		if (count == num_pages) {
			checkphy = 1;
			break;
		}
	}
	//Check Virtual
	if (proc->bp + num_pages * PAGE_SIZE <= (1 << ADDRESS_SIZE)) {//Check virtual memory space có còn đủ free pages cho process mới allocate ko (virtual memory space = 1 MB)
		checkvir = 1;
	}
	if (checkvir == 1 && checkphy == 1) {//Cả physical lẫn virtual đều đủ free pages cho process mới allocate
		mem_avail = 1;
	}
	if (mem_avail) {
		/* We could allocate new memory region to the process */
		ret_mem = proc->bp;
		proc->bp += num_pages * PAGE_SIZE;//Cập nhật lại break pointer ở virtual memory space sau khi allocate process mới
		/* Update status of physical pages which will be allocated
		 * to [proc] in _mem_stat. Tasks to do:
		 * 	- Update [proc], [index], and [next] field
		 * 	- Add entries to segment table page tables of [proc]
		 * 	  to ensure accesses to allocated memory slot is
		 * 	  valid. */
		int count = 0;
		int below[num_pages];//Số thứ tự của page đang xét hiện tại, dùng cho phần cập nhật p_index trong page_table ở dưới
		for (int i = 0; i < NUM_PAGES; i++) {//Update _mem_stat cho các pages của physical memory
			if (_mem_stat[i].proc == 0) {//Note: At the end nhớ check lại next, khá chắc là làm đúng r
				below[count] = i;
				_mem_stat[i].proc = proc->pid;
				_mem_stat[i].index = count;
				if (count != 0) {
					_mem_stat[below[count - 1]].next = i;
				}
				count += 1;
			}
			if (count == num_pages) {
				_mem_stat[below[count - 1]].next = -1;
				break;
			}
		}

		addr_t viraddr[num_pages];//Mảng chứa các Virtual address mỗi page của process
		for (int i = 0; i < num_pages; i++) {
			viraddr[i] = ret_mem + i * PAGE_SIZE;
		}
		count = 1; //Đếm số lượng segment index khác nhau trong các pages mới allocate của process hiện tại
		for (int i = 1; i < num_pages; i++) {
			if (get_first_lv(viraddr[i]) != get_first_lv(viraddr[i - 1])) {
				count += 1;
			}
		}
		addr_t differ_first_lv[count];//Mảng các phần tử là các segment index khác nhau của process hiện tại
		count = 1;
		differ_first_lv[0] = get_first_lv(viraddr[0]);
		for (int i = 1; i < num_pages; i++) {
			if (get_first_lv(viraddr[i]) != get_first_lv(viraddr[i - 1])) {
				differ_first_lv[count] = get_first_lv(viraddr[i]);
				count += 1;
			}
		}
		int check[count];//Check các segment index của process hiện tại đã nằm trong segment table chưa, nếu chưa thì mới push vô 
		for (int k = 0; k < count; k++) {
			check[k] = 0;
		}
		for (int i = 0; i < proc->seg_table->size; i++) {
			for (int j = 0; j < count; j++) {
				if (differ_first_lv[j] == proc->seg_table->table[i].v_index) {
					check[j] = 1;
				}
			}
		}
		for (int i = 0; i < count; i++) {//Cập nhật các segment index và các page_table mới cho segment table
			if (check[i] == 0) {
				proc->seg_table->table[proc->seg_table->size].v_index = differ_first_lv[i];
				proc->seg_table->table[proc->seg_table->size].pages = (struct page_table_t*)malloc(sizeof(struct page_table_t));
				proc->seg_table->size += 1;
			}
		}

		for (int i = 0; i < num_pages; i++) {//Cập nhật ở page_table
			for (int j = 0; j < proc->seg_table->size; j++) {
				if (get_first_lv(viraddr[i]) == proc->seg_table->table[j].v_index) {
					proc->seg_table->table[j].pages->table[proc->seg_table->table[j].pages->size].v_index = get_second_lv(viraddr[i]);
					proc->seg_table->table[j].pages->table[proc->seg_table->table[j].pages->size].p_index = below[i] % 32;//Do mỗi segment index có 32 pages
					proc->seg_table->table[j].pages->size += 1;
				}
			}
		}
	}
	pthread_mutex_unlock(&mem_lock);
	return ret_mem;
}

int free_mem(addr_t address, struct pcb_t* proc) {
	/*TODO: Release memory region allocated by [proc]. The first byte of
	 * this region is indicated by [address]. Task to do:
	 * 	- Set flag [proc] of physical page use by the memory block
	 * 	  back to zero to indicate that it is free.
	 * 	- Remove unused entries in segment table and page tables of
	 * 	  the process [proc].
	 * 	- Remember to use lock to protect the memory from other
	 * 	  processes.  */
	pthread_mutex_lock(&mem_lock);
	int num_pages = 0;					// Number of pages we will use
	addr_t physical_addr;
	addr_t virtual_addr = address;
	int i;
	if (translate(address, &physical_addr, proc)) {	// check address is valid and get physical_addr
		addr_t physical_page = physical_addr >> OFFSET_LEN;

		while (physical_page != -1) {
			_mem_stat[physical_page].proc = 0;
			addr_t segIndex = get_first_lv(virtual_addr);
			for (i = 0; i < proc->seg_table->table[segIndex].pages->size; i++)
				if (proc->seg_table->table[segIndex].pages->table[i].p_index == physical_page) {
					proc->seg_table->table[segIndex].pages->table[i].v_index = 0;
					proc->seg_table->table[segIndex].pages->table[i].p_index = 0;
				}
			physical_page = _mem_stat[physical_page].next;
			virtual_addr += PAGE_SIZE;
		}
	}

	pthread_mutex_unlock(&mem_lock);
	return 0;
}

int read_mem(addr_t address, struct pcb_t* proc, BYTE* data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		*data = _ram[physical_addr];
		return 0;
	}
	else {
		return 1;
	}
}
*/
int write_mem(addr_t address, struct pcb_t* proc, BYTE data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		_ram[physical_addr] = data;
		return 0;
	}
	else {
		return 1;
	}
}

void dump(void) {
	int i;
	for (i = 0; i < NUM_PAGES; i++) {
		if (_mem_stat[i].proc != 0) {
			printf("%03d: ", i);
			printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
				i << OFFSET_LEN,
				((i + 1) << OFFSET_LEN) - 1,
				_mem_stat[i].proc,
				_mem_stat[i].index,
				_mem_stat[i].next
			);
			int j;
			for (j = i << OFFSET_LEN; j < ((i + 1) << OFFSET_LEN) - 1; j++) {

				if (_ram[j] != 0) {
					printf("\t%05x: %02x\n", j, _ram[j]);
				}

			}
		}
	}
}
