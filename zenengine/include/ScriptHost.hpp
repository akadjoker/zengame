#pragma once

#include "Signal.hpp"

// ----------------------------------------------------------------------------
// ScriptHost — pure virtual interface between the C++ engine and a script VM.
//
// The engine (Node) does NOT depend on any specific scripting language.
// The zenpy binding layer (or any other VM) provides a concrete implementation.
//
// Lifecycle mirrors Godot:
//
//   Engine calls   → ScriptHost dispatches to VM
//   _ready()       → vm.invoke(instance, "_ready",  nullptr, 0)
//   _update(dt)    → vm.invoke(instance, "_update", {val_float(dt)}, 1)
//   _draw()        → vm.invoke(instance, "_draw",   nullptr, 0)
//   _on_destroy()  → vm.invoke(instance, "_on_destroy", nullptr, 0)
//
// The C++ class defines its own overrides first; the script is called
// AFTER the C++ implementation unless the script explicitly replaces it.
//
// To attach a script to a node from C++:
//
//   auto* host = new ZenScriptHost(vm, instance);   // zenpy binding
//   node->set_script_host(host);
//
// To attach from zenpy script:
//
//   class MyPlayer(CharacterBody2D):
//       def _ready(self):       ...
//       def _update(self, dt):  ...
//       def _draw(self):        ...
//
//   tree.create_scripted("MyPlayer", parent, source)
// ----------------------------------------------------------------------------

class Node;

class ScriptHost
{
public:
    virtual ~ScriptHost() = default;

    // Called once after the node enters the scene tree.
    virtual void on_ready   (Node* node)             = 0;

    // Called every frame before children.
    virtual void on_update  (Node* node, float dt)   = 0;

    // Called every frame for drawing (after C++ draw).
    virtual void on_draw    (Node* node)             = 0;

    // Called when the node is about to be destroyed.
    virtual void on_destroy (Node* node)             = 0;

    // Called when the node emits a signal (C++ side).
    // The binding can forward this to a script-level connect() call.
    virtual void on_signal  (Node* node, const char* signal, const SignalArgs& args) {}

    // Optional: returns true if the script defines the named method.
    // Lets the engine skip the overhead of calling a missing method.
    virtual bool has_method (const char* name) const { return true; }
};
