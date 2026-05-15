#include "SaveData.hpp"

// ── Write ─────────────────────────────────────────────────────────────────────

void SaveData::set_int(const std::string& key, int v)
{
    m_data[key] = std::to_string(v);
}

void SaveData::set_float(const std::string& key, float v)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%.7g", (double)v);
    m_data[key] = buf;
}

void SaveData::set_bool(const std::string& key, bool v)
{
    m_data[key] = v ? "1" : "0";
}

void SaveData::set_string(const std::string& key, const std::string& v)
{
    // Escape newlines and backslashes in the value
    std::string escaped;
    escaped.reserve(v.size());
    for (char c : v)
    {
        if      (c == '\n') { escaped += "\\n"; }
        else if (c == '\\') { escaped += "\\\\"; }
        else                { escaped += c; }
    }
    m_data[key] = escaped;
}

// ── Read ──────────────────────────────────────────────────────────────────────

int SaveData::get_int(const std::string& key, int def) const
{
    auto it = m_data.find(key);
    return (it != m_data.end()) ? std::stoi(it->second) : def;
}

float SaveData::get_float(const std::string& key, float def) const
{
    auto it = m_data.find(key);
    return (it != m_data.end()) ? (float)std::stod(it->second) : def;
}

bool SaveData::get_bool(const std::string& key, bool def) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return def;
    return it->second == "1" || it->second == "true";
}

std::string SaveData::get_string(const std::string& key, const std::string& def) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return def;

    // Unescape
    const std::string& raw = it->second;
    std::string result;
    result.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); ++i)
    {
        if (raw[i] == '\\' && i + 1 < raw.size())
        {
            ++i;
            if      (raw[i] == 'n')  result += '\n';
            else if (raw[i] == '\\') result += '\\';
            else                     result += raw[i];
        }
        else
        {
            result += raw[i];
        }
    }
    return result;
}

bool SaveData::has(const std::string& key) const
{
    return m_data.count(key) > 0;
}

void SaveData::erase(const std::string& key)
{
    m_data.erase(key);
}

void SaveData::clear()
{
    m_data.clear();
}

// ── File I/O ──────────────────────────────────────────────────────────────────

bool SaveData::save(const std::string& path) const
{
    // Build the whole file content as a string, then hand it to Raylib.
    std::string buf;
    for (const auto& [key, val] : m_data)
        buf += key + "=" + val + "\n";

    // SaveFileText takes a non-const char* but does not modify it.
    return SaveFileText(path.c_str(), buf.data());
}

bool SaveData::load(const std::string& path)
{
    char* raw = LoadFileText(path.c_str());
    if (!raw) return false;

    // Parse line by line in-place
    char* pos = raw;
    while (*pos)
    {
        // Find end of line
        char* eol = pos;
        while (*eol && *eol != '\n' && *eol != '\r') ++eol;
        const char next_char = *eol;
        *eol = '\0';  // temporarily terminate

        // Process the line
        if (*pos != '\0' && *pos != '#')
        {
            char* eq = strchr(pos, '=');
            if (eq)
            {
                *eq = '\0';
                m_data[pos] = eq + 1;
            }
        }

        // Advance past the newline (handle \r\n)
        if (next_char == '\0') break;
        pos = eol + 1;
        if (next_char == '\r' && *pos == '\n') ++pos;
    }

    UnloadFileText(raw);
    return true;
}

// ── Singleton ─────────────────────────────────────────────────────────────────

SaveData& SaveData::get()
{
    static SaveData s_instance;
    return s_instance;
}
