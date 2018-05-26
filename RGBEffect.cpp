#include "RGBEffect.hpp"
#include "Perlin.hpp"

#include <algorithm>
#include <iterator>

#include "ESPixelStick.h"
#ifdef abs
#undef abs
#endif
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#define TARGET_FREQ_HZ 50

void RGBEffect::begin(RGBEffectPattern pattern,
                      RGBEffectColor color,
                      RGBEffectMixingMode mixingMode,
                      uint8_t *pixels,
                      unsigned int pixelCount,
                      PosArray const& posArray)
{
  
        LOG_PORT.println("RGBE::begin");
    _pattern = pattern;
    _color = color;
    setMixingMode(mixingMode);
    _pixels = pixels;
    _pixelCount = pixelCount;

    if (!posArray.array.empty())
        _posArray = posArray;
    else
        _posArray = posArraySimple(pixelCount, 1);

    beginCurrentCombo();
}

RGBEffect::PosArray RGBEffect::posArraySimple(unsigned int width, unsigned int height)
{
    PosArray posArray;
    posArray.width = width;
    posArray.height = height;
    for (unsigned int i = 0; i < width * height; ++i)
        posArray.array.emplace_back(i);
    return posArray;
}

RGBEffect::PosArray RGBEffect::posArrayZigZag(unsigned int width, unsigned int height)
{
    PosArray posArray;
    posArray.width = width;
    posArray.height = height;
    for (unsigned int i = 0; i < width * height; ++i)
    {
        if ((i / width) % 2)
            posArray.array.emplace_back((i / width) * width + ((width - 1) - (i % width)));
        else
            posArray.array.emplace_back(i);
    }
    return posArray;
}

RGBEffect::PosArray RGBEffect::posArrayFromLedArray(std::vector<int> const& ledArray, unsigned int width, unsigned int height)
{
    PosArray posArray;
    posArray.width = width;
    posArray.height = height;

    int ledMax = 0;
    for (auto led : ledArray)
    {
        if (led > ledMax)
            ledMax = led;
    }
    const int ledCount = ledMax + 1;

    for (int i = 0; i < ledCount; ++i)
    {
        auto ledArrayIt = std::find(std::begin(ledArray), std::end(ledArray), i);
        if (ledArrayIt != std::end(ledArray))
        {
            posArray.array.emplace_back(ledArrayIt - std::begin(ledArray));
        }
        else
        {
            posArray.array.emplace_back(0);
        }
    }

    return posArray;
}

void RGBEffect::setPattern(RGBEffectPattern pattern)
{
    _pattern = pattern;
    beginCurrentCombo();
}

void RGBEffect::setColor(RGBEffectColor color)
{
    _color = color;
    beginCurrentCombo();
}

void RGBEffect::setMixingMode(RGBEffectMixingMode mixingMode)
{
    switch (mixingMode)
    {
    case RGBEffectMixingMode::REPLACE:
        mixPixel = mixPixelReplace;
        break;
    case RGBEffectMixingMode::MAX:
        mixPixel = mixPixelMax;
        break;
    case RGBEffectMixingMode::MIN:
        mixPixel = mixPixelMin;
        break;
    case RGBEffectMixingMode::ADD:
        mixPixel = mixPixelAdd;
        break;
    case RGBEffectMixingMode::SUB:
        mixPixel = mixPixelSub;
        break;
    case RGBEffectMixingMode::MULTIPLY:
        mixPixel = mixPixelMultiply;
        break;
    case RGBEffectMixingMode::DIVIDE:
        mixPixel = mixPixelDivide;
        break;
    case RGBEffectMixingMode::SCREEN:
        mixPixel = mixPixelScreen;
        break;
    case RGBEffectMixingMode::OVERLAY:
        mixPixel = mixPixelOverlay;
        break;
    case RGBEffectMixingMode::DODGE:
        mixPixel = mixPixelDodge;
        break;
    case RGBEffectMixingMode::BURN:
        mixPixel = mixPixelBurn;
        break;
    case RGBEffectMixingMode::HARDLIGHT:
        mixPixel = mixPixelHardLight;
        break;
    case RGBEffectMixingMode::SOFTLIGHT:
        mixPixel = mixPixelSoftLight;
        break;
    case RGBEffectMixingMode::GRAINEXTRACT:
        mixPixel = mixPixelGrainExtract;
        break;
    case RGBEffectMixingMode::GRAINMERGE:
        mixPixel = mixPixel;
        break;
    case RGBEffectMixingMode::DIFFERENCE:
        mixPixel = mixPixelDifference;
        break;
    }
}

void RGBEffect::setPosArray(PosArray const& posArray)
{
    _posArray = posArray;
}

int RGBEffect::loopTime() const
{
    return _loopTime;
}

void RGBEffect::setLoopTime(int loopTime)
{
    _loopTime = std::min(std::max(20, loopTime), 20000000);
}

bool RGBEffect::refreshPixels(unsigned long currentMillis)
{
    if (currentMillis - _prevUpdateMillis >= 20) // 50fps
    {
        _prevUpdateMillis = (currentMillis / 20) * 20;
        switch (_pattern)
        {
        case RGBEffectPattern::SMOOTH_ON_OFF:
            refreshPixelsSmoothOnOff();
            break;
        case RGBEffectPattern::STROBE:
            refreshPixelsStrobe();
            break;
        case RGBEffectPattern::STRIPE:
            refreshPixelsStripe();
            break;
        case RGBEffectPattern::STRIPE_H:
            refreshPixelsStripeH();
            break;
        case RGBEffectPattern::STRIPE_V:
            refreshPixelsStripeV();
            break;
        case RGBEffectPattern::PING_PONG:
            refreshPixelsPingPong();
            break;
        case RGBEffectPattern::PING_PONG_H:
            refreshPixelsPingPongH();
            break;
        case RGBEffectPattern::PING_PONG_V:
            refreshPixelsPingPongV();
            break;
        case RGBEffectPattern::ROTATION:
            refreshPixelsRotation();
            break;
        case RGBEffectPattern::ROTATION_SMOOTH:
            refreshPixelsRotationSmooth();
            break;
        case RGBEffectPattern::ROTATION_SMOOTH_THIN:
            refreshPixelsRotationSmoothThin();
            break;
        case RGBEffectPattern::PLASMA:
            refreshPixelsPlasma();
            break;
        }
        return true;
    }
    return false;
}

void RGBEffect::beginCurrentCombo()
{
  
        LOG_PORT.println("RGBE::beginCurrentCombo");
    auto& colors = getColor();

    _gradient.clear();
    for (unsigned int i = 0; i < colors.size(); ++i)
    {
        unsigned int j = i + 1;
        if (j == colors.size())
            j = 0;
            
        LOG_PORT.println("RGBE::emplace gradient 1");
        _gradient.emplace_back(colors[i]);
        int sr = colors[i][0];
        int sg = colors[i][1];
        int sb = colors[i][2];
        int er = colors[j][0];
        int eg = colors[j][1];
        int eb = colors[j][2];

        int stepR = (er - sr);
        int stepG = (eg - sg);
        int stepB = (eb - sb);

        for (int s = 1; s < 15; ++s)
        {
            int gradR = sr + ((stepR * s) / 15);
            int gradG = sg + ((stepG * s) / 15);
            int gradB = sb + ((stepB * s) / 15);
            
        LOG_PORT.println(i);
            _gradient.emplace_back(std::array<uint8_t, 3>{uint8_t(gradR), uint8_t(gradG), uint8_t(gradB)});
        }
    }

        LOG_PORT.println("RGBE::beginCurrentCombo gradient done");
    switch (_pattern)
    {
    case RGBEffectPattern::SMOOTH_ON_OFF:
        beginSmoothOnOff();
        break;
    case RGBEffectPattern::STROBE:
        beginStrobe();
        break;
    case RGBEffectPattern::STRIPE:
        beginStripe();
        break;
    case RGBEffectPattern::STRIPE_H:
        beginStripeH();
        break;
    case RGBEffectPattern::STRIPE_V:
        beginStripeV();
        break;
    case RGBEffectPattern::PING_PONG:
        beginPingPong();
        break;
    case RGBEffectPattern::PING_PONG_H:
        beginPingPongH();
        break;
    case RGBEffectPattern::PING_PONG_V:
        beginPingPongV();
        break;
    case RGBEffectPattern::ROTATION:
        beginRotation();
        break;
    case RGBEffectPattern::ROTATION_SMOOTH:
        beginRotationSmooth();
        break;
    case RGBEffectPattern::ROTATION_SMOOTH_THIN:
        beginRotationSmoothThin();
        break;
    case RGBEffectPattern::PLASMA:
        beginPlasma();
        break;
    }
}

std::vector<std::array<uint8_t, 3>> const& RGBEffect::getColor() const
{
    static const std::vector<std::array<uint8_t, 3>> flameColors = {
        {0xff, 0x00, 0x00},
        {0xff, 0x00, 0x00},
        {0x40, 0x00, 0x80},
        {0xaf, 0x00, 0x00},
        {0xff, 0x00, 0x00},
        {0xdf, 0x50, 0x00},
        {0xfe, 0xee, 0x00},
    };
    static const std::vector<std::array<uint8_t, 3>> grassColors = {
        {0x00, 0xff, 0x00},
        {0x00, 0xff, 0x00},
        {0xc7, 0xee, 0x00},
        {0x00, 0xff, 0x00},
        {0xd3, 0xa1, 0x00},
    };
    static const std::vector<std::array<uint8_t, 3>> rainbowColors = {
        {0xff, 0x00, 0xff},
        {0xff, 0x00, 0xff},
        {0xff, 0xff, 0x00},
        {0x00, 0xff, 0xff},
    };
    static const std::vector<std::array<uint8_t, 3>> oceanColors = {
        {0x00, 0x00, 0xff},
        {0x00, 0x00, 0xff},
        {0x00, 0x3a, 0xb9},
        {0x02, 0xea, 0xff},
        {0x00, 0x3a, 0xb9},
        {0xee, 0xee, 0xfe},
    };
    static const std::vector<std::array<uint8_t, 3>> goldColors = {
        {0xff, 0xd7, 0x00},
        {0xff, 0xd7, 0x00},
        {0xda, 0xa5, 0x20},
        {0xb8, 0x86, 0x0b},
        {0xee, 0xe8, 0xaa},
    };
    static const std::vector<std::array<uint8_t, 3>> whiteColors = {
        {0xff, 0xff, 0xff},
        {0xff, 0xff, 0xff},
        {0xef, 0xeb, 0xdd},
        {0xf5, 0xf5, 0xf5},
        {0xf8, 0xf8, 0xff},
    };
    static const std::vector<std::array<uint8_t, 3>> pinkColors = {
        {0xff, 0x14, 0x93},
        {0xff, 0x14, 0x93},
        {0xff, 0x00, 0xff},
        {0xc7, 0x15, 0x85},
        {0xff, 0x69, 0xb4},
        {0xda, 0x70, 0xd6},
    };

    switch (_color)
    {
    case RGBEffectColor::FLAME:
    default:
        return flameColors;
    case RGBEffectColor::GRASS:
        return grassColors;
    case RGBEffectColor::RAINBOW:
        return rainbowColors;
    case RGBEffectColor::OCEAN:
        return oceanColors;
    case RGBEffectColor::GOLD:
        return goldColors;
    case RGBEffectColor::WHITE:
        return whiteColors;
    case RGBEffectColor::PINK:
        return pinkColors;
    }
}

void RGBEffect::beginSmoothOnOff()
{
}

void RGBEffect::refreshPixelsSmoothOnOff()
{
    int colorAdvance = _prevUpdateMillis % _loopTime;

    auto& colors = getColor();
    int colorIdx = (_prevUpdateMillis / _loopTime) % colors.size();
    int red = colors[colorIdx][0];
    int green = colors[colorIdx][1];
    int blue = colors[colorIdx][2];
    red = (red * ((_loopTime / 2) - std::abs((_loopTime / 2) - colorAdvance))) / (_loopTime / 2);
    green = (green * ((_loopTime / 2) - std::abs((_loopTime / 2) - colorAdvance))) / (_loopTime / 2);
    blue = (blue * ((_loopTime / 2) - std::abs((_loopTime / 2) - colorAdvance))) / (_loopTime / 2);
    std::array<uint8_t, 3> rgb{uint8_t(red), uint8_t(green), uint8_t(blue)};
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        mixPixel(_pixels + i * 3, rgb.data());
    }
}

void RGBEffect::beginStrobe()
{
}

void RGBEffect::refreshPixelsStrobe()
{
    int value = ((_prevUpdateMillis / 80) % 2) ? 0xff : 0;
    std::array<uint8_t, 3> rgb{uint8_t(value), uint8_t(value), uint8_t(value)};
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        mixPixel(_pixels + i * 3, rgb.data());
    }
}

void RGBEffect::beginStripe()
{
}

void RGBEffect::refreshPixelsStripe()
{
    int colorAdvance = _prevUpdateMillis % _loopTime;

    auto& colors = getColor();
    int colorIdx = (_prevUpdateMillis / _loopTime) % colors.size();
    int red = colors[colorIdx][0];
    int green = colors[colorIdx][1];
    int blue = colors[colorIdx][2];
    int position = (_pixelCount * colorAdvance) / _loopTime;
    int length = 4;
    std::array<uint8_t, 3> rgb;
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        int distance = std::abs(position - (int)i);
        if (distance < length)
        {
            rgb[0] = (red * (length - distance)) / length;
            rgb[1] = (green * (length - distance)) / length;
            rgb[2] = (blue * (length - distance)) / length;
        }
        else
        {
            std::fill(std::begin(rgb), std::end(rgb), 0);
        }
        mixPixel(_pixels + i * 3, rgb.data());
    }
}

void RGBEffect::beginStripeH()
{
}

void RGBEffect::refreshPixelsStripeH()
{
    int colorAdvance = _prevUpdateMillis % _loopTime;

    auto& colors = getColor();
    int colorIdx = (_prevUpdateMillis / _loopTime) % colors.size();
    int red = colors[colorIdx][0];
    int green = colors[colorIdx][1];
    int blue = colors[colorIdx][2];
    int position = (_posArray.width * colorAdvance) / _loopTime;
    int length = 2;
    std::array<uint8_t, 3> rgb;
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        int x = _posArray.array[i] % _posArray.width;
        int distance = std::abs(position - (int)x);
        if (distance < length)
        {
            rgb[0] = (red * (length - distance)) / length;
            rgb[1] = (green * (length - distance)) / length;
            rgb[2] = (blue * (length - distance)) / length;
        }
        else
        {
            std::fill(std::begin(rgb), std::end(rgb), 0);
        }
        mixPixel(_pixels + i * 3, rgb.data());
    }
}

void RGBEffect::beginStripeV()
{
}

void RGBEffect::refreshPixelsStripeV()
{
    int colorAdvance = _prevUpdateMillis % _loopTime;

    auto& colors = getColor();
    int colorIdx = (_prevUpdateMillis / _loopTime) % colors.size();
    int red = colors[colorIdx][0];
    int green = colors[colorIdx][1];
    int blue = colors[colorIdx][2];
    int position = (_posArray.height * colorAdvance) / _loopTime;
    int length = 2;
    std::array<uint8_t, 3> rgb;
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        int y = _posArray.array[i] / _posArray.width;
        int distance = std::abs(position - (int)y);
        if (distance < length)
        {
            rgb[0] = (red * (length - distance)) / length;
            rgb[1] = (green * (length - distance)) / length;
            rgb[2] = (blue * (length - distance)) / length;
        }
        else
        {
            std::fill(std::begin(rgb), std::end(rgb), 0);
        }
        mixPixel(_pixels + i * 3, rgb.data());
    }
}

void RGBEffect::beginPingPong()
{
}

void RGBEffect::refreshPixelsPingPong()
{
    int colorAdvance = _prevUpdateMillis % _loopTime;

    auto& colors = getColor();
    int colorIdx = (_prevUpdateMillis / _loopTime) % colors.size();
    int red = colors[colorIdx][0];
    int green = colors[colorIdx][1];
    int blue = colors[colorIdx][2];
    int position = (_pixelCount * ((_loopTime / 2) - std::abs((_loopTime / 2) - colorAdvance))) / (_loopTime / 2);
    int length = 4;
    std::array<uint8_t, 3> rgb;
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        int distance = std::abs(position - (int)i);
        if (distance < length)
        {
            rgb[0] = (red * (length - distance)) / length;
            rgb[1] = (green * (length - distance)) / length;
            rgb[2] = (blue * (length - distance)) / length;
        }
        else
        {
            std::fill(std::begin(rgb), std::end(rgb), 0);
        }
        mixPixel(_pixels + i * 3, rgb.data());
    }
}

void RGBEffect::beginPingPongH()
{
}

void RGBEffect::refreshPixelsPingPongH()
{
    int colorAdvance = _prevUpdateMillis % _loopTime;

    auto& colors = getColor();
    int colorIdx = (_prevUpdateMillis / _loopTime) % colors.size();
    int red = colors[colorIdx][0];
    int green = colors[colorIdx][1];
    int blue = colors[colorIdx][2];
    int position = (_posArray.width * ((_loopTime / 2) - std::abs((_loopTime / 2) - colorAdvance))) / (_loopTime / 2);
    int length = 2;
    std::array<uint8_t, 3> rgb;
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        int x = _posArray.array[i] % _posArray.width;
        int distance = std::abs(position - (int)x);
        if (distance < length)
        {
            rgb[0] = (red * (length - distance)) / length;
            rgb[1] = (green * (length - distance)) / length;
            rgb[2] = (blue * (length - distance)) / length;
        }
        else
        {
            std::fill(std::begin(rgb), std::end(rgb), 0);
        }
        mixPixel(_pixels + i * 3, rgb.data());
    }
}

void RGBEffect::beginPingPongV()
{
}

void RGBEffect::refreshPixelsPingPongV()
{
    int colorAdvance = _prevUpdateMillis % _loopTime;

    auto& colors = getColor();
    int colorIdx = (_prevUpdateMillis / _loopTime) % colors.size();
    int red = colors[colorIdx][0];
    int green = colors[colorIdx][1];
    int blue = colors[colorIdx][2];
    int position = (_posArray.height * ((_loopTime / 2) - std::abs((_loopTime / 2) - colorAdvance))) / (_loopTime / 2);
    int length = 2;
    std::array<uint8_t, 3> rgb;
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        int y = _posArray.array[i] / _posArray.width;
        int distance = std::abs(position - (int)y);
        if (distance < length)
        {
            rgb[0] = (red * (length - distance)) / length;
            rgb[1] = (green * (length - distance)) / length;
            rgb[2] = (blue * (length - distance)) / length;
        }
        else
        {
            std::fill(std::begin(rgb), std::end(rgb), 0);
        }
        mixPixel(_pixels + i * 3, rgb.data());
    }
}

void RGBEffect::beginRotation()
{
}

void RGBEffect::refreshPixelsRotation()
{
    auto& colors = getColor();
    int colorIdx = (_prevUpdateMillis / _loopTime) % colors.size();
    int red = colors[colorIdx][0];
    int green = colors[colorIdx][1];
    int blue = colors[colorIdx][2];

    //3.14159
    //6.28319

    std::array<uint8_t, 3> rgb;
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        int x = _posArray.array[i] % _posArray.width;
        int y = _posArray.array[i] / _posArray.width;
        float nx = (float(x) + 0.5f) - (float(_posArray.width) * 0.5f);
        float ny = -((float(y) + 0.5f) - (float(_posArray.height) * 0.5f));

        float angle = std::atan2(ny, nx);
        int updateMillis = _prevUpdateMillis % _loopTime;
        updateMillis *= 6283;
        updateMillis /= _loopTime;
        float finalAngle = angle + float(updateMillis) / 1000.0f;
        float sine = std::sin(finalAngle * 2.0f);
        if (sine < 0)
        {
            rgb[0] = red;
            rgb[1] = green;
            rgb[2] = blue;
        }
        else
        {
            std::fill(std::begin(rgb), std::end(rgb), 0);
        }
        mixPixel(_pixels + i * 3, rgb.data());
    }
}

void RGBEffect::beginRotationSmooth()
{
}

void RGBEffect::refreshPixelsRotationSmooth()
{
    auto& colors = getColor();
    int colorAdvance = _prevUpdateMillis % (_loopTime * colors.size());
    int colorIdx = (colorAdvance * _gradient.size()) / (_loopTime * colors.size());
    int red = _gradient[colorIdx][0];
    int green = _gradient[colorIdx][1];
    int blue = _gradient[colorIdx][2];

    //3.14159
    //6.28319

    std::array<uint8_t, 3> rgb;
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        int x = _posArray.array[i] % _posArray.width;
        int y = _posArray.array[i] / _posArray.width;
        float nx = (float(x) + 0.5f) - (float(_posArray.width) * 0.5f);
        float ny = -((float(y) + 0.5f) - (float(_posArray.height) * 0.5f));


        float angle = std::atan2(ny, nx);
        int updateMillis = _prevUpdateMillis % _loopTime;
        updateMillis *= 6283;
        updateMillis /= _loopTime;
        float finalAngle = angle + float(updateMillis) / 1000.0f;
        float sine = std::sin(finalAngle * 2.0f);
        float sinePositive = (sine + 1.0f) * 0.5f;

        rgb[0] = red * sinePositive;
        rgb[1] = green * sinePositive;
        rgb[2] = blue * sinePositive;
        mixPixel(_pixels + i * 3, rgb.data());
    }
}

void RGBEffect::beginRotationSmoothThin()
{
}

void RGBEffect::refreshPixelsRotationSmoothThin()
{
    auto& colors = getColor();
    int colorAdvance = _prevUpdateMillis % (_loopTime * colors.size());
    int colorIdx = (colorAdvance * _gradient.size()) / (_loopTime * colors.size());
    int red = _gradient[colorIdx][0];
    int green = _gradient[colorIdx][1];
    int blue = _gradient[colorIdx][2];

    //3.14159
    //6.28319

    std::array<uint8_t, 3> rgb;
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        int x = _posArray.array[i] % _posArray.width;
        int y = _posArray.array[i] / _posArray.width;
        float nx = (float(x) + 0.5f) - (float(_posArray.width) * 0.5f);
        float ny = -((float(y) + 0.5f) - (float(_posArray.height) * 0.5f));


        float angle = std::atan2(ny, nx);
        int updateMillis = _prevUpdateMillis % _loopTime;
        updateMillis *= 6283;
        updateMillis /= _loopTime;
        float finalAngle = angle + float(updateMillis) / 1000.0f;
        float sine = std::sin(finalAngle * 2.0f);
        float sinePositive = (sine + 1.0f) * 0.5f;

        rgb[0] = red * sinePositive * sinePositive;
        rgb[1] = green * sinePositive * sinePositive;
        rgb[2] = blue * sinePositive * sinePositive;
        mixPixel(_pixels + i * 3, rgb.data());
    }
}

void RGBEffect::beginPlasma()
{
}

void RGBEffect::refreshPixelsPlasma()
{
    const int presetSize = 7;
    const int speed = 50000 / _loopTime;

    int size = presetSize / 2;
    int plasmaStepCount = (_prevUpdateMillis / 20) * speed;
    plasmaStepCount %= 256000;

    double square = _posArray.width > _posArray.height ? _posArray.width : _posArray.height;
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        int x = _posArray.array[i] % _posArray.width;
        int y = _posArray.array[i] / _posArray.width;
        double nx = double(x) / square;
        double ny = double(y) / square;
        double n = Perlin::noise(size * nx, size * ny, double(plasmaStepCount) / 1000.0);
        int gradStep = ((n + 1) / 2) * _gradient.size();
        mixPixel(_pixels + i * 3, _gradient[gradStep].data());
    }
}

void RGBEffect::mixPixelReplace(uint8_t* pixel, uint8_t const* values)
{
    for (int i = 0; i < 3; ++i)
    {
        pixel[i] = values[i];
    }
}

void RGBEffect::mixPixelMax(uint8_t* pixel, uint8_t const* values)
{
    for (int i = 0; i < 3; ++i)
    {
        if (pixel[i] < values[i])
            pixel[i] = values[i];
    }
}

void RGBEffect::mixPixelMin(uint8_t* pixel, uint8_t const* values)
{
    for (int i = 0; i < 3; ++i)
    {
        if (pixel[i] > values[i])
            pixel[i] = values[i];
    }
}

void RGBEffect::mixPixelAdd(uint8_t* pixel, uint8_t const* values)
{
    for (int i = 0; i < 3; ++i)
    {
        int value = int(pixel[i]) + int(values[i]);
        pixel[i] = std::min(value, 0xff);
    }
}

void RGBEffect::mixPixelSub(uint8_t* pixel, uint8_t const* values)
{
    for (int i = 0; i < 3; ++i)
    {
        int value = int(pixel[i]) - int(values[i]);
        pixel[i] = std::max(value, 0x00);
    }
}

void RGBEffect::mixPixelMultiply(uint8_t* pixel, uint8_t const* values)
{
    for (int i = 0; i < 3; ++i)
    {
        pixel[i] = (int(pixel[i]) * int(values[i])) / int(0xff);
    }
}

void RGBEffect::mixPixelDivide(uint8_t* pixel, uint8_t const* values)
{
    for (int i = 0; i < 3; ++i)
    {
        int I = pixel[i];
        int M = values[i];
        pixel[i] = std::min(std::max(0, (I * 256) / (M + 1)), 255);
    }
}

void RGBEffect::mixPixelScreen(uint8_t* pixel, uint8_t const* values)
{
    for (int i = 0; i < 3; ++i)
    {
        pixel[i] = int(0xff)
            - (
                    (int(0xff) - int(pixel[i]))
                    *
                    (int(0xff) - int(values[i]))
              ) / int(0xff);
    }
}

void RGBEffect::mixPixelOverlay(uint8_t* pixel, uint8_t const* values)
{
    for (int i = 0; i < 3; ++i)
    {
        int I = pixel[i];
        int M = values[i];
        pixel[i] = (I * (I + (2 * M * (255 - I)) / 255)) / 255;
    }
}

void RGBEffect::mixPixelDodge(uint8_t* pixel, uint8_t const* values)
{
    for (int i = 0; i < 3; ++i)
    {
        int I = pixel[i];
        int M = values[i];
        int res = (I * 256) / (256 - M);
        pixel[i] = std::min(std::max(0, res), 255);
    }
}

void RGBEffect::mixPixelBurn(uint8_t* pixel, uint8_t const* values)
{
    for (int i = 0; i < 3; ++i)
    {
        int I = pixel[i];
        int M = values[i];
        int res = 255 - (256 * (255 - I)) / (M + 1);
        pixel[i] = std::min(std::max(0, res), 255);
    }
}

void RGBEffect::mixPixelHardLight(uint8_t* pixel, uint8_t const* values)
{
    for (int i = 0; i < 3; ++i)
    {
        int I = pixel[i];
        int M = values[i];
        if (M > 128)
            pixel[i] = 255 - ((255 - 2 * (M - 128)) * (255 - I)) / 256;
        else
            pixel[i] = (2 * M * I) / 256;
    }
}

void RGBEffect::mixPixelSoftLight(uint8_t* pixel, uint8_t const* values)
{
    uint8_t screen[3] = {pixel[0], pixel[1], pixel[2]};
    mixPixelScreen(screen, values);
    for (int i = 0; i < 3; ++i)
    {
        int I = pixel[i];
        int M = values[i];
        int R = screen[i];
        int res = (((255 - I) * M + R * 255) * I) / (255 * 255);
        pixel[i] = std::min(std::max(0, res), 255);
    }
}

void RGBEffect::mixPixelGrainExtract(uint8_t* pixel, uint8_t const* values)
{
    for (int i = 0; i < 3; ++i)
    {
        int I = pixel[i];
        int M = values[i];
        int res = I - M + 128;
        pixel[i] = std::min(std::max(0, res), 255);
    }
}

void RGBEffect::mixPixelGrainMerge(uint8_t* pixel, uint8_t const* values)
{
    for (int i = 0; i < 3; ++i)
    {
        int I = pixel[i];
        int M = values[i];
        int res = I + M - 128;
        pixel[i] = std::min(std::max(0, res), 255);
    }
}

void RGBEffect::mixPixelDifference(uint8_t* pixel, uint8_t const* values)
{
    for (int i = 0; i < 3; ++i)
    {
        int I = pixel[i];
        int M = values[i];
        pixel[i] = std::abs(I - M);
    }
}
