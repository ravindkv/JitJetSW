// HLTCaloAnalyzer: step 2 of the High Level Trigger for one event (chapter 13, HLT, of the JitJet book): the
// calorimeter jets.
//
//   the ECAL and HCAL raw data are unpacked and reconstructed into rec-hits (one energy per crystal or HCAL cell:
//   hltEcalRecHit, hltHbhereco, hltHfreco), the rec-hits are summed into calorimeter towers (hltTowerMakerForAll, the
//   same eta-phi grid as the trigger towers of chapter 12, with the thresholds of CaloTowersCreator), the towers are
//   clustered into anti-kT jets of radius 0.4 (hltAK4CaloJets) and corrected (hltAK4CaloJetsCorrected: L1FastJet with
//   the energy density hltFixedGridRhoFastjetAllCalo, L2Relative, L3Absolute, L2L3Residual of the payload AK4CaloHLT);
//   the jet paths then ask for a corrected calo jet above a threshold before they run the expensive tracking
// Pages (one PDF per event, one PNG per page with savePNG):
//   towers   the calorimeter towers of the event (E_T, on the trigger-tower grid of chapter 12) with the corrected
//            calo jets and the generator jets
//   jet<N>   the towers around the calo jet of each of the two leading generator jets, E_T written in each tower
// The log has the rec-hit and tower sums by region, the energy density, and every calo jet with its corrections
// level by level (read by test/common/hltLedger.py).
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "CondFormats/JetMETObjects/interface/JetCorrectorParameters.h"
#include "DataFormats/CaloTowers/interface/CaloTowerCollection.h"
#include "DataFormats/EcalRecHit/interface/EcalRecHitCollections.h"
#include "DataFormats/HcalRecHit/interface/HcalRecHitCollections.h"
#include "DataFormats/JetReco/interface/CaloJet.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "DataFormats/L1Trigger/interface/BXVector.h"
#include "DataFormats/L1Trigger/interface/Jet.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "JetMETCorrections/Objects/interface/JetCorrectionsRecord.h"

#include "HLTDisplay.h"

namespace {
  using namespace hltdisplay;

  // the column of the display grid that holds a given eta (the trigger-tower grid of chapter 12)
  int columnOfEta(double eta) {
    for (int c = -kTowersPerSide; c < kTowersPerSide; ++c)
      if (eta >= etaLow(c) && eta < etaHigh(c))
        return c;
    return eta < 0 ? -kTowersPerSide : kTowersPerSide - 1;
  }
  int iphiOfPhi(double phi) {
    int r = int(std::floor((wrapToPi(phi) + M_PI) / (2. * M_PI / kNPhi)));
    r = std::clamp(r, 0, kNPhi - 1);
    return r < 36 ? r + 37 : r - 35;  // inverse of l1tdisplay::row
  }

  struct Sums {
    int n = 0, nPos = 0;
    double e = 0, et = 0, maxEt = 0, em = 0, had = 0;
    void add(double energy, double eT, double emEt = 0, double hadEt = 0) {
      ++n;
      if (energy > 0)
        ++nPos;
      e += energy;
      et += eT;
      em += emEt;
      had += hadEt;
      maxEt = std::max(maxEt, eT);
    }
  };
}  // namespace

class HLTCaloAnalyzer : public edm::one::EDAnalyzer<> {
public:
  explicit HLTCaloAnalyzer(const edm::ParameterSet&);
  void analyze(const edm::Event&, const edm::EventSetup&) override;

private:
  edm::EDGetTokenT<CaloTowerCollection> towers_;
  edm::EDGetTokenT<EcalRecHitCollection> ebHits_, eeHits_;
  edm::EDGetTokenT<HBHERecHitCollection> hbheHits_;
  edm::EDGetTokenT<HFRecHitCollection> hfHits_;
  edm::EDGetTokenT<reco::CaloJetCollection> rawJets_, corrJets_;
  edm::EDGetTokenT<double> rho_;
  edm::EDGetTokenT<BXVector<l1t::Jet>> l1Jets_;
  edm::EDGetTokenT<std::vector<reco::GenJet>> genJets_;
  edm::ESGetToken<CaloGeometry, CaloGeometryRecord> geometry_;
  edm::ESGetToken<JetCorrectorParametersCollection, JetCorrectionsRecord> jec_;
  std::vector<std::string> levels_;
  std::string pdfFile_, pngPrefix_, sampleLabel_;
  bool savePNG_;
  double jetPtMin_, zMax_;
  int nEvents_ = 0;
  Page page_;
  Corrections corrections_;
};

HLTCaloAnalyzer::HLTCaloAnalyzer(const edm::ParameterSet& ps)
    : towers_(consumes(ps.getParameter<edm::InputTag>("towers"))),
      ebHits_(consumes(ps.getParameter<edm::InputTag>("ebRecHits"))),
      eeHits_(consumes(ps.getParameter<edm::InputTag>("eeRecHits"))),
      hbheHits_(consumes(ps.getParameter<edm::InputTag>("hbheRecHits"))),
      hfHits_(consumes(ps.getParameter<edm::InputTag>("hfRecHits"))),
      rawJets_(consumes(ps.getParameter<edm::InputTag>("rawJets"))),
      corrJets_(consumes(ps.getParameter<edm::InputTag>("correctedJets"))),
      rho_(consumes(ps.getParameter<edm::InputTag>("rho"))),
      l1Jets_(consumes(ps.getParameter<edm::InputTag>("l1Jets"))),
      genJets_(consumes(ps.getParameter<edm::InputTag>("genJets"))),
      geometry_(esConsumes()),
      jec_(esConsumes(edm::ESInputTag("", ps.getParameter<std::string>("jecPayload")))),
      levels_(ps.getParameter<std::vector<std::string>>("jecLevels")),
      pdfFile_(ps.getParameter<std::string>("pdfFile")),
      pngPrefix_(ps.getParameter<std::string>("pngPrefix")),
      sampleLabel_(ps.getParameter<std::string>("sampleLabel")),
      savePNG_(ps.getParameter<bool>("savePNG")),
      jetPtMin_(ps.getParameter<double>("jetPtMin")),
      zMax_(ps.getParameter<double>("zMax")),
      page_("hltCalo") {}

void HLTCaloAnalyzer::analyze(const edm::Event& ev, const edm::EventSetup& es) {
  std::string tag = Form("_run%u_event%llu", ev.id().run(), ev.id().event());
  std::cout << "== HLTCaloAnalyzer run:lumi:event " << ev.id().run() << ":" << ev.luminosityBlock() << ":" << ev.id().event() << "\n";
  const auto& geo = es.getData(geometry_);
  corrections_.set(es.getData(jec_), levels_);

  // ------------------------------------------------------------------ the rec-hits: one energy per crystal or cell
  auto et = [&](DetId id, double e) { return e / std::cosh(geo.getPosition(id).eta()); };
  Sums eb, ee, hb, he, hf;
  for (auto const& h : ev.get(ebHits_))
    eb.add(h.energy(), et(h.id(), h.energy()));
  for (auto const& h : ev.get(eeHits_))
    ee.add(h.energy(), et(h.id(), h.energy()));
  for (auto const& h : ev.get(hbheHits_))
    (h.id().subdet() == HcalBarrel ? hb : he).add(h.energy(), et(h.id(), h.energy()));
  for (auto const& h : ev.get(hfHits_))
    hf.add(h.energy(), et(h.id(), h.energy()));
  for (auto const& [name, s] : std::vector<std::pair<const char*, const Sums*>>{{"EB", &eb}, {"EE", &ee}, {"HB", &hb}, {"HE", &he}, {"HF", &hf}})
    std::cout << "  rechits " << name << ": n=" << s->n << " positive=" << s->nPos << " sumE=" << Form("%.1f", s->e) << " sumEt=" << Form("%.1f", s->et)
              << " maxEt=" << Form("%.2f", s->maxEt) << "\n";

  // ------------------------------------------------------------------ the towers
  const auto& towers = ev.get(towers_);
  struct Cell {
    double et = 0, em = 0, had = 0;
    bool hit = false;
  };
  std::vector<Cell> grid(2 * kTowersPerSide * kNPhi);
  auto cell = [&](int c, int iphi) -> Cell& { return grid[(c + kTowersPerSide) * kNPhi + wrapPhi(iphi) - 1]; };
  Sums tb, te, tf, ta;
  int maxAbsIeta = 0;
  for (auto const& t : towers) {
    int ieta = t.id().ieta(), iphi = t.id().iphi();
    maxAbsIeta = std::max(maxAbsIeta, std::abs(ieta));
    double tet = t.emEt() + t.hadEt();
    double aeta = std::abs(t.eta());
    (aeta < 1.479 ? tb : aeta < 3.0 ? te : tf).add(t.energy(), tet, t.emEt(), t.hadEt());
    ta.add(t.energy(), tet, t.emEt(), t.hadEt());
    // the display grid: the towers 28 and 29 of the endcap in one column; a tower of 10 degrees (|ieta| 21 to 39) or 20
    // degrees (|ieta| 40, 41) shared out over the 5-degree cells it covers, as Layer-1 does with the HF primitives
    int de = std::abs(ieta) == 29 ? (ieta > 0 ? 28 : -28) : ieta;
    int width = std::abs(ieta) >= 40 ? 4 : std::abs(ieta) >= 21 ? 2 : 1;
    int c = column(de);
    if (!validColumn(c))
      continue;
    for (int w = 0; w < width; ++w) {
      Cell& x = cell(c, iphi + w);
      x.et += tet / width;
      x.em += t.emEt() / width;
      x.had += t.hadEt() / width;
      x.hit = true;
    }
  }
  for (auto const& [name, s] : std::vector<std::pair<const char*, const Sums*>>{{"barrel", &tb}, {"endcap", &te}, {"forward", &tf}, {"all", &ta}})
    std::cout << "  towers " << name << ": n=" << s->n << " sumEt=" << Form("%.1f", s->et) << " sumEmEt=" << Form("%.1f", s->em)
              << " sumHadEt=" << Form("%.1f", s->had) << " maxEt=" << Form("%.2f", s->maxEt) << "\n";
  std::cout << "  towers: max |ieta|=" << maxAbsIeta << "\n";

  // ------------------------------------------------------------------ the calo jets and their corrections
  double rho = ev.get(rho_);
  std::cout << "  rho calo (hltFixedGridRhoFastjetAllCalo): " << Form("%.3f", rho) << " GeV per unit area\n";
  std::cout << "  jec payload levels:";
  for (auto const& l : corrections_.levels())
    std::cout << " " << l;
  std::cout << "\n";
  for (auto const& d : corrections_.description())
    std::cout << "  jec calo: " << d << "\n";
  const auto& raw = ev.get(rawJets_);
  const auto& corr = ev.get(corrJets_);
  std::vector<reco::GenJet> gj;
  for (auto const& j : ev.get(genJets_))
    if (j.pt() > jetPtMin_)
      gj.push_back(j);
  for (auto const& g : gj)
    std::cout << "  GenJet: pt=" << Form("%.1f", g.pt()) << " eta=" << Form("%.2f", g.eta()) << " phi=" << Form("%.2f", g.phi()) << "\n";
  int n10 = 0, n30 = 0, n80 = 0;
  for (auto const& j : corr) {
    n10 += j.pt() > 10;
    n30 += j.pt() > 30;
    n80 += j.pt() > 80;
  }
  std::cout << "  calo jets: raw=" << raw.size() << " corrected=" << corr.size() << " corrected>10=" << n10 << " >30=" << n30 << " >80=" << n80 << "\n";
  int k = 0;
  for (auto const& j : raw) {
    if (j.pt() < 5)
      continue;
    auto f = corrections_.factors(j.pt(), j.energy(), j.eta(), j.phi(), j.jetArea(), rho);
    double c = f.empty() ? 1. : f.back();
    // the same jet after the corrector of the menu
    const reco::CaloJet* cj = nullptr;
    for (auto const& x : corr)
      if (dR(x.eta(), x.phi(), j.eta(), j.phi()) < 0.01)
        cj = &x;
    const reco::GenJet* g = nullptr;
    double dr = 9;
    for (auto const& x : gj)
      if (double d = dR(x.eta(), x.phi(), j.eta(), j.phi()); d < 0.4 && d < dr) {
        dr = d;
        g = &x;
      }
    std::cout << "  CaloJet " << k++ << ": rawPt=" << Form("%.1f", j.pt()) << " eta=" << Form("%.2f", j.eta()) << " phi=" << Form("%.2f", j.phi())
              << " area=" << Form("%.3f", j.jetArea()) << " towers=" << j.getCaloConstituents().size() << " n90=" << j.n90()
              << " emf=" << Form("%.2f", j.emEnergyFraction()) << " |" << factorString(corrections_.levels(), f) << " jecBin=" << corrections_.bin((&j)->eta(), (&j)->phi()) << " | recomputedPt=" << Form("%.1f", j.pt() * c)
              << " correctedPt=" << (cj ? Form("%.1f", cj->pt()) : "-") << " | gen=" << (g ? Form("%.1f", g->pt()) : "-")
              << " dR=" << (g ? Form("%.2f", dr) : "-") << "\n";
  }

  // ------------------------------------------------------------------ page 1: the towers and the calo jets
  page_.openPdf(pdfName(pdfFile_, ev.id().run(), ev.id().event(), nEvents_++));
  page_.begin(true);
  TH2F* h = page_.keep(towerMap("hltCaloTowers", Form("step 2: calorimeter towers and AK4 calo jets, run %u event %llu;#eta;#phi", ev.id().run(), ev.id().event())));
  for (int c = -kTowersPerSide; c < kTowersPerSide; ++c)
    for (int p = 1; p <= kNPhi; ++p)
      if (cell(c, p).hit && cell(c, p).et > 0)
        fillTower(h, ietaOfColumn(c), p, std::max(cell(c, p).et, 0.051));
  h->SetMinimum(0.05);
  h->SetMaximum(zMax_);
  h->GetZaxis()->SetTitle("tower E_{T} [GeV]");
  h->GetZaxis()->SetTitleSize(0.034);
  h->GetZaxis()->SetTitleOffset(1.35);
  h->GetZaxis()->SetLabelSize(0.034);
  h->GetYaxis()->SetTitleOffset(0.9);
  h->Draw("COLZ");
  page_.etaLines({1.479, 3.0});
  genJets(page_, gj, jetPtMin_, false);
  int nDrawn = 0;
  for (auto const& j : corr) {
    if (j.pt() < 20)
      continue;
    ++nDrawn;
    page_.circle(j.eta(), j.phi(), 0.4, kRed, 2, 2);
    page_.note(j.eta() + 0.3, j.phi() - 0.42, Form("%.0f", j.pt()), kRed + 1, 0.022);
  }
  page_.label(sampleLabel_);
  page_.text("rec-hits (E_{T} in GeV, count)", 0.025, kBlue + 1);
  page_.text(Form("   ECAL barrel %.0f (%d), endcap %.0f (%d)", eb.et, eb.n, ee.et, ee.n), 0.019);
  page_.text(Form("   HCAL barrel %.0f (%d), endcap %.0f (%d)", hb.et, hb.n, he.et, he.n), 0.019);
  page_.text(Form("   HF %.0f (%d)", hf.et, hf.n), 0.019);
  page_.skip();
  page_.text("towers", 0.025, kBlue + 1);
  page_.text(Form("   %d towers, #SigmaE_{T} = %.0f GeV", ta.n, ta.et), 0.021);
  page_.text(Form("   barrel %.0f, endcap %.0f, HF %.0f GeV", tb.et, te.et, tf.et), 0.019);
  page_.text(Form("   #rho = %.1f GeV per unit area", rho), 0.021);
  page_.skip();
  page_.text("AK4 calo jets, corrected", 0.025, kBlue + 1);
  page_.text(Form("   %d above 10 GeV, %d above 30, %d above 80", n10, n30, n80), 0.021);
  page_.text(Form("   red dashed circles: the %d above 20 GeV", nDrawn), 0.019, kRed);
  for (auto const& g : gj) {
    const reco::CaloJet* best = nullptr;
    double dr = 0.4;
    for (auto const& x : corr)
      if (double d = dR(x.eta(), x.phi(), g.eta(), g.phi()); d < dr) {
        dr = d;
        best = &x;
      }
    page_.text(Form("   gen %.1f at #eta %.2f: calo jet %s", g.pt(), g.eta(), best ? Form("%.1f GeV", best->pt()) : "none"), 0.019, kGenColor);
  }
  page_.skip();
  page_.text("grid of chapter 12; towers of 10#circ, 20#circ shared over 5#circ cells", 0.017, kGray + 2);
  page_.print(savePNG_ ? pngPrefix_ + "_towers" + tag + ".png" : std::string());

  // ------------------------------------------------------------------ pages 2, 3: the towers around the two signal jets
  for (size_t ig = 0; ig < gj.size() && ig < 2; ++ig) {
    const auto& g = gj[ig];
    const reco::CaloJet* cj = nullptr;
    double dr = 0.4;
    for (auto const& x : corr)
      if (double d = dR(x.eta(), x.phi(), g.eta(), g.phi()); d < dr) {
        dr = d;
        cj = &x;
      }
    double ceta = cj ? cj->eta() : g.eta(), cphi = cj ? cj->phi() : g.phi();
    int c0 = columnOfEta(ceta), p0 = iphiOfPhi(cphi);
    const int half = 8, nb = 2 * half + 1;
    page_.begin(true);
    TH2F* z = page_.keep(new TH2F(Form("hltCaloZoom%zu", ig),
                                  Form("step 2: the towers around generator jet %zu (p_{T} %.1f GeV);i#eta;i#phi", ig + 1, g.pt()), nb, -half - 0.5, half + 0.5, nb,
                                  -half - 0.5, half + 0.5));
    z->SetDirectory(nullptr);
    z->SetStats(false);
    double window = 0, window9 = 0;
    for (int dx = -half; dx <= half; ++dx) {
      int c = c0 + dx;
      z->GetXaxis()->SetBinLabel(dx + half + 1, validColumn(c) ? Form("%d", ietaOfColumn(c)) : "");
      z->GetYaxis()->SetBinLabel(dx + half + 1, Form("%d", wrapPhi(p0 + dx)));
      if (!validColumn(c))
        continue;
      for (int dy = -half; dy <= half; ++dy) {
        const Cell& x = cell(c, p0 + dy);
        if (!x.hit || x.et <= 0)
          continue;
        z->SetBinContent(dx + half + 1, dy + half + 1, std::max(x.et, 0.051));
        if (dR(towerEta(ietaOfColumn(c)), towerPhi(p0 + dy), ceta, cphi) < 0.4)
          window += x.et;
        if (std::abs(dx) <= 4 && std::abs(dy) <= 4)
          window9 += x.et;
      }
    }
    z->SetMinimum(0.05);
    z->SetMaximum(zMax_);
    z->SetMarkerSize(1.2);
    z->GetXaxis()->SetLabelSize(0.035);
    z->GetYaxis()->SetLabelSize(0.035);
    z->GetZaxis()->SetLabelSize(0.034);
    z->GetYaxis()->SetTitleOffset(0.9);
    gStyle->SetPaintTextFormat(".1f");
    z->Draw("COLZ TEXT");
    for (int i = -half; i <= half + 1; ++i) {
      page_.line(i - 0.5, -half - 0.5, i - 0.5, half + 0.5, kGray, 1);
      page_.line(-half - 0.5, i - 0.5, half + 0.5, i - 0.5, kGray, 1);
    }
    page_.box(-4.5, -4.5, 4.5, 4.5, kRed, 2, 2);  // the 9 x 9 towers of the Level-1 jet of chapter 12, for comparison
    std::cout << "  zoom gen jet " << ig + 1 << ": pt=" << Form("%.1f", g.pt()) << " centre ieta=" << ietaOfColumn(c0) << " iphi=" << p0
              << " towersEt(dR<0.4 of the jet axis)=" << Form("%.1f", window) << " towersEt(9x9)=" << Form("%.1f", window9) << "\n";
    page_.label(sampleLabel_);
    page_.text(Form("generator jet %zu", ig + 1), 0.025, kGenColor);
    page_.text(Form("   p_{T} %.1f GeV, #eta %.2f, #phi %.2f", g.pt(), g.eta(), g.phi()), 0.021);
    if (cj) {
      const reco::CaloJet* rj = nullptr;
      for (auto const& x : raw)
        if (dR(x.eta(), x.phi(), cj->eta(), cj->phi()) < 0.01)
          rj = &x;
      page_.skip();
      page_.text("the AK4 calo jet", 0.025, kBlue + 1);
      page_.text(Form("   #eta %.2f, #phi %.2f, #DeltaR to gen %.2f", cj->eta(), cj->phi(), dr), 0.021);
      if (rj) {
        auto f = corrections_.factors(rj->pt(), rj->energy(), rj->eta(), rj->phi(), rj->jetArea(), rho);
        page_.text(Form("   %zu towers, area %.2f, EM fraction %.2f", rj->getCaloConstituents().size(), rj->jetArea(), rj->emEnergyFraction()), 0.019);
        page_.text(Form("   raw p_{T} %.1f GeV", rj->pt()), 0.021);
        for (size_t l = 0; l < f.size() && l < corrections_.levels().size(); ++l)
          page_.text(Form("   after %s: %.1f GeV (#times%.3f)", corrections_.levels()[l].c_str(), rj->pt() * f[l], l ? f[l] / f[l - 1] : f[l]), 0.019);
      }
      page_.text(Form("   corrected p_{T} %.1f GeV", cj->pt()), 0.022, kRed + 1);
    }
    page_.skip();
    page_.text(Form("towers within #DeltaR < 0.4: %.1f GeV", window), 0.019);
    page_.text(Form("dashed red: 9 #times 9 towers (L1 jet): %.1f GeV", window9), 0.019, kRed);
    page_.print(savePNG_ ? pngPrefix_ + Form("_jet%zu", ig + 1) + tag + ".png" : std::string());
  }
  page_.closePdf();
}

DEFINE_FWK_MODULE(HLTCaloAnalyzer);
