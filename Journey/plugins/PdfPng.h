#ifndef JitJetSW_Journey_PdfPng_h
#define JitJetSW_Journey_PdfPng_h
// PdfPng.h: the PNG of a page of the analyzers' PDFs (SimHitAnalyzer, DigiAnalyzer and the page of L1TDisplay.h, i.e.
// TrigPrimAnalyzer, the L1T and the HLT analyzers), made from the closed PDF by pdftoppm (poppler) instead of by ROOT.
// ROOT's batch PNG (TCanvas::SaveAs) paints every marker again, about 1.5 ms each: 4 h 34 min of the 4 h 39 min of the
// mixed event of chapter 10 (2.1 million tracker hits in three views, and again for the station ALL), while pdftoppm
// renders the same vector page in seconds (the three tracker pages in 32 s).
// The PNG gets the long side of the canvas in pixels (1396 x 872 for a TCanvas of 1400 x 900, as ROOT's) and
// no resolution chunk (pHYs), as ROOT's PNG: pdfTeX takes it at 72 dpi, so the trims of the book's \includegraphics
// (in bp of the natural size) stay valid. Without pdftoppm the caller falls back to ROOT's SaveAs.
// Usage, for one multi-page PDF:
//   c->Print((pdf + "[").c_str()); pngs.open();
//   c->Print(pdf.c_str()); if (!pngs.add(png)) c->SaveAs(png.c_str());   // every page; png empty: no PNG of this page
//   c->Print((pdf + "]").c_str()); pngs.convert(pdf, c);
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "TCanvas.h"

namespace jitjet {

  // remove the pHYs chunk of a PNG (a chunk is length, type, data, CRC; the CRCs of the other chunks do not change)
  inline bool dropPngResolution(const std::string& png) {
    std::ifstream in(png, std::ios::binary);
    if (!in)
      return false;
    std::string d((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    if (d.size() < 8)
      return false;
    std::string out = d.substr(0, 8);
    size_t i = 8;
    bool dropped = false;
    while (i + 12 <= d.size()) {
      const auto* p = reinterpret_cast<const unsigned char*>(d.data() + i);
      size_t n = (size_t(p[0]) << 24) | (size_t(p[1]) << 16) | (size_t(p[2]) << 8) | size_t(p[3]);
      if (i + 12 + n > d.size())
        return false;
      if (d.compare(i + 4, 4, "pHYs") == 0)
        dropped = true;
      else
        out.append(d, i, 12 + n);
      i += 12 + n;
    }
    if (!dropped)
      return true;
    std::ofstream os(png, std::ios::binary | std::ios::trunc);
    os.write(out.data(), out.size());
    return bool(os);
  }

  inline bool havePdftoppm() {
    static const bool have = std::system("command -v pdftoppm > /dev/null 2>&1") == 0;
    return have;
  }

  // page `page` (counted from 1) of the closed PDF as `png`, its long side `longSide` pixels
  inline bool pdfPageToPng(const std::string& pdf, int page, const std::string& png, unsigned longSide) {
    std::string stem = png.size() > 4 && png.compare(png.size() - 4, 4, ".png") == 0 ? png.substr(0, png.size() - 4) : png;
    std::string cmd = "pdftoppm -cropbox -singlefile -png -f " + std::to_string(page) + " -l " + std::to_string(page) +
                      " -scale-to " + std::to_string(longSide) + " '" + pdf + "' '" + stem + "' 2> /dev/null";
    return std::system(cmd.c_str()) == 0 && dropPngResolution(stem + ".png");
  }

  class PdfPngs {
  public:
    // the PDF was opened: count its pages from 1
    void open() {
      page_ = 0;
      pending_.clear();
    }
    // a page was printed into the PDF. True: its PNG (if any) is made from the PDF when it is closed; false: the
    // caller saves it with ROOT now (pdftoppm not found)
    bool add(const std::string& png) {
      ++page_;
      if (png.empty())
        return true;
      if (!havePdftoppm())
        return false;
      pending_.emplace_back(page_, png);
      return true;
    }
    // the PDF was closed: the PNGs of its recorded pages, the size of the canvas c
    void convert(const std::string& pdf, TCanvas* c) {
      unsigned longSide = std::max(c->GetWw(), c->GetWh());
      for (auto const& [page, png] : pending_)
        if (!pdfPageToPng(pdf, page, png, longSide))
          std::cerr << "PdfPngs: no PNG " << png << " from page " << page << " of " << pdf << std::endl;
      open();
    }

  private:
    int page_ = 0;
    std::vector<std::pair<int, std::string>> pending_;
  };

}  // namespace jitjet

#endif
