#ifndef __FIXTURE_HPP__
#define __FIXTURE_HPP__

#include <cstdint>

class PixelDriver;

#define TARGET_FREQ_HZ 50

class Fixture
{
public:
    void begin(PixelDriver* pixels);
    void updateInput(uint8_t const* data, unsigned int size);
    void refreshPixels();
private:
    void getNumChannels() const;
    PixelDriver* _pixels = nullptr;
};

#endif
