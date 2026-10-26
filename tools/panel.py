#!/usr/bin/env python3
"""panel.py: writes the My lil MCP panel, res/Mcp.svg, and src/layout.hpp from one layout table.

Labels become paths (DejaVu Sans from the Rack install, or the font named by RACK_FONT), so the SVG has no <text>,
as the VCV panel guide asks. The widget reads the same table through layout.hpp, so the art and the components
cannot drift apart. Run from anywhere: py -3.11 tools/panel.py   (needs fontTools).
Author: Ari Klopfer / A_Development. MIT
"""
import os
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen

H = 128.5
INK, INK2 = '#16161a', '#202027'
LIME, MAGENTA, BLUE, WHITE, DIM = '#c8ff3a', '#ff3d9a', '#5ac8e6', '#f2f2f4', '#8d8d96'
FONT = os.environ.get('RACK_FONT', r'C:\Program Files\VCV\Rack2Free\res\fonts\DejaVuSans.ttf')
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# kinds: hero Rogan3PS, big Rogan2PS, knob Rogan1PS (white or blue), tiny BefacoTinyKnob, trim Trimpot,
# latch VCVLightLatch, button VCVButton, light MediumLight, and jacks: play (lime ring), cv (magenta ring),
# seq (blue ring), out (lime pad)
RADIUS = {'hero': 8.8, 'big': 7.35, 'knob': 6.7, 'tiny': 4.3, 'trim': 3.0, 'latch': 3.1, 'button': 3.1,
          'play': 4.0, 'cv': 4.0, 'seq': 4.0, 'out': 4.0, 'light': 1.5}
RING = {'play': LIME, 'cv': MAGENTA, 'seq': BLUE}
LABEL_SIZE = {'hero': 3.4, 'big': 2.8, 'knob': 2.8, 'tiny': 2.4, 'trim': 2.2, 'latch': 2.4, 'button': 2.4,
              'play': 2.4, 'cv': 2.4, 'seq': 2.4, 'out': 2.6}

font = TTFont(FONT)
glyphs, cmap, upm = font.getGlyphSet(), font.getBestCmap(), font['head'].unitsPerEm


def text(s, x, y, size, fill, anchor='middle', stroke=0.0):
    """One <path> for the string: baseline at y, em size in mm, optional stroke for a bolder face."""
    k = size / upm
    width = sum(glyphs[cmap[ord(c)]].width for c in s) * k
    pen_x = x - width / 2 if anchor == 'middle' else x - width if anchor == 'end' else x
    d = []
    for c in s:
        g = glyphs[cmap[ord(c)]]
        pen = SVGPathPen(glyphs)
        g.draw(TransformPen(pen, (k, 0, 0, -k, pen_x, y)))
        d.append(pen.getCommands())
        pen_x += g.width * k
    bold = ' stroke="%s" stroke-width="%g" stroke-linejoin="round"' % (fill, stroke) if stroke else ''
    return '<path fill="%s"%s d="%s"/>' % (fill, bold, ' '.join(d))


def mascot(cx, cy, r):
    """A little acid face: magenta disc, dot eyes, a wavy mouth."""
    e, m = r * 0.37, r * 0.29
    return ''.join([
        '<circle cx="%g" cy="%g" r="%g" fill="%s"/>' % (cx, cy, r, MAGENTA),
        '<circle cx="%g" cy="%g" r="%g" fill="%s"/>' % (cx - e, cy - r * 0.32, r * 0.145, INK),
        '<circle cx="%g" cy="%g" r="%g" fill="%s"/>' % (cx + e, cy - r * 0.32, r * 0.145, INK),
        '<path d="M %g %g q %g %g %g 0 q %g %g %g 0" fill="none" stroke="%s" stroke-width="%g" stroke-linecap="round"/>'
        % (cx - 2 * m, cy + r * 0.29, m, r * 0.45, 2 * m, m, -r * 0.45, 2 * m, INK, r * 0.16),
    ])


def panel(W, title_x, mascot_x, layout, extras, pad_r, band=8.6, title_size=4.3, title_y=6.3, below=True, title='MY LIL ACID BASS'):
    """The common frame: ink panel, lime title band, mascot, brand; then the extras and the labelled components.
    Labels sit below the components (or above, below=False), one baseline per row set by the row's biggest part."""
    out = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<svg xmlns="http://www.w3.org/2000/svg" width="%gmm" height="%gmm" viewBox="0 0 %g %g">' % (W, H, W, H),
        '<rect width="%g" height="%g" fill="%s"/>' % (W, H, INK),
        '<rect width="%g" height="%g" fill="%s"/>' % (W, band, LIME),
        text(title, title_x, title_y, title_size, INK, stroke=title_size * 0.09),
        mascot(mascot_x, band / 2, band * 0.36),
    ]
    out += extras
    rowmax = {}
    for _, kind, x, y, label in layout:
        if kind not in ('out', 'light'):
            rowmax[y] = max(rowmax.get(y, 0), RADIUS[kind])
    for _, kind, x, y, label in layout:
        r = RADIUS[kind]
        if kind in RING:
            out.append('<circle cx="%g" cy="%g" r="%g" fill="none" stroke="%s" stroke-width="0.6"/>' % (x, y, r + 1.3, RING[kind]))
        elif kind == 'out':
            out.append('<circle cx="%g" cy="%g" r="%g" fill="%s"/>' % (x, y, pad_r, LIME))
        if not label:
            continue
        colour, stroke = (LIME, 0.3) if kind == 'hero' else (LIME, 0.25) if kind == 'out' else (WHITE, 0.18)
        reach = pad_r if kind == 'out' else rowmax[y]
        ly = y + reach + 2.8 if below else y - reach - 2.3
        out.append(text(label, x, ly, LABEL_SIZE[kind], colour, stroke=stroke))
    out.append(text('A_DEVELOPMENT', W / 2, 125.6, 2.4, DIM))
    out.append('</svg>')
    return '\n'.join(out) + '\n'


# ---------------------------------------------------------------------------------------------------------------- My lil MCP, 6 HP
W = 5.08 * 6
BAND = 13.0
MCP = [
    ('ENABLED_PARAM', 'latch', W / 2, 36, 'SERVER'),
    ('ACTIVITY_LIGHT', 'light', W / 2, 52, ''),
    ('LISTEN_L_INPUT', 'play', 9.0, 95.5, 'L'), ('LISTEN_R_INPUT', 'play', 21.5, 95.5, 'R'),
    ('HEARTBEAT_OUTPUT', 'out', W / 2, 112, 'BEAT'),
]
EXTRAS = [
    text('BUSY', W / 2, 57.5, 2.2, WHITE),
    text('MCP  127.0.0.1', W / 2, 70, 2.0, DIM),
    '<rect x="3.5" y="86" width="%g" height="35.5" rx="2.5" fill="%s"/>' % (W - 7, INK2),   # listen zone
    '<rect x="3.5" y="84.1" width="%g" height="0.5" fill="%s"/>' % (W - 7, LIME),
    '<rect x="9" y="82" width="12.5" height="4.6" fill="%s"/>' % INK,
    text('LISTEN', W / 2, 85.6, 2.8, LIME, stroke=0.2),
]


def header():
    lines = ['// layout.hpp: generated by tools/panel.py, do not edit. Component centres in mm.', '#pragma once', 'namespace layout {', 'namespace mcp {']
    lines += ['constexpr float %s[2] = {%.2ff, %.2ff};' % (i, x, y) for i, _, x, y, _ in MCP]
    lines.append('constexpr float FACE[3] = {%.2ff, %.2ff, %.3ff};   // mascot centre x, y, radius (TalkingFace)' % (24.8, BAND / 2, BAND * 0.36))
    lines += ['}', '}', '']
    return '\n'.join(lines)


if __name__ == '__main__':
    files = {
        os.path.join('res', 'Mcp.svg'): panel(W, 11, 24.8, MCP, EXTRAS, pad_r=5.6, band=BAND, title_size=2.8, title_y=8.4, title='MY LIL MCP'),
        os.path.join('src', 'layout.hpp'): header(),
    }
    for rel, body in files.items():
        with open(os.path.join(ROOT, rel), 'w', encoding='utf-8', newline='\n') as f:
            f.write(body)
    print('wrote', ', '.join(files), '(%d components)' % len(MCP))
