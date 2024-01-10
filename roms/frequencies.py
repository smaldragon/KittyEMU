master = 3000000/8

def midi_freq( n ):
    return 440*(2**((n-69)/12))

notes = [0] * 128
cur_midi = len(notes)-1

prev_freq = master
for i in range(1,0xFFFF):
    freq = master/i
    
    midi = midi_freq( cur_midi )
    
    if freq < midi:
        if prev_freq - midi > midi - freq:
            notes[cur_midi] = i
        else:
            notes[cur_midi] = i-1
        cur_midi -= 1
        if cur_midi < 0:
            break
            
    prev_freq = freq
    
for i,n in enumerate(notes):
    print(i,n,midi_freq(i))
    
with open("frequencies.asm", "w") as f:
    f.write("_note_lo\n")
    for i in range(0,128,16):
        line = ".byte"
        for u in range(16):
            value = "{:02X}".format(notes[i+u] & 0x00FF)
            line += f" ${value}"
        
        line += "\n"
        f.write(line)
    
    f.write("\n_note_hi\n")
    for i in range(0,128,16):
        line = ".byte"
        for u in range(16):
            value = "{:02X}".format( (notes[i+u] & 0xFF00) >> 8)
            line += f" ${value}"
            
        line += "\n"
        f.write(line)
    
# 0x0b32
# 0x08e3
# 0x0779

