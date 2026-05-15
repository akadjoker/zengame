#pragma once

#include <string>

class Node;
class SceneTree;

// ----------------------------------------------------------------------------
// SceneSerializer — save/load a node subtree to/from an XML file via tinyxml2.
//
// Save example:
//   SceneSerializer::save(tree.get_root(), "scene.xml");
//
// Load example:
//   Node* root = SceneSerializer::load(tree, "scene.xml");
//   if (root) tree.set_root(root);
//
// XML format (human-readable, editor-friendly):
//
//   <scene>
//     <node type="Node2D" name="World">
//       <position x="0" y="0"/>
//       <rotation>0</rotation>
//       <scale x="1" y="1"/>
//       <pivot x="0" y="0"/>
//       <visible>1</visible>
//       <node type="Sprite2D" name="Player">
//         <graph>5</graph>
//         <flip_x>0</flip_x>
//         <flip_y>0</flip_y>
//         <color r="255" g="255" b="255" a="255"/>
//         <offset x="0" y="0"/>
//       </node>
//       <node type="Line2D" name="Wall">
//         <width>3</width>
//         <closed>0</closed>
//         <color r="255" g="0" b="0" a="255"/>
//         <points>
//           <p x="0" y="0"/>
//           <p x="200" y="100"/>
//         </points>
//       </node>
//     </node>
//   </scene>
//
// Notes:
//  - Node2D properties are written for any node whose is_node2d() == true.
//  - Type-specific extras (Sprite2D, Line2D, TextNode2D) are written afterwards.
//  - Unknown types on load are created as plain Node (no properties lost — they
//    are still in the XML, just ignored at runtime).
//  - Uses raylib SaveFileText/LoadFileText for cross-platform I/O.
// ----------------------------------------------------------------------------

class SceneSerializer
{
public:
    // Serialise subtree rooted at 'node' into 'path'.
    // Returns true on success.
    static bool save(const Node* node, const std::string& path);

    // Deserialise from 'path', allocate nodes via 'tree', return the root.
    // Returns nullptr on parse error or missing file.
    static Node* load(SceneTree& tree, const std::string& path);
};
