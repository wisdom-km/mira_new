// BoardComposer: Implementation for the DirectorDesk Storyboard module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

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
            int textY = sx(card.y) + 28;
            if (!card.metaLine.empty()) {
                DrawText(result.pixels, font, sx(card.x) + 8, textY, card.metaLine, 14.0f * scale);
                textY += static_cast<int>(16.0f * scale);
            }
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
            DrawText(result.pixels, font, sx(card.x) + 8, textY,
                     std::string(link) + " · " + preview, 14.0f * scale);
            const auto thumb = request.thumbnails.find(card.shotId);
            if (thumb != request.thumbnails.end()) {
                Blit(result.pixels, sx(card.x) + 10, textY + static_cast<int>(20.0f * scale),
                     thumb->second);
            }
        }
    }
    return Core::Result<BoardComposeResult>::Ok(std::move(result));
}

void BlitFit(ImageBuffer& dest, int x, int y, int w, int h, const ImageBuffer& src) {
    if (src.width == 0 || src.height == 0 || w <= 0 || h <= 0) {
        return;
    }
    const float scale = std::min(static_cast<float>(w) / static_cast<float>(src.width),
                                 static_cast<float>(h) / static_cast<float>(src.height));
    const int dw = std::max(1, static_cast<int>(static_cast<float>(src.width) * scale));
    const int dh = std::max(1, static_cast<int>(static_cast<float>(src.height) * scale));
    const int ox = x + (w - dw) / 2;
    const int oy = y + (h - dh) / 2;
    for (int yy = 0; yy < dh; ++yy) {
        const int srcY = yy * static_cast<int>(src.height) / dh;
        for (int xx = 0; xx < dw; ++xx) {
            const int srcX = xx * static_cast<int>(src.width) / dw;
            const std::size_t i =
                (static_cast<std::size_t>(srcY) * src.width + static_cast<std::size_t>(srcX)) * 4u;
            if (i + 3 >= src.rgba.size()) {
                continue;
            }
            PutPixel(dest, ox + xx, oy + yy, src.rgba[i], src.rgba[i + 1], src.rgba[i + 2],
                     src.rgba[i + 3]);
        }
    }
}

Core::Result<BoardPdfResult> ComposePdfPages(const BoardComposeRequest& request, int cellsPerPage) {
    std::vector<const LayoutCard*> shots;
    for (const LayoutCard& card : request.layout.cards) {
        if (card.kind == CardKind::Shot) {
            shots.push_back(&card);
        }
    }
    if (shots.empty()) {
        return Core::Result<BoardPdfResult>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "No shots to export", "没有可导出的镜头"));
    }
    if (cellsPerPage < 1) {
        cellsPerPage = 6;
    }
    constexpr int kPageW = 1684;
    constexpr int kPageH = 1190;
    constexpr int kCols = 3;
    const int rows = (cellsPerPage + kCols - 1) / kCols;
    const int margin = 36;
    const int gap = 16;
    const int caption = 48;
    const int cellW = (kPageW - margin * 2 - gap * (kCols - 1)) / kCols;
    const int cellH = (kPageH - margin * 2 - gap * (rows - 1)) / rows;
    FontBlit font = LoadFont(request.fontPath);

    BoardPdfResult result;
    const int pageCount = (static_cast<int>(shots.size()) + cellsPerPage - 1) / cellsPerPage;
    for (int page = 0; page < pageCount; ++page) {
        ImageBuffer image;
        image.width = kPageW;
        image.height = kPageH;
        image.rgba.assign(static_cast<std::size_t>(kPageW) * kPageH * 4u, 255);
        for (std::size_t i = 0; i < image.rgba.size(); i += 4) {
            image.rgba[i] = 245;
            image.rgba[i + 1] = 245;
            image.rgba[i + 2] = 247;
        }
        for (int slot = 0; slot < cellsPerPage; ++slot) {
            const int index = page * cellsPerPage + slot;
            if (index >= static_cast<int>(shots.size())) {
                break;
            }
            const int col = slot % kCols;
            const int row = slot / kCols;
            const int x = margin + col * (cellW + gap);
            const int y = margin + row * (cellH + gap);
            FillRect(image, x, y, cellW, cellH, 32, 34, 40);
            const LayoutCard* card = shots[static_cast<std::size_t>(index)];
            DrawText(image, font, x + 10, y + 8, card->title, 22.0f);
            if (!card->metaLine.empty()) {
                DrawText(image, font, x + 10, y + 28, card->metaLine, 16.0f);
            }
            const auto thumb = request.thumbnails.find(card->shotId);
            if (thumb != request.thumbnails.end()) {
                BlitFit(image, x + 10, y + caption, cellW - 20, cellH - caption - 12, thumb->second);
            }
        }
        result.pages.push_back(std::move(image));
    }
    return Core::Result<BoardPdfResult>::Ok(std::move(result));
}

} // namespace DirectorDesk::Storyboard
