#include "Fixture.hpp"
#include "ESPixelStick.h"
#include "PixelDriver.h"

void Fixture::begin(PixelDriver* pixels)
{
    _pixels = pixels;
}

void Fixture::updateInput(uint8_t const* data, unsigned int size)
{
    if (size < getNumChannels())
    {
        LOG_PORT.println("Not enough bytes in data, abandon");
    }

    int r = data[0];
    int g = data[1];
    int b = data[2];
    unsigned int pixelIdx = 0;
    for (unsigned int i = 3; i < getNumChannels(); ++i)
    {
        pixels.setValue((i - 3) + 0, (r * data[i]) / 255);
        pixels.setValue((i - 3) + 1, (g * data[i]) / 255);
        pixels.setValue((i - 3) + 2, (b * data[i]) / 255);
    }
}

void Fixture::refreshPixels()
{
}

void Fixture::getNumChannels() const
{
    return 3 + _pixels->getNumPixels();
}
