// TrigPrimAnalyzer: the calorimeter trigger primitives of one event (chapter 11, DIGI, of the JitJet book).
//
// The DIGI step makes, next to the digis, a second and much coarser copy of the calorimeter data for the Level-1
// trigger of chapter 12: the trigger primitives, one transverse energy per trigger tower and bunch crossing.
//   ECAL  simEcalTriggerPrimitiveDigis: 8 bits, 0.5 GeV per count, with the fine-grain bit and the trigger-tower
//         flag (low / mid / high interest) that drives the selective readout of the same DIGI step
//   HCAL  simHcalTriggerPrimitiveDigis: 8 bits (decoded with the CaloTPGTranscoder: 0.5 GeV per count), a few time
//         samples around the sample of interest, feature bits; the HF primitives are 2 or 4 towers wide in phi
// The analyzer draws them on the 82 x 72 trigger-tower grid (the grid and the pages of plugins/L1TDisplay.h) and
// compares them with what the DIGI step knows and the trigger will not:
//   - the energy the particles of this sample deposited in the ECAL crystals of each tower (the PCaloHits of
//     g4SimHits, summed per trigger tower as transverse energy): what the primitive measures, and which primitives
//     have no deposit behind them (noise). In the mixed sample (mixed = true) the file holds only the hits of the
//     signal, so a primitive without a deposit is pileup or noise
//   - the selective-readout flags that the ECAL digitisation derived from the trigger-tower flags
//   - the time samples of the HCAL primitives (which bunch crossing the energy is assigned to)
// Pages (one PDF per event, one PNG per page with savePNG): the ECAL primitives, the HCAL primitives and, for every
// generator jet above jetPtMin, the 17 x 17 towers around the jet axis with the transverse energy written in each
// tower, for the ECAL and for the HCAL primitives. The log repeats every number in key=value form (read by
// test/common/trigPrimLedger.py).
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "CalibFormats/CaloTPG/interface/CaloTPGRecord.h"
#include "CalibFormats/CaloTPG/interface/CaloTPGTranscoder.h"
#include "DataFormats/EcalDigi/interface/EcalDigiCollections.h"
#include "DataFormats/HcalDigi/interface/HcalDigiCollections.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/CaloTopology/interface/EcalTrigTowerConstituentsMap.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "Geometry/Records/interface/IdealGeometryRecord.h"
#include "SimDataFormats/CaloHit/interface/PCaloHitContainer.h"

#include "L1TDisplay.h"

namespace {
  using namespace l1tdisplay;

  struct TPCell {
    float ecalTP = 0, hcalTP = 0, ecalSim = 0;  // GeV (transverse)
    int ecalRaw = 0, hcalRaw = 0, ttFlag = 0, hcalWidth = 0;
    bool ecalFG = false;
    float hcalSample[4] = {0, 0, 0, 0};  // the time samples of the HCAL primitive, GeV
  };
  struct TPGrid {
    std::array<std::array<TPCell, kNPhi + 1>, 2 * kTowersPerSide> cells;
    TPCell& at(int c, int iphi) { return cells[c + kTowersPerSide][wrapPhi(iphi)]; }
    const TPCell& at(int c, int iphi) const { return cells[c + kTowersPerSide][wrapPhi(iphi)]; }
  };
  struct TPSum {
    int n = 0, nonzero = 0, saturated = 0;
    double et = 0, max = 0;
    int maxIeta = 0, maxIphi = 0;
    void add(double e, int raw, int ieta, int iphi) {
      ++n;
      nonzero += raw > 0;
      saturated += raw >= 255;
      et += e;
      if (e > max) {
        max = e;
        maxIeta = ieta;
        maxIphi = iphi;
      }
    }
  };
  // the tower that contains a direction
  int columnOfEta(double eta) {
    for (int c = -kTowersPerSide; c < kTowersPerSide; ++c)
      if (eta >= etaLow(c) && eta < etaHigh(c))
        return c;
    return eta < 0 ? -kTowersPerSide : kTowersPerSide - 1;
  }
  int iphiOfPhi(double phi) {
    double p = phi < 0 ? phi + 2. * M_PI : phi;
    return wrapPhi(int(p / (2. * M_PI / kNPhi)) + 1);
  }
}  // namespace

class TrigPrimAnalyzer : public edm::one::EDAnalyzer<> {
public:
  explicit TrigPrimAnalyzer(const edm::ParameterSet&);
  void analyze(const edm::Event&, const edm::EventSetup&) override;

private:
  void drawMap(const TPGrid& g, int what, const std::string& title);
  void drawZoom(const TPGrid& g, int what, int cj, int pj, const std::string& title);

  edm::EDGetTokenT<EcalTrigPrimDigiCollection> ecalTPs_;
  edm::EDGetTokenT<HcalTrigPrimDigiCollection> hcalTPs_;
  edm::EDGetTokenT<EBSrFlagCollection> ebSrFlags_;
  edm::EDGetTokenT<EESrFlagCollection> eeSrFlags_;
  edm::EDGetTokenT<edm::PCaloHitContainer> ebHits_, eeHits_;
  edm::EDGetTokenT<std::vector<reco::GenJet>> genJets_;
  edm::ESGetToken<CaloTPGTranscoder, CaloTPGRecord> decoder_;
  edm::ESGetToken<EcalTrigTowerConstituentsMap, IdealGeometryRecord> towerMap_;
  edm::ESGetToken<CaloGeometry, CaloGeometryRecord> caloGeom_;
  std::string pdfFile_, pngPrefix_, sampleLabel_;
  bool savePNG_, mixed_;
  double jetPtMin_, zMax_;
  int nEvents_ = 0;
  Page page_;
};

TrigPrimAnalyzer::TrigPrimAnalyzer(const edm::ParameterSet& ps)
    : ecalTPs_(consumes(ps.getParameter<edm::InputTag>("ecalTPs"))),
      hcalTPs_(consumes(ps.getParameter<edm::InputTag>("hcalTPs"))),
      ebSrFlags_(consumes(ps.getParameter<edm::InputTag>("ebSrFlags"))),
      eeSrFlags_(consumes(ps.getParameter<edm::InputTag>("eeSrFlags"))),
      ebHits_(consumes(ps.getParameter<edm::InputTag>("ebSimHits"))),
      eeHits_(consumes(ps.getParameter<edm::InputTag>("eeSimHits"))),
      genJets_(consumes(ps.getParameter<edm::InputTag>("genJets"))),
      decoder_(esConsumes()),
      towerMap_(esConsumes()),
      caloGeom_(esConsumes()),
      pdfFile_(ps.getParameter<std::string>("pdfFile")),
      pngPrefix_(ps.getParameter<std::string>("pngPrefix")),
      sampleLabel_(ps.getParameter<std::string>("sampleLabel")),
      savePNG_(ps.getParameter<bool>("savePNG")),
      mixed_(ps.getParameter<bool>("mixed")),
      jetPtMin_(ps.getParameter<double>("jetPtMin")),
      zMax_(ps.getParameter<double>("zMax")),
      page_("trigPrim") {}

void TrigPrimAnalyzer::analyze(const edm::Event& ev, const edm::EventSetup& es) {
  const CaloTPGTranscoder& decoder = es.getData(decoder_);
  const EcalTrigTowerConstituentsMap& towerMap = es.getData(towerMap_);
  const CaloGeometry& geom = es.getData(caloGeom_);
  std::string tag = Form("_run%u_event%llu", ev.id().run(), ev.id().event());
  std::cout << "== TrigPrimAnalyzer run:lumi:event " << ev.id().run() << ":" << ev.luminosityBlock() << ":" << ev.id().event() << "\n";

  auto grid = std::make_unique<TPGrid>();
  TPGrid& g = *grid;
  std::map<std::string, TPSum> tp;
  std::map<std::string, std::array<int, 8>> ttFlags;  // region -> number of towers per trigger-tower flag value

  // ------------------------------------------------------------------ the ECAL primitives
  int ecalSamples = 0, ecalSoi = 0;
  double ecalSampleSum[5] = {0, 0, 0, 0, 0};  // summed ET per time sample, aligned so that index 2 is the sample of interest
  for (auto const& t : ev.get(ecalTPs_)) {
    int ieta = t.id().ieta(), iphi = t.id().iphi(), raw = t.compressedEt();
    ecalSamples = t.size();
    ecalSoi = t.sampleOfInterest();
    for (int i = 0; i < t.size(); ++i) {
      int k = i - t.sampleOfInterest() + 2;
      if (k >= 0 && k < 5)
        ecalSampleSum[k] += 0.5 * t.sample(i).compressedEt();
    }
    TPCell& c = g.at(column(ieta), iphi);
    c.ecalRaw = raw;
    c.ecalTP = 0.5 * raw;
    c.ecalFG = t.fineGrain();
    c.ttFlag = t.ttFlag();
    const char* r = ecalRegion(ieta);
    tp[r].add(c.ecalTP, raw, ieta, iphi);
    tp["ECAL"].add(c.ecalTP, raw, ieta, iphi);
    ++ttFlags[r][t.ttFlag() & 0x7];
  }
  // ------------------------------------------------------------------ the HCAL primitives
  int hcalSamples = 0, hcalPresamples = 0;
  double sampleSum[2][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}};  // HB + HE, HF: summed ET per time sample
  for (auto const& t : ev.get(hcalTPs_)) {
    int ieta = t.id().ieta(), iphi = t.id().iphi(), raw = t.SOI_compressedEt(), a = std::abs(ieta);
    if (a == 29 || a > 41 || (a >= 30 && t.id().version() == 0))
      continue;
    TPCell& c = g.at(column(ieta), iphi);
    c.hcalRaw = raw;
    c.hcalTP = decoder.hcaletValue(t.id(), t.t0());
    c.hcalWidth = a >= 40 ? 4 : a >= 30 ? 2 : 1;
    if (a < 30) {
      hcalSamples = t.size();
      hcalPresamples = t.presamples();
    }
    for (int i = 0; i < std::min(t.size(), 4); ++i) {
      c.hcalSample[i] = decoder.hcaletValue(t.id(), t.sample(i));
      // align the HF primitives (fewer samples) on the sample of interest of the HB / HE ones
      int k = i - t.presamples() + 2;
      if (k >= 0 && k < 4)
        sampleSum[a >= 30][k] += c.hcalSample[i];
    }
    const char* r = hcalRegion(ieta);
    tp[r].add(c.hcalTP, raw, ieta, iphi);
    tp["HCAL"].add(c.hcalTP, raw, ieta, iphi);
  }
  std::cout << "  format: ECAL samples per primitive=" << ecalSamples << " sampleOfInterest=" << ecalSoi << " | HCAL (HB, HE) samples per primitive="
            << hcalSamples << " presamples=" << hcalPresamples << "\n";
  for (const char* r : {"ECAL", "EB", "EE", "HCAL", "HB", "HE", "HF"}) {
    const TPSum& s = tp[r];
    std::cout << "  TP " << r << ": n=" << s.n << " nonzero=" << s.nonzero << " sumEt=" << s.et << " maxEt=" << s.max << " maxIeta=" << s.maxIeta
              << " maxIphi=" << s.maxIphi << " saturated=" << s.saturated << "\n";
  }
  std::cout << "  ECAL TP time samples (sum ET per sample; index 2 is the sample of interest, 25 ns per sample): ECAL=" << ecalSampleSum[0] << " "
            << ecalSampleSum[1] << " " << ecalSampleSum[2] << " " << ecalSampleSum[3] << " " << ecalSampleSum[4] << "\n";
  std::cout << "  HCAL TP time samples (sum ET per sample; index 2 is the sample of interest, 25 ns per sample): HBHE=" << sampleSum[0][0] << " "
            << sampleSum[0][1] << " " << sampleSum[0][2] << " " << sampleSum[0][3] << " HF=" << sampleSum[1][0] << " " << sampleSum[1][1] << " "
            << sampleSum[1][2] << " " << sampleSum[1][3] << "\n";

  // ------------------------------------------------------------------ what drives the ECAL readout: the trigger-tower flags
  for (const char* r : {"EB", "EE"})
    std::cout << "  ECAL TT flags " << r << ": low(0)=" << ttFlags[r][0] << " mid(1)=" << ttFlags[r][1] << " high(3)=" << ttFlags[r][3] << " other="
              << ttFlags[r][2] + ttFlags[r][4] + ttFlags[r][5] + ttFlags[r][6] + ttFlags[r][7] << "\n";
  std::array<int, 8> ebSr{}, eeSr{};
  int srFullWithHigh = 0, srFullWithoutFlag = 0;
  for (auto const& f : ev.get(ebSrFlags_)) {
    ++ebSr[f.value() & 0x7];
    if ((f.value() & 0x3) == 3) {
      const TPCell& c = g.at(column(f.id().ieta()), f.id().iphi());
      if (c.ttFlag == 3)
        ++srFullWithHigh;
      else
        ++srFullWithoutFlag;
      std::cout << "  EB tower read in full: ieta=" << f.id().ieta() << " iphi=" << f.id().iphi() << " ecalTP=" << c.ecalTP << " ttFlag=" << c.ttFlag << "\n";
    }
  }
  for (auto const& f : ev.get(eeSrFlags_))
    ++eeSr[f.value() & 0x7];
  std::cout << "  ECAL SR flags EB (2448 towers): suppress(0)=" << ebSr[0] << " zs(1)=" << ebSr[1] << " zs2(2)=" << ebSr[2] << " full(3)=" << ebSr[3]
            << " | full readout with TT flag high=" << srFullWithHigh << " as neighbour of one=" << srFullWithoutFlag << "\n";
  std::cout << "  ECAL SR flags EE (624 supercrystals): suppress(0)=" << eeSr[0] << " zs(1)=" << eeSr[1] << " zs2(2)=" << eeSr[2] << " full(3)=" << eeSr[3] << "\n";

  // ------------------------------------------------------------------ what the particles of this sample deposited in the ECAL
  auto addHits = [&](const edm::PCaloHitContainer& hits) {
    for (auto const& h : hits) {
      DetId id(h.id());
      EcalTrigTowerDetId tt = towerMap.towerOf(id);
      if (tt.null() || tt.ieta() == 0 || std::abs(tt.ieta()) > 28)
        continue;
      GlobalPoint p = geom.getPosition(id);
      g.at(column(tt.ieta()), tt.iphi()).ecalSim += h.energy() * p.perp() / p.mag();
    }
  };
  addHits(ev.get(ebHits_));
  addHits(ev.get(eeHits_));
  struct SimCmp {
    int simTowers = 0, tpOnly = 0, simOnly = 0, both = 0;
    double simEt = 0, tpOnlyEt = 0, simOnlyEt = 0, bothSim = 0, bothTP = 0;
  };
  std::map<std::string, SimCmp> cmp;
  for (int c = -kTowersPerSide; c < kTowersPerSide; ++c) {
    int ieta = ietaOfColumn(c);
    if (std::abs(ieta) > 28)
      continue;
    for (int p = 1; p <= kNPhi; ++p) {
      const TPCell& cell = g.at(c, p);
      for (const char* r : {ecalRegion(ieta), "ECAL"}) {
        SimCmp& s = cmp[r];
        s.simEt += cell.ecalSim;
        bool sim = cell.ecalSim >= 0.25, hasTP = cell.ecalRaw > 0;  // 0.25 GeV: half a count
        s.simTowers += cell.ecalSim > 0;
        if (hasTP && !sim) {
          ++s.tpOnly;
          s.tpOnlyEt += cell.ecalTP;
        } else if (!hasTP && sim) {
          ++s.simOnly;
          s.simOnlyEt += cell.ecalSim;
        } else if (hasTP && sim) {
          ++s.both;
          s.bothSim += cell.ecalSim;
          s.bothTP += cell.ecalTP;
        }
      }
    }
  }
  for (const char* r : {"ECAL", "EB", "EE"}) {
    const SimCmp& s = cmp[r];
    std::cout << "  ECAL deposits " << r << ": towersWithDeposit=" << s.simTowers << " sumDepositEt=" << s.simEt << " | deposit>=0.25GeV and TP: towers="
              << s.both << " depositEt=" << s.bothSim << " tpEt=" << s.bothTP << " | TP without deposit>=0.25GeV: towers=" << s.tpOnly << " tpEt="
              << s.tpOnlyEt << " | deposit>=0.25GeV without TP: towers=" << s.simOnly << " depositEt=" << s.simOnlyEt << "\n";
  }

  // ------------------------------------------------------------------ the generator jets of the sample, tower by tower
  std::vector<reco::GenJet> gj;
  for (auto const& j : ev.get(genJets_))
    if (j.pt() > jetPtMin_)
      gj.push_back(j);
  std::vector<std::pair<int, int>> centres;
  for (size_t k = 0; k < gj.size(); ++k) {
    int cj = columnOfEta(gj[k].eta()), pj = iphiOfPhi(gj[k].phi());
    centres.emplace_back(cj, pj);
    double wE = 0, wH = 0, wS = 0, hs[4] = {0, 0, 0, 0};
    int nE = 0, nH = 0;
    for (int c = cj - 4; c <= cj + 4; ++c) {
      if (!validColumn(c))
        continue;
      bool hf = std::abs(ietaOfColumn(c)) >= 30;
      for (int p = pj - (hf ? 7 : 4); p <= pj + 4; ++p) {
        const TPCell& cell = g.at(c, p);
        if (p >= pj - 4) {
          wE += cell.ecalTP;
          wS += cell.ecalSim;
          nE += cell.ecalRaw > 0;
        }
        // an HF primitive covers 2 or 4 towers: count the fraction of it inside the window
        int inside = 0;
        for (int w = 0; w < std::max(cell.hcalWidth, 1); ++w)
          inside += (p + w >= pj - 4 && p + w <= pj + 4);
        if (cell.hcalRaw > 0 && inside > 0) {
          double f = double(inside) / std::max(cell.hcalWidth, 1);
          wH += f * cell.hcalTP;
          ++nH;
          for (int i = 0; i < 4; ++i)
            hs[i] += f * cell.hcalSample[i];
        }
      }
    }
    std::cout << "  GenJet " << k + 1 << ": pt=" << gj[k].pt() << " eta=" << gj[k].eta() << " phi=" << gj[k].phi() << " towerIeta=" << ietaOfColumn(cj)
              << " towerIphi=" << pj << " | 9x9 towers around it: ecalTP=" << wE << " (" << nE << " towers) hcalTP=" << wH << " (" << nH
              << " primitives) ecalDepositEt=" << wS << " | hcal samples of the window (HB/HE: index 2 = sample of interest; HF: index 1): " << hs[0]
              << " " << hs[1] << " " << hs[2] << " " << hs[3] << "\n";
  }
  // the largest primitives
  std::vector<std::array<float, 3>> top;
  for (int c = -kTowersPerSide; c < kTowersPerSide; ++c)
    for (int p = 1; p <= kNPhi; ++p)
      if (g.at(c, p).ecalRaw + g.at(c, p).hcalRaw > 0)
        top.push_back({{g.at(c, p).ecalTP + g.at(c, p).hcalTP, float(c), float(p)}});
  std::sort(top.begin(), top.end(), [](auto const& a, auto const& b) { return a[0] > b[0]; });
  for (size_t i = 0; i < std::min<size_t>(top.size(), 8); ++i) {
    const TPCell& c = g.at(int(top[i][1]), int(top[i][2]));
    std::cout << "  top tower " << i << ": ieta=" << ietaOfColumn(int(top[i][1])) << " iphi=" << int(top[i][2]) << " ecalTP=" << c.ecalTP << " ttFlag="
              << c.ttFlag << " fineGrain=" << c.ecalFG << " ecalDepositEt=" << c.ecalSim << " hcalTP=" << c.hcalTP << " hcalSamples=" << c.hcalSample[0]
              << "," << c.hcalSample[1] << "," << c.hcalSample[2] << "," << c.hcalSample[3] << "\n";
  }

  // ------------------------------------------------------------------ the pages
  page_.openPdf(pdfName(pdfFile_, ev.id().run(), ev.id().event(), nEvents_++));
  auto png = [&](const std::string& name) { return savePNG_ ? pngPrefix_ + "_" + name + tag + ".png" : std::string(); };
  auto jets = [&](bool labels) {
    for (auto const& j : gj) {
      page_.circle(j.eta(), j.phi(), 0.4, kMagenta + 1, 2);
      if (labels)
        page_.note(j.eta(), j.phi() + (j.phi() > 2.5 ? -0.55 : 0.55), Form("GenJet %.0f GeV", j.pt()), kMagenta + 1, 0.022, 22);
    }
  };
  auto regionLine = [&](const char* r, const TPSum& s) {
    page_.text(Form("%s: %d of %d, #Sigma E_{T} %.1f GeV", r, s.nonzero, s.n, s.et), 0.0205);
  };
  // the ECAL primitives
  drawMap(g, 0, Form("ECAL trigger primitives, run %u event %llu;#eta;#phi", ev.id().run(), ev.id().event()));
  jets(true);
  page_.label(sampleLabel_);
  page_.text("ECAL trigger primitives", 0.026, kGreen + 3);
  page_.text("one E_{T} per tower (5 x 5 crystals in EB)", 0.021);
  page_.text("8 bits, 0.5 GeV per count: max 127.5 GeV", 0.021);
  page_.skip();
  regionLine("EB, |i#eta| #leq 17", tp["EB"]);
  regionLine("EE, |i#eta| 18-28", tp["EE"]);
  page_.text(Form("largest: %.1f GeV at i#eta %d, i#phi %d", tp["ECAL"].max, tp["ECAL"].maxIeta, tp["ECAL"].maxIphi), 0.0205);
  page_.skip();
  page_.text(mixed_ ? "deposited by the particles of the signal:" : "deposited by the particles of this sample:", 0.021, kBlue + 1);
  page_.text(Form("#Sigma E_{T} %.1f GeV in %d towers", cmp["ECAL"].simEt, cmp["ECAL"].simTowers), 0.0205);
  page_.text(Form("primitives with a deposit #geq 0.25 GeV: %d", cmp["ECAL"].both), 0.0205);
  page_.text(Form("without one (%s): %d, %.1f GeV", mixed_ ? "pileup or noise" : "noise", cmp["ECAL"].tpOnly, cmp["ECAL"].tpOnlyEt), 0.0205);
  page_.skip();
  page_.text("the time samples of the primitives (#Sigma E_{T}, GeV):", 0.021, kBlue + 1);
  page_.text(Form("%.1f  %.1f  [%.1f]  %.1f  %.1f", ecalSampleSum[0], ecalSampleSum[1], ecalSampleSum[2], ecalSampleSum[3], ecalSampleSum[4]), 0.0205);
  page_.text("[ ]: the sample of interest, 25 ns per sample", 0.019, kGray + 2);
  page_.skip();
  page_.text("trigger-tower flags (drive the ECAL readout):", 0.021, kBlue + 1);
  page_.text(Form("EB: %d mid, %d high;  EE: %d mid, %d high", ttFlags["EB"][1], ttFlags["EB"][3], ttFlags["EE"][1], ttFlags["EE"][3]), 0.0205);
  page_.text(Form("read in full: %d EB towers, %d EE supercrystals", ebSr[3], eeSr[3]), 0.0205);
  if (!gj.empty())
    page_.text(Form("circles: ak4GenJets, p_{T} > %.0f GeV", jetPtMin_), 0.02, kMagenta + 1);
  page_.print(png("ecaltp"));
  // the HCAL primitives
  drawMap(g, 1, Form("HCAL trigger primitives, run %u event %llu;#eta;#phi", ev.id().run(), ev.id().event()));
  jets(true);
  page_.label(sampleLabel_);
  page_.text("HCAL trigger primitives", 0.026, kRed + 1);
  page_.text("the depths of a tower summed to one E_{T}", 0.021);
  page_.text("8 bits, 0.5 GeV per count: max 127.5 GeV", 0.021);
  page_.skip();
  regionLine("HB, |i#eta| #leq 16", tp["HB"]);
  regionLine("HE, |i#eta| 17-28", tp["HE"]);
  regionLine("HF, |i#eta| 30-41", tp["HF"]);
  page_.text(Form("largest: %.1f GeV at i#eta %d, i#phi %d", tp["HCAL"].max, tp["HCAL"].maxIeta, tp["HCAL"].maxIphi), 0.0205);
  page_.skip();
  page_.text("the time samples of the primitives (#Sigma E_{T}, GeV):", 0.021, kBlue + 1);
  page_.text(Form("HB + HE: %.1f  %.1f  [%.1f]  %.1f", sampleSum[0][0], sampleSum[0][1], sampleSum[0][2], sampleSum[0][3]), 0.0205);
  page_.text(Form("HF: %.1f  [%.1f]  %.1f", sampleSum[1][1], sampleSum[1][2], sampleSum[1][3]), 0.0205);
  page_.text("[ ]: the sample of interest, 25 ns per sample", 0.019, kGray + 2);
  page_.skip();
  page_.text("HF: 10#circ wide in #phi (20#circ for |i#eta| #geq 40)", 0.02, kGray + 2);
  if (!gj.empty())
    page_.text(Form("circles: ak4GenJets, p_{T} > %.0f GeV", jetPtMin_), 0.02, kMagenta + 1);
  page_.print(png("hcaltp"));
  // the towers around every generator jet
  for (size_t k = 0; k < gj.size(); ++k) {
    const char* names[] = {"ecaltp", "hcaltp"};
    const char* titles[] = {"ECAL trigger primitives", "HCAL trigger primitives"};
    for (int what = 0; what < 2; ++what) {
      int cj = centres[k].first, pj = centres[k].second;
      drawZoom(g, what, cj, pj,
               Form("GenJet %zu (%.1f GeV): %s around the jet axis, E_{T} in GeV;i#eta of the tower;i#phi of the tower", k + 1, gj[k].pt(), titles[what]));
      double wE = 0, wH = 0, wS = 0;
      for (int c = cj - 4; c <= cj + 4; ++c)
        for (int p = pj - 4; p <= pj + 4; ++p)
          if (validColumn(c)) {
            wE += g.at(c, p).ecalTP;
            wS += g.at(c, p).ecalSim;
            if (g.at(c, p).hcalWidth == 1)
              wH += g.at(c, p).hcalTP;
          }
      page_.label(sampleLabel_);
      page_.text(Form("GenJet %zu: %s", k + 1, titles[what]), 0.025, what == 0 ? kGreen + 3 : kRed + 1);
      page_.text(Form("p_{T} %.1f GeV, #eta %.2f, #phi %.2f", gj[k].pt(), gj[k].eta(), gj[k].phi()), 0.021, kMagenta + 1);
      page_.text(Form("jet axis in tower i#eta %d, i#phi %d (black)", ietaOfColumn(cj), pj), 0.021);
      page_.skip();
      page_.text("in the 9 x 9 towers around it (red):", 0.022, kRed);
      page_.text(Form("ECAL primitives   %6.1f GeV", wE), 0.02, kBlack, 82);
      if (std::abs(ietaOfColumn(cj)) + 4 < 30)
        page_.text(Form("HCAL primitives   %6.1f GeV", wH), 0.02, kBlack, 82);
      page_.text(Form("ECAL deposits     %6.1f GeV", wS), 0.02, kBlue + 1, 82);
      page_.skip();
      page_.text("deposits: E_{T} left in the crystals by the", 0.019, kGray + 2);
      page_.text(mixed_ ? "particles of the signal (chapter 10)" : "particles of this sample (chapter 10)", 0.019, kGray + 2);
      if (what == 1 && std::abs(ietaOfColumn(cj)) + 8 >= 30)
        page_.text("an HF primitive spans 2 (4) towers in #phi (frames)", 0.019, kGray + 2);
      page_.text("i#eta 29 does not exist in the trigger", 0.019, kGray + 2);
      page_.print(png(Form("jet%zu_%s", k + 1, names[what])));
    }
  }
  page_.closePdf();
}

// the primitives in eta-phi: what = 0 ECAL, 1 HCAL
void TrigPrimAnalyzer::drawMap(const TPGrid& g, int what, const std::string& title) {
  page_.begin(true);
  TH2F* h = page_.keep(towerMap(Form("trigPrimMap%d", what), title.c_str()));
  for (int c = -kTowersPerSide; c < kTowersPerSide; ++c)
    for (int p = 1; p <= kNPhi; ++p) {
      const TPCell& cell = g.at(c, p);
      if (what == 0 && cell.ecalRaw > 0)
        fillTower(h, ietaOfColumn(c), p, cell.ecalTP);
      else if (what == 1 && cell.hcalRaw > 0)
        fillTower(h, ietaOfColumn(c), p, std::max(cell.hcalTP, 0.26f), cell.hcalWidth);
    }
  h->SetMinimum(0.25);
  h->SetMaximum(zMax_);
  h->GetZaxis()->SetTitle("E_{T} of the trigger tower [GeV]");
  h->GetZaxis()->SetTitleSize(0.034);
  h->GetZaxis()->SetTitleOffset(1.35);
  h->GetZaxis()->SetLabelSize(0.034);
  h->GetYaxis()->SetTitleOffset(0.9);
  h->Draw("COLZ");
  page_.etaLines({1.479, 3.0});
  page_.note(-1.479, 3.19, "EB|EE", kGray + 2, 0.02, 21);
  page_.note(1.479, 3.19, "EB|EE", kGray + 2, 0.02, 21);
  page_.note(-3.0, 3.19, "HF|HE", kGray + 2, 0.02, 21);
  page_.note(3.0, 3.19, "HE|HF", kGray + 2, 0.02, 21);
}

// 17 x 17 towers around a tower, one square per tower with its transverse energy written in it
void TrigPrimAnalyzer::drawZoom(const TPGrid& g, int what, int cj, int pj, const std::string& title) {
  page_.begin(true);
  const int half = 8, nb = 2 * half + 1;
  TH2F* h = page_.keep(new TH2F(Form("trigPrimZoom%d", what), title.c_str(), nb, -half - 0.5, half + 0.5, nb, -half - 0.5, half + 0.5));
  h->SetDirectory(nullptr);
  h->SetStats(false);
  for (int dx = -half; dx <= half; ++dx) {
    int c = cj + dx;
    h->GetXaxis()->SetBinLabel(dx + half + 1, validColumn(c) ? Form("%d", ietaOfColumn(c)) : "");
    h->GetYaxis()->SetBinLabel(dx + half + 1, Form("%d", wrapPhi(pj + dx)));
    if (!validColumn(c))
      continue;
    for (int dy = -half; dy <= half; ++dy) {
      const TPCell& cell = g.at(c, pj + dy);
      if ((what == 0 ? cell.ecalRaw : cell.hcalRaw) > 0)
        h->SetBinContent(dx + half + 1, dy + half + 1, std::max(what == 0 ? cell.ecalTP : cell.hcalTP, 0.26f));
    }
  }
  h->SetMinimum(0.25);
  h->SetMaximum(zMax_);
  h->SetMarkerSize(1.25);
  h->GetZaxis()->SetLabelSize(0.034);
  h->GetXaxis()->SetLabelSize(0.035);
  h->GetYaxis()->SetLabelSize(0.035);
  h->GetYaxis()->SetTitleOffset(0.9);
  gStyle->SetPaintTextFormat(".1f");
  h->Draw("COLZ TEXT");
  for (int i = -half; i <= half + 1; ++i) {
    page_.line(i - 0.5, -half - 0.5, i - 0.5, half + 0.5, kGray, 1);
    page_.line(-half - 0.5, i - 0.5, half + 0.5, i - 0.5, kGray, 1);
  }
  if (what == 1)
    for (int dx = -half; dx <= half; ++dx) {
      int c = cj + dx;
      if (!validColumn(c) || std::abs(ietaOfColumn(c)) < 30)
        continue;
      for (int dy = -half - 3; dy <= half; ++dy) {
        const TPCell& cell = g.at(c, pj + dy);
        double y1 = std::max(dy - 0.45, -half - 0.5), y2 = std::min(dy + cell.hcalWidth - 0.55, half + 0.5);
        if (cell.hcalWidth > 1 && cell.hcalRaw > 0 && y2 > y1)
          page_.box(dx - 0.45, y1, dx + 0.45, y2, kGray + 2, 1);
      }
    }
  page_.box(-4.5, -4.5, 4.5, 4.5, kRed, 4);
  page_.box(-0.5, -0.5, 0.5, 0.5, kBlack, 3);
}

DEFINE_FWK_MODULE(TrigPrimAnalyzer);
