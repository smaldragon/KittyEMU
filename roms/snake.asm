#
#   A SNAKE Game thing by smal
#   (uses the capyasm compiler)

.cpu 65c02
.org [$8000]

_NMI
rti
_RESET
    sei
    # Mute sound
    stz [$70F0]; stz [$70F1]; stz [$70F2]; stz [$70F3]
    
    # Game Variables
    .var snake_index $10
    .var snake_size  $11
    stz <snake_index>; lda 5; sta <snake_size>
    .var score       $12
    stz <score>

    .var snake_timer $13
    .var snake_delay  $14
    .var snake_delayc $15
    .var snake_delayh $16
    lda 7; sta <snake_delay>
    lda $40; sta <snake_delayc>
    lda $01; sta <snake_delayh>

    .var symb_app    $FC

    lda snake_delay; sta <snake_timer>

    .var snake_curdir $16
    .var snake_newdir $17
    

    # Arrays of snake coords
    .var snake_x     $200
    .var snake_y     $300

    .var level        $1C
    stz <level>
    jsr [LoadLevel]    

    .var random       $18
    lda $F0; sta <random>

    .var gameover     $1A
    stz <gameover>
    .var apple_count  $1B

    .var fxvolume     $1D
    stz <fxvolume>
    .var fxnote       $1E
    stz <fxnote>

    ldx <snake_size>
    __draw
        dec X
        jsr [CalcSnakeAddr]

        # finally, write the symbol
        lda $89
        cpx <snake_index>; bne (next); lda $8A
        ___next
        sta [<$00>]
    txa; bne (draw)
    
    .var lastsegtimer $1F
    lda $FF; sta <lastsegtimer>
    
    cli

# End of RESET
__fim
bra (fim)

# 50 Hz game loop
_IRQ
sei
    lda <lastsegtimer>; bmi (nolastanim)
        dec <lastsegtimer>; bne (nolastanim)        
        lda <snake_index>; dec A; dec A; dec A; clc; adc <snake_size>; tax
        jsr [CalcSnakeAddr]
        lda $89; sta [<$00>]
    __nolastanim
    
    lda <gameover>; beq (notover)
        jsr [GameOver]
        cli
        rti
    
    __notover
        jsr [Random]
        dec <snake_timer>; beq (moving)
        jmp [fin]

    __moving
    # We will move
    lda <snake_delay>; sta <snake_timer>
    cmp 3; beq (nospeedup); dec <snake_delayc>; bne (nospeedup); dec <snake_delayh>; bpl (nospeedup)
        dec <snake_delay>; lda $20; sta <snake_delayc>; lda 0; sta <snake_delayh>
    __nospeedup
    #-----
    # Update current first and last tile of snake
    ldx <snake_index>
    jsr [CalcSnakeAddr]
    lda $89; sta [<$00>]

    lda <snake_index>; clc; adc <snake_size>; dec A; tax
    jsr [CalcSnakeAddr]
    lda ' '; sta [<$00>]
    
    #-----
    # Update the new first tile of snake
    ldy <snake_index>
    dec <snake_index>
    ldx <snake_index>
    
    #-----
    # Move snake according to DIR
    
    lda <snake_newdir>; sta <snake_curdir>
    beq (right)
    cmp 1; beq (down)
    cmp 2; beq (left)
    
    __up
    lda [snake_x + Y]; sta [snake_x + X] 
    lda [snake_y + Y]; dec A; and %000_11111; sta [snake_y + X]
    bra (next)
    
    __down
    lda [snake_x + Y]; sta [snake_x + X] 
    lda [snake_y + Y]; inc A; and %000_11111; sta [snake_y + X]
    bra (next)
    
    __right
    lda [snake_x + Y]; inc A; and %000_11111; sta [snake_x + X] 
    lda [snake_y + Y]; sta [snake_y + X]
    bra (next)
    
    __left
    lda [snake_x + Y]; dec A; and %000_11111; sta [snake_x + X] 
    lda [snake_y + Y]; sta [snake_y + X]
    
    __next
    ldx <snake_index>
    jsr [CalcSnakeAddr]
    lda [<$00>]; tay; lda $8A; sta [<$00>]; tya
    cmp symb_app; beq (eat); cmp ' '; beq (noeat)
    lda 1; sta <gameover>
    __eat
    lda <snake_index>; clc; adc <snake_size>; dec A; tax
    jsr [CalcSnakeAddr]
    lda $A3; sta [<$00>]
    lda 4; sta <lastsegtimer>
    
    lda $FF; sta <fxvolume>; dec <fxnote>
    inc <snake_size>; lda $80; cmp <snake_size>; bpl (notnewlevel); jsr [LoadLevel]; bra (fin) 
    __notnewlevel
    jsr [DoubleLast]
    dec <apple_count>; bne (noeat)
    jsr [GenApples]
    __noeat


_fin
    ldx <snake_newdir>
    lda [$7040]; tay # Keyboard Input
    cmp $00; beq (invalid); jsr [Random]
    tya
    
    __right
    cmp %1000_0000; bne (down); ldx 0
    __down
    cmp %0100_0000; bne (left); ldx 1
    __left
    cmp %0010_0000; bne (up); ldx 2
    __up
    cmp %0001_0000; bne (nexti); ldx 3
    __nexti

    txa; clc; adc 2; and %0000_0011; cmp <snake_curdir>; beq (invalid)
    stx <snake_newdir>
    __invalid

    __sound
    lda <fxvolume>; beq (nosound)
        ldx <fxnote>; bpl (next0); ldx 12; stx <fxnote>
        ___next0        
        cmp $FF; bne (next1)
            #lda %00_11_011_0; sta [$70E3]
            #lda [NoteTableLo+X]; sta [$70E0]
            #lda [NoteTableHi+X]; asl A; sta [$70E0]
            bra (next2)
        ___next1
        cmp $CC; bne (next2)
            #lda %00_11_011_0; sta [$70E3]
            #lda [NoteTableLo+X]; sta [$70E0]
            #lda [NoteTableHi+X]; sta [$70E0]
        ___next2
            lda <fxvolume>; sta [$70F3]
            sec; sbc $11; sta <fxvolume>; bne (nosound)
            sta [$70F0]
    __nosound
cli
rti

# Doubles current last segment of the snak
_DoubleLast
    lda <snake_size>; clc; adc <snake_index>; tax
    lda [snake_x+X]; tay; lda [snake_y+X]
    inc <snake_size>
    sta [snake_y+X]; tya; sta [snake_x+X] 
rts

_CalcSnakeAddr
    # From X reg, store address of block in <$02,$03>
    
    # Add to $6C00 X coordinate and Y coordinate (shifted 5 left)
    lda [snake_x + X]; sta <$00>
    lda $6C; sta <$01>
    
    # extract upper 2 bits of Y coord    
    lda [snake_y + X]
    lsr A; lsr A; lsr A; tay
    # extract lower 3 bits of Y coord
    lda [snake_y + X]
    asl A; asl A; asl A; asl A; asl A
    
    # add the two halves of the Y coord to each byte
    clc; adc <$00>; sta <$00>
    tya; adc <$01>; sta <$01>
rts

_GenApples
    stz <apple_count>
    lda <snake_size>
    lsr A
    lsr A
    __loop
    psh A
    jsr [Random]; jsr [Random]; jsr [Random]
    jsr [GenApple]
    pul A
    
    lsr A; bne (loop)
rts

_GenApple
    lda 128; sta <$06>   # Attempt to spawn up to 128(!) times
    __loop
    dec <$06>; beq (return)    
    jsr [Random]
    jsr [Random]
    
    lda <random>; and %000_11111; sta <$02>
    lda $6C; sta <$03>
    
    jsr [Random]
    jsr [Random]
    jsr [Random]
    
    lda <random+1>; and %000_11111; tax
    lsr A; lsr A; lsr A; tay
    txa
    asl A; asl A; asl A; asl A; asl A
    
    clc; adc <$02>; sta <$02>
    tya; adc <$03>; sta <$03>

    lda [<$02>]; cmp ' '; bne (loop)
    
    inc <apple_count>
    lda symb_app; sta [<$02>]

    __return
rts

_Random
    # LSFR "random" number generator
    lda <random>; lsr A; lsr A; lsr A; xor <random>; and 1
    ror A
    ror <random>
    lda <random+1>
    ror A
    sta <random+1>
rts

_GameOver
    ldx 0
    __textprint
    lda [text+X]
    beq (textover)
    sta [$6C00+X]
    inc X
    bra (textprint)
    __textover
rts
    __text
    .byte "Game Over :("

_LoadLevel
    sei
    # Initial Snake Positions
    stz <snake_curdir>; stz <snake_newdir>
    stz <snake_index>
    
    lda $0F

    sta [snake_y]
    sta [snake_y + 1]
    sta [snake_y + 2]
    sta [snake_y + 3]
    sta [snake_y + 4]

    lda 12; sta [snake_x]
    lda 11; sta [snake_x + 1]
    lda 10; sta [snake_x + 2]
    lda 09; sta [snake_x + 3]
    lda 08; sta [snake_x + 4]

    lda 4; sta <snake_delay>
    lda $00; sta <snake_delayc>
    lda $01; sta <snake_delayh>

    lda 5; sta <snake_size>

    inc <level>; lda <level>
    cmp 2; beq (Level2)
    
    ldx $00
    __fill
        # Palette
        lda $F5 # F5
        sta [$6800+X]; sta [$6900+X]; sta [$6A00+X]; sta [$6B00+X]
        
        # Character
        lda ' '
        sta [$6C00+X]; sta [$6D00+X]; sta [$6E00+X]; sta [$6F00+X]

    inx; bne (fill)
    jsr [GenApples]
    cli
rts
_Level2
    # Fill screen with nothing graphics
    ldx 0
    __fill
        # Palette
        lda $FA
        sta [$6800+X]; sta [$6900+X]; sta [$6A00+X]; sta [$6B00+X]
        
        # Character
        lda ' '
        sta [$6C00+X]; sta [$6D00+X]; sta [$6E00+X]; sta [$6F00+X]

    inx; bne (fill)
    
    ldx $00
    ldy $08
    __edges
        psh Y; ldy $08        
        ___loopsie
        # Fill with texture + color
        lda $A0
        sta [$6C00+X]; sta [$6C18+X]; sta [$6F00+X]; sta [$6F18+X]
        lda $FF
        sta [$6800+X]; sta [$6818+X]; sta [$6B00+X]; sta [$6B18+X]
        
        inc X
        # Looping                
        dec Y; bne (loopsie); pul Y
        txa; clc; adc 24; tax; dec Y
    bne (edges)
    jsr [GenApples]
    cli
rts

_NoteTableLo
.byte $65,$BA,$23,$A2,$37,$E5,$AC,$8E,$8D,$AA,$E7,$47,$CB
_NoteTableHi
.byte $16,$17,$19,$1A,$1C,$1D,$1F,$21,$23,$25,$27,$2A,$2C

.pad [VECTORS]
.word NMI
.word RESET
.word IRQ

# Fill up the remaining 15 banks on a 512Kb chip
.pad $8000 * 15