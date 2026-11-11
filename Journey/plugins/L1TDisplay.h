#ifndef JitJetSW_Journey_L1TDisplay_h
#define JitJetSW_Journey_L1TDisplay_h
// L1TDisplay.h: what the three Level-1 analyzers of chapter 12 share (L1TCaloAnalyzer, L1TMuonAnalyzer,
// L1TGlobalAnalyzer).
//   - the trigger-tower grid of the calorimeter trigger: 82 x 72 towers, ieta = -41..-1, 1..41 (29 is not used by the
//     trigger), iphi = 1..72 (iphi 1 is 0 < phi < 5 degrees), with the eta boundaries of the towers
//   - a page: a 1400 x 900 canvas with a drawing pad (the left two thirds) and a text panel on the right, like the
//     DigiAnalyzer pages but with a larger text, printed as one page of a PDF and as a PNG
// ROOT keeps one global PDF: every analyzer opens and closes its PDF inside its own analyze() call, so that several
// analyzers can run in the same job.
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "TBox.h"
#include "TCanvas.h"
#include "TEllipse.h"
#include "TH2F.h"
#include "TLatex.h"
#include "TLine.h"
#include "TMarker.h"
#include "TPad.h"
#include "TROOT.h"
#include "TStyle.h"

namespace l1tdisplay {

  // ------------------------------------------------------------------------------------------- the tower grid
  constexpr int kTowersPerSide = 40;  // ieta 1..28 and 30..41
  constexpr int kNPhi = 72;
  // upper eta edge of the k-th tower of one side (k = 1..40); tower 28 is HE towers 28 + 29, towers 30..41 are the HF
  inline const double* etaEdgesHalf() {
    static const double e[kTowersPerSide + 1] = {0.,    0.087, 0.174, 0.261, 0.348, 0.435, 0.522, 0.609, 0.696, 0.783, 0.870,
                                                 0.957, 1.044, 1.131, 1.218, 1.305, 1.392, 1.479, 1.566, 1.653, 1.740, 1.830,
                                                 1.930, 2.043, 2.172, 2.322, 2.500, 2.650, 3.000, 3.139, 3.314, 3.489, 3.664,
                                                 3.839, 4.013, 4.191, 4.363, 4.538, 4.716, 4.889, 5.191};
    return e;
  }
  // a column index without holes: c = -40..-1 for ieta = -41..-1, c = 0..39 for ieta = 1..41 (ieta 29 has no column)
  inline int column(int ieta) {
    int a = std::abs(ieta);
    int k = a >= 30 ? a - 1 : a;  // 1..40
    return ieta > 0 ? k - 1 : -k;
  }
  inline bool validColumn(int c) { return c >= -kTowersPerSide && c < kTowersPerSide; }
  inline int ietaOfColumn(int c) {
    int k = c >= 0 ? c + 1 : -c;
    int a = k >= 29 ? k + 1 : k;
    return c >= 0 ? a : -a;
  }
  inline double etaLow(int c) { return c >= 0 ? etaEdgesHalf()[c] : -etaEdgesHalf()[-c]; }
  inline double etaHigh(int c) { return etaLow(c + 1); }
  inline double towerEta(int ieta) {
    int c = column(ieta);
    return 0.5 * (etaLow(c) + etaHigh(c));
  }
  inline int wrapPhi(int iphi) {
    while (iphi > kNPhi)
      iphi -= kNPhi;
    while (iphi < 1)
      iphi += kNPhi;
    return iphi;
  }
  // the phi row of the page: the page runs from -pi to pi, so iphi 37..72 comes first (row 0..35), then iphi 1..36
  inline int row(int iphi) {
    iphi = wrapPhi(iphi);
    return iphi > 36 ? iphi - 37 : iphi + 35;
  }
  inline double phiLow(int iphi) { return -M_PI + row(iphi) * 2. * M_PI / kNPhi; }
  inline double phiHigh(int iphi) { return phiLow(iphi) + 2. * M_PI / kNPhi; }
  inline double towerPhi(int iphi) { return phiLow(iphi) + M_PI / kNPhi; }
  inline double wrapToPi(double phi) {
    while (phi > M_PI)
      phi -= 2. * M_PI;
    while (phi <= -M_PI)
      phi += 2. * M_PI;
    return phi;
  }
  // the calorimeter region of a tower, in the trigger's words
  inline const char* ecalRegion(int ieta) { return std::abs(ieta) <= 17 ? "EB" : "EE"; }
  inline const char* hcalRegion(int ieta) { return std::abs(ieta) <= 16 ? "HB" : std::abs(ieta) <= 28 ? "HE" : "HF"; }

  // an empty tower map: 80 eta columns with the tower boundaries, 72 phi rows from -pi to pi
  inline TH2F* towerMap(const char* name, const char* title) {
    double edges[2 * kTowersPerSide + 1];
    for (int c = -kTowersPerSide; c <= kTowersPerSide; ++c)
      edges[c + kTowersPerSide] = etaLow(c);
    auto h = new TH2F(name, title, 2 * kTowersPerSide, edges, kNPhi, -M_PI, M_PI);
    h->SetDirectory(nullptr);
    h->SetStats(false);
    return h;
  }
  // fill one tower (width phi rows from iphi on, for the HF trigger primitives that are 2 or 4 towers wide)
  inline void fillTower(TH2F* h, int ieta, int iphi, double value, int width = 1) {
    int c = column(ieta);
    if (!validColumn(c))
      return;
    for (int w = 0; w < width; ++w)
      h->SetBinContent(c + kTowersPerSide + 1, row(iphi + w) + 1, value);
  }

  // ------------------------------------------------------------------------------------------- the page
  class Page {
  public:
    explicit Page(const std::string& name) {
      gROOT->SetBatch(true);
      gStyle->SetOptStat(0);
      gStyle->SetPaperSize(29., 20.);
      gStyle->SetNumberContours(64);
      gStyle->SetTitleFontSize(0.04);
      canvas_ = std::make_unique<TCanvas>((name + "Canvas").c_str(), "", 1400, 900);
      pad_ = new TPad((name + "Pad").c_str(), "", 0., 0., 0.66, 1.);
      side_ = new TPad((name + "Side").c_str(), "", 0.66, 0., 1., 1.);
      pad_->SetLeftMargin(0.08);
      pad_->SetRightMargin(0.14);
      pad_->SetTopMargin(0.07);
      canvas_->cd();
      pad_->Draw();
      side_->Draw();
    }
    static constexpr double kTextScale = 1.75;
    // start a new page: clear, select the drawing pad
    void begin(bool logz = false) {
      pad_->Clear();
      side_->Clear();
      keep_.clear();
      pad_->SetLogz(logz);
      pad_->cd();
      y_ = 0.965;
    }
    TPad* pad() { return pad_; }
    template <class T>
    T* keep(T* obj) {
      keep_.emplace_back(obj);
      return obj;
    }
    // a line of text in the right panel
    void text(const std::string& s, double size = 0.024, int color = kBlack, int font = 42) {
      side_->cd();
      auto t = keep(new TLatex(0.02, y_, s.c_str()));
      t->SetNDC();
      t->SetTextSize(size * kTextScale);  // the panel is narrow: ROOT scales the text with its width
      t->SetTextColor(color);
      t->SetTextFont(font);  // 42: proportional; 82: fixed width, for the rows of a table
      t->Draw();
      y_ -= size * 1.3;
      pad_->cd();
    }
    void skip(double dy = 0.01) { y_ -= dy; }
    // the sample label: lines separated by '|'
    void label(std::string s) {
      size_t pos;
      while ((pos = s.find('|')) != std::string::npos) {
        text(s.substr(0, pos), 0.026);
        s = s.substr(pos + 1);
      }
      text(s, 0.026);
      skip();
    }
    // drawing helpers (in the coordinates of the drawing pad)
    void circle(double x, double y, double r, int color, int width = 2, int style = 1) {
      auto e = keep(new TEllipse(x, y, r, r));
      e->SetFillStyle(0);
      e->SetLineColor(color);
      e->SetLineWidth(width);
      e->SetLineStyle(style);
      e->Draw();
    }
    void box(double x1, double y1, double x2, double y2, int color, int width = 2, int style = 1) {
      auto b = keep(new TBox(x1, y1, x2, y2));
      b->SetFillStyle(0);
      b->SetLineColor(color);
      b->SetLineWidth(width);
      b->SetLineStyle(style);
      b->Draw();
    }
    void line(double x1, double y1, double x2, double y2, int color, int width = 1, int style = 1) {
      auto l = keep(new TLine(x1, y1, x2, y2));
      l->SetLineColor(color);
      l->SetLineWidth(width);
      l->SetLineStyle(style);
      l->Draw();
    }
    void marker(double x, double y, int style, int color, double size) {
      auto m = keep(new TMarker(x, y, style));
      m->SetMarkerColor(color);
      m->SetMarkerSize(size);
      m->Draw();
    }
    void note(double x, double y, const std::string& s, int color = kBlack, double size = 0.022, int align = 12) {
      auto t = keep(new TLatex(x, y, s.c_str()));
      t->SetTextSize(size);
      t->SetTextColor(color);
      t->SetTextFont(42);
      t->SetTextAlign(align);
      t->Draw();
    }
    // a box in tower coordinates, columns c1..c2 and phi rows from iphi1 over n towers, split where phi wraps
    void towerBox(int c1, int c2, int iphi1, int n, int color, int width = 2, int style = 1) {
      c1 = std::max(c1, -kTowersPerSide);
      c2 = std::min(c2, kTowersPerSide - 1);
      if (c1 > c2)
        return;
      int r1 = row(iphi1);
      double dphi = 2. * M_PI / kNPhi;
      if (r1 + n <= kNPhi) {
        box(etaLow(c1), -M_PI + r1 * dphi, etaHigh(c2), -M_PI + (r1 + n) * dphi, color, width, style);
      } else {  // open boxes on both sides of the phi = +-pi line
        double x1 = etaLow(c1), x2 = etaHigh(c2), ya = -M_PI + r1 * dphi, yb = -M_PI + (r1 + n - kNPhi) * dphi;
        line(x1, ya, x2, ya, color, width, style);
        line(x1, ya, x1, M_PI, color, width, style);
        line(x2, ya, x2, M_PI, color, width, style);
        line(x1, yb, x2, yb, color, width, style);
        line(x1, -M_PI, x1, yb, color, width, style);
        line(x2, -M_PI, x2, yb, color, width, style);
      }
    }
    // the detector boundaries the trigger knows about
    void etaLines(const std::vector<double>& etas, int color = kGray + 1, int style = 3) {
      for (double e : etas) {
        line(e, -M_PI, e, M_PI, color, 1, style);
        line(-e, -M_PI, -e, M_PI, color, 1, style);
      }
    }
    // print the page: one page of the open PDF and a PNG
    void print(const std::string& png) {
      canvas_->cd();
      canvas_->Update();
      if (pdfOpen_)
        canvas_->Print(pdf_.c_str());
      if (!png.empty())
        canvas_->SaveAs(png.c_str());
    }
    void openPdf(const std::string& pdf) {
      pdf_ = pdf;
      pdfOpen_ = !pdf.empty();
      if (pdfOpen_)
        canvas_->Print((pdf_ + "[").c_str());
    }
    void closePdf() {
      if (pdfOpen_)
        canvas_->Print((pdf_ + "]").c_str());
      pdfOpen_ = false;
      pad_->Clear();
      side_->Clear();
      keep_.clear();
    }

  private:
    std::unique_ptr<TCanvas> canvas_;
    TPad *pad_ = nullptr, *side_ = nullptr;
    std::vector<std::unique_ptr<TObject>> keep_;
    std::string pdf_;
    bool pdfOpen_ = false;
    double y_ = 0.965;
  };

  // the PDF of an event: the configured name for the first event of the job, <stem>_run<R>_event<E>.pdf afterwards
  inline std::string pdfName(const std::string& pdfFile, unsigned run, unsigned long long event, int nthEvent) {
    if (pdfFile.empty() || nthEvent == 0)
      return pdfFile;
    std::string stem = pdfFile.substr(0, pdfFile.rfind(".pdf"));
    return stem + "_run" + std::to_string(run) + "_event" + std::to_string(event) + ".pdf";
  }

}  // namespace l1tdisplay

#endif
