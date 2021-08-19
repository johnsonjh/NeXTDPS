|
|
| os_routinesmachcpsi.s
| Leo 02May88
| Rovner 02Jan90
|
	.globl	_os_abort,_abort
_os_abort:
	jmp	_abort
|
	.globl	_os_exit,_exit
_os_exit:
	jmp	_exit
|
	.globl	_os_bzero,_bzero
_os_bzero:
	jmp	_bzero
|
	.globl	_os_bcopy,_bcopy
_os_bcopy:
	jmp	_bcopy
|
|
