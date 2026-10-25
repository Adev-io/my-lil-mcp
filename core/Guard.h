// Guard.h: which HTTP requests the server answers. The Host must be loopback (stops DNS rebinding), and a browser's
// Origin must itself be loopback (stops any web page you have open from driving your Rack). No Rack dependency.
// Author: Ari Klopfer / A_Development. MIT
#pragma once
#include <string>

namespace guard
{

// "127.0.0.1", "127.0.0.1:2600", "localhost", "localhost:2600", "[::1]", "[::1]:2600"
inline bool isLoopbackHost (const std::string& host)
{
    static const char* const loopback[] = { "127.0.0.1", "localhost", "[::1]" };
    for (const char* h : loopback)
    {
        const std::string s (h);
        if (host == s) return true;
        if (host.size() > s.size() && host.compare (0, s.size(), s) == 0 && host[s.size()] == ':') return true;
    }
    return false;
}

// No Origin header (curl, Claude Code, Python) is fine; a browser's Origin must be a loopback http origin.
inline bool isAllowedOrigin (const std::string& origin)
{
    if (origin.empty()) return true;
    const std::string http = "http://";
    if (origin.compare (0, http.size(), http) != 0) return false;
    return isLoopbackHost (origin.substr (http.size()));
}

} // namespace guard
