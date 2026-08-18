#include "DirectorDesk/Storyboard/BoardComposer.h"

#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Storyboard/Layout.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace DirectorDesk::Storyboard {
namespace {

void PutPixel(ImageBuffer& image, int x, int y, std::uint8_t r, std::uint8_t g, std::uint8_t b,
              std::uint8_t a = 255) {
    if (x < 0 || y < 0 || x >= static_cast<int>(image.width) ||
        y >= static_cast<int>(image.height)) {
        return;
    }
    const std::size_t i =
        (static_cast<std::size_t>(y) * image.width + static_cast<std::size_t>(x)) * 4u;
    image.rgba[i] = r;
    image.rgba[i + 1] = g;
    image.rgba[i + 2] = b;
    image.rgba[i + 3] = a;
}

void FillRect(ImageBuffer& image, int x, int y, int w, int h, std::uint8_t r, std::uint8_t g,
              std::uint8_t b) {
    for (int yy = y; yy < y + h; ++yy) {
        for (int xx = x; xx < x + w; ++xx) {
            PutPixel(image, xx, yy, r, g, b);
        }
    }
}

void DrawLine(ImageBuffer& image, int x0, int y0, int x1, int y1, std::uint8_t r, std::uint8_t g,
              std::uint8_t b) {
    const int dx = std::abs(x1 - x0);
    const int sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0);
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        PutPixel(image, x0, y0, r, g, b);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void Blit(ImageBuffer& dest, int x, int y, const ImageBuffer& src) {
    if (src.width == 0 || src.height == 0 || src.rgba.size() < src.width * src.height * 4u) {
        return;
    }
    for (std::uint32_t sy = 0; sy < src.height; ++sy) {
        for (std::uint32_t sx = 0; sx < src.width; ++sx) {
            const std::size_t i = (static_cast<std::size_t>(sy) * src.width + sx) * 4u;
            PutPixel(dest, x + static_cast<int>(sx), y + static_cast<int>(sy), src.rgba[i],
                     src.rgba[i + 1], src.rgba[i + 2], src.rgba[i + 3]);
        }
    }
}

struct FontBlit {
    stbtt_fontinfo font{};
    std::vector<std::uint8_t> bytes;
    bool ok = false;
};

FontBlit LoadFont(const std::string& path) {
    FontBlit font;
    if (path.empty() || !Platform::Paths::Exists(path)) {
        return font;
    }
    auto data = Platform::Paths::ReadBinaryFile(path);
    if (!data.IsOk()) {
        return font;
    }
    font.bytes = std::move(data.Value());
    if (stbtt_InitFont(&font.font, font.bytes.data(),
                       stbtt_GetFontOffsetForIndex(font.bytes.data(), 0)) != 0) {
        font.ok = true;
    }
    return font;
}

void DrawText(ImageBuffer& image, FontBlit& font, int x, int y, const std::string& text,
              float pixelHeight) {
    if (!font.ok || text.empty()) {
        return;
    }
    const float scale = stbtt_ScaleForPixelHeight(&font.font, pixelHeight);
    int ascent = 0;
    stbtt_GetFontVMetrics(&font.font, &ascent, nullptr, nullptr);
    int cursor = x;
    const int baseline = y + static_cast<int>(static_cast<float>(ascent) * scale);
    std::size_t i = 0;
    while (i < text.size() && cursor < static_cast<int>(image.width)) {
        unsigned char lead = static_cast<unsigned char>(text[i]);
        int cp = lead;
        std::size_t step = 1;
        if (lead >= 0x80) {
            if ((lead & 0xe0) == 0xc0 && i + 1 < text.size()) {
                cp = ((lead & 0x1f) << 6) | (static_cast<unsigned char>(text[i + 1]) & 0x3f);
                step = 2;
            } else if ((lead & 0xf0) == 0xe0 && i + 2 < text.size()) {
                cp = ((lead & 0x0f) << 12) | ((static_cast<unsigned char>(text[i + 1]) & 0x3f) << 6) |
                     (static_cast<unsigned char>(text[i + 2]) & 0x3f);
                step = 3;
            } else {
                ++i;
                continue;
            }
        }
        i += step;
        int ax = 0;
        int lsb = 0;
        stbtt_GetCodepointHMetrics(&font.font, cp, &ax, &lsb);
        int w = 0;
        int h = 0;
        int xoff = 0;
        int yoff = 0;
        unsigned char* bitmap =
            stbtt_GetCodepointBitmap(&font.font, scale, scale, cp, &w, &h, &xoff, &yoff);
        if (bitmap != nullptr) {
            for (int py = 0; py < h; ++py) {
                for (int px = 0; px < w; ++px) {
                    if (bitmap[py * w + px] > 64) {
                        PutPixel(image, cursor + xoff + px, baseline + yoff + py, 240, 240, 240);
                    }
                }
            }
            stbtt_FreeBitmap(bitmap, nullptr);
        }
        cursor += static_cast<int>(static_cast<float>(ax) * scale);
    }
}

} // namespace

Core::Result<BoardComposeResult> ComposeBoard(const BoardComposeRequest& request) {
    float width = std::max(request.layout.contentWidth, 64.0f);
    float height = std::max(request.layout.contentHeight, 64.0f);
    float scale = 1.0f;
    const float longest = std::max(width, height);
    if (longest > static_cast<float>(request.maxEdge)) {
        scale = static_cast<float>(request.maxEdge) / longest;
        width *= scale;
        height *= scale;
    }
    BoardComposeResult result;
    result.scaledToMax = scale < 1.0f;
    result.pixels.width = static_cast<std::uint32_t>(std::ceil(width));
    result.pixels.height = static_cast<std::uint32_t>(std::ceil(height));
    result.pixels.rgba.assign(result.pixels.width * result.pixels.height * 4u, 255);
    for (std::size_t i = 0; i < result.pixels.rgba.size(); i += 4) {
        result.pixels.rgba[i] = 28;
        result.pixels.rgba[i + 1] = 30;
        result.pixels.rgba[i + 2] = 36;
    }

    auto sx = [&](float v) { return static_cast<int>(v * scale); };

    for (const LayoutEdge& edge : request.layout.edges) {
        const LayoutCard* from = FindCard(request.layout, edge.fromId);
        const LayoutCard* to = FindCard(request.layout, edge.toId);
        if (from == nullptr || to == nullptr) {
            continue;
        }
        DrawLine(result.pixels, sx(from->x + from->w), sx(from->y + from->h * 0.5f), sx(to->x),
                 sx(to->y + to->h * 0.5f), 90, 100, 120);
    }

    FontBlit font = LoadFont(request.fontPath);
    for (const LayoutCard& card : request.layout.cards) {
        std::uint8_t r = 46;
        std::uint8_t g = 52;
        std::uint8_t b = 64;
        if (card.kind == CardKind::Root) {
            r = 40;
            g = 70;
            b = 80;
        } else if (card.kind == CardKind::Scene) {
            r = 52;
            g = 48;
            b = 72;
        }
        FillRect(result.pixels, sx(card.x), sx(card.y), sx(card.w), sx(card.h), r, g, b);
        DrawText(result.pixels, font, sx(card.x) + 8, sx(card.y) + 8, card.title, 18.0f * scale);
        if (card.kind == CardKind::Shot) {
            const char* link = card.link == LinkStatus::Linked ? "已关联" : "未关联";
            const char* preview = "缺失";
            switch (card.preview) {
            case PreviewStatus::Ready:
                preview = "就绪";
                break;
            case PreviewStatus::Stale:
                preview = "过期";
                break;
            case PreviewStatus::Rendering:
                preview = "渲染中";
                break;
            case PreviewStatus::Failed:
                preview = "失败";
                break;
            case PreviewStatus::Missing:
            default:
                break;
            }
            DrawText(result.pixels, font, sx(card.x) + 8, sx(card.y) + 28,
                     std::string(link) + " · " + preview, 14.0f * scale);
            const auto thumb = request.thumbnails.find(card.shotId);
            if (thumb != request.thumbnails.end()) {
                Blit(result.pixels, sx(card.x) + 10, sx(card.y) + 48, thumb->second);
            }
        }
    }
    return Core::Result<BoardComposeResult>::Ok(std::move(result));
}

} // namespace DirectorDesk::Storyboard
