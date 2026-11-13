/* flags: -mm -c -O -G */
/* compiler: Turbo C++ 1.01 */
/* OSI library: loading a library entry by the type in its offset's high
   byte (see LIBSIZE.C): LZW compressed (LZC.C), stored, or through one of
   two loaders set by the program. Martian Dreams' variant of Savage Empire's
   LZC_load (LZCLOAD.C). */

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
extern int lzcAnyType;	/* LZC.C: LZC_decompress takes any type without bits C0h */
extern void far * pascal LZC_decompress(struct libfile *f, void far *out, int memflags, char far *work);

typedef void far *(*tLoader)(struct libfile *f, void far *out, int memflags, char far *work);

/*1CB0*/tLoader lzcLoader40 = 0;	/* loads the entries of type 40h (with other bits) */
/*1CB4*/tLoader lzcLoader80 = 0;	/* loads the entries of type 80h (with other bits) */

/* load entry index of the library name to out, or, if out is NULL, to
   memory allocated with flags memflags; work: a work area for an LZW
   compressed entry, or NULL. The type's bits C0h choose: 0 LZW compressed,
   40h lzcLoader40, 80h lzcLoader80; C0h, or no other bit set, stored.
   Returns the data, NULL on an error (osiError: 8 the entry is empty, 4 no
   memory, or as LIB_open, LZC_decompress) */
/*C_13DD_0007*/void far * pascal LZC_loadType(char far *name, int index, void far *out, register int memflags, char far *work)
{
	void far *p;
	struct libfile f;
	register int kind;

	p = 0;
	if (LIB_open(name, index, &f)) {
		kind = f.type & 0xC0;
		if (!(f.type & 0xFF3F))
			kind = 0xC0;
		switch (kind) {
		case 0:
			lzcAnyType++;
			p = LZC_decompress(&f, out, memflags, work);
			lzcAnyType--;
			break;
		case 0x40:
			p = (*lzcLoader40)(&f, out, memflags, work);
			break;
		case 0x80:
			p = (*lzcLoader80)(&f, out, memflags, work);
			break;
		case 0xC0:
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
			break;
		}
		LIB_close(&f);
	}
	return p;
}
