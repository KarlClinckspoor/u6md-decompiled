/* flags: -mm -c -O -G */
/* compiler: Turbo C++ 1.01 */
/* OSI library: loading a library entry, LZW compressed (type 1, LZC.C) or
   not: LZC_load of Savage Empire's LZC.C (GAME.EXE 2EC5), a module of its
   own in Martian Dreams (see LZC.C). */

#include <dos.h>

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

extern void far *MEM_alloc(long size, int flags);
extern int pascal LIB_open(char far *name, int index, struct libfile *f);
extern void pascal LIB_close(struct libfile *f);
extern int pascal LIB_read(struct libfile *f, void far *buf, long n);

extern void far *lzcData;	/* LZC.C: the data loaded */
extern void far * pascal LZC_decompress(struct libfile *f, void far *out, int memflags, char far *work);

/* load entry index of the library name to out, or, if out is NULL, to
   memory allocated with flags memflags; work: a work area for an LZW
   compressed entry, or NULL. Returns the data, NULL on an error (osiError: 8
   the entry is empty, 4 no memory, or as LIB_open, LZC_decompress) */
/*C_13F3_0007*/void far * pascal LZC_load(char far *name, int index, void far *out, int memflags, char far *work)
{
	void far *p;
	struct libfile f;

	p = 0;
	if (LIB_open(name, index, &f)) {
		switch (f.type) {
		case 1:
			p = LZC_decompress(&f, out, memflags, work);
			break;
		default:
			if (f.length == 0) {
				p = 0;
				osiError = 8;
				break;
			}
			p = out;
			if (p == 0) {
				p = lzcData = MEM_alloc(f.length, memflags);
				if (p == 0)
					osiError = 4;
			}
			if (p != 0) {
				if (!LIB_read(&f, p, f.length))
					p = 0;
			}
		}
		LIB_close(&f);
	}
	return p;
}
