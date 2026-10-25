// Json.h: the tiny JSON builders and key-pickers the server uses (bodies are small and the shapes are ours).
// No Rack dependency. jansson is used where a whole document must be parsed (module state).
// Author: Ari Klopfer / A_Development. MIT (forked from Neural Harmonics MCP Server, MIT)
#pragma once
#include <cstdio>
#include <string>

inline std::string jsonStr (const std::string& s)
{
    std::string out = "\"";
    for (char c : s)
    {
        switch (c)
        {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out + "\"";
}

inline std::string jsonNum (double v)
{
    char b[32];
    std::snprintf (b, sizeof b, "%.3f", v);
    return b;
}

inline std::string jsonKV (const std::string& k, const std::string& v, bool last = false) { return jsonStr (k) + ": " + v + (last ? "" : ", "); }
inline std::string jsonKVs (const std::string& k, const std::string& s, bool last = false) { return jsonStr (k) + ": " + jsonStr (s) + (last ? "" : ", "); }
inline std::string ok (const std::string& body) { return "{" + jsonKVs ("status", "ok") + jsonKV ("data", body, true) + "}"; }
inline std::string err (const std::string& msg) { return "{" + jsonKVs ("status", "error") + jsonKVs ("message", msg, true) + "}"; }

// Decodes a JSON string body (the text between the quotes): \" \\ \/ \b \f \n \r \t and \uXXXX (UTF-16 pairs to UTF-8).
inline std::string jsonUnescape (const std::string& s)
{
    std::string out;
    auto putUtf8 = [&out] (unsigned cp)
    {
        if (cp < 0x80) out += (char) cp;
        else if (cp < 0x800) { out += (char) (0xC0 | (cp >> 6)); out += (char) (0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000) { out += (char) (0xE0 | (cp >> 12)); out += (char) (0x80 | ((cp >> 6) & 0x3F)); out += (char) (0x80 | (cp & 0x3F)); }
        else { out += (char) (0xF0 | (cp >> 18)); out += (char) (0x80 | ((cp >> 12) & 0x3F)); out += (char) (0x80 | ((cp >> 6) & 0x3F)); out += (char) (0x80 | (cp & 0x3F)); }
    };
    auto hex4 = [&s] (size_t at, unsigned& v)
    {
        if (at + 4 > s.size()) return false;
        v = 0;
        for (size_t i = at; i < at + 4; ++i)
        {
            char c = s[i]; v <<= 4;
            if (c >= '0' && c <= '9') v |= c - '0'; else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10; else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10; else return false;
        }
        return true;
    };
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] != '\\' || i + 1 >= s.size()) { out += s[i]; continue; }
        char e = s[++i];
        switch (e)
        {
            case '"': out += '"'; break;   case '\\': out += '\\'; break;   case '/': out += '/'; break;
            case 'b': out += '\b'; break;  case 'f': out += '\f'; break;    case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;  case 't': out += '\t'; break;
            case 'u':
            {
                unsigned cp;
                if (! hex4 (i + 1, cp)) { out += "\\u"; break; }
                i += 4;
                if (cp >= 0xD800 && cp <= 0xDBFF && i + 6 < s.size() && s[i + 1] == '\\' && s[i + 2] == 'u')
                {
                    unsigned lo;
                    if (hex4 (i + 3, lo) && lo >= 0xDC00 && lo <= 0xDFFF) { cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00); i += 6; }
                }
                putUtf8 (cp);
                break;
            }
            default: out += '\\'; out += e; break;
        }
    }
    return out;
}

inline std::string parseJsonString (const std::string& json, const std::string& key)
{
    std::string searchKey = "\"" + key + "\"";
    auto pos = json.find (searchKey);
    if (pos == std::string::npos) return "";
    pos = json.find (':', pos + searchKey.size());
    if (pos == std::string::npos) return "";
    pos = json.find ('"', pos + 1);
    if (pos == std::string::npos) return "";
    auto end = pos + 1;
    while (end < json.size() && json[end] != '"') { if (json[end] == '\\') ++end; ++end; }
    if (end >= json.size()) return "";
    return jsonUnescape (json.substr (pos + 1, end - pos - 1));
}

inline double parseJsonDouble (const std::string& json, const std::string& key, double def = 0.0)
{
    std::string searchKey = "\"" + key + "\"";
    auto pos = json.find (searchKey);
    if (pos == std::string::npos) return def;
    pos = json.find (':', pos + searchKey.size());
    if (pos == std::string::npos) return def;
    while (pos < json.size() && (json[pos] == ':' || json[pos] == ' ')) pos++;
    if (pos < json.size() && json[pos] == '"') pos++;   // agents sometimes send numbers as strings
    try { return std::stod (json.substr (pos)); } catch (...) { return def; }
}

// The raw JSON value (string, number, object, array, bool, null) for a key.
inline std::string parseRawValue (const std::string& json, const std::string& key)
{
    std::string sk = "\"" + key + "\"";
    auto p = json.find (sk);
    if (p == std::string::npos) return "";
    p += sk.size();
    while (p < json.size() && (json[p] == ' ' || json[p] == ':' || json[p] == '\t' || json[p] == '\n')) p++;
    if (p >= json.size()) return "";
    char c = json[p];
    if (c == '"')
    {
        auto e = p + 1;
        while (e < json.size())
        {
            if (json[e] == '\\') { e += 2; continue; }
            if (json[e] == '"') break;
            e++;
        }
        return json.substr (p, e - p + 1);
    }
    if (c == '{' || c == '[')
    {
        char open = c, close = (c == '{') ? '}' : ']';
        int depth = 0;
        auto start = p;
        bool inStr = false;
        for (; p < json.size(); p++)
        {
            char ch = json[p];
            if (ch == '\\') { p++; continue; }
            if (ch == '"') { inStr = ! inStr; continue; }
            if (inStr) continue;
            if (ch == open) depth++;
            else if (ch == close && --depth == 0) return json.substr (start, p - start + 1);
        }
        return "";
    }
    auto e = p;
    while (e < json.size() && json[e] != ',' && json[e] != '}' && json[e] != ']' && json[e] != ' ' && json[e] != '\n' && json[e] != '\r') e++;
    return json.substr (p, e - p);
}

// The JSON-RPC "id" as raw JSON, so a number stays a number and a string stays quoted.
inline std::string parseJsonRpcId (const std::string& body)
{
    std::string key = "\"id\"";
    auto pos = body.find (key);
    if (pos == std::string::npos) return "null";
    pos += key.size();
    while (pos < body.size() && (body[pos] == ' ' || body[pos] == ':' || body[pos] == '\t')) pos++;
    if (pos >= body.size()) return "null";
    char c = body[pos];
    if (c == '"')
    {
        auto end = pos + 1;
        while (end < body.size())
        {
            if (body[end] == '\\') { end += 2; continue; }
            if (body[end] == '"') break;
            end++;
        }
        return body.substr (pos, end - pos + 1);
    }
    if (c == 'n') return "null";
    auto end = pos;
    while (end < body.size() && body[end] != ',' && body[end] != '}' && body[end] != ' ' && body[end] != '\r' && body[end] != '\n') end++;
    return body.substr (pos, end - pos);
}

inline std::string mcpOk (const std::string& id, const std::string& result) { return "{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}"; }
inline std::string mcpErr (const std::string& id, int code, const std::string& msg)
{
    return "{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"error\":{\"code\":" + std::to_string (code) + ",\"message\":" + jsonStr (msg) + "}}";
}
inline std::string toolOk (const std::string& text) { return "{\"content\":[{\"type\":\"text\",\"text\":" + jsonStr (text) + "}],\"isError\":false}"; }
inline std::string toolFail (const std::string& text) { return "{\"content\":[{\"type\":\"text\",\"text\":" + jsonStr (text) + "}],\"isError\":true}"; }
