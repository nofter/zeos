#include <asm.h>
#include <segment.h>

ENTRY (sys_call_table)
	.long sys_ni_syscall	//00
	.long sys_ni_syscall	//01
	.long sys_ni_syscall	//02
	.long sys_ni_syscall	//03
	.long sys_write				//04
	.long sys_ni_syscall	//05
	.long sys_ni_syscall	//06
	.long sys_ni_syscall	//07
	.long sys_ni_syscall	//08
	.long sys_ni_syscall	//09
	.long sys_gettime			//10

.globl MAX_SYSCALL
MAX_SYSCALL = (. - sys_call_table)/4
