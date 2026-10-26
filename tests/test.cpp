// test.cpp: checks for the Rack-free core (JSON helpers, request guard, meter). Run: make test
// Author: Ari Klopfer / A_Development. MIT
#include "Json.h"
#include "Guard.h"
#include "Meter.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace
{
int failures = 0;

void check (bool ok, const char* what, double got = 0.0)
{
    std::printf ("%s  %s  (%g)\n", ok ? "ok  " : "FAIL", what, got);
    if (! ok) ++failures;
}
bool near (double a, double b, double tol = 1e-6) { return std::fabs (a - b) <= tol * std::max (1.0, std::fabs (b)); }

void testJson()
{
    check (jsonStr ("a\"b\\c\n") == "\"a\\\"b\\\\c\\n\"", "jsonStr escapes quote, backslash, newline");
    check (ok ("[1]") == "{\"status\": \"ok\", \"data\": [1]}", "ok() wrapper");
    check (err ("x") == "{\"status\": \"error\", \"message\": \"x\"}", "err() wrapper");
    check (jsonNum (-9.0309) == "-9.031", "jsonNum three decimals");
    check (jsonNum (NAN) == "null", "jsonNum NaN -> null");
    check (jsonNum (INFINITY) == "null", "jsonNum inf -> null");
    check (parseJsonString ("{\"name\": \"VCO\"}", "name") == "VCO", "parseJsonString");
    check (parseJsonString ("{}", "name") == "", "parseJsonString missing -> empty");
    check (parseJsonString ("{\"p\": \"C:\\\\Users\\\\a\\\\x.vcv\"}", "p") == "C:\\Users\\a\\x.vcv", "parseJsonString unescapes backslashes");
    check (parseJsonString ("{\"s\": \"say \\\"hi\\\"\"}", "s") == "say \"hi\"", "parseJsonString keeps an escaped quote inside");
    check (parseJsonString ("{\"s\": \"caf\\u00e9\"}", "s") == "caf\xc3\xa9", "parseJsonString decodes \\u00e9 to UTF-8");
    check (parseJsonString ("{\"s\": \"\\ud83c\\udfb9\"}", "s") == "\xf0\x9f\x8e\xb9", "parseJsonString joins a surrogate pair");
    check (parseJsonDouble ("{\"x\": \"12.5\"}", "x") == 12.5, "parseJsonDouble accepts a quoted number");
    check (parseJsonDouble ("{}", "x", -1) == -1, "parseJsonDouble missing -> default");
    check (parseRawValue ("{\"params\": [{\"id\": 1}, {\"id\": 2}], \"k\": 1}", "params") == "[{\"id\": 1}, {\"id\": 2}]", "parseRawValue nested array");
    check (parseRawValue ("{\"s\": \"a}b\"}", "s") == "\"a}b\"", "parseRawValue string keeps its quotes and braces");
    check (parseRawValue ("{\"s\": \"a\\\"b\", \"k\": 1}", "s") == "\"a\\\"b\"", "parseRawValue string with escaped quote");
    check (parseRawValue ("{\"b\": true}", "b") == "true", "parseRawValue bare literal");
    check (parseJsonRpcId ("{\"id\": 7, \"method\": \"ping\"}") == "7", "rpc id number");
    check (parseJsonRpcId ("{\"id\": \"abc\"}") == "\"abc\"", "rpc id string");
    check (parseJsonRpcId ("{\"method\": \"x\"}") == "null", "rpc id missing -> null");
    check (toolOk ("{}") == "{\"content\":[{\"type\":\"text\",\"text\":\"{}\"}],\"isError\":false}", "toolOk shape");
    check (mcpErr ("1", -32601, "nope") == "{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":{\"code\":-32601,\"message\":\"nope\"}}", "mcpErr shape");
}

void testGuard()
{
    check (guard::isLoopbackHost ("127.0.0.1"), "host 127.0.0.1");
    check (guard::isLoopbackHost ("127.0.0.1:2600"), "host 127.0.0.1:2600");
    check (guard::isLoopbackHost ("localhost:2600"), "host localhost:2600");
    check (guard::isLoopbackHost ("[::1]:2600"), "host [::1]:2600");
    check (! guard::isLoopbackHost ("evil.com"), "foreign host refused");
    check (! guard::isLoopbackHost ("127.0.0.1.evil.com"), "prefix-spoofed host refused");
    check (! guard::isLoopbackHost (""), "empty host refused");
    check (guard::isAllowedOrigin (""), "no Origin header (curl, Claude Code) allowed");
    check (guard::isAllowedOrigin ("http://localhost:5173"), "loopback origin allowed");
    check (guard::isAllowedOrigin ("http://127.0.0.1"), "loopback origin without port allowed");
    check (! guard::isAllowedOrigin ("https://evil.com"), "foreign origin refused");
    check (! guard::isAllowedOrigin ("http://evil.com"), "foreign http origin refused");
    check (! guard::isAllowedOrigin ("http://localhost.evil.com"), "spoofed loopback origin refused");
    check (! guard::isAllowedOrigin ("null"), "opaque origin refused");
}

void testMeter()
{
    // The K-weighting designed for 48 kHz must reproduce the table in ITU-R BS.1770 (the only rate the spec tabulates).
    Biquad s = kShelf (48000.0);
    check (near (s.b0, 1.53512485958697, 1e-5) && near (s.b1, -2.69169618940638, 1e-5) && near (s.b2, 1.19839281085285, 1e-5)
           && near (s.a1, -1.69065929318241, 1e-5) && near (s.a2, 0.73248077421585, 1e-5), "K shelf matches BS.1770 at 48 kHz", s.b0);
    Biquad h = kHighPass (48000.0);
    check (near (h.a1, -1.99004745483398, 1e-5) && near (h.a2, 0.99007225036621, 1e-5) && h.b0 == 1 && h.b1 == -2 && h.b2 == 1,
           "K high-pass matches BS.1770 at 48 kHz", h.a1);

    // One second of a 0 dBFS 997 Hz sine in one channel: BS.1770 says -3.01 LKFS.
    Meter m;
    m.setSampleRate (48000.0);
    for (int i = 0; i < 48000; ++i) m.push ((float) std::sin (2 * kPi * 997.0 * i / 48000.0), 0.f);
    Meter::Report r = m.report();
    check (near (r.rmsDb[0], -3.0103, 1e-2), "sine RMS is -3.01 dBFS", r.rmsDb[0]);
    check (std::fabs (r.peakDb[0]) < 0.01, "sine peak is 0 dBFS", r.peakDb[0]);
    check (near (r.crestDb[0], 3.0103, 1e-2), "sine crest is 3.01 dB", r.crestDb[0]);
    check (std::fabs (r.dc[0]) < 1e-3, "sine has no DC", r.dc[0]);
    check (r.clips[0] == 0 && r.clips[1] == 0, "no clips");
    check (std::fabs (r.lufs + 3.01) < 0.01, "one channel 0 dBFS 997 Hz = -3.01 LUFS", r.lufs);
    check (r.rmsDb[1] <= -170.0 && ! r.silent, "empty right channel reads as floor; window not silent", r.rmsDb[1]);

    // Both channels: 0 LUFS.
    Meter m2;
    m2.setSampleRate (48000.0);
    for (int i = 0; i < 48000; ++i) { const float v = (float) std::sin (2 * kPi * 997.0 * i / 48000.0); m2.push (v, v); }
    r = m2.report();
    check (std::fabs (r.lufs) < 0.01, "both channels 0 dBFS 997 Hz = 0 LUFS", r.lufs);

    // The K-weighting is designed per rate, so the reference tone stays near -3.01 LUFS away from 48 kHz too.
    for (double fs : { 44100.0, 192000.0 })
    {
        Meter mf;
        mf.setSampleRate (fs);
        for (int i = 0; i < (int) fs; ++i) mf.push ((float) std::sin (2 * kPi * 997.0 * i / fs), 0.f);
        const double lufs = mf.report().lufs;
        check (near (lufs, -3.01, 0.05), fs < 48000 ? "one channel sine = -3.01 LUFS at 44.1 kHz" : "one channel sine = -3.01 LUFS at 192 kHz", lufs);
    }

    // DC and clipping: 0.5 is -6.02 dB of pure offset; 1.5 is over full scale on every sample.
    Meter m3;
    m3.setSampleRate (44100.0);
    for (int i = 0; i < 1000; ++i) m3.push (0.5f, 1.5f);
    r = m3.report();
    check (near (r.dc[0], 0.5, 1e-6) && near (r.rmsDb[0], -6.0206, 1e-3), "DC offset and its RMS", r.dc[0]);
    check (r.clips[0] == 0 && r.clips[1] == 1000, "clips counted per channel", (double) r.clips[1]);

    // Negative overs clip too; |-1.5| is +3.52 dBFS.
    Meter mn;
    mn.setSampleRate (48000.0);
    for (int i = 0; i < 10; ++i) mn.push (-1.5f, 0.f);
    r = mn.report();
    check (r.clips[0] == 10 && near (r.peakDb[0], 3.5218, 1e-3), "negative overs clip and set the peak", r.peakDb[0]);

    // Non-finite samples are counted and skipped, so they cannot poison the window.
    Meter mx;
    mx.setSampleRate (48000.0);
    mx.push (NAN, INFINITY);
    for (int i = 0; i < 1000; ++i) mx.push (0.5f, 0.5f);
    r = mx.report();
    check (r.nonFinite[0] == 1 && r.nonFinite[1] == 1, "non-finite samples counted per channel", (double) r.nonFinite[0]);
    check (std::isfinite (r.lufs) && near (r.dc[0], 0.5, 1e-6) && r.samples == 1001, "non-finite samples skipped; window stays finite", r.lufs);

    // reset() keeps the design, empties the window.
    m3.reset();
    r = m3.report();
    check (r.samples == 0 && r.silent, "reset empties the window");

    // Silence.
    Meter m4;
    m4.setSampleRate (48000.0);
    for (int i = 0; i < 100; ++i) m4.push (0.f, 0.f);
    r = m4.report();
    check (r.silent && r.rmsDb[0] <= -170.0 && r.lufs <= -170.0, "silence reads as silent with floor values", r.lufs);
}
} // namespace

int main()
{
    testJson();
    testGuard();
    testMeter();
    std::printf ("%s (%d failures)\n", failures ? "FAILED" : "all ok", failures);
    return failures ? 1 : 0;
}
