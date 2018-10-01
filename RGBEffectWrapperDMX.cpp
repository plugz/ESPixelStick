#include "RGBEffectWrapperDMX.hpp"

#include "ESPixelStick.h"

#define MYMIN(x, y) ((x) < (y) ? (x) : (y))

void RGBEffectWrapperDMX::begin(uint8_t* pixels, int pixelCount)
{
    LOG_PORT.println("RGBEffectWrapperDMX::begin");
    _pixels = pixels;
    _pixelCount = pixelCount;
    memset(_currentData, 0, sizeof(_currentData));
    _currentDataSize = 0;
}

void RGBEffectWrapperDMX::begin()
{
    _currentEffects.clear();

    for (unsigned int i = 0; i < _currentDataSize / 4; ++i)
    {
        unsigned int pattern = _currentData[i * 4 + 0];
        unsigned int color = _currentData[i * 4 + 1];
        unsigned int speed = _currentData[i * 4 + 2];
        unsigned int mixMode = _currentData[i * 4 + 3];

        // pattern == 0 means skip to the next one
        if (pattern == 0)
            continue;
        pattern -= 1;

        if (pattern >= (unsigned int)RGBEffectPattern::COUNT)
            pattern = 0;
        if (color >= (unsigned int)RGBEffectColor::COUNT)
            color = 0;
        if (mixMode >= (unsigned int)RGBEffectMixingMode::COUNT)
            mixMode = 0;

        // 50ms < loopTime < 12800ms
        unsigned int loopTime = (256 - speed) * 50;

        _currentEffects.push_back({});

        LOG_PORT.println("begincurrentEffect");
        _currentEffects.back().begin(
                (RGBEffectPattern)pattern,
                (RGBEffectColor)color,
                (RGBEffectMixingMode)mixMode,
                _pixels,
                _pixelCount,
                RGBEffectWrapper::posArray
                );
        LOG_PORT.println("setLoopTime");
        _currentEffects.back().setLoopTime(loopTime);
    }
}

void RGBEffectWrapperDMX::setInputDMX(uint8_t const* data, unsigned int size)
{
    if (size != _currentDataSize || memcmp(data, _currentData, size))
    {
        memcpy(_currentData, data, size);
        begin();
    }
}

bool RGBEffectWrapperDMX::refreshPixels(unsigned long currentMillis)
{
    bool ret = false;

    for (auto& effect : _currentEffects)
        ret |= effect.refreshPixels(currentMillis);

    return ret;
}

