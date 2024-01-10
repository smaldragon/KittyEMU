.cpu 65c02

.var char_timer $10
.var line_buffer   $0200
.var char_cur      $11
.var line_buffer_i $12
.var line_buffer_l $13


.var keyboard_cache $14
.var line_cur       $20

.org [$8000]
_nmi
_reset

# Mute Audio
ldx 7
_muteloop
    stz [$70F0+X]
dec X; bpl (muteloop)

ldx $00
__clrscreen
    stz [$0200+X]
    lda ' '
    sta [$6C00+X]; sta [$6D00+X]; sta [$6E00+X]; sta [$6F00+X]
    lda $F0
    sta [$6800+X]; sta [$6900+X]; sta [$6A00+X]; sta [$6B00+X]
inc X; bne (clrscreen)

ldx $00
__printheader
    lda [tHeader+X]; beq (break)
    sta [$6C00+X]
    
    lda [tHeaderColor+X]
    sta [$6800+X]
    
    inc X; bra (printheader)
___break

stz <line_buffer_i>
ldx 4
__keyzero
    stz <keyboard_cache+X>
dec X; bpl (keyzero)
_fim
cli
bra (fim)

_irq

lda <line_buffer_i>
lda $E0; sta <line_cur>
lda $6F; sta <line_cur+1>

sei
stz [char_cur]
lda <line_buffer_i>; sta <line_buffer_l>
ldx 4
_keyloop
    psh X
    txa; asl A; asl A; asl A; asl A; tax
    lda [$7000+X]
    pul X
    
    psh A           # PUSH 01
    cmp [keyboard_cache+X]; bne (change)
    jmp [nochange]
    _change
        
    __bit7
    asl [keyboard_cache+X]; bcs (bit6)
    bit %1000_0000; beq (bit6)
    psh A; lda [kKeys7+X]; sta [char_cur]; pul A
    __bit6
    asl [keyboard_cache+X]; bcs (bit5)
    bit %0100_0000; beq (bit5)
    psh A; lda [kKeys6+X]; sta [char_cur]; pul A
    __bit5
    asl [keyboard_cache+X]; bcs (bit4)
    bit %0010_0000; beq (bit4)
    psh A; lda [kKeys5+X]; sta [char_cur]; pul A
    __bit4
    asl [keyboard_cache+X]; bcs (bit3)
    bit %0001_0000; beq (bit3)
    psh A; lda [kKeys4+X]; sta [char_cur]; pul A
    __bit3
    asl [keyboard_cache+X]; bcs (bit2)
    bit %0000_1000; beq (bit2)
    psh A; lda [kKeys3+X]; sta [char_cur]; pul A
    __bit2
    asl [keyboard_cache+X]; bcs (bit1)
    bit %0000_0100; beq (bit1)
    psh A; lda [kKeys2+X]; sta [char_cur]; pul A
    __bit1
    asl [keyboard_cache+X]; bcs (bit0)
    bit %0000_0010; beq (bit0)
    psh A; lda [kKeys1+X]; sta [char_cur]; pul A
    __bit0
    asl [keyboard_cache+X]; bcs (bitend)
    bit %0000_0001; beq (bitend)
    psh A; lda [kKeys0+X]; sta [char_cur]; pul A
    __bitend
    
    _nochange
    pul A # PULL 01
    sta [keyboard_cache+X]
dec X; bmi (keyend); jmp [keyloop]
__keyend


_write
lda <char_cur>; bne (writing); jmp [break]
__writing
bmi (specialchar)


sta <$00>   # char to write
sec; sbc 32; tax
__normalchar
lda $01; bit <keyboard_cache+2>; beq (noshift)
lda [kShift+X]; sta <$00>
bra (modend)
___noshift
lda $01; bit <keyboard_cache+1>; beq (modend)
lda [kAlt+X]; sta <$00>
bra (modend)

__modend
lda 30; cmp <line_buffer_i>; bpl (next)
jmp [break]
___next
lda <$00>
ldx <line_buffer_i>; sta [line_buffer+X]; psh X; pul Y; sta [<line_cur>+Y]
inc <line_buffer_i>
bra (break)

__specialchar
___testback
cmp $80; bne (testenter); lda <line_buffer_i>; beq (testenter)

ldx <line_buffer_i>
lda ' '; psh X; pul Y; sta [<line_cur>+Y]
stz [line_buffer+X];
dec X; stx <line_buffer_i>
stz [line_buffer+X]; lda ' '; psh X; pul Y; sta [<line_cur>+Y]
bra (break)
___testenter
cmp $81; bne (testescape)
jsr [Run]; bra (break)
___testescape
cmp $82; bne (testarrow)
jsr [Clear]; bra (break)

___testarrow
cmp $F0; bmi (break)
____nobreak
tax; lda $01; bit <keyboard_cache+1>; beq (arrowmove)
txa; sta <$00>
jmp [modend]
____arrowmove
txa
____left
cmp $F0; bne (up)
ldy <line_buffer_i>; beq (break)
dec <line_buffer_i>; bra (break)
____up
cmp $F1; bne (right)
stz <line_buffer_i>; bra (break)
____right
cmp $F2; bne (bottom)
ldy <line_buffer_i>
lda [line_buffer+Y]; beq (break)
inc <line_buffer_i>; bra (break)
____bottom
#ldy <line_buffer_i>; lda [line_buffer+Y]; sta [<line_cur>+Y]
_____loop
    lda [line_buffer+Y]; beq (over)
    inc Y; bra (loop)
_____over
sty <line_buffer_i>
__break

# Blinking: first return last cursor pos to normal color
# $00/01 → pointer to color of cursor
ldy <line_buffer_l>
lda <line_cur>; sta <$00>
lda <line_cur+1>; sec; sbc $04; sta <$01>
lda $F0; sta [<$00>+Y]

# Blinking: clear timer if a char was input
lda <char_cur>; beq (blink)
stz <char_timer>
_blink
ldx $0F
lda <char_timer>; bit %0001_0000; beq (next)
ldx $F0
__next
ldy <line_buffer_i>
lda <line_cur>; sta <$00>
lda <line_cur+1>; sec; sbc $04; sta <$01>
txa; sta [<$00>+Y]

inc <char_timer>

rti

_Run
    ldy $00
    jsr [read]
    
    __interpret
    # The Vector goes into <$08,$09>
    lda <$00>; jsr [TextToHex]; bmi (BAD)
    asl A; asl A; asl A; asl A
    sta <$09>
    lda <$01>; jsr [TextToHex]; bmi (BAD)
    ora <$09>; sta <$09>
    
    lda <$02>; cmp $F2; beq (poke); jsr [TextToHex]; bmi (BAD)
    asl A; asl A; asl A; asl A
    sta <$08>
    lda <$03>; jsr [TextToHex]; bmi (BAD)
    ora <$08>; sta <$08>
    
    lda <$04>; cmp $F2; bne (BAD)
    jsr [peek]
    jmp [cmdend]
    
    __poke
    lda <$09>; sta <$10>
    ldy 3; jsr [read]
    
    lda <$00>; jsr [TextToHex]; bmi (BAD)
    asl A; asl A; asl A; asl A
    sta <$09>
    lda <$01>; jsr [TextToHex]; bmi (BAD)
    ora <$09>; sta <$09>
    
    lda <$02>; jsr [TextToHex]; bmi (BAD)
    asl A; asl A; asl A; asl A
    sta <$08>
    lda <$03>; jsr [TextToHex]; bmi (BAD)
    ora <$08>; sta <$08>
    
    lda <$10>; sta [<$08>]
    jmp [cmdend]
    
    __BAD
    #jsr [Clear]
rts
    __read
        ldx $00
        ___loop
            lda [line_buffer+Y]; beq (break)
            sta <$00+X>; inc X

            cmp $80; bpl (break)
            inc Y
        bra (loop)
        ___break
        stz <$00+X>
    rts
    __peek
        lda [<$08>]; jsr [HexToText]
        lda <$00>; sta [$6FFE]
        lda <$01>; sta [$6FFF]
    rts
    __cmdend
    lda $00; sta <$00>
    lda $6C; sta <$01>
    
    lda $E0; sta <$02>
    lda $6B; sta <$03>
    
    ldx 4
    ldy $40
    ___moveloop
        lda [<$00>+Y]
        sta [<$02>+Y]
    inc Y; bne (moveloop)
    ldy $00
    inc <$03>; inc <$01>; lda <$01>; dec X; bne (moveloop)
    
    ldx $E0
    lda ' '
    ___spaceloop
        sta [$6F00+X]
    inc X; bne (spaceloop)
    jsr [Clear]
    rts
_HexToText
# Converts a value in A into a Hex String at <$00-$01>
    sta <$10>
    and $F0; lsr A; lsr A; lsr A; lsr A
    tax; lda [tHex+X]; sta <$00>
    lda <$10>
    and $0F
    tax; lda [tHex+X]; sta <$01>
    
    lda <$10>; rts
    
_TextToHex
# Converts a character in A into a number, outputs 80 if invalid
    sec; sbc $30; bmi (invalid)
    cmp $0A; bpl (letter)
    # Is A value from 0-9
    clc; adc 0
    rts
    __letter
    and %1101_1111
    sec; sbc 7; bmi (invalid)
    cmp $10; bpl (invalid)
    clc; adc 0
    rts
__invalid
    lda $80; rts 

_Clear
    ldy $00
    __loop
        lda [line_buffer+Y]; beq (break)
        lda 0; sta [line_buffer+Y]; lda ' '; sta [<line_cur>+Y]
    inc Y; bra (loop)
    ___break
    stz <line_buffer_i>
rts

# Text
_tHeader
.byte $D1
.byte 'foxmon'
.byte $D1,$D1,$D1,$D1,$D1,$D1,$D1,$D1,$00

_tHeaderColor
.byte $0F
.byte $0F,$0F,$0F,$0F,$0F,$0F
.byte $F8,$8C,$CE,$E4,$43,$39,$9B,$B0

_tHex
.byte '0123456789abcdef'

#--------------------------
# Keyboard Layout
_kKeys7
.byte 'o','p',$80,$81,$F2
_kKeys6
.byte 'i','k','l','.',$F3
_kKeys5
.byte 'u','j','m','|',$F0
_kKeys4
.byte 't','y','h','n',$F1

_kKeys3
.byte 'r','g','v','b',' '
_kKeys2
.byte 'e','s','d','f','c'
_kKeys1
.byte 'w','q','a','z','x'
_kKeys0
.byte $82,$00,$00,$00,$00

_kShift
.byte $20,$21,$22,$23,$24,$25,$26,$27,$28,$29,$2A,$2B,$2C,$2D,',',$2F
.byte $30,$31,$32,$33,$34,$35,$36,$37,$38,$39,$3A,$3B,$3C,$3D,$3E,$3F
.byte $40,$41,$42,$43,$44,$45,$46,$47,$48,$49,$4A,$4B,$4C,$4D,$4E,$4F
.byte $50,$51,$52,$53,$54,$55,$56,$57,$58,$59,$5A,$5B,$5C,$5D,$5E,$5F
.byte $60,'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O'
.byte 'P','Q','R','S','T','U','V','W','X','Y','Z',$7B,$7C,$7D,$7E,$7F
_kAlt
.byte $20,$21,$22,$23,$24,$25,$26,$27,$28,$29,$2A,$2B,$2C,$2D,$2E,$2F
.byte $30,$31,$32,$33,$34,$35,$36,$37,$38,$39,$3A,$3B,$3C,$3D,$3E,$3F
.byte $40,$41,$42,$43,$44,$45,$46,$47,$48,$49,$4A,$4B,$4C,$4D,$4E,$4F
.byte $50,$51,$52,$53,$54,$55,$56,$57,$58,$59,$5A,$5B,$5C,$5D,$5E,$5F
.byte $60,'@',';',$63,$64,'3',$66,$67,$68,'8',$6A,$6B,$6C,$6D,$6E,'9'
.byte '0','1','4',$73,'5','7',$76,'2',$78,'6',$7A,$7B,$7C,$7D,$7E,$7F

.pad [VECTORS]
.word nmi
.word reset
.word irq
# Other banks
.pad $8000*15