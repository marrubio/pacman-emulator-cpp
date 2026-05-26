#pragma once
#include <cstdint>
#include <array>
#include <string>

class PacmanMemory {
public:
    PacmanMemory();
    ~PacmanMemory() = default;

    // Métodos de acceso a memoria física
    uint8_t ReadByte(uint16_t address) const;
    void WriteByte(uint16_t address, uint8_t value);

    // Carga de la ROM original (16 KB)
    bool LoadROM(const std::string& filepath);

    // Métodos específicos para depurar y leer variables críticas del juego
    uint16_t GetP1Score() const;
    uint16_t GetP2Score() const;
    uint16_t GetHighScore() const;
    uint8_t GetLives() const;
    uint8_t GetPlayerCount() const;

    // Acceso directo a secciones de memoria para el renderizador
    const uint8_t* GetVRAMPtr() const { return m_vram.data(); }
    const uint8_t* GetColorRAMPtr() const { return m_color_ram.data(); }
    const uint8_t* GetSpriteCoordsPtr() const { return &m_io_registers[0x60]; } // 5060h - 506Fh

    // Simulación de estados de entrada física (botones del mando)
    void SetP1Button(uint8_t bit_mask, bool pressed);
    void SetP2Button(uint8_t bit_mask, bool pressed);
    void SetDipSwitch(uint8_t bit_mask, bool state);

    // Consultar estado de interrupciones
    bool AreInterruptsEnabled() const { return m_interrupts_enabled; }
    
    // Simular el disparo del watchdog
    bool IsWatchdogResetTriggered() { bool val = m_watchdog_triggered; m_watchdog_triggered = false; return val; }

private:
    // Estructuras de almacenamiento interno (Mapeo de hardware)
    std::array<uint8_t, 16384> m_rom;        // 0000h - 3FFFh (16 KB)
    std::array<uint8_t, 1024>  m_vram;       // 4000h - 43FFh (1 KB)
    std::array<uint8_t, 1024>  m_color_ram;  // 4400h - 47FFh (1 KB)
    std::array<uint8_t, 1024>  m_system_ram; // 4C00h - 4FFFh (1 KB) (y espejo de 4800h-4BFFh)
    
    // Registros I/O Mapeados en Memoria (5000h - 50FFh)
    uint8_t m_in0;                           // Entrada Mandos Jugador 1 (Lectura en 5000h)
    uint8_t m_in1;                           // Entrada Mandos Jugador 2 (Lectura en 5001h)
    uint8_t m_dsw1;                          // Dip Switches (Lectura en 5002h)
    
    // Registros de Escritura Mapeados en 5000h-50FFh
    std::array<uint8_t, 256> m_io_registers; // Almacenamiento intermedio de escrituras
    
    bool m_interrupts_enabled;               // Controlado en 5000h (Escritura)
    bool m_sound_enabled;                    // Controlado en 5001h (Escritura)
    bool m_flip_screen;                      // Controlado en 5003h (Escritura)
    
    // Temporizador y control de watchdog
    uint32_t m_watchdog_counter;
    bool     m_watchdog_triggered;

    // Convertir puntuación guardada en formato BCD (Binary Coded Decimal) a binario normal
    uint16_t BCDToInteger(const uint8_t* bcd_ptr, size_t num_bytes) const;
};
