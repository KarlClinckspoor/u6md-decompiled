/* flags: -mm -c -O -G */
/* The conventional heap (see HEAP.C): how much of it is free (GAME.EXE 29F0; SE's GAME.EXE 2E29) */

/* an entry of the heap's block table (the table grows down from the top of
   the heap, heapTable is its lowest entry) */
struct heapent {
	long addr;	/* +0 linear address of the block */
	long info;	/* +4 length (bits 0-19), 40000000h fixed, 80000000h in use */
};

extern long heapStart;	/* linear address of the heap */
extern long heapLength;	/* its length */
extern long heapTable;	/* linear address of the lowest table entry */

extern struct heapent far * pascal LinearToPtr(long linear);

/* the total length of the free blocks */
/*C_29F0_0009*/long HEAP_totalFree(void)
{
	struct heapent far *e;
	long a;
	long total;

	total = 0;
	for (a = heapStart + heapLength - 8; a >= heapTable; a -= 8) {
		e = LinearToPtr(a);
		if (!(e->info & 0x80000000L))
			total += e->info & 0xFFFFFL;
	}
	return total;
}

/* the length of the largest free block */
/*C_29F0_0098*/long HEAP_largestFree(void)
{
	struct heapent far *e;
	long a;
	long best;

	best = 0;
	for (a = heapStart + heapLength - 8; a >= heapTable; a -= 8) {
		e = LinearToPtr(a);
		if (!(e->info & 0x80000000L) && (e->info & 0xFFFFFL) > best)
			best = e->info & 0xFFFFFL;
	}
	return best;
}
