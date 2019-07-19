/*
 * \brief   ASM routines like do_skinit or reboot.
 * \date    2006-03-28
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2006  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the OSLO package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


	.text
	.globl  _start, _exit, reboot, jmp_multiboot, do_skinit,  _skinit, _stack_end, _stack

_start:
	leal    _stack,%esp
	xchg    %eax,%edx
	xchg    %ebx,%eax
	pushl	%eax
	pushl   $_exit
	jmp     _main

_mbheader:
        .align  4, 0x90
        .long   0x1BADB002              /* magic */
        .long   0x00000000              /* feature flags */
        .long   0 - 0x1BADB002

reboot:
	/* triple fault */
	.byte 0xff, 0xff

jmp_multiboot:
	xchg %eax, %ebx
	mov $0x2badb002, %eax
	jmp *%edx
	  	  
do_skinit:
	mov    $__LOADER_START__, %eax
	skinit
	ret

_skinit:
        /* switch back to our stack */
	leal    _stack, %esp

	/* enable svm */
	mov     $0xc0000080, %ecx
	mov	$0x1000,     %eax
	xor	%edx, %edx
	wrmsr

	/* clear VM_CR to enable debugging */
 	mov     $0xc0010114, %ecx
  	xor	%eax,        %eax
  	xor	%edx,        %edx
 	wrmsr

        /* init gdt and segment register */
	lgdt    %ss:pgdt_desc
        movw    $0x10, %ax
	mov	%ax,   %ds
	mov	%ax,   %es
	mov	%ax,   %fs
	mov	%ax,   %gs

	/* load mbi address where we have saved them in _start */
	sub	$8, %esp
	movl	4(%esp), %eax

	/* allow interrupts and call main */
	stgi
	jmp     oslo


	/* the gdt to load after skinit */
	.align(8)
gdt:
pgdt_desc:
	.word 0x17
	.long gdt
	.word 0
_gdt_cs:
	.word 0xffff
	.word 0x0
	.word 0x9b00
	.word 0x00cf
_gdt_ds:
	.word 0xffff
	.word 0x0
	.word 0x9300
	.word 0x00cf


	/* our stack */
	.bss
_stack_end:
        .space  512
_stack:
