	ld sp,$fff       ; $0000  setup stack

	xor              ; $0003  zero the memory from $8000-$9fff (vram)
	ld hl,$9ff       ; $0004
addr_0007:
	ld (hl-),        ; $0007
	bit 7,           ; $0008
	jr nz, addr_000  ; $000a

	ld hl,$ff2       ; $000c  setup audio
	ld c,$1          ; $000f
	ld a,$8          ; $0011
	ld (hl-),        ; $0013
	ld ($ff00+c),    ; $0014
	inc              ; $0015
	ld a,$f          ; $0016
	ld ($ff00+c),    ; $0018
	ld (hl-),        ; $0019
	ld a,$7          ; $001a
	ld (hl),         ; $001c

	ld a,$f          ; $001d  setup bg palette
	ld ($ff00+$47),  ; $001f

	ld de,$010       ; $0021  convert and load logo data from cart into video ram
	ld hl,$801       ; $0024
addr_0027:
	ld a,(de         ; $0027
	call $009        ; $0028
	call $009        ; $002b
	inc d            ; $002e
	ld a,            ; $002f
	cp $3            ; $0030
	jr nz, addr_002  ; $0032

	ld de,$00d       ; $0034  load 8 additional bytes into video ram (the tile for ®)
	ld b,$0          ; $0037
addr_0039:
	ld a,(de         ; $0039
	inc d            ; $003a
	ld (hl+),        ; $003b
	inc h            ; $003c
	dec              ; $003d
	jr nz, addr_003  ; $003e

	ld a,$1          ; $0040  setup background tilemap
	ld ($9910),      ; $0042
	ld hl,$992       ; $0045
addr_0048:
	ld c,$0          ; $0048
addr_004a:
	dec              ; $004a
	jr z, addr_005   ; $004b
	ld (hl-),        ; $004d
	dec              ; $004e
	jr nz, addr_004  ; $004f
	ld l,$0          ; $0051
	jr addr_004      ; $0053

	; === scroll logo on screen, and play logo sound===

addr_0055:
	ld h,            ; $0055  initialize scroll count, h=0
	ld a,$6          ; $0056
	ld d,            ; $0058  set loop count, d=$64
	ld ($ff00+$42),  ; $0059  set vertical scroll register
	ld a,$9          ; $005b
	ld ($ff00+$40),  ; $005d  turn on lcd, showing background
	inc              ; $005f  set b=1
addr_0060:
	ld e,$0          ; $0060
addr_0062:
	ld c,$0          ; $0062
addr_0064:
	ld a,($ff00+$44  ; $0064  wait for screen frame
	cp $9            ; $0066
	jr nz, addr_006  ; $0068
	dec              ; $006a
	jr nz, addr_006  ; $006b
	dec              ; $006d
	jr nz, addr_006  ; $006e

	ld c,$1          ; $0070
	inc              ; $0072  increment scroll count
	ld a,            ; $0073
	ld e,$8          ; $0074
	cp $6            ; $0076  $62 counts in, play sound #1
	jr z, addr_008   ; $0078
	ld e,$c          ; $007a
	cp $6            ; $007c
	jr nz, addr_008  ; $007e  $64 counts in, play sound #2
addr_0080:
	ld a,            ; $0080  play sound
	ld ($ff00+c),    ; $0081
	inc              ; $0082
	ld a,$8          ; $0083
	ld ($ff00+c),    ; $0085
addr_0086:
	ld a,($ff00+$42  ; $0086
	sub              ; $0088
	ld ($ff00+$42),  ; $0089  scroll logo up if b=1
	dec              ; $008b
	jr nz, addr_006  ; $008c

	dec              ; $008e  set b=0 first time
	jr nz, addr_00e  ; $008f    ... next time, cause jump to "nintendo logo check"

	ld d,$2          ; $0091  use scrolling loop to pause
	jr addr_006      ; $0093

	; ==== graphic routine ====

	ld c,            ; $0095  "double up" all the bits of the graphics data
	ld b,$0          ; $0096     and store in video ram
addr_0098:
	push b           ; $0098
	rl               ; $0099
	rl               ; $009b
	pop b            ; $009c
	rl               ; $009d
	rl               ; $009f
	dec              ; $00a0
	jr nz, addr_009  ; $00a1
	ld (hl+),        ; $00a3
	inc h            ; $00a4
	ld (hl+),        ; $00a5
	inc h            ; $00a6
	re               ; $00a7

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
	ld hl,$010       ; $00e      ; point hl to nintendo logo in cart
	ld de,$00a       ; $00e      ; point de to nintendo logo in dmg rom

addr_00e6:
	ld a,(de         ; $00e6
	inc d            ; $00e7
	cp (hl           ; $00e      ;compare logo data in cart to dmg rom
	jr nz,$f         ; $00e      ;if not a match, lock up here
	inc h            ; $00eb
	ld a,            ; $00ec
	cp $3            ; $00e      ;do this for $30 bytes
	jr nz, addr_00e  ; $00ef

	ld b,$1          ; $00f1
	ld a,            ; $00f3
addr_00f4:
	add (hl          ; $00f4
	inc h            ; $00f5
	dec              ; $00f6
	jr nz, addr_00f  ; $00f7
	add (hl          ; $00f9
	jr nz,$f         ; $00f      ; if $19 + bytes from $0134-$014d  don't add to $00
	;  ... lock up

	ld a,$0          ; $00fc
	ld ($ff00+$50),  ; $00f      ;turn off dmg rom