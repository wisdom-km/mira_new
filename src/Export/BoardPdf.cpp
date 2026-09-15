// BoardPdf: Minimal A4-landscape PDF writer for storyboard pages (FND-43).
#include "DirectorDesk/Export/ShotExport.h"

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Platform/Paths.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace DirectorDesk::Export {
namespace {

void Put(std::vector<std::uint8_t>& out, const char* text) {
    out.insert(out.end(), text, text + std::strlen(text));
}

void Put(std::vector<std::uint8_t>& out, const std::string& text) {
    out.insert(out.end(), text.begin(), text.end());
}

std::vector<std::uint8_t> ToRgb(const Renderer::PixelBuffer& page) {
    const std::size_t pixels = static_cast<std::size_t>(page.width) * page.height;
    std::vector<std::uint8_t> rgb(pixels * 3u, 255);
    const std::size_t available = page.rgba.size() / 4u;
    const std::size_t count = pixels < available ? pixels : available;
    for (std::size_t i = 0; i < count; ++i) {
        rgb[i * 3u] = page.rgba[i * 4u];
        rgb[i * 3u + 1u] = page.rgba[i * 4u + 1u];
        rgb[i * 3u + 2u] = page.rgba[i * 4u + 2u];
    }
    return rgb;
}

} // namespace

Core::Result<void> WriteBoardPdf(const std::string& utf8Path,
                                 const std::vector<Renderer::PixelBuffer>& pages) {
    if (utf8Path.empty() || pages.empty()) {
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "PDF pages are empty", "没有可导出的分镜页"));
    }
    for (const Renderer::PixelBuffer& page : pages) {
        if (page.width == 0 || page.height == 0) {
            return Core::Result<void>::Fail(Core::Error::Make(
                Core::ErrorCode::InvalidArgument, "PDF page has zero size", "分镜页尺寸无效"));
        }
    }

    std::vector<std::uint8_t> pdf;
    Put(pdf, "%PDF-1.4\n%\xE2\xE3\xCF\xD3\n");
    std::vector<std::size_t> xref(1, 0);

    auto beginObj = [&](int id) {
        while (xref.size() <= static_cast<std::size_t>(id)) {
            xref.push_back(0);
        }
        xref[static_cast<std::size_t>(id)] = pdf.size();
        Put(pdf, std::to_string(id) + " 0 obj\n");
    };
    auto endObj = [&]() { Put(pdf, "\nendobj\n"); };

    const int pageCount = static_cast<int>(pages.size());
    beginObj(1);
    Put(pdf, "<< /Type /Catalog /Pages 2 0 R >>");
    endObj();

    std::string kids;
    for (int i = 0; i < pageCount; ++i) {
        if (i != 0) {
            kids += " ";
        }
        kids += std::to_string(5 + i * 3) + " 0 R";
    }
    beginObj(2);
    Put(pdf, "<< /Type /Pages /Count " + std::to_string(pageCount) + " /Kids [" + kids + "] >>");
    endObj();

    for (int i = 0; i < pageCount; ++i) {
        const std::vector<std::uint8_t> rgb = ToRgb(pages[static_cast<std::size_t>(i)]);
        const int imageId = 3 + i * 3;
        const int contentId = 4 + i * 3;
        const int pageId = 5 + i * 3;
        beginObj(imageId);
        Put(pdf, "<< /Type /XObject /Subtype /Image /Width " +
                     std::to_string(pages[static_cast<std::size_t>(i)].width) + " /Height " +
                     std::to_string(pages[static_cast<std::size_t>(i)].height) +
                     " /ColorSpace /DeviceRGB /BitsPerComponent 8 /Length " +
                     std::to_string(rgb.size()) + " >>\nstream\n");
        pdf.insert(pdf.end(), rgb.begin(), rgb.end());
        Put(pdf, "\nendstream");
        endObj();

        const std::string content = "q 842 0 0 595 0 0 cm /Im1 Do Q\n";
        beginObj(contentId);
        Put(pdf, "<< /Length " + std::to_string(content.size()) + " >>\nstream\n" + content +
                     "endstream");
        endObj();

        beginObj(pageId);
        Put(pdf, "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 842 595] /Resources << /XObject << "
                 "/Im1 " +
                     std::to_string(imageId) + " 0 R >> >> /Contents " + std::to_string(contentId) +
                     " 0 R >>");
        endObj();
    }

    const std::size_t xrefPos = pdf.size();
    Put(pdf, "xref\n0 " + std::to_string(xref.size()) + "\n");
    Put(pdf, "0000000000 65535 f \n");
    for (std::size_t i = 1; i < xref.size(); ++i) {
        char line[32];
        std::snprintf(line, sizeof(line), "%010zu 00000 n \n", xref[i]);
        Put(pdf, line);
    }
    Put(pdf, "trailer << /Size " + std::to_string(xref.size()) + " /Root 1 0 R >>\nstartxref\n" +
                 std::to_string(xrefPos) + "\n%%EOF\n");

    return Platform::Paths::WriteBinaryFile(utf8Path, pdf.data(), pdf.size());
}

} // namespace DirectorDesk::Export
