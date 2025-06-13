#Proyecto2UCU
Panel Sensorial del Clima — Ciencia de Datos e Inclusión Sensorial

**Descripción**
Este proyecto combina ciencia de datos, electrónica y diseño inclusivo para ayudar a niños con Trastorno del Espectro Autista (TEA) a anticipar y comprender el clima mediante estímulos sensoriales visuales. El sistema permite representar el clima estimado para los próximos siete días (el día actual y seis días posteriores), de forma accesible y lúdica.
Gracias a un módulo Arduino con conexión WiFi, el dispositivo se conecta a internet, obtiene la dirección IP y accede a datos meteorológicos reales desde una API. Luego, interpreta la información y activa estímulos visuales asociados a cada estado del clima, facilitando la comprensión mediante objetos impresos en 3D y un arcoíris móvil como homenaje simbólico a la neurodiversidad.

**Objetivos**
- Acceder a datos climáticos reales y predecibles mediante ciencia de datos.
- Representar el clima futuro de forma visual y tangible.
- Favorecer la anticipación y la organización emocional en niños con TEA.
- Promover la comprensión del entorno mediante recursos sensoriales visuales.
- Estimular la interacción a través de un sistema sencillo y atractivo.

**Ciencia de Datos Aplicada**
- Obtención de datos: El módulo Arduino con WiFi se conecta a una API meteorológica.
- Procesamiento: El código incorporado interpreta los datos y asocia el estado climático a un día específico.
- Representación sensorial: Al presionar un botón del control remoto, se activa el estímulo correspondiente al día seleccionado.

**Componentes**
- Microcontrolador: Arduino UNO con módulo WiFi
- Control remoto por infrarrojo: Para seleccionar el día deseado
- Receptor IR: Conectado al Arduino
- Luces LED RGB: Para iluminar objetos según el clima
- Servomotor: Para mover un arcoíris 3D como símbolo del TEA (movimiento de 180°)
- **Objetos impresos en 3D:**
Sol
Nube
Nube con sol
Nube con lluvia (con tiras LED para simular la lluvia)
Nube con granizo
- Proyector para Mapping: Representación animada del cambio climático (ej. sol saliendo/ocultándose)
- Letras impresas en 3D (opcional): Para proyectar sobre ellas y reforzar el título del proyecto

**Diferencias respecto a versiones anteriores**
Ya no se representan sonidos.
El sistema no muestra días anteriores, solo el día actual y los 6 siguientes.
Se eliminó la necesidad de una PC externa: todo el proceso ocurre en el Arduino con WiFi.
Se incorpora un arcoíris móvil como símbolo del TEA.
