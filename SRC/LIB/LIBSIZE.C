/* flags: -mm -c -O -G */
/* compiler: Turbo C++ 1.01 */
/* OSI library: the length of a library entry's data (GAME.EXE 2AA2; Savage Empire's
   LIBSIZE.C, SE's GAME.EXE 2E8C, with Martian Dreams' entry types) */
#include <dos.h>

/* an open library entry (LIBOPEN.C) */
struct libfile {
	int handle;	/* +00 */
	int last;	/* +02 1: the last entry (it ends at the end of the file) */
	int count;	/* +04 entries in the library */
	int type;	/* +06 the offset's high byte: 0 stored, bits 40h/80h stored,
			   anything else LZW compressed (see LZC.C) */
	long start;	/* +08 offset of the entry in the file */
	long pos;	/* +0C position in the entry */
	long length;	/* +10 */
};

extern int osiError;	/* error code of the file routines */

extern int pascal LIB_open(char far *name, int index, struct libfile *f);
extern void pascal LIB_close(struct libfile *f);
extern int pascal LIB_read(struct libfile *f, void far *buf, long n);

/* the length of entry index of the library name, uncompressed (an LZW
   compressed entry starts with it); -1L on an error */
/*C_2AA2_0009*/long pascal LIB_entrySize(char far *name, int index)
{
	long size;
	struct libfile f;

	size = -1;
	if (LIB_open(name, index, &f)) {
		switch (f.type & 0xC0) {
		case 0:
			if (!(f.type & ~0xC0))
				size = f.length;
			else {
				LIB_read(&f, MK_FP(_DS, &size), 4L);
				if (osiError)
					size = -1;
			}
			break;
		default:
			size = f.length;
		}
		LIB_close(&f);
	}
	return size;
}
