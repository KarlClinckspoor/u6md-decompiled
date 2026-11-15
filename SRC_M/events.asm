;----------------------------------------
;MARTIAN.EXE segment 0F2C: events: the timer tick (int 8) handler and its callbacks,
;the keyboard (Escape, Alt-X, Ctrl-Alt-Del), the mouse, and a queue of input events
;(16 bytes each). A library module (Origin's, pascal). Savage Empire's SRC_S/events.asm
;(SAVAGE.EXE 07D9) with the addresses moved (+6 in the segment) and two changes: the
;timer is hooked with 10h for 0Ah, and keyDown sits at KEYB_TEXT:0124 (the literal
;offsets 124h, 1A3h, 140h, 15Bh). MARTIAN.EXE's other modules use only the Escape flag
;_escapePressed (title.c).
;(the relocations show that TASM wrote these fixups in another order than MASM:)
;TASM fixup order: 026D 0276 0251 0588 04FA 086C 089C 08C8 0825
;(offsets in the module's segment, as in SAVAGE.EXE: tools/md/tasmorder.py)
;----------------------------------------

_DATA	SEGMENT PUBLIC WORD 'DATA'
EXTRN _tmDrvCode:DWORD
EXTRN _vidMode:WORD	;the video mode
EXTRN _timerRate:WORD
EXTRN _superTmDrvCode:DWORD
EXTRN curMouse:WORD	;the mouse (struct tMouse *) (OSILIB/MOUSDATA.ASM)
EXTRN mouseState:BYTE	;a struct tMouse
EXTRN mouseDX:WORD	;how far the mouse moved: x
EXTRN mouseDY:WORD	;  y
EXTRN mouseMoved:BYTE	;1: the mouse moved
_DATA	ENDS

DGROUP	GROUP _DATA

_DATA	SEGMENT PUBLIC WORD 'DATA'
PUBLIC evtMouseX,evtMouseY,evtButtonL,evtButtonR,evtButtonM,mouseShowLevel,evtCount,evtHead,evtBuffer,evtBufferLast
D_18CA	db 8 dup(0)	;D_18CA (not referenced)
evtMouseX	dw 0	;D_18D2 the mouse at the last update (copied to mouseState): x
evtMouseY	dw 0	;D_18D4   y
evtButtonL	db 0	;D_18D6   buttons (each of the three bytes gets them all; left bit 0)
evtButtonR	db 0	;D_18D7   (right: bit 1)
evtButtonM	db 0	;D_18D8   (middle: bit 2)
mouseShowLevel	dw 0	;D_18D9 the pointer is shown when 0, hidden when below (-1 after the initialisation)
D_18DB	db 0	;D_18DB (copied with the mouse state, not otherwise used)
evtCount	dw 0	;D_18DC number of events in the queue
evtClock	dw 0	;D_18DE the time of day at the initialisation (seconds, long)
evtClockHi	dw 0	;D_18E0
evtTicks	dw 0	;D_18E2 timer ticks (long)
evtTicksHi	dw 0	;D_18E4
evtTicks2	dw 0	;D_18E6 timer ticks (long; counted, not read here)
evtTicks2Hi	dw 0	;D_18E8
keyRepeatDelay	dw 0	;D_18EA ticks before a held key repeats (never set: 0)
keyRepeatRate	dw 0	;D_18EC then ticks between two repeats (never set: 0)
dblClickTime	dw 0	;D_18EE a second click within this many ticks is a double click (long, never set: 0)
dblClickTimeHi	dw 0	;D_18F0
D_18F2	dw 0	;D_18F2 (not referenced)
D_18F4	dw 0	;D_18F4 (set to 0, not read)
keyRepeating	dw 0	;D_18F6 1: the held key has repeated
keyTime	dw 0	;D_18F8 evtTicks at the last key event (long)
keyTimeHi	dw 0	;D_18FA
clickTime	dw 0	;D_18FC evtTicks at the last click (long)
clickTimeHi	dw 0	;D_18FE
lastKey	dw 0FFFFh	;D_1900 the last key
keyEventDX	dw 0FFFFh	;D_1902 the key events' DX (always -1)
evtNoMouse	dw 0	;D_1904 1: no mouse (what the initialisation callback returned)
clickButton	dw 0	;D_1906 the last click's button (DX of a double click)
mouseBits	dw 0	;D_1908 the mouse buttons for the events' state bits (bits 1-3; bit 0: any)
lastButtons	dw 0	;D_190A the last mouse buttons
evtStateBits	dw 0	;D_190C the state bits of the last event (its +0Ah)
evtHead	dw 0	;D_190E the queue: first event
evtTail	dw 0	;D_1910 where the next event goes
evtBuffer	dw 0	;D_1912 start of the buffer
evtBufferLast	dw 0	;D_1914 its last event
_DATA	ENDS

EXTRN _malloc:FAR
EXTRN _free:FAR
EXTRN _exit:FAR
EXTRN SHP_height:FAR
EXTRN SHP_maxSize:FAR
EXTRN EVT_get:FAR
EXTRN MOUSE_init:FAR
EXTRN MOUSE_show:FAR
EXTRN MOUSE_hide:FAR
EXTRN _MEM_alloc:FAR
EXTRN _MEM_free:FAR
EXTRN _KeyboardInstall:FAR
EXTRN _KeyboardRemove:FAR
EXTRN _KeyboardSetCallback:FAR
EXTRN _HookVector:FAR
EXTRN _UnhookAll:FAR
EXTRN _SetTimerRate:FAR
EXTRN keyDown:BYTE	;the keyboard: a byte per scan code, 1 while the key is down

EVENTS_TEXT	SEGMENT PUBLIC WORD 'CODE'
	ASSUME CS:EVENTS_TEXT,DS:DGROUP

PUBLIC _escapePressed,EvtCheckAltX,EvtGetMousePos
;(the far pointers start as DGROUP:0000: their segment word is relocated)
altXCallback	dw 0	;0F2C:0008 called by EvtCheckAltX after Alt-X
altXCallbackSeg	dw DGROUP
timerPreHook	dw 0	;0F2C:000C called (as an interrupt) first at every tick: EvtIret, nothing sets another
timerPreHookSeg	dw DGROUP
timerChain	dw 0	;0F2C:0010 the previous timer handler (HookVector keeps it here): jumped to at the end
timerChainSeg	dw DGROUP
cv_0014	dw 0	;0F2C:0014 (not used)
cv_0016	dw DGROUP
tickCallback	dw 0	;0F2C:0018 called at every tick
tickCallbackSeg	dw DGROUP
periodicCallback	dw 0	;0F2C:001C called every periodicInterval+1 ticks
periodicCallbackSeg	dw DGROUP
evtDS	dw 0	;0F2C:0020 DGROUP
tickCountdown	dw 0	;0F2C:0022 ticks to the next callback
tickDivisor	dw 0	;0F2C:0024 ticks between two callbacks
altXPressed	db 0	;0F2C:0026 1: Alt-X was pressed
_escapePressed	db 0	;0F2C:0027 nonzero when Escape (or, as the title sets it, any key) was pressed
inTimer	db 0	;0F2C:0028 nonzero while the timer handler runs
evtLocked	db 0	;0F2C:0029 1: do not queue events
periodicCountdown	dw 0	;0F2C:002A ticks to the next periodicCallback
periodicInterval	dw 0	;0F2C:002C (5)
evtInstalled	dw 0	;0F2C:002E 1: installed

;the default callback: does nothing
	PUBLIC	EvtNoCallback
EvtNoCallback	PROC FAR	;0F2C:0030
	RET	;0030
EvtNoCallback	ENDP

;(near) evtClock = the time of day in seconds (int 21h function 2Ch)
EvtReadClock	PROC NEAR	;0F2C:0031
	MOV	AX,2C21h	;0031
	INT	21h	;0034
	XOR	AH,AH	;0036
	MOV	AL,DH	;0038
	MOV	BX,3Ch	;003A
	MUL	BX	;003D
	MOV	evtClock,AX	;003F
	MOV	evtClockHi,DX	;0042
	XOR	AH,AH	;0046
	MOV	AL,CL	;0048
	MOV	BX,0E10h	;004A
	MUL	BX	;004D
	ADD	evtClock,AX	;004F
	ADC	evtClockHi,DX	;0053
	MOV	AX,0E10h	;0057
	MOV	CL,CH	;005A
	XOR	CH,CH	;005C
	XOR	DX,DX	;005E
	JCXZ	L_006C	;0060
L_0062:
	ADD	evtClock,AX	;0062
	ADC	evtClockHi,DX	;0066
	LOOP	L_0062	;006A
L_006C:
	RET	;006C
EvtReadClock	ENDP

;installs: (pascal) EvtInstall(int nEvents, void far *init, int n): calls init (2: give up),
;allocates a queue of nEvents events, installs the timer handler (int 8) and the keyboard
;handler; returns 1
EvtInstall	PROC FAR	;0F2C:006D
	PUSH	BP	;006D
	MOV	BP,SP	;006E
	PUSH	DS	;0070
	PUSH	SI	;0071
	PUSH	DI	;0072
	PUSH	ES	;0073
	PUSHF	;0074
	PUSH	CS	;0075
	CALL	NEAR PTR EvtUninstall	;0076
	CALL	EvtReadClock	;0079
	LES	DI,[BP+8]	;007C
	MOV	AX,ES	;007F
	OR	AX,DI	;0081
	MOV	AX,1	;0083
	JZ	L_00A7	;0086
	MOV	CS:tickCallback,DI	;0088
	MOV	AX,ES	;008D
	MOV	CS:tickCallbackSeg,AX	;008F
	CALL	DWORD PTR CS:tickCallback	;0093
	CMP	AX,2	;0098
	JNZ	L_00A7	;009B
	MOV	WORD PTR CS:evtInstalled,0	;009D
	JMP	L_01A4	;00A4
L_00A7:
	MOV	evtNoMouse,AX	;00A7
	MOV	AX,DS	;00AA
	MOV	CS:evtDS,AX	;00AC
	MOV	AX,[BP+0Ch]	;00B0
	MOV	DX,10h	;00B3
	MUL	DX	;00B6
	PUSH	AX	;00B8
	CALL	_malloc	;00B9
	POP	DX	;00BE
	CMP	AX,0	;00BF
	JNZ	L_00CE	;00C2
	MOV	WORD PTR CS:evtInstalled,0	;00C4
	JMP	L_01A4	;00CB
L_00CE:
	MOV	evtBuffer,AX	;00CE
	MOV	evtHead,AX	;00D1
	MOV	evtTail,AX	;00D4
	SUB	DX,10h	;00D7
	ADD	AX,DX	;00DA
	MOV	evtBufferLast,AX	;00DC
	XOR	AX,AX	;00DF
	MOV	evtTicks,AX	;00E1
	MOV	evtTicksHi,AX	;00E4
	MOV	clickTime,AX	;00E7
	MOV	clickTimeHi,AX	;00EA
	MOV	evtTicks2,AX	;00ED
	MOV	evtTicks2Hi,AX	;00F0
	MOV	evtCount,AX	;00F3
	MOV	evtStateBits,AX	;00F6
	MOV	lastButtons,AX	;00F9
	MOV	D_18F4,AX	;00FC
	MOV	CS:periodicCountdown,AX	;00FF
	MOV	WORD PTR CS:periodicInterval,5	;0103
	MOV	BYTE PTR mouseState+19h,AL	;010A
	MOV	WORD PTR CS:periodicCallback,OFFSET EvtNoCallback	;010D
	MOV	WORD PTR CS:periodicCallbackSeg,EVENTS_TEXT	;0114
	MOV	WORD PTR CS:tickCallback,OFFSET EvtNoCallback	;011B
	MOV	WORD PTR CS:tickCallbackSeg,EVENTS_TEXT	;0122
	MOV	WORD PTR CS:timerPreHook,OFFSET EvtIret	;0129
	MOV	WORD PTR CS:timerPreHookSeg,EVENTS_TEXT	;0130
	INC	AX	;0137
	MOV	CS:tickCountdown,AX	;0138
	MOV	CS:tickDivisor,AX	;013C
	SUB	AX,2	;0140
	MOV	lastKey,AX	;0143
	MOV	keyEventDX,AX	;0146
	MOV	mouseShowLevel,AX	;0149
	CMP	WORD PTR evtNoMouse,1	;014C
	JZ	L_0162	;0151
	PUSH	WORD PTR mouseState+0Bh	;0153
	PUSH	WORD PTR mouseState+9	;0157
	PUSH	WORD PTR [BP+6]	;015B
	PUSH	CS	;015E
	CALL	NEAR PTR EvtMouseInit	;015F
L_0162:
	MOV	AX,1	;0162
	PUSH	AX	;0165
	PUSH	CS	;0166
	CALL	NEAR PTR EvtSetRate	;0167
	PUSH	CS	;016A
	MOV	AX,10h	;016B
	PUSH	AX	;016E
	XOR	AX,AX	;016F
	PUSH	AX	;0171
	MOV	AX,EVENTS_TEXT	;0172
	PUSH	AX	;0175
	MOV	AX,OFFSET EvtTimerHandler	;0176
	PUSH	AX	;0179
	MOV	AX,8	;017A
	PUSH	AX	;017D
	CLI	;017E
	CALL	_HookVector	;017F
	STI	;0184
	ADD	SP,0Ch	;0185
	CALL	_KeyboardInstall	;0188
	MOV	AX,EVENTS_TEXT	;018D
	PUSH	AX	;0190
	MOV	AX,OFFSET EvtKeyCallback	;0191
	PUSH	AX	;0194
	CALL	_KeyboardSetCallback	;0195
	ADD	SP,4	;019A
	MOV	AX,1	;019D
	MOV	CS:evtInstalled,AX	;01A0
L_01A4:
	POPF	;01A4
	POP	ES	;01A5
	POP	DI	;01A6
	POP	SI	;01A7
	POP	DS	;01A8
	POP	BP	;01A9
	RET	8	;01AA
EvtInstall	ENDP

;uninstalls: frees the queue, restores the timer and keyboard handlers
EvtUninstall	PROC FAR	;0F2C:01AD
	CMP	WORD PTR CS:evtInstalled,1	;01AD
	JNZ	L_01D0	;01B3
	MOV	AX,evtBuffer	;01B5
	PUSH	AX	;01B8
	CALL	_free	;01B9
	POP	AX	;01BE
	CALL	_UnhookAll	;01BF
	CALL	_KeyboardRemove	;01C4
	MOV	WORD PTR CS:evtInstalled,0	;01C9
L_01D0:
	RET	;01D0
EvtUninstall	ENDP

;(pascal) sets the rate of the timer callback (from _timerRate when it is set)
EvtSetRate	PROC FAR	;0F2C:01D1
	PUSH	BP	;01D1
	MOV	BP,SP	;01D2
	CMP	WORD PTR _timerRate,0	;01D4
	JZ	L_01F1	;01D9
	MOV	AX,_timerRate	;01DB
	MOV	BX,3Ch	;01DE
	XOR	DX,DX	;01E1
	DIV	BX	;01E3
	CMP	AX,0	;01E5
	JNZ	L_01EB	;01E8
	INC	AX	;01EA
L_01EB:
	MOV	CS:tickDivisor,AX	;01EB
	JMP	SHORT L_020B	;01EF
L_01F1:
	MOV	AX,[BP+6]	;01F1
	MOV	CS:tickDivisor,AX	;01F4
	MOV	BX,3Ch	;01F8
	MUL	BX	;01FB
	PUSH	AX	;01FD
	CALL	_SetTimerRate	;01FE
	POP	AX	;0203
	MOV	AX,1	;0204
	MOV	CS:tickCountdown,AX	;0207
L_020B:
	POP	BP	;020B
	RET	2	;020C
EvtSetRate	ENDP

;(pascal) EvtMouseInit(void far *p, int n): allocates the mouse (from p) and initialises it
EvtMouseInit	PROC FAR	;0F2C:020F
	PUSH	BP	;020F
	MOV	BP,SP	;0210
	MOV	AX,[BP+8]	;0212
	MOV	WORD PTR mouseState+9,AX	;0215
	MOV	AX,[BP+0Ah]	;0218
	MOV	WORD PTR mouseState+0Bh,AX	;021B
	PUSH	WORD PTR [BP+0Ah]	;021E
	PUSH	WORD PTR [BP+8]	;0221
	CALL	SHP_maxSize	;0224
	CMP	WORD PTR _vidMode,0Dh	;0229
	JNZ	L_024A	;022E
	PUSH	DX	;0230
	PUSH	AX	;0231
	PUSH	WORD PTR [BP+0Ah]	;0232
	PUSH	WORD PTR [BP+8]	;0235
	CALL	SHP_height	;0238
	SHL	AX,1	;023D
	SHL	AX,1	;023F
	MOV	CX,AX	;0241
	POP	AX	;0243
	POP	DX	;0244
	ADD	AX,CX	;0245
	ADC	DX,0	;0247
L_024A:
	PUSH	AX	;024A
	PUSH	BP	;024B
	MOV	BP,SP	;024C
	MOV	WORD PTR [BP+2],10h	;024E
	POP	BP	;0253
	PUSH	DX	;0254
	PUSH	AX	;0255
	CALL	_MEM_alloc	;0256
	ADD	SP,6	;025B
	MOV	BX,AX	;025E
	OR	BX,DX	;0260
	JZ	L_0277	;0262
	MOV	WORD PTR mouseState+0Fh,AX	;0264
	MOV	WORD PTR mouseState+11h,DX	;0267
	MOV	AX,OFFSET DGROUP:mouseState	;026B
	PUSH	AX	;026E
	PUSH	WORD PTR [BP+6]	;026F
	CALL	MOUSE_init	;0272
L_0277:
	POP	BP	;0277
	RET	6	;0278
EvtMouseInit	ENDP

;frees the mouse pointer shapes, the mouse off
EvtMouseFree	PROC FAR	;0F2C:027B
	CALL	MOUSE_hide	;027B
	PUSH	WORD PTR mouseState+11h	;0280
	PUSH	WORD PTR mouseState+0Fh	;0284
	CALL	_MEM_free	;0288
	ADD	SP,4	;028D
	MOV	BX,curMouse	;0290
	MOV	BYTE PTR [BX+19h],0	;0294
	RET	;0298
EvtMouseFree	ENDP

inQueue	db 0	;0F2C:0299 nonzero while EvtQueue runs

;queues an event: AX type, CX, DX (16 bytes: type, cx, dx, ticks (long), buttons, mouse x, y)
EvtQueue	PROC FAR	;0F2C:029A
	PUSHF	;029A
	PUSH	AX	;029B
	PUSH	BX	;029C
	PUSH	CX	;029D
	PUSH	DX	;029E
	PUSH	SI	;029F
	PUSH	DS	;02A0
	CMP	BYTE PTR CS:evtLocked,1	;02A1
	JNZ	L_02B1	;02A7
	CMP	AX,1	;02A9
	JNZ	L_02B1	;02AC
	JMP	L_03CC	;02AE
L_02B1:
	CMP	BYTE PTR CS:inQueue,1	;02B1
	JNZ	L_02BC	;02B7
	JMP	L_03CC	;02B9
L_02BC:
	INC	BYTE PTR CS:inQueue	;02BC
	MOV	BX,CS:evtDS	;02C1
	MOV	DS,BX	;02C6
	CMP	AX,3	;02C8
	JNL	L_02F6	;02CB
	db	083h,026h	;AND	WORD PTR evtStateBits,-20h ;02D3
	dw	OFFSET DGROUP:evtStateBits
	db	0E0h
	db	083h,026h	;AND	WORD PTR mouseBits,0Fh ;02D8
	dw	OFFSET DGROUP:mouseBits
	db	00Fh
	CMP	AX,2	;02D7
	JNZ	L_02E6	;02DA
	SHL	CX,1	;02DC
	OR	mouseBits,CX	;02DE
	SHR	CX,1	;02E2
	JMP	SHORT L_0316	;02E4
L_02E6:
	SHL	CX,1	;02E6
	db	083h,0F1h,0FFh	;XOR	CX,-1 ;02EE
	AND	mouseBits,CX	;02EB
	db	083h,0F1h,0FFh	;XOR	CX,-1 ;02F5
	SHR	CX,1	;02F2
	JMP	SHORT L_0316	;02F4
L_02F6:
	CMP	AX,9	;02F6
	JNL	L_0307	;02F9
	db	083h,026h	;AND	WORD PTR mouseBits,-20h ;0301
	dw	OFFSET DGROUP:mouseBits
	db	0E0h
	db	083h,026h	;AND	WORD PTR evtStateBits,0Fh ;0306
	dw	OFFSET DGROUP:evtStateBits
	db	00Fh
	JMP	SHORT L_0316	;0305
L_0307:
	CMP	AX,0Dh	;0307
	JNZ	L_0316	;030A
	db	083h,026h	;AND	WORD PTR evtStateBits,-20h ;0312
	dw	OFFSET DGROUP:evtStateBits
	db	0E0h
	db	083h,026h	;AND	WORD PTR mouseBits,0Fh ;0317
	dw	OFFSET DGROUP:mouseBits
	db	00Fh
L_0316:
	MOV	BX,AX	;0316
	MOV	SI,evtTail	;0318
	MOV	[SI],AX	;031C
	MOV	AX,evtTicks	;031E
	MOV	[SI+6],AX	;0321
	MOV	AX,evtTicksHi	;0324
	MOV	[SI+8],AX	;0327
	MOV	AX,mouseBits	;032A
	OR	AX,evtStateBits	;032D
	MOV	evtStateBits,AX	;0331
	MOV	[SI+0Ah],AX	;0334
	MOV	[SI+2],CX	;0337
	MOV	[SI+4],DX	;033A
	MOV	AX,evtMouseX	;033D
	MOV	[SI+0Ch],AX	;0340
	MOV	AX,evtMouseY	;0343
	MOV	[SI+0Eh],AX	;0346
	CMP	BX,2	;0349
	JNZ	L_038C	;034C
	CMP	DX,3	;034E
	JNZ	L_0359	;0351
	db	083h,04Ch,00Ah,010h	;OR	WORD PTR [SI+0Ah],10h ;0359
	JMP	SHORT L_037B	;0357
L_0359:
	MOV	AX,[SI+6]	;0359
	MOV	DX,[SI+8]	;035C
	SUB	AX,clickTime	;035F
	SBB	DX,clickTimeHi	;0363
	SUB	AX,dblClickTime	;0367
	SBB	DX,dblClickTimeHi	;036B
	JNC	L_037B	;036F
	db	083h,04Ch,00Ah,010h	;OR	WORD PTR [SI+0Ah],10h ;0377
	MOV	AX,clickButton	;0375
	MOV	[SI+4],AX	;0378
L_037B:
	MOV	AX,[SI+6]	;037B
	MOV	DX,[SI+8]	;037E
	MOV	clickTime,AX	;0381
	MOV	clickTimeHi,DX	;0384
	MOV	clickButton,CX	;0388
L_038C:
	MOV	AX,evtTail	;038C
	CMP	WORD PTR evtCount,0	;038F
	JZ	L_03B4	;0394
	CMP	AX,evtHead	;0396
	JNZ	L_03B4	;039A
	ADD	AX,10h	;039C
	CMP	AX,evtBufferLast	;039F
	JNA	L_03A8	;03A3
	MOV	AX,evtBuffer	;03A5
L_03A8:
	MOV	evtHead,AX	;03A8
	MOV	evtTail,AX	;03AB
	DEC	WORD PTR evtCount	;03AE
	JMP	SHORT L_03C3	;03B2
L_03B4:
	ADD	AX,10h	;03B4
	CMP	AX,evtBufferLast	;03B7
	JNA	L_03C0	;03BB
	MOV	AX,evtBuffer	;03BD
L_03C0:
	MOV	evtTail,AX	;03C0
L_03C3:
	INC	WORD PTR evtCount	;03C3
	DEC	BYTE PTR CS:inQueue	;03C7
L_03CC:
	POP	DS	;03CC
	POP	SI	;03CD
	POP	DX	;03CE
	POP	CX	;03CF
	POP	BX	;03D0
	POP	AX	;03D1
	POPF	;03D2
	RET	;03D3
EvtQueue	ENDP

;calls EvtDebugStub twice (AX 8888h, 0F0Fh)
EvtDebugMark	PROC FAR	;0F2C:03D4
	PUSH	AX	;03D4
	MOV	AX,8888h	;03D5
	PUSH	CS	;03D8
	CALL	NEAR PTR EvtDebugStub	;03D9
	MOV	AX,0F0Fh	;03DC
	PUSH	CS	;03DF
	CALL	NEAR PTR EvtDebugStub	;03E0
	POP	AX	;03E3
	RET	;03E4
EvtDebugMark	ENDP

;reads the mouse (int 33h function 3), queues button changes and motion; returns 1 when it changed
EvtPollMouse	PROC FAR	;0F2C:03E5
	MOV	BYTE PTR mouseMoved,0	;03E5
	MOV	BX,curMouse	;03EA
	CMP	BYTE PTR [BX+19h],0	;03EE
	JNZ	L_03F7	;03F2
	JMP	L_0507	;03F4
L_03F7:
	CMP	BYTE PTR [BX+19h],3	;03F7
	JNG	L_0400	;03FB
	JMP	L_0507	;03FD
L_0400:
	MOV	AX,3	;0400
	INT	33h	;0403
	SHR	CX,1	;0405
	db	083h,026h	;AND	WORD PTR mouseBits,-10h ;040D
	dw	OFFSET DGROUP:mouseBits
	db	0F0h
	MOV	AX,BX	;040C
	SHL	AL,1	;040E
	CMP	AL,0	;0410
	JZ	L_0416	;0412
	OR	AL,1	;0414
L_0416:
	OR	mouseBits,AX	;0416
	CMP	BYTE PTR CS:evtLocked,1	;041A
	JNZ	L_0428	;0420
	MOV	AX,BX	;0422
	PUSH	DX	;0424
	PUSH	CX	;0425
	JMP	SHORT L_043B	;0426
L_0428:
	MOV	AX,lastButtons	;0428
	MOV	lastButtons,BX	;042B
	XOR	AX,BX	;042F
	JZ	L_0486	;0431
	MOV	evtMouseX,CX	;0433
	MOV	evtMouseY,DX	;0437
L_043B:
	XOR	DX,DX	;043B
	MOV	CX,1	;043D
	SHR	AL,1	;0440
	JNC	L_0452	;0442
	PUSH	AX	;0444
	MOV	AL,BL	;0445
	AND	AL,1	;0447
	SHR	AL,1	;0449
	ADC	AL,1	;044B
	PUSH	CS	;044D
	CALL	NEAR PTR EvtQueue	;044E
	POP	AX	;0451
L_0452:
	INC	CX	;0452
	SHR	AL,1	;0453
	JNC	L_0467	;0455
	PUSH	AX	;0457
	MOV	AL,BL	;0458
	SHR	AL,1	;045A
	AND	AL,1	;045C
	SHR	AL,1	;045E
	ADC	AL,1	;0460
	PUSH	CS	;0462
	CALL	NEAR PTR EvtQueue	;0463
	POP	AX	;0466
L_0467:
	INC	CX	;0467
	SHR	AL,1	;0468
	JNC	L_047C	;046A
	MOV	AL,BL	;046C
	SHR	AL,1	;046E
	SHR	AL,1	;0470
	AND	AL,1	;0472
	SHR	AL,1	;0474
	ADC	AL,1	;0476
	PUSH	CS	;0478
	CALL	NEAR PTR EvtQueue	;0479
L_047C:
	CMP	BYTE PTR CS:evtLocked,1	;047C
	JNZ	L_04B3	;0482
	POP	CX	;0484
	POP	DX	;0485
L_0486:
	XOR	AX,AX	;0486
	CMP	evtMouseX,CX	;0488
	JNZ	L_0494	;048C
	CMP	evtMouseY,DX	;048E
	JZ	L_0507	;0492
L_0494:
	MOV	AX,CX	;0494
	SUB	AX,evtMouseX	;0496
	MOV	mouseDX,AX	;049A
	MOV	AX,DX	;049D
	SUB	AX,evtMouseY	;049F
	MOV	mouseDY,AX	;04A3
	MOV	BYTE PTR mouseMoved,1	;04A6
	MOV	evtMouseX,CX	;04AB
	MOV	evtMouseY,DX	;04AF
L_04B3:
	XOR	AX,AX	;04B3
	MOV	evtButtonL,BL	;04B5
	MOV	evtButtonR,BL	;04B9
	MOV	evtButtonM,BL	;04BD
	CMP	WORD PTR mouseShowLevel,-1	;04C1
	JNG	L_0507	;04C6
	PUSH	ES	;04C8
	PUSH	DI	;04C9
	LES	DI,_tmDrvCode	;04CA
	MOV	AX,ES:[DI]	;04CE
	POP	DI	;04D1
	POP	ES	;04D2
	OR	AX,AX	;04D3
	JNZ	L_0504	;04D5
	PUSH	ES	;04D7
	PUSH	DI	;04D8
	LES	DI,_superTmDrvCode	;04D9
	MOV	AX,ES:[DI]	;04DD
	POP	DI	;04E0
	POP	ES	;04E1
	OR	AX,AX	;04E2
	JNZ	L_0504	;04E4
	CALL	MOUSE_hide	;04E6
	PUSH	DI	;04EB
	PUSH	SI	;04EC
	MOV	AX,DS	;04ED
	MOV	ES,AX	;04EF
	MOV	DI,OFFSET DGROUP:mouseState	;04F1
	MOV	SI,OFFSET DGROUP:evtMouseX	;04F4
	MOV	CX,9	;04F7
	CLD	;04FA
	REP	movsb	;04FB
	POP	SI	;04FD
	POP	DI	;04FE
	CALL	MOUSE_show	;04FF
L_0504:
	MOV	AX,1	;0504
L_0507:
	RET	;0507
EvtPollMouse	ENDP

;the timer (int 8) handler: calls the previous one, counts the ticks, calls the callbacks
EvtTimerHandler	PROC FAR	;0F2C:0508
	PUSHF	;0508
	PUSHF	;0509
	CALL	DWORD PTR CS:timerPreHook	;050A
	CMP	BYTE PTR CS:inTimer,0	;050F
	JNZ	L_0587	;0515
	DEC	WORD PTR CS:tickCountdown	;0517
	JNZ	L_0587	;051C
	INC	BYTE PTR CS:inTimer	;051E
	PUSH	DS	;0523
	PUSH	AX	;0524
	PUSH	BX	;0525
	PUSH	CX	;0526
	PUSH	DX	;0527
	PUSH	SI	;0528
	PUSH	DI	;0529
	PUSH	ES	;052A
	MOV	AX,CS:tickDivisor	;052B
	MOV	CS:tickCountdown,AX	;052F
	MOV	AX,CS:evtDS	;0533
	MOV	DS,AX	;0537
	CMP	BYTE PTR CS:evtLocked,1	;0539
	JZ	L_0562	;053F
	PUSH	CS	;0541
	CALL	NEAR PTR EvtPollMouse	;0542
	CALL	DWORD PTR CS:tickCallback	;0545
	DEC	WORD PTR CS:periodicCountdown	;054A
	JNS	L_0562	;054F
	MOV	AX,CS:periodicInterval	;0551
	MOV	CS:periodicCountdown,AX	;0555
	OR	AX,AX	;0559
	JZ	L_0562	;055B
	CALL	DWORD PTR CS:periodicCallback	;055D
L_0562:
	MOV	AX,1	;0562
	ADD	evtTicks,AX	;0565
	ADC	WORD PTR evtTicksHi,0	;0569
	MOV	AX,1	;056E
	ADD	evtTicks2,AX	;0571
	ADC	WORD PTR evtTicks2Hi,0	;0575
	POP	ES	;057A
	POP	DI	;057B
	POP	SI	;057C
	POP	DX	;057D
	POP	CX	;057E
	POP	BX	;057F
	POP	AX	;0580
	POP	DS	;0581
	DEC	BYTE PTR CS:inTimer	;0582
L_0587:
	POPF	;0587
	JMP	DWORD PTR CS:timerChain	;0588
EvtTimerHandler	ENDP

;iret: the "previous" timer handler until one is installed
EvtIret	PROC FAR	;0F2C:058D
	IRET	;058D
EvtIret	ENDP

rebootVector	dw 0	;0F2C:058E
rebootVectorSeg	dw DGROUP

;the keyboard callback: Alt-X sets altXPressed, Ctrl-Alt-Del reboots, Escape sets _escapePressed
EvtCheckKeys	PROC FAR	;0F2C:0592
	PUSH	ES	;0592
	PUSH	DI	;0593
	PUSH	AX	;0594
	MOV	AX,SEG keyDown	;0595
	MOV	ES,AX	;0598
	MOV	DI,124h	;059A
	CMP	BYTE PTR ES:[DI+38h],1	;059D
	JNZ	L_05D1	;05A2
	CMP	BYTE PTR ES:[DI+2Dh],1	;05A4
	JNZ	L_05B3	;05A9
	MOV	BYTE PTR CS:altXPressed,1	;05AB
	JMP	SHORT L_05DE	;05B1
L_05B3:
	CMP	BYTE PTR ES:[DI+53h],1	;05B3
	JNZ	L_05D1	;05B8
	CMP	BYTE PTR ES:[DI+1Dh],1	;05BA
	JNZ	L_05D1	;05BF
	XOR	AX,AX	;05C1
	MOV	CS:rebootVector,AX	;05C3
	DEC	AX	;05C7
	MOV	CS:rebootVectorSeg,AX	;05C8
	JMP	DWORD PTR CS:rebootVector	;05CC
L_05D1:
	CMP	BYTE PTR ES:[DI+1],1	;05D1
	JNZ	L_05DE	;05D6
	MOV	BYTE PTR CS:_escapePressed,1	;05D8
L_05DE:
	POP	AX	;05DE
	POP	DI	;05DF
	POP	ES	;05E0
	RET	;05E1
EvtCheckKeys	ENDP

;does nothing
EvtDebugStub	PROC FAR	;0F2C:05E2
	RET	;05E2
EvtDebugStub	ENDP

;fills the first 640 words of A000h with AX
EvtFillScreen	PROC FAR	;0F2C:05E3
	PUSHF	;05E3
	PUSH	ES	;05E4
	PUSH	DI	;05E5
	PUSH	BX	;05E6
	PUSH	CX	;05E7
	CLD	;05E8
	XOR	DI,DI	;05E9
	MOV	BX,0A000h	;05EB
	MOV	ES,BX	;05EE
	MOV	CX,280h	;05F0
	REP	stosw	;05F3
	POP	CX	;05F5
	POP	BX	;05F6
	POP	DI	;05F7
	POP	ES	;05F8
	POPF	;05F9
	RET	;05FA
EvtFillScreen	ENDP

;the key callback (AL = scan code): queues key events (and repeats of a held key)
EvtKeyCallback	PROC FAR	;0F2C:05FB
	PUSH	DS	;05FB
	PUSH	AX	;05FC
	PUSH	BX	;05FD
	PUSH	CX	;05FE
	PUSH	DX	;05FF
	MOV	DX,CS:evtDS	;0600
	MOV	DS,DX	;0605
	CMP	BYTE PTR CS:evtLocked,1	;0607
	JNZ	L_0612	;060D
	JMP	L_06A8	;060F
L_0612:
	XOR	AH,AH	;0612
	CMP	AX,lastKey	;0614
	JNZ	L_0665	;0618
	TEST	AX,80h	;061A
	JZ	L_0622	;061D
	JMP	L_06A8	;061F
L_0622:
	MOV	CX,evtTicks	;0622
	MOV	DX,evtTicksHi	;0626
	SUB	CX,keyTime	;062A
	SBB	DX,keyTimeHi	;062E
	CMP	WORD PTR keyRepeating,0	;0632
	JZ	L_063F	;0637
	SUB	CX,keyRepeatRate	;0639
	JMP	SHORT L_0643	;063D
L_063F:
	SUB	CX,keyRepeatDelay	;063F
L_0643:
	JC	L_06A8	;0643
	CMP	WORD PTR keyRepeating,1	;0645
	JZ	L_0650	;064A
	INC	WORD PTR keyRepeating	;064C
L_0650:
	MOV	CX,evtTicks	;0650
	MOV	DX,evtTicksHi	;0654
	MOV	keyTime,CX	;0658
	MOV	keyTimeHi,DX	;065C
	MOV	AX,5	;0660
	JMP	SHORT L_068C	;0663
L_0665:
	MOV	lastKey,AX	;0665
	CMP	BYTE PTR CS:evtLocked,1	;0668
	JZ	L_06A8	;066E
	SHL	AL,1	;0670
	MOV	AL,3	;0672
	ADC	AL,AH	;0674
	MOV	WORD PTR keyRepeating,0	;0676
	MOV	CX,evtTicks	;067C
	MOV	DX,evtTicksHi	;0680
	MOV	keyTime,CX	;0684
	MOV	keyTimeHi,DX	;0688
L_068C:
	MOV	CX,lastKey	;068C
	MOV	DX,keyEventDX	;0690
	PUSH	AX	;0694
	MOV	AX,8888h	;0695
	PUSH	CS	;0698
	CALL	NEAR PTR EvtDebugStub	;0699
	MOV	AX,0F0Fh	;069C
	PUSH	CS	;069F
	CALL	NEAR PTR EvtDebugStub	;06A0
	POP	AX	;06A3
	PUSH	CS	;06A4
	CALL	NEAR PTR EvtQueue	;06A5
L_06A8:
	PUSH	CS	;06A8
	CALL	NEAR PTR EvtCheckKeys	;06A9
	POP	DX	;06AC
	POP	CX	;06AD
	POP	BX	;06AE
	POP	AX	;06AF
	POP	DS	;06B0
	RET	;06B1
EvtKeyCallback	ENDP

;empties the queue
EvtFlush	PROC FAR	;0F2C:06B2
	PUSHF	;06B2
	CLI	;06B3
	MOV	AX,evtBuffer	;06B4
	MOV	evtHead,AX	;06B7
	MOV	evtTail,AX	;06BA
	MOV	WORD PTR evtCount,0	;06BD
	MOV	AX,mouseBits	;06C3
	MOV	evtStateBits,AX	;06C6
	POPF	;06C9
	RET	;06CA
EvtFlush	ENDP

;queues an event for each key held down (but Ctrl and Alt)
EvtQueueHeldKeys	PROC FAR	;0F2C:06CB
	PUSH	SI	;06CB
	PUSH	DI	;06CC
	PUSHF	;06CD
	STD	;06CE
	MOV	AX,SEG keyDown	;06CF
	MOV	ES,AX	;06D2
	MOV	DI,1A3h	;06D4
	XOR	DX,DX	;06D7
	XOR	AX,AX	;06D9
	MOV	CX,7Fh	;06DB
L_06DE:
	SCASB	;06DE
	JZ	L_06F5	;06DF
	CMP	DI,140h	;06E1
	JZ	L_06F5	;06E5
	CMP	DI,15Bh	;06E7
	JZ	L_06F5	;06EB
	MOV	AL,3	;06ED
	PUSH	CS	;06EF
	CALL	NEAR PTR EvtQueue	;06F0
	XOR	AL,AL	;06F3
L_06F5:
	LOOP	L_06DE	;06F5
	POPF	;06F7
	POP	DI	;06F8
	POP	SI	;06F9
	RET	;06FA
EvtQueueHeldKeys	ENDP

;finds the first queued event of type AL: SI (AX 0: none)
EvtFind	PROC FAR	;0F2C:06FB
	PUSH	BX	;06FB
	MOV	BL,CS:evtLocked	;06FC
	MOV	BYTE PTR CS:evtLocked,1	;0701
	MOV	CX,evtCount	;0707
	JCXZ	L_0726	;070B
	MOV	SI,evtHead	;070D
L_0711:
	CMP	AL,[SI]	;0711
	JZ	L_0728	;0713
	CMP	SI,evtBufferLast	;0715
	JC	L_0721	;0719
	MOV	SI,evtBuffer	;071B
	JMP	SHORT L_0724	;071F
L_0721:
	ADD	SI,10h	;0721
L_0724:
	LOOP	L_0711	;0724
L_0726:
	XOR	AX,AX	;0726
L_0728:
	MOV	CS:evtLocked,BL	;0728
	POP	BX	;072D
	RET	;072E
EvtFind	ENDP

;(pascal) EvtPeek(event *e, int type): copies the first event of that type
EvtPeek	PROC FAR	;0F2C:072F
	PUSH	BP	;072F
	MOV	BP,SP	;0730
	PUSH	SI	;0732
	PUSH	DI	;0733
	PUSH	ES	;0734
	MOV	AX,[BP+6]	;0735
	PUSH	CS	;0738
	CALL	NEAR PTR EvtFind	;0739
	OR	AX,AX	;073C
	JZ	L_074A	;073E
	PUSH	DS	;0740
	POP	ES	;0741
	MOV	DI,[BP+8]	;0742
	MOV	CX,8	;0745
	REP	movsw	;0748
L_074A:
	POP	ES	;074A
	POP	DI	;074B
	POP	SI	;074C
	POP	BP	;074D
	RET	4	;074E
EvtPeek	ENDP

;(pascal) EvtTake(event *e, int type): takes the first event of that type out of the queue
EvtTake	PROC FAR	;0F2C:0751
	PUSH	BP	;0751
	MOV	BP,SP	;0752
	PUSH	SI	;0754
	PUSH	DI	;0755
	PUSH	ES	;0756
	MOV	AX,[BP+6]	;0757
	PUSH	CS	;075A
	CALL	NEAR PTR EvtFind	;075B
	OR	AX,AX	;075E
	JZ	L_0774	;0760
	MOV	WORD PTR [SI],0FFFFh	;0762
	PUSH	DS	;0766
	POP	ES	;0767
	MOV	DI,[BP+8]	;0768
	MOV	CX,8	;076B
	REP	movsw	;076E
	MOV	DI,[BP+8]	;0770
	STOSW	;0773
L_0774:
	POP	ES	;0774
	POP	DI	;0775
	POP	SI	;0776
	POP	BP	;0777
	RET	4	;0778
EvtTake	ENDP

;(pascal) removes all the events of a type
EvtDiscard	PROC FAR	;0F2C:077B
	PUSH	BP	;077B
	MOV	BP,SP	;077C
	PUSH	SI	;077E
	PUSH	DI	;077F
	PUSH	ES	;0780
L_0781:
	MOV	AX,[BP+6]	;0781
	PUSH	CS	;0784
	CALL	NEAR PTR EvtFind	;0785
	OR	AX,AX	;0788
	JZ	L_0792	;078A
	MOV	WORD PTR [SI],0FFFFh	;078C
	JMP	SHORT L_0781	;0790
L_0792:
	POP	ES	;0792
	POP	DI	;0793
	POP	SI	;0794
	POP	BP	;0795
	RET	2	;0796
EvtDiscard	ENDP

;(pascal) whether an event of a type is queued
EvtPending	PROC FAR	;0F2C:0799
	PUSH	BP	;0799
	MOV	BP,SP	;079A
	PUSH	DS	;079C
	PUSH	SI	;079D
	PUSH	DI	;079E
	MOV	AX,[BP+6]	;079F
	PUSH	CS	;07A2
	CALL	NEAR PTR EvtFind	;07A3
	POP	DI	;07A6
	POP	SI	;07A7
	POP	DS	;07A8
	POP	BP	;07A9
	RET	2	;07AA
EvtPending	ENDP

;after Alt-X: calls altXCallback, and when it returns 0, uninstalls and exits
EvtCheckAltX	PROC FAR	;0F2C:07AD
	CMP	BYTE PTR CS:altXPressed,0	;07AD
	JZ	L_07CD	;07B3
	CALL	DWORD PTR CS:altXCallback	;07B5
	OR	AX,AX	;07BA
	JNZ	L_07CD	;07BC
	CALL	_UnhookAll	;07BE
	CALL	_KeyboardRemove	;07C3
	CALL	_exit	;07C8
L_07CD:
	MOV	BYTE PTR CS:altXPressed,0	;07CD
	RET	;07D3
EvtCheckAltX	ENDP

;(pascal) EvtUpdate(int arg, int what): Alt-X check, empties the queue, queues
;keys held (1), the mouse (2), calls the callbacks (4, 8), then EVT_get(arg)
EvtUpdate	PROC FAR	;0F2C:07D4
	PUSH	BP	;07D4
	MOV	BP,SP	;07D5
	PUSH	SI	;07D7
	PUSH	DI	;07D8
	PUSH	DS	;07D9
	PUSH	CS	;07DA
	CALL	NEAR PTR EvtCheckAltX	;07DB
	PUSH	CS	;07DE
	CALL	NEAR PTR EvtFlush	;07DF
	TEST	WORD PTR [BP+6],1	;07E2
	JZ	L_07ED	;07E7
	PUSH	CS	;07E9
	CALL	NEAR PTR EvtQueueHeldKeys	;07EA
L_07ED:
	TEST	WORD PTR [BP+6],2	;07ED
	JZ	L_080E	;07F2
	PUSH	CS	;07F4
	CALL	NEAR PTR EvtPollMouse	;07F5
	CMP	BYTE PTR mouseMoved,1	;07F8
	JNZ	L_080E	;07FD
	MOV	AX,0Dh	;07FF
	MOV	CX,mouseDX	;0802
	MOV	DX,mouseDY	;0806
	PUSH	CS	;080A
	CALL	NEAR PTR EvtQueue	;080B
L_080E:
	TEST	WORD PTR [BP+6],4	;080E
	JZ	L_081A	;0813
	CALL	DWORD PTR CS:periodicCallback	;0815
L_081A:
	TEST	WORD PTR [BP+6],8	;081A
	JZ	L_0826	;081F
	CALL	DWORD PTR CS:tickCallback	;0821
L_0826:
	MOV	AX,[BP+8]	;0826
	PUSH	AX	;0829
	CALL	EVT_get	;082A
	POP	DS	;082F
	POP	DI	;0830
	POP	SI	;0831
	POP	BP	;0832
	RET	4	;0833
EvtUpdate	ENDP

;(pascal) puts the mouse at (x, y) (int 33h function 4)
EvtSetMousePos	PROC FAR	;0F2C:0836
	PUSH	BP	;0836
	MOV	BP,SP	;0837
	PUSH	DS	;0839
	MOV	AX,CS:evtDS	;083A
	MOV	DS,AX	;083E
	MOV	BX,curMouse	;0840
	CMP	BYTE PTR [BX+19h],0	;0844
	JZ	L_085A	;0848
	PUSHF	;084A
	CLI	;084B
	MOV	CX,[BP+8]	;084C
	SHL	CX,1	;084F
	MOV	DX,[BP+6]	;0851
	MOV	AX,4	;0854
	INT	33h	;0857
	POPF	;0859
L_085A:
	POP	DS	;085A
	POP	BP	;085B
	RET	4	;085C
EvtSetMousePos	ENDP

;(pascal) sets the tick callback tickCallback (0: none)
EvtSetTickCallback	PROC FAR	;0F2C:085F
	PUSH	BP	;085F
	MOV	BP,SP	;0860
	PUSH	SI	;0862
	PUSH	DI	;0863
	MOV	AX,[BP+6]	;0864
	OR	AX,[BP+8]	;0867
	JNZ	L_0876	;086A
	MOV	WORD PTR [BP+6],OFFSET EvtNoCallback	;086C
	MOV	WORD PTR [BP+8],EVENTS_TEXT	;0871
L_0876:
	PUSH	ES	;0876
	PUSH	DI	;0877
	LES	DI,[BP+6]	;0878
	CLI	;087B
	MOV	CS:tickCallback,DI	;087C
	MOV	CS:tickCallbackSeg,ES	;0881
	STI	;0886
	POP	DI	;0887
	POP	ES	;0888
	POP	DI	;0889
	POP	SI	;088A
	POP	BP	;088B
	RET	4	;088C
EvtSetTickCallback	ENDP

;(pascal) sets the Alt-X callback altXCallback (0: none)
EvtSetAltXCallback	PROC FAR	;0F2C:088F
	PUSH	BP	;088F
	MOV	BP,SP	;0890
	PUSH	SI	;0892
	PUSH	DI	;0893
	MOV	AX,[BP+6]	;0894
	OR	AX,[BP+8]	;0897
	JNZ	L_08A6	;089A
	MOV	WORD PTR [BP+6],OFFSET EvtNoCallback	;089C
	MOV	WORD PTR [BP+8],EVENTS_TEXT	;08A1
L_08A6:
	PUSH	ES	;08A6
	PUSH	DI	;08A7
	LES	DI,[BP+6]	;08A8
	MOV	CS:altXCallback,DI	;08AB
	MOV	CS:altXCallbackSeg,ES	;08B0
	POP	DI	;08B5
	POP	ES	;08B6
	POP	DI	;08B7
	POP	SI	;08B8
	POP	BP	;08B9
	RET	4	;08BA
EvtSetAltXCallback	ENDP

;(pascal) sets the periodic callback periodicCallback (0: none)
EvtSetPeriodicCallback	PROC FAR	;0F2C:08BD
	PUSH	BP	;08BD
	MOV	BP,SP	;08BE
	MOV	AX,[BP+6]	;08C0
	OR	AX,[BP+8]	;08C3
	JNZ	L_08D2	;08C6
	MOV	WORD PTR [BP+6],OFFSET EvtNoCallback	;08C8
	MOV	WORD PTR [BP+8],EVENTS_TEXT	;08CD
L_08D2:
	PUSH	ES	;08D2
	PUSH	DI	;08D3
	LES	DI,[BP+6]	;08D4
	CLI	;08D7
	MOV	CS:periodicCallback,DI	;08D8
	MOV	CS:periodicCallbackSeg,ES	;08DD
	STI	;08E2
	POP	DI	;08E3
	POP	ES	;08E4
	POP	BP	;08E5
	RET	4	;08E6
EvtSetPeriodicCallback	ENDP

;(pascal) EvtGetMousePos(int *xy): the mouse position (int 33h function 3)
EvtGetMousePos	PROC FAR	;0F2C:08E9
	PUSH	BP	;08E9
	MOV	BP,SP	;08EA
	PUSH	DS	;08EC
	MOV	AX,CS:evtDS	;08ED
	MOV	DS,AX	;08F1
	MOV	BX,curMouse	;08F3
	CMP	BYTE PTR [BX+19h],0	;08F7
	JZ	L_090F	;08FB
	PUSHF	;08FD
	CLI	;08FE
	MOV	AX,3	;08FF
	INT	33h	;0902
	POPF	;0904
	MOV	BX,[BP+6]	;0905
	SHR	CX,1	;0908
	MOV	[BX],CX	;090A
	MOV	[BX+2],DX	;090C
L_090F:
	POP	DS	;090F
	POP	BP	;0910
	RET	2	;0911
EvtGetMousePos	ENDP

EVENTS_TEXT	ENDS

END
