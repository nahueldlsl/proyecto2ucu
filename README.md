# proyecto2ucu
Panel Sensorial del Clima — Ciencia de Datos e Inclusión Sensorial

Descripción:

El Panel Sensorial del Clima es un proyecto que une ciencia de datos y estimulación sensorial para representar las condiciones climáticas de manera accesible y comprensible. Está especialmente diseñado para personas con TEA (Trastorno del Espectro Autista), con el objetivo de ayudarles a entender y anticipar el clima a través de estímulos visuales, sonoros y táctiles.

A través del uso de datos reales del clima, el sistema interpreta el estado meteorológico actual o estimado, y activa un escenario sensorial acorde, utilizando un Arduino UNO como microcontrolador central.

Objetivos:
- Usar ciencia de datos para obtener la información del clima actual o predecible.
- Facilitar la comprensión del clima mediante estímulos multisensoriales (luces, sonidos, texturas).
- Crear una herramienta educativa e inclusiva, especialmente pensada para personas con TEA.

Ciencia de Datos aplicada:
- El proyecto emplea ciencia de datos para:
- Obtener y procesar datos meteorológicos de fuentes reales (API o datos descargados).
- Determinar la probabilidad o presencia de ciertos estados climáticos.
- Comunicar esa información al panel físico para una representación sensorial clara.
- Este procesamiento se realiza en una PC externa, y el resultado se envía al Arduino UNO para activar el clima correspondiente.

Componentes:
- Microcontrolador: Arduino UNO
- Luces LED RGB: para representar visualmente el estado del clima
- Módulo de sonido: (DFPlayer Mini + parlante pequeño)
- Proyector para mapping
- Figuras de las condiciones climáticas impresas en 3D
- Cables, resistencias, protoboard o placa de montaje
- Script externo en Python (opcional): para la parte de ciencia de datos
- Fuente de alimentación / batería

Climas representados:
Clima	// Estímulos sensoriales

Soleado //	Luz cálida amarilla + sonido de pájaros + sol en 3D + mapping iluminando el sol

Lluvia //	Luz azul tenue + sonido de gotas + mapping de lluvia

Tormenta //	Flashes de luz + sonido de truenos + mapping que oscurece las nubes

Viento // Nubes 3D, mapping sobre las nubes simulando viento



