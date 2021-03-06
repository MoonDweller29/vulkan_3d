#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>
#include "magma/glm_inc.h"

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;

    static std::vector<vk::VertexInputBindingDescription> getBindingDescription();
    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions();
};

class Mesh
{
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
public:
    Mesh(
        std::string name,
        std::vector<Vertex> vertices,
        std::vector<uint32_t> indices);
    const std::vector<Vertex> &getVertices() const { return vertices; };
    const std::vector<uint32_t> &getIndices() const { return indices; }
};
