// IRenderer: Public or internal interface for the DirectorDesk Renderer module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Core/Result.h"
#include "DirectorDesk/Renderer/Types.h"

#include <cstdint>

namespace DirectorDesk::Renderer {

class IRenderer {
public:
    virtual ~IRenderer() = default;

    /// Initializes the backend and takes ownership of renderer-side GPU resources.
    virtual Core::Result<void> Init(const RendererInitDesc& desc) = 0;
    /// Releases GPU resources; implementations must tolerate a single shutdown call.
    virtual void Shutdown() = 0;
    /// Begins a frame and synchronizes the window swapchain size.
    virtual void BeginFrame(std::uint32_t framebufferWidth, std::uint32_t framebufferHeight) = 0;
    /// Submits a scene to either the interactive viewport or an offscreen target.
    virtual void RenderScene(const RenderSceneView& scene, const CameraView& view,
                             const RenderTargetDesc& target) = 0;
    /// Performs a synchronous readback for export and other blocking callers.
    virtual Core::Result<PixelBuffer> ReadbackTarget(const RenderTargetDesc& target) = 0;
    /// Queues a readback without blocking the UI frame.
    virtual Core::Result<void> RequestReadback(const RenderTargetDesc& target) = 0;
    /// Reports whether an asynchronous readback is in flight.
    [[nodiscard]] virtual bool HasPendingReadback() const = 0;
    /// Retrieves a completed asynchronous readback.
    virtual Core::Result<PixelBuffer> TakeReadback() = 0;
    /// Uploads a model and returns an opaque backend-owned model identifier.
    virtual Core::Result<std::uint32_t> CreateModel(const GpuModelDesc& desc) = 0;
    /// Releases a model previously returned by CreateModel.
    virtual void DestroyModel(std::uint32_t modelId) = 0;
    /// Uploads RGBA pixels and returns a texture identifier usable by the UI.
    virtual Core::Result<std::uint16_t> CreateRgbaTexture(std::uint32_t width, std::uint32_t height,
                                                          const std::uint8_t* rgba) = 0;
    /// Releases a texture previously returned by CreateRgbaTexture.
    virtual void DestroyRgbaTexture(std::uint16_t textureIndex) = 0;
    /// Presents the submitted frame and advances the backend frame counter.
    virtual void EndFrame() = 0;

    /// Resizes the offscreen framebuffer used by the interactive viewport.
    virtual void SetViewportSize(std::uint32_t width, std::uint32_t height) = 0;
    [[nodiscard]] virtual std::uint16_t ViewportTextureIndex() const = 0;
    [[nodiscard]] virtual std::uint32_t ViewportWidth() const = 0;
    [[nodiscard]] virtual std::uint32_t ViewportHeight() const = 0;
};

} // namespace DirectorDesk::Renderer
