	.text
	.def	@feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
.set @feat.00, 0
	.file	"Mascal"
	.def	main;
	.scl	2;
	.type	32;
	.endef
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
main:                                   # @main
# %bb.0:                                # %entry
	pushq	%rbp
	subq	$32, %rsp
	leaq	32(%rsp), %rbp
	xorl	%esi, %esi
	callq	__main
	xorl	%eax, %eax
	.p2align	4, 0x90
.LBB0_1:                                # %while
                                        # =>This Inner Loop Header: Depth=1
	addl	$10, %eax
	incl	%esi
	cmpl	$1000, %esi                     # imm = 0x3E8
	jb	.LBB0_1
# %bb.2:                                # %continue
	addq	$32, %rsp
	popq	%rbp
	retq
                                        # -- End function
