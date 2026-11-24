# RS-2A — Compresor óptico VST3 (estilo LA-2A)

Plugin VST3 para Windows (x64), hecho con JUCE 8. Emula el comportamiento de un compresor
óptico tipo LA-2A con los controles de tu imagen.

## Cómo obtener el .vst3 (elige UNA opción)

### Opción A — Sin instalar nada (GitHub Actions)
1. Crea un repositorio en GitHub y sube el contenido de esta carpeta.
2. Pestaña **Actions** → "Build RS-2A VST3 (Windows)" (se ejecuta solo al subir, o pulsa *Run workflow*).
3. Al terminar, descarga el artefacto **RS-2A-VST3-Windows** (contiene `RS-2A.vst3`).

### Opción B — En tu PC con Windows
1. Instala: Visual Studio 2022 (carga "Desarrollo para el escritorio con C++"), CMake y Git.
2. Doble clic en `build_windows.bat` (la primera vez descarga JUCE automáticamente).
3. Resultado: `build\RS2A_artefacts\Release\VST3\RS-2A.vst3`

### Instalar
Copia la carpeta `RS-2A.vst3` a `C:\Program Files\Common Files\VST3\` y reescanea plugins en tu DAW.
El runtime está enlazado estáticamente: no hace falta Visual C++ Redistributable.

## Controles

| Control | Función |
|---|---|
| PEAK REDUCTION | Cantidad de compresión (ganancia del sidechain, 0–100) |
| GAIN | Ganancia de compensación (−12 a +28 dB) |
| COMPRESS / LIMIT | Compresión suave ~3:1 o limitación ~10:1 |
| SIDECHAIN HPF | Filtro pasa-altos del detector: OFF, 60, 90, 150, 300 Hz |
| TUBE SATURATION | Saturación valvular (OFF, 1–5), con sobremuestreo 2x |
| MIX | Mezcla paralela dry/wet |
| ATTACK / RELEASE | De SLOW a FAST (por defecto ≈ 9 ms de ataque, como la celda T4) |
| NOISE | Añade ruido de fondo sutil (≈ −74 dBFS) |
| VU / GR | El medidor muestra nivel de salida (VU) o reducción de ganancia (GR) |
| OUTPUT | Nivel de salida (−18 a +18 dB) + medidor de LEDs |
| BYPASS | Bypass con compensación de latencia |

Doble clic en una perilla = valor por defecto.

## Comportamiento óptico
- Detector tipo RMS (~10 ms) sobre la entrada, enlace estéreo.
- Celda óptica de dos etapas: la liberación inicial es rápida y luego hay una cola lenta
  (hasta varios segundos) que depende de cuánto tiempo estuvo comprimiendo (dependiente del programa).

## Personalizar la placa
En `Source/PluginEditor.cpp` (arriba): `kBrand`, `kBrandSub`, `kModel`.
La marca del cristal del medidor está en `Source/GuiComponents.h` ("SARMI STUDIOS").

## Licencia de JUCE
JUCE se distribuye bajo AGPLv3 o licencia comercial. Para vender el plugin sin liberar el código
necesitas una licencia comercial de JUCE.
