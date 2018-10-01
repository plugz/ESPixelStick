#ifndef __RGBEFFECTWRAPPERDMX_HPP__
#define __RGBEFFECTWRAPPERDMX_HPP__

#include "RGBEffectWrapper.hpp"

// Channels:
// PATTERN
// COLOR
// SPEED
// MIXMODE
// -----> 4 channels per RGBEffect
// -----> 94/4 -> 23 RGBEffects max

class RGBEffectWrapperDMX
{
public:
    void begin(uint8_t* pixels, int pixelCount);
    void setInputDMX(uint8_t const* data, unsigned int size);
    bool refreshPixels(unsigned long currentMillis);

private:
    void begin();
    uint8_t* _pixels;
    unsigned int _pixelCount;
    uint8_t _currentData[112];
    unsigned int _currentDataSize;
    std::vector<RGBEffect> _currentEffects;
};

#endif

