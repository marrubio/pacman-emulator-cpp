# Pac-Man Z80 Documentation (AGENTS.md)

Welcome! The primary, fully detailed documentation for this workspace is written in Spanish at **[README.md](file:///C:/ssg/arch/ai/pacman/README.md)**. 

If you are an AI developer or coding agent, please refer to that file for a comprehensive breakdown of the hardware memory map, compilation procedures, MAME emulators, python ROM splitters, and critical development guidelines.

## Quick Reference

### Z80 Compilation
To compile the assembly file, install **Pasmo** (a Z80 cross-assembler) and run:
```bash
pasmo --bin pacman.asm pacman.bin
```

### Memory Map Overview
*   `0000h` - `3FFFh`: ROM (16 KB)
*   `4000h` - `43FFh`: Video RAM
*   `4400h` - `47FFh`: Color RAM
*   `4C00h` - `4FFFh`: System RAM
*   `50C0h`: Watchdog reset register (Crucial to write here regularly to prevent hardware reboots).
