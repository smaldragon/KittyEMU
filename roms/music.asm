.cpu 65c02
.org [$8000]

.var ch_note $10
.var ch_octave $14
.var ch_volume $18
.var ch_wave $1C
# 16-bit values
.var ch_wait_tick $20
.var ch_wait_note $24
.var ch_index $28
.var ch_inst  $2A
.var song_tempo $30

_nmi
    rti

_reset
    # Clear Volume
    stz [$70F0]; stz [$70F1]; stz [$70F2]; stz [$70F3]
    # Clear Waveform
    stz [$70F4]; stz [$70F5]; stz [$70F6]; stz [$70F7]

    # Clear Screen
    ldx 0
    __ClearScreen
        lda $F0
        sta [$6800+X]; sta [$6900+X]; sta [$6A00+X]; sta [$6B00+X]
        
        lda ' '
        sta [$6C00+X]; sta [$6D00+X]; sta [$6E00+X]; sta [$6F00+X]
    inc X; bne (ClearScreen)
    
    # Screen Text
    ldy 32*4
    ldx '4'
    __chtext
        lda 'C'; sta [$6C00+Y]
        lda 'h'; sta [$6C01+Y]
        txa    ; sta [$6C02+Y]
        dec X
    tya; sec; sbc 32; tay; bne (chtext)
    
    lda 'T'; sta [$6C05]; lda 'i'; sta [$6C06]; lda 'm'; sta [$6C07]; lda 'e'; sta [$6C08]
    lda 'N'; sta [$6C0B]; lda 'o'; sta [$6C0C]; lda 't'; sta [$6C0D]; lda 'e'; sta [$6C0E]
    lda 'V'; sta [$6C11]; lda 'o'; sta [$6C12]; lda 'l'; sta [$6C13]; lda '.'; sta [$6C14]
    lda 'W'; sta [$6C17]; lda 'a'; sta [$6C18]; lda 'v'; sta [$6C19]; lda 'e'; sta [$6C1A]
    
    lda $0F; sta [$6805]; sta [$6806]; sta [$6807]; sta [$6808]
    lda $0F; sta [$680B]; sta [$680C]; sta [$680D]; sta [$680E]
    lda $0F; sta [$6811]; sta [$6812]; sta [$6813]; sta [$6814]
    lda $0F; sta [$6817]; sta [$6818]; sta [$6819]; sta [$681A]
    
    # Init Music Variables
    ldx $00
    __fillzero
        stz <$00+X>
    inc X; bne (fillzero)
    
    lda 1; sta <ch_wait_tick+0>; sta <ch_wait_tick+1>; sta <ch_wait_tick+2>; sta <ch_wait_tick+3>
    
    lda Track01.lo; sta <ch_index+0>; lda Track01.hi; sta <ch_index+1>
    lda Track02.lo; sta <ch_index+2>; lda Track02.hi; sta <ch_index+3>
    lda Track03.lo; sta <ch_index+4>; lda Track03.hi; sta <ch_index+5>
    lda Track04.lo; sta <ch_index+6>; lda Track04.hi; sta <ch_index+7>
    
    lda 2; sta <song_tempo>
    
    sei
_Main
    ldx $00; jsr [ProcessCh]
    ldx $01; jsr [ProcessCh]
    ldx $02; jsr [ProcessCh]
    ldx $03; jsr [ProcessCh]
    
    # Display the registers
    ldx $00; jsr [ShowRegs]
    ldx $01; jsr [ShowRegs]
    ldx $02; jsr [ShowRegs]
    ldx $03; jsr [ShowRegs]
    
    wai
jmp [Main]

_ShowRegs
    txa; asl A; asl A; asl A; asl A; asl A; tay
    
    lda <ch_wait_tick+X>; jsr [HexToText]
    lda <$00>; sta [$6C25+Y]; lda <$01>; sta [$6C26+Y]
    lda <ch_wait_note+X>; jsr [HexToText]
    lda <$00>; sta [$6C27+Y]; lda <$01>; sta [$6C28+Y]

    lda <ch_note+X>; jsr [HexToText]
    lda <$00>; sta [$6C2C+Y]; lda <$01>; sta [$6C2D+Y]
    
    lda <ch_volume+X>; jsr [HexToText]
    lda <$00>; sta [$6C32+Y]; lda <$01>; sta [$6C33+Y]
    
    #lda <ch_wave+X>; jsr [HexToText]
    #lda <$00>; sta [$6C38+Y]; lda <$01>; sta [$6C39+Y]
    lda <ch_wave+X>; sta <$02>
    ldx 8
    lda $37; sta <$00>; lda $6C; sta <$01>
    __showwave
    lda '_'
    rol <$02>; bcc (low)
    lda $03
    ___low
    sta [<$00>+Y]
    inc <$00>
    dec X; bne (showwave)
rts

_HexToText
    # Stores in $00 and $01
    __LO
    psh A
    and $0F;
        cmp 10; bmi (number)
        clc; adc $37
        bra (HI)
        ___number
        clc; adc $30
        #bra (HI)
    __HI
    sta <$01>
    pul A; psh A
    lsr A; lsr A; lsr A; lsr A
        cmp 10; bmi (number)
        clc; adc $37
        bra (END)
        ___number
        clc; adc $30
    __END
    sta <$00>
    pul A
rts

_ProcessCh
    dec <ch_wait_tick+X>; bne (next)
        lda <song_tempo>; sta <ch_wait_tick+X>
    lda <ch_wait_note+X>; dec <ch_wait_note+X>; beq (run)
    __next
    rts
    
    __run
        jsr [GetMusOpcode]
        
        cmp xO; bcc (note)
        cmp xV; bcc (octave)
        cmp $F0; bcc (volume)
    #--------------------
    # FUNCTIONS
    __functions
        # Sets the Waveform
        cmp xW; beq (waveform)
        cmp xT; beq (tickset)
        cmp xI; beq (instrument)
    __end
        psh X
        txa; asl A; tax
        lda [TrackStart+X]; sta <ch_index+X>; lda [TrackStart+1]; sta <ch_index+1+X>
        pul X
        jmp [run]
    __tickset
        jsr [GetMusOpcode]
        sta <ch_wait_tick+X>
        
        jmp [run]
    __waveform
        jsr [GetMusOpcode]
        sta <ch_wave+X>; sta [$70F4+X]
        jmp [run]
    __instrument
        jsr [GetMusOpcode]
        sta <ch_inst+X>
        jmp [run]
    #--------------------
    # Main Functions
    __octave
        and $0F; cmp %0000_1000; beq (decrement); bcs (increment)
        inc A; sta <ch_octave+X>
        jmp [run]
        ___decrement
        dec <ch_octave+X>
        jmp [run]
        ___increment
        inc <ch_octave+X>
        jmp [run]
    __volume
        and $0F; sta <ch_volume+X>
        asl A; asl A; asl A; asl A
        ora <ch_volume+X>
        sta [$70F0+X]
        jmp [run]
    __note
        psh A; psh A
        # Set the note
        lda <ch_octave+X>; tay
        pul A; lsr A; lsr A; lsr A; lsr A; beq (timeset)
        ___octaveinc
            clc; adc 12
        dec Y; bne (octaveinc)
        dec A; sta [ch_note+X]
        
        tay
        lda [CtrlWords+X]; sta [$70E3]
        lda [note_lo+Y]; sta [$70E0+X]
        lda [note_hi+Y]; sta [$70E0+X]
        
        ___timeset
        # Set the note wait time
        pul A
        and $0F; tay
        lda [delays+Y]; sta <ch_wait_note+X>
    rts

_GetMusOpcode
 psh X
    txa; asl A; tax
    lda [<ch_index+X>]
    inc <ch_index+X>; bne (noinc); inc <ch_index+1+X>
__noinc
pul X
rts

_delays
.byte 0, 1
.byte 2, 3
.byte 4, 6
.byte 8, 12
.byte 16, 24
.byte 32, 48
.byte 64, 96
.byte 128, 192

.var nR   %0000_0000
.var nC   %0001_0000
.var nCC  %0010_0000
.var nD   %0011_0000
.var nDD  %0100_0000
.var nE   %0101_0000
.var nF   %0110_0000
.var nFF  %0111_0000
.var nG   %1000_0000
.var nGG  %1001_0000
.var nA   %1010_0000
.var nAA  %1011_0000
.var nB   %1100_0000
.var xO   %1101_0000
.var xV   %1110_0000
.var xW   %1111_0000
.var xT   %1111_0001
.var xI   %1111_0010
.var xEND %1111_1111

_Track01

.byte xW, $F0, xO+5
.byte xV+0, nCC+2, xV+1, nR+2, xV+2, nR+2, xV+3, nR+2, xV+4, nR+2, xV+5, nR+2, xV+6, nR+2, xV+7, nR+2
.byte xV+8, nR+2, xV+9, nR+2, xV+10, nR+2, xV+11, nR+2, xV+12, nR+2, xV+13, nR+2, xV+14, nR+2, xV+15, nR+2
.byte xW, %1011_0000, xO+5
.byte xV+0, nCC+2, xV+1, nR+2, xV+2, nR+2, xV+3, nR+2, xV+4, nR+2, xV+5, nR+2, xV+6, nR+2, xV+7, nR+2
.byte xV+8, nR+2, xV+9, nR+2, xV+10, nR+2, xV+11, nR+2, xV+12, nR+2, xV+13, nR+2, xV+14, nR+2, xV+15, nR+2
.byte xEND

#.byte xW, $F0, xO+5
#.byte xV+15, nCC+1 xV+14, nR+1, xV+13, nR+1, xV+12, nR+1
#.byte xV+15, nDD+1 xV+14, nR+1, xV+13, nR+1, xV+12, nR+1
#.byte xV+15, nFF+1 xV+14, nR+1, xV+13, nR+1, xV+12, nR+1, nR+4, nR+10
#.byte xV+15, nF+1 xV+14, nR+1, xV+13, nR+1, xV+12, nR+1
#.byte xV+15, nDD+1 xV+14, nR+1, xV+13, nR+1, xV+12, nR+1
#.byte xV+15, nCC+1 xV+14, nR+1, xV+13, nR+1, xV+12, nR+1
#.byte xV+15, xO+8, nGG+6, nAA+4

#.byte xV+12, nR+10, nR+6, nFF+4, nGG+4, nAA+4, xO+9, nCC+6, xO+8, nGG+4


_Track02
.byte nR+15,xEND

_Track03
.byte nR+15,xEND

_Track04
.byte nR+15,xEND

_TrackStart
.word Track01
.word Track02
.word Track03
.word Track04

.asm frequencies
_CtrlWords
.byte $34 + %0000_0000
.byte $34 + %0100_0000
.byte $34 + %1000_0000

.pad [VECTORS]
.word nmi
.word reset
.word Main

.pad $8000*15