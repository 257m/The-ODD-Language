	.text
	.file	"test.odd"
	.section	.rodata.cst8,"aM",@progbits,8
	.p2align	3                               # -- Begin function factorial
.LCPI0_0:
	.quad	0xbff0000000000000              # double -1
.LCPI0_1:
	.quad	0x3ff0000000000000              # double 1
	.text
	.globl	factorial
	.p2align	4, 0x90
	.type	factorial,@function
factorial:                              # @factorial
# %bb.0:                                # %entry
	movsd	.LCPI0_0(%rip), %xmm1           # xmm1 = mem[0],zero
	addsd	%xmm0, %xmm1
	movsd	.LCPI0_1(%rip), %xmm2           # xmm2 = mem[0],zero
	ucomisd	%xmm1, %xmm2
	jae	.LBB0_3
# %bb.1:                                # %then.preheader
	movsd	.LCPI0_0(%rip), %xmm3           # xmm3 = mem[0],zero
	.p2align	4, 0x90
.LBB0_2:                                # %then
                                        # =>This Inner Loop Header: Depth=1
	mulsd	%xmm1, %xmm0
	addsd	%xmm3, %xmm1
	ucomisd	%xmm1, %xmm2
	jb	.LBB0_2
.LBB0_3:                                # %ifcont
	retq
.Lfunc_end0:
	.size	factorial, .Lfunc_end0-factorial
                                        # -- End function
	.section	.rodata.cst8,"aM",@progbits,8
	.p2align	3                               # -- Begin function degtorad
.LCPI1_0:
	.quad	0x3f91df46a226e211              # double 0.017453292509999999
	.text
	.globl	degtorad
	.p2align	4, 0x90
	.type	degtorad,@function
degtorad:                               # @degtorad
# %bb.0:                                # %entry
	mulsd	.LCPI1_0(%rip), %xmm0
	retq
.Lfunc_end1:
	.size	degtorad, .Lfunc_end1-degtorad
                                        # -- End function
	.section	.rodata.cst8,"aM",@progbits,8
	.p2align	3                               # -- Begin function main
.LCPI2_0:
	.quad	0x405e000000000000              # double 120
	.text
	.globl	main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rax
	.cfi_def_cfa_offset 16
	movsd	.LCPI2_0(%rip), %xmm0           # xmm0 = mem[0],zero
	callq	printd@PLT
	xorps	%xmm0, %xmm0
	popq	%rax
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end2:
	.size	main, .Lfunc_end2-main
	.cfi_endproc
                                        # -- End function
	.section	".note.GNU-stack","",@progbits