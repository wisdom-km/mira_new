#include "CreateBgfxRenderer.h"

#include "DirectorDesk/Core/Log.h"
#include "DirectorDesk/Platform/Paths.h"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace DirectorDesk::Backends {
namespace {

constexpr bgfx::ViewId kViewportView = 0;
constexpr bgfx::ViewId kOffscreenView = 1;
constexpr bgfx::ViewId kBlitView = 2;

struct MeshVertex {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
    std::uint32_t abgr;
};

struct UploadedPrimitive {
    bgfx::VertexBufferHandle vertexBuffer = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle indexBuffer = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;
    bool ownsTexture = false;
    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    glm::mat4 localTransform{1.0f};
};

void DestroyHandle(bgfx::ProgramHandle& handle) {
    if (bgfx::isValid(handle)) {
        bgfx::destroy(handle);
        handle = BGFX_INVALID_HANDLE;
    }
}

void DestroyHandle(bgfx::UniformHandle& handle) {
    if (bgfx::isValid(handle)) {
        bgfx::destroy(handle);
        handle = BGFX_INVALID_HANDLE;
    }
}

void DestroyHandle(bgfx::VertexBufferHandle& handle) {
    if (bgfx::isValid(handle)) {
        bgfx::destroy(handle);
        handle = BGFX_INVALID_HANDLE;
    }
}

void DestroyHandle(bgfx::IndexBufferHandle& handle) {
    if (bgfx::isValid(handle)) {
        bgfx::destroy(handle);
        handle = BGFX_INVALID_HANDLE;
    }
}

void DestroyHandle(bgfx::FrameBufferHandle& handle) {
    if (bgfx::isValid(handle)) {
        bgfx::destroy(handle);
        handle = BGFX_INVALID_HANDLE;
    }
}

void DestroyHandle(bgfx::TextureHandle& handle) {
    if (bgfx::isValid(handle)) {
        bgfx::destroy(handle);
        handle = BGFX_INVALID_HANDLE;
    }
}

std::string RendererFolder(bgfx::RendererType::Enum type) {
    switch (type) {
        case bgfx::RendererType::Direct3D11:
        case bgfx::RendererType::Direct3D12:
            return "dx11";
        case bgfx::RendererType::Metal:
            return "metal";
        case bgfx::RendererType::OpenGL:
        case bgfx::RendererType::OpenGLES:
            return "glsl";
        case bgfx::RendererType::Vulkan:
            return "spirv";
        default:
            return {};
    }
}

bgfx::ShaderHandle LoadShader(const std::string& shaderDirectory, const std::string& name) {
    const std::string folder = RendererFolder(bgfx::getRendererType());
    if (folder.empty()) {
        return BGFX_INVALID_HANDLE;
    }
    const std::string path =
        Platform::Paths::Join(Platform::Paths::Join(shaderDirectory, folder), name + ".bin");
    auto bytes = Platform::Paths::ReadBinaryFile(path);
    if (!bytes.IsOk()) {
        DD_LOG_ERROR("Failed to load shader {}: {}", path, bytes.GetError().technicalMessage);
        return BGFX_INVALID_HANDLE;
    }
    const bgfx::Memory* memory =
        bgfx::copy(bytes.Value().data(), static_cast<std::uint32_t>(bytes.Value().size()));
    return bgfx::createShader(memory);
}

struct FramebufferResources {
    bgfx::FrameBufferHandle frameBuffer = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle color = BGFX_INVALID_HANDLE;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

class BgfxRenderer final : public Renderer::IRenderer {
public:
    ~BgfxRenderer() override {
        Shutdown();
    }

    Core::Result<void> Init(const Renderer::RendererInitDesc& desc) override {
        if (m_initialized) {
            return Core::Result<void>::Fail(Core::Error::Make(Core::ErrorCode::AlreadyInitialized,
                                                              "BgfxRenderer already initialized",
                                                              "渲染器已经初始化"));
        }
        if (desc.nativeWindowHandle == nullptr) {
            return Core::Result<void>::Fail(Core::Error::Make(
                Core::ErrorCode::InvalidArgument, "nativeWindowHandle is null", "窗口句柄无效"));
        }

        bgfx::Init init;
        init.type = bgfx::RendererType::Count;
        init.platformData.nwh = desc.nativeWindowHandle;
        init.resolution.width = desc.width == 0 ? 1 : desc.width;
        init.resolution.height = desc.height == 0 ? 1 : desc.height;
        init.resolution.reset = BGFX_RESET_VSYNC;
        if (!bgfx::init(init)) {
            return Core::Result<void>::Fail(Core::Error::Make(
                Core::ErrorCode::Internal, "bgfx::init failed", "无法初始化渲染器"));
        }

        m_shaderDirectory = desc.shaderDirectory;
        m_backbufferWidth = init.resolution.width;
        m_backbufferHeight = init.resolution.height;

        auto loadResult = CreateGpuResources();
        if (!loadResult.IsOk()) {
            bgfx::shutdown();
            return loadResult;
        }

        m_initialized = true;
        DD_LOG_INFO("bgfx initialized renderer={} shaderDir={}",
                    bgfx::getRendererName(bgfx::getRendererType()), m_shaderDirectory);
        return Core::Result<void>::Ok();
    }

    void Shutdown() override {
        if (!m_initialized) {
            return;
        }
        DestroyGpuResources();
        bgfx::shutdown();
        m_initialized = false;
        DD_LOG_INFO("bgfx shutdown");
    }

    void BeginFrame(std::uint32_t framebufferWidth, std::uint32_t framebufferHeight) override {
        if (!m_initialized) {
            return;
        }
        const std::uint32_t width = framebufferWidth == 0 ? 1 : framebufferWidth;
        const std::uint32_t height = framebufferHeight == 0 ? 1 : framebufferHeight;
        if (width != m_backbufferWidth || height != m_backbufferHeight) {
            bgfx::reset(width, height, BGFX_RESET_VSYNC);
            m_backbufferWidth = width;
            m_backbufferHeight = height;
        }
        bgfx::setViewRect(255, 0, 0, static_cast<std::uint16_t>(width),
                          static_cast<std::uint16_t>(height));
        bgfx::setViewClear(255, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x17181cff, 1.0f, 0);
        bgfx::touch(255);
    }

    void RenderScene(const Renderer::RenderSceneView& scene, const Renderer::CameraView& view,
                     const Renderer::RenderTargetDesc& target) override {
        if (!m_initialized) {
            return;
        }

        const bool offscreen = target.kind == Renderer::RenderTargetKind::Offscreen;
        const bgfx::ViewId viewId = offscreen ? kOffscreenView : kViewportView;
        FramebufferResources& framebuffer = offscreen ? m_offscreen : m_viewport;
        const std::uint32_t width = offscreen ? (target.width == 0 ? 1280 : target.width)
                                              : (m_viewport.width == 0 ? 1 : m_viewport.width);
        const std::uint32_t height = offscreen ? (target.height == 0 ? 720 : target.height)
                                               : (m_viewport.height == 0 ? 1 : m_viewport.height);

        if (!EnsureFramebuffer(framebuffer, width, height)) {
            DD_LOG_ERROR("Failed to create render target {}x{}", width, height);
            return;
        }

        const std::uint32_t clearColor = target.transparentBackground ? 0x00000000 : 0x3a4a62ff;
        bgfx::setViewFrameBuffer(viewId, framebuffer.frameBuffer);
        bgfx::setViewRect(viewId, 0, 0, static_cast<std::uint16_t>(width),
                          static_cast<std::uint16_t>(height));
        bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clearColor, 1.0f, 0);
        bgfx::setViewTransform(viewId, glm::value_ptr(view.view), glm::value_ptr(view.projection));
        bgfx::touch(viewId);

        const float lightDir[4] = {scene.light.direction.x, scene.light.direction.y,
                                   scene.light.direction.z, 0.0f};
        const float lightColor[4] = {scene.light.color.x, scene.light.color.y, scene.light.color.z,
                                     1.0f};
        bgfx::setUniform(m_lightDir, lightDir);
        bgfx::setUniform(m_lightColor, lightColor);

        if (scene.showGroundGrid && !offscreen) {
            const float identity[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
            const float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            bgfx::setTransform(identity);
            bgfx::setUniform(m_baseColor, white);
            bgfx::setTexture(0, m_sampler, m_whiteTexture);
            bgfx::setVertexBuffer(0, m_gridVertexBuffer);
            bgfx::setIndexBuffer(m_gridIndexBuffer);
            bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
                           BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_PT_LINES | BGFX_STATE_MSAA);
            bgfx::submit(viewId, m_program);
        }

        if (scene.showTestMesh) {
            const float identity[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
            const float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            bgfx::setTransform(identity);
            bgfx::setUniform(m_baseColor, white);
            bgfx::setTexture(0, m_sampler, m_whiteTexture);
            bgfx::setVertexBuffer(0, m_vertexBuffer);
            bgfx::setIndexBuffer(m_indexBuffer);
            bgfx::setState(BGFX_STATE_DEFAULT);
            bgfx::submit(viewId, m_program);
        }

        for (const Renderer::RenderMeshInstance& instance : scene.instances) {
            if (!instance.visible) {
                continue;
            }
            const auto found = m_models.find(instance.modelId);
            if (found == m_models.end()) {
                continue;
            }
            for (const UploadedPrimitive& primitive : found->second) {
                const glm::mat4 world = instance.world * primitive.localTransform;
                bgfx::setTransform(glm::value_ptr(world));
                bgfx::setUniform(m_baseColor, glm::value_ptr(primitive.baseColor));
                bgfx::setTexture(0, m_sampler, primitive.texture);
                bgfx::setVertexBuffer(0, primitive.vertexBuffer);
                bgfx::setIndexBuffer(primitive.indexBuffer);
                bgfx::setState(BGFX_STATE_DEFAULT);
                bgfx::submit(viewId, m_program);
            }
        }
    }

    Core::Result<Renderer::PixelBuffer>
    ReadbackTarget(const Renderer::RenderTargetDesc& target) override {
        if (!m_initialized) {
            return Core::Result<Renderer::PixelBuffer>::Fail(
                Core::Error::Make(Core::ErrorCode::NotInitialized, "Renderer is not initialized",
                                  "渲染器尚未初始化"));
        }

        const bool offscreen = target.kind == Renderer::RenderTargetKind::Offscreen;
        FramebufferResources& framebuffer = offscreen ? m_offscreen : m_viewport;
        if (!bgfx::isValid(framebuffer.frameBuffer) || !bgfx::isValid(framebuffer.color)) {
            return Core::Result<Renderer::PixelBuffer>::Fail(Core::Error::Make(
                Core::ErrorCode::Internal, "Render target is not ready", "渲染目标尚未准备好"));
        }

        const bgfx::Caps* caps = bgfx::getCaps();
        if (caps == nullptr || (caps->supported & BGFX_CAPS_TEXTURE_BLIT) == 0 ||
            (caps->supported & BGFX_CAPS_TEXTURE_READ_BACK) == 0) {
            return Core::Result<Renderer::PixelBuffer>::Fail(Core::Error::Make(
                Core::ErrorCode::Internal, "GPU does not support texture blit/readback",
                "当前显卡不支持离屏回读"));
        }

        if (!EnsureReadbackTexture(framebuffer.width, framebuffer.height)) {
            return Core::Result<Renderer::PixelBuffer>::Fail(
                Core::Error::Make(Core::ErrorCode::Internal, "Failed to create readback texture",
                                  "无法创建回读纹理"));
        }

        Renderer::PixelBuffer buffer;
        buffer.width = framebuffer.width;
        buffer.height = framebuffer.height;
        buffer.rgba.resize(static_cast<std::size_t>(buffer.width) * buffer.height * 4u);

        bgfx::blit(kBlitView, m_readbackTexture, 0, 0, framebuffer.color, 0, 0);
        const std::uint32_t readyFrame = bgfx::readTexture(m_readbackTexture, buffer.rgba.data());
        while (bgfx::frame() < readyFrame) {
        }

        return Core::Result<Renderer::PixelBuffer>::Ok(std::move(buffer));
    }

    Core::Result<std::uint32_t> CreateModel(const Renderer::GpuModelDesc& desc) override {
        if (!m_initialized) {
            return Core::Result<std::uint32_t>::Fail(
                Core::Error::Make(Core::ErrorCode::NotInitialized, "Renderer is not initialized",
                                  "渲染器尚未初始化"));
        }
        if (desc.primitives.empty()) {
            return Core::Result<std::uint32_t>::Fail(
                Core::Error::Make(Core::ErrorCode::InvalidArgument,
                                  "GpuModelDesc has no primitives", "模型没有网格"));
        }

        std::vector<UploadedPrimitive> uploaded;
        uploaded.reserve(desc.primitives.size());
        for (const Renderer::GpuPrimitive& primitive : desc.primitives) {
            if (primitive.vertices.empty() || primitive.indices.empty()) {
                continue;
            }
            UploadedPrimitive gpu;
            gpu.baseColor = primitive.baseColor;
            gpu.localTransform = primitive.localTransform;
            gpu.vertexBuffer = bgfx::createVertexBuffer(
                bgfx::copy(primitive.vertices.data(),
                           static_cast<std::uint32_t>(primitive.vertices.size() *
                                                      sizeof(Renderer::GpuVertex))),
                m_layout);
            gpu.indexBuffer = bgfx::createIndexBuffer(
                bgfx::copy(
                    primitive.indices.data(),
                    static_cast<std::uint32_t>(primitive.indices.size() * sizeof(std::uint32_t))),
                BGFX_BUFFER_INDEX32);
            if (primitive.textureWidth > 0 && primitive.textureHeight > 0 &&
                primitive.rgba.size() == static_cast<std::size_t>(primitive.textureWidth) *
                                             primitive.textureHeight * 4u) {
                gpu.texture = bgfx::createTexture2D(
                    static_cast<std::uint16_t>(primitive.textureWidth),
                    static_cast<std::uint16_t>(primitive.textureHeight), false, 1,
                    bgfx::TextureFormat::RGBA8, 0,
                    bgfx::copy(primitive.rgba.data(),
                               static_cast<std::uint32_t>(primitive.rgba.size())));
                gpu.ownsTexture = true;
            } else {
                gpu.texture = m_whiteTexture;
            }
            uploaded.push_back(gpu);
        }
        if (uploaded.empty()) {
            return Core::Result<std::uint32_t>::Fail(
                Core::Error::Make(Core::ErrorCode::InvalidArgument,
                                  "GpuModelDesc primitives were empty", "模型没有有效网格"));
        }

        const std::uint32_t id = m_nextModelId++;
        m_models.emplace(id, std::move(uploaded));
        return Core::Result<std::uint32_t>::Ok(id);
    }

    void DestroyModel(std::uint32_t modelId) override {
        const auto found = m_models.find(modelId);
        if (found == m_models.end()) {
            return;
        }
        for (UploadedPrimitive& primitive : found->second) {
            DestroyHandle(primitive.vertexBuffer);
            DestroyHandle(primitive.indexBuffer);
            if (primitive.ownsTexture) {
                DestroyHandle(primitive.texture);
            }
        }
        m_models.erase(found);
    }

    void EndFrame() override {
        if (m_initialized) {
            bgfx::frame();
        }
    }

    void SetViewportSize(std::uint32_t width, std::uint32_t height) override {
        const std::uint32_t w = width == 0 ? 1 : width;
        const std::uint32_t h = height == 0 ? 1 : height;
        if (!EnsureFramebuffer(m_viewport, w, h)) {
            DD_LOG_ERROR("Failed to resize viewport {}x{}", w, h);
        }
    }

    std::uint16_t ViewportTextureIndex() const override {
        if (!bgfx::isValid(m_viewport.frameBuffer)) {
            return 0xFFFFu;
        }
        const bgfx::TextureHandle color = bgfx::getTexture(m_viewport.frameBuffer, 0);
        return bgfx::isValid(color) ? color.idx : 0xFFFFu;
    }

    std::uint32_t ViewportWidth() const override {
        return m_viewport.width;
    }
    std::uint32_t ViewportHeight() const override {
        return m_viewport.height;
    }

private:
    Core::Result<void> CreateGpuResources() {
        m_layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();

        CreateCube();
        CreateGroundGrid();

        const bgfx::ShaderHandle vs = LoadShader(m_shaderDirectory, "vs_mesh");
        const bgfx::ShaderHandle fs = LoadShader(m_shaderDirectory, "fs_mesh");
        if (!bgfx::isValid(vs) || !bgfx::isValid(fs)) {
            if (bgfx::isValid(vs)) {
                bgfx::destroy(vs);
            }
            if (bgfx::isValid(fs)) {
                bgfx::destroy(fs);
            }
            DestroyGpuResources();
            return Core::Result<void>::Fail(Core::Error::Make(
                Core::ErrorCode::NotFound, "Mesh shaders were not found", "找不到网格着色器"));
        }
        m_program = bgfx::createProgram(vs, fs, true);
        m_lightDir = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4);
        m_lightColor = bgfx::createUniform("u_lightColor", bgfx::UniformType::Vec4);
        m_baseColor = bgfx::createUniform("u_baseColor", bgfx::UniformType::Vec4);
        m_sampler = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);
        const std::uint32_t white = 0xffffffffu;
        m_whiteTexture = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, 0,
                                               bgfx::copy(&white, sizeof(white)));
        SetViewportSize(1280, 720);
        return Core::Result<void>::Ok();
    }

    void CreateCube() {
        constexpr std::uint32_t kRed = 0xff5a4cff;
        constexpr std::uint32_t kGreen = 0xff4caf7a;
        constexpr std::uint32_t kBlue = 0xfff2c14e;
        constexpr std::uint32_t kWhite = 0xffe8e4dc;

        const MeshVertex vertices[] = {
            {-1, -1, 1, 0, 0, 1, 0, 0, kRed},    {1, -1, 1, 0, 0, 1, 1, 0, kRed},
            {1, 1, 1, 0, 0, 1, 1, 1, kRed},      {-1, 1, 1, 0, 0, 1, 0, 1, kRed},
            {1, -1, -1, 0, 0, -1, 0, 0, kGreen}, {-1, -1, -1, 0, 0, -1, 1, 0, kGreen},
            {-1, 1, -1, 0, 0, -1, 1, 1, kGreen}, {1, 1, -1, 0, 0, -1, 0, 1, kGreen},
            {-1, -1, -1, -1, 0, 0, 0, 0, kBlue}, {-1, -1, 1, -1, 0, 0, 1, 0, kBlue},
            {-1, 1, 1, -1, 0, 0, 1, 1, kBlue},   {-1, 1, -1, -1, 0, 0, 0, 1, kBlue},
            {1, -1, 1, 1, 0, 0, 0, 0, kWhite},   {1, -1, -1, 1, 0, 0, 1, 0, kWhite},
            {1, 1, -1, 1, 0, 0, 1, 1, kWhite},   {1, 1, 1, 1, 0, 0, 0, 1, kWhite},
            {-1, 1, 1, 0, 1, 0, 0, 0, kRed},     {1, 1, 1, 0, 1, 0, 1, 0, kGreen},
            {1, 1, -1, 0, 1, 0, 1, 1, kBlue},    {-1, 1, -1, 0, 1, 0, 0, 1, kWhite},
            {-1, -1, -1, 0, -1, 0, 0, 0, kRed},  {1, -1, -1, 0, -1, 0, 1, 0, kGreen},
            {1, -1, 1, 0, -1, 0, 1, 1, kBlue},   {-1, -1, 1, 0, -1, 0, 0, 1, kWhite},
        };
        const std::uint16_t indices[] = {0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,
                                         8,  9,  10, 8,  10, 11, 12, 13, 14, 12, 14, 15,
                                         16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23};

        m_vertexBuffer = bgfx::createVertexBuffer(bgfx::copy(vertices, sizeof(vertices)), m_layout);
        m_indexBuffer = bgfx::createIndexBuffer(bgfx::copy(indices, sizeof(indices)));
    }

    void CreateGroundGrid() {
        constexpr float kExtent = 10.0f;
        constexpr float kStep = 1.0f;
        constexpr std::uint32_t kGray = 0xff6a7380;
        constexpr std::uint32_t kAxisX = 0xff3d5cff;
        constexpr std::uint32_t kAxisZ = 0xffffb14a;

        std::vector<MeshVertex> vertices;
        std::vector<std::uint16_t> indices;
        auto addLine = [&](float x0, float y0, float z0, float x1, float y1, float z1,
                           std::uint32_t color) {
            const std::uint16_t start = static_cast<std::uint16_t>(vertices.size());
            vertices.push_back({x0, y0, z0, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, color});
            vertices.push_back({x1, y1, z1, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, color});
            indices.push_back(start);
            indices.push_back(static_cast<std::uint16_t>(start + 1));
        };

        for (float x = -kExtent; x <= kExtent + 0.01f; x += kStep) {
            addLine(x, 0.0f, -kExtent, x, 0.0f, kExtent, kGray);
        }
        for (float z = -kExtent; z <= kExtent + 0.01f; z += kStep) {
            addLine(-kExtent, 0.0f, z, kExtent, 0.0f, z, kGray);
        }
        addLine(-kExtent, 0.002f, 0.0f, kExtent, 0.002f, 0.0f, kAxisX);
        addLine(0.0f, 0.002f, -kExtent, 0.0f, 0.002f, kExtent, kAxisZ);

        m_gridVertexBuffer =
            bgfx::createVertexBuffer(bgfx::copy(vertices.data(), static_cast<std::uint32_t>(
                                                                     vertices.size() * sizeof(MeshVertex))),
                                     m_layout);
        m_gridIndexBuffer = bgfx::createIndexBuffer(
            bgfx::copy(indices.data(), static_cast<std::uint32_t>(indices.size() * sizeof(std::uint16_t))));
    }

    bool EnsureFramebuffer(FramebufferResources& framebuffer, std::uint32_t width,
                           std::uint32_t height) {
        if (bgfx::isValid(framebuffer.frameBuffer) && framebuffer.width == width &&
            framebuffer.height == height) {
            return true;
        }
        DestroyHandle(framebuffer.frameBuffer);
        framebuffer.color = BGFX_INVALID_HANDLE;

        const std::uint64_t colorFlags =
            BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
        const std::uint64_t depthFlags =
            BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
        const bgfx::TextureHandle color = bgfx::createTexture2D(
            static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height), false, 1,
            bgfx::TextureFormat::RGBA8, colorFlags);
        bgfx::TextureHandle depth = bgfx::createTexture2D(
            static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height), false, 1,
            bgfx::TextureFormat::D24S8, depthFlags);
        if (!bgfx::isValid(color) || !bgfx::isValid(depth)) {
            if (bgfx::isValid(color)) {
                bgfx::destroy(color);
            }
            if (bgfx::isValid(depth)) {
                bgfx::destroy(depth);
            }
            return false;
        }

        bgfx::Attachment attachments[2];
        attachments[0].init(color);
        attachments[1].init(depth);
        framebuffer.frameBuffer = bgfx::createFrameBuffer(2, attachments, true);
        if (!bgfx::isValid(framebuffer.frameBuffer)) {
            bgfx::destroy(color);
            bgfx::destroy(depth);
            return false;
        }
        framebuffer.color = color;
        framebuffer.width = width;
        framebuffer.height = height;
        return true;
    }

    bool EnsureReadbackTexture(std::uint32_t width, std::uint32_t height) {
        if (bgfx::isValid(m_readbackTexture) && m_readbackWidth == width &&
            m_readbackHeight == height) {
            return true;
        }
        DestroyHandle(m_readbackTexture);
        m_readbackTexture = bgfx::createTexture2D(
            static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height), false, 1,
            bgfx::TextureFormat::RGBA8,
            BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK | BGFX_SAMPLER_NONE);
        m_readbackWidth = width;
        m_readbackHeight = height;
        return bgfx::isValid(m_readbackTexture);
    }

    void DestroyGpuResources() {
        std::vector<std::uint32_t> ids;
        ids.reserve(m_models.size());
        for (const auto& entry : m_models) {
            ids.push_back(entry.first);
        }
        for (std::uint32_t id : ids) {
            DestroyModel(id);
        }
        DestroyHandle(m_program);
        DestroyHandle(m_lightDir);
        DestroyHandle(m_lightColor);
        DestroyHandle(m_baseColor);
        DestroyHandle(m_sampler);
        DestroyHandle(m_whiteTexture);
        DestroyHandle(m_vertexBuffer);
        DestroyHandle(m_indexBuffer);
        DestroyHandle(m_gridVertexBuffer);
        DestroyHandle(m_gridIndexBuffer);
        DestroyHandle(m_viewport.frameBuffer);
        m_viewport.color = BGFX_INVALID_HANDLE;
        DestroyHandle(m_offscreen.frameBuffer);
        m_offscreen.color = BGFX_INVALID_HANDLE;
        DestroyHandle(m_readbackTexture);
        m_viewport = {};
        m_offscreen = {};
        m_readbackWidth = 0;
        m_readbackHeight = 0;
        m_nextModelId = 1;
    }

    bool m_initialized = false;
    std::string m_shaderDirectory;
    std::uint32_t m_backbufferWidth = 0;
    std::uint32_t m_backbufferHeight = 0;
    bgfx::VertexLayout m_layout{};
    bgfx::VertexBufferHandle m_vertexBuffer = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle m_indexBuffer = BGFX_INVALID_HANDLE;
    bgfx::VertexBufferHandle m_gridVertexBuffer = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle m_gridIndexBuffer = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lightDir = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lightColor = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_baseColor = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sampler = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_whiteTexture = BGFX_INVALID_HANDLE;
    FramebufferResources m_viewport;
    FramebufferResources m_offscreen;
    bgfx::TextureHandle m_readbackTexture = BGFX_INVALID_HANDLE;
    std::uint32_t m_readbackWidth = 0;
    std::uint32_t m_readbackHeight = 0;
    std::uint32_t m_nextModelId = 1;
    std::unordered_map<std::uint32_t, std::vector<UploadedPrimitive>> m_models;
};

} // namespace

std::unique_ptr<Renderer::IRenderer> CreateBgfxRenderer() {
    return std::make_unique<BgfxRenderer>();
}

} // namespace DirectorDesk::Backends
