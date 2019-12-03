	ld sp,$fffe		; $0000  setup stack

	xor a			; $0003  zero the memory from $8000-$9fff (vram)
	ld hl,$9fff		; $0004
addr_0007:
	ld (hl-),a		; $0007
	bit 7,h		; $0008
	jr nz, addr_0007	; $000a

	ld hl,$ff26		; $000c  setup audio
	ld c,$11		; $000f
	ld a,$80		; $0011
	ld (hl-),a		; $0013
	ld ($ff00+c),a	; $0014
	inc c			; $0015
	ld a,$f3		; $0016
	ld ($ff00+c),a	; $0018
	ld (hl-),a		; $0019
	ld a,$77		; $001a
	ld (hl),a		; $001c

	ld a,$fc		; $001d  setup bg palette
	ld ($ff00+$47),a	; $001f

	ld de,$0104		; $0021  convert and load logo data from cart into video ram
	ld hl,$8010		; $0024
addr_0027:
	ld a,(de)		; $0027
	call $0095		; $0028
	call $0096		; $002b
	inc de		; $002e
	ld a,e		; $002f
	cp $34		; $0030
	jr nz, addr_0027	; $0032

	ld de,$00d8		; $0034  load 8 additional bytes into video ram (the tile for ®)
	ld b,$08		; $0037
addr_0039:
	ld a,(de)		; $0039
	inc de		; $003a
	ld (hl+),a		; $003b
	inc hl		; $003c
	dec b			; $003d
	jr nz, addr_0039	; $003e

	ld a,$19		; $0040  setup background tilemap
	ld ($9910),a	; $0042
	ld hl,$992f		; $0045
addr_0048:
	ld c,$0c		; $0048
addr_004a:
	dec a			; $004a
	jr z, addr_0055	; $004b
	ld (hl-),a		; $004d
	dec c			; $004e
	jr nz, addr_004a	; $004f
	ld l,$0f		; $0051
	jr addr_0048	; $0053

	; === scroll logo on screen, and play logo sound===

addr_0055:
	ld h,a		; $0055  initialize scroll count, h=0
	ld a,$64		; $0056
	ld d,a		; $0058  set loop count, d=$64
	ld ($ff00+$42),a	; $0059  set vertical scroll register
	ld a,$91		; $005b
	ld ($ff00+$40),a	; $005d  turn on lcd, showing background
	inc b			; $005f  set b=1
addr_0060:
	ld e,$02		; $0060
addr_0062:
	ld c,$0c		; $0062
addr_0064:
	ld a,($ff00+$44)	; $0064  wait for screen frame
	cp $90		; $0066
	jr nz, addr_0064	; $0068
	dec c			; $006a
	jr nz, addr_0064	; $006b
	dec e			; $006d
	jr nz, addr_0062	; $006e

	ld c,$13		; $0070
	inc h			; $0072  increment scroll count
	ld a,h		; $0073
	ld e,$83		; $0074
	cp $62		; $0076  $62 counts in, play sound #1
	jr z, addr_0080	; $0078
	ld e,$c1		; $007a
	cp $64		; $007c
	jr nz, addr_0086	; $007e  $64 counts in, play sound #2
addr_0080:
	ld a,e		; $0080  play sound
	ld ($ff00+c),a	; $0081
	inc c			; $0082
	ld a,$87		; $0083
	ld ($ff00+c),a	; $0085
addr_0086:
	ld a,($ff00+$42)	; $0086
	sub b			; $0088
	ld ($ff00+$42),a	; $0089  scroll logo up if b=1
	dec d			; $008b
	jr nz, addr_0060	; $008c

	dec b			; $008e  set b=0 first time
	jr nz, addr_00e0	; $008f    ... next time, cause jump to "nintendo logo check"

	ld d,$20		; $0091  use scrolling loop to pause
	jr addr_0060	; $0093

	; ==== graphic routine ====

	ld c,a		; $0095  "double up" all the bits of the graphics data
	ld b,$04		; $0096     and store in video ram
addr_0098:
	push bc		; $0098
	rl c			; $0099
	rla			; $009b
	pop bc		; $009c
	rl c			; $009d
	rla			; $009f
	dec b			; $00a0
	jr nz, addr_0098	; $00a1
	ld (hl+),a		; $00a3
	inc hl		; $00a4
	ld (hl+),a		; $00a5
	inc hl		; $00a6
	ret			; $00a7

addr_00a8:
	;nintendo logo
	.db $ce,$ed,$66,$66,$cc,$0d,$00,$0b,$03,$73,$00,$83,$00,$0c,$00,$0d
	.db $00,$08,$11,$1f,$88,$89,$00,$0e,$dc,$cc,$6e,$e6,$dd,$dd,$d9,$99
	.db $bb,$bb,$67,$63,$6e,$0e,$ec,$cc,$dd,$dc,$99,$9f,$bb,$b9,$33,$3e

addr_00d8:
	;more video data (the tile data for ®)
	.db $3c,$42,$b9,$a5,$b9,$a5,$42,$3c

	; ===== nintendo logo comparison routine =====

addr_00e0:
	ld hl,$0104		; $00e0	; point hl to nintendo logo in cart
	ld de,$00a8		; $00e3	; point de to nintendo logo in dmg rom

addr_00e6:
	ld a,(de)		; $00e6
	inc de		; $00e7
	cp (hl)		; $00e8	;compare logo data in cart to dmg rom
	jr nz,$fe		; $00e9	;if not a match, lock up here
	inc hl		; $00eb
	ld a,l		; $00ec
	cp $34		; $00ed	;do this for $30 bytes
	jr nz, addr_00e6	; $00ef

	ld b,$19		; $00f1
	ld a,b		; $00f3
addr_00f4:
	add (hl)		; $00f4
	inc hl		; $00f5
	dec b			; $00f6
	jr nz, addr_00f4	; $00f7
	add (hl)		; $00f9
	jr nz,$fe		; $00fa	; if $19 + bytes from $0134-$014d  don't add to $00
						;  ... lock up

	ld a,$01		; $00fc
	ld ($ff00+$50),a	; $00fe	;turn off dmg rom