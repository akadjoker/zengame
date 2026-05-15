#pragma once

#include <string>
#include <unordered_map>

// ----------------------------------------------------------------------------
// SaveData — simple key/value persistence (text file, no dependencies).
//
// Format: one entry per line → "key=value\n"
// Values are always stored as strings and converted on read.
//
// Usage:
//   SaveData& save = SaveData::get();
//   save.set_int("score", 1200);
//   save.set_string("player_name", "Hero");
//   save.save("save.dat");
//
//   SaveData& save = SaveData::get();
//   save.load("save.dat");
//   int score = save.get_int("score", 0);
//
// You can also use separate SaveData instances (e.g. per-level data):
//   SaveData level_data;
//   level_data.load("level1.dat");
// ----------------------------------------------------------------------------

class SaveData
{
public:
    SaveData() = default;

    // ── Write ─────────────────────────────────────────────────────────────────
    void set_int   (const std::string& key, int v);
    void set_float (const std::string& key, float v);
    void set_bool  (const std::string& key, bool v);
    void set_string(const std::string& key, const std::string& v);

    // ── Read ──────────────────────────────────────────────────────────────────
    int         get_int   (const std::string& key, int         def = 0)     const;
    float       get_float (const std::string& key, float       def = 0.0f)  const;
    bool        get_bool  (const std::string& key, bool        def = false)  const;
    std::string get_string(const std::string& key, const std::string& def = "") const;

    bool has(const std::string& key) const;
    void erase(const std::string& key);
    void clear();

    // ── File I/O ──────────────────────────────────────────────────────────────

    // Save to file. Returns true on success.
    bool save(const std::string& path) const;

    // Load from file. Merges with existing keys (existing keys are overwritten).
    // Returns true on success.
    bool load(const std::string& path);

    // ── Global singleton ──────────────────────────────────────────────────────
    // Convenience accessor — same instance across all calls.
    static SaveData& get();

private:
    std::unordered_map<std::string, std::string> m_data;
};
