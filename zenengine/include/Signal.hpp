#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// ----------------------------------------------------------------------------
// SignalArg — lightweight variant for signal arguments.
//
// Supported types: nil, int, float, bool, string, ptr (Node* or any raw ptr).
//
// C++ usage:
//   node->emit_signal("hit", { SignalArg::from_int(damage) });
//   node->connect("hit", [](const SignalArgs& a) {
//       int dmg = (int)a[0].as_int();
//   });
//
// zenpy binding: ScriptHost::on_signal marshals SignalArgs → zenpy Values.
// ----------------------------------------------------------------------------

struct SignalArg
{
    enum Type : uint8_t { Nil, Int, Float, Bool, Str, Ptr } type = Nil;

    union { int64_t i; double f; bool b; void* p; } val = {};
    std::string s;  // outside union — proper construction/destruction

    static SignalArg nil()                             { return {}; }
    static SignalArg from_int   (int64_t v)            { SignalArg a; a.type = Int;   a.val.i = v; return a; }
    static SignalArg from_float (double v)             { SignalArg a; a.type = Float; a.val.f = v; return a; }
    static SignalArg from_bool  (bool v)               { SignalArg a; a.type = Bool;  a.val.b = v; return a; }
    static SignalArg from_ptr   (void* v)              { SignalArg a; a.type = Ptr;   a.val.p = v; return a; }
    static SignalArg from_string(const std::string& v) { SignalArg a; a.type = Str;   a.s = v;     return a; }

    int64_t            as_int()    const { return val.i; }
    double             as_float()  const { return val.f; }
    bool               as_bool()   const { return val.b; }
    void*              as_ptr()    const { return val.p; }
    const std::string& as_string() const { return s; }
};

using SignalArgs     = std::vector<SignalArg>;
using SignalCallback = std::function<void(const SignalArgs&)>;

// A registered connection.
// tag is used to identify/disconnect a group of connections (usually target Node*).
struct SignalConnection
{
    SignalCallback callback;
    void*          tag = nullptr;   // nullptr = anonymous (can't be disconnected by tag)
};
