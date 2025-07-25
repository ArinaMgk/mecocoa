; ASCII NASM0207 TAB4 CRLF
; Attribute: CPU(x86) File(HerELF)
; LastCheck: 20240505
; AllAuthor: @dosconio
; ModuTitle: User-lib of Mecocoa
; Copyright: Dosconio Mecocoa, BSD 3-Clause License

[CPU 586]
;%include "mecocoa/kernel.inc"

GLOBAL syscall

section .text

syscall:
	;PUSHAD
	PUSH EBX
	PUSH ECX
	PUSH EDX
	PUSH ESI
	PUSH EDI
	PUSH EBP
	MOV EAX, [ESP + 4*(1+6+0)]
	MOV ECX, [ESP + 4*(1+6+1)]
	MOV EDX, [ESP + 4*(1+6+2)]
	MOV EBX, [ESP + 4*(1+6+3)]
	CALL 8*3|3:0
	;POPAD;{TODO} PROC RETURN VALUE
	POP EBP
	POP EDI
	POP ESI
	POP EDX
	POP ECX
	POP EBX

RET

