/* flags: -mm -c -O -G */
/* The conventional heap (see HEAP.C): how much of it is free (GAME.EXE 2E29) */

/* an entry of the heap's block table (the table grows down from the top of
   the heap, D_2E6E is its lowest entry) */
struct heapent {
	long addr;	/* +0 linear address of the block */
	long info;	/* +4 length (bits 0-19), 40000000h fixed, 80000000h in use */
};

extern long D_2E66;	/* linear address of the heap */
extern long D_2E6A;	/* its length */
extern long D_2E6E;	/* linear address of the lowest table entry */

extern struct heapent far * pascal LinearToPtr(long linear);

/* the total length of the free blocks */
/*C_29F0_0009*/long HEAP_totalFree(void)
{
	struct heapent far *e;
	long a;
	long total;

	total = 0;
	for (a = D_2E66 + D_2E6A - 8; a >= D_2E6E; a -= 8) {
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
	for (a = D_2E66 + D_2E6A - 8; a >= D_2E6E; a -= 8) {
		e = LinearToPtr(a);
		if (!(e->info & 0x80000000L) && (e->info & 0xFFFFFL) > best)
			best = e->info & 0xFFFFFL;
	}
	return best;
}
