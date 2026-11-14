/*
 * Martian Dreams - GAME.EXE overlay 2F5B: what is left of the drive/floppy
 * module (Savage Empire's disk.c, SE GAME.EXE 2A81, from Ultima VI's).
 *
 * MD has no floppy handling: DISK_IsInDrive, DISK_ScanDrive and the body
 * of DISK_confirm are gone (an empty function without arguments sits in
 * DISK_confirm's place), and DISK_init only installs the null drive error
 * callback and reads the DOS version. DiskNames, unused, is still there;
 * CurrentDisk, CurrentDrive, DiskDrive, DriveType and DiskIdFile are not.
 * DISK_full() is called by the file error handler on a short write.
 */
#include "se.h"

extern void (*DriveErrorCallback)();/*DriveErrorCallback (LOW.ASM): called by OSI_selectDrive when selection fails*/

/*console*/
extern int16 CON_getch();/*CON_getch*/
extern cdecl CON_printf(char far *, ...);/*CON_printf*/

extern int16 D_0F18;/*MapAnimEnabled*/
extern unsigned char D_EC5A, D_EC5B;/*DosVerMajor, DosVerMinor*/
#define MapAnimEnabled D_0F18

static DISK_NullCallback();

/*floppy of each disk number (not used)*/
/*1DA4*/char *DiskNames[] = {
	/*1DB8*/"",
	/*1DB9*/"DISK 1",
	/*1DC0*/"DISK 2",
	/*1DC7*/"DISK 3",
	"DISK 2",
	"DISK 1",
	"DISK 1",
	"DISK 2",
	"DISK 3"
};
/*1DB6*/int16 DiskFullChoice = ' ';/*"disk full" answer: 'S'ave/'R'estore, ' ' none*/

/*default "drive selection failed" callback (DriveErrorCallback): do nothing*/
/*C_2F5B_0000*/static DISK_NullCallback() {
	return 0;
}

/*does nothing (where SE has DISK_confirm; not called)*/
/*C_2F5B_0007*/DISK_Unused() {
}

/*asks whether to save or restore; the answer is left in DiskFullChoice*/
/*C_2F5B_000C*/DISK_full() {
	int16 answer;

	MapAnimEnabled = 0;
	CON_printf("Disk full.  Do you want to Save the game and continue, ");
	CON_printf("or Restore your last saved game? ");
	do {
		answer = CON_getch();
	} while(answer != 'S' && answer != 'R');
	DiskFullChoice = answer;
	if(answer == 'S')
		CON_printf("S\n\nSaving...\n");
	else
		CON_printf("R\n\nRestoring...\n");
}

/*installs the null drive error callback, reads the DOS version*/
/*C_2F5B_005D*/DISK_init() {
	union REGS regs;

	DriveErrorCallback = DISK_NullCallback;
	/*get dos version*/
	regs.h.ah = 0x30;
	intdos(&regs, &regs);
	D_EC5A = regs.h.al;
	D_EC5B = regs.h.ah;
}
