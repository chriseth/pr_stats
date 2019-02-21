#include "leds.h"
#include "prs.h"


PRs prs;
LEDs leds;

void setup() {
  delay(200); // power-up safety delay
  leds.setup();

  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  while (!Serial) {};

  Serial.println();

  prs.setup();

  Serial.println("Ready.");
}

void loop()
{
  //  prs.render();
  prs.update();
  leds.render(prs.prs);
  //if (Serial.available())
  //  Github::parseResponse(Serial);
  // Serial.println();
  // Serial.println("Waiting 10s before the next round...");
  delay(100);
}  
