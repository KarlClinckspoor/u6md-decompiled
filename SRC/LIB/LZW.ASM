;----------------------------------------
;LZW decompression: Savage Empire's LZW.ASM (GAME.EXE 2E5D), Origin's LZW
;(as U6's LZW.ASM: 9 to 12 bit codes, 256 clear, 257 end), decoding from a
;file through an input buffer to far memory. LZC sets it up: lzwFile the
;file, lzwIn the input buffer of lzwInSize bytes, lzwOut where the data goes,
;lzwDictSeg the segment of the dictionary (3 bytes per code).
;Martian Dreams' version keeps a state in its code segment (lzwStateCS) so
;that LZW_decompress can go on where it stopped, and holds a second code
;reader (lzwReadCodeR, not called) that leaves LZW_decompress itself when
;the input runs out with lzwNoRefill set.
;----------------------------------------

DGROUP	GROUP _DATA

_DATA	SEGMENT WORD PUBLIC 'DATA'
PUBLIC _lzwNoRefill,_lzwFile,_lzwDictSeg,_lzwOut,_lzwIn,_lzwInSize,_lzwBitPos
_lzwNoRefill	dw 0		;D_2E76 nonzero: stop when the input buffer runs out
lzwStackCount	dw 0		;D_2E78 stack count: the bytes of a string, pushed backwards
lzwCodeBits	dw 0		;D_2E7A bits per code (9-12)
lzwCodeLimit	dw 0		;D_2E7C the first code that needs one more bit
_lzwFile	dw 0		;D_2E7E handle of the file
lzwNextCode	dw 0		;D_2E80 the next free code
_lzwDictSeg	dw 0		;D_2E82 segment of the dictionary
lzwCode	dw 0		;D_2E84 the code just read
lzwFirstByte	db 0		;D_2E86 the first byte of the last string
lzwPrevCode	dw 0		;D_2E87 the previous code
_lzwOut	dw 0,DGROUP	;D_2E89 where the next byte goes (far pointer)
_lzwIn	dw 0,DGROUP	;D_2E8D input: the byte of the next code (far pointer)
_lzwInSize	dw 0		;D_2E91 length of the input buffer
lzwInStart	dw 0		;D_2E93 offset of the input buffer
_lzwBitPos	dw 0		;D_2E95 bit offset of the next code in its byte
lzwNextBitPos	dw 0		;D_2E97 lzwBitPos after the code being read (lzwReadCodeR)
lzwMasks	dw 1FFh,3FFh,7FFh,0FFFh	;D_2E99 the masks of 9 to 12 bits
_DATA	ENDS

LZW_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:LZW_TEXT,DS:DGROUP

lzwInStartCS	dw 0		;2A2D:000C offset of the input buffer
lzwNoRefillCS	dw 0		;2A2D:000E lzwNoRefill
lzwStateCS	dw 0		;2A2D:0010 0: start, 1: reading codes, 2: after a clear code

;read the next code: AX (as lzwReadCode). lzwBitPos moves on only once the
;code is read; when the buffer runs low with lzwNoRefill set, it returns
;from LZW_decompress
lzwReadCodeR	PROC NEAR	;2A2D:0012
	PUSH	BP
	MOV	BP,_lzwInSize
	MOV	BX,BP
	SUB	BX,3
	ADD	BX,lzwInStart
	MOV	DX,_lzwFile
	MOV	AX,_lzwBitPos
	MOV	CX,AX
	SHR	AX,1
	SHR	AX,1
	SHR	AX,1
	ADD	AX,WORD PTR _lzwIn
	MOV	WORD PTR _lzwIn,AX
	MOV	AX,CX
	AND	AX,7
	MOV	CX,AX
	ADD	AX,lzwCodeBits
	MOV	lzwNextBitPos,AX
	PUSH	CX
	PUSH	DS
	LDS	AX,DWORD PTR _lzwIn
	MOV	CX,AX
	CMP	AX,BX
	JB	L_008A
	MOV	SI,DS
	POP	DS
	PUSH	DS
	TEST	CS:lzwNoRefillCS,0FFFFh
	JZ	L_0060
	db	83h,26h		;AND _lzwBitPos,7 (83 /4)
	dw	OFFSET DGROUP:_lzwBitPos
	db	7
	POP	DS
	POP	CX
	POP	BP
	POP	AX		;the return address
	POP	SI		;LZW_decompress's registers
	POP	DI
	POP	BP
	db	0CBh		;RETF: back to LZW_decompress's caller
L_0060:
	MOV	AX,CS:lzwInStartCS
	MOV	WORD PTR _lzwIn,AX
	MOV	lzwInStart,AX
	MOV	DS,SI
	PUSH	ES
	PUSH	AX
	PUSH	DS
	POP	ES
	MOV	DI,AX
	MOV	SI,CX
	SUB	CX,BP
	SUB	AX,CX
	MOV	CX,AX
	REP	MOVSB
	SUB	BP,AX
	MOV	CX,BP
	MOV	BX,DX
	MOV	DX,DI
	MOV	AH,3Fh		;read file
	INT	21h
	POP	AX
	POP	ES
L_008A:
	MOV	SI,AX
	LODSW
	MOV	BX,AX
	LODSB
	POP	DS
	POP	CX
	JCXZ	L_009A
L_0094:
	SHR	AL,1
	RCR	BX,1
	LOOP	L_0094
L_009A:
	MOV	AX,BX
	MOV	BX,lzwCodeBits
	SHL	BX,1
	AND	AX,lzwMasks[BX-18]
	MOV	BX,lzwNextBitPos
	MOV	_lzwBitPos,BX
	POP	BP
	RET
lzwReadCodeR	ENDP

;read the next code: AX. When the buffer runs low, what is left moves to
;its start and the rest is read from the file (or, if lzwNoRefillCS is set, the
;decompression stops)
lzwReadCode	PROC NEAR	;2A2D:00BA
	PUSH	BP
	MOV	BP,_lzwInSize
	MOV	BX,BP
	SUB	BX,3
	ADD	BX,lzwInStart
	MOV	DX,_lzwFile
	MOV	AX,_lzwBitPos
	MOV	CX,AX
	SHR	AX,1
	SHR	AX,1
	SHR	AX,1
	ADD	AX,WORD PTR _lzwIn
	MOV	WORD PTR _lzwIn,AX
	MOV	AX,CX
	AND	AX,7
	MOV	CX,AX
	ADD	AX,lzwCodeBits
	MOV	_lzwBitPos,AX
	PUSH	CX
	PUSH	DS
	LDS	AX,DWORD PTR _lzwIn
	MOV	CX,AX
	CMP	AX,BX
	JL	L_012C
	MOV	SI,DS
	POP	DS
	MOV	AX,CS:lzwInStartCS
	MOV	WORD PTR _lzwIn,AX
	MOV	lzwInStart,AX
	PUSH	DS
	TEST	CS:lzwNoRefillCS,0FFFFh
	JZ	L_010C
	ADD	SP,8
	POP	SI
	POP	DI
	POP	BP
	RET
L_010C:
	MOV	DS,SI
	PUSH	ES
	PUSH	AX
	PUSH	DS
	POP	ES
	MOV	DI,AX
	MOV	SI,CX
	SUB	CX,BP
	SUB	AX,CX
	MOV	CX,AX
	REP	MOVSB
	SUB	BP,AX
	MOV	CX,BP
	MOV	BX,DX
	MOV	DX,DI
	MOV	AH,3Fh		;read file
	INT	21h
	POP	AX
	POP	ES
L_012C:
	MOV	SI,AX
	LODSW
	MOV	BX,AX
	LODSB
	POP	DS
	POP	CX
	JCXZ	L_013C
L_0136:
	SHR	AL,1
	RCR	BX,1
	LOOP	L_0136
L_013C:
	MOV	AX,BX
	MOV	BX,lzwCodeBits
	SHL	BX,1
	AND	AX,lzwMasks[BX-18]
	POP	BP
	RET
lzwReadCode	ENDP

;void LZW_decompress(void): decompress until the end code; goes on where it
;stopped when lzwStateCS is not 0
PUBLIC _LZW_decompress
_LZW_decompress	PROC FAR	;2A2D:0154
	PUSH	BP
	PUSH	DI
	PUSH	SI
	MOV	AX,WORD PTR _lzwIn
	MOV	lzwInStartCS,AX
	MOV	lzwInStart,AX
	MOV	AX,_lzwNoRefill
	MOV	lzwNoRefillCS,AX
	MOV	ES,_lzwDictSeg
	CMP	lzwStateCS,1
	JZ	L_018B
	CMP	lzwStateCS,2
	JZ	L_01CC
	CLD
	MOV	lzwCodeBits,9
	MOV	lzwCodeLimit,200h
	MOV	lzwNextCode,102h
	MOV	lzwStackCount,0
L_018B:
	MOV	lzwStateCS,1
	CALL	lzwReadCode
	CMP	AX,101h		;end
	JNZ	L_01AB
	MOV	_lzwNoRefill,0
	MOV	lzwStateCS,0
	POP	SI
	POP	DI
	POP	BP
	RET
L_01AB:
	CMP	AX,100h		;clear
	JZ	L_01B3
	JMP	SHORT L_01E8
	NOP
L_01B3:
	MOV	lzwCodeBits,9
	MOV	lzwCodeLimit,200h
	MOV	lzwNextCode,102h
	MOV	lzwStateCS,2
L_01CC:
	CALL	lzwReadCode
	MOV	lzwPrevCode,AX
	MOV	lzwFirstByte,AL
	PUSH	ES
	LES	DI,DWORD PTR _lzwOut
	STOSB
	POP	ES
	INC	WORD PTR _lzwOut
	JNZ	L_01E6
	INC	WORD PTR _lzwOut+2
L_01E6:
	JMP	SHORT L_018B
L_01E8:
	MOV	lzwCode,AX
	MOV	BX,AX
	SHL	BX,1
	ADD	BX,AX
	CMP	AX,lzwNextCode
	JL	L_020B
	MOV	AL,lzwFirstByte
	PUSH	AX
	INC	lzwStackCount
	MOV	ES:[BX+02],AL
	MOV	AX,lzwPrevCode
	MOV	ES:[BX],AX
	JMP	SHORT L_021C
L_020B:
	CMP	AX,0FFh
	JLE	L_0224
	MOV	AL,ES:[BX+02]
	PUSH	AX
	INC	lzwStackCount
	MOV	AX,ES:[BX]
L_021C:
	MOV	BX,AX
	SHL	BX,1
	ADD	BX,AX
	JMP	SHORT L_020B
L_0224:
	MOV	lzwFirstByte,AL
	PUSH	AX
	INC	lzwStackCount
	MOV	DX,ES
	LES	DI,DWORD PTR _lzwOut
	MOV	CX,lzwStackCount
	JCXZ	L_0250
	MOV	BX,ES
L_023A:
	POP	AX
	STOSB
	OR	DI,DI
	JNZ	L_0246
	ADD	BX,1000h
	MOV	ES,BX
L_0246:
	LOOP	L_023A
	MOV	WORD PTR _lzwOut+2,BX
	MOV	WORD PTR _lzwOut,DI
L_0250:
	MOV	lzwStackCount,CX
	MOV	ES,DX
	MOV	AX,lzwNextCode
	MOV	BX,AX
	SHL	AX,1
	ADD	BX,AX
	MOV	AL,lzwFirstByte
	MOV	ES:[BX+02],AL
	MOV	AX,lzwPrevCode
	MOV	ES:[BX],AX
	INC	lzwNextCode
	MOV	AX,lzwCode
	MOV	lzwPrevCode,AX
	MOV	BX,lzwNextCode
	CMP	BX,lzwCodeLimit
	JL	L_028F
	CMP	lzwCodeBits,0Ch
	JZ	L_028F
	INC	lzwCodeBits
	SHL	lzwCodeLimit,1
L_028F:
	JMP	L_018B
_LZW_decompress	ENDP

LZW_TEXT	ENDS
END
