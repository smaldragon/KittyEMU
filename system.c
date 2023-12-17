#include <stdio.h>
# include <SDL2/SDL.h>
# include <SDL2/SDL_image.h>
#include <termios.h>
#include <unistd.h>
#include "w6502.c"

const int screen_width = 256; const int screen_height = 256;

const uint32_t tick_interval = 1000/50;
uint32_t next_time = 0;

uint8_t system_memory[0x10000];
int cur_cycle = 7;

int font[256][8][8];

uint32_t palette[16] = {
    0x000000,
    0x4C0000,
    0x684C00,
    0xFF4C00,
    0x006800,
    0x4C6800,
    0x69FF00,
    0xFFFF00,
    
    0x0000FF,
    0x4C00FF,
    0x684CFF,
    0xFF4CFF,
    0x0068FF,
    0x4C68FF,
    0x68FFFF,
    0xFFFFFF
};

uint32_t time_left(void)
{
    uint32_t now;

    now = SDL_GetTicks();
    if(next_time <= now)
        return 0;
    else
        return next_time - now;
}

int mygetch ( void ) 
{
  int ch;
  struct termios oldt, newt;
  
  tcgetattr ( STDIN_FILENO, &oldt );
  newt = oldt;
  newt.c_lflag &= ~( ICANON | ECHO );
  tcsetattr ( STDIN_FILENO, TCSANOW, &newt );
  ch = getchar();
  tcsetattr ( STDIN_FILENO, TCSANOW, &oldt );
  
  return ch;
}

int cpu_state(CPU* cpu) {
    //printf("CYCLE: %x | PC: %x INSTRUCTION: %x A: %x X: %x Y: %x STACK POINTER: %x FLAGS: %x", cpu->C, cpu->PC, cpu->I, cpu->A, cpu->X, cpu->Y, cpu->S, cpu->P);
    if (cpu->C == 1)
    printf("A:%X X:%X Y:%X P:%X SP:%X I:%X C:%X | PC:%4X \n", cpu->A, cpu->X, cpu->Y, cpu->P, cpu->S, cpu->I, cpu->C, cpu->PC);
}

int render_screen(SDL_Texture* texture) {
    int *pixels = NULL;
    int pitch;

    
    SDL_LockTexture(texture, NULL, (void **) &pixels,&pitch);

    for (int i = 0; i < screen_width*screen_height; i++) {
        int x = i%screen_width;
        int y = i/screen_width;
        
        int p = 0;
        
        int index = (x/8) + (y/8)*32;
        
        uint8_t character = system_memory[0x6C00 + index];
        uint32_t pal[2] = {
            palette[system_memory[0x6800 + index] & 0x0F],
            palette[system_memory[0x6800 + index] >> 4]
        };
        
        p = font[character][x%8][y%8];
        
        pixels[x + y*(pitch/4)] = pal[p];
    }
    SDL_UnlockTexture(texture);

}

int main(void)
{   
    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_Window* window = SDL_CreateWindow(
        "KITTY Emu",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        screen_width * 2, screen_height* 2,
        SDL_WINDOW_SHOWN
    );
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        0
    );
    SDL_Texture* system_screen = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        256,256
    ); 
    
    SDL_Surface* font_texture = IMG_Load("font.png");
    SDL_Surface* font_texture_rgb = SDL_ConvertSurfaceFormat(font_texture, SDL_PIXELFORMAT_RGBA32, 0);
    
    uint32_t* pixels = (uint32_t*)font_texture_rgb->pixels;
    printf("\n%X  ", font_texture_rgb->pitch);
    
    for (int character = 0; character < 256; character++) {
        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                font[character][x][y] = pixels[
                    x + y*(font_texture_rgb->pitch/4) + (character%16)*8 + (character/16)*(font_texture_rgb->pitch/4)*8
                ] == 0xFFFFFFFF;
            }
        }
        
    }
    
    SDL_FreeSurface(font_texture);
    SDL_FreeSurface(font_texture_rgb);
    
    FILE *fp;
    fp = fopen("prg.65x", "rb");
    fread(system_memory+0x8000, sizeof(uint8_t), 32*1024, fp);
    fclose(fp);
    
    w6502_setup();
    CPU cpu;
    cpu.C = 0; cpu.IRQ = 0; cpu.NMI = 0; cpu.RESET = 1;
    cpu.P  = 0x24;
    cpu.S  = 0xFD;
    
    ACCESS result;
    
    fp = fopen("log.txt", "r");
    
    char linebuffer[255];
    
    
    int wait = 0;
    int int_count = 0;
    int cycle_count = -1;
    
    int quit = 0;
    SDL_Event event;
    while (!quit) {
        
        if (cycle_count >= (384/2)*312 || cycle_count == -1) {
            if (!time_left()) {
            render_screen(system_screen);
            
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, system_screen, NULL, NULL);
            SDL_RenderPresent(renderer);
            SDL_UpdateWindowSurface(window);
            
            next_time = SDL_GetTicks() + tick_interval;
            
            if (cycle_count > 0) cpu.IRQ = 1;
            cycle_count = 0;
            
            while(SDL_PollEvent(&event)){
               if (event.type == SDL_QUIT){
                  quit = 1;
               }
            }
            }
        }
        else {

        if (0) {
            printf("\n%X  ", cpu.PC);
            
            cpu_state(&cpu);
            int_count+=1;
            
            fgets(linebuffer, 255, fp);
            linebuffer[4] = 0;
            
            
            char* string;
            asprintf(&string, "%04X", cpu.PC);
            
            printf("%s %s",linebuffer,string);
            
            
            if (strcmp(string,linebuffer) != 0) {
                char c = mygetch();
            }
            free(string);
        }
        
        //printf("\n\nø2 - %d\n", cur_cycle++);
        
        cpu_tick1(&cpu, &result);
        
        uint8_t operand = result.value;
        if (result.type == READ) {
            operand = system_memory[result.address];
        } else {
            system_memory[result.address] = operand;
            
        }
        
        //printf(" , %xm %x @%x", result.type, operand, result.address);
        
        
        cpu_tick2(&cpu, operand);
        
        if (0) {
            cpu_state(&cpu);
            if (result.type == WRITE) {
            printf("Wrote %X(%c) to %4X \n", operand, operand, result.address);
            }
            else {
            printf("Read %X(%c) from %4X \n", operand, operand, result.address);
            }

        }
        
        cycle_count += 1;
        }
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}