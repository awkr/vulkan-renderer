#pragma once

#include "inexor/vulkan-renderer/gltf-model/bounding_box.hpp"
#include "inexor/vulkan-renderer/gltf-model/mesh.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace inexor::vulkan_renderer::gltf_model {

struct ModelNode;

///
struct ModelSkin {
    std::string name;

    std::shared_ptr<ModelNode> skeletonRoot = nullptr;

    std::vector<glm::mat4> inverseBindMatrices;

    std::vector<std::shared_ptr<ModelNode>> joints;
};

///
struct ModelNode {
    std::shared_ptr<ModelNode> parent;

    uint32_t index;

    std::vector<std::shared_ptr<ModelNode>> children;

    glm::mat4 matrix;

    std::string name;

    std::shared_ptr<Mesh> mesh;

    std::shared_ptr<ModelSkin> skin;

    int32_t skinIndex = -1;

    glm::vec3 translation{};

    glm::vec3 scale{1.0f};

    glm::quat rotation{};

    BoundingBox bvh;

    BoundingBox aabb;

    glm::mat4 localMatrix() { return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix; }

    glm::mat4 getMatrix() {
        glm::mat4 m = localMatrix();

        std::shared_ptr<ModelNode> p = parent;

        while (p) {
            m = p->localMatrix() * m;
            p = p->parent;
        }

        return m;
    }

    void update(const std::shared_ptr<UniformBufferManager> uniform_buffer_manager) {
        if (mesh) {
            glm::mat4 m = getMatrix();

            if (skin) {
                mesh->uniform_block.matrix = m;

                // Update join matrices.
                glm::mat4 inverseTransform = glm::inverse(m);

                size_t numJoints = std::min((uint32_t)skin->joints.size(), MAX_NUM_JOINTS);

                for (size_t i = 0; i < numJoints; i++) {
                    std::shared_ptr<ModelNode> jointNode = skin->joints[i];

                    glm::mat4 jointMat = jointNode->getMatrix() * skin->inverseBindMatrices[i];
                    jointMat = inverseTransform * jointMat;
                    mesh->uniform_block.joint_matrix[i] = jointMat;
                }

                mesh->uniform_block.joint_count = (float)numJoints;

                spdlog::debug("Updating uniform buffers.");

                // Update uniform buffer.
                uniform_buffer_manager->update_uniform_buffer(mesh->uniform_buffer, &mesh->uniform_block, sizeof(glm::mat4));
            } else {
                spdlog::debug("Updating uniform buffers.");

                // Updat uniform buffer.
                uniform_buffer_manager->update_uniform_buffer(mesh->uniform_buffer, &m, sizeof(glm::mat4));
            }
        }

        for (auto &child : children) {
            child->update(uniform_buffer_manager);
        }
    }
};

} // namespace inexor::vulkan_renderer::gltf_model
