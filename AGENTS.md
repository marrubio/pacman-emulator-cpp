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


## 5. Directrices y Consejos para Agentes de IA

Cuando trabaje como agente de IA en este proyecto, tenga en cuenta las siguientes restricciones y recomendaciones:

1.  **Ediciones Selectivas**: Si se trabaja de forma local con el archivo `pacman.asm` (el cual debe ser descargado manualmente y no está en el repositorio por temas de copyright), ten en cuenta que contiene casi 10,000 líneas. Intentar reescribir o leer el archivo completo consume una gran cantidad de contexto. Utilice herramientas como `replace_file_content` o `multi_replace_file_content` proporcionando el fragmento exacto que desea modificar.
2.  **Cuidado con el Watchdog**: Si modifica o reescribe rutinas en el ciclo principal o de interrupción, asegúrese de preservar la instrucción de escritura al registro de watchdog `50C0h` (por ejemplo: `ld (#50C0), a`). De lo contrario, los emuladores precisos (como MAME) se reiniciarán constantemente imposibilitando la ejecución estable del juego.
3.  **Etiquetas en lugar de Direcciones Numéricas**: Z80 es muy sensible a los desplazamientos. Si agrega o elimina código, todos los saltos relativos (`jr offset`) y absolutos (`jp address`) que apunten a direcciones explícitas (como `jp #008D` o `jr #0051`) podrían romperse. Siempre que añada lógica, defina y utilice **etiquetas descriptivas** (labels) en el ensamblador para permitir que Pasmo recalcule automáticamente los saltos durante el proceso de compilación.
4.  **Formatos numéricos**: Recuerde que en este código ensamblador, los números hexadecimales se expresan frecuentemente con un prefijo numeral `#` (por ejemplo, `#3f`, `#4ecc`) en lugar del formato moderno `0x`. Asegúrese de respetar esta convención sintáctica para evitar fallos de parseo en Pasmo.