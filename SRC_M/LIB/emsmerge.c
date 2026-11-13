/* flags: -mm -c -O -G */ /* compiler: tcpp101 (Turbo C++ 1.01) */
/*
 * EMS memory: merging free blocks of the EMS heap (a block: a 4-byte header, its length
 * as a long with bit 0 set while in use, then the data; see LIB/EMSALLOC.ASM). A library
 * module of MARTIAN.EXE's (not in SAVAGE.EXE), compiled with Turbo C++ 1.01.
 * The original reads the header of q for `used` as a word (mov ax,es:[bx-4]; and ax,1;
 * mov [bp-11h],al); every C form tried gives a byte read (TC narrows the AND; a 1-bit
 * field gives mov al / and ax,1), so that statement is inline assembly.
 */
/* fixup order: 0114 00F7 00C6 0080 0064 0043 000F (Turbo C wrote this module's fixups last to first; MASM, which assembles it because of the inline asm, writes them in code order: tools/md/tasmorder.py puts them back) */

extern void far *emsMapped;/*the virtual pointer mapped in (LIB/EMSALLOC.ASM)*/
extern void far *EMS_map(void far *p);/*maps virtual pointer p in, returns its real address*/
extern void far * pascal EMS_next(void far *p);/*the block after p; 0 at the end (LIB/EMSNEXT.ASM)*/
extern long pascal PtrToLinear(void far *p);/*LIB/PTRLIN.ASM*/

/*joins every free block that follows a free block to it, from the first block of the heap
  up to the block at limit; the mapping is left as it was*/
/*C_1208_0004*/void pascal EMS_merge(void far *limit) {
	unsigned long end;
	void far *saved;
	void far *p;
	void far *prev;
	char used;
	unsigned long size;
	char far *q;
	char far *r;

	end = PtrToLinear(limit);
	saved = emsMapped;
	p = (void far *)0xc0000004L;
	do {
		prev = p;
		q = EMS_map(p);
		/* used = *(unsigned long far *)(q - 4) & 1; TC narrows this to a byte read: */
		asm les bx,q
		asm mov ax,es:[bx-4]
		asm and ax,1
		asm mov used,al
		p = EMS_next(p);
		if(p == 0)
			break;
		r = EMS_map(p);
		if(!used && !(*(unsigned long far *)(r - 4) & 1)) {
			size = (*(unsigned long far *)(r - 4) & 0xfffffffeL) + 4;
			q = EMS_map(prev);
			*(unsigned long far *)(q - 4) += size;
			p = prev;
		}
	} while(PtrToLinear(p) <= end);
	EMS_map(saved);
}
