#ifndef __FIXTURE2_HPP__
#define __FIXTURE2_HPP__

#include <array>
#include <cstdint>
#include <vector>

enum Fixture2Mode
{
    PLASMA,
    SMOOTH_ON_OFF,
    PING_PONG,
    PING_PONG_H,
    PING_PONG_V,
};

enum Fixture2Color
{
    FLAME,
    GRASS,
    OCEAN,
    RAINBOW,
};

enum Fixture2FlashMode
{
    STROBE,
};

class Fixture2
{
  public:
    void begin(std::vector<Fixture2Mode> const& modes,
               std::vector<Fixture2Color> const& colors,
               uint8_t *pixels,
               unsigned int width,
               unsigned int height = 1,
               bool zigzag = false);
    void nextColor();
    void nextMode();
    void startFlash(Fixture2FlashMode mode);
    void stopFlash();
    bool refreshPixels();

  private:
    void beginCurrentCombo();
    std::vector<std::array<uint8_t, 3>> const& getColor() const;
    uint8_t* _pixels;
    unsigned int _pixelCount;
    unsigned int _width;
    unsigned int _height;
    std::vector<int> _posArray;
    unsigned long _prevUpdateMillis = 0;
    std::vector<Fixture2Mode> _modes;
    std::vector<Fixture2Color> _colors;
    int _currentMode = 0;
    int _currentColor = 0;
    bool _flashing = false;
    Fixture2FlashMode _flashMode;

    // plasma
    void beginPlasma();
    void refreshPixelsPlasma();
    int _plasmaStepCount = 100;
    std::vector<std::array<uint8_t, 3>> _plasmaGradient;

    // smooth on-off
    void beginSmoothOnOff();
    void refreshPixelsSmoothOnOff();

    // ping pong (single pixel)
    void beginPingPong();
    void refreshPixelsPingPong();

    // ping pong (single pixel)
    void beginPingPongH();
    void refreshPixelsPingPongH();

    // ping pong (single pixel)
    void beginPingPongV();
    void refreshPixelsPingPongV();

    // Flash strobe
    void flashStrobe();
};

#endif
