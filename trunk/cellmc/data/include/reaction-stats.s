	.file	"reaction-stats.c"
	.section	.rodata
	.align 16
	.type	UV_1_4sf, @object
	.size	UV_1_4sf, 16
UV_1_4sf:
	.long	1065353216
	.long	1065353216
	.long	1065353216
	.long	1065353216
	.align 16
	.type	UV_0_4sf, @object
	.size	UV_0_4sf, 16
UV_0_4sf:
	.zero	16
	.align 16
	.type	UV_0_2df, @object
	.size	UV_0_2df, 16
UV_0_2df:
	.zero	16
	.align 16
	.type	UV_1_2df, @object
	.size	UV_1_2df, 16
UV_1_2df:
	.long	0
	.long	1072693248
	.long	0
	.long	1072693248
	.align 16
	.type	UV_0_4si, @object
	.size	UV_0_4si, 16
UV_0_4si:
	.zero	16
	.align 16
	.type	UV_1_4si, @object
	.size	UV_1_4si, 16
UV_1_4si:
	.long	1
	.long	1
	.long	1
	.long	1
	.align 16
	.type	UV_1_2di, @object
	.size	UV_1_2di, 16
UV_1_2di:
	.quad	1
	.quad	1
	.align 16
	.type	UV_f_2di, @object
	.size	UV_f_2di, 16
UV_f_2di:
	.quad	-1
	.quad	-1
	.text
	.type	_cmc_id_search, @function
_cmc_id_search:
.LFB496:
	pushq	%rbp
.LCFI0:
	movq	%rsp, %rbp
.LCFI1:
	subq	$48, %rsp
.LCFI2:
	movq	%rdi, -24(%rbp)
	movl	%esi, -28(%rbp)
	movq	%rdx, -40(%rbp)
	movl	$0, -4(%rbp)
	jmp	.L2
.L3:
	movl	-4(%rbp), %eax
	cltq
	salq	$3, %rax
	addq	-24(%rbp), %rax
	movq	(%rax), %rax
	leaq	1(%rax), %rdi
	call	strlen
	movq	%rax, %rdx
	movl	-4(%rbp), %eax
	cltq
	salq	$3, %rax
	addq	-24(%rbp), %rax
	movq	(%rax), %rdi
	movq	-40(%rbp), %rsi
	call	strncmp
	testl	%eax, %eax
	jne	.L4
	movl	-4(%rbp), %eax
	movl	%eax, -44(%rbp)
	jmp	.L6
.L4:
	addl	$1, -4(%rbp)
.L2:
	movl	-4(%rbp), %eax
	cmpl	-28(%rbp), %eax
	jl	.L3
	movl	$-1, -44(%rbp)
.L6:
	movl	-44(%rbp), %eax
	leave
	ret
.LFE496:
	.size	_cmc_id_search, .-_cmc_id_search
	.section	.rodata
.LC0:
	.string	"single"
.LC1:
	.string	"double"
	.data
	.align 16
	.type	_cmc_prec, @object
	.size	_cmc_prec, 24
_cmc_prec:
	.quad	0
	.quad	.LC0
	.quad	.LC1
	.section	.rodata
.LC2:
	.string	"asm"
.LC3:
	.string	"lib"
.LC4:
	.string	"fpu"
	.data
	.align 32
	.type	_cmc_log, @object
	.size	_cmc_log, 32
_cmc_log:
	.quad	0
	.quad	.LC2
	.quad	.LC3
	.quad	.LC4
	.section	.rodata
.LC5:
	.string	"stdlib"
.LC6:
	.string	"rsmt"
.LC7:
	.string	"mc_rand"
	.data
	.align 32
	.type	_cmc_rng, @object
	.size	_cmc_rng, 32
_cmc_rng:
	.quad	0
	.quad	.LC5
	.quad	.LC6
	.quad	.LC7
	.section	.rodata
.LC8:
	.string	"none"
.LC9:
	.string	"semi"
.LC10:
	.string	"full"
	.data
	.align 16
	.type	_cmc_lpr, @object
	.size	_cmc_lpr, 24
_cmc_lpr:
	.quad	.LC8
	.quad	.LC9
	.quad	.LC10
	.section	.rodata
.LC11:
	.string	"off"
.LC12:
	.string	"on"
	.data
	.align 16
	.type	_cmc_bool_str, @object
	.size	_cmc_bool_str, 16
_cmc_bool_str:
	.quad	.LC11
	.quad	.LC12
	.section	.rodata
.LC13:
	.string	"reaction-stats.c"
.LC14:
	.string	"FATAL: %s:%d:%s(): "
	.align 8
.LC15:
	.string	"File pointer 'fp' cannot be NULL"
.LC16:
	.string	".\n"
	.align 8
.LC17:
	.ascii	"<xsl:transform\n  xmlns:s='http://www.sbml.org/sbml/level2/v"
	.ascii	"ersion3'\n  xmlns:xsl='http://www.w3.org/1999/XSL/Transform'"
	.ascii	"\n  xmlns:m='http://www.w3.org/1998/Math/MathML'\n  xmlns:k="
	.ascii	"'http://polacksbacken.net/wiki/SSACBE'\n  version='1.0'\n>\n"
	.ascii	"\n  <xsl:output\n    method='xml'\n    media-type='text/xml'"
	.ascii	"\n    indent='yes'\n    encoding='utf-8'/>\n\n  <xsl:strip-s"
	.ascii	"pace elements='*'/>\n\n  <xsl:template match='k:provenance'>"
	.ascii	"\n    <k:provenance>\n      <xsl:apply-templates/>\n      <"
	.string	"k:via>%s</k:via>\n    </k:provenance>\n  </xsl:template>\n\n  <xsl:template match='@*|node()'>\n    <xsl:copy>\n      <xsl:apply-templates select='@*|node()'/>\n    </xsl:copy>\n  </xsl:template>\n\n  <xsl:template match='s:listOfReactions'>\n    <s:listOfReactions>\n"
	.align 8
.LC18:
	.string	"Stream pointer must not be NULL"
	.align 8
.LC19:
	.string	"    </s:listOfReactions>\n  </xsl:template>\n</xsl:transform>\n"
	.align 8
.LC20:
	.string	"Helper data (reaction_stats) is NULL"
.LC21:
	.string	"FILE pointer must not be NULL"
.LC22:
	.string	"calloc"
	.align 8
.LC23:
	.string	"ERROR: at %s:%d:%s() in call to %s(): "
