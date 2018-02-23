#ifndef __FIXTURE_HPP__
#define __FIXTURE_HPP__

#include <cstdint>

class PixelDriver;

#define TARGET_FREQ_HZ 50

enum class FixtureMode
{
    SIMPLE, // RGB, RGB, RGB.....
    R_G_B_LEVELS, // 1 RGB for all pixels, then brightness of each pixel
    DEMO,
};

class Fixture
{
public:
    void begin(PixelDriver* pixels);
    void updateInput(uint8_t const* data, unsigned int size);
    void refreshPixels();
private:
    void updateInputSimple(uint8_t const* data);
    void updateInputRGBLevels(uint8_t const* data);
    void updateInputDemo(uint8_t const* data);
    unsigned int getNumChannels() const;
    void refreshPixelsDemo();
    PixelDriver* _pixels = nullptr;
    FixtureMode _mode;

    long int demoPrevColorMillis = 0;
    long int demoPrevModeMillis = 0;
    int demoCurrentColorIdx = 0;
    int demoCurrentModeIdx = 0;
    void refreshPixelsDemoPlainSwitch(int colorAdvance);
    void refreshPixelsDemoPlainFade(int colorAdvance);
    void refreshPixelsDemoSnake(int colorAdvance);
    void refreshPixelsDemoSnakeFade(int colorAdvance);
};

#endif