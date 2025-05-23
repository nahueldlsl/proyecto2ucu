# proyecto2ucu
Panel Sensorial del Clima — Ciencia de Datos e Inclusión Sensorial

Descripción:

Este proyecto utiliza ciencia de datos para acceder, almacenar y procesar información del clima durante un período de siete días (tres pasados, el actual y tres futuros). Los datos son extraídos de fuentes confiables mediante una API meteorológica, y convertidos en estímulos sensoriales simples, ayudando a niños con TEA a anticipar y entender el comportamiento del clima en su entorno.

A través del uso de datos reales del clima, el sistema interpreta el estado meteorológico actual o estimado, y activa un escenario sensorial acorde, utilizando un Arduino UNO como microcontrolador central.

Objetivos:
- Usar ciencia de datos para obtener la información del clima actual o predecible.
Esto permite a los niños:
- Recordar qué clima hubo (refuerzo de memoria).
- Facilitar la comprensión del clima mediante estímulos multisensoriales (luces, sonidos, texturas).
- Crear una herramienta educativa e inclusiva, especialmente pensada para personas con TEA.
- Anticiparse al clima próximo (muy útil para la rutina y organización emocional).

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



