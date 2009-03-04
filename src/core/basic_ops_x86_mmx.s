	.file	"basic_ops_x86.c"
	.text
	.p2align 4,,15
.globl alignedMemCpyMMX
	.type	alignedMemCpyMMX, @function
alignedMemCpyMMX:
	pushl	%ebx
	subl	$112, %esp
	movl	128(%esp), %ebx
	movl	124(%esp), %eax
	shrl	$6, %ebx
#APP
# 42 "/home/toby/development/git/lmms/src/core/basic_ops_x86.c" 1
	 fsave 4(%esp); fwait

# 0 "" 2
# 44 "/home/toby/development/git/lmms/src/core/basic_ops_x86.c" 1
	1: prefetchnta (%eax)
   prefetchnta 64(%eax)
   prefetchnta 128(%eax)
   prefetchnta 192(%eax)
   prefetchnta 256(%eax)

# 0 "" 2
#NO_APP
	testl	%ebx, %ebx
	je	.L2
	movl	120(%esp), %ecx
	xorl	%edx, %edx
	.p2align 4,,7
	.p2align 3
.L3:
#APP
# 53 "/home/toby/development/git/lmms/src/core/basic_ops_x86.c" 1
	1: prefetchnta 320(%eax)
2: movq (%eax), %mm0
   movq 8(%eax), %mm1
   movq 16(%eax), %mm2
   movq 24(%eax), %mm3
   movq %mm0, (%ecx)
   movq %mm1, 8(%ecx)
   movq %mm2, 16(%ecx)
   movq %mm3, 24(%ecx)
   movq 32(%eax), %mm0
   movq 40(%eax), %mm1
   movq 48(%eax), %mm2
   movq 56(%eax), %mm3
   movq %mm0, 32(%ecx)
   movq %mm1, 40(%ecx)
   movq %mm2, 48(%ecx)
   movq %mm3, 56(%ecx)

# 0 "" 2
#NO_APP
	addl	$1, %edx
	addl	$64, %eax
	addl	$64, %ecx
	cmpl	%edx, %ebx
	jne	.L3
.L2:
#APP
# 75 "/home/toby/development/git/lmms/src/core/basic_ops_x86.c" 1
	 fsave 4(%esp); fwait

# 0 "" 2
#NO_APP
	addl	$112, %esp
	popl	%ebx
	ret
	.size	alignedMemCpyMMX, .-alignedMemCpyMMX
	.p2align 4,,15
.globl alignedMemClearMMX
	.type	alignedMemClearMMX, @function
alignedMemClearMMX:
	movl	8(%esp), %ecx
	shrl	$6, %ecx
	testl	%ecx, %ecx
	je	.L8
	movl	4(%esp), %edx
	xorl	%eax, %eax
	pxor	%mm0, %mm0
	.p2align 4,,7
	.p2align 3
.L9:
#APP
# 90 "/home/toby/development/git/lmms/src/core/basic_ops_x86.c" 1
	movq    %mm0, (%edx)
movq    %mm0, 8(%edx)
movq    %mm0, 16(%edx)
movq    %mm0, 24(%edx)
movq    %mm0, 32(%edx)
movq    %mm0, 40(%edx)
movq    %mm0, 48(%edx)
movq    %mm0, 56(%edx)

# 0 "" 2
#NO_APP
	addl	$1, %eax
	addl	$64, %edx
	cmpl	%eax, %ecx
	jne	.L9
.L8:
	emms
	ret
	.size	alignedMemClearMMX, .-alignedMemClearMMX
	.ident	"GCC: (GNU) 4.4.0 20090304 (experimental)"
	.section	.note.GNU-stack,"",@progbits
