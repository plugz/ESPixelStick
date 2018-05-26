#include "Fixture2.hpp"
#include "Perlin.hpp"
#include "ESPixelStick.h"

#include <iterator>

#define TARGET_FREQ_HZ 50

#ifndef LOG_PORT
class NoSerialClass
{
public:
    template<typename... T>
    void begin(T...) {}
    template<typename... T>
    void print(T...) {}
    template<typename... T>
    void println(T...) {}
};
NoSerialClass NoSerial;
#define LOG_PORT NoSerial

int millis()
{
    static int prev = 0;
    prev += 20;
    return prev;
}
#endif

void Fixture2::begin(std::vector<Fixture2Mode> const& modes,
                     std::vector<Fixture2Color> const& colors,
                     uint8_t *pixels,
                     unsigned int width,
                     unsigned int height,
                     bool zigzag)
{
    _modes = modes;
    _colors = colors;
    _pixels = pixels;
    _pixelCount = width * height;
    _width = width;
    _height = height;
    _posArray.clear();
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        if (!zigzag)
            _posArray.emplace_back(i);
        else
        {
            if ((i / _width) % 2)
                _posArray.emplace_back((i / _width) * _width + (_width - (i % _width)));
            else
                _posArray.emplace_back(i);
        }
    }

    beginCurrentCombo();
}

void Fixture2::nextColor()
{
    _currentColor = (_currentColor + 1) % _colors.size();
    beginCurrentCombo();
}

void Fixture2::nextMode()
{
    _currentMode = (_currentMode + 1) % _modes.size();
    beginCurrentCombo();
}

void Fixture2::startFlash(Fixture2FlashMode mode)
{
    _flashing = true;
    _flashMode = mode;
}

void Fixture2::stopFlash()
{
    _flashing = false;
}

bool Fixture2::refreshPixels()
{
    unsigned long currentMillis = millis();
    if (currentMillis - _prevUpdateMillis >= 20) // 50fps
    {
        _prevUpdateMillis += ((currentMillis - _prevUpdateMillis) / 20) * 20;
        switch (_modes[_currentMode])
        {
        case Fixture2Mode::PLASMA:
            refreshPixelsPlasma();
            break;
        case Fixture2Mode::SMOOTH_ON_OFF:
            refreshPixelsSmoothOnOff();
            break;
        case Fixture2Mode::PING_PONG:
            refreshPixelsPingPong();
            break;
        case Fixture2Mode::PING_PONG_H:
            refreshPixelsPingPongH();
            break;
        case Fixture2Mode::PING_PONG_V:
            refreshPixelsPingPongV();
            break;
        }
        if (_flashing)
        {
            switch (_flashMode)
            {
            case Fixture2FlashMode::STROBE:
                flashStrobe();
            }
        }
        return true;
    }
    return false;
}

void Fixture2::beginCurrentCombo()
{
    switch (_modes[_currentMode])
    {
    case Fixture2Mode::PLASMA:
        beginPlasma();
        break;
    case Fixture2Mode::SMOOTH_ON_OFF:
        beginSmoothOnOff();
        break;
    case Fixture2Mode::PING_PONG:
        beginPingPong();
        break;
    case Fixture2Mode::PING_PONG_H:
        beginPingPongH();
        break;
    case Fixture2Mode::PING_PONG_V:
        beginPingPongV();
        break;
    }
}

std::vector<std::array<uint8_t, 3>> const& Fixture2::getColor() const
{
    static const std::vector<std::array<uint8_t, 3>> flameColors = {
        {0xff, 0x00, 0x00},
        {0xff, 0x00, 0x00},
        {0x40, 0x00, 0x80},
        {0xaf, 0x00, 0x00},
        {0xff, 0x00, 0x00},
        {0xdf, 0x50, 0x00},
        {0xfe, 0xee, 0x00},
        {0xff, 0x00, 0x00},
    };
    static const std::vector<std::array<uint8_t, 3>> grassColors = {
        {0x00, 0xff, 0x00},
        {0x00, 0xff, 0x00},
        {0xc7, 0xee, 0x00},
        {0x00, 0xff, 0x00},
        {0xd3, 0xa1, 0x00},
        {0x00, 0xff, 0x00},
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
        {0x00, 0x00, 0xff},
    };

    switch (_colors[_currentColor])
    {
    case Fixture2Color::FLAME:
    default:
        return flameColors;
    case Fixture2Color::GRASS:
        return grassColors;
    case Fixture2Color::RAINBOW:
        return rainbowColors;
    case Fixture2Color::OCEAN:
        return oceanColors;
    }
}

// 8s
#define LOOPTIME 8000

void Fixture2::beginSmoothOnOff()
{
}

void Fixture2::refreshPixelsSmoothOnOff()
{
    int colorAdvance = _prevUpdateMillis % LOOPTIME;

    auto& colors = getColor();
    int red = colors[0][0];
    int green = colors[0][1];
    int blue = colors[0][2];
    red = (red * ((LOOPTIME / 2) - std::abs((LOOPTIME / 2) - colorAdvance))) / (LOOPTIME / 2);
    green = (green * ((LOOPTIME / 2) - std::abs((LOOPTIME / 2) - colorAdvance))) / (LOOPTIME / 2);
    blue = (blue * ((LOOPTIME / 2) - std::abs((LOOPTIME / 2) - colorAdvance))) / (LOOPTIME / 2);
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        _pixels[i * 3 + 0] = red;
        _pixels[i * 3 + 1] = green;
        _pixels[i * 3 + 2] = blue;
    }
}

void Fixture2::beginPingPong()
{
}

void Fixture2::refreshPixelsPingPong()
{
    int loopTime = LOOPTIME / 2;
    int colorAdvance = _prevUpdateMillis % loopTime;

    auto& colors = getColor();
    int red = colors[0][0];
    int green = colors[0][1];
    int blue = colors[0][2];
    int position = (_pixelCount * ((loopTime / 2) - std::abs((loopTime / 2) - colorAdvance))) / (loopTime / 2);
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        if (std::abs(position - (int)i) < 4)
        {
            _pixels[i * 3 + 0] = red;
            _pixels[i * 3 + 1] = green;
            _pixels[i * 3 + 2] = blue;
        }
        else
        {
            _pixels[i * 3 + 0] = 0;
            _pixels[i * 3 + 1] = 0;
            _pixels[i * 3 + 2] = 0;
        }
    }
}

void Fixture2::beginPingPongH()
{
}

void Fixture2::refreshPixelsPingPongH()
{
    int loopTime = LOOPTIME / 8;
    int colorAdvance = _prevUpdateMillis % loopTime;

    auto& colors = getColor();
    int red = colors[0][0];
    int green = colors[0][1];
    int blue = colors[0][2];
    int position = (_width * ((loopTime / 2) - std::abs((loopTime / 2) - colorAdvance))) / (loopTime / 2);
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        int pos = _posArray[i];
        int x = pos % _width;
        int y = pos / _width;
        if (std::abs(position - (int)x) < 2)
        {
            _pixels[pos * 3 + 0] = red;
            _pixels[pos * 3 + 1] = green;
            _pixels[pos * 3 + 2] = blue;
        }
        else
        {
            _pixels[pos * 3 + 0] = 0;
            _pixels[pos * 3 + 1] = 0;
            _pixels[pos * 3 + 2] = 0;
        }
    }
}

void Fixture2::beginPingPongV()
{
}

void Fixture2::refreshPixelsPingPongV()
{
    int loopTime = LOOPTIME / 8;
    int colorAdvance = _prevUpdateMillis % loopTime;

    auto& colors = getColor();
    int red = colors[0][0];
    int green = colors[0][1];
    int blue = colors[0][2];
    int position = (_height * ((loopTime / 2) - std::abs((loopTime / 2) - colorAdvance))) / (loopTime / 2);
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        int pos = _posArray[i];
        int x = pos % _width;
        int y = pos / _width;
        if (std::abs(position - (int)y) < 2)
        {
            _pixels[pos * 3 + 0] = red;
            _pixels[pos * 3 + 1] = green;
            _pixels[pos * 3 + 2] = blue;
        }
        else
        {
            _pixels[pos * 3 + 0] = 0;
            _pixels[pos * 3 + 1] = 0;
            _pixels[pos * 3 + 2] = 0;
        }
    }
}

void Fixture2::beginPlasma()
{
    auto& colors = getColor();

    _plasmaGradient.clear();
    for (unsigned int i = 0; i < colors.size(); ++i)
    {
        unsigned int j = i + 1;
        if (j == colors.size())
            j = 0;
        _plasmaGradient.emplace_back(colors[i]);
        int sr = colors[i][0];
        int sg = colors[i][1];
        int sb = colors[i][2];
        int er = colors[j][0];
        int eg = colors[j][1];
        int eb = colors[j][2];

        int stepR = (er - sr);
        int stepG = (eg - sg);
        int stepB = (eb - sb);

        for (int s = 1; s < 300; ++s)
        {
            int gradR = sr + ((stepR * s) / 300);
            int gradG = sg + ((stepG * s) / 300);
            int gradB = sb + ((stepB * s) / 300);
            _plasmaGradient.emplace_back(std::array<uint8_t, 3>{uint8_t(gradR), uint8_t(gradG), uint8_t(gradB)});
        }
    }
}

void Fixture2::refreshPixelsPlasma()
{
    const int presetSize = 7;
    const int speed = 7;

    int size = presetSize / 2;
    _plasmaStepCount += speed;
    _plasmaStepCount %= 256000;

    double square = _width > _height ? _width : _height;
    for (unsigned int i = 0; i < _pixelCount; ++i)
    {
        int x = _posArray[i] % _width;
        int y = _posArray[i] / _width;
        double nx = double(x) / square;
        double ny = double(y) / square;
        double n = Perlin::noise(size * nx, size * ny, double(_plasmaStepCount) / 1000.0);
        int gradStep = ((n + 1) / 2) * _plasmaGradient.size();
        std::copy(std::begin(_plasmaGradient[gradStep]),
                  std::end(_plasmaGradient[gradStep]),
                  _pixels + (i * 3));
    }
}

void Fixture2::flashStrobe()
{
    if ((_prevUpdateMillis / 100) % 2)
    {
        for (unsigned int i = 0; i < _pixelCount * 3; ++i)
            _pixels[i] = 0xff;
    }
}