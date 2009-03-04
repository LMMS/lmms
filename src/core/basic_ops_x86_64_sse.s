	.file	"basic_ops_x86.c"
	.text
	.align 16
.globl alignedMemCpySSE
	.type	alignedMemCpySSE, @function
alignedMemCpySSE:
.LFB509:
	movslq	%edx,%rdx
	shrq	$6, %rdx
	testl	%edx, %edx
	jle	.L4
	subl	$1, %edx
	xorl	%eax, %eax
	addq	$1, %rdx
	salq	$6, %rdx
	.align 16
.L3:
	movaps	(%rsi,%rax), %xmm0
	movaps	%xmm0, (%rdi,%rax)
	movaps	16(%rsi,%rax), %xmm0
	movaps	%xmm0, 16(%rdi,%rax)
	movaps	32(%rsi,%rax), %xmm0
	movaps	%xmm0, 32(%rdi,%rax)
	movaps	48(%rsi,%rax), %xmm0
	movaps	%xmm0, 48(%rdi,%rax)
	addq	$64, %rax
	cmpq	%rdx, %rax
	jne	.L3
.L4:
	rep
	ret
.LFE509:
	.size	alignedMemCpySSE, .-alignedMemCpySSE
	.align 16
.globl alignedMemClearSSE
	.type	alignedMemClearSSE, @function
alignedMemClearSSE:
.LFB510:
	movslq	%esi,%rsi
	shrq	$6, %rsi
	testl	%esi, %esi
	jle	.L10
	subl	$1, %esi
	xorps	%xmm0, %xmm0
	salq	$6, %rsi
	leaq	64(%rdi,%rsi), %rax
	.align 16
.L9:
	movaps	%xmm0, (%rdi)
	movaps	%xmm0, 16(%rdi)
	movaps	%xmm0, 32(%rdi)
	movaps	%xmm0, 48(%rdi)
	addq	$64, %rdi
	cmpq	%rax, %rdi
	jne	.L9
.L10:
	rep
	ret
.LFE510:
	.size	alignedMemClearSSE, .-alignedMemClearSSE
	.align 16
.globl alignedBufApplyGainSSE
	.type	alignedBufApplyGainSSE, @function
alignedBufApplyGainSSE:
.LFB511:
	testl	%esi, %esi
	jle	.L15
	leal	-1(%rsi), %edx
	shufps	$0, %xmm0, %xmm0
	xorl	%eax, %eax
	shrl	$3, %edx
	addl	$1, %edx
	.align 16
.L14:
	movaps	16(%rdi), %xmm3
	addl	$1, %eax
	movaps	32(%rdi), %xmm2
	mulps	%xmm0, %xmm3
	movaps	48(%rdi), %xmm1
	mulps	%xmm0, %xmm2
	movaps	(%rdi), %xmm4
	mulps	%xmm0, %xmm1
	movaps	%xmm3, 16(%rdi)
	mulps	%xmm0, %xmm4
	movaps	%xmm2, 32(%rdi)
	movaps	%xmm1, 48(%rdi)
	movaps	%xmm4, (%rdi)
	addq	$64, %rdi
	cmpl	%eax, %edx
	ja	.L14
.L15:
	rep
	ret
.LFE511:
	.size	alignedBufApplyGainSSE, .-alignedBufApplyGainSSE
	.align 16
.globl alignedBufMixSSE
	.type	alignedBufMixSSE, @function
alignedBufMixSSE:
.LFB512:
	testl	%edx, %edx
	jle	.L20
	leal	-1(%rdx), %ecx
	xorl	%eax, %eax
	xorl	%edx, %edx
	shrl	$3, %ecx
	addl	$1, %ecx
	.align 16
.L19:
	movaps	16(%rdi,%rax), %xmm2
	addl	$1, %edx
	movaps	32(%rdi,%rax), %xmm1
	addps	16(%rsi,%rax), %xmm2
	movaps	48(%rdi,%rax), %xmm0
	addps	32(%rsi,%rax), %xmm1
	movaps	(%rdi,%rax), %xmm3
	addps	48(%rsi,%rax), %xmm0
	addps	(%rsi,%rax), %xmm3
	movaps	%xmm2, 16(%rdi,%rax)
	movaps	%xmm1, 32(%rdi,%rax)
	movaps	%xmm0, 48(%rdi,%rax)
	movaps	%xmm3, (%rdi,%rax)
	addq	$64, %rax
	cmpl	%edx, %ecx
	ja	.L19
.L20:
	rep
	ret
.LFE512:
	.size	alignedBufMixSSE, .-alignedBufMixSSE
	.align 16
.globl alignedBufMixLRCoeffSSE
	.type	alignedBufMixLRCoeffSSE, @function
alignedBufMixLRCoeffSSE:
.LFB513:
	testl	%edx, %edx
	jle	.L25
	unpcklps	%xmm1, %xmm0
	leal	-1(%rdx), %ecx
	xorl	%eax, %eax
	xorl	%edx, %edx
	shrl	$2, %ecx
	movlhps	%xmm0, %xmm0
	addl	$1, %ecx
	.align 16
.L24:
	movaps	16(%rsi,%rax), %xmm2
	addl	$1, %edx
	movaps	(%rsi,%rax), %xmm3
	mulps	%xmm0, %xmm2
	mulps	%xmm0, %xmm3
	addps	16(%rdi,%rax), %xmm2
	addps	(%rdi,%rax), %xmm3
	movaps	%xmm2, 16(%rdi,%rax)
	movaps	%xmm3, (%rdi,%rax)
	addq	$32, %rax
	cmpl	%edx, %ecx
	ja	.L24
.L25:
	rep
	ret
.LFE513:
	.size	alignedBufMixLRCoeffSSE, .-alignedBufMixLRCoeffSSE
	.align 16
.globl alignedBufWetDryMixSSE
	.type	alignedBufWetDryMixSSE, @function
alignedBufWetDryMixSSE:
.LFB515:
	testl	%edx, %edx
	jle	.L30
	leal	-1(%rdx), %ecx
	shufps	$0, %xmm1, %xmm1
	shufps	$0, %xmm0, %xmm0
	xorl	%eax, %eax
	shrl	$2, %ecx
	xorl	%edx, %edx
	addl	$1, %ecx
	.align 16
.L29:
	movaps	16(%rsi,%rax), %xmm3
	addl	$1, %edx
	movaps	16(%rdi,%rax), %xmm2
	mulps	%xmm0, %xmm3
	movaps	(%rsi,%rax), %xmm4
	mulps	%xmm1, %xmm2
	mulps	%xmm0, %xmm4
	addps	%xmm3, %xmm2
	movaps	(%rdi,%rax), %xmm3
	mulps	%xmm1, %xmm3
	movaps	%xmm2, 16(%rdi,%rax)
	addps	%xmm4, %xmm3
	movaps	%xmm3, (%rdi,%rax)
	addq	$32, %rax
	cmpl	%edx, %ecx
	ja	.L29
.L30:
	rep
	ret
.LFE515:
	.size	alignedBufWetDryMixSSE, .-alignedBufWetDryMixSSE
	.align 16
.globl alignedBufWetDryMixSplittedSSE
	.type	alignedBufWetDryMixSplittedSSE, @function
alignedBufWetDryMixSplittedSSE:
.LFB516:
	pushq	%rbp
.LCFI0:
	testl	%ecx, %ecx
	pushq	%rbx
.LCFI1:
	jle	.L39
	leal	-1(%rcx), %ebx
	shrl	%ebx
	addl	$1, %ebx
	movl	%ebx, %r11d
	shrl	$2, %r11d
	cmpl	$3, %ebx
	leal	0(,%r11,4), %ebp
	jbe	.L40
	testl	%ebp, %ebp
	jne	.L34
.L40:
	xorl	%r9d, %r9d
	jmp	.L36
	.align 16
.L34:
	movaps	%xmm1, %xmm2
	movq	%rdi, %rax
	xorps	%xmm10, %xmm10
	movq	%rsi, %r9
	shufps	$0, %xmm2, %xmm2
	movq	%rdx, %r8
	xorl	%r10d, %r10d
	movaps	%xmm2, %xmm12
	movaps	%xmm0, %xmm2
	shufps	$0, %xmm2, %xmm2
	movaps	%xmm2, %xmm11
	.align 16
.L37:
	movaps	(%rax), %xmm2
	addl	$1, %r10d
	movaps	%xmm10, %xmm9
	movaps	16(%rax), %xmm5
	movaps	%xmm2, %xmm4
	movlps	(%r9), %xmm9
	movaps	%xmm10, %xmm8
	movaps	32(%rax), %xmm14
	shufps	$136, %xmm5, %xmm4
	movhps	8(%r9), %xmm9
	movaps	48(%rax), %xmm3
	movaps	%xmm14, %xmm15
	movlps	16(%r9), %xmm8
	shufps	$221, %xmm5, %xmm2
	shufps	$136, %xmm3, %xmm15
	movhps	24(%r9), %xmm8
	shufps	$221, %xmm3, %xmm14
	movaps	%xmm4, %xmm5
	addq	$32, %r9
	movaps	%xmm9, %xmm3
	shufps	$136, %xmm15, %xmm5
	movaps	%xmm10, %xmm7
	shufps	$136, %xmm8, %xmm3
	movlps	(%r8), %xmm7
	movaps	%xmm10, %xmm6
	mulps	%xmm12, %xmm5
	movhps	8(%r8), %xmm7
	mulps	%xmm11, %xmm3
	movlps	16(%r8), %xmm6
	movaps	%xmm7, %xmm13
	movhps	24(%r8), %xmm6
	shufps	$221, %xmm15, %xmm4
	shufps	$221, %xmm8, %xmm9
	addq	$32, %r8
	shufps	$136, %xmm6, %xmm13
	addps	%xmm3, %xmm5
	movaps	%xmm2, %xmm3
	shufps	$221, %xmm6, %xmm7
	shufps	$136, %xmm14, %xmm3
	shufps	$221, %xmm14, %xmm2
	mulps	%xmm11, %xmm13
	movaps	%xmm5, %xmm6
	mulps	%xmm12, %xmm3
	mulps	%xmm12, %xmm4
	mulps	%xmm11, %xmm9
	addps	%xmm13, %xmm3
	mulps	%xmm12, %xmm2
	mulps	%xmm11, %xmm7
	addps	%xmm9, %xmm4
	addps	%xmm7, %xmm2
	unpcklps	%xmm4, %xmm6
	unpckhps	%xmm4, %xmm5
	movaps	%xmm3, %xmm4
	unpcklps	%xmm2, %xmm4
	unpckhps	%xmm2, %xmm3
	movaps	%xmm6, %xmm2
	unpcklps	%xmm4, %xmm2
	unpckhps	%xmm4, %xmm6
	movaps	%xmm2, (%rax)
	movaps	%xmm5, %xmm2
	unpckhps	%xmm3, %xmm5
	unpcklps	%xmm3, %xmm2
	movaps	%xmm6, 16(%rax)
	movaps	%xmm2, 32(%rax)
	movaps	%xmm5, 48(%rax)
	addq	$64, %rax
	cmpl	%r10d, %r11d
	ja	.L37
	cmpl	%ebx, %ebp
	leal	(%rbp,%rbp), %r9d
	je	.L39
.L36:
	movslq	%r9d,%rax
	leaq	1(%rax), %rbx
	leaq	0(,%rax,4), %r10
	leaq	(%rdi,%rax,8), %r8
	leaq	(%rdi,%rbx,8), %rax
	salq	$2, %rbx
	leaq	(%rsi,%r10), %r11
	leaq	(%rdx,%r10), %r10
	addq	%rbx, %rsi
	addq	%rbx, %rdx
	.align 16
.L38:
	movss	(%r11), %xmm3
	addl	$2, %r9d
	movss	(%r8), %xmm2
	mulss	%xmm0, %xmm3
	mulss	%xmm1, %xmm2
	addq	$8, %r11
	addss	%xmm3, %xmm2
	movss	%xmm2, (%r8)
	movss	4(%r8), %xmm2
	movss	(%r10), %xmm3
	mulss	%xmm1, %xmm2
	addq	$8, %r10
	mulss	%xmm0, %xmm3
	addss	%xmm3, %xmm2
	movss	%xmm2, 4(%r8)
	addq	$16, %r8
	movss	(%rsi), %xmm3
	addq	$8, %rsi
	movss	(%rax), %xmm2
	mulss	%xmm0, %xmm3
	mulss	%xmm1, %xmm2
	addss	%xmm3, %xmm2
	movss	%xmm2, (%rax)
	movss	4(%rax), %xmm2
	movss	(%rdx), %xmm3
	mulss	%xmm1, %xmm2
	addq	$8, %rdx
	mulss	%xmm0, %xmm3
	addss	%xmm3, %xmm2
	movss	%xmm2, 4(%rax)
	addq	$16, %rax
	cmpl	%r9d, %ecx
	jg	.L38
.L39:
	popq	%rbx
	popq	%rbp
	ret
.LFE516:
	.size	alignedBufWetDryMixSplittedSSE, .-alignedBufWetDryMixSplittedSSE
	.align 16
.globl unalignedBufMixLRCoeffSSE
	.type	unalignedBufMixLRCoeffSSE, @function
unalignedBufMixLRCoeffSSE:
.LFB514:
	movl	%edx, %ecx
	shrl	$31, %ecx
	leal	(%rdx,%rcx), %eax
	andl	$1, %eax
	cmpl	%ecx, %eax
	jne	.L52
.L44:
	testl	%edx, %edx
	jle	.L49
	subl	$1, %edx
	shrl	%edx
	testb	$15, %dil
	jne	.L46
	unpcklps	%xmm1, %xmm0
	addl	$1, %edx
	xorps	%xmm3, %xmm3
	xorl	%eax, %eax
	movlhps	%xmm0, %xmm0
	.align 16
.L47:
	movaps	%xmm3, %xmm2
	addl	$1, %eax
	movaps	%xmm3, %xmm1
	movlps	(%rsi), %xmm2
	movlps	(%rdi), %xmm1
	movhps	8(%rsi), %xmm2
	addq	$16, %rsi
	movhps	8(%rdi), %xmm1
	mulps	%xmm0, %xmm2
	addps	%xmm2, %xmm1
	movaps	%xmm1, (%rdi)
	addq	$16, %rdi
	cmpl	%edx, %eax
	jb	.L47
	rep
	ret
	.align 16
.L46:
	mov	%edx, %edx
	xorl	%eax, %eax
	addq	$1, %rdx
	salq	$4, %rdx
	.align 16
.L48:
	movss	(%rsi,%rax), %xmm2
	mulss	%xmm0, %xmm2
	addss	(%rdi,%rax), %xmm2
	movss	%xmm2, (%rdi,%rax)
	movss	4(%rsi,%rax), %xmm2
	mulss	%xmm1, %xmm2
	addss	4(%rdi,%rax), %xmm2
	movss	%xmm2, 4(%rdi,%rax)
	movss	8(%rsi,%rax), %xmm2
	mulss	%xmm0, %xmm2
	addss	8(%rdi,%rax), %xmm2
	movss	%xmm2, 8(%rdi,%rax)
	movss	12(%rsi,%rax), %xmm2
	mulss	%xmm1, %xmm2
	addss	12(%rdi,%rax), %xmm2
	movss	%xmm2, 12(%rdi,%rax)
	addq	$16, %rax
	cmpq	%rdx, %rax
	jne	.L48
.L49:
	rep
	ret
.L52:
	movss	(%rsi), %xmm2
	subl	$1, %edx
	mulss	%xmm0, %xmm2
	addss	(%rdi), %xmm2
	movss	%xmm2, (%rdi)
	movss	4(%rsi), %xmm2
	addq	$8, %rsi
	mulss	%xmm1, %xmm2
	addss	4(%rdi), %xmm2
	movss	%xmm2, 4(%rdi)
	addq	$8, %rdi
	jmp	.L44
.LFE514:
	.size	unalignedBufMixLRCoeffSSE, .-unalignedBufMixLRCoeffSSE
	.section	.eh_frame,"aw",@progbits
.Lframe1:
	.long	.LECIE1-.LSCIE1
.LSCIE1:
	.long	0x0
	.byte	0x1
	.string	"zR"
	.byte	0x1
	.byte	0x78
	.byte	0x10
	.byte	0x1
	.byte	0x3
	.byte	0xc
	.byte	0x7
	.byte	0x8
	.byte	0x11
	.byte	0x10
	.byte	0x1
	.align 8
.LECIE1:
.LSFDE1:
	.long	.LEFDE1-.LASFDE1
.LASFDE1:
	.long	.LASFDE1-.Lframe1
	.long	.LFB509
	.long	.LFE509-.LFB509
	.byte	0x0
	.align 8
.LEFDE1:
.LSFDE3:
	.long	.LEFDE3-.LASFDE3
.LASFDE3:
	.long	.LASFDE3-.Lframe1
	.long	.LFB510
	.long	.LFE510-.LFB510
	.byte	0x0
	.align 8
.LEFDE3:
.LSFDE5:
	.long	.LEFDE5-.LASFDE5
.LASFDE5:
	.long	.LASFDE5-.Lframe1
	.long	.LFB511
	.long	.LFE511-.LFB511
	.byte	0x0
	.align 8
.LEFDE5:
.LSFDE7:
	.long	.LEFDE7-.LASFDE7
.LASFDE7:
	.long	.LASFDE7-.Lframe1
	.long	.LFB512
	.long	.LFE512-.LFB512
	.byte	0x0
	.align 8
.LEFDE7:
.LSFDE9:
	.long	.LEFDE9-.LASFDE9
.LASFDE9:
	.long	.LASFDE9-.Lframe1
	.long	.LFB513
	.long	.LFE513-.LFB513
	.byte	0x0
	.align 8
.LEFDE9:
.LSFDE11:
	.long	.LEFDE11-.LASFDE11
.LASFDE11:
	.long	.LASFDE11-.Lframe1
	.long	.LFB515
	.long	.LFE515-.LFB515
	.byte	0x0
	.align 8
.LEFDE11:
.LSFDE13:
	.long	.LEFDE13-.LASFDE13
.LASFDE13:
	.long	.LASFDE13-.Lframe1
	.long	.LFB516
	.long	.LFE516-.LFB516
	.byte	0x0
	.byte	0x4
	.long	.LCFI0-.LFB516
	.byte	0xe
	.byte	0x10
	.byte	0x4
	.long	.LCFI1-.LCFI0
	.byte	0xe
	.byte	0x18
	.byte	0x11
	.byte	0x3
	.byte	0x3
	.byte	0x11
	.byte	0x6
	.byte	0x2
	.align 8
.LEFDE13:
.LSFDE15:
	.long	.LEFDE15-.LASFDE15
.LASFDE15:
	.long	.LASFDE15-.Lframe1
	.long	.LFB514
	.long	.LFE514-.LFB514
	.byte	0x0
	.align 8
.LEFDE15:
	.ident	"GCC: (GNU) 4.4.0 20090304 (experimental)"
