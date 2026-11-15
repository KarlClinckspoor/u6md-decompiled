/* flags: -mm -c -O -G */
/* compiler: Turbo C++ 1.01 */
/* The conventional heap (GAME.EXE 2924; Savage Empire's HEAP.C, SE's GAME.EXE 2D70, a little
   later): one block of DOS memory, taken with farmalloc() at the start,
   given out in blocks. Addresses are linear (longs; LinearToPtr makes a far
   pointer of one, PtrToLinear the other way). The block table is at the top
   of the heap and grows down: one 8-byte entry per block, in the order of
   the blocks (the lowest entry, heapTable, is the lowest block);
   heapSpareEntries entries are kept for it. On a machine with 624 KB (BIOS:
   270h KB) the heap stops at 98000h and what was above it is moved up to
   9C000h (and back at the end). Martian Dreams' version marks word- and
   paragraph-aligned blocks (so HEAP_size can give their usable length) and
   initialises the heap's variables. */

#include <alloc.h>

struct heapent {
	long addr;	/* +0 linear address of the block */
	long info;	/* +4 length (bits 0-19), 10000000h word aligned (one byte
			   more), 20000000h paragraph aligned (15 more),
			   40000000h fixed, 80000000h in use */
};

/*F24A*/extern void far *heapBlock;	/* the heap, as farmalloc() gave it (BSS, 4 bytes) */

extern struct heapent far * pascal LinearToPtr(long linear);
extern long pascal PtrToLinear(void far *p);
extern void pascal MemMove(void far *dest, void far *src, unsigned n);

/*2E62*/int heapReady = 0;		/* 1: the heap is set up */
/*2E64*/int heapSpareEntries = 0x80;	/* table entries kept at the top */
/*2E66*/long heapStart = 0;		/* linear address of the heap */
/*2E6A*/long heapLength = 0;		/* its length */
/*2E6E*/long heapTable = 0;		/* linear address of the lowest table entry */
/*2E72*/long heapMovedBytes = 0;	/* bytes moved from below 9C000h (624 KB machines) */

/* merge the free block of entry a with the next one (entry a + 8) if that is
   free too; the entries below a move up. 1 if merged */
/*C_2924_0003*/int HEAP_merge(long a)
{
	struct heapent far *e;

	e = LinearToPtr(a);
	if (!(e->info & 0x80000000L) && !(e[1].info & 0x80000000L)
	    && e->addr + (e->info & 0xFFFFFL) == e[1].addr) {
		e[1].addr = e->addr;
		e[1].info += e->info & 0xFFFFFL;
		while (a > heapTable) {
			a -= 8;
			e = LinearToPtr(a);
			e[1].addr = e->addr;
			e[1].info = e->info;
		}
		heapTable += 8;
		return 1;
	}
	return 0;
}

/* free the block of entry a and merge it with free neighbours; returns its
   entry's address now */
/*C_2924_0105*/long HEAP_freeEntry(long a)
{
	struct heapent far *e;

	e = LinearToPtr(a);
	e->info &= 0x7FFFFFFFL;
	if (a + 8 < heapStart + heapLength) {
		if (HEAP_merge(a))
			a += 8;
	}
	if (a > heapTable)
		HEAP_merge(a - 8);
	return a;
}

/* empty the heap: with flags 10h, one free block over all of it; otherwise
   free every block that is not fixed */
/*C_2924_019D*/void HEAP_reset(int flags)
{
	long a;
	struct heapent far *e;

	if (heapReady) {
		if (flags & 0x10) {
			heapTable = heapStart + heapLength - 8;
			e = LinearToPtr(heapTable);
			e->addr = heapStart;
			e->info = heapLength - heapSpareEntries * 8;
		} else {
			for (a = heapStart + heapLength - 8; a >= heapTable; a -= 8) {
				e = LinearToPtr(a);
				if (!(e->info & 0x40000000L))
					a = HEAP_freeEntry(a);
			}
		}
	}
}

/* set up the heap: all the DOS memory left; returns heapReady (1 if done) */
/*C_2924_0290*/int HEAP_init(void)
{
	if (!heapReady) {
		heapLength = farcoreleft();
		heapBlock = 0;
		if (heapSpareEntries * 8 < heapLength)
			heapBlock = farmalloc(heapLength);
		if (heapBlock) {
			heapReady++;
			heapStart = PtrToLinear(heapBlock);
			if (*(int far *)0x00400013L == 0x270) {
				heapMovedBytes = 0x9C000L - (heapLength + heapStart);
				heapLength = 0x98000L - (heapMovedBytes + heapStart);
				if (heapSpareEntries * 8 < heapLength) {
					MemMove(LinearToPtr(heapStart + heapLength),
						LinearToPtr(0x9C000L - heapMovedBytes), heapMovedBytes);
				} else {
					farfree(heapBlock);
					heapReady = 0;
				}
			}
			if (heapReady)
				HEAP_reset(0x10);
		}
	}
	return heapReady;
}

/* give the heap back (moving back what was moved) */
/*C_2924_03D9*/void HEAP_shutdown(void)
{
	if (heapMovedBytes)
		MemMove(LinearToPtr(0x9C000L - heapMovedBytes),
			LinearToPtr(heapStart + heapLength), heapMovedBytes);
	if (heapReady)
		farfree(heapBlock);
	heapReady = 0;
}

/* allocate size bytes at the top of the heap (the last free block that is
   large enough); see HEAP_alloc */
/*C_2924_043C*/void far *HEAP_allocTop(long size, int flags)
{
	long a;
	long result;
	long len;
	long b;
	long flagbits;
	struct heapent far *e;
	struct heapent far *e2;

	if (size <= 0)
		return 0;
	flagbits = 0x80000000L;
	if ((flags & 3) == 1) {
		flagbits |= 0x10000000L;
		size += 1;
	} else if ((flags & 3) == 2) {
		flagbits |= 0x20000000L;
		size += 15;
	}
	if (flags & 0x10)
		flagbits |= 0x40000000L;
	result = 0;
	for (a = heapStart + heapLength - 8; a >= heapTable; a -= 8) {
		e = LinearToPtr(a);
		len = e->info & 0xFFFFFL;
		if (!(e->info & 0x80000000L) && size <= len) {
			if (size < len) {
				if (heapStart + heapLength - heapSpareEntries * 8 >= heapTable) {
					e2 = LinearToPtr(heapStart + heapLength - 8);
					if (!(e2->info & 0x80000000L)
					    && e2->addr + (e2->info & 0xFFFFFL) == heapTable
					    && (e2->info & 0xFFFFFL) > 8) {
						e2->info -= 8;
						heapSpareEntries++;
					}
				}
				if (heapStart + heapLength - heapSpareEntries * 8 < heapTable) {
					heapTable -= 8;
					for (b = heapTable; b < a; b += 8) {
						e2 = LinearToPtr(b);
						e2->addr = e2[1].addr;
						e2->info = e2[1].info;
					}
					a -= 8;
					e = LinearToPtr(a);
					e->info -= size;
					e[1].addr += e->info & 0xFFFFFFL;
					e[1].info = size + flagbits;
					result = e[1].addr;
					break;
				}
			} else {
				e->info |= flagbits;
				result = e->addr;
				break;
			}
		}
	}
	if (result) {
		if ((flags & 3) == 1)
			result = (result + 1) & 0xFFFFFFFEL;
		else if ((flags & 3) == 2)
			result = (result + 15) & 0xFFFFFFF0L;
	}
	return LinearToPtr(result);
}

/* allocate size bytes (the first free block that is large enough); flags:
   1 word aligned, 2 paragraph aligned, 10h fixed (HEAP_reset leaves it),
   20h at the top of the heap. NULL if there is no room */
/*C_2924_079D*/void far *HEAP_alloc(long size, int flags)
{
	long a;
	long result;
	long len;
	long b;
	long flagbits;
	struct heapent far *e;
	struct heapent far *e2;

	if (flags & 0x20)
		return HEAP_allocTop(size, flags);
	if (size <= 0)
		return 0;
	flagbits = 0x80000000L;
	if ((flags & 3) == 1) {
		flagbits |= 0x10000000L;
		size += 1;
	} else if ((flags & 3) == 2) {
		flagbits |= 0x20000000L;
		size += 15;
	}
	if (flags & 0x10)
		flagbits |= 0x40000000L;
	result = 0;
	for (a = heapTable; a < heapStart + heapLength; a += 8) {
		e = LinearToPtr(a);
		len = e->info & 0xFFFFFL;
		if (!(e->info & 0x80000000L) && size <= len) {
			if (size < len) {
				if (heapStart + heapLength - heapSpareEntries * 8 >= heapTable) {
					e2 = LinearToPtr(heapStart + heapLength - 8);
					if (!(e2->info & 0x80000000L)
					    && e2->addr + (e2->info & 0xFFFFFL) == heapTable
					    && (e2->info & 0xFFFFFL) > 8) {
						e2->info -= 8;
						heapSpareEntries++;
					}
				}
				if (heapStart + heapLength - heapSpareEntries * 8 < heapTable) {
					heapTable -= 8;
					for (b = heapTable; b < a; b += 8) {
						e2 = LinearToPtr(b);
						e2->addr = e2[1].addr;
						e2->info = e2[1].info;
					}
					a -= 8;
					e = LinearToPtr(a);
					e->info = size + flagbits;
					e[1].addr += size;
					e[1].info -= size;
					result = e->addr;
					break;
				}
			} else {
				e->info |= flagbits;
				result = e->addr;
				break;
			}
		}
	}
	if (result) {
		if ((flags & 3) == 1)
			result = (result + 1) & 0xFFFFFFFEL;
		else if ((flags & 3) == 2)
			result = (result + 15) & 0xFFFFFFF0L;
	}
	return LinearToPtr(result);
}

/* free the block holding p */
/*C_2924_0AFE*/void HEAP_free(void far *p)
{
	long a;
	long lin;
	struct heapent far *e;

	lin = PtrToLinear(p);
	for (a = heapStart + heapLength - 8; a >= heapTable; a -= 8) {
		e = LinearToPtr(a);
		if (e->addr <= lin && e->addr + (e->info & 0xFFFFFL) > lin) {
			HEAP_freeEntry(a);
			break;
		}
	}
}

/* the usable length of the block holding p (without what its alignment
   took); 0 if p is not in the heap */
/*C_2924_0BB2*/long HEAP_size(void far *p)
{
	long a;
	long lin;
	long len;
	struct heapent far *e;

	len = 0;
	lin = PtrToLinear(p);
	for (a = heapStart + heapLength - 8; a >= heapTable; a -= 8) {
		e = LinearToPtr(a);
		if (e->addr <= lin && e->addr + (e->info & 0xFFFFFL) > lin) {
			len = e->info & 0xFFFFFL;
			if (e->info & 0x10000000L)
				len -= 1;
			else if (e->info & 0x20000000L)
				len -= 15;
			break;
		}
	}
	return len;
}
