# Pac-Man: Port Nativo x86 y Simulador de Hardware Z80

Este proyecto tiene como objetivo principal ejecutar el clásico **Pac-Man (1980)** de forma nativa en arquitecturas modernas (x86/x64) mediante la simulación en C++ del mapa de memoria y los registros del hardware arcade original basado en el procesador **Zilog Z80**. 

A partir del desensamblado del código original en ensamblador Z80, se ha estructurado una **versión beta en C++** que simula el comportamiento físico del mueble arcade. Esta base sirve para estudiar, depurar y completar la lógica del juego directamente de forma nativa sin necesidad de un emulador externo de Z80, recreando la experiencia del hardware clásico a través de una capa de software moderna.

---

## 1. Estructura y Contenido de `pacman_cpp`

La carpeta `pacman_cpp` contiene la implementación en C++ del port nativo y el simulador de hardware. Utiliza la biblioteca gráfica **Raylib** para renderizar la pantalla y capturar las entradas de teclado a bajo nivel.

A continuación se detalla la estructura y propósito de cada archivo en `pacman_cpp/src/`:

*   **[CMakeLists.txt](file:///home/mrg/share/github/pacman-emulator-cpp/pacman_cpp/CMakeLists.txt)**: Script de construcción de CMake. Configura el estándar C++20, gestiona la descarga automática y el enlace estático de la biblioteca **Raylib 5.0** (a través de `FetchContent`) y compila el ejecutable nativo.
*   **[src/main.cpp](file:///home/mrg/share/github/pacman-emulator-cpp/pacman_cpp/src/main.cpp)**: Punto de entrada del programa. Se encarga de inicializar el motor del juego, ejecutar el bucle principal de renderizado y física, y liberar los recursos del sistema al salir.
*   **[src/game.hpp](file:///home/mrg/share/github/pacman-emulator-cpp/pacman_cpp/src/game.hpp) / [game.cpp](file:///home/mrg/share/github/pacman-emulator-cpp/pacman_cpp/src/game.cpp)**: Coordina el flujo global del juego. Implementa el bucle principal sincronizado a la frecuencia exacta del hardware arcade (~60.606 Hz). También se encarga de:
    *   Cargar dinámicamente un archivo ROM de Pac-Man original (`pacman.bin`) si está presente en el directorio.
    *   Generar un mapa e interactividad de respaldo por defecto (para pruebas en caso de que no haya ROM).
    *   Procesar entradas de teclado nativas y mapearlas a las direcciones simuladas correspondientes del hardware.
    *   Realizar comprobaciones básicas de colisiones y actualizar el estado de las pastillas y puntuaciones en formato BCD.
*   **[src/memory.hpp](file:///home/mrg/share/github/pacman-emulator-cpp/pacman_cpp/src/memory.hpp) / [src/memory.cpp](file:///home/mrg/share/github/pacman-emulator-cpp/pacman_cpp/src/memory.cpp)**: Simula de forma precisa el mapa de memoria física y el bus de datos del hardware de Pac-Man. Define los rangos direccionables (`ROM`, `VRAM`, `Color RAM`, `System RAM`, registros de E/S de escritura/lectura y el Watchdog Reset). Incluye utilidades para decodificar puntuaciones almacenadas en el formato original BCD (Binary Coded Decimal).
*   **[src/renderer.hpp](file:///home/mrg/share/github/pacman-emulator-cpp/pacman_cpp/src/renderer.hpp) / [src/renderer.cpp](file:///home/mrg/share/github/pacman-emulator-cpp/pacman_cpp/src/renderer.cpp)**: Módulo de renderizado gráfico basado en Raylib. Lee directamente las estructuras de la VRAM y la Color RAM virtuales del sistema para dibujar el laberinto, los muros vectoriales, las pastillas y energizantes (con efectos de parpadeo). Adicionalmente:
    *   Renderiza a Pac-Man (Sprite 0) de forma procedural como un sector circular animado que abre y cierra la boca según su movimiento.
    *   Dibuja de forma procedural a los cuatro fantasmas clásicos (Blinky, Pinky, Inky y Clyde) replicando sus colores y la oscilación característica de sus cuerpos y ojos.
    *   Pinta el HUD clásico con el puntaje actual del jugador, el récord histórico de la máquina (High Score) y las vidas restantes.

---

## 2. Requisitos y Compilación del Port Nativo (C++)

Para compilar y ejecutar el port en C++ necesitas:
1. Un compilador compatible con **C++20** (GCC 10+, Clang 10+ o MSVC 2019+).
2. **CMake** versión 3.12 o superior instalado.
3. Conexión a Internet activa (solo en la primera compilación) para que CMake descargue la biblioteca Raylib de forma automática.

### Instrucciones de Compilación (Linux / macOS)

Abre una terminal en la raíz del proyecto y ejecuta los siguientes comandos:

```bash
# Crear y acceder al directorio de construcción
mkdir -p build && cd build

# Configurar el proyecto con CMake (añadir -DCMAKE_POLICY_VERSION_MINIMUM=3.5 si da error en CMake moderno)
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ../pacman_cpp

# Compilar el ejecutable
make

# Ejecutar el juego
./pacman_native
```

### Controles de Juego
*   **FLECHAS DE DIRECCIÓN (Teclado)**: Mover a Pac-Man en las cuatro direcciones espaciales.
*   **Mecánica**: Comer las pastillas incrementará tu puntuación y se actualizará automáticamente en el HUD superior en tiempo real.

---

## 3. Trabajo con el Ensamblador Z80 Original (Opcional)

Si eres un desarrollador avanzado o investigador que desea trabajar con el código en ensamblador original en formato `.asm` o interactuar con el juego a través de emuladores arcade tradicionales como MAME, esta sección detalla los pasos opcionales para realizarlo.

> [!NOTE]
> Por motivos de copyright, el código fuente completo en ensamblador (`pacman.asm`) no se distribuye en este repositorio. Debes descargarlo o suministrarlo por separado para realizar estos pasos opcionales.

### A. Ensamblado con Pasmo
Para compilar el archivo `.asm` y obtener un bloque binario plano de **16 KB (16,384 bytes)** que representa toda la ROM del juego, se utiliza el cross-assembler **Pasmo**:

```bash
# Compilar a binario plano Z80
pasmo --bin pacman.asm pacman.bin

# Generar opcionalmente una tabla de símbolos de depuración
pasmo --bin pacman.asm pacman.bin pacman.sym
```

El archivo resultante `pacman.bin` puede ser copiado a la carpeta de ejecución de `pacman_native` para que el simulador lo lea dinámicamente.

### B. Emulación en MAME (Opcional)
Si deseas ejecutar la ROM en el emulador **MAME**, es necesario dividir el bloque binario en los cuatro chips físicos originales de **4 KB** cada uno:

1.  **`pacman.6e`**: Rango `0000h` a `0FFFh`.
2.  **`pacman.6f`**: Rango `1000h` a `1FFFh`.
3.  **`pacman.6h`**: Rango `2000h` a `2FFFh`.
4.  **`pacman.6j`**: Rango `3000h` a `3FFFh`.

#### Script de Automatización (Python)
Puedes usar el script `split_rom.py` proporcionado en el proyecto para dividir de forma automática tu binario compilado `pacman.bin` y empaquetarlo en un archivo ZIP compatible con MAME:

```bash
python3 split_rom.py
```
Esto creará el archivo `pacman.zip`. Cópialo al directorio de ROMs de MAME y ejecútalo mediante:
```bash
mame pacman
```

---

## 4. Mapa de Memoria del Hardware Simulado (Referencia)

El bus de memoria física direccionado de 16 bits (`0000h` a `FFFFh`) que simula nuestra clase `PacmanMemory` tiene la siguiente estructura técnica:

| Rango de Direcciones (Hex) | Tamaño | Descripción del Componente / Función |
| :--- | :--- | :--- |
| `0000h` - `3FFFh` | 16 KB | **ROM del Juego**: Almacena el código ejecutable de Z80 y las tablas de datos fijos. |
| `4000h` - `43FFh` | 1 KB | **Video RAM (VRAM)**: Mapeo bidimensional de la matriz de baldosas (tiles) en pantalla. |
| `4400h` - `47FFh` | 1 KB | **Color RAM**: Mapeo de colores y atributos asociados a cada mosaico en pantalla. |
| `4800h` - `4BFFh` | 1 KB | Espejo auxiliar de la RAM de Sistema. |
| `4C00h` - `4FFFh` | 1 KB | **RAM del Sistema (Working RAM)**: Variables del juego, puntuaciones y estados. |
| `5000h` - `50FFh` | 256 B | **I/O Mapeado en Memoria**: Puertos de entrada/salida física (controles, dip switches, etc.). |

### Direcciones de Variables Clave
*   **`4370h`**: Número de jugadores (`00h` = 1 Jugador, `01h` = 2 Jugadores).
*   **`4E66h`**: Estado y registro del monedero arcade.
*   **`4E6Fh`**: Cantidad de vidas iniciales del jugador.
*   **`4E80h` - `4E83h`**: Puntuación del Jugador 1 (almacenada en BCD little-endian de 4 bytes).
*   **`4E88h` - `4E8Bh`**: Récord Máximo (High Score) histórico de la máquina (BCD).
*   **`5000h` (Lectura)**: Puerto `IN0`. Mapea controles del Jugador 1 (Arriba, Izquierda, Derecha, Abajo, Moneda, etc.).
*   **`5002h` (Lectura)**: Puerto de Dip Switches `DSW1` (Configuración de dificultad y créditos).
*   **`50C0h` (Escritura)**: **Watchdog Reset**. Escribir aquí reinicia el temporizador de reinicio físico de la CPU.
