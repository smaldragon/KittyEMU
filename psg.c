typedef struct psg {
    uint8_t waves[4];
    uint8_t volumes[4];
    // 82c54
    uint16_t timers[3];
    uint8_t control;
    
    uint8_t cycle = 0;
} PSG;

uint8_t tick_wave(uint8_t wave) {
    int lowest_bit = wave & 0x01;
    wave = (wave >> 1) + (lowest_bit << 7);
    return wave;
}

uint8_t psg_access(PSG *psg, ACCESS *result) {
    operand = result->value;
    
    uint8_t address = (result->address) & 0x1F;
    switch (address >> 4) {
        case 0:
            uint8_t sub_address = address & 0x03;
            // 82c54 (frequency)
            if (result->TYPE == WRITE) {
            
            } else {
            
            }
            break;
        case 1:
            // Volume and Wave
            if (result->TYPE == WRITE) {
                uint8_t sub_address = address & 0x07;
                if (sub_address < 4) {
                    psg->volume[sub_address] = operand;
                } else {
                    sub_address -= 4;
                    psg->waves[sub_address] = operand;
                }
            }
            break;
    }
    
    return operand;
}

uint8_t psg_getsample(PSG *psg) {
    uint8_t value = psg->volumes[psg->cycle];
    if (! (psg->waves[psg->cycle] & 0x01)) {
        value = 0;
    }
    psg->cycle = (psg->cycle + 1)%4;
    return value;
}