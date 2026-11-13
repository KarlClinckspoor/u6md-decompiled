/* flags: -mm -c -O -G */
/* compiler: Turbo C++ 1.01 */
/* Memory: the two heaps together, the EMS heap (EMSINIT, EMSALLOC, EMSFREE,
   EMSMAP) and the conventional one (HEAP, HEAPFREE) (Savage Empire's
   MEMORY.C, GAME.EXE 2E3C, with Martian Dreams' changes: MEM_alloc sets up
   the conventional heap if needed, MEM_free compacts the EMS heap, MEM_size
   is new).
   The flags of each call say which: 4 the EMS heap first (else the
   conventional one), 8 the other one too if the first fails; 10h is passed
   on to the conventional heap. */

extern int EMS_init(void);
extern void EMS_reset(void);
extern void EMS_shutdown(void);
extern void far *EMS_alloc(long size);
extern void EMS_free(void far *p);
extern void far *EMS_map();
extern void pascal EMS_merge(void far *p);	/* merges free EMS blocks */
extern long emsMapped;
extern long EMS_totalFree(void);
extern long EMS_largestFree(void);
extern void HEAP_reset(int flags);
extern int HEAP_init(void);
extern void HEAP_shutdown(void);
extern void far *HEAP_alloc(long size, int flags);
extern void HEAP_free(void far *p);
extern long HEAP_size(void far *p);
extern int heapReady;
extern long HEAP_totalFree(void);
extern long HEAP_largestFree(void);

/* empty the heap(s): everything allocated is freed */
/*C_1339_000B*/void MEM_reset(int flags)
{
	if (flags & 4)
		EMS_reset();
	else
		HEAP_reset(flags);
	if (flags & 8) {
		if (flags & 4)
			HEAP_reset(flags);
		else
			EMS_reset();
	}
}

/* set up the heap(s) */
/*C_1339_0045*/int MEM_init(int flags)
{
	if (flags & 4)
		EMS_init();
	else
		HEAP_init();
	if (flags & 8) {
		if (flags & 4)
			HEAP_init();
		else
			EMS_init();
	}
	return 1;
}

/* give the heap(s) back */
/*C_1339_007C*/void MEM_shutdown(int flags)
{
	if (flags & 4)
		EMS_shutdown();
	else
		HEAP_shutdown();
	if (flags & 8) {
		if (flags & 4)
			HEAP_shutdown();
		else
			EMS_shutdown();
	}
}

/* allocate size bytes; NULL if there is not enough memory */
/*C_1339_00B0*/void far *MEM_alloc(long size, int flags)
{
	void far *p;

	if (!heapReady)
		HEAP_init();
	if (flags & 4)
		p = EMS_alloc(size);
	else
		p = HEAP_alloc(size, flags);
	if (p == 0 && (flags & 8)) {
		if (flags & 4)
			p = HEAP_alloc(size, flags);
		else
			p = EMS_alloc(size);
	}
	return p;
}

/* free what MEM_alloc allocated (an EMS pointer has segment C000h-FFFFh) */
/*C_1339_0135*/void MEM_free(void far *p)
{
	if ((((unsigned)((unsigned long)p >> 16)) & 0xC000) == 0xC000) {
		EMS_free(p);
		EMS_merge(p);
	} else
		HEAP_free(p);
}

/* the usable length of the block p points into */
/*C_1339_016E*/long MEM_size(void far *p)
{
	long len;
	long saved;
	long far *q;

	if ((((unsigned)((unsigned long)p >> 16)) & 0xC000) == 0xC000) {
		saved = emsMapped;
		q = EMS_map(p);
		len = q[-1] & 0xFFFFFFFEL;
		EMS_map(saved);
		return len;
	}
	return HEAP_size(p);
}

/* the total free memory */
/*C_1339_01E0*/long MEM_totalFree(int flags)
{
	long n;

	n = 0;
	if (flags & 4)
		n += EMS_totalFree();
	else
		n += HEAP_totalFree();
	if (flags & 8) {
		if (flags & 4)
			n += HEAP_totalFree();
		else
			n += EMS_totalFree();
	}
	return n;
}

/* the largest block that can be allocated */
/*C_1339_0235*/long MEM_largestFree(int flags)
{
	long ems;
	long conv;

	ems = conv = 0;
	if ((flags & 4) || (flags & 8))
		ems = EMS_largestFree();
	if (!(flags & 4) || (flags & 8))
		conv = HEAP_largestFree();
	return ems > conv ? ems : conv;
}
