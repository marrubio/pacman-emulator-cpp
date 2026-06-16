#include "memory.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

PacmanMemory::PacmanMemory()
    : m_in0(0xDF) // 1101 1111b (Por defecto, monedas y mandos sin pulsar. Los bits activos son 0)
    , m_in1(0xFF) // Todos los botones arriba
    , m_dsw1(0xC9) // Configuración por defecto: 1 moneda/1 crédito, 3 vidas, juego difícil
    , m_interrupts_enabled(false)
    , m_sound_enabled(false)
    , m_flip_screen(false)
    , m_watchdog_counter(0)
    , m_watchdog_triggered(false)
{
    m_rom.fill(0x00);
    m_vram.fill(0x00);
    m_color_ram.fill(0x00);
    m_system_ram.fill(0x00);
    m_io_registers.fill(0x00);
}

bool PacmanMemory::LoadROM(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Advertencia: Archivo de ROM original '" << filepath 
                  << "' no encontrado. Iniciando con ROM en blanco." << std::endl;
        return false;
    }

    file.read(reinterpret_cast<char*>(m_rom.data()), m_rom.size());
    size_t bytes_read = file.gcount();
    std::cout << "Éxito: " << bytes_read << " bytes cargados de ROM en memoria del sistema." << std::endl;
    return true;
}

uint8_t PacmanMemory::ReadByte(uint16_t address) const {
    // 1. ROM del Juego (0000h - 3FFFh)
    if (address < 0x4000) {
        return m_rom[address];
    }
    
    // 2. Video RAM (4000h - 43FFh)
    if (address >= 0x4000 && address < 0x4400) {
        return m_vram[address - 0x4000];
    }
    
    // 3. Color RAM (4400h - 47FFh)
    if (address >= 0x4400 && address < 0x4800) {
        return m_color_ram[address - 0x4400];
    }
    
    // 4. Puertos de I/O mapeados en lectura (5000h - 50FFh)
    if (address >= 0x5000 && address < 0x5100) {
        uint8_t reg_offset = address & 0xFF;
        switch (reg_offset) {
            case 0x00: return m_in0;
            case 0x01: return m_in1;
            case 0x02: return m_dsw1;
            default:   return m_io_registers[reg_offset];
        }
    }
    
    // 5. RAM de Sistema (4C00h - 4FFFh) y espejo auxiliar (4800h - 4BFFh)
    if (address >= 0x4800 && address < 0x5000) {
        // En el hardware real, 4800-4BFF se mapea a la misma RAM física que 4C00-4FFF en muchos esquemas
        uint16_t offset = (address - 0x4800) & 0x3FF;
        return m_system_ram[offset];
    }

    return 0xFF; // Fuera de rango físico del hardware
}

void PacmanMemory::WriteByte(uint16_t address, uint8_t value) {
    // 1. Escribir en VRAM (4000h - 43FFh)
    if (address >= 0x4000 && address < 0x4400) {
        m_vram[address - 0x4000] = value;
        return;
    }
    
    // 2. Escribir en Color RAM (4400h - 47FFh)
    if (address >= 0x4400 && address < 0x4800) {
        m_color_ram[address - 0x4400] = value;
        return;
    }
    
    // 3. Escribir en RAM de Sistema y su espejo (4C00h - 4FFFh / 4800h - 4BFFh)
    if (address >= 0x4800 && address < 0x5000) {
        uint16_t offset = (address - 0x4800) & 0x3FF;
        m_system_ram[offset] = value;
        return;
    }
    
    // 4. Puertos e I/O de Escritura (5000h - 50FFh)
    if (address >= 0x5000 && address < 0x5100) {
        uint8_t reg_offset = address & 0xFF;
        m_io_registers[reg_offset] = value;
        
        switch (reg_offset) {
            case 0x00:
                m_interrupts_enabled = (value & 0x01) != 0;
                break;
            case 0x01:
                m_sound_enabled = (value & 0x01) != 0;
                break;
            case 0x03:
                m_flip_screen = (value & 0x01) != 0;
                break;
            case 0xC0:
                // Watchdog reset kick!
                m_watchdog_counter = 0;
                break;
        }
        return;
    }
}

void PacmanMemory::SetP1Button(uint8_t bit_mask, bool pressed) {
    if (pressed) {
        m_in0 &= ~bit_mask; // Los botones se activan en nivel bajo (0)
    } else {
        m_in0 |= bit_mask;
    }
}

void PacmanMemory::SetP2Button(uint8_t bit_mask, bool pressed) {
    if (pressed) {
        m_in1 &= ~bit_mask;
    } else {
        m_in1 |= bit_mask;
    }
}

void PacmanMemory::SetDipSwitch(uint8_t bit_mask, bool state) {
    if (state) {
        m_dsw1 &= ~bit_mask;
    } else {
        m_dsw1 |= bit_mask;
    }
}

uint16_t PacmanMemory::GetP1Score() const {
    // 4E80h - 4E83h P1 score (4 bytes en BCD)
    return BCDToInteger(&m_system_ram[0x0680 - 0x0400], 4);
}

uint16_t PacmanMemory::GetP2Score() const {
    // 4E84h - 4E87h P2 score (4 bytes en BCD)
    return BCDToInteger(&m_system_ram[0x0684 - 0x0400], 4);
}

uint16_t PacmanMemory::GetHighScore() const {
    // 4E88h - 4E8Bh High score (4 bytes en BCD)
    return BCDToInteger(&m_system_ram[0x0688 - 0x0400], 4);
}

uint8_t PacmanMemory::GetLives() const {
    // 4E6Fh lives per game
    return m_system_ram[0x066F - 0x0400];
}

uint8_t PacmanMemory::GetPlayerCount() const {
    // 4370h #players (0=1, 1=2) (Ubicado en la zona alta de la VRAM en la placa física)
    return m_vram[0x0370];
}

uint16_t PacmanMemory::BCDToInteger(const uint8_t* bcd_ptr, size_t num_bytes) const {
    uint32_t val = 0;
    // BCD almacena dígitos numéricos en nibbles (4 bits cada uno).
    // Pac-Man los almacena en orden inverso (little endian a nivel de dígito)
    for (int i = static_cast<int>(num_bytes) - 1; i >= 0; --i) {
        uint8_t byte = bcd_ptr[i];
        uint8_t high_digit = (byte >> 4) & 0x0F;
        uint8_t low_digit = byte & 0x0F;
        
        val = val * 10 + high_digit;
        val = val * 10 + low_digit;
    }
    return static_cast<uint16_t>(val);
}
