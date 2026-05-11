#include "pch.hpp"
#include <raylib.h>
#include <unordered_set>
#include "SceneTree.hpp"
#include "View2D.hpp"
#include "CollisionObject2D.hpp"
#include "Collider2D.hpp"
#include "Node2D.hpp"
#include "SpatialHash2D.hpp"
#include "Collision2D.hpp"
#include "TileMap2D.hpp"
#include "StaticBody2D.hpp"
#include "Collider2D.hpp"
#include "filebuffer.hpp"
#include "tinyxml2.h"
#include <sstream>

static void CollectCollisionObjects(Node* node, std::vector<CollisionObject2D*>& out)
{
    if (!node)
    {
        return;
    }
    if (node->is_collision_obj())
    {
        out.push_back(static_cast<CollisionObject2D*>(node));
    }
    for (size_t i = 0; i < node->get_child_count(); ++i)
    {
        CollectCollisionObjects(node->get_child(i), out);
    }
}

static uint64_t PairKey(const CollisionObject2D* a, const CollisionObject2D* b)
{
    uintptr_t pa = reinterpret_cast<uintptr_t>(a);
    uintptr_t pb = reinterpret_cast<uintptr_t>(b);
    if (pa > pb)
    {
        std::swap(pa, pb);
    }
    const uint64_t ha = static_cast<uint64_t>(std::hash<uintptr_t>{}(pa));
    const uint64_t hb = static_cast<uint64_t>(std::hash<uintptr_t>{}(pb));
    return ha ^ (hb + 0x9e3779b97f4a7c15ULL + (ha << 6) + (ha >> 2));
}

static View2D* FindCurrentCamera(Node* node)
{
    if (!node)
    {
        return nullptr;
    }

    if (node->is_a(NodeType::View2D))
    {
        View2D* cam = static_cast<View2D*>(node);
        if (cam->is_current && cam->visible)
        {
            return cam;
        }
    }

    for (size_t i = 0; i < node->get_child_count(); ++i)
    {
        if (View2D* child_cam = FindCurrentCamera(node->get_child(i)))
        {
            return child_cam;
        }
    }

    return nullptr;
}

static Vec2 SupportPoint(const std::vector<Vec2>& pts, const Vec2& dir)
{
    if (pts.empty())
    {
        return Vec2();
    }
    float best = pts[0].dot(dir);
    Vec2 best_pt = pts[0];
    for (size_t i = 1; i < pts.size(); ++i)
    {
        const float d = pts[i].dot(dir);
        if (d > best)
        {
            best = d;
            best_pt = pts[i];
        }
    }
    return best_pt;
}

static Vec2 ComputeContactPoint(const Collider2D& a, const Collider2D& b, const Vec2& normal)
{
    const Vec2 n = normal.normalised();

    if (a.shape == Collider2D::ShapeType::Circle && b.shape == Collider2D::ShapeType::Circle)
    {
        const Vec2 ca = a.get_world_center();
        const Vec2 cb = b.get_world_center();
        const Vec2 pa = ca + n * a.get_world_radius();
        const Vec2 pb = cb - n * b.get_world_radius();
        return (pa + pb) * 0.5f;
    }

    if (a.shape == Collider2D::ShapeType::Circle)
    {
        const Vec2 ca = a.get_world_center();
        const Vec2 pa = ca + n * a.get_world_radius();
        std::vector<Vec2> bpoly;
        b.get_world_polygon(bpoly);
        const Vec2 pb = bpoly.empty() ? b.get_world_center() : SupportPoint(bpoly, -n);
        return (pa + pb) * 0.5f;
    }

    if (b.shape == Collider2D::ShapeType::Circle)
    {
        const Vec2 cb = b.get_world_center();
        const Vec2 pb = cb - n * b.get_world_radius();
        std::vector<Vec2> apoly;
        a.get_world_polygon(apoly);
        const Vec2 pa = apoly.empty() ? a.get_world_center() : SupportPoint(apoly, n);
        return (pa + pb) * 0.5f;
    }

    std::vector<Vec2> apoly;
    std::vector<Vec2> bpoly;
    a.get_world_polygon(apoly);
    b.get_world_polygon(bpoly);
    const Vec2 pa = apoly.empty() ? a.get_world_center() : SupportPoint(apoly, n);
    const Vec2 pb = bpoly.empty() ? b.get_world_center() : SupportPoint(bpoly, -n);
    return (pa + pb) * 0.5f;
}

// ============================================================
// SceneTree - Implementation
// ============================================================

// ----------------------------------------------------------
// Constructor / Destructor
// ----------------------------------------------------------

SceneTree::SceneTree()
    : m_root(nullptr)
    , m_ui_root(nullptr)
    , m_running(false)
    , m_debug_draw_flags(DEBUG_NONE)
    , m_physics_hash(96.0f)
{
}

SceneTree::~SceneTree()
{
    if (m_root)
    {
        set_tree_recursive(m_root, nullptr);
        m_root->_do_exit_tree();
        delete m_root;
    }

    if (m_ui_root)
    {
        set_tree_recursive(m_ui_root, nullptr);
        m_ui_root->_do_exit_tree();
        delete m_ui_root;
    }
}

// ----------------------------------------------------------
// Scene management
// ----------------------------------------------------------

void SceneTree::set_root(Node* root)
{
    if (m_root)
    {
        set_tree_recursive(m_root, nullptr);
        m_root->_do_exit_tree();
        delete m_root;
    }

    m_root = root;

    if (m_root)
    {
        set_tree_recursive(m_root, this);
        m_root->_do_enter_tree();
        m_root->_do_ready();
    }
}

Node* SceneTree::get_root() const
{
    return m_root;
}

void SceneTree::set_ui_root(Node* root)
{
    if (m_ui_root)
    {
        set_tree_recursive(m_ui_root, nullptr);
        m_ui_root->_do_exit_tree();
        delete m_ui_root;
    }

    m_ui_root = root;

    if (m_ui_root)
    {
        set_tree_recursive(m_ui_root, this);
        m_ui_root->_do_enter_tree();
        m_ui_root->_do_ready();
    }
}

Node* SceneTree::get_ui_root() const
{
    return m_ui_root;
}

// ----------------------------------------------------------
// Game loop
// ----------------------------------------------------------

void SceneTree::process(float dt)
{
    if (m_root && m_running)
    {
        // Advance the per-frame transform cache counter so all Node2D
        // cached world matrices are lazily recomputed on first access.
        static uint32_t s_frame_counter = 0;
        Node2D::begin_frame(++s_frame_counter);

        m_root->propagate_update(dt);
        update_physics();
        cleanup_queued_nodes(m_root);
        cleanup_queued_nodes(m_ui_root);
    }
}

void SceneTree::draw()
{
    draw_world_pass();
    draw_ui_pass();
    draw_overlay_pass();
}

void SceneTree::draw_world_pass()
{
    View2D* cam = get_current_camera();
    if (cam)
    {
        cam->screen_center = Vec2(GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f);
        cam->begin();
    }

    if (m_root)
    {
        m_root->propagate_draw();
        if (m_debug_draw_flags != DEBUG_NONE)
        {
            draw_debug(m_root);
        }
    }

    if (cam)
    {
        cam->end();
    }
}

void SceneTree::draw_ui_pass()
{
    if (m_ui_root)
    {
        m_ui_root->propagate_draw();
    }
}

void SceneTree::draw_overlay_pass()
{
    if (has_debug_draw_flag(DEBUG_TREE))
    {
        draw_tree_overlay();
    }
}

void SceneTree::start()
{
    m_running = true;
}

void SceneTree::quit()
{
    m_running = false;
}

bool SceneTree::is_running() const
{
    return m_running;
}

View2D* SceneTree::get_current_camera() const
{
    return FindCurrentCamera(m_root);
}

void SceneTree::query_collision_candidates(const Rectangle& area,
                                           const CollisionObject2D* self,
                                           std::vector<CollisionObject2D*>& out_candidates) const
{
    out_candidates.clear();

    std::vector<int> ids;
    m_physics_hash.query_aabb(area, ids);
    out_candidates.reserve(ids.size());

    std::vector<Collider2D*> cols;
    for (int id : ids)
    {
        auto it = m_physics_by_id.find(id);
        if (it == m_physics_by_id.end())
        {
            continue;
        }
        CollisionObject2D* obj = it->second;
        if (!obj || obj == self)
        {
            continue;
        }
        if (!obj->enabled || !obj->active)
        {
            continue;
        }
        obj->get_colliders(cols);
        bool any_visible = false;
        for (Collider2D* col : cols)
        {
            if (col && col->visible)
            {
                any_visible = true;
                break;
            }
        }
        if (!any_visible)
        {
            continue;
        }
        out_candidates.push_back(obj);
    }
}

void SceneTree::set_debug_draw_flags(uint32_t flags)
{
    m_debug_draw_flags = flags;
}

uint32_t SceneTree::get_debug_draw_flags() const
{
    return m_debug_draw_flags;
}

bool SceneTree::has_debug_draw_flag(DebugDrawFlags flag) const
{
    return (m_debug_draw_flags & static_cast<uint32_t>(flag)) != 0;
}

void SceneTree::cleanup_queued_nodes(Node* node)
{
    if (!node)
    {
        return;
    }

    for (size_t i = node->get_child_count(); i > 0; --i)
    {
        Node* child = node->get_child(i - 1);
        if (!child)
        {
            continue;
        }

        cleanup_queued_nodes(child);

        if (child->is_queued_for_deletion())
        {
            node->destroy_child(child);
        }
    }
}

void SceneTree::draw_debug(Node* node) const
{
    if (!node || !node->visible)
    {
        return;
    }

    if (node->is_node2d())
    {
        Node2D* n2d = static_cast<Node2D*>(node);
        const Vec2 p = n2d->get_global_position();

        if (has_debug_draw_flag(DEBUG_PIVOT))
        {
            DrawCircleV(Vector2{p.x, p.y}, 3.0f, YELLOW);
        }

        if (has_debug_draw_flag(DEBUG_DIRECTION))
        {
            const float len = 26.0f;
            const float rad = n2d->rotation * DEG_TO_RAD;
            const Vector2 to = {p.x + std::cos(rad) * len, p.y + std::sin(rad) * len};
            DrawLineV(Vector2{p.x, p.y}, to, SKYBLUE);
        }

        if (has_debug_draw_flag(DEBUG_NAMES))
        {
            DrawText(node->name.c_str(), static_cast<int>(p.x) + 6, static_cast<int>(p.y) - 16, 12, LIGHTGRAY);
        }
    }

    if (has_debug_draw_flag(DEBUG_SHAPES) && node->is_a(NodeType::Collider2D))
    {
        Collider2D* col = static_cast<Collider2D*>(node);
        if (col->shape == Collider2D::ShapeType::Circle)
        {
            const Vec2 c = col->get_world_center();
            DrawCircleLines(static_cast<int>(c.x), static_cast<int>(c.y), col->get_world_radius(), GREEN);
        }
        else
        {
            std::vector<Vec2> poly;
            col->get_world_polygon(poly);
            if (poly.size() >= 2)
            {
                for (size_t i = 0; i < poly.size(); ++i)
                {
                    const Vec2& a = poly[i];
                    const Vec2& b = poly[(i + 1) % poly.size()];
                    DrawLineV(Vector2{a.x, a.y}, Vector2{b.x, b.y}, GREEN);
                }
            }
        }
    }

    if (has_debug_draw_flag(DEBUG_SPATIAL) && node == m_root)
    {
        m_physics_hash.debug_draw_cells(Color{40, 220, 80, 120});

        std::unordered_set<const CollisionObject2D*> colliding;
        colliding.reserve(m_debug_contacts.size() * 2 + 1);
        for (const DebugContact& c : m_debug_contacts)
        {
            if (c.a) colliding.insert(c.a);
            if (c.b) colliding.insert(c.b);
        }

        for (const auto& it : m_physics_by_id)
        {
            CollisionObject2D* obj = it.second;
            if (!obj) continue;
            Collider2D* col = obj->get_collider();
            if (!col) continue;
            Rectangle r = col->get_world_aabb();
            const bool is_colliding = colliding.find(obj) != colliding.end();
            const Color c = is_colliding ? RED : Color{240, 190, 60, 255};
            DrawRectangleLines((int)r.x, (int)r.y, (int)r.width, (int)r.height, c);
        }
    }

    if (has_debug_draw_flag(DEBUG_CONTACTS) && node == m_root)
    {
        for (const DebugContact& c : m_debug_contacts)
        {
            const Vector2 p0{c.point.x, c.point.y};
            const Vector2 p1{c.point.x + c.normal.x * 26.0f, c.point.y + c.normal.y * 26.0f};
            DrawCircleV(p0, 4.0f, ORANGE);
            DrawLineEx(p0, p1, 2.0f, MAGENTA);
            DrawText(TextFormat("d=%.2f", c.depth), (int)c.point.x + 6, (int)c.point.y + 6, 10, ORANGE);
        }
    }

    for (size_t i = 0; i < node->get_child_count(); ++i)
    {
        draw_debug(node->get_child(i));
    }
}

void SceneTree::update_physics()
{
    m_debug_contacts.clear();

    std::vector<CollisionObject2D*> objects;
    objects.reserve(64);
    CollectCollisionObjects(m_root, objects);

    m_physics_hash.clear();
    m_physics_by_id.clear();
    int next_id = 1;

    for (CollisionObject2D* obj : objects)
    {
        if (!obj || !obj->enabled || !obj->active)
        {
            continue;
        }
        std::vector<Collider2D*> colliders;
        obj->get_colliders(colliders);
        if (colliders.empty())
        {
            continue;
        }

        bool has_visible = false;
        Rectangle merged = {};
        bool merged_valid = false;
        for (Collider2D* collider : colliders)
        {
            if (!collider || !collider->visible)
            {
                continue;
            }
            const Rectangle r = collider->get_world_aabb();
            if (!merged_valid)
            {
                merged = r;
                merged_valid = true;
            }
            else
            {
                const float min_x = std::min(merged.x, r.x);
                const float min_y = std::min(merged.y, r.y);
                const float max_x = std::max(merged.x + merged.width, r.x + r.width);
                const float max_y = std::max(merged.y + merged.height, r.y + r.height);
                merged.x = min_x;
                merged.y = min_y;
                merged.width = max_x - min_x;
                merged.height = max_y - min_y;
            }
            has_visible = true;
        }
        if (!has_visible || !merged_valid)
        {
            continue;
        }

        SpatialBody2D body;
        body.id = next_id++;
        body.user_data = obj;
        body.aabb = merged;
        body.layer = obj->collision_layer;
        body.mask = obj->collision_mask;
        m_physics_hash.upsert(body);
        m_physics_by_id[body.id] = obj;
    }

    std::vector<std::pair<int, int>> pairs;
    m_physics_hash.query_pairs(pairs);

    std::unordered_map<uint64_t, std::pair<CollisionObject2D*, CollisionObject2D*>> current_collisions;
    for (const auto& ids : pairs)
    {
        auto ita = m_physics_by_id.find(ids.first);
        auto itb = m_physics_by_id.find(ids.second);
        if (ita == m_physics_by_id.end() || itb == m_physics_by_id.end())
        {
            continue;
        }

        CollisionObject2D* a = ita->second;
        CollisionObject2D* b = itb->second;
        Collider2D* col_a = a->get_collider();
        Collider2D* col_b = b->get_collider();
        if (!col_a || !col_b)
        {
            continue;
        }

        Contact2D contact;
        if (!Collision2D::Collide(*col_a, *col_b, &contact))
        {
            continue;
        }

        if (has_debug_draw_flag(DEBUG_CONTACTS))
        {
            DebugContact dc;
            dc.point = ComputeContactPoint(*col_a, *col_b, contact.normal);
            dc.normal = contact.normal;
            dc.depth = contact.depth;
            dc.a = a;
            dc.b = b;
            m_debug_contacts.push_back(dc);
        }

        const uint64_t key = PairKey(a, b);
        current_collisions[key] = {a, b};

        if (m_prev_collisions.find(key) == m_prev_collisions.end())
        {
            a->on_collision_enter(b);
            b->on_collision_enter(a);
        }
        else
        {
            a->on_collision_stay(b);
            b->on_collision_stay(a);
        }
    }

    for (const auto& it : m_prev_collisions)
    {
        if (current_collisions.find(it.first) == current_collisions.end())
        {
            CollisionObject2D* a = it.second.first;
            CollisionObject2D* b = it.second.second;
            if (a) a->on_collision_exit(b);
            if (b) b->on_collision_exit(a);
        }
    }

    m_prev_collisions.swap(current_collisions);
}

void SceneTree::set_tree_recursive(Node* node, SceneTree* tree)
{
    if (!node)
    {
        return;
    }
    node->m_tree = tree;
    for (size_t i = 0; i < node->get_child_count(); ++i)
    {
        set_tree_recursive(node->get_child(i), tree);
    }
}

void SceneTree::draw_tree_overlay() const
{
    if (!m_root)
    {
        return;
    }

    std::vector<std::string> lines;
    lines.reserve(128);
    collect_tree_lines(m_root, 0, lines);

    int max_chars = 0;
    for (const std::string& l : lines)
    {
        max_chars = std::max(max_chars, (int)l.size());
    }

    const int line_h = 16;
    const int box_w = std::max(240, max_chars * 8 + 20);
    const int box_h = (int)lines.size() * line_h + 14;
    DrawRectangle(10, 10, box_w, box_h, ColorAlpha(BLACK, 0.65f));
    DrawRectangleLines(10, 10, box_w, box_h, ColorAlpha(RAYWHITE, 0.5f));

    int y = 18;
    for (const std::string& l : lines)
    {
        DrawText(l.c_str(), 16, y, 12, RAYWHITE);
        y += line_h;
    }
}

void SceneTree::collect_tree_lines(const Node* node, int depth, std::vector<std::string>& out_lines) const
{
    if (!node)
    {
        return;
    }

    std::string indent(depth * 2, ' ');
    std::string line = indent + "- " + node->name;
    if (!node->active) line += " [inactive]";
    if (!node->visible) line += " [hidden]";
    out_lines.push_back(line);

    for (size_t i = 0; i < node->get_child_count(); ++i)
    {
        collect_tree_lines(node->get_child(i), depth + 1, out_lines);
    }
}

// -----------------------------------------------------------------------------
// load_tilemap_from_tmx
//
// Parses a .tmx file and creates one TileMap2D child node per tile layer.
// All layers share the same tileset (first tileset in the file).
// Layers are added as children of `parent` in order (layer 0 first).
// Returns the number of layers created, or -1 on parse error.
// Never throws — errors reported via TraceLog.
// -----------------------------------------------------------------------------
int SceneTree::load_tilemap_from_tmx(const char* tmx_path, Node* parent)
{
    if (!parent)
    {
        TraceLog(LOG_ERROR, "SceneTree::load_tilemap_from_tmx: parent is null");
        return -1;
    }

    // 1. Read file
    FileBuffer file;
    if (!file.load(tmx_path))
    {
        TraceLog(LOG_ERROR, "SceneTree::load_tilemap_from_tmx: cannot open '%s'", tmx_path);
        return -1;
    }

    // 2. Parse XML
    tinyxml2::XMLDocument doc;
    if (doc.Parse(file.c_str(), file.size()) != tinyxml2::XML_SUCCESS)
    {
        TraceLog(LOG_ERROR, "SceneTree::load_tilemap_from_tmx: XML error in '%s': %s",
                 tmx_path, doc.ErrorStr());
        return -1;
    }

    tinyxml2::XMLElement* mapElem = doc.FirstChildElement("map");
    if (!mapElem)
    {
        TraceLog(LOG_ERROR, "SceneTree::load_tilemap_from_tmx: no <map> in '%s'", tmx_path);
        return -1;
    }

    // 3. Count tile layers
    int layer_count = 0;
    for (tinyxml2::XMLElement* e = mapElem->FirstChildElement("layer"); e;
         e = e->NextSiblingElement("layer"))
        ++layer_count;

    if (layer_count == 0)
    {
        TraceLog(LOG_WARNING, "SceneTree::load_tilemap_from_tmx: no <layer> in '%s'", tmx_path);
        return 0;
    }

    // 4. Create one TileMap2D per layer via TileMap2D::load_from_tmx
    int created = 0;
    for (int i = 0; i < layer_count; ++i)
    {
        std::string layer_name = "TileLayer" + std::to_string(i);

        // Get the actual layer name from XML if available
        tinyxml2::XMLElement* le = mapElem->FirstChildElement("layer");
        for (int k = 0; k < i && le; ++k)
            le = le->NextSiblingElement("layer");
        if (le && le->Attribute("name"))
            layer_name = le->Attribute("name");

        TileMap2D* tm = new TileMap2D(layer_name);
        if (tm->load_from_tmx(tmx_path, i))
        {
            parent->add_child(tm);
            ++created;
        }
        else
        {
            delete tm;
            TraceLog(LOG_WARNING,
                     "SceneTree::load_tilemap_from_tmx: failed to load layer %d", i);
        }
    }

    TraceLog(LOG_INFO, "SceneTree::load_tilemap_from_tmx: '%s' -> %d/%d layers",
             tmx_path, created, layer_count);
    return created;
}

// -----------------------------------------------------------------------------
// load_collision_from_tmx
//
// Parses all <objectgroup> elements in a .tmx file.
// For each <object> creates a StaticBody2D with a Collider2D child:
//   - plain rect  → Collider2D::Rectangle  (points = rect corners)
//   - <ellipse/>  → Collider2D::Circle     (radius = min(w,h)/2)
//   - <polygon/>  → Collider2D::Polygon    (local points from "points" attr)
// Returns number of bodies created, or -1 on parse error.
// Never throws — errors via TraceLog.
// -----------------------------------------------------------------------------
int SceneTree::load_collision_from_tmx(const char* tmx_path, Node* parent)
{
    if (!parent)
    {
        TraceLog(LOG_ERROR, "SceneTree::load_collision_from_tmx: parent is null");
        return -1;
    }

    // 1. Read and parse XML
    FileBuffer file;
    if (!file.load(tmx_path))
    {
        TraceLog(LOG_ERROR, "SceneTree::load_collision_from_tmx: cannot open '%s'", tmx_path);
        return -1;
    }

    tinyxml2::XMLDocument doc;
    if (doc.Parse(file.c_str(), file.size()) != tinyxml2::XML_SUCCESS)
    {
        TraceLog(LOG_ERROR, "SceneTree::load_collision_from_tmx: XML error in '%s': %s",
                 tmx_path, doc.ErrorStr());
        return -1;
    }

    tinyxml2::XMLElement* mapElem = doc.FirstChildElement("map");
    if (!mapElem)
    {
        TraceLog(LOG_ERROR, "SceneTree::load_collision_from_tmx: no <map> in '%s'", tmx_path);
        return -1;
    }

    int created = 0;
    int obj_seq = 0;

    // 2. Iterate objectgroups
    for (tinyxml2::XMLElement* grp = mapElem->FirstChildElement("objectgroup");
         grp; grp = grp->NextSiblingElement("objectgroup"))
    {
        for (tinyxml2::XMLElement* obj = grp->FirstChildElement("object");
             obj; obj = obj->NextSiblingElement("object"))
        {
            const float ox = obj->FloatAttribute("x");
            const float oy = obj->FloatAttribute("y");
            const float ow = obj->FloatAttribute("width",  0.0f);
            const float oh = obj->FloatAttribute("height", 0.0f);

            // Build a unique name: prefer the "name" attribute from the TMX
            std::string body_name;
            if (const char* n = obj->Attribute("name"); n && n[0] != '\0')
                body_name = n;
            else
                body_name = "Collision_" + std::to_string(obj_seq);
            ++obj_seq;

            // Create body
            StaticBody2D* body = new StaticBody2D(body_name);
            body->position = Vec2(ox, oy);

            Collider2D* col = new Collider2D("Shape");

            if (obj->FirstChildElement("ellipse"))
            {
                // Ellipse → circle at centre of the bounding box
                col->shape  = Collider2D::ShapeType::Circle;
                col->radius = std::min(ow, oh) * 0.5f;
                // Offset collider to centre (Tiled gives top-left for ellipses too)
                col->position = Vec2(ow * 0.5f, oh * 0.5f);
            }
            else if (tinyxml2::XMLElement* polyElem = obj->FirstChildElement("polygon"))
            {
                // Polygon — points are space-separated "x,y" pairs, local to object
                col->shape = Collider2D::ShapeType::Polygon;
                col->points.clear();

                const char* pts_str = polyElem->Attribute("points");
                if (pts_str)
                {
                    // Parse "x1,y1 x2,y2 ..."
                    std::istringstream ss(pts_str);
                    std::string token;
                    while (ss >> token)
                    {
                        const size_t comma = token.find(',');
                        if (comma != std::string::npos)
                        {
                            const float px = std::stof(token.substr(0, comma));
                            const float py = std::stof(token.substr(comma + 1));
                            col->points.emplace_back(px, py);
                        }
                    }
                }
            }
            else
            {
                // Default: rectangle
                col->shape = Collider2D::ShapeType::Rectangle;
                col->points = {
                    Vec2(0.0f, 0.0f),
                    Vec2(ow,   0.0f),
                    Vec2(ow,   oh),
                    Vec2(0.0f, oh)
                };
                col->size = Vec2(ow, oh);
            }

            body->add_child(col);
            parent->add_child(body);
            ++created;
        }
    }

    TraceLog(LOG_INFO, "SceneTree::load_collision_from_tmx: '%s' -> %d bodies",
             tmx_path, created);
    return created;
}
