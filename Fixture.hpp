#ifndef __FIXTURE_HPP__
#define __FIXTURE_HPP__

#include <cstdint>

class PixelDriver;

#define TARGET_FREQ_HZ 50

enum class FixtureMode
{
    SIMPLE, // RGB, RGB, RGB.....
    R_G_B_LEVELS, // 1 RGB for all pixels, then brightness of each pixel

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
    unsigned int getNumChannels() const;
    PixelDriver* _pixels = nullptr;
    FixtureMode _mode = FixtureMode::SIMPLE;
};

#endif
