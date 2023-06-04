.text
.global _gintctl_gint_gdb_bank1_test

_gintctl_gint_gdb_bank1_test:
	// SR.RB = 0
	stc	sr, r2
	mov	r2, r1
	mov.l	.sr_mask, r0
	or	r0, r1
	ldc	r1, sr

	// Now SR.RB = 1
	mov	#0x13, r3
	mov	#0x37, r4
	shll8	r3
	or	r3, r4
	ldc	r4, R0_BANK

	stc	R2_BANK, r2
	ldc	r2, sr
	// Now SR.RB = 0
	rts
	nop

.align 4
.sr_mask:	.long (1 << 29)
