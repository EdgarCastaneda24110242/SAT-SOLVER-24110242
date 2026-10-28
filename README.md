# SAT Solver

## Dependencias

- C++17 y GNU Make.
- SFML 3.x es opcional: si está disponible se abre la interfaz gráfica; si no, el Makefile compila automáticamente la versión de consola.

En MSYS2 MinGW64:

```bash
pacman -S mingw-w64-x86_64-sfml
```

En macOS:

```bash
xcode-select --install
brew install sfml pkg-config
```

En Ubuntu/Linux:

```bash
sudo apt install build-essential libsfml-dev pkg-config
```

En macOS, el Makefile usa `pkg-config` y, como alternativa, `brew --prefix sfml`, por lo que funciona tanto en Apple Silicon como en Intel sin editar rutas.
En Windows también intenta `pkg-config` y las bibliotecas estándar de SFML. Si no encuentra las cabeceras, usa consola automáticamente.

## Compilar y ejecutar

Desde la raíz del proyecto:

```bash
make clean
make run SAT
```

El ejecutable y los objetos se generan dentro de `build/`. Los datos y recursos se resuelven con rutas relativas a la raíz del proyecto.

## Supuestos y riesgos no verificados

- No es posible ejecutar macOS desde este entorno; la compilación real debe verificarse en una Mac con SFML 3 instalada.
- La ventana depende de que SFML 3 y sus DLL/dylib estén disponibles en el entorno de ejecución.
- En Windows, la ejecución gráfica puede ser bloqueada por políticas del sistema aunque el proyecto compile correctamente.
- El Makefile detecta SFML mediante una prueba de cabeceras; si una instalación tiene cabeceras incompletas o bibliotecas incompatibles, puede seleccionar consola.
- La fuente y los datos se buscan con rutas relativas; `make run SAT` debe ejecutarse desde la raíz del proyecto.
