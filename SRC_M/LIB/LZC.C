/* flags: -mm -c -O -G */
/* compiler: Turbo C++ 1.01 */
/* OSI library: decompressing an LZW compressed library entry (see LZW.ASM):
   LZC_decompress of Savage Empire's LZC.C (SE's GAME.EXE 2EC5). In Martian Dreams
   LZC.C is three modules, linked apart: this one (13A3), LZCTYPE.C (13DD,
   loading by the entry types of LIBSIZE.C) and LZCLOAD.C (13F3, SE's
   LZC_load); LIBSIZE.C sits between the first two. A compressed entry starts
   with the length of its data (a long). The decompression needs a work area
   of 3020h bytes: the dictionary (3000h) and an input buffer (400h, which may
   overlap the end of it); lzcWorkArea, if set, is one to use. */

#include <dos.h>
#include <stdlib.h>

/* an open library entry (LIBOPEN.C) */
struct libfile {
	int handle;	/* +00 */
	int last;	/* +02 1: the last entry (it ends at the end of the file) */
	int count;	/* +04 entries in the library */
	int type;	/* +06 the offset's high byte (see LIBSIZE.C) */
	long start;	/* +08 offset of the entry in the file */
	long pos;	/* +0C position in the entry */
	long length;	/* +10 */
};

extern int osiError;		/* error code of the file routines */
extern int lzwNoRefill;		/* LZW.ASM: stop when the input runs out */
extern int lzwFile;		/* LZW.ASM: the file */
extern unsigned lzwDictSeg;		/* LZW.ASM: segment of the dictionary */
extern char far *lzwOut;	/* LZW.ASM: output */
extern char far *lzwIn;	/* LZW.ASM: input buffer */
extern unsigned lzwInSize;		/* LZW.ASM: its length */
extern int lzwBitPos;		/* LZW.ASM: bit offset */

extern void far *EMS_map(void far *p);
extern void LZW_decompress(void);
extern void far *MEM_alloc(long size, int flags);
extern void MEM_free(void far *p);
extern int pascal LIB_open(char far *name, int index, struct libfile *f);
extern void pascal LIB_close(struct libfile *f);
extern int pascal LIB_read(struct libfile *f, void far *buf, long n);
extern long pascal LIB_seek(struct libfile *f, long offset, int whence);
extern char far * pascal NormalizePtr(char far *p);

/*1CA4*/char far *lzcWorkArea = 0;	/* a work area to use (3020h bytes) */
/*1CA8*/void far *lzcData = 0;	/* the data loaded */
/*1CAC*/unsigned lzcWorkInSize = 0;	/* the length of the input buffer in it (0: 400h) */
/*1CAE*/int lzcAnyType = 0;	/* nonzero: LZC_decompress takes any type without bits C0h */

/* decompress the open entry f (type 1, or, while lzcAnyType is set, any type
   without bits C0h) to out, or, if out is NULL, to memory
   allocated with flags memflags (MEM_alloc); work: a work area or NULL.
   Returns the data, NULL on an error (osiError: 6 not compressed, 1 and 2 no
   memory for the work area, 5 seek error, 4 no memory for the data) */
/*C_13A3_0006*/void far * pascal LZC_decompress(register struct libfile *f, void far *out, register int memflags, char far *work)
{
	char far *tbl;
	char far *buf1;
	char far *buf2;
	char allocated;
	long len;
	void far *result;

	allocated = 0;
	result = 0;
	lzcData = 0;
	if (lzcAnyType) {
		if (f->type & 0xC0) {
			osiError = 6;
			return 0;
		}
	} else if (f->type != 1) {
		osiError = 6;
		return 0;
	}
	if (work == 0 && lzcWorkArea != 0) {
		work = lzcWorkArea;
		lzcWorkInSize = 0;
	}
	if (work == 0) {
		/* near memory first (tested and freed as near pointers) */
		buf1 = (char *)malloc(0x3020);
		buf2 = (char *)malloc(0x410);
		if (FP_OFF(buf1) == 0 || FP_OFF(buf2) == 0) {
			if (FP_OFF(buf1) != 0)
				free((char *)buf1);
			if (FP_OFF(buf2) != 0)
				free((char *)buf2);
			allocated = 1;
			buf1 = MEM_alloc(0x3000L, 0x22);
			if (buf1 == 0) {
				osiError = 1;
				return 0;
			}
			buf2 = MEM_alloc(0x400L, 0x22);
			if (buf2 == 0) {
				MEM_free(buf1);
				osiError = 2;
				return 0;
			}
		}
		tbl = NormalizePtr(buf1);
		lzwIn = NormalizePtr(buf2);
		lzwInSize = 0x400;
	} else {
		tbl = NormalizePtr(work);
		lzwInSize = 0x400;
		len = 0x3020;
		lzwIn = work + len;
		lzwIn = NormalizePtr(lzwIn);
		if (lzcWorkInSize)
			lzwInSize = lzcWorkInSize;
	}
	lzwFile = f->handle;
	lzwNoRefill = 0;
	lzwBitPos = 0;
	lzwDictSeg = FP_SEG(tbl);
	if (FP_OFF(tbl))
		lzwDictSeg++;
	if (LIB_seek(f, 0L, 0) == -1L) {
		osiError = 5;
		return 0;
	}
	if (out == 0) {
		if (LIB_read(f, MK_FP(_DS, &len), 4L)) {
			lzcData = MEM_alloc(len, memflags);
			if (lzcData == 0)
				osiError = 4;
		}
	} else {
		if (LIB_seek(f, 4L, 0) == -1L) {
			osiError = 5;
			return 0;
		}
		lzcData = out;
	}
	result = lzcData;
	if (!LIB_read(f, lzwIn, lzwInSize))
		result = 0;
	lzwOut = EMS_map(result);
	if (result)
		LZW_decompress();
	if (work == 0) {
		if (allocated) {
			MEM_free(buf2);
			MEM_free(buf1);
		} else {
			free((char *)buf2);
			free((char *)buf1);
		}
	}
	return result;
}
