//
// Created by alex on 13/02/2022.
//

#ifndef BACKGROUND_MIULAW_H
#define BACKGROUND_MIULAW_H

#include <stdint.h>

// https://dystopiancode.blogspot.com/2012/02/pcm-law-and-u-law-companding-algorithms.html

int8_t mulaw_encode(float f)
{
    int16_t number = int16_t(f * 32768.f);
    const uint16_t MULAW_MAX = 0x1FFF;
    const uint16_t MULAW_BIAS = 33;
    uint16_t mask = 0x1000;
    uint8_t sign = 0;
    uint8_t position = 12;
    uint8_t lsb = 0;
    if (number < 0)
    {
        number = -number;
        sign = 0x80;
    }
    number += MULAW_BIAS;
    if (number > MULAW_MAX)
    {
        number = MULAW_MAX;
    }
    for (; ((number & mask) != mask && position >= 5); mask >>= 1, position--)
        ;
    lsb = (number >> (position - 4)) & 0x0f;
    return (~(sign | ((position - 5) << 4) | lsb));
}

float mulaw_decode(int8_t number) {
    const uint16_t MULAW_BIAS = 33;
    uint8_t sign = 0, position = 0;
    int16_t decoded = 0;
    number = ~number;
    if (number & 0x80)
    {
        number &= ~(1 << 7);
        sign = -1;
    }
    position = ((number & 0xF0) >> 4) + 5;
    decoded = ((1 << position) | ((number & 0x0F) << (position - 4))
               | (1 << (position - 5))) - MULAW_BIAS;
    int r = (sign == 0) ? (decoded) : (-(decoded));
    return float(r)/32768.f;
}


#endif //BACKGROUND_MIULAW_H
