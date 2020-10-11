#+TITLE: CHIP-8 Emulator
#+TOC:nil

# Build
```bash
mkdir build
cd build
cmake ..
make
```

# TODOs
* TODO implement graphics backend
  * implement a special class for graphics in its own file
  * use SDL2 codebase (e.g. ~/Projects/SDL2_Test/src/sdl2_window.cpp) as starting point
  * implement macro pixels s.t. small original 64x32 window can be easily zoomed
* TODO implement sound backend
  * implement a special class for sound in its own file (even tho its just a beeper engine)
  * use SDL2 codebase (e.g. ~/Projects/SDL2_Test/src/sdl2_synthesizer.cpp) as starting point
  * normally one does not need to run it in a special thread, should already be managed by SDL2
* verify disassembler results
  * TODO write unit tests
* verify opcode functionality
  * TODO write unit tests
* verify assembler functionality
  * TODO write unit tests
