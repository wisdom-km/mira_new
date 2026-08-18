#pragma once

#include "DirectorDesk/Core/Result.h"
#include "DirectorDesk/Renderer/Types.h"

#include <cstdint>

namespace DirectorDesk::Renderer {

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual Core::Result<void> Init(const RendererInitDesc& desc) = 0;
    virtual void Shutdown() = 0;
    virtual void BeginFrame(std::uint32_t framebufferWidth, std::uint32_t framebufferHeight) = 0;
    virtual void RenderScene(const RenderSceneView& scene, const CameraView& view,
                             const RenderTargetDesc& target) = 0;
    virtual Core::Result<PixelBuffer> ReadbackTarget(const RenderTargetDesc& target) = 0;
    virtual Core::Result<void> RequestReadback(const RenderTargetDesc& target) = 0;
    [[nodiscard]] virtual bool HasPendingReadback() const = 0;
    virtual Core::Result<PixelBuffer> TakeReadback() = 0;
    virtual Core::Result<std::uint32_t> CreateModel(const GpuModelDesc& desc) = 0;
    virtual void DestroyModel(std::uint32_t modelId) = 0;
    virtual Core::Result<std::uint16_t> CreateRgbaTexture(std::uint32_t width, std::uint32_t height,
                                                          const std::uint8_t* rgba) = 0;
    virtual void DestroyRgbaTexture(std::uint16_t textureIndex) = 0;
    virtual void EndFrame() = 0;

    virtual void SetViewportSize(std::uint32_t width, std::uint32_t height) = 0;
    [[nodiscard]] virtual std::uint16_t ViewportTextureIndex() const = 0;
    [[nodiscard]] virtual std::uint32_t ViewportWidth() const = 0;
    [[nodiscard]] virtual std::uint32_t ViewportHeight() const = 0;
};

} // namespace DirectorDesk::Renderer
