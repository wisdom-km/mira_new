// ImGuiGlfwBackend: Implementation for the DirectorDesk imgui module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "ImGuiGlfwBackend.h"

#include "DirectorDesk/Core/Log.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Platform/Window.h"

#include <GLFW/glfw3.h>
#include <bgfx/bgfx.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>

#include <cstdint>
#include <cstring>
#include <vector>

namespace DirectorDesk::Backends {
namespace {

ImVec4 Color(unsigned int hex, float alpha = 1.0f) {
    return ImVec4(static_cast<float>((hex >> 16u) & 0xffu) / 255.0f,
                  static_cast<float>((hex >> 8u) & 0xffu) / 255.0f,
                  static_cast<float>(hex & 0xffu) / 255.0f, alpha);
}

void ApplyDirectorDeskStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Compact, editor-like geometry: dense controls, restrained rounding, clear grouping.
    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.CellPadding = ImVec2(7.0f, 4.0f);
    style.ItemSpacing = ImVec2(7.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.IndentSpacing = 18.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;

    style.WindowRounding = 4.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;

    constexpr unsigned int kCanvas = 0x0b0d12;
    constexpr unsigned int kWindow = 0x101319;
    constexpr unsigned int kSurface = 0x151922;
    constexpr unsigned int kRaised = 0x1c222d;
    constexpr unsigned int kBorder = 0x2b3340;
    constexpr unsigned int kBorderHot = 0x465265;
    constexpr unsigned int kText = 0xe9edf4;
    constexpr unsigned int kMuted = 0x8f9baa;
    constexpr unsigned int kAccent = 0xd89a4a;
    constexpr unsigned int kAccentHot = 0xebb25f;
    constexpr unsigned int kSelection = 0x31547d;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = Color(kText);
    colors[ImGuiCol_TextDisabled] = Color(kMuted);
    colors[ImGuiCol_WindowBg] = Color(kWindow);
    colors[ImGuiCol_ChildBg] = Color(kSurface, 0.78f);
    colors[ImGuiCol_PopupBg] = Color(kWindow, 1.0f);
    colors[ImGuiCol_Border] = Color(kBorder);
    colors[ImGuiCol_BorderShadow] = Color(kCanvas, 0.0f);
    colors[ImGuiCol_FrameBg] = Color(kRaised);
    colors[ImGuiCol_FrameBgHovered] = Color(0x27313f);
    colors[ImGuiCol_FrameBgActive] = Color(0x303b4b);
    colors[ImGuiCol_TitleBg] = Color(kCanvas);
    colors[ImGuiCol_TitleBgActive] = Color(kCanvas);
    colors[ImGuiCol_TitleBgCollapsed] = Color(kCanvas);
    colors[ImGuiCol_MenuBarBg] = Color(kCanvas);
    colors[ImGuiCol_ScrollbarBg] = Color(kCanvas, 0.62f);
    colors[ImGuiCol_ScrollbarGrab] = Color(kBorder);
    colors[ImGuiCol_ScrollbarGrabHovered] = Color(kBorderHot);
    colors[ImGuiCol_ScrollbarGrabActive] = Color(kAccent);
    colors[ImGuiCol_CheckMark] = Color(kAccentHot);
    colors[ImGuiCol_CheckboxSelectedBg] = Color(kSelection);
    colors[ImGuiCol_SliderGrab] = Color(kAccent);
    colors[ImGuiCol_SliderGrabActive] = Color(kAccentHot);
    colors[ImGuiCol_Button] = Color(kBorder);
    colors[ImGuiCol_ButtonHovered] = Color(kBorderHot);
    colors[ImGuiCol_ButtonActive] = Color(kSelection);
    colors[ImGuiCol_Header] = Color(kSelection, 0.72f);
    colors[ImGuiCol_HeaderHovered] = Color(0x3b6595, 0.82f);
    colors[ImGuiCol_HeaderActive] = Color(0x4776aa);
    colors[ImGuiCol_Separator] = Color(kBorder);
    colors[ImGuiCol_SeparatorHovered] = Color(kAccent, 0.78f);
    colors[ImGuiCol_SeparatorActive] = Color(kAccentHot);
    colors[ImGuiCol_ResizeGrip] = Color(kBorder, 0.35f);
    colors[ImGuiCol_ResizeGripHovered] = Color(kAccent, 0.72f);
    colors[ImGuiCol_ResizeGripActive] = Color(kAccentHot);
    colors[ImGuiCol_InputTextCursor] = Color(kText);
    colors[ImGuiCol_Tab] = Color(kSurface);
    colors[ImGuiCol_TabHovered] = Color(0x283342);
    colors[ImGuiCol_TabSelected] = Color(kRaised);
    colors[ImGuiCol_TabSelectedOverline] = Color(kAccent);
    colors[ImGuiCol_TabDimmed] = Color(kCanvas);
    colors[ImGuiCol_TabDimmedSelected] = Color(kSurface);
    colors[ImGuiCol_DockingPreview] = Color(kAccent, 0.62f);
    colors[ImGuiCol_DockingEmptyBg] = Color(kCanvas);
    colors[ImGuiCol_PlotLines] = Color(kMuted);
    colors[ImGuiCol_PlotHistogram] = Color(kAccent);
    colors[ImGuiCol_TableHeaderBg] = Color(kRaised);
    colors[ImGuiCol_TableBorderStrong] = Color(kBorderHot);
    colors[ImGuiCol_TableBorderLight] = Color(kBorder);
    colors[ImGuiCol_TableRowBg] = Color(kWindow, 0.0f);
    colors[ImGuiCol_TableRowBgAlt] = Color(kRaised, 0.34f);
    colors[ImGuiCol_TextLink] = Color(kAccentHot);
    colors[ImGuiCol_TextSelectedBg] = Color(kSelection);
    colors[ImGuiCol_TreeLines] = Color(kBorder);
    colors[ImGuiCol_DragDropTarget] = Color(kAccentHot);
    colors[ImGuiCol_DragDropTargetBg] = Color(kAccent, 0.22f);
    colors[ImGuiCol_UnsavedMarker] = Color(kAccent);
    colors[ImGuiCol_NavCursor] = Color(kAccentHot);
    colors[ImGuiCol_NavWindowingHighlight] = Color(kAccentHot, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = Color(kCanvas, 0.65f);
    colors[ImGuiCol_ModalWindowDimBg] = Color(kCanvas, 0.62f);
}

struct ImGuiVertex {
    float x, y;
    float u, v;
    std::uint32_t abgr;

    static bgfx::VertexLayout Layout() {
        bgfx::VertexLayout layout;
        layout.begin()
            .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
        return layout;
    }
};

void LoadUiFont() {
    auto fontPath = Platform::Paths::UiFontFile();
    if (!fontPath.IsOk()) {
        DD_LOG_WARN("{}", fontPath.GetError().technicalMessage);
        return;
    }
    auto bytes = Platform::Paths::ReadBinaryFile(fontPath.Value());
    if (!bytes.IsOk()) {
        DD_LOG_WARN("Failed to read UI font {}", fontPath.Value());
        return;
    }

    void* copy = IM_ALLOC(bytes.Value().size());
    if (copy == nullptr) {
        DD_LOG_WARN("Failed to allocate UI font {}", fontPath.Value());
        return;
    }
    std::memcpy(copy, bytes.Value().data(), bytes.Value().size());

    ImFontConfig config;
    config.OversampleH = 1;
    config.OversampleV = 1;
    config.PixelSnapH = true;
    config.FontDataOwnedByAtlas = true;
    ImFont* font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        copy, static_cast<int>(bytes.Value().size()), 18.0f, &config);
    if (font == nullptr) {
        DD_LOG_WARN("Failed to load UI font {}", fontPath.Value());
        return;
    }
    DD_LOG_INFO("Loaded UI font {}", fontPath.Value());
}

void UpdateImGuiTexture(ImTextureData* texture) {
    if (texture->Status == ImTextureStatus_WantCreate) {
        if (texture->Format != ImTextureFormat_RGBA32) {
            DD_LOG_ERROR("ImGui texture format is not RGBA32");
            return;
        }
        const bgfx::TextureHandle handle = bgfx::createTexture2D(
            static_cast<std::uint16_t>(texture->Width), static_cast<std::uint16_t>(texture->Height),
            false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE | BGFX_SAMPLER_U_CLAMP |
                                                      BGFX_SAMPLER_V_CLAMP,
            nullptr);
        bgfx::updateTexture2D(handle, 0, 0, 0, 0, static_cast<std::uint16_t>(texture->Width),
                              static_cast<std::uint16_t>(texture->Height),
                              bgfx::copy(texture->GetPixels(),
                                         static_cast<std::uint32_t>(texture->GetSizeInBytes())));
        texture->SetTexID(static_cast<ImTextureID>(handle.idx));
        texture->SetStatus(ImTextureStatus_OK);
        return;
    }

    if (texture->Status == ImTextureStatus_WantUpdates) {
        const bgfx::TextureHandle handle = {static_cast<std::uint16_t>(texture->GetTexID())};
        if (!bgfx::isValid(handle)) {
            return;
        }
        for (const ImTextureRect& rect : texture->Updates) {
            const int bytesPerPixel = texture->BytesPerPixel;
            std::vector<unsigned char> packed(
                static_cast<std::size_t>(rect.w) * static_cast<std::size_t>(rect.h) *
                static_cast<std::size_t>(bytesPerPixel));
            for (int row = 0; row < rect.h; ++row) {
                std::memcpy(packed.data() + static_cast<std::size_t>(row) * rect.w * bytesPerPixel,
                            texture->GetPixelsAt(rect.x, rect.y + row),
                            static_cast<std::size_t>(rect.w) * bytesPerPixel);
            }
            bgfx::updateTexture2D(
                handle, 0, 0, rect.x, rect.y, rect.w, rect.h,
                bgfx::copy(packed.data(), static_cast<std::uint32_t>(packed.size())),
                static_cast<std::uint16_t>(rect.w * bytesPerPixel));
        }
        texture->SetStatus(ImTextureStatus_OK);
        return;
    }

    if (texture->Status == ImTextureStatus_WantDestroy && texture->UnusedFrames > 0) {
        const bgfx::TextureHandle handle = {static_cast<std::uint16_t>(texture->GetTexID())};
        if (bgfx::isValid(handle)) {
            bgfx::destroy(handle);
        }
        texture->SetTexID(ImTextureID_Invalid);
        texture->SetStatus(ImTextureStatus_Destroyed);
    }
}

void UpdateImGuiTextures(ImDrawData* drawData) {
    if (drawData == nullptr || drawData->Textures == nullptr) {
        return;
    }
    for (ImTextureData* texture : *drawData->Textures) {
        if (texture != nullptr && texture->Status != ImTextureStatus_OK) {
            UpdateImGuiTexture(texture);
        }
    }
}

void DestroyOwnedImGuiTextures() {
    ImGuiPlatformIO& platformIo = ImGui::GetPlatformIO();
    for (ImTextureData* texture : platformIo.Textures) {
        if (texture == nullptr || texture->GetTexID() == ImTextureID_Invalid) {
            continue;
        }
        const bgfx::TextureHandle handle = {static_cast<std::uint16_t>(texture->GetTexID())};
        if (bgfx::isValid(handle)) {
            bgfx::destroy(handle);
        }
        texture->SetTexID(ImTextureID_Invalid);
        texture->SetStatus(ImTextureStatus_Destroyed);
    }
}

bgfx::ShaderHandle LoadShader(const std::string& shaderDirectory, const std::string& name) {
    const bgfx::RendererType::Enum type = bgfx::getRendererType();
    const char* folder = "dx11";
    switch (type) {
        case bgfx::RendererType::Metal:
            folder = "metal";
            break;
        case bgfx::RendererType::OpenGL:
        case bgfx::RendererType::OpenGLES:
            folder = "glsl";
            break;
        case bgfx::RendererType::Vulkan:
            folder = "spirv";
            break;
        default:
            folder = "dx11";
            break;
    }
    const std::string path =
        Platform::Paths::Join(Platform::Paths::Join(shaderDirectory, folder), name + ".bin");
    auto bytes = Platform::Paths::ReadBinaryFile(path);
    if (!bytes.IsOk()) {
        DD_LOG_ERROR("Failed to load imgui shader {}", path);
        return BGFX_INVALID_HANDLE;
    }
    return bgfx::createShader(
        bgfx::copy(bytes.Value().data(), static_cast<std::uint32_t>(bytes.Value().size())));
}

} // namespace

ImGuiGlfwBackend::~ImGuiGlfwBackend() {
    Shutdown();
}

Core::Result<void> ImGuiGlfwBackend::Init(Platform::Window& window,
                                          const std::string& shaderDirectory, std::uint8_t viewId) {
    if (m_initialized) {
        return Core::Result<void>::Fail(Core::Error::Make(Core::ErrorCode::AlreadyInitialized,
                                                          "ImGui backend already initialized",
                                                          "界面系统已经初始化"));
    }

    auto* glfwWindow = static_cast<GLFWwindow*>(window.NativeHandle());
    if (glfwWindow == nullptr) {
        return Core::Result<void>::Fail(Core::Error::Make(Core::ErrorCode::NotInitialized,
                                                          "ImGui init requires a created window",
                                                          "窗口尚未创建"));
    }

    m_shaderDirectory = shaderDirectory;
    m_viewId = viewId;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ApplyDirectorDeskStyle();
    LoadUiFont();

    if (!ImGui_ImplGlfw_InitForOther(glfwWindow, true)) {
        ImGui::DestroyContext();
        return Core::Result<void>::Fail(Core::Error::Make(Core::ErrorCode::Internal,
                                                          "ImGui_ImplGlfw_InitForOther failed",
                                                          "无法初始化界面窗口后端"));
    }

    auto resources = CreateResources();
    if (!resources.IsOk()) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return resources;
    }

    m_initialized = true;
    DD_LOG_INFO("ImGui bgfx backend initialized");
    return Core::Result<void>::Ok();
}

Core::Result<void> ImGuiGlfwBackend::CreateResources() {
    const bgfx::ShaderHandle vs = LoadShader(m_shaderDirectory, "vs_imgui");
    const bgfx::ShaderHandle fs = LoadShader(m_shaderDirectory, "fs_imgui");
    if (!bgfx::isValid(vs) || !bgfx::isValid(fs)) {
        if (bgfx::isValid(vs)) {
            bgfx::destroy(vs);
        }
        if (bgfx::isValid(fs)) {
            bgfx::destroy(fs);
        }
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::NotFound, "ImGui shaders were not found", "找不到界面着色器"));
    }

    const bgfx::ProgramHandle program = bgfx::createProgram(vs, fs, true);
    const bgfx::UniformHandle sampler = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);
    m_program = program.idx;
    m_textureSampler = sampler.idx;
    return Core::Result<void>::Ok();
}

void ImGuiGlfwBackend::DestroyResources() {
    if (m_program != 0xFFFFu) {
        bgfx::ProgramHandle handle = {m_program};
        bgfx::destroy(handle);
        m_program = 0xFFFFu;
    }
    if (m_textureSampler != 0xFFFFu) {
        bgfx::UniformHandle handle = {m_textureSampler};
        bgfx::destroy(handle);
        m_textureSampler = 0xFFFFu;
    }
    if (ImGui::GetCurrentContext() != nullptr) {
        DestroyOwnedImGuiTextures();
    }
}

void ImGuiGlfwBackend::Shutdown() {
    if (!m_initialized) {
        return;
    }
    DestroyResources();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

void ImGuiGlfwBackend::BeginFrame() {
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiGlfwBackend::Submit(std::uint32_t framebufferWidth, std::uint32_t framebufferHeight) {
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData == nullptr || framebufferWidth == 0 || framebufferHeight == 0) {
        return;
    }
    UpdateImGuiTextures(drawData);
    if (drawData->CmdListsCount == 0) {
        return;
    }

    const bgfx::Caps* caps = bgfx::getCaps();
    const float capOriginBottomLeft =
        (caps != nullptr && (caps->originBottomLeft != 0)) ? 1.0f : 0.0f;
    (void)capOriginBottomLeft;

    const glm::mat4 projection =
        glm::orthoRH_ZO(0.0f, static_cast<float>(framebufferWidth),
                        static_cast<float>(framebufferHeight), 0.0f, 0.0f, 1.0f);
    const glm::mat4 view(1.0f);
    bgfx::setViewName(m_viewId, "ImGui");
    bgfx::setViewMode(m_viewId, bgfx::ViewMode::Sequential);
    bgfx::setViewRect(m_viewId, 0, 0, static_cast<std::uint16_t>(framebufferWidth),
                      static_cast<std::uint16_t>(framebufferHeight));
    bgfx::setViewTransform(m_viewId, glm::value_ptr(view), glm::value_ptr(projection));

    const bgfx::VertexLayout layout = ImGuiVertex::Layout();
    const bgfx::ProgramHandle program = {m_program};
    const bgfx::UniformHandle sampler = {m_textureSampler};

    for (int list = 0; list < drawData->CmdListsCount; ++list) {
        const ImDrawList* cmdList = drawData->CmdLists[list];
        const bgfx::TransientVertexBuffer tvb{};
        const bgfx::TransientIndexBuffer tib{};
        bgfx::TransientVertexBuffer vertexBuffer = tvb;
        bgfx::TransientIndexBuffer indexBuffer = tib;

        if (!bgfx::getAvailTransientVertexBuffer(cmdList->VtxBuffer.Size, layout) ||
            !bgfx::getAvailTransientIndexBuffer(cmdList->IdxBuffer.Size, sizeof(ImDrawIdx) == 4)) {
            break;
        }
        bgfx::allocTransientVertexBuffer(&vertexBuffer, cmdList->VtxBuffer.Size, layout);
        bgfx::allocTransientIndexBuffer(&indexBuffer, cmdList->IdxBuffer.Size,
                                        sizeof(ImDrawIdx) == 4);
        std::memcpy(vertexBuffer.data, cmdList->VtxBuffer.Data,
                    sizeof(ImDrawVert) * cmdList->VtxBuffer.Size);
        std::memcpy(indexBuffer.data, cmdList->IdxBuffer.Data,
                    sizeof(ImDrawIdx) * cmdList->IdxBuffer.Size);

        for (int commandIndex = 0; commandIndex < cmdList->CmdBuffer.Size; ++commandIndex) {
            const ImDrawCmd& command = cmdList->CmdBuffer[commandIndex];
            if (command.UserCallback != nullptr) {
                command.UserCallback(cmdList, &command);
                continue;
            }

            const std::uint16_t xx =
                static_cast<std::uint16_t>(command.ClipRect.x > 0.0f ? command.ClipRect.x : 0.0f);
            const std::uint16_t yy =
                static_cast<std::uint16_t>(command.ClipRect.y > 0.0f ? command.ClipRect.y : 0.0f);
            const std::uint16_t ww =
                static_cast<std::uint16_t>((command.ClipRect.z - command.ClipRect.x) > 0.0f
                                               ? (command.ClipRect.z - command.ClipRect.x)
                                               : 0.0f);
            const std::uint16_t hh =
                static_cast<std::uint16_t>((command.ClipRect.w - command.ClipRect.y) > 0.0f
                                               ? (command.ClipRect.w - command.ClipRect.y)
                                               : 0.0f);

            bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;
            const ImTextureID texId = command.GetTexID();
            if (texId != ImTextureID_Invalid) {
                texture.idx = static_cast<std::uint16_t>(texId);
            }
            if (!bgfx::isValid(texture) || command.ElemCount == 0) {
                continue;
            }

            const std::uint32_t vtxOffset = command.VtxOffset;
            const std::uint32_t vtxCount =
                static_cast<std::uint32_t>(cmdList->VtxBuffer.Size) - vtxOffset;
            bgfx::setScissor(xx, yy, ww, hh);
            bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA |
                           BGFX_STATE_BLEND_FUNC_SEPARATE(
                               BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA,
                               BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA));
            bgfx::setTexture(0, sampler, texture);
            bgfx::setVertexBuffer(0, &vertexBuffer, vtxOffset, vtxCount);
            bgfx::setIndexBuffer(&indexBuffer, command.IdxOffset, command.ElemCount);
            bgfx::submit(m_viewId, program);
        }
    }
}

} // namespace DirectorDesk::Backends
