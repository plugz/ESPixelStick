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
        setAlwaysOn();
    }
}

void Fixture::setAlwaysOn() {
    for (int pixelIdx : {0, 40, 80}) {
        _pixels->setValue(pixelIdx * 3 + 0, 255);
        _pixels->setValue(pixelIdx * 3 + 1, 255);
        _pixels->setValue(pixelIdx * 3 + 2, 255);
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
    PLAINBLINK,
    PLAINFADE,
    SNAKE,
    SNAKEFADE
};

static DemoMode demoModes[] = {
    DemoMode::PLAINSWITCH,
    DemoMode::PLAINFADE,
    DemoMode::PLAINBLINK,
    DemoMode::PLAINFADE,
//    DemoMode::SNAKE,
//    DemoMode::SNAKEFADE
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

static DemoColor demoColors[] = {
//    DemoColor::RED,
//    DemoColor::GREEN,
//    DemoColor::BLUE,
//    DemoColor::YELLOW,
//    DemoColor::CYAN,
//    DemoColor::MAGENTA,
    DemoColor::WHITE
};

// 10000ms switch
#define DEMO_SWITCHTIME 10000

void Fixture::refreshPixelsDemo()
{
    long int currentMillis = millis();
    long int colorAdvance = currentMillis - demoPrevColorMillis;
    demoCurrentColorIdx += colorAdvance / DEMO_SWITCHTIME;
    demoCurrentColorIdx %= ARRAYSIZE(demoColors);
    demoPrevColorMillis += (colorAdvance / DEMO_SWITCHTIME) * DEMO_SWITCHTIME;
    colorAdvance -= (colorAdvance / DEMO_SWITCHTIME) * DEMO_SWITCHTIME;
    long int modeAdvance = currentMillis - demoPrevModeMillis;
    demoCurrentModeIdx += modeAdvance / (DEMO_SWITCHTIME * ARRAYSIZE(demoColors));
    demoCurrentModeIdx %= ARRAYSIZE(demoModes);
    demoPrevModeMillis += (modeAdvance / (DEMO_SWITCHTIME * ARRAYSIZE(demoColors))) * (DEMO_SWITCHTIME * ARRAYSIZE(demoColors));

    switch (demoModes[demoCurrentModeIdx])
    {
        case DemoMode::PLAINSWITCH:
            refreshPixelsDemoPlainSwitch(colorAdvance);
            break;
        case DemoMode::PLAINBLINK:
            refreshPixelsDemoPlainBlink(colorAdvance);
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

void Fixture::refreshPixelsDemoPlainBlink(int colorAdvance)
{
    int red = ((int)demoColors[demoCurrentColorIdx] >> 16) & 0xff;
    int green = ((int)demoColors[demoCurrentColorIdx] >> 8) & 0xff;
    int blue = ((int)demoColors[demoCurrentColorIdx]) & 0xff;
    red = red * (((colorAdvance * 10) / DEMO_SWITCHTIME) % 2);
    green = green * (((colorAdvance * 10) / DEMO_SWITCHTIME) % 2);
    blue = blue * (((colorAdvance * 10) / DEMO_SWITCHTIME) % 2);
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
    red = (red * ((DEMO_SWITCHTIME / 2) - std::abs((DEMO_SWITCHTIME / 2) - colorAdvance))) / (DEMO_SWITCHTIME / 2);
    green = (green * ((DEMO_SWITCHTIME / 2) - std::abs((DEMO_SWITCHTIME / 2) - colorAdvance))) / (DEMO_SWITCHTIME / 2);
    blue = (blue * ((DEMO_SWITCHTIME / 2) - std::abs((DEMO_SWITCHTIME / 2) - colorAdvance))) / (DEMO_SWITCHTIME / 2);
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
    int position = (_pixels->getNumPixels() * ((DEMO_SWITCHTIME / 2) - std::abs((DEMO_SWITCHTIME / 2) - colorAdvance))) / (DEMO_SWITCHTIME / 2);
    for (unsigned int i = 0; i < _pixels->getNumPixels(); ++i)
    {
        if (std::abs(position - (int)i) < 4)
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
    int position = (_pixels->getNumPixels() * colorAdvance) / DEMO_SWITCHTIME;
    for (unsigned int i = 0; i < _pixels->getNumPixels(); ++i)
    {
        int r = (red * (_pixels->getNumPixels() - std::abs(position - (int)i))) / _pixels->getNumPixels();
        int g = (green * (_pixels->getNumPixels() - std::abs(position - (int)i))) / _pixels->getNumPixels();
        int b = (blue * (_pixels->getNumPixels() - std::abs(position - (int)i))) / _pixels->getNumPixels();
        _pixels->setValue(i * 3 + 0, r);
        _pixels->setValue(i * 3 + 1, g);
        _pixels->setValue(i * 3 + 2, b);
    }
}
