// ASCII g++ TAB4 LF
// Attribute: 
// LastCheck: 20240218
// AllAuthor: @dosconio, @ArinaMgk
// ModuTitle: Demonstration - ELF32-C++ x86 Bare-Metal
// Copyright: Dosconio Mecocoa, BSD 3-Clause License
#define _STYLE_RUST
#define _DEBUG
#include <c/consio.h>
#include <c/driver/keyboard.h>
#include "../include/atx-x86-flap32.hpp"

use crate uni;
#ifdef _ARC_x86 // x86:

Handler_t syscalls[_TEMP 1];

//{TODO} Syscall class
//{TODO} Use callgate-para (but register nor kernel-area) to pass parameters
static const byte syscall_paracnts[0x100] = {
	1, //OUTC
	3, //INNC
	1, //EXIT
	0, //TIME ret(second)
	0, //REST
	3, //COMM
	0,0, 0,0,0,0,0,0,0,0,// 0XH
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,// 1XH
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,// 2XH
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,// 3XH
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,// 4XH
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,// 5XH
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,// 6XH
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,// 7XH
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,//	8XH
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,// 9XH
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,// AXH
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,// BXH
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,// CXH
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,// DXH
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,// EXH
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,
	3, //TEST // FXH
};
static stduint call_body(const syscall_t callid, ...) {
	auto task_switch_enable_old = task_switch_enable;//{TODO} {MUTEX for multi-Proc}
	task_switch_enable = false;
	//
	Letpara(paras, callid);
	stduint para[3];
	stduint ret = 0;
	if (_IMM(callid) >= numsof(syscall_paracnts)) return ~_IMM0;
	byte pcnt = syscall_paracnts[_IMM(callid)];
	for0(i, pcnt) {
		para[i] = para_next(paras, stduint);
	}
	switch (callid) {
	case syscall_t::OUTC: {
		ProcessBlock* pb = TaskGet(ProcessBlock::cpu0_task);
		ttycons[pb->focus_tty_id]->OutChar(para[0]);
		break;
	}
	case syscall_t::INNC: _TODO
		// Read from its TTY
		break;
	case syscall_t::EXIT:
		task_switch_enable = task_switch_enable_old;
		__asm("mov %0, %%eax" : : "m"(para[0]));//{unchk};
		TaskReturn();
		break;
	case syscall_t::TIME:
		task_switch_enable = task_switch_enable_old;
		return mecocoa_global->system_time.sec;
		break;
	case syscall_t::REST:
	// case_syscall_t_REST:
		switch_halt();
		break;
	case syscall_t::COMM:// (mode, obj, vaddr msg)
	{
		// assert(!src && src < numsof(Tasks));
		//{}INTERRUPT src == ~1
		ProcessBlock* pb = TaskGet(ProcessBlock::cpu0_task);
		if (para[0] == 0b01) { // SEND
			// ploginfo("%d wanna send to %d", pb->getID(), para[1]);
			ret = msg_send(pb, (para[1]), (CommMsg*)para[2]);
			// TaskGet(2)->Unblock(ProcessBlock::BlockReason::BR_RecvMsg);
		}
		else if (para[0] == 0b10) { // RECV
			// ploginfo("%d wanna recv fo %d", pb->getID(), para[1]);
			ret = msg_recv(pb, (para[1]), (CommMsg*)para[2]);
			// TaskGet(2)->Block(ProcessBlock::BlockReason::BR_RecvMsg);
		}
		else {
			printlog(_LOG_ERROR, "Bad `mode` of syscall 0x%[32H]", _IMM(callid));
		}
		if (pb->state == ProcessBlock::State::Pended) {
			// goto case_syscall_t_REST;
			switch_halt();
		}
		break;
	}






	case syscall_t::TEST:
		//{TODO} ISSUE 20250706 Each time subapp (Ring3) print %d or other integer by outsfmt() will panic, but OutInteger() or Kernel Ring0 is OK.
		if (para[0] == 'T' && para[1] == 'E' && para[2] == 'S') {
			rostr test_msg = "Syscalls Test OK!";
			Console.OutFormat("\xFF\x70[Mecocoa]\xFF\x02 PID");
			Console.OutInteger(ProcessBlock::cpu0_task, +10);
			Console.OutFormat(": %s\xFF\x07\n\r", test_msg + 0x80000000);
		}
		else {
			rostr test_msg = "\xFF\x70[Mecocoa]\xFF\x04 Syscalls Test FAIL!\xFF\x07";
			Console.OutFormat("%s %x %x %x\n\r", test_msg + 0x80000000,
				para[0], para[1], para[2]);
		}
		ret = ProcessBlock::cpu0_task;
		task_switch_enable = task_switch_enable_old;
		return ret; break;
		default:
		printlog(_LOG_ERROR, "Bad syscall: 0x%[32H]", _IMM(callid));
		break;
	}
	task_switch_enable = task_switch_enable_old;//{MUTEX for multi-Proc}
	return ret;
}


void call_gate() { // noreturn
	stduint para[4];// a c d b
	__asm("mov  %%eax, %0" : "=m"(para[0]));
	__asm("mov  %%ecx, %0" : "=m"(para[1]));
	__asm("mov  %%edx, %0" : "=m"(para[2]));
	__asm("mov  %%ebx, %0" : "=m"(para[3]));
	__asm("push %ebx;push %ecx;push %edx;push %ebp;push %esi;push %edi;cli;");
	__asm("call PG_PUSH");
	//auto task_switch_enable_old = task_switch_enable;//{TODO} {MUTEX for multi-Proc}
	//task_switch_enable = false;
	//ProcessBlock* pb_src = TaskGet(ProcessBlock::cpu0_task);
	//task_switch_enable = task_switch_enable_old;
	stduint ret = call_body((syscall_t)para[0], para[1], para[2], para[3]);
	__asm("call PG_POP");
	// pb_src->TSS.PDBR = getCR3();
	__asm("mov  %0, %%eax" : : "m"(ret));
	__asm("pop %edi");// popad
	__asm("pop %esi");
	__asm("pop %ebp");
	__asm("pop %edx");
	__asm("pop %ecx");
	__asm("pop %ebx");
	__asm("mov  %ebp, %esp");
	__asm("pop  %ebp      ");
	__asm("sti");
	__asm("jmp returnfar");
	__asm("callgate_endo:");
	loop;
}

void call_intr() {
	stduint para[4];// a c d b
	__asm("mov  %%eax, %0" : "=m"(para[0]));
	__asm("mov  %%ecx, %0" : "=m"(para[1]));
	__asm("mov  %%edx, %0" : "=m"(para[2]));
	__asm("mov  %%ebx, %0" : "=m"(para[3]));
	__asm("push %ebx;push %ecx;push %edx;push %ebp;push %esi;push %edi;");
	__asm("call PG_PUSH");
	stduint ret = call_body((syscall_t)para[0], para[1], para[2], para[3]);
	__asm("call PG_POP");
	__asm("mov  %0, %%eax" : : "m"(ret));
	__asm("pop %edi");// popad
	__asm("pop %esi");
	__asm("pop %ebp");
	__asm("pop %edx");
	__asm("pop %ecx");
	__asm("pop %ebx");
	__asm("mov  %ebp, %esp");
	__asm("pop  %ebp      ");
	__asm("iretl");// iretd
}

void* call_gate_entry() {
	return (void*)call_gate;
}

static stduint syscall0(syscall_t callid) {
	stduint ret;
	__asm("mov  %0, %%eax" : : "m"(callid));
	CallFar(0, SegCall);
	__asm("mov  %%eax, %0" : "=m"(ret));
	return ret;
}
static stduint syscall1(syscall_t callid, stduint para1) {
	stduint ret;
	__asm("mov  %0, %%eax" : : "m"(callid));
	__asm("mov  %0, %%ecx" : : "m"(para1));
	CallFar(0, SegCall);
	__asm("mov  %%eax, %0" : "=m"(ret));
	return ret;
}
static stduint syscall2(syscall_t callid, stduint para1, stduint para2) {
	stduint ret;
	__asm("mov  %0, %%eax" : : "m"(callid));
	__asm("mov  %0, %%ecx" : : "m"(para1));
	__asm("mov  %0, %%edx" : : "m"(para2));
	CallFar(0, SegCall);
	__asm("mov  %%eax, %0" : "=m"(ret));
	return ret;
}
static stduint syscall3(syscall_t callid, stduint para1, stduint para2, stduint para3) {
	stduint ret;
	__asm("mov  %0, %%eax" : : "m"(callid));
	__asm("mov  %0, %%ecx" : : "m"(para1));
	__asm("mov  %0, %%edx" : : "m"(para2));
	__asm("mov  %0, %%ebx" : : "m"(para3));
	CallFar(0, SegCall);
	__asm("mov  %%eax, %0" : "=m"(ret));
	return ret;
}

stduint syscall(syscall_t callid, ...) {
	// GCC style
	Letpara(paras, callid);
	stduint p1, p2, p3;
	switch (syscall_paracnts[_IMM(callid)])
	{
	case 0:
		return syscall0(callid);
		break;
	case 1:
		p1 = para_next(paras, stduint);
		return syscall1(callid, p1);
		break;
	case 2:
		p1 = para_next(paras, stduint);
		p2 = para_next(paras, stduint);
		return syscall2(callid, p1, p2);
		break;
	case 3:
		p1 = para_next(paras, stduint);
		p2 = para_next(paras, stduint);
		p3 = para_next(paras, stduint);
		return syscall3(callid, p1, p2, p3);
		break;
	default:
		break;
	}
	return ~_IMM0;
}

#endif
