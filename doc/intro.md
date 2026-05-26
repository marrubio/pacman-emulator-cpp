# Introducción al Módulo de Juego (Game) en Pac-Man C++

Este documento explica el funcionamiento de la lógica principal del juego en `game.hpp` y `game.cpp`, así como una breve introducción a la estructura de archivos en C++ para aquellos desarrolladores que necesiten refrescar sus conocimientos.

---

## 1. La Estructura de Archivos en C++: ¿Por qué `.hpp` y `.cpp`?

En C++, a diferencia de lenguajes modernos que unifican todo en un único archivo (como Python o Java), la lógica se divide en dos componentes fundamentales para separar la **interfaz** (declaración) de la **implementación** (definición):

### El archivo de cabecera (`.hpp` o `.h`) — *La Declaración*
* **¿Qué hace?** Declara **qué** existe. Define la estructura de la clase: sus variables miembro (atributos) y los prototipos de sus funciones (métodos), sin escribir el código de lo que hacen por dentro.
* **¿Para qué sirve?** Sirve para que otros archivos del proyecto sepan cómo comunicarse con esta clase. Al hacer `#include "game.hpp"`, el compilador sabe qué métodos tiene la clase `Game`, qué parámetros reciben y qué devuelven, permitiendo su uso sin necesidad de compilar todo el código interno cada vez.
* **Directiva clave:** Comienza con `#pragma once`, una directiva que indica al compilador que solo cargue este archivo una vez por compilación, evitando colisiones o redefiniciones.

### El archivo de código fuente (`.cpp`) — *La Definición*
* **¿Qué hace?** Define **cómo** funciona. Contiene el código real de todas las funciones que fueron declaradas en el archivo `.hpp`.
* **¿Para qué sirve?** Contiene la lógica ejecutable. Durante la compilación, cada archivo `.cpp` se compila por separado en un "archivo objeto" (`.obj` o `.o`). Posteriormente, el enlazador (**linker**) une todos estos objetos para generar el ejecutable definitivo.

---

## 2. Explicación Detallada de los Archivos del Juego

La arquitectura de este clon de Pac-Man es muy interesante: no es solo un clon gráfico simple, sino que **emula el comportamiento y la estructura del hardware original** de la máquina arcade clásica de Pac-Man (basada en el microprocesador Z80 y su direccionamiento de memoria).

### 📄 [game.hpp](file:///c:/ssg/arch/ai/pacman/pacman_cpp/src/game.hpp)
Es la cabecera de la clase principal `Game`. Es muy compacta y define la estructura básica:

* **Variables privadas de estado:**
  * `m_memory`: Instancia de `PacmanMemory`. Simula de manera exacta el mapa de memoria RAM/VRAM del hardware arcade original.
  * `m_renderer`: Instancia de `PacmanRenderer`. Encargado del dibujado gráfico en pantalla utilizando la biblioteca Raylib.
  * `m_is_running`: Un booleano que controla el ciclo de vida del bucle del juego.
* **Métodos públicos:** Define el ciclo de vida del juego: `Initialize()` (arranque), `Run()` (bucle de ejecución) y `Shutdown()` (cierre limpio).
* **Métodos privados:** Funciones internas para procesar la entrada (`ProcessInput`), actualizar la lógica/física (`Update`), e inicializar un laberinto en memoria por defecto si no hay un archivo ROM disponible (`InitializeDefaultMaze`).

---

### 📄 [game.cpp](file:///c:/ssg/arch/ai/pacman/pacman_cpp/src/game.cpp)
Es donde reside la implementación real de la lógica del videojuego. A continuación, se detallan sus componentes clave:

#### A. Inicialización (`Initialize`)
* Arranca el subsistema de renderizado gráfico.
* Intenta cargar la ROM clásica del Pac-Man original (`pacman.bin`) en la memoria emulada (`m_memory.LoadROM`).
* Si no encuentra el archivo ROM, llama a `InitializeDefaultMaze()` para poblar la RAM con el laberinto clásico en formato interactivo de demostración.

#### B. El Bucle de Juego (`Run`)
Implementa un bucle clásico de desarrollo de videojuegos con actualización a velocidad constante (**fixed timestep**):
* La máquina recreativa original de Pac-Man funcionaba exactamente a **~60.606 Hz** (FPS). El código calcula este intervalo exacto (`1.0 / 60.606`).
* Calcula el tiempo transcurrido en el mundo real y ejecuta tantos pasos físicos (`Update`) como sean necesarios para mantener el ritmo original de la recreativa de forma independiente a la velocidad de refresco del monitor moderno.
* En cada frame lee la entrada del teclado, actualiza el estado y dibuja llamando a `m_renderer.Render(m_memory)`.

#### C. Simulación del Hardware y Control (`ProcessInput`)
Interactúa de manera directa con las posiciones de memoria física del hardware original:
* Lee la posición en píxeles de Pac-Man directamente de las direcciones de memoria emuladas `0x5060` (coordenada Y del Sprite 0) y `0x5061` (coordenada X del Sprite 0).
* *Curiosidad del hardware:* Debido a cómo estaba montado el monitor original CRT (rotado y en espejo), en el hardware real el eje X está invertido. Por ello, ir a la derecha **resta** valor a X (`px -= speed`) e ir a la izquierda **suma** (`px += speed`).
* Escribe la nueva posición de vuelta en la RAM simulada.
* Mapea las teclas de dirección del teclado moderno a los registros del puerto de entrada `IN0` (`0x5000`) de la máquina recreativa original.

#### D. Actualización y Colisiones (`Update`)
* Convierte las coordenadas en píxeles finos de Pac-Man a coordenadas de cuadrícula de baldosas (**tiles** de `8x8` píxeles).
* Consulta el contenido del tile en la dirección VRAM correspondiente (`0x4000` + offset).
* Si Pac-Man pasa por encima de una pastilla normal (`0x10`) o un energizante (`0x14`), borra ese elemento escribiendo un vacío (`0x00`) en dicha dirección de la VRAM.
* **Sistema de puntuación BCD (Binary Coded Decimal):** En los arcades clásicos de los 80, las puntuaciones se guardaban en formato BCD (donde cada 4 bits/nibble representan un dígito decimal del 0 al 9 directamente) para evitar costosas conversiones matemáticas en el procesador. El código emula esto sumando en BCD y aplicando el ajuste decimal necesario cuando se supera el valor de 9 (`>= 0x0A`).

#### E. Generación del Laberinto por Defecto (`InitializeDefaultMaze`)
* Contiene una representación visual en formato ASCII del mapa icónico de Pac-Man.
* Carga este diseño ASCII en la VRAM emulada:
  * Los muros (`#`) se escriben con el código `0x01` en el búfer de tiles (`0x4000`) y se les asigna el atributo de color azul (`0x07`) en el búfer de colores (`0x4400`).
  * Las pastillas y energizantes se graban con sus respectivos códigos gráficos.
* Inicializa variables en la RAM como las vidas (3 iniciales en `0x4E6F`), la puntuación del jugador a cero, el récord máximo por defecto en BCD a 5000 puntos (`0x4E88`), e inicializa las coordenadas iniciales de los Sprites en memoria (Pac-Man y los cuatro fantasmas: Blinky, Pinky, Inky y Clyde).
