/* flags: -mm -c -O -G */
/* compiler: Turbo C++ 1.01 */
/* OSI library: opening one entry of a library file (Savage Empire's LIBOPEN.C,
   GAME.EXE 2E94; here the buffers are passed as DS far pointers). A library
   starts with its length (a long), then the offset of each entry (a long: the
   high byte is the entry's type); the first offset tells how many there are. */

/* an open library entry */
struct libfile {
	int handle;	/* +00 */
	int last;	/* +02 1: the last entry (it ends at the end of the file) */
	int count;	/* +04 entries in the library */
	int type;	/* +06 the offset's high byte (see LIBSIZE.C) */
	long start;	/* +08 offset of the entry in the file */
	long pos;	/* +0C position in the entry */
	long length;	/* +10 */
};

#include <dos.h>
#pragma inline	/* Origin compiled this module through the assembler: its fixups are in code order */

extern int osiError;	/* error code of the file routines */

extern int pascal DOS_open(char far *name);
extern void pascal DOS_close(int handle);
extern long pascal DOS_read(int handle, long pos, long size, void far *buf);
extern long pascal DOS_seek(int handle, long pos, int whence);

/* open entry index of the library name into f; 1 if done (0: osiError = 3 if
   there is no such entry) */
/*C_2ACF_000B*/int pascal LIB_open(char far *name, int index, struct libfile *f)
{
	int count;
	int type;
	long filesize;
	long first;
	long pos;
	long end;
	long start;
	int handle;

	handle = DOS_open(name);
	if (handle == -1)
		goto fail;
	if (!DOS_read(handle, 0L, 4L, MK_FP(_DS, &filesize)))
		goto fail;
	if (!DOS_read(handle, 4L, 4L, MK_FP(_DS, &first)))
		goto fail;
	/* through a near pointer: the original computes the long difference
	   (add/adc) and keeps its low word, where an int cast gives dec ax */
	count = (int)(void near *)(first / 4 - 1);
	if (index >= count) {
		osiError = 3;
		goto fail;
	}
	pos = (index + 1) * 4U;
	if (!DOS_read(handle, pos, 4L, MK_FP(_DS, &start)))
		goto fail;
	/* the type is the offset's high byte (written with the pseudo-registers:
	   xor ah,ah, and AH stored back as the 0) */
	_AL = ((unsigned char *)&start)[3];
	_AH ^= _AH;
	type = _AX;
	((unsigned char *)&start)[3] = _AH;
	f->last = 0;
	if (count - 1 == index) {
		end = filesize;
		f->last = 1;
	} else {
		pos += 4;
		if (!DOS_read(handle, pos, 4L, MK_FP(_DS, &end)))
			goto fail;
		((unsigned char *)&end)[3] = 0;
	}
	filesize = end - start;
	if (!DOS_seek(handle, start, 0))
		goto fail;
	f->handle = handle;
	f->count = count;
	f->type = type;
	f->start = start;
	f->pos = 0;
	f->length = filesize;
	return 1;
fail:
	DOS_close(handle);
	return 0;
}

/* close the library entry */
/*C_2ACF_017C*/void pascal LIB_close(struct libfile *f)
{
	DOS_close(f->handle);
}
