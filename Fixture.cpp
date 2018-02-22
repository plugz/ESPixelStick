#include "Fixture.hpp"
#include "ESPixelStick.h"
#include "PixelDriver.h"

void Fixture::begin(PixelDriver* pixels)
{
    _pixels = pixels;
    _mode = FixtureMode::R_G_B_LEVELS;
}

void Fixture::updateInput(uint8_t const* data, unsigned int size)
{
    if (size < getNumChannels())
    {
        LOG_PORT.println("Not enough bytes in data, abandon");
    }

    switch (_mode)
    {
        case FixtureMode::SIMPLE:
        default:
            updateInputSimple(data);
            break;
        case FixtureMode::R_G_B_LEVELS:
            updateInputRGBLevels(data);
            break;
    }
}

void Fixture::updateInputSimple(uint8_t const* data)
{
    for (unsigned int i = 0; i < getNumChannels(); ++i)
    {
        _pixels->setValue(i, data[i]);
    }
}

void Fixture::updateInputRGBLevels(uint8_t const* data)
{
    int r = data[0];
    int g = data[1];
    int b = data[2];
    unsigned int pixelIdx = 0;
    for (unsigned int i = 0; i < getNumChannels() - 3; ++i)
    {
        int brightness = data[i + 3];
        _pixels->setValue(i * 3 + 0, (r * brightness) / 255);
        _pixels->setValue(i * 3 + 1, (g * brightness) / 255);
        _pixels->setValue(i * 3 + 2, (b * brightness) / 255);
    }
}

void Fixture::refreshPixels()
{
}

unsigned int Fixture::getNumChannels() const
{
    switch (_mode)
    {
        case FixtureMode::SIMPLE:
        default:
            return _pixels->getNumPixels() * 3;
        case FixtureMode::R_G_B_LEVELS:
            return 3 + _pixels->getNumPixels();
    }
}
