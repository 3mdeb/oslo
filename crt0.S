	.text
	.globl  _start, _stack, _magic
_start:
	leal    _stack,%esp
	movl    %eax,%edx
	movl    %ebx,%eax	
	pushl   $_exit
	jmp     _main


        .align  4, 0x90
        .long   0x1BADB002              /* magic */
        .long   0x00000000              /* feature flags */
        .long   0 - 0x1BADB002

_magic:	
	.long   0x12345678
	.bss
        .space  512
_stack:
