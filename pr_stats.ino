#include "leds.h"
#include "prs.h"


LEDs leds;
PRs prs;

void setup() {
  delay(200);

  xTaskCreatePinnedToCore(wifiLoop, "Wifi Task", 8192 * 2, NULL, 1, NULL, 0);

  delay(100);

  leds.setup();
}


void wifiLoop(void* _param)
{
  delay(100);
  prs.setup();
  while (true)
  {
    prs.update();
    delay(1000);
  }
}

int sequenceNumber = -1;
std::vector<PR> prStats;
void loop()
{
  sequenceNumber = prs.updatedPRs(sequenceNumber, prStats);
  for (int i = 0; i < 100; i++)
    leds.render(prStats);
  delay(10);
}  
