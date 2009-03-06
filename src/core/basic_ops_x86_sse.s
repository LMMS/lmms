	.file	"basic_ops_x86.c"
	.text
	.p2align 4,,15
.globl alignedMemCpySSE
	.type	alignedMemCpySSE, @function
alignedMemCpySSE:
	pushl	%esi
	pushl	%ebx
	movl	20(%esp), %esi
	movl	12(%esp), %edx
	movl	16(%esp), %ecx
	shrl	$6, %esi
	testl	%esi, %esi
	je	.L4
	xorl	%eax, %eax
	xorl	%ebx, %ebx
	.p2align 4,,7
	.p2align 3
.L3:
	movaps	(%ecx,%eax), %xmm0
	addl	$1, %ebx
	movaps	%xmm0, (%edx,%eax)
	movaps	16(%ecx,%eax), %xmm0
	movaps	%xmm0, 16(%edx,%eax)
	movaps	32(%ecx,%eax), %xmm0
	movaps	%xmm0, 32(%edx,%eax)
	movaps	48(%ecx,%eax), %xmm0
	movaps	%xmm0, 48(%edx,%eax)
	addl	$64, %eax
	cmpl	%ebx, %esi
	jne	.L3
.L4:
	popl	%ebx
	popl	%esi
	ret
	.size	alignedMemCpySSE, .-alignedMemCpySSE
	.p2align 4,,15
.globl alignedMemClearSSE
	.type	alignedMemClearSSE, @function
alignedMemClearSSE:
	movl	8(%esp), %ecx
	shrl	$6, %ecx
	testl	%ecx, %ecx
	je	.L10
	movl	4(%esp), %eax
	xorps	%xmm0, %xmm0
	xorl	%edx, %edx
	.p2align 4,,7
	.p2align 3
.L9:
	addl	$1, %edx
	movaps	%xmm0, (%eax)
	movaps	%xmm0, 16(%eax)
	movaps	%xmm0, 32(%eax)
	movaps	%xmm0, 48(%eax)
	addl	$64, %eax
	cmpl	%edx, %ecx
	jne	.L9
.L10:
	rep
	ret
	.size	alignedMemClearSSE, .-alignedMemClearSSE
	.p2align 4,,15
.globl alignedBufApplyGainSSE
	.type	alignedBufApplyGainSSE, @function
alignedBufApplyGainSSE:
	movl	12(%esp), %ecx
	testl	%ecx, %ecx
	jle	.L15
	movss	8(%esp), %xmm0
	subl	$1, %ecx
	movl	4(%esp), %eax
	shrl	$3, %ecx
	xorl	%edx, %edx
	addl	$1, %ecx
	shufps	$0, %xmm0, %xmm0
	.p2align 4,,7
	.p2align 3
.L14:
	movaps	16(%eax), %xmm3
	addl	$1, %edx
	movaps	32(%eax), %xmm2
	mulps	%xmm0, %xmm3
	movaps	48(%eax), %xmm1
	mulps	%xmm0, %xmm2
	movaps	(%eax), %xmm4
	mulps	%xmm0, %xmm1
	movaps	%xmm3, 16(%eax)
	mulps	%xmm0, %xmm4
	movaps	%xmm2, 32(%eax)
	movaps	%xmm1, 48(%eax)
	movaps	%xmm4, (%eax)
	addl	$64, %eax
	cmpl	%edx, %ecx
	ja	.L14
.L15:
	rep
	ret
	.size	alignedBufApplyGainSSE, .-alignedBufApplyGainSSE
	.p2align 4,,15
.globl alignedBufMixSSE
	.type	alignedBufMixSSE, @function
alignedBufMixSSE:
	pushl	%esi
	pushl	%ebx
	movl	20(%esp), %esi
	movl	12(%esp), %edx
	movl	16(%esp), %ecx
	testl	%esi, %esi
	jle	.L20
	subl	$1, %esi
	xorl	%eax, %eax
	shrl	$3, %esi
	xorl	%ebx, %ebx
	addl	$1, %esi
	.p2align 4,,7
	.p2align 3
.L19:
	movaps	16(%edx,%eax), %xmm2
	addl	$1, %ebx
	movaps	32(%edx,%eax), %xmm1
	movaps	48(%edx,%eax), %xmm0
	movaps	(%edx,%eax), %xmm3
	addps	16(%ecx,%eax), %xmm2
	addps	32(%ecx,%eax), %xmm1
	addps	48(%ecx,%eax), %xmm0
	addps	(%ecx,%eax), %xmm3
	movaps	%xmm2, 16(%edx,%eax)
	movaps	%xmm3, (%edx,%eax)
	movaps	%xmm1, 32(%edx,%eax)
	movaps	%xmm0, 48(%edx,%eax)
	addl	$64, %eax
	cmpl	%ebx, %esi
	ja	.L19
.L20:
	popl	%ebx
	popl	%esi
	ret
	.size	alignedBufMixSSE, .-alignedBufMixSSE
	.p2align 4,,15
.globl alignedBufMixLRCoeffSSE
	.type	alignedBufMixLRCoeffSSE, @function
alignedBufMixLRCoeffSSE:
	pushl	%esi
	pushl	%ebx
	movl	28(%esp), %esi
	movl	12(%esp), %edx
	movl	16(%esp), %ebx
	testl	%esi, %esi
	jle	.L25
	movss	24(%esp), %xmm2
	subl	$1, %esi
	movss	20(%esp), %xmm0
	xorl	%eax, %eax
	shrl	$2, %esi
	xorl	%ecx, %ecx
	addl	$1, %esi
	unpcklps	%xmm2, %xmm0
	movaps	%xmm0, %xmm2
	movlhps	%xmm0, %xmm2
	.p2align 4,,7
	.p2align 3
.L24:
	movaps	16(%ebx,%eax), %xmm0
	addl	$1, %ecx
	movaps	(%ebx,%eax), %xmm1
	mulps	%xmm2, %xmm0
	mulps	%xmm2, %xmm1
	addps	16(%edx,%eax), %xmm0
	addps	(%edx,%eax), %xmm1
	movaps	%xmm0, 16(%edx,%eax)
	movaps	%xmm1, (%edx,%eax)
	addl	$32, %eax
	cmpl	%ecx, %esi
	ja	.L24
.L25:
	popl	%ebx
	popl	%esi
	ret
	.size	alignedBufMixLRCoeffSSE, .-alignedBufMixLRCoeffSSE
	.p2align 4,,15
.globl alignedBufWetDryMixSSE
	.type	alignedBufWetDryMixSSE, @function
alignedBufWetDryMixSSE:
	pushl	%esi
	pushl	%ebx
	movl	28(%esp), %esi
	movl	12(%esp), %edx
	movl	16(%esp), %ebx
	testl	%esi, %esi
	jle	.L30
	movss	24(%esp), %xmm3
	subl	$1, %esi
	movss	20(%esp), %xmm2
	xorl	%eax, %eax
	shrl	$2, %esi
	xorl	%ecx, %ecx
	shufps	$0, %xmm3, %xmm3
	addl	$1, %esi
	shufps	$0, %xmm2, %xmm2
	.p2align 4,,7
	.p2align 3
.L29:
	movaps	16(%ebx,%eax), %xmm1
	addl	$1, %ecx
	movaps	16(%edx,%eax), %xmm0
	mulps	%xmm2, %xmm1
	movaps	(%ebx,%eax), %xmm4
	mulps	%xmm3, %xmm0
	mulps	%xmm2, %xmm4
	addps	%xmm1, %xmm0
	movaps	(%edx,%eax), %xmm1
	mulps	%xmm3, %xmm1
	movaps	%xmm0, 16(%edx,%eax)
	addps	%xmm4, %xmm1
	movaps	%xmm1, (%edx,%eax)
	addl	$32, %eax
	cmpl	%ecx, %esi
	ja	.L29
.L30:
	popl	%ebx
	popl	%esi
	ret
	.size	alignedBufWetDryMixSSE, .-alignedBufWetDryMixSSE
	.p2align 4,,15
.globl alignedBufWetDryMixSplittedSSE
	.type	alignedBufWetDryMixSplittedSSE, @function
alignedBufWetDryMixSplittedSSE:
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$140, %esp
	movl	180(%esp), %eax
	flds	172(%esp)
	movl	160(%esp), %edx
	movl	164(%esp), %esi
	testl	%eax, %eax
	movl	168(%esp), %ecx
	flds	176(%esp)
	jle	.L43
	movl	180(%esp), %eax
	subl	$1, %eax
	shrl	%eax
	addl	$1, %eax
	movl	%eax, %ebp
	movl	%eax, 120(%esp)
	shrl	$2, %ebp
	cmpl	$3, 120(%esp)
	leal	0(,%ebp,4), %eax
	movl	%eax, 124(%esp)
	jbe	.L40
	testl	%eax, %eax
	jne	.L34
.L40:
	fxch	%st(1)
	xorl	%edi, %edi
	jmp	.L36
	.p2align 4,,7
	.p2align 3
.L34:
	fsts	12(%esp)
	fxch	%st(1)
	xorps	%xmm7, %xmm7
	movss	12(%esp), %xmm0
	movl	%esi, %ebx
	fsts	12(%esp)
	xorl	%eax, %eax
	xorl	%edi, %edi
	shufps	$0, %xmm0, %xmm0
	movaps	%xmm0, 32(%esp)
	movss	12(%esp), %xmm0
	shufps	$0, %xmm0, %xmm0
	movaps	%xmm0, 16(%esp)
	.p2align 4,,7
	.p2align 3
.L37:
	movaps	(%edx,%eax,2), %xmm5
	addl	$1, %edi
	movaps	16(%edx,%eax,2), %xmm6
	movaps	%xmm5, %xmm0
	shufps	$136, %xmm6, %xmm0
	movaps	32(%edx,%eax,2), %xmm4
	shufps	$221, %xmm6, %xmm5
	movaps	%xmm0, 96(%esp)
	movaps	48(%edx,%eax,2), %xmm3
	movaps	%xmm4, %xmm0
	shufps	$136, %xmm3, %xmm0
	movaps	96(%esp), %xmm2
	shufps	$221, %xmm3, %xmm4
	movaps	%xmm7, %xmm6
	movlps	(%ebx), %xmm6
	movaps	%xmm5, 80(%esp)
	movhps	8(%ebx), %xmm6
	shufps	$136, %xmm0, %xmm2
	movaps	%xmm0, 64(%esp)
	movaps	%xmm7, %xmm5
	movaps	%xmm6, %xmm0
	movlps	16(%ebx), %xmm5
	movhps	24(%ebx), %xmm5
	shufps	$136, %xmm5, %xmm0
	mulps	32(%esp), %xmm2
	shufps	$221, %xmm5, %xmm6
	movaps	%xmm4, 48(%esp)
	addl	$32, %ebx
	mulps	16(%esp), %xmm0
	movaps	%xmm7, %xmm4
	movlps	(%eax,%ecx), %xmm4
	movaps	%xmm7, %xmm3
	movhps	8(%eax,%ecx), %xmm4
	movaps	%xmm4, %xmm1
	movlps	16(%ecx,%eax), %xmm3
	movhps	24(%ecx,%eax), %xmm3
	shufps	$136, %xmm3, %xmm1
	addps	%xmm0, %xmm2
	movaps	80(%esp), %xmm0
	shufps	$221, %xmm3, %xmm4
	shufps	$136, 48(%esp), %xmm0
	mulps	16(%esp), %xmm1
	movaps	%xmm2, %xmm3
	movaps	80(%esp), %xmm5
	mulps	32(%esp), %xmm0
	shufps	$221, 48(%esp), %xmm5
	mulps	16(%esp), %xmm6
	addps	%xmm1, %xmm0
	movaps	96(%esp), %xmm1
	shufps	$221, 64(%esp), %xmm1
	mulps	16(%esp), %xmm4
	mulps	32(%esp), %xmm1
	mulps	32(%esp), %xmm5
	addps	%xmm6, %xmm1
	addps	%xmm4, %xmm5
	movaps	%xmm0, %xmm4
	unpcklps	%xmm1, %xmm3
	unpcklps	%xmm5, %xmm4
	unpckhps	%xmm1, %xmm2
	movaps	%xmm3, %xmm1
	unpckhps	%xmm5, %xmm0
	unpcklps	%xmm4, %xmm1
	unpckhps	%xmm4, %xmm3
	movaps	%xmm1, (%edx,%eax,2)
	movaps	%xmm2, %xmm1
	unpckhps	%xmm0, %xmm2
	unpcklps	%xmm0, %xmm1
	movaps	%xmm3, 16(%edx,%eax,2)
	movaps	%xmm1, 32(%edx,%eax,2)
	movaps	%xmm2, 48(%edx,%eax,2)
	addl	$32, %eax
	cmpl	%edi, %ebp
	ja	.L37
	movl	124(%esp), %edi
	movl	120(%esp), %eax
	addl	%edi, %edi
	cmpl	%eax, 124(%esp)
	je	.L44
.L36:
	leal	(%edx,%edi,8), %ebx
	xorl	%ebp, %ebp
	leal	8(%edx,%edi,8), %edx
	movl	%edi, %eax
	.p2align 4,,7
	.p2align 3
.L38:
	flds	(%ebx)
	addl	$2, %ebp
	fmul	%st(2), %st
	flds	(%esi,%eax,4)
	fmul	%st(2), %st
	faddp	%st, %st(1)
	fstps	(%ebx)
	flds	4(%ebx)
	fmul	%st(2), %st
	flds	(%ecx,%eax,4)
	fmul	%st(2), %st
	faddp	%st, %st(1)
	fstps	4(%ebx)
	addl	$16, %ebx
	flds	(%edx)
	fmul	%st(2), %st
	flds	4(%esi,%eax,4)
	fmul	%st(2), %st
	faddp	%st, %st(1)
	fstps	(%edx)
	flds	4(%edx)
	fmul	%st(2), %st
	flds	4(%ecx,%eax,4)
	leal	(%edi,%ebp), %eax
	fmul	%st(2), %st
	faddp	%st, %st(1)
	fstps	4(%edx)
	addl	$16, %edx
	cmpl	%eax, 180(%esp)
	jg	.L38
	fstp	%st(0)
	fstp	%st(0)
	jmp	.L39
.L43:
	fstp	%st(0)
	fstp	%st(0)
	jmp	.L39
.L44:
	fstp	%st(0)
	fstp	%st(0)
	.p2align 4,,7
	.p2align 3
.L39:
	addl	$140, %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
	.size	alignedBufWetDryMixSplittedSSE, .-alignedBufWetDryMixSplittedSSE
	.p2align 4,,15
.globl unalignedBufMixLRCoeffSSE
	.type	unalignedBufMixLRCoeffSSE, @function
unalignedBufMixLRCoeffSSE:
	pushl	%esi
	pushl	%ebx
	subl	$4, %esp
	movl	32(%esp), %esi
	flds	24(%esp)
	movl	16(%esp), %eax
	movl	20(%esp), %edx
	movl	%esi, %ebx
	flds	28(%esp)
	shrl	$31, %ebx
	leal	(%esi,%ebx), %ecx
	andl	$1, %ecx
	cmpl	%ebx, %ecx
	jne	.L54
.L46:
	testl	%esi, %esi
	jle	.L55
	leal	-1(%esi), %ebx
	shrl	%ebx
	testb	$15, %al
	jne	.L48
	fxch	%st(1)
	fstps	(%esp)
	xorps	%xmm2, %xmm2
	movss	(%esp), %xmm0
	addl	$1, %ebx
	fstps	(%esp)
	xorl	%ecx, %ecx
	movss	(%esp), %xmm1
	unpcklps	%xmm1, %xmm0
	movaps	%xmm0, %xmm3
	movlhps	%xmm0, %xmm3
	.p2align 4,,7
	.p2align 3
.L49:
	movaps	%xmm2, %xmm1
	addl	$1, %ecx
	movlps	(%edx), %xmm1
	movhps	8(%edx), %xmm1
	movaps	%xmm2, %xmm0
	movlps	(%eax), %xmm0
	movhps	8(%eax), %xmm0
	addl	$16, %edx
	mulps	%xmm3, %xmm1
	addps	%xmm1, %xmm0
	movaps	%xmm0, (%eax)
	addl	$16, %eax
	cmpl	%ebx, %ecx
	jb	.L49
	jmp	.L51
	.p2align 4,,7
	.p2align 3
.L55:
	fstp	%st(0)
	fstp	%st(0)
	.p2align 4,,7
	.p2align 3
.L51:
	addl	$4, %esp
	popl	%ebx
	popl	%esi
	ret
	.p2align 4,,7
	.p2align 3
.L48:
	xorl	%ecx, %ecx
	.p2align 4,,7
	.p2align 3
.L50:
	flds	(%edx,%ecx,8)
	fmul	%st(2), %st
	fadds	(%eax,%ecx,8)
	fstps	(%eax,%ecx,8)
	flds	4(%edx,%ecx,8)
	fmul	%st(1), %st
	fadds	4(%eax,%ecx,8)
	fstps	4(%eax,%ecx,8)
	flds	8(%edx,%ecx,8)
	fmul	%st(2), %st
	fadds	8(%eax,%ecx,8)
	fstps	8(%eax,%ecx,8)
	flds	12(%edx,%ecx,8)
	fmul	%st(1), %st
	fadds	12(%eax,%ecx,8)
	fstps	12(%eax,%ecx,8)
	addl	$2, %ecx
	cmpl	%ecx, %esi
	jg	.L50
	fstp	%st(0)
	fstp	%st(0)
	addl	$4, %esp
	popl	%ebx
	popl	%esi
	ret
.L54:
	flds	(%edx)
	subl	$1, %esi
	fmul	%st(2), %st
	fadds	(%eax)
	fstps	(%eax)
	flds	4(%edx)
	addl	$8, %edx
	fmul	%st(1), %st
	fadds	4(%eax)
	fstps	4(%eax)
	addl	$8, %eax
	jmp	.L46
	.size	unalignedBufMixLRCoeffSSE, .-unalignedBufMixLRCoeffSSE
	.ident	"GCC: (GNU) 4.4.0 20090304 (experimental)"
	.section	.note.GNU-stack,"",@progbits
