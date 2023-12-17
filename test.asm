# This test ROM was written for the CapyAsm assembler (smal's weird wip thing)
# But you can use any assembler you want.
.cpu 65c02

.var CHR $6C00
.var PAL $6800

.var KEY_1    $7000
.var KEY_2    $7010
.var KEY_3    $7020
.var KEY_4    $7030
.var KEY_5    $7040

.var OSC_0    $70E0
.var OSC_1    $70E1
.var OSC_2    $70E2
.var OSC_CTRL $70E3

.org [$8000]
_reset

cld
sei

# Mute Audio
stz [$70F0]
stz [$70F1]
stz [$70F2]
stz [$70F3]

# Make the square waves ring :)
ldx 60 #C4
lda %00_11_011_0; sta [OSC_CTRL]; lda [note_lo+X]; sta [OSC_0]; lda [note_hi+X]; sta [OSC_0];

ldx 64 #E4
lda %01_11_011_0; sta [OSC_CTRL]; lda [note_lo+X]; sta [OSC_1]; lda [note_hi+X]; sta [OSC_1];

ldx 67 #G4
lda %10_11_011_0; sta [OSC_CTRL]; lda [note_lo+X]; sta [OSC_2]; lda [note_hi+X]; sta [OSC_2];

# Fill up the screen with data
lda ' ' # character
ldx 0
__fill_chr
    # Character Memory
    sta [CHR+$0000+X]
    sta [CHR+$0100+X]
    sta [CHR+$0200+X]
    sta [CHR+$0300+X]
inx; bne (fill_chr)

lda $F0
ldx 0
__fill_pal
    # Palette Memory
    sta [PAL+$0000+X]
    sta [PAL+$0100+X]
    sta [PAL+$0200+X]
    sta [PAL+$0300+X]
inx; bne (fill_pal)

lda $F8; sta <$00>
stz <$01>
stz <$10>




_test1
ldx 0
__loop
    lda [text02+X]; beq (end)
    sta [CHR+64+X]
    inc X
    bra (loop)
__end
_test2
ldx 0
__loop
    lda [textload+X]; beq (end)
    sta [CHR+320+2+X]
    inc X
    bra (loop)
__end

# 89AB CDEF

# Print Palette to Screen
lda $00; sta [$6AA8]
lda $11; sta [$6AA9]
lda $22; sta [$6AAA]
lda $33; sta [$6AAB]
lda $44; sta [$6AAC]
lda $55; sta [$6AAD]
lda $66; sta [$6AAE]
lda $77; sta [$6AAF]

lda $88; sta [$6AB0]
lda $99; sta [$6AB1]
lda $AA; sta [$6AB2]
lda $BB; sta [$6AB3]
lda $CC; sta [$6AB4]
lda $DD; sta [$6AB5]
lda $EE; sta [$6AB6]
lda $FF; sta [$6AB7]

ldx $00
__display_font
# Character Memory
txa; sta [$6F00+X]
inx; bne (display_font)

cli
__fim
bra (fim)

#---------------------------------------------------------
# INTERUPT REQUEST
_irq
sei

dec <$01>; bne (next)
    inc <$00>
    lda 50; sta <$01>
__next

ldx 0
__loop
    lda [text01+X]; beq (end)
    sta [$6C00+X]
    lda <$00>; sta [$6800+X]
    inc X
    bra (loop)
__end

# Printing Keyboard to the screen
lda <$6E>; sta <$21>

lda $0C; sta <$20>
lda [KEY_1]
jsr [keyprint]

lda $2C; sta <$20>
lda [KEY_2]
jsr [keyprint]

lda $4C; sta <$20>
lda [KEY_3]
jsr [keyprint]

lda $6C; sta <$20>
lda [KEY_4]
jsr [keyprint]

lda $8C; sta <$20>
lda [KEY_5]
jsr [keyprint]


inc <$10>; lda <$10>; 
sta [$70F0]
sta [$70F1]
sta [$70F2]

cli
rti

_text01
.byte " Hello! Welcome to my Computer! "
_text02
.byte "Running @3Mhz with custom video + sound! "
_textload
.byte "Drag & Drop a 32k ROM to run"

_keyprint
    ldy 8
    __loop
        ldx $FD;
        asl A; bcc (nopress); ldx $FC
        __nopress
        psh A
        txa; ldx <$20>; sta [$6E00+X]; inc <$20>
        pul A
    dey; bne (loop)
rts

_note_lo
.byte $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00
.byte $00 $00 $62 $29 $BD $11 $1C $D2 $2B $1D $9F $A9 $35 $39 $B1 $95
.byte $DE $89 $8E $E9 $96 $8E $CF $55 $1A $1D $58 $CA $6F $44 $47 $75
.byte $CB $47 $E8 $AA $8D $8E $AC $E5 $38 $A2 $24 $BA $65 $24 $F4 $D5
.byte $C7 $C7 $D6 $F3 $1C $51 $92 $DD $33 $92 $FA $6B $E3 $64 $EB $79
.byte $0E $A9 $49 $EF $99 $49 $FD $B5 $72 $32 $F6 $BD $87 $54 $24 $F7
.byte $CD $A4 $7E $5B $39 $19 $FB $DE $C3 $AA $92 $7C $66 $52 $3F $2D
.byte $1C $0C $FD $EF $E2 $D5 $C9 $BE $B3 $A9 $A0 $97 $8E $86 $7F $78

_note_hi
.byte $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00
.byte $00 $00 $FD $EF $E1 $D5 $C9 $BD $B3 $A9 $9F $96 $8E $86 $7E $77
.byte $70 $6A $64 $5E $59 $54 $4F $4B $47 $43 $3F $3B $38 $35 $32 $2F
.byte $2C $2A $27 $25 $23 $21 $1F $1D $1C $1A $19 $17 $16 $15 $13 $12
.byte $11 $10 $0F $0E $0E $0D $0C $0B $0B $0A $09 $09 $08 $08 $07 $07
.byte $07 $06 $06 $05 $05 $05 $04 $04 $04 $04 $03 $03 $03 $03 $03 $02
.byte $02 $02 $02 $02 $02 $02 $01 $01 $01 $01 $01 $01 $01 $01 $01 $01
.byte $01 $01 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00


#
#   END OF CODE
#
.pad [VECTORS]
.word reset
.word reset
.word irq


# The Other banks
.pad $8000
.pad $8000
.pad $8000