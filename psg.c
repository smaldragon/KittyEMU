typedef struct psg {
    uint8_t cycle;
    uint8_t wave[4];
    uint8_t volume[4];
    
    // 82c54
    uint8_t timer_control[3];
    uint16_t timer_reload[3];
    uint16_t timer_count[3];
    uint8_t timer_latch[3];
    uint8_t timer_state[3];
    
    uint8_t timer_output[3];
} PSG;


uint8_t psg_tick_wave(uint8_t wave) {
    int lowest_bit = wave & 0x01;
    wave = (wave >> 1) + (lowest_bit << 7);
    return wave;
}

// psg noise ticking is handled externally
uint8_t psg_tick_noise(PSG *psg) {
    psg->wave[4] = psg_tick_wave(psg->wave[4]);
}

void psg_tick_82c54(PSG *psg) {
    for (int i = 0; i < 3; i++) {
        int bdc  = psg->timer_control[i] & 0x01;
        int mode = (psg->timer_control[i] & 0x06) >> 1;
        int rw   = (psg->timer_control[i] & 0x30) >> 4;
        int out  = psg->timer_output[i];
        
        switch (mode) {
        case 0:
            psg->timer_count[i] -= 1;
            if (psg->timer_count[i] == 0) psg->timer_output[i] = 1;
        break;
        case 1:
            psg->timer_output[i] = 1;
        break;
        case 6:
        case 2:
            psg->timer_count[i] -= 1;
            if (psg->timer_count[i] == 0) {
                psg->timer_output[i] = 1;
                psg->timer_count[i] = psg->timer_reload[i];
            } else {
                psg->timer_output[i] = 0;
            }
        break;
        case 7:
        case 3:
            psg->timer_count[i] -= 1;
            if (psg->timer_count[i] == 0) {
                psg->timer_output[i] = 1;
                psg->timer_count[i] = psg->timer_reload[i];
            } else {
                psg->timer_output[i] = 0;
                if (psg->timer_count[i] < psg->timer_reload[i]/2) {
                    psg->timer_output[i] = 0;
                }
            }
        break;
        case 4:
            break;
        case 5:
            break;
        }
        if (out == 0 && psg->timer_output[i] == 1) {
            psg->wave[i] = psg_tick_wave(psg->wave[i]);
        }
    }
}

uint8_t psg_access(PSG *psg, ACCESS *result) {
    uint8_t operand = result->value;
    
    uint8_t address = (result->address) & 0x1F;
    switch (address >> 4) {
        case 0:
            uint8_t sub_address = address & 0x03;
            // 82c54 (frequency)
            if (result->type == WRITE) {
                // 
                if (sub_address == 3) {
                    uint8_t target = operand >> 6;
                    psg->timer_control[target] = operand;
                    psg->timer_state[target] = 0;
                } else {
                    int rw   = (psg->timer_control[sub_address] & 0x30) >> 4;
                    // low only
                    if (rw == 1) {
                        if (psg->timer_state[sub_address] == 0) {
                            psg->timer_state[sub_address] = 1;
                            psg->timer_reload[sub_address] = (psg->timer_reload[sub_address] & 0xFF00) + operand;
                        }
                    }
                    if (rw == 2) {
                        if (psg->timer_state[sub_address] == 0) {
                            psg->timer_state[sub_address] = 1;
                            psg->timer_reload[sub_address] = (psg->timer_reload[sub_address] & 0x00FF) + (operand<<8);
                        }
                    }
                    if (rw == 3) {
                        if (psg->timer_state[sub_address] == 0) {
                            psg->timer_state[sub_address] = 1;
                            psg->timer_reload[sub_address] = (psg->timer_reload[sub_address] & 0xFF00) + operand;
                        }
                        else if (psg->timer_state[sub_address] == 1) {
                            psg->timer_state[sub_address] = 2;
                            psg->timer_reload[sub_address] = (psg->timer_reload[sub_address] & 0x00FF) + (operand<<8);
                        }
                    }
                }
            } else {
                // implement later
            }
            break;
        case 1:
            // Volume and Wave
            if (result->type == WRITE) {
                uint8_t sub_address = address & 0x07;
                if (sub_address < 4) {
                    psg->volume[sub_address] = operand;
                } else {
                    sub_address -= 4;
                    psg->wave[sub_address] = operand;
                }
            }
            break;
    }
    
    return operand;
}

uint8_t psg_getsample(PSG *psg) {
    uint8_t value = psg->volume[psg->cycle];
    if (! (psg->wave[psg->cycle] & 0x01)) {
        value = 0;
    }
    psg->cycle = (psg->cycle + 1)%4;
    
    //printf("%i",value);
    return value;
}