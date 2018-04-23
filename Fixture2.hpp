#ifndef __FIXTURE_HPP__
#define __FIXTURE_HPP__

#include <array>
#include <cstdint>
#include <vector>

#define TARGET_FREQ_HZ 50

enum Fixture2Mode
{
    PLASMA,
    SMOOTH_ON_OFF,
    PING_PONG,
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
               unsigned int channelCount);
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

    // ping pong
    void beginPingPong();
    void refreshPixelsPingPong();

    // Flash strobe
    void flashStrobe();
};

#endif
