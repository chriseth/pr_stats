#include <NeoPixelBus.h>

#include "pr.h"

#include <map>

#define LED_PIN     13
#define NUM_LEDS    120
#define BRIGHTNESS  255


class LEDs {
  public:
    void setup() {
        m_leds.Begin();
    }
    void render(std::vector<PR> const& _prs);
    void renderEmpty();
    RgbColor statusToColor(PRState _state) const;
    double flashFactor() const;
    static int piecewiseLinear(int x, int _x0, int _y0, int _x1, int _y1, int _x2, int _y2, int _x3, int _y3);

  private:
    NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> m_leds{NUM_LEDS, LED_PIN};
    int m_millis;
};

void LEDs::render(std::vector<PR> const& _prs)
{
    m_millis = millis();
    m_leds.ClearTo(RgbColor(0, 0, 0));
    if (_prs.empty())
        renderEmpty();
    else
    {
        int now = time(nullptr);
        int yesterday = now - 3600 * 24;
        int weekAgo = now - 3600 * 24 * 7;
        int minTime = now;
        for (PR const& pr: _prs)
            minTime = std::min(minTime, pr.createdAt);

        int currentLED = NUM_LEDS;
        for (PR const& pr: _prs)
        {
            int index = piecewiseLinear(
                pr.createdAt,
                minTime, 0,
                weekAgo, NUM_LEDS / 4,
                yesterday, NUM_LEDS * 3 / 4,
                now, NUM_LEDS
            );
            if (index >= currentLED)
                index = currentLED - 1;
            currentLED = index;
            if (currentLED < 0)
                break;
            RgbColor color = statusToColor(pr.combinedStatus);
            m_leds.SetPixelColor(currentLED, color);
            //Serial.printf("Setting LED %d to %d, %d, %d\n", currentLED, color.R, color.G, color.B);
        }
    }
    m_leds.Show();
}

void LEDs::renderEmpty()
{
    double speed = 50;
    int index = int(m_millis / speed) % NUM_LEDS;
    int rem = int(256 * ((m_millis / speed) - int(m_millis / speed)));
    m_leds.SetPixelColor(index, RgbColor(255, 0, 0));
    m_leds.SetPixelColor((index + 1) % NUM_LEDS, RgbColor(rem, 0, 0));
    m_leds.SetPixelColor((index + NUM_LEDS - 1) % NUM_LEDS, RgbColor(255 - rem, 0, 0));
    m_leds.Show();
}


double LEDs::flashFactor() const
{
    double factor = sin(m_millis / 300.0);
    if (factor < 0)
        factor = 0;
    return factor;
}

RgbColor LEDs::statusToColor(PRState _state) const
{
    switch (_state)
    {
    case PRState::MERGEABLE:
        return RgbColor(0, 0, 255 * flashFactor());
    case PRState::WAITING_FOR_TESTS:
        return RgbColor(128, 128, 0);
    case PRState::AUTHOR:
        return RgbColor(255 * flashFactor(), 0, 0);
    case PRState::REVIEWER:
        return RgbColor(128 * flashFactor(), 128 * flashFactor(), 0);
    case PRState::ERROR:
        return RgbColor(255, 255, 255);
    }
}

int LEDs::piecewiseLinear(int x, int _x0, int _y0, int _x1, int _y1, int _x2, int _y2, int _x3, int _y3)
{
    if (x < _x0)
        return _y0;
    else if (x < _x1)
        return double(x - _x0) / (_x1 - _x0) * (_y1 - _y0) + _y0;
    else if (x < _x2)
        return double(x - _x1) / (_x2 - _x1) * (_y2 - _y1) + _y1;
    else if (x < _x3)
        return double(x - _x2) / (_x3 - _x2) * (_y3 - _y2) + _y2;
    else
        return _y3;
}
