set PATH=C:\Program Files (x86)\Dev-Cpp\MinGW64\bin
gcc -m32 -o EmuBoy apu.c cpu.c gb.c gpu.c main.c mbc.c mmu.c -I"E:\Ferramentas\Dev\sdl2_32bit\include\SDL2" -L"E:\Ferramentas\Dev\sdl2_32bit\lib" -lmingw32 -lSDL2main -lSDL2
pause