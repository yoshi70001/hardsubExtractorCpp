# Extractor de Frames con Hardsubs

Una aplicación en C++11 para analizar archivos de video y extraer los fotogramas que contienen texto (subtítulos incrustados o "hardsubs").

## Funcionalidad Principal

- **Detección de Texto:** Analiza un video para identificar los fotogramas donde hay texto presente.
- **Extracción de Fotogramas:** Guarda los fotogramas identificados como imágenes individuales.
- **Marcas de Tiempo:** Para cada fotograma extraído, se determina el período de tiempo en el que el texto está visible. Los nombres de los archivos de imagen generados suelen corresponder a estas marcas de tiempo.

**Importante:** Esta herramienta **no realiza OCR** (Reconocimiento Óptico de Caracteres). Su único propósito es detectar la presencia y la duración del texto en pantalla, no convertir dicho texto a caracteres legibles.

## Tecnología

- **Lenguaje:** C++11

## Estructura del Directorio

```
.
├── build/              # Archivos de compilación y ejecutables
├── videos/             # Directorio para colocar los videos de entrada
├── main.cpp            # Punto de entrada principal de la aplicación C++
└── README.md           # Este archivo
```

## Requisitos

*   **OpenCV:** Es fundamental tener OpenCV instalado. Puedes descargarlo desde su [sitio web oficial](https://opencv.org/releases/). Deberás compilarlo tú mismo o usar una versión precompilada. Asegúrate de que las bibliotecas de OpenCV y los archivos de cabecera sean accesibles para tu compilador, ya sea agregándolos al PATH del sistema o configurando las rutas directamente en tu proyecto de compilación (CMake, Visual Studio, etc.).
*   **Compilador C++11:** Un compilador compatible con el estándar C++11 o superior.

## Uso

1.  **Coloca tu video**: Mueve el archivo de video a procesar al directorio `videos/`.
2.  **Compila el proyecto**: Usa las herramientas de compilación adecuadas para tu entorno (ej. `cmake`, `make`, Visual Studio).
3.  **Ejecuta la aplicación**: Lanza el ejecutable generado.
4.  **Revisa los resultados**: Las imágenes de los fotogramas con texto se guardarán en el directorio `output_images/` o en una carpeta específica para ese video.