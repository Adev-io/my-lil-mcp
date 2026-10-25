// test.cpp: checks for the Rack-free core (JSON helpers, request guard, meter). Run: make test
// Author: Ari Klopfer / A_Development. MIT
#include "Json.h"
#include "Guard.h"
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
[[maybe_unused]] bool near (double a, double b, double tol = 1e-6) { return std::fabs (a - b) <= tol * std::max (1.0, std::fabs (b)); }

void testJson()
{
    check (jsonStr ("a\"b\\c\n") == "\"a\\\"b\\\\c\\n\"", "jsonStr escapes quote, backslash, newline");
    check (ok ("[1]") == "{\"status\": \"ok\", \"data\": [1]}", "ok() wrapper");
    check (err ("x") == "{\"status\": \"error\", \"message\": \"x\"}", "err() wrapper");
    check (jsonNum (-9.0309) == "-9.031", "jsonNum three decimals");
    check (parseJsonString ("{\"name\": \"VCO\"}", "name") == "VCO", "parseJsonString");
    check (parseJsonString ("{}", "name") == "", "parseJsonString missing -> empty");
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
    check (! guard::isAllowedOrigin ("http://localhost.evil.com"), "spoofed loopback origin refused");
    check (! guard::isAllowedOrigin ("null"), "opaque origin refused");
}
} // namespace

int main()
{
    testJson();
    testGuard();
    std::printf ("%s (%d failures)\n", failures ? "FAILED" : "all ok", failures);
    return failures ? 1 : 0;
}
