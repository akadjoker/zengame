#include "SceneSerializer.hpp"
#include "tinyxml2.h"

#include "Node.hpp"
#include "Node2D.hpp"
#include "Sprite2D.hpp"
#include "TextNode2D.hpp"
#include "Line2D.hpp"
#include "Rect2D.hpp"
#include "Circle2D.hpp"
#include "Path2D.hpp"
#include "PathFollow2D.hpp"
#include "StaticBody2D.hpp"
#include "CharacterBody2D.hpp"
#include "RigidBody2D.hpp"
#include "Area2D.hpp"
#include "ParticleEmitter2D.hpp"
#include "Skeleton2D.hpp"
 
#include "SceneTree.hpp"

using namespace tinyxml2;

// ────────────────────────────────────────────────────────────────────────────
// Helpers
// ────────────────────────────────────────────────────────────────────────────

static const char* node_type_name(NodeType t)
{
    switch (t)
    {
    case NodeType::Node:              return "Node";
    case NodeType::Node2D:            return "Node2D";
    case NodeType::Sprite2D:          return "Sprite2D";
    case NodeType::TextNode2D:        return "TextNode2D";
    case NodeType::Line2D:            return "Line2D";
    case NodeType::Rect2D:            return "Rect2D";
    case NodeType::Circle2D:          return "Circle2D";
    case NodeType::Path2D:            return "Path2D";
    case NodeType::PathFollow2D:      return "PathFollow2D";
    case NodeType::StaticBody2D:      return "StaticBody2D";
    case NodeType::CharacterBody2D:   return "CharacterBody2D";
    case NodeType::RigidBody2D:       return "RigidBody2D";
    case NodeType::Area2D:            return "Area2D";
    case NodeType::ParticleEmitter2D: return "ParticleEmitter2D";
    case NodeType::TileMap2D:         return "TileMap2D";
    case NodeType::View2D:            return "View2D";
    case NodeType::RayCast2D:         return "RayCast2D";
    case NodeType::ShadowCaster2D:    return "ShadowCaster2D";
    case NodeType::PolyMesh2D:        return "PolyMesh2D";
    case NodeType::Parallax2D:        return "Parallax2D";
    case NodeType::ParallaxLayer2D:   return "ParallaxLayer2D";
    case NodeType::Collider2D:        return "Collider2D";
    case NodeType::CollisionObject2D: return "CollisionObject2D";
    case NodeType::NavigationGrid2D:  return "NavigationGrid2D";
    case NodeType::AnimatedSprite2D:  return "AnimatedSprite2D";
    case NodeType::Skeleton2D:        return "Skeleton2D";
    case NodeType::Bone2D:            return "Bone2D";
    default:                          return "Node";
    }
}

static XMLElement* vec2_elem(XMLDocument& doc, const char* tag, float x, float y)
{
    XMLElement* e = doc.NewElement(tag);
    e->SetAttribute("x", x);
    e->SetAttribute("y", y);
    return e;
}

static XMLElement* color_elem(XMLDocument& doc, Color c)
{
    XMLElement* e = doc.NewElement("color");
    e->SetAttribute("r", (int)c.r);
    e->SetAttribute("g", (int)c.g);
    e->SetAttribute("b", (int)c.b);
    e->SetAttribute("a", (int)c.a);
    return e;
}

// ────────────────────────────────────────────────────────────────────────────
// Save — recursive serialisation
// ────────────────────────────────────────────────────────────────────────────

static void serialise_node(XMLDocument& doc, XMLElement* parent_elem, const Node* node)
{
    XMLElement* elem = doc.NewElement("node");
    elem->SetAttribute("type", node_type_name(node->node_type));
    elem->SetAttribute("name", node->name.c_str());
    elem->SetAttribute("visible", node->visible ? 1 : 0);

    // ── Node2D properties ─────────────────────────────────────────────────
    if (node->is_node2d())
    {
        const Node2D* n2 = static_cast<const Node2D*>(node);
        elem->InsertEndChild(vec2_elem(doc, "position", n2->position.x, n2->position.y));
        elem->InsertEndChild(vec2_elem(doc, "scale",    n2->scale.x,    n2->scale.y));
        elem->InsertEndChild(vec2_elem(doc, "pivot",    n2->pivot.x,    n2->pivot.y));
        elem->InsertEndChild(vec2_elem(doc, "skew",     n2->skew.x,     n2->skew.y));

        XMLElement* rot = doc.NewElement("rotation");
        rot->SetText(n2->rotation);
        elem->InsertEndChild(rot);

        // ── Sprite2D ───────────────────────────────────────────────────────
        if (node->node_type == NodeType::Sprite2D)
        {
            const Sprite2D* sp = static_cast<const Sprite2D*>(node);
            XMLElement* g = doc.NewElement("graph");     g->SetText(sp->graph);          elem->InsertEndChild(g);
            XMLElement* fx = doc.NewElement("flip_x");   fx->SetText(sp->flip_x ? 1 : 0); elem->InsertEndChild(fx);
            XMLElement* fy = doc.NewElement("flip_y");   fy->SetText(sp->flip_y ? 1 : 0); elem->InsertEndChild(fy);
            XMLElement* bl = doc.NewElement("blend");    bl->SetText(sp->blend);          elem->InsertEndChild(bl);
            elem->InsertEndChild(vec2_elem(doc, "offset", sp->offset.x, sp->offset.y));
            elem->InsertEndChild(color_elem(doc, sp->color));
        }

        // ── Line2D ─────────────────────────────────────────────────────────
        else if (node->node_type == NodeType::Line2D)
        {
            const Line2D* ln = static_cast<const Line2D*>(node);
            XMLElement* w = doc.NewElement("width");   w->SetText(ln->width);              elem->InsertEndChild(w);
            XMLElement* c = doc.NewElement("closed");  c->SetText(ln->closed ? 1 : 0);     elem->InsertEndChild(c);
            XMLElement* ec = doc.NewElement("end_caps"); ec->SetText(ln->end_caps ? 1 : 0); elem->InsertEndChild(ec);
            elem->InsertEndChild(color_elem(doc, ln->color));

            XMLElement* pts = doc.NewElement("points");
            for (const Vec2& p : ln->points)
                pts->InsertEndChild(vec2_elem(doc, "p", p.x, p.y));
            elem->InsertEndChild(pts);
        }

        // ── TextNode2D ─────────────────────────────────────────────────────
        else if (node->node_type == NodeType::TextNode2D)
        {
            const TextNode2D* tn = static_cast<const TextNode2D*>(node);
            XMLElement* txt = doc.NewElement("text");  txt->SetText(tn->text.c_str());  elem->InsertEndChild(txt);
            XMLElement* sz  = doc.NewElement("font_size"); sz->SetText(tn->font_size);  elem->InsertEndChild(sz);
            elem->InsertEndChild(color_elem(doc, tn->color));
        }
        // ── Rect2D ──────────────────────────────────────────────────
        else if (node->node_type == NodeType::Rect2D)
        {
            const Rect2D* rn = static_cast<const Rect2D*>(node);
            elem->InsertEndChild(vec2_elem(doc, "size", rn->size.x, rn->size.y));
            elem->InsertEndChild(color_elem(doc, rn->color));
            XMLElement* f  = doc.NewElement("filled");     f->SetText(rn->filled ? 1 : 0);    elem->InsertEndChild(f);
            XMLElement* lw = doc.NewElement("line_width"); lw->SetText(rn->line_width);        elem->InsertEndChild(lw);
        }

        // ── Circle2D ───────────────────────────────────────────────
        else if (node->node_type == NodeType::Circle2D)
        {
            const Circle2D* cn = static_cast<const Circle2D*>(node);
            XMLElement* r  = doc.NewElement("radius");     r->SetText(cn->radius);             elem->InsertEndChild(r);
            elem->InsertEndChild(color_elem(doc, cn->color));
            XMLElement* f  = doc.NewElement("filled");     f->SetText(cn->filled ? 1 : 0);    elem->InsertEndChild(f);
            XMLElement* lw = doc.NewElement("line_width"); lw->SetText(cn->line_width);        elem->InsertEndChild(lw);
        }
        // ── PathFollow2D ───────────────────────────────────────────────────
        else if (node->node_type == NodeType::PathFollow2D)
        {
            const PathFollow2D* pf = static_cast<const PathFollow2D*>(node);
            XMLElement* off = doc.NewElement("offset");  off->SetText(pf->offset);          elem->InsertEndChild(off);
            XMLElement* spd = doc.NewElement("speed");   spd->SetText(pf->speed);           elem->InsertEndChild(spd);
            XMLElement* lp  = doc.NewElement("loop");    lp->SetText(pf->loop ? 1 : 0);     elem->InsertEndChild(lp);
            XMLElement* rot = doc.NewElement("rotate");  rot->SetText(pf->rotate ? 1 : 0);  elem->InsertEndChild(rot);
            XMLElement* ho  = doc.NewElement("h_offset"); ho->SetText(pf->h_offset);        elem->InsertEndChild(ho);
        }

        // ── Skeleton2D — save only the source file reference ───────────────
        else if (node->node_type == NodeType::Skeleton2D)
        {
            const Skeleton2D* sk = static_cast<const Skeleton2D*>(node);
            if (!sk->source_path.empty())
            {
                XMLElement* src = doc.NewElement("source");
                src->SetText(sk->source_path.c_str());
                elem->InsertEndChild(src);
            }
            XMLElement* sp = doc.NewElement("speed"); sp->SetText(sk->speed); elem->InsertEndChild(sp);
            if (!sk->current_animation().empty())
            {
                XMLElement* anim = doc.NewElement("animation");
                anim->SetText(sk->current_animation().c_str());
                elem->InsertEndChild(anim);
            }
            // Children (Bone2D hierarchy) are NOT serialised individually —
            // they are recreated by SkeletonLoader::load() at load time.
            parent_elem->InsertEndChild(elem);
            return;  // skip the child recursion below
        }

        // ── Bone2D — skip when part of a Skeleton2D (handled by SkeletonLoader) ─
        else if (node->node_type == NodeType::Bone2D)
        {
            parent_elem->InsertEndChild(elem);
            return;
        }
    }

    // ── Recurse into children ──────────────────────────────────────────────
    for (size_t i = 0; i < node->get_child_count(); ++i)
        serialise_node(doc, elem, node->get_child(i));

    parent_elem->InsertEndChild(elem);
}

bool SceneSerializer::save(const Node* root, const std::string& path)
{
    if (!root) return false;

    XMLDocument doc;
    doc.InsertFirstChild(doc.NewDeclaration());
    XMLElement* scene_elem = doc.NewElement("scene");
    doc.InsertEndChild(scene_elem);

    serialise_node(doc, scene_elem, root);

    // Serialise to a memory string, then hand to raylib
    XMLPrinter printer;
    doc.Print(&printer);
    // SaveFileText takes non-const char* but does not modify it
    return SaveFileText(path.c_str(), const_cast<char*>(printer.CStr()));
}

// ────────────────────────────────────────────────────────────────────────────
// Load — factory + recursive deserialisation
// ────────────────────────────────────────────────────────────────────────────

static Vec2 read_vec2(const XMLElement* e, float def_x = 0.f, float def_y = 0.f)
{
    if (!e) return {def_x, def_y};
    return { e->FloatAttribute("x", def_x), e->FloatAttribute("y", def_y) };
}

static Color read_color(const XMLElement* e, Color def = WHITE)
{
    if (!e) return def;
    return {
        (unsigned char)e->IntAttribute("r", 255),
        (unsigned char)e->IntAttribute("g", 255),
        (unsigned char)e->IntAttribute("b", 255),
        (unsigned char)e->IntAttribute("a", 255)
    };
}

static Node* create_node_by_type(SceneTree& tree, const char* type_str, const char* name)
{
    const std::string t = type_str ? type_str : "Node";
    const std::string n = name    ? name     : t;

    if (t == "Node2D")            return tree.create<Node2D>(n);
    if (t == "Sprite2D")          return tree.create<Sprite2D>(n);
    if (t == "TextNode2D")        return tree.create<TextNode2D>(n);
    if (t == "Line2D")            return tree.create<Line2D>(n);
    if (t == "Rect2D")            return tree.create<Rect2D>(n);
    if (t == "Circle2D")          return tree.create<Circle2D>(n);
    if (t == "Path2D")            return tree.create<Path2D>(n);
    if (t == "PathFollow2D")      return tree.create<PathFollow2D>(n);
    if (t == "StaticBody2D")      return tree.create<StaticBody2D>(n);
    if (t == "CharacterBody2D")   return tree.create<CharacterBody2D>(n);
    if (t == "RigidBody2D")       return tree.create<RigidBody2D>(n);
    if (t == "Area2D")            return tree.create<Area2D>(n);
    if (t == "ParticleEmitter2D") return tree.create<ParticleEmitter2D>(n);
    // Skeleton2D is handled separately in deserialise_node via SkeletonLoader
    // Fallback: plain Node
    return tree.create<Node>(n);
}

static Node* deserialise_node(SceneTree& tree, const XMLElement* elem)
{
    const char* type_str = elem->Attribute("type");
    const char* name_str = elem->Attribute("name");

    // ── Skeleton2D — load from source file reference ──────────────────────
    // if (type_str && std::string(type_str) == "Skeleton2D")
    // {
    //     const char* src = nullptr;
    //     if (auto* e = elem->FirstChildElement("source")) src = e->GetText();

    //     Skeleton2D* sk = nullptr;
    //     if (src && src[0] != '\0')
    //         sk = SkeletonLoader::load(tree, nullptr, src);
    //     if (!sk)
    //         sk = tree.create<Skeleton2D>(nullptr, name_str ? name_str : "Skeleton2D");

    //     sk->name    = name_str ? name_str : sk->name;
    //     sk->visible = elem->IntAttribute("visible", 1) != 0;
    //     if (auto* e = elem->FirstChildElement("speed"))  sk->speed = e->FloatText(1.f);

    //     // Node2D base properties
    //     sk->position = read_vec2(elem->FirstChildElement("position"));
    //     sk->scale    = read_vec2(elem->FirstChildElement("scale"), 1.f, 1.f);
    //     sk->rotation = 0.f;
    //     if (auto* r  = elem->FirstChildElement("rotation")) sk->rotation = r->FloatText(0.f);

    //     if (auto* e = elem->FirstChildElement("animation"))
    //     {
    //         const char* anim = e->GetText();
    //         if (anim && anim[0] != '\0') sk->play(anim);
    //     }
    //     return sk;
    // }

    Node* node = create_node_by_type(tree, type_str, name_str);
    node->visible = elem->IntAttribute("visible", 1) != 0;

    if (node->is_node2d())
    {
        Node2D* n2 = static_cast<Node2D*>(node);
        n2->position = read_vec2(elem->FirstChildElement("position"));
        n2->scale    = read_vec2(elem->FirstChildElement("scale"), 1.f, 1.f);
        n2->pivot    = read_vec2(elem->FirstChildElement("pivot"));
        n2->skew     = read_vec2(elem->FirstChildElement("skew"));
        if (auto* r = elem->FirstChildElement("rotation")) n2->rotation = r->FloatText(0.f);

        if (node->node_type == NodeType::Sprite2D)
        {
            Sprite2D* sp = static_cast<Sprite2D*>(node);
            if (auto* e = elem->FirstChildElement("graph"))    sp->graph  = e->IntText(0);
            if (auto* e = elem->FirstChildElement("flip_x"))   sp->flip_x = e->IntText(0) != 0;
            if (auto* e = elem->FirstChildElement("flip_y"))   sp->flip_y = e->IntText(0) != 0;
            if (auto* e = elem->FirstChildElement("blend"))    sp->blend  = e->IntText(0);
            sp->offset = read_vec2(elem->FirstChildElement("offset"));
            sp->color  = read_color(elem->FirstChildElement("color"));
        }
        else if (node->node_type == NodeType::Line2D)
        {
            Line2D* ln = static_cast<Line2D*>(node);
            if (auto* e = elem->FirstChildElement("width"))    ln->width    = e->FloatText(2.f);
            if (auto* e = elem->FirstChildElement("closed"))   ln->closed   = e->IntText(0) != 0;
            if (auto* e = elem->FirstChildElement("end_caps")) ln->end_caps = e->IntText(1) != 0;
            ln->color = read_color(elem->FirstChildElement("color"));
            if (auto* pts = elem->FirstChildElement("points"))
            {
                for (auto* p = pts->FirstChildElement("p"); p; p = p->NextSiblingElement("p"))
                    ln->add_point(read_vec2(p));
            }
        }
        else if (node->node_type == NodeType::TextNode2D)
        {
            TextNode2D* tn = static_cast<TextNode2D*>(node);
            if (auto* e = elem->FirstChildElement("text"))      tn->text      = e->GetText() ? e->GetText() : "";
            if (auto* e = elem->FirstChildElement("font_size")) tn->font_size = e->FloatText(16.f);
            tn->color = read_color(elem->FirstChildElement("color"));
        }
        else if (node->node_type == NodeType::Rect2D)
        {
            Rect2D* rn = static_cast<Rect2D*>(node);
            rn->size = read_vec2(elem->FirstChildElement("size"), 64.f, 64.f);
            rn->color = read_color(elem->FirstChildElement("color"));
            if (auto* e = elem->FirstChildElement("filled"))     rn->filled     = e->IntText(1) != 0;
            if (auto* e = elem->FirstChildElement("line_width")) rn->line_width = e->FloatText(1.f);
        }
        else if (node->node_type == NodeType::Circle2D)
        {
            Circle2D* cn = static_cast<Circle2D*>(node);
            if (auto* e = elem->FirstChildElement("radius"))     cn->radius     = e->FloatText(32.f);
            cn->color = read_color(elem->FirstChildElement("color"));
            if (auto* e = elem->FirstChildElement("filled"))     cn->filled     = e->IntText(1) != 0;
            if (auto* e = elem->FirstChildElement("line_width")) cn->line_width = e->FloatText(1.f);
        }
        else if (node->node_type == NodeType::PathFollow2D)
        {
            PathFollow2D* pf = static_cast<PathFollow2D*>(node);
            if (auto* e = elem->FirstChildElement("offset"))   pf->offset   = e->FloatText(0.f);
            if (auto* e = elem->FirstChildElement("speed"))    pf->speed    = e->FloatText(0.f);
            if (auto* e = elem->FirstChildElement("loop"))     pf->loop     = e->IntText(1) != 0;
            if (auto* e = elem->FirstChildElement("rotate"))   pf->rotate   = e->IntText(1) != 0;
            if (auto* e = elem->FirstChildElement("h_offset")) pf->h_offset = e->FloatText(0.f);
        }
    }

    // ── Recurse into children ──────────────────────────────────────────────
    for (auto* child_elem = elem->FirstChildElement("node");
         child_elem;
         child_elem = child_elem->NextSiblingElement("node"))
    {
        Node* child = deserialise_node(tree, child_elem);
        node->add_child(child);
    }

    return node;
}

Node* SceneSerializer::load(SceneTree& tree, const std::string& path)
{
    char* raw = LoadFileText(path.c_str());
    if (!raw) return nullptr;

    XMLDocument doc;
    XMLError err = doc.Parse(raw);
    UnloadFileText(raw);
    if (err != XML_SUCCESS) return nullptr;

    const XMLElement* scene_elem = doc.FirstChildElement("scene");
    if (!scene_elem) return nullptr;

    const XMLElement* root_elem = scene_elem->FirstChildElement("node");
    if (!root_elem) return nullptr;

    return deserialise_node(tree, root_elem);
}
