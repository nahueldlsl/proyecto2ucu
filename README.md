# Proyecto2 UCU – Panel Sensorial del Clima  
### Ciencia de Datos, Inclusión Sensorial y Tecnología Interactiva

## Descripción

Este proyecto combina ciencia de datos, electrónica y diseño inclusivo para ayudar a niños con Trastorno del Espectro Autista (TEA) a anticipar y comprender el clima mediante estímulos sensoriales visuales y táctiles. El dispositivo representa el estado del tiempo para los próximos seis días de forma lúdica y accesible, utilizando objetos 3D retroiluminados, pantalla LCD y un control remoto. El sistema se conecta a internet mediante un ESP8266 y accede a la API de Open-Meteo para obtener datos meteorológicos reales, que luego se representan físicamente en el panel.

Además, se incluye un arcoíris móvil activado por servomotor como símbolo de la neurodiversidad, y proyecciones visuales mediante videomapping que acompañan los fenómenos climáticos representados. Se puede alternar entre diferentes ciudades (Maldonado, París, Londres y Tokio), así como visualizar la descripción del clima o la probabilidad de lluvia.

---

## Objetivos

- Acceder a datos climáticos reales mediante ciencia de datos.
- Representar el clima futuro de forma tangible y sensorial.
- Favorecer la anticipación emocional en niños con TEA.
- Promover la comprensión del entorno mediante estímulos visuales amigables.
- Estimular la interacción a través de un sistema simple y atractivo.
- Desarrollar una solución accesible, portable y de bajo costo.

---

## Ciencia de Datos Aplicada

- **Obtención de datos:** Conexión del ESP8266 a la API de Open-Meteo.
- **Procesamiento:** Interpretación de datos meteorológicos por día y ciudad.
- **Representación sensorial:** Al presionar un botón, se enciende la figura 3D correspondiente, acompañada de proyecciones y datos en pantalla.

---

## Componentes

- **Microcontroladores:**
  - Arduino UNO
  - ESP8266 (módulo WiFi)

- **Display:**
  - Pantalla LCD 16x2

- **Entrada de usuario:**
  - Control remoto infrarrojo
  - Receptor IR

- **Salida visual:**
  - 5 matrices LED de 8x8 (iluminan figuras 3D)
  - Proyector para mapping dinámico
  - Figuras 3D impresas (con 3 cm de profundidad):
    - Sol
    - Nube
    - Nube con sol
    - Nube con lluvia (con tiras LED simulan gotas)
    - Nube con tormenta (posiblemente mappeado)

- **Elemento móvil:**
  - Servomotor (mueve arcoíris impreso en 3D)

- **Extras:**
  - Letras 3D opcionales (para mapping de título)

---

## Funcionalidades actuales

- Muestra el clima real de hasta seis días a futuro.
- Alterna entre descripción del clima y probabilidad de lluvia.
- Cambia entre 4 ciudades: Maldonado, París, Londres y Tokio.
- Activa un arcoíris móvil mediante control remoto.
- Muestra información visual en pantalla LCD.
- Realiza proyección visual mediante videomapping según el clima.

---

## Cambios respecto a versiones anteriores

- Se eliminó el uso de sonidos para evitar sobreestimulación sensorial.
- Ya no se requiere PC externa: el ESP8266 maneja toda la lógica y conexión a la API.
- Se integró un sistema de mapping visual.
- Las figuras 3D se optimizaron para mayor luminosidad (pasaron de 1 cm a 3 cm de grosor).
- Se reemplazaron los focos LED por matrices LED 8x8 para mejor iluminación.
- Se agregó el control de ciudades y tipos de datos mostrados desde el control remoto.

---

## Organización y gestión del proyecto

La planificación, avances y documentación del proyecto se realizaron mediante [Trello](https://trello.com/b/eOYLk6ht/proyecto-2), donde se registraron tareas, hitos y responsabilidades del equipo durante todo el desarrollo.

---

## Repositorio

Este repositorio contiene:

- Código Arduino del sistema completo (`.ino`)
- Archivos `.stl` para impresión 3D de figuras y letras
- Archivos de diseño del mapping
- Documentación de uso y presentación final del proyecto

---

## Créditos

Proyecto desarrollado en el marco del Taller Interdisciplinario de Ingeniería (TI3) – Universidad Católica del Uruguay.  
Estudiantes: De Los Santos, Nahuel; Gallinal, Facundo; Muñoz, Sofía; Nicora, Brahian 
Docentes:  Cilintano, Augusto; Duarte, Gonzalo; Serrano, Vanessa

