// TalkingFace.hpp: the My lil mascot's mouth. A line along the face that follows the module's signal and glows from ink to lime
// as it gets louder. To use: give the module a FaceTap, push() one -1..1 value per sample, add a TalkingFace at the mascot.
// Author: Ari Klopfer / A_Development. MIT
#pragma once
#include "FaceTap.h"
#include "plugin.hpp"
#include <cmath>

// Box = the face's disc. The panel SVG draws the disc, eyes and a flat mouth; this paints over that mouth.
struct TalkingFace : TransparentWidget
{
    const FaceTap* tap = nullptr;   // null in the library / browser preview: draws a still sine
    float level = 0.f;              // smoothed 0..1, drives the colour

    void draw (const DrawArgs& a) override
    {
        const float r = box.size.x / 2, cx = r, cy = box.size.y / 2;
        const float x0 = cx - r * 0.5f, w = r, base = cy + r * 0.3f, amp = r * 0.28f;   // wave + stroke stay inside the disc
        float v[FaceTap::N], peak = 0.f;
        for (int i = 0; i < FaceTap::N; ++i)
        {
            v[i] = tap ? tap->buf[(tap->head + i) % FaceTap::N] : 0.4f * std::sin (i * 0.5f);
            peak = std::max (peak, std::fabs (v[i]));
        }
        level += 0.25f * (std::min (1.f, peak) - level);

        nvgSave (a.vg);                                       // wipe the SVG's flat mouth, clipped to the disc
        nvgScissor (a.vg, x0 - r * 0.12f, base - r * 0.42f, w + r * 0.24f, r * 0.84f);
        nvgBeginPath (a.vg);
        nvgCircle (a.vg, cx, cy, r * 0.97f);
        nvgFillColor (a.vg, nvgRGB (0xff, 0x3d, 0x9a));
        nvgFill (a.vg);
        nvgRestore (a.vg);

        const float t = std::sqrt (level);                    // ink (#16161a) -> lime (#c8ff3a)
        nvgBeginPath (a.vg);
        for (int i = 0; i < FaceTap::N; ++i)
        {
            const float x = x0 + w * i / (FaceTap::N - 1), y = base - amp * clamp (v[i], -1.f, 1.f);
            if (i) nvgLineTo (a.vg, x, y); else nvgMoveTo (a.vg, x, y);
        }
        nvgStrokeColor (a.vg, nvgRGBf (0.086f + t * 0.698f, 0.086f + t * 0.914f, 0.102f + t * 0.125f));
        nvgStrokeWidth (a.vg, r * 0.16f);
        nvgLineCap (a.vg, NVG_ROUND);
        nvgLineJoin (a.vg, NVG_ROUND);
        nvgStroke (a.vg);
    }
};
