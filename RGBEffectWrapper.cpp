#include "RGBEffectWrapper.hpp"

#include "ESPixelStick.h"

#define MYMIN(x, y) ((x) < (y) ? (x) : (y))

//static const RGBEffect::PosArray sPosArray =
//    RGBEffect::posArrayFromLedArray({// Fairy Wings
//        -1, -1, -1, -1, -1, -1, -1, 26, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 13, -1, -1, -1, -1, -1, -1, -1,
//        -1, -1, -1, -1, -1, 27, -1, -1, -1, 25, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 14, -1, -1, -1, 12, -1, -1, -1, -1, -1,
//        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 24, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//        -1, -1, -1, 28, -1, -1, -1, -1, -1, -1, -1, -1, -1, 23, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 16, -1, -1, -1, -1, -1, -1, -1, -1, -1, 11, -1, -1, -1,
//        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 22, -1, -1, -1, -1, -1, -1, -1, -1, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//        -1, 29, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, -1, -1, -1, -1, 18, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, -1, -1,
//        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//        30, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 20, 19, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  9, -1, -1,
//        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//        -1, 31, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  8, -1, -1, -1,
//        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//        -1, -1, -1, 32, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  7, -1, -1, -1, -1, -1,
//        -1, -1, -1, -1, -1, 33, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  6, -1, -1, -1, -1, -1, -1, -1,
//        -1, -1, -1, -1, -1, -1, -1, 34, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//        -1, -1, -1, -1, -1, -1, -1, -1, -1, 35, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 36, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 37, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 38, -1, 39, -1, -1,  0, -1,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//        }, 40, 18);
/*
static const RGBEffect::PosArray sPosArray =
    RGBEffect::posArrayFromLedArray({// Eventail
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 17, -1, 18, -1, 19, -1, 20, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 16, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 22, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 23, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 24, -1, -1, -1, 28, 29, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 25, -1, -1, -1, 27, -1, -1, -1, -1, 30, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 26, -1, -1, -1, -1, -1, -1, -1, -1, 31, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 32, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 33, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         8, -1, -1, -1, -1, -1, -1, -1,  3, -1,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 34, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 35, -1, -1,
        -1,  7, -1, -1, -1, -1, -1,  4, -1, -1, -1,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 36, -1, -1, -1, -1,
        -1, -1, -1,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 37, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1,  5, -1, -1, -1, -1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 38, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 39, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        }, 40, 18);
*/
static const RGBEffect::PosArray sPosArray = RGBEffect::posArraySimple(LED_COUNT, 1);

struct EffectDesc
{
    RGBEffectPattern pattern;
    RGBEffectMixingMode mixingMode;
    int loopTime;
};

struct EffectComboDesc
{
    std::vector<EffectDesc> effects;
    std::vector<EffectDesc> strobeEffects;
};

EffectComboDesc sEffects[] = {
    {
        {
            {RGBEffectPattern::PLASMA, RGBEffectMixingMode::REPLACE, 8000}
        },
        {
            {RGBEffectPattern::STROBE, RGBEffectMixingMode::ADD, 8000}
        }
    },
    {
        {
            {RGBEffectPattern::ROTATION_SMOOTH, RGBEffectMixingMode::REPLACE, 8000}
        },
        {
            {RGBEffectPattern::STRIPE_V, RGBEffectMixingMode::ADD, 400},
        }
    },
    {
        {
            {RGBEffectPattern::ROTATION, RGBEffectMixingMode::REPLACE, 8000}
        },
        {
            {RGBEffectPattern::STROBE, RGBEffectMixingMode::ADD, 600},
        }
    },
    {
        {
            {RGBEffectPattern::PING_PONG_H, RGBEffectMixingMode::REPLACE, 6000}
        },
        {
            {RGBEffectPattern::PING_PONG_H, RGBEffectMixingMode::ADD, 2400},
        }
    },
};

struct ColorComboDesc
{
    std::vector<RGBEffectColor> colors;
    std::vector<RGBEffectColor> strobeColors;
};

ColorComboDesc sColors[] = {
    {
        {RGBEffectColor::FLAME},
        {RGBEffectColor::GOLD}
    },
    {
        {RGBEffectColor::OCEAN},
        {RGBEffectColor::WHITE}
    },
    {
        {RGBEffectColor::GRASS},
        {RGBEffectColor::GOLD}
    },
    {
        {RGBEffectColor::RAINBOW},
        {RGBEffectColor::WHITE}
    },
    {
        {RGBEffectColor::GOLD},
        {RGBEffectColor::WHITE}
    },
    {
        {RGBEffectColor::PINK},
        {RGBEffectColor::WHITE}
    },
};

void RGBEffectWrapper::begin(uint8_t* pixels, int pixelCount)
{
    LOG_PORT.println("begin 1");
    _pixels = pixels;
    _pixelCount = pixelCount;
    begin();
}

void RGBEffectWrapper::startFlash()
{
    _flashing = true;
}

void RGBEffectWrapper::stopFlash()
{
    _flashing = false;
}

void RGBEffectWrapper::nextMode()
{
    //_currentEffects.clear();
    //_currentStrobeEffects.clear();
    _currentEffectsIdx = (_currentEffectsIdx + 1) % (sizeof(sEffects) / sizeof(*sEffects));

    begin();
}

void RGBEffectWrapper::begin()
{
    LOG_PORT.println("begin 2");
    _currentEffects.clear();//(sEffects[_currentEffectsIdx].effects.size());
    _currentStrobeEffects.clear();//resize(sEffects[_currentEffectsIdx].strobeEffects.size());


    LOG_PORT.println("start effects");
    int idx = 0;
    for (auto& effectDesc : sEffects[_currentEffectsIdx].effects)
    {
        LOG_PORT.print("idx=");
        LOG_PORT.print(idx, DEC);
        LOG_PORT.println();
        int colorIdx = MYMIN(idx, sColors[_currentColorsIdx].colors.size());
        
        LOG_PORT.print("colorIdx=");
        LOG_PORT.print(colorIdx, DEC);
        LOG_PORT.println();
        LOG_PORT.print("_currentColorsIdx=");
        LOG_PORT.print(_currentColorsIdx, DEC);
        LOG_PORT.println();
        _currentEffects.push_back({});
        
        LOG_PORT.println("begincurrentEffect");
        _currentEffects[idx].begin(
                effectDesc.pattern,
                sColors[_currentColorsIdx].colors[colorIdx],
                effectDesc.mixingMode,
                _pixels,
                _pixelCount,
                sPosArray
                );
        LOG_PORT.println("setLoopTim");
        _currentEffects[idx].setLoopTime(effectDesc.loopTime);
        ++idx;
    }
    
    LOG_PORT.println("start strobe effects");
    idx = 0;
    for (auto& strobeEffectDesc : sEffects[_currentEffectsIdx].strobeEffects)
    {
        int colorIdx = MYMIN(idx, sColors[_currentColorsIdx].strobeColors.size());
        
        _currentStrobeEffects.push_back({});
        _currentStrobeEffects[idx].begin(
                strobeEffectDesc.pattern,
                sColors[_currentColorsIdx].strobeColors[colorIdx],
                strobeEffectDesc.mixingMode,
                _pixels,
                _pixelCount,
                sPosArray
                );
        _currentStrobeEffects[idx].setLoopTime(strobeEffectDesc.loopTime);
        ++idx;
    }
}

void RGBEffectWrapper::nextColor()
{
    _currentColorsIdx = (_currentColorsIdx + 1) % (sizeof(sColors) / sizeof(*sColors));

    int idx = 0;
    for (auto& effect : _currentEffects)
    {
        int colorIdx = MYMIN(idx, sColors[_currentColorsIdx].colors.size());
        effect.setColor(sColors[_currentColorsIdx].colors[colorIdx]);
        ++idx;
    }
    idx = 0;
    for (auto& strobeEffect : _currentStrobeEffects)
    {
        int colorIdx = MYMIN(idx, sColors[_currentColorsIdx].strobeColors.size());
        strobeEffect.setColor(sColors[_currentColorsIdx].strobeColors[colorIdx]);
        ++idx;
    }
}

bool RGBEffectWrapper::refreshPixels(unsigned long currentMillis)
{
    bool ret = false;

    for (auto& effect : _currentEffects)
        ret |= effect.refreshPixels(currentMillis);

    if (_flashing)
    {
        for (auto& strobeEffect : _currentStrobeEffects)
            ret |= strobeEffect.refreshPixels(currentMillis);
    }

    return ret;
}
