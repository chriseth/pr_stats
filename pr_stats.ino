#include <FastLED.h>

#define LED_PIN     5
#define NUM_LEDS    120
#define BRIGHTNESS  255
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

void setup() {
    Serial.setTimeout(50);
    Serial.begin(9600);
    while (!Serial) {};

    delay(200); // power-up safety delay
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setDither(DISABLE_DITHER);
    FastLED.setBrightness(BRIGHTNESS);

    Serial.println("Ready.");
}

struct PRStatus {
    PRStatus() {}
    PRStatus(int _id, int _time, CRGB const& _color, bool _flash):
        id(_id), time(_time), color(_color), flash(_flash), lastUpdate(0)
    {}
    static PRStatus fromSerial() {
      int values[6] = {0, 0, 0, 0, 0, 0};
      for (int i = 0; i < 6; i++) {
        values[i] = Serial.parseInt();
        if (Serial.peek() == '\n' || Serial.peek() == ',')
          break;
      }

      /*
      Serial.print("-> ");
      for (int i = 0; i < 6; i++) {
        Serial.print(values[i]);
        Serial.print(" ");
      }
      Serial.println("");
      */
      return PRStatus(values[0], values[1], CRGB(values[2], values[3], values[4]), values[5] > 0);
    }
        
    int id = 0;
    int time = 0;
    CRGB color;
    int lastUpdate = 0;
    bool flash = false;
    bool operator ==(PRStatus const& _other) {
      return id == _other.id && color == _other.color && flash == _other.flash;
    }
    bool operator!=(PRStatus const& _other) { return !operator==(_other); }
};

struct PRs {
    PRs() {
        active = 2;
        status[0].color = CRGB(255, 0, 0);
        status[0].flash = true;
        status[0].time = 500;
        status[1].color = CRGB(0, 0, 255);
        status[1].flash = true;
        status[1].time = 990;
    }
    void readStatus()
    {
        if (Serial.peek() < '0' || Serial.peek() > '9')
        {
            Serial.read();
            return;
        }
        Serial.println("Receiving data...");
        int now = millis();
        for (active = 0; active < 100;) {
            // TODO actually compare by id
            PRStatus s = PRStatus::fromSerial();
            if (s.id == status[active].id && s != status[active])
                s.lastUpdate = now;
            status[active] = s;
            active++;

            if (Serial.peek() == '\n')
            {
                Serial.read();
                Serial.print("Got ");
                Serial.print(active);
                Serial.println(" LEDs.");
                return;
            }
        }
        for (int i = active; i < 100; i++)
            status[i] = PRStatus();
        Serial.println("Overflow.");
    }
    void render()
    {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        int lastLED = NUM_LEDS;
        int now = millis();
        for (int i = active - 1; i >= 0; --i)
        {
            int led = int(float(status[i].time) / 1000.0 * NUM_LEDS);
            if (led >= lastLED)
                led = lastLED - 1;
            lastLED = led;
            if (led < 0)
                break;
            CRGB c = status[i].color;
            float factor = .55;
            if (status[i].flash) {
                factor = sin(now / 300.0) * .40 + .55;
            }
            if (now >= status[i].lastUpdate + 1000.0 * 20)
                factor *= .3;

            c.r = c.r * factor;
            c.g = c.g * factor;
            c.b = c.b * factor;
            leds[led] = c;
        }
        FastLED.show();
    }

    PRStatus status[100];
    int active = 0;
};

PRs prs;

void loop()
{
    if (Serial.available())
        prs.readStatus();
    prs.render();
}  

