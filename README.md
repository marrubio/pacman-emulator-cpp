# Guía de Desarrollo y Ejecución: Pac-Man Z80

Este documento sirve como manual técnico y guía de referencia para desarrolladores y **agentes de IA** que interactúen con este repositorio. Contiene los detalles de la arquitectura del hardware original, mapa de memoria, requisitos de compilación, instrucciones de emulación y mejores prácticas para la modificación del código fuente.

---

## 1. Introducción al Proyecto

El motor original del juego **Pac-Man (1980)** de Namco/Midway fue programado en lenguaje ensamblador para la CPU **Zilog Z80**, un microprocesador de 8 bits muy popular en la edad de oro de los videojuegos arcade.

Para el desarrollo y emulación local, se trabaja a partir de una versión desensamblada, comentada y totalmente estructurada de dicho código binario original:
*   [Desensamblado de Pac-Man por Cubeman (pacman.asm)](http://cubeman.org/arcade-source/pacman.asm) (Referencia clásica comentada).
*   [Traducción anotada y comentada de mburkley/pacman-c](https://github.com/mburkley/pacman-c) (Mantiene el ensamblador original como comentarios de C).

El código fuente de este ensamblador se compila en un bloque binario plano de **16 KB (16,384 bytes)** que representa toda la lógica de ejecución del juego, incluyendo el bucle principal, la inteligencia artificial de los fantasmas (Blinky, Pinky, Inky y Clyde), las físicas del mapa, los patrones de fruta, el cálculo de puntuaciones y la reproducción de audio.

---

## 2. Mapa de Memoria del Hardware Original

El hardware arcade de Pac-Man utiliza un direccionamiento de memoria de 16 bits (rango `0000h` a `FFFFh`). La memoria física está mapeada de la siguiente manera:

| Rango de Direcciones (Hex) | Tamaño | Descripción del Componente / Función |
| :--- | :--- | :--- |
| `0000h` - `3FFFh` | 16 KB | **ROM del Juego**: Almacena el código ejecutable y las tablas de datos fijas del juego. |
| `4000h` - `43FFh` | 1 KB | **Video RAM (VRAM)**: Mapeo de la matriz de la pantalla para los mosaicos (tiles). |
| `4400h` - `47FFh` | 1 KB | **Color RAM**: Atributos de color y paleta de cada mosaico en pantalla. |
| `4800h` - `4BFFh` | 1 KB | Espacio reservado / RAM libre auxiliar. |
| `4C00h` - `4FFFh` | 1 KB | **RAM del Sistema (Working RAM)**: Variables del juego, puntuaciones y estados. |
| `5000h` - `50FFh` | 256 B | **I/O Mapeado en Memoria**: Puertos de entrada/salida para controles, sonido y hardware. |

### Direcciones de Variables Críticas (RAM del Sistema)
*   **`4370h`**: Número de jugadores actuales (`00h` = 1 jugador, `01h` = 2 jugadores).
*   **`4E66h`**: Estado y registro de las ranuras de monedas.
*   **`4E80h` - `4E83h`**: Puntuación del Jugador 1 (almacenado en formato BCD - Código Binario Decimal).
*   **`4E84h` - `4E87h`**: Puntuación del Jugador 2 (formato BCD).
*   **`4E88h` - `4E8Bh`**: Puntuación Máxima (High Score) histórica del mueble arcade.
*   **`4E8Ch` - `4E9Bh`**: Registros para la frecuencia y volumen del sonido.

### Registros de Entrada/Salida (I/O) Mapeados
*   **`5000h` (Lectura)**: Puerto de entrada `IN0`. Mapea controles del Jugador 1 (Arriba, Abajo, Izquierda, Derecha, Moneda 1, Moneda 2, etc.).
*   **`5000h` (Escritura)**: Habilitación de interrupciones de hardware (VBLANK). Escribir `01h` habilita la interrupción.
*   **`5001h` (Lectura)**: Puerto de entrada `IN1`. Mapea controles del Jugador 2 y botones de selección de inicio (Start 1P, Start 2P).
*   **`5001h` (Escritura)**: Habilitación del generador de sonido personalizado (Namco WSG).
*   **`5002h` (Lectura)**: Dip Switches (`DSW1`). Permite configurar dificultad, vidas iniciales (`4E6Fh`), y costos de créditos.
*   **`5003h` (Escritura)**: Voltear pantalla (`Flip Screen`) para modo cóctel de 2 jugadores.
*   **`50C0h` (Escritura)**: **Watchdog Reset**. Escribir cualquier valor en este registro le indica al hardware que el software está corriendo correctamente. Si transcurre un corto periodo sin escrituras en esta dirección, el circuito integrado reiniciará físicamente la CPU.

---

## 3. Requisitos de Compilación y Ensamblado

Para ensamblar el archivo de texto estructurado `pacman.asm` (que deberás haber descargado localmente en la raíz del proyecto según lo indicado en la Sección 1) y convertirlo en un binario ejecutable de Z80, se requiere un **compilador cruzado (cross-assembler) Z80**. 

El ensamblador recomendado por su compatibilidad directa con la sintaxis del archivo es **Pasmo**.

### Instalación de Pasmo
*   **Windows**: Puede descargar el ejecutable de Pasmo directamente de su sitio oficial o mediante repositorios de software retro.
*   **Linux (Ubuntu/Debian)**: 
    ```bash
    sudo apt-get update
    sudo apt-get install pasmo
    ```
*   **macOS**: Disponible a través de Homebrew o compitiendo desde la fuente.

### Comandos de Ensamblado
Abra una consola en la raíz de este proyecto y ejecute:

```bash
# Compilar a binario de Z80 puro (Raw Binary)
pasmo --bin pacman.asm pacman.bin
```

Si desea generar una tabla de símbolos detallada para tareas de depuración o investigación profunda de direcciones de etiquetas:

```bash
pasmo --bin pacman.asm pacman.bin pacman.sym
```

---

## 4. Requisitos y Proceso de Ejecución (Emulación)

Debido a que Pac-Man corre en un hardware arcade propietario de 1980 y no en un ordenador genérico Z80, el archivo compilado `pacman.bin` no se puede ejecutar directamente en un emulador de ZX Spectrum o MSX sin modificaciones estructurales drásticas.

Existen dos alternativas principales para ejecutar el código:

### Método A: Emulación en MAME (Recomendado para pruebas reales)
MAME (Multiple Arcade Machine Emulator) es el estándar de emulación arcade. Sin embargo, MAME espera que el código ROM esté fragmentado en los cuatro chips físicos originales de **4 KB cada uno**. 

Los nombres y rangos de memoria de estos chips son:
1.  **`pacman.6e`**: Contiene el código de `0000h` a `0FFFh` (primeros 4096 bytes).
2.  **`pacman.6f`**: Contiene el código de `1000h` a `1FFFh` (siguientes 4096 bytes).
3.  **`pacman.6h`**: Contiene el código de `2000h` a `2FFFh` (siguientes 4096 bytes).
4.  **`pacman.6j`**: Contiene el código de `3000h` a `3FFFh` (últimos 4096 bytes).

#### Script de Automatización (Python)
Para facilitar este proceso, puede ejecutar el siguiente script de Python (`split_rom.py`) en la raíz del proyecto para dividir de forma automática `pacman.bin` tras compilarlo:

```python
import os
import zipfile

def split_and_zip_roms():
    bin_file = "pacman.bin"
    if not os.path.exists(bin_file):
        print(f"Error: {bin_file} no encontrado. Ejecute pasmo primero.")
        return

    with open(bin_file, "rb") as f:
        data = f.read()

    if len(data) != 16384:
        print(f"Advertencia: El archivo compilado mide {len(data)} bytes en lugar de 16384.")
        # Rellenar con ceros si es más pequeño
        data = data.ljust(16384, b'\x00')

    rom_splits = {
        "pacman.6e": data[0:4096],
        "pacman.6f": data[4096:8192],
        "pacman.6h": data[8192:12288],
        "pacman.6j": data[12288:16384]
    }

    # Escribir los archivos locales
    for name, content in rom_splits.items():
        with open(name, "wb") as out_f:
            out_f.write(content)
        print(f"Creado: {name} ({len(content)} bytes)")

    # Empaquetar en pacman.zip para que MAME lo reconozca
    zip_name = "pacman.zip"
    with zipfile.ZipFile(zip_name, "w") as zip_f:
        for name in rom_splits.keys():
            zip_f.write(name)
            os.remove(name) # Limpiar archivos sueltos
    
    print(f"\n¡Éxito! Archivo '{zip_name}' generado de manera óptima.")
    print("Coloque 'pacman.zip' en su directorio de ROMs de MAME y ejecute:")
    print("  mame pacman")

if __name__ == "__main__":
    split_and_zip_roms()
```

