#include "Fixture.hpp"
#include "ESPixelStick.h"
#include "PixelDriver.h"

#include <iterator>

#define ARRAYSIZE(array) (sizeof(array) / sizeof(*array))

void Fixture::begin(PixelDriver* pixels)
{
    _pixels = pixels;
    _mode = FixtureMode::DEMO;
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
        case FixtureMode::DEMO:
            updateInputDemo(data);
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
    for (unsigned int i = 0; i < getNumChannels() - 3; ++i)
    {
        int brightness = data[i + 3];
        _pixels->setValue(i * 3 + 0, (r * brightness) / 255);
        _pixels->setValue(i * 3 + 1, (g * brightness) / 255);
        _pixels->setValue(i * 3 + 2, (b * brightness) / 255);
    }
}

void Fixture::updateInputDemo(uint8_t const*)
{
}

void Fixture::refreshPixels()
{
    unsigned long currentMillis = millis();
    if (currentMillis - _prevUpdateMillis > 20) // 50fps
    {
        _prevUpdateMillis += 20;
        switch (_mode)
        {
            case FixtureMode::SIMPLE:
            default:
            case FixtureMode::R_G_B_LEVELS:
                break;
            case FixtureMode::DEMO:
                refreshPixelsDemo();
                break;
        }
    }
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
        case FixtureMode::DEMO:
            return 1;
    }
}

enum class DemoMode
{
    PLAINSWITCH,
    PLAINFADE,
    SNAKE,
    SNAKEFADE
};

DemoMode demoModes[] = {
    DemoMode::PLAINSWITCH,
    DemoMode::PLAINFADE,
    DemoMode::SNAKE,
    DemoMode::SNAKEFADE
};

enum class DemoColor
{
    RED = 0xff0000,
    YELLOW = 0xffff00,
    GREEN = 0x00ff00,
    CYAN = 0x00ffff,
    BLUE = 0x0000ff,
    MAGENTA = 0xff00ff,
    WHITE = 0xffffff,
};

DemoColor demoColors[] = {
    DemoColor::RED,
    DemoColor::YELLOW,
    DemoColor::GREEN,
    DemoColor::CYAN,
    DemoColor::BLUE,
    DemoColor::MAGENTA,
    DemoColor::WHITE
};

// 5000ms switch
void Fixture::refreshPixelsDemo()
{
    long int currentMillis = millis();
    long int colorAdvance = currentMillis - demoPrevColorMillis;
    demoCurrentColorIdx += colorAdvance / 5000;
    demoCurrentColorIdx %= ARRAYSIZE(demoColors);
    demoPrevColorMillis += (colorAdvance / 5000) * 5000;
    long int modeAdvance = currentMillis - demoPrevModeMillis;
    demoCurrentModeIdx += modeAdvance / (5000 * ARRAYSIZE(demoColors));
    demoCurrentModeIdx %= ARRAYSIZE(demoModes);
    demoPrevModeMillis += (modeAdvance / (5000 * ARRAYSIZE(demoColors))) * (5000 * ARRAYSIZE(demoColors));

    switch (demoModes[demoCurrentModeIdx])
    {
        case DemoMode::PLAINSWITCH:
            refreshPixelsDemoPlainSwitch(colorAdvance);
            break;
        case DemoMode::PLAINFADE:
            refreshPixelsDemoPlainFade(colorAdvance);
            break;
        case DemoMode::SNAKE:
            refreshPixelsDemoSnake(colorAdvance);
            break;
        case DemoMode::SNAKEFADE:
            refreshPixelsDemoSnakeFade(colorAdvance);
            break;
    }
}

void Fixture::refreshPixelsDemoPlainSwitch(int)
{
    int red = ((int)demoColors[demoCurrentColorIdx] >> 16) & 0xff;
    int green = ((int)demoColors[demoCurrentColorIdx] >> 8) & 0xff;
    int blue = ((int)demoColors[demoCurrentColorIdx]) & 0xff;
    for (unsigned int i = 0; i < _pixels->getNumPixels(); ++i)
    {
        _pixels->setValue(i * 3 + 0, red);
        _pixels->setValue(i * 3 + 1, green);
        _pixels->setValue(i * 3 + 2, blue);
    }
}

void Fixture::refreshPixelsDemoPlainFade(int colorAdvance)
{
    int red = ((int)demoColors[demoCurrentColorIdx] >> 16) & 0xff;
    int green = ((int)demoColors[demoCurrentColorIdx] >> 8) & 0xff;
    int blue = ((int)demoColors[demoCurrentColorIdx]) & 0xff;
    red = (red * (2500 - std::abs(2500 - colorAdvance))) / 2500;
    green = (green * (2500 - std::abs(2500 - colorAdvance))) / 2500;
    blue = (blue * (2500 - std::abs(2500 - colorAdvance))) / 2500;
    for (unsigned int i = 0; i < _pixels->getNumPixels(); ++i)
    {
        _pixels->setValue(i * 3 + 0, red);
        _pixels->setValue(i * 3 + 1, green);
        _pixels->setValue(i * 3 + 2, blue);
    }
}

void Fixture::refreshPixelsDemoSnake(int colorAdvance)
{
    int red = ((int)demoColors[demoCurrentColorIdx] >> 16) & 0xff;
    int green = ((int)demoColors[demoCurrentColorIdx] >> 8) & 0xff;
    int blue = ((int)demoColors[demoCurrentColorIdx]) & 0xff;
    int position = (_pixels->getNumPixels() * (2500 - std::abs(2500 - colorAdvance))) / 2500;
    for (unsigned int i = 0; i < _pixels->getNumPixels(); ++i)
    {
        if (std::abs(position - i) < 4)
        {
            _pixels->setValue(i * 3 + 0, red);
            _pixels->setValue(i * 3 + 1, green);
            _pixels->setValue(i * 3 + 2, blue);
        }
        else
        {
            _pixels->setValue(i * 3 + 0, 0);
            _pixels->setValue(i * 3 + 1, 0);
            _pixels->setValue(i * 3 + 2, 0);
        }
    }
}

void Fixture::refreshPixelsDemoSnakeFade(int colorAdvance)
{
    int red = ((int)demoColors[demoCurrentColorIdx] >> 16) & 0xff;
    int green = ((int)demoColors[demoCurrentColorIdx] >> 8) & 0xff;
    int blue = ((int)demoColors[demoCurrentColorIdx]) & 0xff;
    int position = (_pixels->getNumPixels() * colorAdvance) / 5000;
    for (unsigned int i = 0; i < _pixels->getNumPixels(); ++i)
    {
        int r = (red * std::abs(position - i)) / _pixels->getNumPixels();
        int g = (red * std::abs(position - i)) / _pixels->getNumPixels();
        int b = (red * std::abs(position - i)) / _pixels->getNumPixels();
        _pixels->setValue(i * 3 + 0, r);
        _pixels->setValue(i * 3 + 1, g);
        _pixels->setValue(i * 3 + 2, b);
    }
}
