#include "pch.hpp"
#include "Parallax2D.hpp"

Parallax2D::Parallax2D(const std::string& p_name)
    : Node2D(p_name)
{
    node_type = NodeType::Parallax2D;
}
