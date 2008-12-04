	.file	"basic_ops_x86.c"
	.text
	.align 16
.globl alignedMemCpySSE2
	.type	alignedMemCpySSE2, @function
alignedMemCpySSE2:
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
	movdqa	(%rsi,%rax), %xmm0
	movdqa	%xmm0, (%rdi,%rax)
	movdqa	16(%rsi,%rax), %xmm0
	movdqa	%xmm0, 16(%rdi,%rax)
	movdqa	32(%rsi,%rax), %xmm0
	movdqa	%xmm0, 32(%rdi,%rax)
	movdqa	48(%rsi,%rax), %xmm0
	movdqa	%xmm0, 48(%rdi,%rax)
	addq	$64, %rax
	cmpq	%rdx, %rax
	jne	.L3
.L4:
	rep
	ret
.LFE509:
	.size	alignedMemCpySSE2, .-alignedMemCpySSE2
	.align 16
.globl alignedMemClearSSE2
	.type	alignedMemClearSSE2, @function
alignedMemClearSSE2:
.LFB510:
	movslq	%esi,%rax
	shrq	$6, %rax
	testl	%eax, %eax
	jle	.L10
	subl	$1, %eax
	pxor	%xmm0, %xmm0
	salq	$6, %rax
	leaq	64(%rax,%rdi), %rax
	.align 16
.L9:
	movdqa	%xmm0, (%rdi)
	movdqa	%xmm0, 16(%rdi)
	movdqa	%xmm0, 32(%rdi)
	movdqa	%xmm0, 48(%rdi)
	addq	$64, %rdi
	cmpq	%rax, %rdi
	jne	.L9
.L10:
	rep
	ret
.LFE510:
	.size	alignedMemClearSSE2, .-alignedMemClearSSE2
	.align 16
.globl alignedConvertToS16SSE2
	.type	alignedConvertToS16SSE2, @function
alignedConvertToS16SSE2:
.LFB511:
	pushq	%rbp
.LCFI0:
	testb	%cl, %cl
	movl	%edx, %eax
	mulss	.LC0(%rip), %xmm0
	pushq	%rbx
.LCFI1:
	jne	.L13
	testw	%dx, %dx
	jle	.L15
	movl	%edx, %ebx
	shrw	$2, %bx
	cmpw	$3, %dx
	leal	0(,%rbx,4), %r8d
	ja	.L33
.L28:
	xorl	%r8d, %r8d
	.align 16
.L23:
	movswq	%r8w,%rdx
	movl	$32767, %ebx
	leaq	(%rdi,%rdx,8), %rcx
	leaq	(%rsi,%rdx,4), %rdx
	movl	$-32768, %edi
	.align 16
.L25:
	movaps	%xmm0, %xmm1
	mulss	(%rcx), %xmm1
	cvttss2si	%xmm1, %esi
	movaps	%xmm0, %xmm1
	mulss	4(%rcx), %xmm1
	cmpl	$-32768, %esi
	cmovl	%edi, %esi
	cmpl	$32767, %esi
	cmovg	%ebx, %esi
	movw	%si, (%rdx)
	cvttss2si	%xmm1, %esi
	cmpl	$-32768, %esi
	cmovl	%edi, %esi
	cmpl	$32767, %esi
	cmovg	%ebx, %esi
	addl	$1, %r8d
	addq	$8, %rcx
	movw	%si, 2(%rdx)
	addq	$4, %rdx
	cmpw	%r8w, %ax
	jg	.L25
.L15:
	cwtl
	popq	%rbx
	sall	$2, %eax
	popq	%rbp
	ret
	.align 16
.L13:
	testw	%dx, %dx
	jle	.L15
	movl	%edx, %ebx
	shrw	$2, %bx
	cmpw	$3, %dx
	leal	0(,%rbx,4), %r8d
	ja	.L34
.L27:
	xorl	%r8d, %r8d
	.align 16
.L18:
	movswq	%r8w,%rdx
	leaq	(%rdi,%rdx,8), %rcx
	leaq	(%rsi,%rdx,4), %rdx
	movl	$-32768, %edi
	movl	$32767, %esi
	.align 16
.L20:
	movaps	%xmm0, %xmm1
	mulss	(%rcx), %xmm1
	cvttss2si	%xmm1, %ebx
	movaps	%xmm0, %xmm1
	mulss	4(%rcx), %xmm1
	cmpl	$-32768, %ebx
	cmovl	%edi, %ebx
	cmpl	$32767, %ebx
	cmovg	%esi, %ebx
	movzbl	%bh, %ebp
	sall	$8, %ebx
	movl	%ebp, %r9d
	orl	%r9d, %ebx
	movw	%bx, (%rdx)
	cvttss2si	%xmm1, %ebx
	cmpl	$-32768, %ebx
	cmovl	%edi, %ebx
	cmpl	$32767, %ebx
	cmovg	%esi, %ebx
	addl	$1, %r8d
	addq	$8, %rcx
	movzbl	%bh, %ebp
	sall	$8, %ebx
	movl	%ebp, %r9d
	orl	%r9d, %ebx
	movw	%bx, 2(%rdx)
	addq	$4, %rdx
	cmpw	%r8w, %ax
	jg	.L20
	cwtl
	popq	%rbx
	sall	$2, %eax
	popq	%rbp
	ret
	.align 16
.L34:
	testw	%r8w, %r8w
	je	.L27
	movaps	%xmm0, %xmm1
	movq	%rdi, %rcx
	movdqa	.LC1(%rip), %xmm2
	movq	%rsi, %r10
	shufps	$0, %xmm1, %xmm1
	xorl	%r9d, %r9d
	movdqa	.LC3(%rip), %xmm8
	movaps	%xmm1, %xmm9
	movdqa	.LC2(%rip), %xmm1
	.align 16
.L19:
	movaps	%xmm9, %xmm4
	addl	$1, %r9d
	movaps	%xmm9, %xmm3
	mulps	(%rcx), %xmm4
	movdqa	%xmm1, %xmm6
	mulps	16(%rcx), %xmm3
	addq	$32, %rcx
	cvttps2dq	%xmm4, %xmm4
	movdqa	%xmm4, %xmm5
	pcmpgtd	%xmm2, %xmm5
	cvttps2dq	%xmm3, %xmm3
	pand	%xmm5, %xmm4
	pandn	%xmm2, %xmm5
	por	%xmm5, %xmm4
	movdqa	%xmm4, %xmm5
	pcmpgtd	%xmm1, %xmm5
	pand	%xmm5, %xmm6
	pandn	%xmm4, %xmm5
	movdqa	%xmm5, %xmm4
	movdqa	%xmm3, %xmm5
	por	%xmm6, %xmm4
	movdqa	%xmm1, %xmm6
	pcmpgtd	%xmm2, %xmm5
	pand	%xmm5, %xmm3
	pandn	%xmm2, %xmm5
	movdqa	%xmm4, %xmm7
	pslld	$8, %xmm4
	pand	%xmm8, %xmm7
	por	%xmm5, %xmm3
	psrad	$8, %xmm7
	movdqa	%xmm3, %xmm5
	pcmpgtd	%xmm1, %xmm5
	pand	%xmm5, %xmm6
	pandn	%xmm3, %xmm5
	movdqa	%xmm5, %xmm3
	por	%xmm6, %xmm3
	movdqa	%xmm7, %xmm6
	movdqa	%xmm3, %xmm5
	pslld	$8, %xmm3
	pand	%xmm8, %xmm5
	psrad	$8, %xmm5
	punpcklwd	%xmm5, %xmm7
	punpckhwd	%xmm5, %xmm6
	movdqa	%xmm4, %xmm5
	punpcklwd	%xmm3, %xmm4
	movdqa	%xmm7, %xmm10
	punpckhwd	%xmm3, %xmm5
	punpcklwd	%xmm6, %xmm7
	punpckhwd	%xmm6, %xmm10
	punpcklwd	%xmm10, %xmm7
	movdqa	%xmm4, %xmm10
	punpcklwd	%xmm5, %xmm4
	punpckhwd	%xmm5, %xmm10
	punpcklwd	%xmm10, %xmm4
	por	%xmm7, %xmm4
	movdqa	%xmm4, (%r10)
	addq	$16, %r10
	cmpw	%r9w, %bx
	ja	.L19
	cmpw	%dx, %r8w
	jne	.L18
	jmp	.L15
	.align 16
.L33:
	testw	%r8w, %r8w
	je	.L28
	movaps	%xmm0, %xmm1
	movq	%rdi, %rcx
	movdqa	.LC1(%rip), %xmm2
	movq	%rsi, %r10
	shufps	$0, %xmm1, %xmm1
	xorl	%r9d, %r9d
	movaps	%xmm1, %xmm6
	movdqa	.LC2(%rip), %xmm1
	.align 16
.L24:
	movaps	%xmm6, %xmm4
	addl	$1, %r9d
	movaps	%xmm6, %xmm3
	mulps	(%rcx), %xmm4
	movdqa	%xmm1, %xmm7
	mulps	16(%rcx), %xmm3
	addq	$32, %rcx
	cvttps2dq	%xmm4, %xmm4
	movdqa	%xmm4, %xmm5
	pcmpgtd	%xmm2, %xmm5
	cvttps2dq	%xmm3, %xmm3
	pand	%xmm5, %xmm4
	pandn	%xmm2, %xmm5
	por	%xmm5, %xmm4
	movdqa	%xmm4, %xmm5
	pcmpgtd	%xmm1, %xmm5
	pand	%xmm5, %xmm7
	pandn	%xmm4, %xmm5
	movdqa	%xmm5, %xmm4
	movdqa	%xmm3, %xmm5
	por	%xmm7, %xmm4
	movdqa	%xmm1, %xmm7
	pcmpgtd	%xmm2, %xmm5
	pand	%xmm5, %xmm3
	pandn	%xmm2, %xmm5
	por	%xmm5, %xmm3
	movdqa	%xmm3, %xmm5
	pcmpgtd	%xmm1, %xmm5
	pand	%xmm5, %xmm7
	pandn	%xmm3, %xmm5
	movdqa	%xmm5, %xmm3
	movdqa	%xmm4, %xmm5
	por	%xmm7, %xmm3
	punpcklwd	%xmm3, %xmm4
	punpckhwd	%xmm3, %xmm5
	movdqa	%xmm4, %xmm7
	punpcklwd	%xmm5, %xmm4
	punpckhwd	%xmm5, %xmm7
	punpcklwd	%xmm7, %xmm4
	movdqa	%xmm4, (%r10)
	addq	$16, %r10
	cmpw	%r9w, %bx
	ja	.L24
	cmpw	%r8w, %dx
	jne	.L23
	jmp	.L15
.LFE511:
	.size	alignedConvertToS16SSE2, .-alignedConvertToS16SSE2
	.section	.rodata
	.align 4
.LC0:
	.long	1191181824
	.align 16
.LC1:
	.long	-32768
	.long	-32768
	.long	-32768
	.long	-32768
	.align 16
.LC2:
	.long	32767
	.long	32767
	.long	32767
	.long	32767
	.align 16
.LC3:
	.long	65280
	.long	65280
	.long	65280
	.long	65280
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
	.byte	0x4
	.long	.LCFI0-.LFB511
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
.LEFDE5:
	.ident	"GCC: (GNU) 4.4.0 20081204 (experimental)"
