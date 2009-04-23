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
	subl	$124, %esp
	movl	164(%esp), %eax
	movl	144(%esp), %edx
	movl	148(%esp), %esi
	movl	152(%esp), %ecx
	testl	%eax, %eax
	jle	.L39
	movl	164(%esp), %eax
	subl	$1, %eax
	shrl	%eax
	addl	$1, %eax
	movl	%eax, %ebp
	movl	%eax, 104(%esp)
	shrl	$2, %ebp
	cmpl	$3, 104(%esp)
	leal	0(,%ebp,4), %eax
	movl	%eax, 108(%esp)
	jbe	.L40
	testl	%eax, %eax
	jne	.L34
.L40:
	xorl	%edi, %edi
	jmp	.L36
	.p2align 4,,7
	.p2align 3
.L34:
	movss	160(%esp), %xmm0
	xorps	%xmm7, %xmm7
	movl	%esi, %ebx
	xorl	%eax, %eax
	xorl	%edi, %edi
	shufps	$0, %xmm0, %xmm0
	movaps	%xmm0, 16(%esp)
	movss	156(%esp), %xmm0
	shufps	$0, %xmm0, %xmm0
	movaps	%xmm0, (%esp)
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
	movaps	%xmm0, 80(%esp)
	movaps	48(%edx,%eax,2), %xmm3
	movaps	%xmm4, %xmm0
	shufps	$136, %xmm3, %xmm0
	movaps	80(%esp), %xmm2
	shufps	$221, %xmm3, %xmm4
	movaps	%xmm7, %xmm6
	movlps	(%ebx), %xmm6
	movaps	%xmm5, 64(%esp)
	movhps	8(%ebx), %xmm6
	shufps	$136, %xmm0, %xmm2
	movaps	%xmm0, 48(%esp)
	movaps	%xmm7, %xmm5
	movaps	%xmm6, %xmm0
	movlps	16(%ebx), %xmm5
	movhps	24(%ebx), %xmm5
	shufps	$136, %xmm5, %xmm0
	mulps	16(%esp), %xmm2
	shufps	$221, %xmm5, %xmm6
	movaps	%xmm4, 32(%esp)
	addl	$32, %ebx
	mulps	(%esp), %xmm0
	movaps	%xmm7, %xmm4
	movlps	(%eax,%ecx), %xmm4
	movaps	%xmm7, %xmm3
	movhps	8(%eax,%ecx), %xmm4
	movaps	%xmm4, %xmm1
	movlps	16(%ecx,%eax), %xmm3
	movhps	24(%ecx,%eax), %xmm3
	shufps	$136, %xmm3, %xmm1
	addps	%xmm0, %xmm2
	movaps	64(%esp), %xmm0
	shufps	$221, %xmm3, %xmm4
	shufps	$136, 32(%esp), %xmm0
	mulps	(%esp), %xmm1
	movaps	%xmm2, %xmm3
	movaps	64(%esp), %xmm5
	mulps	16(%esp), %xmm0
	shufps	$221, 32(%esp), %xmm5
	mulps	(%esp), %xmm6
	addps	%xmm1, %xmm0
	movaps	80(%esp), %xmm1
	shufps	$221, 48(%esp), %xmm1
	mulps	(%esp), %xmm4
	mulps	16(%esp), %xmm1
	mulps	16(%esp), %xmm5
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
	movl	108(%esp), %edi
	movl	104(%esp), %eax
	addl	%edi, %edi
	cmpl	%eax, 108(%esp)
	je	.L39
.L36:
	movss	156(%esp), %xmm0
	xorl	%ebp, %ebp
	movss	160(%esp), %xmm1
	movl	%edi, %eax
	leal	(%edx,%edi,8), %ebx
	leal	8(%edx,%edi,8), %edx
	.p2align 4,,7
	.p2align 3
.L38:
	movss	(%esi,%eax,4), %xmm3
	addl	$2, %ebp
	movss	(%ebx), %xmm2
	mulss	%xmm0, %xmm3
	mulss	%xmm1, %xmm2
	addss	%xmm3, %xmm2
	movss	%xmm2, (%ebx)
	movss	4(%ebx), %xmm2
	movss	(%ecx,%eax,4), %xmm3
	mulss	%xmm1, %xmm2
	mulss	%xmm0, %xmm3
	addss	%xmm3, %xmm2
	movss	%xmm2, 4(%ebx)
	addl	$16, %ebx
	movss	4(%esi,%eax,4), %xmm3
	movss	(%edx), %xmm2
	mulss	%xmm0, %xmm3
	mulss	%xmm1, %xmm2
	addss	%xmm3, %xmm2
	movss	%xmm2, (%edx)
	movss	4(%edx), %xmm2
	movss	4(%ecx,%eax,4), %xmm3
	mulss	%xmm1, %xmm2
	leal	(%edi,%ebp), %eax
	mulss	%xmm0, %xmm3
	addss	%xmm3, %xmm2
	movss	%xmm2, 4(%edx)
	addl	$16, %edx
	cmpl	%eax, 164(%esp)
	jg	.L38
.L39:
	addl	$124, %esp
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
	movl	28(%esp), %ebx
	movl	12(%esp), %eax
	movl	16(%esp), %edx
	movss	20(%esp), %xmm1
	movl	%ebx, %esi
	shrl	$31, %esi
	leal	(%ebx,%esi), %ecx
	andl	$1, %ecx
	cmpl	%esi, %ecx
	movss	24(%esp), %xmm3
	jne	.L52
.L44:
	testl	%ebx, %ebx
	jle	.L49
	testb	$15, %al
	jne	.L46
	movaps	%xmm1, %xmm0
	subl	$1, %ebx
	unpcklps	%xmm3, %xmm0
	shrl	%ebx
	xorps	%xmm2, %xmm2
	movaps	%xmm0, %xmm3
	addl	$1, %ebx
	movlhps	%xmm0, %xmm3
	xorl	%ecx, %ecx
	.p2align 4,,7
	.p2align 3
.L47:
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
	jb	.L47
.L49:
	popl	%ebx
	popl	%esi
	ret
	.p2align 4,,7
	.p2align 3
.L46:
	xorl	%ecx, %ecx
	.p2align 4,,7
	.p2align 3
.L48:
	movss	(%edx,%ecx,8), %xmm0
	mulss	%xmm1, %xmm0
	addss	(%eax,%ecx,8), %xmm0
	movss	%xmm0, (%eax,%ecx,8)
	movss	4(%edx,%ecx,8), %xmm0
	mulss	%xmm3, %xmm0
	addss	4(%eax,%ecx,8), %xmm0
	movss	%xmm0, 4(%eax,%ecx,8)
	movss	8(%edx,%ecx,8), %xmm0
	mulss	%xmm1, %xmm0
	addss	8(%eax,%ecx,8), %xmm0
	movss	%xmm0, 8(%eax,%ecx,8)
	movss	12(%edx,%ecx,8), %xmm0
	mulss	%xmm3, %xmm0
	addss	12(%eax,%ecx,8), %xmm0
	movss	%xmm0, 12(%eax,%ecx,8)
	addl	$2, %ecx
	cmpl	%ecx, %ebx
	jg	.L48
	popl	%ebx
	popl	%esi
	ret
.L52:
	movss	(%edx), %xmm0
	subl	$1, %ebx
	mulss	%xmm1, %xmm0
	addss	(%eax), %xmm0
	movss	%xmm0, (%eax)
	movss	4(%edx), %xmm0
	addl	$8, %edx
	mulss	%xmm3, %xmm0
	addss	4(%eax), %xmm0
	movss	%xmm0, 4(%eax)
	addl	$8, %eax
	jmp	.L44
	.size	unalignedBufMixLRCoeffSSE, .-unalignedBufMixLRCoeffSSE
	.ident	"GCC: (Ubuntu 4.4.0-0ubuntu2) 4.4.0"
	.section	.note.GNU-stack,"",@progbits
