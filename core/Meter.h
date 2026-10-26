// Meter.h: level meter for the LISTEN inputs. Push normalised samples (1.0 = Rack's 10 V full scale, what Core Audio
// clips at), read a Report: RMS / peak / crest / DC / clipped samples per channel and an ungated BS.1770 K-weighted
// loudness over the whole window (no 400 ms gating: a window holding silence reads lower than a gated meter).
// Call setSampleRate() before push(); a default-constructed Meter has identity filters and measures unweighted power.
// No Rack dependency, no threads: the module owns the reset/snapshot handshake.
// Author: Ari Klopfer / A_Development. MIT
#pragma once
#include <cmath>
#include <cstdint>

constexpr double kPi = 3.14159265358979323846;

struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1 = 0, z2 = 0;

    double run (double x)   // transposed direct form II
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void clear() { z1 = z2 = 0; }
};

// ITU-R BS.1770 K-weighting: a high shelf then a high-pass, designed for any sample rate (the spec tabulates 48 kHz only).
inline Biquad kShelf (double fs)
{
    const double f0 = 1681.974450955533, gainDb = 3.999843853973347, q = 0.7071752369554196;
    const double k = std::tan (kPi * f0 / fs), vh = std::pow (10.0, gainDb / 20.0), vb = std::pow (vh, 0.4996667741545416);
    const double a0 = 1 + k / q + k * k;
    Biquad f;
    f.b0 = (vh + vb * k / q + k * k) / a0;
    f.b1 = 2 * (k * k - vh) / a0;
    f.b2 = (vh - vb * k / q + k * k) / a0;
    f.a1 = 2 * (k * k - 1) / a0;
    f.a2 = (1 - k / q + k * k) / a0;
    return f;
}

inline Biquad kHighPass (double fs)
{
    const double f0 = 38.13547087602444, q = 0.5003270373238773;
    const double k = std::tan (kPi * f0 / fs);
    const double a0 = 1 + k / q + k * k;
    Biquad f;
    f.b0 = 1; f.b1 = -2; f.b2 = 1;   // the spec's table keeps these un-normalised
    f.a1 = 2 * (k * k - 1) / a0;
    f.a2 = (1 - k / q + k * k) / a0;
    return f;
}

struct Meter
{
    struct Channel
    {
        double sumSq = 0, sum = 0, peak = 0, kSumSq = 0;
        int64_t clips = 0, nonFinite = 0;
        Biquad shelf, hp;
    };
    struct Report
    {
        double rmsDb[2] = {}, peakDb[2] = {}, crestDb[2] = {}, dc[2] = {};
        int64_t clips[2] = {}, nonFinite[2] = {};
        double lufs = 0;
        int64_t samples = 0;
        bool silent = true;
    };

    Channel ch[2];
    int64_t samples = 0;

    void setSampleRate (double fs)
    {
        for (Channel& c : ch) { c.shelf = kShelf (fs); c.hp = kHighPass (fs); }
        reset();
    }

    void reset()   // keeps the filter design, empties the window
    {
        for (Channel& c : ch) { c.sumSq = c.sum = c.peak = c.kSumSq = 0; c.clips = c.nonFinite = 0; c.shelf.clear(); c.hp.clear(); }
        samples = 0;
    }

    void push (float l, float r)
    {
        const float v[2] = { l, r };
        for (int i = 0; i < 2; ++i)
        {
            Channel& c = ch[i];
            const double x = v[i];
            if (! std::isfinite (x)) { ++c.nonFinite; continue; }   // Rack does not sanitise cables; one NaN would poison the window
            const double a = std::fabs (x);
            c.sumSq += x * x;
            c.sum += x;
            if (a > c.peak) c.peak = a;
            if (a > 1.0) ++c.clips;
            const double k = c.hp.run (c.shelf.run (x));
            c.kSumSq += k * k;
        }
        ++samples;
    }

    static double db (double lin) { return lin > 1e-9 ? 20.0 * std::log10 (lin) : -180.0; }

    Report report() const
    {
        Report r;
        r.samples = samples;
        double kPower = 0, peak = 0;
        for (int i = 0; i < 2; ++i)
        {
            const Channel& c = ch[i];
            const int64_t taken = samples - c.nonFinite;   // skipped samples must not dilute the averages
            const double n = taken > 0 ? (double) taken : 1.0;
            r.rmsDb[i] = db (std::sqrt (c.sumSq / n));
            r.peakDb[i] = db (c.peak);
            r.crestDb[i] = r.peakDb[i] - r.rmsDb[i];
            r.dc[i] = c.sum / n;
            r.clips[i] = c.clips;
            r.nonFinite[i] = c.nonFinite;
            kPower += c.kSumSq / n;
            if (c.peak > peak) peak = c.peak;
        }
        r.lufs = kPower > 1e-18 ? -0.691 + 10.0 * std::log10 (kPower) : -180.0;   // ponytail: ungated; add 400 ms blocks + the -70 LUFS / -10 LU gates when windows hold silence
        r.silent = peak < 1e-4;
        return r;
    }
};
