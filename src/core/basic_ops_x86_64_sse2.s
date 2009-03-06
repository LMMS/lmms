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
	movslq	%esi,%rsi
	shrq	$6, %rsi
	testl	%esi, %esi
	jle	.L10
	subl	$1, %esi
	pxor	%xmm0, %xmm0
	salq	$6, %rsi
	leaq	64(%rdi,%rsi), %rax
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
	movss	(%rcx), %xmm1
	mulss	%xmm0, %xmm1
	cvttss2si	%xmm1, %esi
	movss	4(%rcx), %xmm1
	mulss	%xmm0, %xmm1
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
	movss	(%rcx), %xmm1
	mulss	%xmm0, %xmm1
	cvttss2si	%xmm1, %ebx
	movss	4(%rcx), %xmm1
	mulss	%xmm0, %xmm1
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
	movdqa	.LC1(%rip), %xmm4
	movq	%rsi, %r10
	shufps	$0, %xmm1, %xmm1
	xorl	%r9d, %r9d
	movdqa	.LC2(%rip), %xmm3
	movaps	%xmm1, %xmm9
	movdqa	.LC3(%rip), %xmm8
	.align 16
.L19:
	movaps	(%rcx), %xmm1
	addl	$1, %r9d
	movdqa	%xmm3, %xmm5
	mulps	%xmm9, %xmm1
	movaps	16(%rcx), %xmm6
	movdqa	%xmm3, %xmm7
	addq	$32, %rcx
	mulps	%xmm9, %xmm6
	cvttps2dq	%xmm1, %xmm1
	movdqa	%xmm1, %xmm2
	pcmpgtd	%xmm4, %xmm2
	cvttps2dq	%xmm6, %xmm6
	pand	%xmm2, %xmm1
	pandn	%xmm4, %xmm2
	por	%xmm1, %xmm2
	movdqa	%xmm2, %xmm1
	pcmpgtd	%xmm3, %xmm1
	pand	%xmm1, %xmm5
	pandn	%xmm2, %xmm1
	movdqa	%xmm1, %xmm2
	movdqa	%xmm6, %xmm1
	por	%xmm5, %xmm2
	pcmpgtd	%xmm4, %xmm1
	pand	%xmm1, %xmm6
	pandn	%xmm4, %xmm1
	movdqa	%xmm2, %xmm5
	pslld	$8, %xmm2
	pand	%xmm8, %xmm5
	por	%xmm6, %xmm1
	psrad	$8, %xmm5
	movdqa	%xmm1, %xmm6
	pcmpgtd	%xmm3, %xmm6
	pand	%xmm6, %xmm7
	pandn	%xmm1, %xmm6
	movdqa	%xmm6, %xmm1
	por	%xmm7, %xmm1
	movdqa	%xmm5, %xmm7
	movdqa	%xmm1, %xmm6
	pslld	$8, %xmm1
	pand	%xmm8, %xmm6
	psrad	$8, %xmm6
	punpcklwd	%xmm6, %xmm5
	punpckhwd	%xmm6, %xmm7
	movdqa	%xmm5, %xmm6
	punpcklwd	%xmm7, %xmm5
	punpckhwd	%xmm7, %xmm6
	punpcklwd	%xmm6, %xmm5
	movdqa	%xmm2, %xmm6
	punpcklwd	%xmm1, %xmm2
	punpckhwd	%xmm1, %xmm6
	movdqa	%xmm2, %xmm1
	punpcklwd	%xmm6, %xmm2
	punpckhwd	%xmm6, %xmm1
	punpcklwd	%xmm1, %xmm2
	por	%xmm2, %xmm5
	movdqa	%xmm5, (%r10)
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
	movdqa	.LC1(%rip), %xmm4
	movq	%rsi, %r10
	shufps	$0, %xmm1, %xmm1
	xorl	%r9d, %r9d
	movdqa	.LC2(%rip), %xmm3
	movaps	%xmm1, %xmm6
	.align 16
.L24:
	movaps	(%rcx), %xmm1
	addl	$1, %r9d
	movdqa	%xmm3, %xmm7
	mulps	%xmm6, %xmm1
	movaps	16(%rcx), %xmm5
	addq	$32, %rcx
	mulps	%xmm6, %xmm5
	cvttps2dq	%xmm1, %xmm1
	movdqa	%xmm1, %xmm2
	pcmpgtd	%xmm4, %xmm2
	cvttps2dq	%xmm5, %xmm5
	pand	%xmm2, %xmm1
	pandn	%xmm4, %xmm2
	por	%xmm1, %xmm2
	movdqa	%xmm2, %xmm1
	pcmpgtd	%xmm3, %xmm1
	pand	%xmm1, %xmm7
	pandn	%xmm2, %xmm1
	movdqa	%xmm1, %xmm2
	movdqa	%xmm5, %xmm1
	por	%xmm7, %xmm2
	movdqa	%xmm3, %xmm7
	pcmpgtd	%xmm4, %xmm1
	pand	%xmm1, %xmm5
	pandn	%xmm4, %xmm1
	por	%xmm5, %xmm1
	movdqa	%xmm1, %xmm5
	pcmpgtd	%xmm3, %xmm5
	pand	%xmm5, %xmm7
	pandn	%xmm1, %xmm5
	movdqa	%xmm5, %xmm1
	movdqa	%xmm2, %xmm5
	por	%xmm7, %xmm1
	punpcklwd	%xmm1, %xmm2
	punpckhwd	%xmm1, %xmm5
	movdqa	%xmm2, %xmm1
	punpcklwd	%xmm5, %xmm2
	punpckhwd	%xmm5, %xmm1
	punpcklwd	%xmm1, %xmm2
	movdqa	%xmm2, (%r10)
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
	.ident	"GCC: (GNU) 4.4.0 20090304 (experimental)"
