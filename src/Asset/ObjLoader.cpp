#define TINYOBJLOADER_IMPLEMENTATION
#include "DirectorDesk/Asset/IModelLoader.h"

#include "MeshUtils.h"
#include "TextureDecode.h"

#include "DirectorDesk/Platform/Paths.h"

#include <tiny_obj_loader.h>

#include <memory>

namespace DirectorDesk::Asset {
namespace {

std::uint32_t ColorToAbgr(float r, float g, float b, float a) {
    auto toByte = [](float value) {
        const float clamped = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
        return static_cast<std::uint32_t>(clamped * 255.0f + 0.5f);
    };
    return toByte(a) << 24 | toByte(b) << 16 | toByte(g) << 8 | toByte(r);
}

} // namespace

class ObjLoader final : public IModelLoader {
public:
    [[nodiscard]] bool CanLoad(const std::string& extensionUtf8) const override {
        return extensionUtf8 == ".obj";
    }

    Core::Result<ModelData> Load(const std::string& utf8Path) const override {
        auto objText = Platform::Paths::ReadTextFile(utf8Path);
        if (!objText.IsOk()) {
            return Core::Result<ModelData>::Fail(objText.GetError());
        }

        std::string loadedMtl;
        const std::string parent = Platform::Paths::Parent(utf8Path);
        const std::string siblingMtl =
            Platform::Paths::Join(parent, Platform::Paths::Stem(utf8Path) + ".mtl");
        if (Platform::Paths::Exists(siblingMtl) && !Platform::Paths::IsDirectory(siblingMtl)) {
            auto mtlFile = Platform::Paths::ReadTextFile(siblingMtl);
            if (mtlFile.IsOk()) {
                loadedMtl = mtlFile.Value();
            }
        }

        tinyobj::ObjReaderConfig config;
        config.triangulate = true;
        config.vertex_color = true;

        tinyobj::ObjReader reader;
        if (!reader.ParseFromString(objText.Value(), loadedMtl, config)) {
            return Core::Result<ModelData>::Fail(Core::Error::Make(
                Core::ErrorCode::ParseFailure,
                reader.Error().empty() ? "OBJ parse failed" : reader.Error(), "无法解析 OBJ 文件"));
        }

        const auto& attrib = reader.GetAttrib();
        const auto& shapes = reader.GetShapes();
        const auto& materials = reader.GetMaterials();
        if (shapes.empty() || attrib.vertices.empty()) {
            return Core::Result<ModelData>::Fail(Core::Error::Make(Core::ErrorCode::ParseFailure,
                                                                   "OBJ contained no vertices",
                                                                   "OBJ 中没有可显示的网格"));
        }

        ModelData model;
        model.sourcePath = utf8Path;
        model.name = Platform::Paths::Stem(utf8Path);
        if (!reader.Warning().empty()) {
            model.warnings.push_back(reader.Warning());
        }

        model.materials.resize(materials.size());
        for (std::size_t i = 0; i < materials.size(); ++i) {
            const tinyobj::material_t& src = materials[i];
            Material& dst = model.materials[i];
            dst.baseColor = glm::vec4(src.diffuse[0], src.diffuse[1], src.diffuse[2], 1.0f);
            if (!src.diffuse_texname.empty()) {
                const std::string texturePath = Platform::Paths::Join(parent, src.diffuse_texname);
                auto decoded = DecodeImageFile(texturePath);
                if (decoded.IsOk()) {
                    dst.textureWidth = decoded.Value().width;
                    dst.textureHeight = decoded.Value().height;
                    dst.rgba = std::move(decoded.Value().rgba);
                } else {
                    model.warnings.emplace_back("OBJ 纹理无法加载，已使用纯色材质");
                }
            }
        }
        if (model.materials.empty()) {
            model.materials.push_back(Material{});
        }

        for (const tinyobj::shape_t& shape : shapes) {
            Primitive primitive;
            const auto& mesh = shape.mesh;
            const std::size_t indexCount = mesh.indices.size();
            primitive.vertices.reserve(indexCount);
            primitive.indices.reserve(indexCount);
            bool missingNormals = attrib.normals.empty();

            for (std::size_t i = 0; i < indexCount; ++i) {
                const tinyobj::index_t idx = mesh.indices[i];
                Vertex vertex;
                if (idx.vertex_index >= 0) {
                    const tinyobj::real_t* p =
                        &attrib.vertices[static_cast<std::size_t>(idx.vertex_index) * 3];
                    vertex.position = glm::vec3(p[0], p[1], p[2]);
                }
                if (idx.normal_index >= 0 && !attrib.normals.empty()) {
                    const tinyobj::real_t* n =
                        &attrib.normals[static_cast<std::size_t>(idx.normal_index) * 3];
                    vertex.normal = glm::vec3(n[0], n[1], n[2]);
                } else {
                    missingNormals = true;
                }
                if (idx.texcoord_index >= 0 && !attrib.texcoords.empty()) {
                    const tinyobj::real_t* t =
                        &attrib.texcoords[static_cast<std::size_t>(idx.texcoord_index) * 2];
                    vertex.uv = glm::vec2(t[0], t[1]);
                }
                if (!attrib.colors.empty() && idx.vertex_index >= 0) {
                    const tinyobj::real_t* c =
                        &attrib.colors[static_cast<std::size_t>(idx.vertex_index) * 3];
                    vertex.abgr = ColorToAbgr(c[0], c[1], c[2], 1.0f);
                }
                primitive.indices.push_back(static_cast<std::uint32_t>(primitive.vertices.size()));
                primitive.vertices.push_back(vertex);
            }

            if (!mesh.material_ids.empty() && mesh.material_ids[0] >= 0) {
                primitive.materialIndex = static_cast<std::uint32_t>(mesh.material_ids[0]);
                if (primitive.materialIndex >= model.materials.size()) {
                    primitive.materialIndex = 0;
                }
            }

            if (missingNormals) {
                GenerateNormals(primitive.vertices, primitive.indices);
            }
            if (!primitive.vertices.empty()) {
                model.primitives.push_back(std::move(primitive));
            }
        }

        if (model.primitives.empty()) {
            return Core::Result<ModelData>::Fail(
                Core::Error::Make(Core::ErrorCode::ParseFailure, "OBJ contained no triangle meshes",
                                  "OBJ 中没有可显示的三角网格"));
        }
        return Core::Result<ModelData>::Ok(std::move(model));
    }
};

std::unique_ptr<IModelLoader> CreateObjLoader() {
    return std::make_unique<ObjLoader>();
}

} // namespace DirectorDesk::Asset
