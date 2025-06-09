
#include <Adafruit_NeoPixel.h>

#define PIN        13  // Pin digital donde conectaste la entrada de datos del NeoPixel
#define NUMPIXELS  4  // Número de LEDs en tu tira

// Cuando se declaran objetos NeoPixel globales, se inicializan antes de que se ejecute setup().
// Para inicializar correctamente NUMPIXELS, debe ser una constante o macro.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 500 // Tiempo de espera entre cambios de color en milisegundos



void setup() {
   pixels.begin(); // Inicializa la biblioteca NeoPixel.
}
   NeoPixel.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
}
  


 

void loop() {
   
}pixels.clear(); // Establece todos los píxeles a 'apagado'

  // El primer píxel se enciende en rojo.
  pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.show();   // Envía los colores actualizados a los píxeles.
  delay(DELAYVAL); // Espera

  // El segundo píxel se enciende en verde.
  pixels.setPixelColor(1, pixels.Color(0, 255, 0));
  pixels.show();
  delay(DELAYVAL);

  // El tercer píxel se enciende en azul.
  pixels.setPixelColor(2, pixels.Color(0, 0, 255));
  pixels.show();
  delay(DELAYVAL);

  // El cuarto píxel se enciende en blanco.
  pixels.setPixelColor(3, pixels.Color(255, 255, 255));
  pixels.show();
  delay(DELAYVAL);

  // Apaga todos los píxeles para el siguiente ciclo
  pixels.clear();
  pixels.show();
  delay(DELAYVAL);

  }
}