#define CGLTF_IMPLEMENTATION
#include "DirectorDesk/Asset/IModelLoader.h"

#include "MeshUtils.h"
#include "TextureDecode.h"

#include "DirectorDesk/Platform/Paths.h"

#include <cgltf.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>

#include <cstring>
#include <memory>

namespace DirectorDesk::Asset {
namespace {

std::uint32_t PackAbgr(const float* color, cgltf_size components) {
    const float r = components > 0 ? color[0] : 1.0f;
    const float g = components > 1 ? color[1] : 1.0f;
    const float b = components > 2 ? color[2] : 1.0f;
    const float a = components > 3 ? color[3] : 1.0f;
    auto toByte = [](float value) {
        const float clamped = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
        return static_cast<std::uint32_t>(clamped * 255.0f + 0.5f);
    };
    return toByte(a) << 24 | toByte(b) << 16 | toByte(g) << 8 | toByte(r);
}

glm::mat4 NodeWorld(const cgltf_node* node) {
    cgltf_float matrix[16];
    cgltf_node_transform_world(node, matrix);
    return glm::make_mat4(matrix);
}

bool ReadVec3(const cgltf_accessor* accessor, cgltf_size index, glm::vec3& out) {
    float values[3] = {0.0f, 0.0f, 0.0f};
    if (accessor == nullptr || !cgltf_accessor_read_float(accessor, index, values, 3)) {
        return false;
    }
    out = glm::vec3(values[0], values[1], values[2]);
    return true;
}

bool ReadVec2(const cgltf_accessor* accessor, cgltf_size index, glm::vec2& out) {
    float values[2] = {0.0f, 0.0f};
    if (accessor == nullptr || !cgltf_accessor_read_float(accessor, index, values, 2)) {
        return false;
    }
    out = glm::vec2(values[0], values[1]);
    return true;
}

const cgltf_accessor* Attribute(const cgltf_primitive& primitive, cgltf_attribute_type type) {
    for (cgltf_size i = 0; i < primitive.attributes_count; ++i) {
        if (primitive.attributes[i].type == type && primitive.attributes[i].index == 0) {
            return primitive.attributes[i].data;
        }
    }
    return nullptr;
}

Core::Result<DecodedImage> LoadGltfImage(const cgltf_image* image) {
    if (image == nullptr) {
        return Core::Result<DecodedImage>::Fail(Core::Error::Make(
            Core::ErrorCode::NotFound, "Material texture image is missing", "缺少纹理"));
    }
    if (image->buffer_view != nullptr && image->buffer_view->buffer != nullptr &&
        image->buffer_view->buffer->data != nullptr) {
        const auto* begin = static_cast<const std::uint8_t*>(image->buffer_view->buffer->data) +
                            image->buffer_view->offset;
        return DecodeImageMemory(begin, image->buffer_view->size);
    }
    return Core::Result<DecodedImage>::Fail(Core::Error::Make(
        Core::ErrorCode::Unsupported, "External glTF image URIs are not loaded for GLB",
        "暂不支持外部纹理路径"));
}

} // namespace

class GlbLoader final : public IModelLoader {
public:
    [[nodiscard]] bool CanLoad(const std::string& extensionUtf8) const override {
        return extensionUtf8 == ".glb";
    }

    Core::Result<ModelData> Load(const std::string& utf8Path) const override {
        auto bytes = Platform::Paths::ReadBinaryFile(utf8Path);
        if (!bytes.IsOk()) {
            return Core::Result<ModelData>::Fail(bytes.GetError());
        }

        cgltf_options options{};
        cgltf_data* data = nullptr;
        cgltf_result parsed =
            cgltf_parse(&options, bytes.Value().data(), bytes.Value().size(), &data);
        if (parsed != cgltf_result_success || data == nullptr) {
            return Core::Result<ModelData>::Fail(Core::Error::Make(
                Core::ErrorCode::ParseFailure, "cgltf_parse failed", "无法解析 GLB 文件"));
        }

        std::unique_ptr<cgltf_data, void (*)(cgltf_data*)> guard(data, cgltf_free);
        if (cgltf_load_buffers(&options, data, nullptr) != cgltf_result_success) {
            return Core::Result<ModelData>::Fail(Core::Error::Make(Core::ErrorCode::ParseFailure,
                                                                   "cgltf_load_buffers failed",
                                                                   "无法读取 GLB 网格数据"));
        }

        ModelData model;
        model.sourcePath = utf8Path;
        model.name = Platform::Paths::Stem(utf8Path);
        if (data->skins_count > 0) {
            model.warnings.emplace_back("已忽略骨骼动画");
        }
        if (data->animations_count > 0) {
            model.warnings.emplace_back("已忽略动画轨道");
        }

        model.materials.resize(data->materials_count);
        for (cgltf_size i = 0; i < data->materials_count; ++i) {
            const cgltf_material& src = data->materials[i];
            Material& dst = model.materials[i];
            if (src.has_pbr_metallic_roughness) {
                const float* color = src.pbr_metallic_roughness.base_color_factor;
                dst.baseColor = glm::vec4(color[0], color[1], color[2], color[3]);
                const cgltf_texture* texture =
                    src.pbr_metallic_roughness.base_color_texture.texture;
                if (texture != nullptr && texture->image != nullptr) {
                    auto decoded = LoadGltfImage(texture->image);
                    if (decoded.IsOk()) {
                        dst.textureWidth = decoded.Value().width;
                        dst.textureHeight = decoded.Value().height;
                        dst.rgba = std::move(decoded.Value().rgba);
                    } else {
                        model.warnings.emplace_back("基础色纹理无法解码，已使用纯色材质");
                    }
                }
            }
        }
        if (model.materials.empty()) {
            model.materials.push_back(Material{});
        }

        for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex) {
            const cgltf_node* node = &data->nodes[nodeIndex];
            if (node->mesh == nullptr) {
                continue;
            }
            const glm::mat4 world = NodeWorld(node);
            const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(world)));
            for (cgltf_size primIndex = 0; primIndex < node->mesh->primitives_count; ++primIndex) {
                const cgltf_primitive& primitive = node->mesh->primitives[primIndex];
                if (primitive.type != cgltf_primitive_type_triangles) {
                    model.warnings.emplace_back("已忽略非三角面片");
                    continue;
                }
                const cgltf_accessor* positions =
                    Attribute(primitive, cgltf_attribute_type_position);
                if (positions == nullptr || positions->count == 0) {
                    continue;
                }
                const cgltf_accessor* normals = Attribute(primitive, cgltf_attribute_type_normal);
                const cgltf_accessor* uvs = Attribute(primitive, cgltf_attribute_type_texcoord);
                const cgltf_accessor* colors = Attribute(primitive, cgltf_attribute_type_color);

                Primitive dst;
                dst.localTransform = world;
                dst.materialIndex = 0;
                if (primitive.material != nullptr && data->materials_count > 0) {
                    dst.materialIndex =
                        static_cast<std::uint32_t>(primitive.material - data->materials);
                    if (dst.materialIndex >= model.materials.size()) {
                        dst.materialIndex = 0;
                    }
                }

                dst.vertices.resize(positions->count);
                bool missingNormals = normals == nullptr;
                for (cgltf_size v = 0; v < positions->count; ++v) {
                    Vertex vertex;
                    ReadVec3(positions, v, vertex.position);
                    if (normals != nullptr) {
                        ReadVec3(normals, v, vertex.normal);
                        vertex.normal = glm::normalize(normalMatrix * vertex.normal);
                    }
                    if (uvs != nullptr) {
                        ReadVec2(uvs, v, vertex.uv);
                    }
                    if (colors != nullptr) {
                        float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                        cgltf_accessor_read_float(colors, v, color,
                                                  colors->type == cgltf_type_vec4 ? 4 : 3);
                        vertex.abgr = PackAbgr(color, colors->type == cgltf_type_vec4 ? 4 : 3);
                    }
                    dst.vertices[v] = vertex;
                }

                if (primitive.indices != nullptr) {
                    dst.indices.resize(primitive.indices->count);
                    for (cgltf_size i = 0; i < primitive.indices->count; ++i) {
                        dst.indices[i] = static_cast<std::uint32_t>(
                            cgltf_accessor_read_index(primitive.indices, i));
                    }
                } else {
                    dst.indices.resize(dst.vertices.size());
                    for (std::uint32_t i = 0; i < dst.indices.size(); ++i) {
                        dst.indices[i] = i;
                    }
                }

                if (missingNormals) {
                    GenerateNormals(dst.vertices, dst.indices);
                    for (Vertex& vertex : dst.vertices) {
                        vertex.normal = glm::normalize(normalMatrix * vertex.normal);
                    }
                }

                if (!dst.vertices.empty() && !dst.indices.empty()) {
                    model.primitives.push_back(std::move(dst));
                }
            }
        }

        if (model.primitives.empty()) {
            return Core::Result<ModelData>::Fail(
                Core::Error::Make(Core::ErrorCode::ParseFailure, "GLB contained no triangle meshes",
                                  "GLB 中没有可显示的三角网格"));
        }
        return Core::Result<ModelData>::Ok(std::move(model));
    }
};

std::unique_ptr<IModelLoader> CreateGlbLoader() {
    return std::make_unique<GlbLoader>();
}

} // namespace DirectorDesk::Asset
