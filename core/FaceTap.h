// FaceTap.h: the last ~40 ms of a signal, thinned to N points, for the mascot's mouth. No Rack dependency.
// Each point is the largest-magnitude sample of its stride, so a one-sample trigger still shows.
// Audio thread writes, UI thread reads; a torn float only blurs one frame.
// Author: Ari Klopfer / A_Development. MIT
#pragma once
#include <algorithm>
#include <cmath>

struct FaceTap
{
    static constexpr int N = 48;
    float buf[N] = {};
    int head = 0;

    // v is already normalised to -1..1 by the module (its choice of what the mouth says)
    void push (float v, float sampleRate)
    {
        const int stride = std::max (1, (int) (sampleRate * 0.04f / N));
        if (std::fabs (v) > std::fabs (peak)) peak = v;
        if (++tick < stride) return;
        buf[head] = peak;
        head = (head + 1) % N;
        tick = 0;
        peak = 0.f;
    }

private:
    int tick = 0;
    float peak = 0.f;
};
