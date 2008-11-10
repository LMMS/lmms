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
	movaps	%xmm0, %xmm3
	addl	$1, %edx
	movaps	%xmm0, %xmm2
	movaps	%xmm0, %xmm1
	movaps	%xmm0, %xmm4
	mulps	16(%eax), %xmm3
	mulps	32(%eax), %xmm2
	mulps	48(%eax), %xmm1
	movaps	%xmm3, 16(%eax)
	mulps	(%eax), %xmm4
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
	movss	24(%esp), %xmm0
	subl	$1, %esi
	movss	20(%esp), %xmm1
	xorl	%eax, %eax
	shrl	$2, %esi
	xorl	%ecx, %ecx
	addl	$1, %esi
	unpcklps	%xmm0, %xmm1
	movaps	%xmm1, %xmm0
	movlhps	%xmm1, %xmm0
	.p2align 4,,7
	.p2align 3
.L24:
	movaps	%xmm0, %xmm1
	addl	$1, %ecx
	movaps	%xmm0, %xmm2
	mulps	16(%ebx,%eax), %xmm1
	mulps	(%ebx,%eax), %xmm2
	addps	16(%edx,%eax), %xmm1
	addps	(%edx,%eax), %xmm2
	movaps	%xmm1, 16(%edx,%eax)
	movaps	%xmm2, (%edx,%eax)
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
	movss	24(%esp), %xmm1
	subl	$1, %esi
	movss	20(%esp), %xmm0
	xorl	%eax, %eax
	shrl	$2, %esi
	xorl	%ecx, %ecx
	shufps	$0, %xmm1, %xmm1
	addl	$1, %esi
	shufps	$0, %xmm0, %xmm0
	.p2align 4,,7
	.p2align 3
.L29:
	movaps	%xmm1, %xmm3
	addl	$1, %ecx
	movaps	%xmm0, %xmm2
	movaps	%xmm1, %xmm4
	mulps	16(%edx,%eax), %xmm3
	mulps	16(%ebx,%eax), %xmm2
	mulps	(%edx,%eax), %xmm4
	addps	%xmm3, %xmm2
	movaps	%xmm0, %xmm3
	mulps	(%ebx,%eax), %xmm3
	movaps	%xmm2, 16(%edx,%eax)
	addps	%xmm4, %xmm3
	movaps	%xmm3, (%edx,%eax)
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
	movl	160(%esp), %edx
	movl	164(%esp), %esi
	movl	168(%esp), %ecx
	testl	%eax, %eax
	movss	172(%esp), %xmm4
	movss	176(%esp), %xmm5
	jle	.L39
	movl	180(%esp), %eax
	subl	$1, %eax
	shrl	%eax
	addl	$1, %eax
	movl	%eax, %ebp
	movl	%eax, 112(%esp)
	shrl	$2, %ebp
	cmpl	$3, 112(%esp)
	leal	0(,%ebp,4), %eax
	movl	%eax, 116(%esp)
	jbe	.L40
	testl	%eax, %eax
	jne	.L34
.L40:
	xorl	%edi, %edi
	jmp	.L36
	.p2align 4,,7
	.p2align 3
.L34:
	movaps	%xmm4, %xmm2
	xorps	%xmm6, %xmm6
	shufps	$0, %xmm2, %xmm2
	movaps	%xmm5, %xmm1
	movl	%esi, %ebx
	shufps	$0, %xmm1, %xmm1
	movaps	%xmm2, 32(%esp)
	xorl	%eax, %eax
	xorl	%edi, %edi
	movss	%xmm5, 124(%esp)
	movss	%xmm4, 120(%esp)
	movaps	%xmm1, %xmm4
	.p2align 4,,7
	.p2align 3
.L37:
	movaps	16(%edx,%eax,2), %xmm3
	addl	$1, %edi
	movaps	(%edx,%eax,2), %xmm2
	movaps	48(%edx,%eax,2), %xmm0
	movaps	%xmm2, %xmm5
	shufps	$221, %xmm3, %xmm2
	movaps	32(%edx,%eax,2), %xmm1
	shufps	$136, %xmm3, %xmm5
	movaps	%xmm2, 96(%esp)
	movaps	%xmm1, %xmm7
	shufps	$221, %xmm0, %xmm1
	shufps	$136, %xmm0, %xmm7
	movaps	%xmm1, 64(%esp)
	movaps	%xmm6, %xmm3
	movaps	%xmm5, (%esp)
	shufps	$136, %xmm7, %xmm5
	movlps	(%ebx), %xmm3
	movaps	%xmm6, %xmm2
	movhps	8(%ebx), %xmm3
	movaps	%xmm7, 80(%esp)
	movlps	16(%ebx), %xmm2
	movhps	24(%ebx), %xmm2
	movaps	96(%esp), %xmm7
	addl	$32, %ebx
	movaps	%xmm3, %xmm0
	shufps	$221, %xmm2, %xmm3
	shufps	$136, %xmm2, %xmm0
	shufps	$136, 64(%esp), %xmm7
	mulps	32(%esp), %xmm0
	movaps	%xmm6, %xmm1
	movlps	(%ecx,%eax), %xmm1
	movhps	8(%ecx,%eax), %xmm1
	movaps	96(%esp), %xmm2
	mulps	%xmm4, %xmm7
	shufps	$221, 64(%esp), %xmm2
	mulps	%xmm4, %xmm5
	mulps	32(%esp), %xmm3
	movaps	%xmm7, 16(%esp)
	movaps	%xmm1, %xmm7
	addps	%xmm0, %xmm5
	movaps	%xmm6, %xmm0
	movlps	16(%ecx,%eax), %xmm0
	movhps	24(%ecx,%eax), %xmm0
	shufps	$136, %xmm0, %xmm7
	shufps	$221, %xmm0, %xmm1
	mulps	32(%esp), %xmm7
	mulps	32(%esp), %xmm1
	mulps	%xmm4, %xmm2
	movaps	%xmm7, 48(%esp)
	movaps	16(%esp), %xmm7
	addps	48(%esp), %xmm7
	addps	%xmm1, %xmm2
	movaps	%xmm7, 16(%esp)
	movaps	(%esp), %xmm7
	shufps	$221, 80(%esp), %xmm7
	movaps	16(%esp), %xmm1
	mulps	%xmm4, %xmm7
	movaps	16(%esp), %xmm0
	unpckhps	%xmm2, %xmm1
	unpcklps	%xmm2, %xmm0
	movaps	%xmm1, %xmm2
	addps	%xmm3, %xmm7
	movaps	%xmm5, %xmm3
	unpcklps	%xmm7, %xmm3
	unpckhps	%xmm7, %xmm5
	movaps	%xmm3, %xmm1
	unpckhps	%xmm0, %xmm3
	unpcklps	%xmm0, %xmm1
	movaps	%xmm5, %xmm0
	unpckhps	%xmm2, %xmm5
	unpcklps	%xmm2, %xmm0
	movaps	%xmm1, (%edx,%eax,2)
	movaps	%xmm3, 16(%edx,%eax,2)
	movaps	%xmm0, 32(%edx,%eax,2)
	movaps	%xmm5, 48(%edx,%eax,2)
	addl	$32, %eax
	cmpl	%edi, %ebp
	ja	.L37
	movl	116(%esp), %edi
	movl	112(%esp), %eax
	movss	120(%esp), %xmm4
	movss	124(%esp), %xmm5
	addl	%edi, %edi
	cmpl	%eax, 116(%esp)
	je	.L39
.L36:
	leal	(%edx,%edi,8), %ebx
	xorl	%ebp, %ebp
	leal	8(%edx,%edi,8), %edx
	movl	%edi, %eax
	.p2align 4,,7
	.p2align 3
.L38:
	movaps	%xmm5, %xmm1
	addl	$2, %ebp
	movaps	%xmm4, %xmm0
	mulss	(%ebx), %xmm1
	mulss	(%esi,%eax,4), %xmm0
	addss	%xmm1, %xmm0
	movaps	%xmm5, %xmm1
	movss	%xmm0, (%ebx)
	movaps	%xmm4, %xmm0
	mulss	4(%ebx), %xmm1
	mulss	(%ecx,%eax,4), %xmm0
	addss	%xmm1, %xmm0
	movaps	%xmm5, %xmm1
	movss	%xmm0, 4(%ebx)
	addl	$16, %ebx
	movaps	%xmm4, %xmm0
	mulss	(%edx), %xmm1
	mulss	4(%esi,%eax,4), %xmm0
	addss	%xmm1, %xmm0
	movaps	%xmm5, %xmm1
	movss	%xmm0, (%edx)
	movaps	%xmm4, %xmm0
	mulss	4(%edx), %xmm1
	mulss	4(%ecx,%eax,4), %xmm0
	leal	(%edi,%ebp), %eax
	addss	%xmm1, %xmm0
	movss	%xmm0, 4(%edx)
	addl	$16, %edx
	cmpl	%eax, 180(%esp)
	jg	.L38
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
	movl	28(%esp), %esi
	movl	12(%esp), %eax
	movl	16(%esp), %edx
	movss	20(%esp), %xmm0
	movl	%esi, %ecx
	shrl	$31, %ecx
	leal	(%esi,%ecx), %ebx
	andl	$1, %ebx
	cmpl	%ecx, %ebx
	movss	24(%esp), %xmm3
	jne	.L52
.L44:
	testl	%esi, %esi
	jle	.L49
	leal	-1(%esi), %ebx
	shrl	%ebx
	testb	$15, %al
	jne	.L46
	movaps	%xmm0, %xmm1
	xorps	%xmm2, %xmm2
	unpcklps	%xmm3, %xmm1
	addl	$1, %ebx
	xorl	%ecx, %ecx
	movaps	%xmm1, %xmm3
	movlhps	%xmm1, %xmm3
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
	movaps	%xmm0, %xmm1
	mulss	(%edx,%ecx,8), %xmm1
	addss	(%eax,%ecx,8), %xmm1
	movss	%xmm1, (%eax,%ecx,8)
	movaps	%xmm3, %xmm1
	mulss	4(%edx,%ecx,8), %xmm1
	addss	4(%eax,%ecx,8), %xmm1
	movss	%xmm1, 4(%eax,%ecx,8)
	movaps	%xmm0, %xmm1
	mulss	8(%edx,%ecx,8), %xmm1
	addss	8(%eax,%ecx,8), %xmm1
	movss	%xmm1, 8(%eax,%ecx,8)
	movaps	%xmm3, %xmm1
	mulss	12(%edx,%ecx,8), %xmm1
	addss	12(%eax,%ecx,8), %xmm1
	movss	%xmm1, 12(%eax,%ecx,8)
	addl	$2, %ecx
	cmpl	%ecx, %esi
	jg	.L48
	popl	%ebx
	popl	%esi
	ret
.L52:
	movaps	%xmm0, %xmm1
	subl	$1, %esi
	movss	(%eax), %xmm2
	mulss	(%edx), %xmm1
	addss	%xmm2, %xmm1
	movss	4(%eax), %xmm2
	movss	%xmm1, (%eax)
	movaps	%xmm3, %xmm1
	mulss	4(%edx), %xmm1
	addl	$8, %edx
	addss	%xmm2, %xmm1
	movss	%xmm1, 4(%eax)
	addl	$8, %eax
	jmp	.L44
	.size	unalignedBufMixLRCoeffSSE, .-unalignedBufMixLRCoeffSSE
	.ident	"GCC: (GNU) 4.4.0 20081110 (experimental)"
	.section	.note.GNU-stack,"",@progbits
