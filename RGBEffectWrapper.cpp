#include "RGBEffectWrapper.hpp"

#include "ESPixelStick.h"

#define MYMIN(x, y) ((x) < (y) ? (x) : (y))

/**/
const RGBEffect::PosArray RGBEffectWrapper::posArray =
    RGBEffect::posArrayFromLedArray({// Fairy Wings
        -1, -1, -1, -1, -1, -1, -1, 26, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 13, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, 27, -1, -1, -1, 25, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 14, -1, -1, -1, 12, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 24, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, 28, -1, -1, -1, -1, -1, -1, -1, -1, -1, 23, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 16, -1, -1, -1, -1, -1, -1, -1, -1, -1, 11, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 22, -1, -1, -1, -1, -1, -1, -1, -1, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, 29, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, -1, -1, -1, -1, 18, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        30, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 20, 19, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  9, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, 31, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  8, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, 32, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  7, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, 33, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  6, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, 34, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, 35, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 36, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 37, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 38, -1, 39, -1, -1,  0, -1,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        }, 40, 18);
/**/
/** /
static const RGBEffect::PosArray posArray =
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
/ **/
/** /
static const RGBEffect::PosArray posArray =
    RGBEffect::posArrayFromLedArray({ // Jupe
        -1, -1, -1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 39, -1, 38, -1, -1,
        -1, -1, -1, -1, -1, -1,  2, -1,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, -1, 37,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1,  9, -1, 11, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 36,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  6, -1,  7, -1,  8, -1, -1, -1, 12, -1,
        23, -1, 22, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 35, -1,
        -1, -1, -1, -1, 21, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 13, -1, -1, -1,
        24, -1, -1, -1, -1, -1, 20, -1, 19, -1, 18, -1, 17, -1, 16, -1, 15, -1, 14, -1, 34, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, 25, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 33, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, 26, -1, 27, -1, 28, -1, 29, -1, 30, -1, 31, -1, 32, -1, -1, -1, -1, -1, -1, -1,
        }, 24, 12);
/ **/
//static const RGBEffect::PosArray posArray = RGBEffect::posArraySimple(LED_COUNT, 1);

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
            {RGBEffectPattern::PLASMA, RGBEffectMixingMode::REPLACE, 4000}
        },
        {
            {RGBEffectPattern::STROBE, RGBEffectMixingMode::MAX, 160}
        }
    },
    {
        {
            {RGBEffectPattern::PLASMA, RGBEffectMixingMode::REPLACE, 4000},
            {RGBEffectPattern::STRIPE_SMOOTH_H_LEFT_RIGHT, RGBEffectMixingMode::SUB, 1700}
        },
        {
            {RGBEffectPattern::SMOOTHER_ON_OFF, RGBEffectMixingMode::MAX, 160}
        }
    },
    {
        {
            {RGBEffectPattern::ROTATION_SMOOTH, RGBEffectMixingMode::ADD, 1700},
            {RGBEffectPattern::ROTATION_SMOOTH_THIN, RGBEffectMixingMode::SUB, 1500}
        },
        {
            {RGBEffectPattern::ROTATION, RGBEffectMixingMode::GRAINMERGE, 400}
        }
    },
    {
        {
            {RGBEffectPattern::STRIPE_SMOOTH_V_DOWN_UP, RGBEffectMixingMode::REPLACE, 1700},
            {RGBEffectPattern::STRIPE_V_DOWN_UP, RGBEffectMixingMode::MAX, 1200}
        },
        {
            {RGBEffectPattern::PING_PONG_SMOOTH_V, RGBEffectMixingMode::MAX, 280}
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
        {
            RGBEffectColor::OCEAN,
            RGBEffectColor::WHITE
        },
        {
            RGBEffectColor::WHITE
        }
    },
    {
        {
            RGBEffectColor::FLAME,
            RGBEffectColor::WHITE
        },
        {
            RGBEffectColor::WHITE
        }
    },
    {
        {
            RGBEffectColor::GRASS,
            RGBEffectColor::WHITE
        },
        {
            RGBEffectColor::WHITE
        }
    },
    {
        {
            RGBEffectColor::RAINBOW,
            RGBEffectColor::WHITE
        },
        {
            RGBEffectColor::WHITE
        }
    },
    {
        {
            RGBEffectColor::PINK,
            RGBEffectColor::WHITE
        },
        {
            RGBEffectColor::WHITE
        }
    },
    {
        {
            RGBEffectColor::GOLD,
            RGBEffectColor::WHITE
        },
        {
            RGBEffectColor::WHITE
        }
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
    _currentEffectsIdx = (_currentEffectsIdx + 1) % (sizeof(sEffects) / sizeof(*sEffects));

    begin();
}

void RGBEffectWrapper::begin()
{
    LOG_PORT.println("begin 2");
    _currentEffects.clear();
    _currentStrobeEffects.clear();


    LOG_PORT.println("start effects");
    unsigned int idx = 0;
    for (auto& effectDesc : sEffects[_currentEffectsIdx].effects)
    {
        LOG_PORT.print("idx=");
        LOG_PORT.print(idx, DEC);
        LOG_PORT.println();
        unsigned int colorIdx = MYMIN(idx, sColors[_currentColorsIdx].colors.size());

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
                posArray
                );
        LOG_PORT.println("setLoopTim");
        _currentEffects[idx].setLoopTime(effectDesc.loopTime);
        ++idx;
    }

    LOG_PORT.println("start strobe effects");
    idx = 0;
    for (auto& strobeEffectDesc : sEffects[_currentEffectsIdx].strobeEffects)
    {
        unsigned int colorIdx = MYMIN(idx, sColors[_currentColorsIdx].strobeColors.size());

        _currentStrobeEffects.push_back({});
        _currentStrobeEffects[idx].begin(
                strobeEffectDesc.pattern,
                sColors[_currentColorsIdx].strobeColors[colorIdx],
                strobeEffectDesc.mixingMode,
                _pixels,
                _pixelCount,
                posArray
                );
        _currentStrobeEffects[idx].setLoopTime(strobeEffectDesc.loopTime);
        ++idx;
    }
}

void RGBEffectWrapper::nextColor()
{
    _currentColorsIdx = (_currentColorsIdx + 1) % (sizeof(sColors) / sizeof(*sColors));

    unsigned int idx = 0;
    for (auto& effect : _currentEffects)
    {
        unsigned int colorIdx = MYMIN(idx, sColors[_currentColorsIdx].colors.size());
        effect.setColor(sColors[_currentColorsIdx].colors[colorIdx]);
        ++idx;
    }
    idx = 0;
    for (auto& strobeEffect : _currentStrobeEffects)
    {
        unsigned int colorIdx = MYMIN(idx, sColors[_currentColorsIdx].strobeColors.size());
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
