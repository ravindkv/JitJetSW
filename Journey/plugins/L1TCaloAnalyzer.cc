// L1TCaloAnalyzer: the calorimeter trigger of one event, step by step (chapter 12, L1T, of the JitJet book).
//
// The Level-1 calorimeter trigger sees the event on a grid of 82 x 72 trigger towers. This analyzer draws that grid
// at every step of the chain and prints the numbers of each step:
//   step 1  the trigger primitives: one transverse energy per tower and bunch crossing from the ECAL
//           (simEcalTriggerPrimitiveDigis, 8 bits, 0.5 GeV per count, with the fine-grain bit) and one from the HCAL
//           (simHcalTriggerPrimitiveDigis, 8 bits decoded with the CaloTPGTranscoder: 0.5 GeV per count since the
//           scale was made linear in 2018, with the feature bits; the HF primitives are 2 or 4 towers wide in phi)
//   step 2  calorimeter Layer-1 (simCaloStage2Layer1Digis): the two primitives of a tower calibrated and summed into
//           one tower of 9 bits, 0.5 GeV per count, with the E/H ratio code and the flag bits
//   step 3  calorimeter Layer-2 (simCaloStage2Digis): the jets, 9 x 9 towers around a seed, with the pileup
//           estimate and the calibration; the e/gamma and tau candidates and the energy sums. The pileup estimate
//           is what the conditions ask for: the "phi ring" of the 2024 trigger (the 9 eta rings of the window
//           summed over all 72 towers in phi, scaled to the 9 towers of the window: jetPUSUsePhiRing = 1) or the
//           "chunky donut" of 2016-2022 (the three quietest of the four 3 x 9 strips around the window)
// Pages (one PDF per event, one PNG per page with savePNG): the ECAL primitives, the HCAL primitives, the Layer-1
// towers and the Layer-2 jets in eta-phi with the generator jets as circles; then, for the zoomJets leading jets,
// the 17 x 17 towers around the seed as the firmware sees them (one square per tower, the transverse energy written
// in it) for the ECAL primitives, the HCAL primitives and the Layer-1 towers, with the jet window and the towers
// of the pileup estimate.
// The log repeats every number in key=value form (read by test/common/l1tLedger.py).
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
#include "CondFormats/DataRecord/interface/L1TCaloParamsRcd.h"
#include "CondFormats/L1TObjects/interface/CaloParams.h"
#include "DataFormats/EcalDigi/interface/EcalDigiCollections.h"
#include "DataFormats/HcalDigi/interface/HcalDigiCollections.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "DataFormats/L1TCalorimeter/interface/CaloTower.h"
#include "DataFormats/L1Trigger/interface/BXVector.h"
#include "DataFormats/L1Trigger/interface/EGamma.h"
#include "DataFormats/L1Trigger/interface/EtSum.h"
#include "DataFormats/L1Trigger/interface/Jet.h"
#include "DataFormats/L1Trigger/interface/Tau.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "L1Trigger/L1TCalorimeter/interface/CaloParamsHelper.h"

#include "L1TDisplay.h"

namespace {
  using namespace l1tdisplay;

  // one tower of the 82 x 72 grid at the three steps (GeV)
  struct Cell {
    float ecalTP = 0, hcalTP = 0;              // the primitives (HF: the primitive of the 2 or 4 towers, in its first tower)
    float l1 = 0, l1Em = 0, l1Had = 0;         // the Layer-1 tower and its two calibrated inputs
    int ecalRaw = 0, hcalRaw = 0, l1Raw = 0;   // the integers
    int l1Ratio = 0, l1Flags = 0;              // the E/H ratio code and the flag bits of the Layer-1 tower word
    int hcalWidth = 0;                         // phi width of the HCAL primitive that starts here (0: none)
    bool ecalFG = false;
  };
  struct Grid {
    std::array<std::array<Cell, kNPhi + 1>, 2 * kTowersPerSide> cells;  // [column + 40][iphi]
    Cell& at(int c, int iphi) { return cells[c + kTowersPerSide][wrapPhi(iphi)]; }
    const Cell& at(int c, int iphi) const { return cells[c + kTowersPerSide][wrapPhi(iphi)]; }
  };
  // the pileup estimate of a jet seeded at (column cj, iphi pj), in Layer-1 counts (0.5 GeV), as Layer-2 does it
  // (Stage2Layer2JetAlgorithmFirmwareImp1): the phi rings or the chunky donut
  struct Pileup {
    bool phiRing = false;
    int ring[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};  // phi ring: the sum over all 72 towers in phi of each of the 9 eta rings
    int ringSum = 0;                            //           their sum; pileup = ringSum * 9 / 72
    int sum[4] = {0, 0, 0, 0};                  // donut: the four 3 x 9 strips: phi up, phi down, eta up, eta down
    int dropped = 0;                            //        the strip with the largest sum, not used
    int pu = 0;                                 // the estimate
  };
  Pileup pileupOf(const Grid& g, int cj, int pj, bool phiRing) {
    Pileup d;
    d.phiRing = phiRing;
    if (phiRing) {
      for (int i = 0; i < 9; ++i) {
        int c = cj - 4 + i;
        if (!validColumn(c))
          continue;
        for (int p = 1; p <= kNPhi; ++p)
          d.ring[i] += g.at(c, p).l1Raw;
        d.ringSum += d.ring[i];
      }
      d.pu = d.ringSum * 9 / kNPhi;
      return d;
    }
    for (int s = 0; s < 3; ++s) {
      for (int c = cj - 4; c <= cj + 4; ++c) {
        if (!validColumn(c))
          continue;
        d.sum[0] += g.at(c, pj + 5 + s).l1Raw;
        d.sum[1] += g.at(c, pj - 5 - s).l1Raw;
      }
      for (int p = pj - 4; p <= pj + 4; ++p) {
        if (validColumn(cj + 5 + s))
          d.sum[2] += g.at(cj + 5 + s, p).l1Raw;
        if (validColumn(cj - 5 - s))
          d.sum[3] += g.at(cj - 5 - s, p).l1Raw;
      }
    }
    d.dropped = int(std::max_element(d.sum, d.sum + 4) - d.sum);
    for (int i = 0; i < 4; ++i)
      if (i != d.dropped)
        d.pu += d.sum[i];
    return d;
  }
  const char* stripName(int i) {
    static const char* n[] = {"phi+", "phi-", "eta+", "eta-"};
    return n[i];
  }
}  // namespace

class L1TCaloAnalyzer : public edm::one::EDAnalyzer<> {
public:
  explicit L1TCaloAnalyzer(const edm::ParameterSet&);
  void analyze(const edm::Event&, const edm::EventSetup&) override;

private:
  void drawMap(const Grid& g, int what, const std::string& title, const std::string& ztitle);
  void drawGenJets(const std::vector<reco::GenJet>& gj, bool labels);
  void drawZoom(const Grid& g, int what, const l1t::Jet& jet, int ijet, const Pileup& d, const std::string& title);

  edm::EDGetTokenT<EcalTrigPrimDigiCollection> ecalTPs_;
  edm::EDGetTokenT<HcalTrigPrimDigiCollection> hcalTPs_;
  edm::EDGetTokenT<BXVector<l1t::CaloTower>> towers_;
  edm::EDGetTokenT<BXVector<l1t::Jet>> jets_;
  edm::EDGetTokenT<BXVector<l1t::EGamma>> egammas_;
  edm::EDGetTokenT<BXVector<l1t::Tau>> taus_;
  edm::EDGetTokenT<BXVector<l1t::EtSum>> sums_;
  edm::EDGetTokenT<std::vector<reco::GenJet>> genJets_;
  edm::ESGetToken<CaloTPGTranscoder, CaloTPGRecord> decoder_;
  edm::ESGetToken<l1t::CaloParams, L1TCaloParamsRcd> params_;
  std::string pdfFile_, pngPrefix_, sampleLabel_;
  bool savePNG_;
  double jetPtMin_, zMax_;
  int zoomJets_, nEvents_ = 0;
  Page page_;
  std::string tag_;  // _run<R>_event<E>
};

L1TCaloAnalyzer::L1TCaloAnalyzer(const edm::ParameterSet& ps)
    : ecalTPs_(consumes(ps.getParameter<edm::InputTag>("ecalTPs"))),
      hcalTPs_(consumes(ps.getParameter<edm::InputTag>("hcalTPs"))),
      towers_(consumes(ps.getParameter<edm::InputTag>("layer1Towers"))),
      jets_(consumes(ps.getParameter<edm::InputTag>("jets"))),
      egammas_(consumes(ps.getParameter<edm::InputTag>("egammas"))),
      taus_(consumes(ps.getParameter<edm::InputTag>("taus"))),
      sums_(consumes(ps.getParameter<edm::InputTag>("etSums"))),
      genJets_(consumes(ps.getParameter<edm::InputTag>("genJets"))),
      decoder_(esConsumes()),
      params_(esConsumes()),
      pdfFile_(ps.getParameter<std::string>("pdfFile")),
      pngPrefix_(ps.getParameter<std::string>("pngPrefix")),
      sampleLabel_(ps.getParameter<std::string>("sampleLabel")),
      savePNG_(ps.getParameter<bool>("savePNG")),
      jetPtMin_(ps.getParameter<double>("jetPtMin")),
      zMax_(ps.getParameter<double>("zMax")),
      zoomJets_(ps.getParameter<int>("zoomJets")),
      page_("l1tCalo") {}

void L1TCaloAnalyzer::analyze(const edm::Event& ev, const edm::EventSetup& es) {
  const CaloTPGTranscoder& decoder = es.getData(decoder_);
  l1t::CaloParamsHelper par(es.getData(params_));
  tag_ = Form("_run%u_event%llu", ev.id().run(), ev.id().event());
  std::cout << "== L1TCaloAnalyzer run:lumi:event " << ev.id().run() << ":" << ev.luminosityBlock() << ":" << ev.id().event() << "\n";

  // ------------------------------------------------------------------ the parameters of the calorimeter trigger
  std::cout << "  caloParams: towerLsbSum=" << par.towerLsbSum() << " towerLsbE=" << par.towerLsbE() << " towerLsbH=" << par.towerLsbH()
            << " towerNBitsSum=" << par.towerNBitsSum() << " jetLsb=" << par.jetLsb() << " jetSeedThreshold=" << par.jetSeedThreshold()
            << " jetNeighbourThreshold=" << par.jetNeighbourThreshold() << " jetBypassPUS=" << par.jetBypassPUS()
            << " jetPUSUsePhiRing=" << par.jetPUSUsePhiRing() << " jetPUSType=" << par.jetPUSType()
            << " jetCalibrationType=" << par.jetCalibrationType() << " egLsb=" << par.egLsb() << " egSeedThreshold=" << par.egSeedThreshold()
            << " egNeighbourThreshold=" << par.egNeighbourThreshold() << " egHcalThreshold=" << par.egHcalThreshold()
            << " tauSeedThreshold=" << par.tauSeedThreshold() << " tauNeighbourThreshold=" << par.tauNeighbourThreshold()
            << " etSumLsb=" << par.etSumLsb() << "\n";
  std::cout << "  caloParams etSum (index: etaMin etaMax etThreshold):";
  for (unsigned i = 0; i < 5; ++i)
    std::cout << " " << i << ": " << par.etSumEtaMin(i) << " " << par.etSumEtaMax(i) << " " << par.etSumEtThreshold(i) << ";";
  std::cout << "\n";
  auto printBins = [](const char* name, const auto& v) {
    std::cout << "  caloParams " << name << " (" << v.size() << "):";
    for (auto x : v)
      std::cout << " " << x;
    std::cout << "\n";
  };
  printBins("layer1ECalScaleETBins", par.layer1ECalScaleETBins());
  printBins("layer1HCalScaleETBins", par.layer1HCalScaleETBins());
  printBins("layer1HFScaleETBins", par.layer1HFScaleETBins());
  // the Layer-1 scale factors: one per ET bin of the primitive and per tower ring (index etBin * nEta + ieta - first)
  auto printSF = [](const char* name, const std::vector<double>& sf, const auto& bins, unsigned nEta, int first) {
    if (bins.empty() || sf.size() < bins.size() * nEta)
      return;
    std::cout << "  caloParams " << name << " (" << sf.size() << " values, columns: |ieta| = " << first << ".." << first + int(nEta) - 1 << ")\n";
    for (size_t b = 0; b < bins.size(); ++b) {
      std::cout << "    ET < " << bins[b] << ":";
      for (unsigned e = 0; e < nEta; ++e)
        std::cout << " " << sf[b * nEta + e];
      std::cout << "\n";
    }
  };
  printSF("layer1ECalScaleFactors", par.layer1ECalScaleFactors(), par.layer1ECalScaleETBins(), 28, 1);
  printSF("layer1HCalScaleFactors", par.layer1HCalScaleFactors(), par.layer1HCalScaleETBins(), 28, 1);
  printSF("layer1HFScaleFactors", par.layer1HFScaleFactors(), par.layer1HFScaleETBins(), 12, 30);
  const bool phiRing = !par.jetBypassPUS() && par.jetPUSUsePhiRing();
  std::cout << "  jet pileup estimate: " << (par.jetBypassPUS() ? "none (bypassed)" : phiRing ? "PhiRing1" : "ChunkyDonut") << "\n";

  // ------------------------------------------------------------------ step 1: the trigger primitives
  auto grid = std::make_unique<Grid>();
  Grid& g = *grid;
  struct Sum {
    int n = 0, nonzero = 0, saturated = 0, flagged = 0;
    double et = 0, max = 0;
    int maxIeta = 0, maxIphi = 0;
    void add(double e, int raw, bool flag, int ieta, int iphi) {
      ++n;
      if (raw > 0)
        ++nonzero;
      if (raw >= 255)
        ++saturated;
      if (flag && raw > 0)
        ++flagged;
      et += e;
      if (e > max) {
        max = e;
        maxIeta = ieta;
        maxIphi = iphi;
      }
    }
  };
  std::map<std::string, Sum> tp;
  for (auto const& t : ev.get(ecalTPs_)) {
    int ieta = t.id().ieta(), iphi = t.id().iphi(), raw = t.compressedEt();
    Cell& c = g.at(column(ieta), iphi);
    c.ecalRaw = raw;
    c.ecalTP = 0.5 * raw;  // the ECAL primitive is linear, 0.5 GeV per count
    c.ecalFG = t.fineGrain();
    tp[ecalRegion(ieta)].add(c.ecalTP, raw, t.fineGrain(), ieta, iphi);
    tp["ECAL"].add(c.ecalTP, raw, t.fineGrain(), ieta, iphi);
  }
  for (auto const& t : ev.get(hcalTPs_)) {
    int ieta = t.id().ieta(), iphi = t.id().iphi(), raw = t.SOI_compressedEt(), a = std::abs(ieta);
    if (a == 29 || a > 41 || (a >= 30 && t.id().version() == 0))
      continue;  // as Layer-1: tower 29 and the old 2x3 HF primitives are not used
    Cell& c = g.at(column(ieta), iphi);
    c.hcalRaw = raw;
    c.hcalTP = decoder.hcaletValue(t.id(), t.t0());  // the 8-bit scale decoded to GeV (linear, 0.5 GeV per count, since 2018)
    c.hcalWidth = a >= 40 ? 4 : a >= 30 ? 2 : 1;
    tp[hcalRegion(ieta)].add(c.hcalTP, raw, t.SOI_fineGrain(), ieta, iphi);
    tp["HCAL"].add(c.hcalTP, raw, t.SOI_fineGrain(), ieta, iphi);
  }
  for (const char* r : {"ECAL", "EB", "EE", "HCAL", "HB", "HE", "HF"}) {
    const Sum& s = tp[r];
    std::cout << "  TP " << r << ": n=" << s.n << " nonzero=" << s.nonzero << " sumEt=" << s.et << " maxEt=" << s.max << " maxIeta=" << s.maxIeta
              << " maxIphi=" << s.maxIphi << " saturated=" << s.saturated << " fineGrain=" << s.flagged << "\n";
  }

  // ------------------------------------------------------------------ step 2: the Layer-1 towers
  const auto& towers = ev.get(towers_);
  std::map<std::string, Sum> l1;
  double l1Em = 0, l1Had = 0;
  std::vector<std::array<int, 3>> top;  // hwPt, ieta, iphi
  for (auto it = towers.begin(0); it != towers.end(0); ++it) {
    int ieta = it->hwEta(), iphi = it->hwPhi();
    if (ieta == 0 || std::abs(ieta) == 29 || std::abs(ieta) > 41)
      continue;
    Cell& c = g.at(column(ieta), iphi);
    c.l1Raw = it->hwPt();
    c.l1 = 0.5 * it->hwPt();
    c.l1Em = 0.5 * it->hwEtEm();
    c.l1Had = 0.5 * it->hwEtHad();
    c.l1Ratio = it->hwEtRatio();
    c.l1Flags = it->hwQual();
    const char* region = std::abs(ieta) <= 17 ? "barrel" : std::abs(ieta) <= 28 ? "endcap" : "forward";
    l1[region].add(c.l1, it->hwPt(), false, ieta, iphi);
    l1["all"].add(c.l1, it->hwPt(), false, ieta, iphi);
    if (it->hwPt() >= 511)
      ++l1[region].saturated, ++l1["all"].saturated;
    l1Em += c.l1Em;
    l1Had += c.l1Had;
    if (it->hwPt() > 0)
      top.push_back({{it->hwPt(), ieta, iphi}});
  }
  for (const char* r : {"all", "barrel", "endcap", "forward"}) {
    const Sum& s = l1[r];
    std::cout << "  Layer1 " << r << ": n=" << s.n << " nonzero=" << s.nonzero << " sumEt=" << s.et << " maxEt=" << s.max << " maxIeta=" << s.maxIeta
              << " maxIphi=" << s.maxIphi << "\n";
  }
  std::cout << "  Layer1 inputs after calibration (barrel + endcap): sumEm=" << l1Em << " sumHad=" << l1Had << "\n";
  // ring by ring (all 72 towers in phi of one ieta): the primitives in, the towers out
  for (int c = -kTowersPerSide; c < kTowersPerSide; ++c) {
    double e = 0, h = 0, t = 0, em = 0, had = 0;
    for (int p = 1; p <= kNPhi; ++p) {
      const Cell& cell = g.at(c, p);
      e += cell.ecalTP;
      h += cell.hcalTP;
      t += cell.l1;
      em += cell.l1Em;
      had += cell.l1Had;
    }
    std::cout << "  ring ieta=" << ietaOfColumn(c) << ": ecalTP=" << e << " hcalTP=" << h << " layer1=" << t << " layer1Em=" << em << " layer1Had=" << had << "\n";
  }
  std::sort(top.begin(), top.end(), [](auto const& a, auto const& b) { return a[0] > b[0]; });
  for (size_t i = 0; i < std::min<size_t>(top.size(), 10); ++i) {
    const Cell& c = g.at(column(top[i][1]), top[i][2]);
    // the HF primitive is shared by the towers it covers: look for it at the start of its phi group
    float hf = 0;
    if (std::abs(top[i][1]) >= 30)
      for (int w = 0; w < 4; ++w) {
        const Cell& h = g.at(column(top[i][1]), top[i][2] - w);
        if (h.hcalWidth > w) {
          hf = h.hcalTP;
          break;
        }
      }
    std::cout << "  Layer1 top tower " << i << ": ieta=" << top[i][1] << " iphi=" << top[i][2] << " et=" << c.l1 << " ecalTP=" << c.ecalTP
              << " hcalTP=" << (std::abs(top[i][1]) >= 30 ? hf : c.hcalTP) << " calibratedEm=" << c.l1Em << " calibratedHad=" << c.l1Had << " | tower word: et=" << c.l1Raw << " ratio=" << c.l1Ratio << " flags=" << c.l1Flags << "\n";
  }

  // ------------------------------------------------------------------ step 3: the Layer-2 objects
  std::vector<reco::GenJet> gj;
  for (auto const& j : ev.get(genJets_))
    if (j.pt() > jetPtMin_)
      gj.push_back(j);
  for (auto const& j : gj)
    std::cout << "  GenJet: pt=" << j.pt() << " eta=" << j.eta() << " phi=" << j.phi() << " energy=" << j.energy() << "\n";
  const auto& jets = ev.get(jets_);
  std::vector<l1t::Jet> jv(jets.begin(0), jets.end(0));
  std::sort(jv.begin(), jv.end(), [](auto const& a, auto const& b) { return a.hwPt() > b.hwPt(); });
  std::vector<Pileup> donuts;
  for (size_t i = 0; i < jv.size(); ++i) {
    const l1t::Jet& j = jv[i];
    int cj = column(j.towerIEta()), pj = j.towerIPhi();
    Pileup d = pileupOf(g, cj, pj, phiRing);
    donuts.push_back(d);
    double wEcal = 0, wHcal = 0, wL1 = 0, wEm = 0, wHad = 0;
    for (int c = cj - 4; c <= cj + 4; ++c) {
      if (!validColumn(c))
        continue;
      for (int p = pj - 4; p <= pj + 4; ++p) {
        const Cell& cell = g.at(c, p);
        wEcal += cell.ecalTP;
        wL1 += cell.l1;
        wEm += cell.l1Em;
        wHad += cell.l1Had;
        if (std::abs(ietaOfColumn(c)) < 30)
          wHcal += cell.hcalTP;
      }
      // an HF primitive covers 2 or 4 towers: count the fraction of it inside the window
      if (std::abs(ietaOfColumn(c)) >= 30)
        for (int p = pj - 7; p <= pj + 4; ++p) {
          const Cell& cell = g.at(c, p);
          for (int w = 0; w < cell.hcalWidth; ++w)
            if (p + w >= pj - 4 && p + w <= pj + 4)
              wHcal += cell.hcalTP / cell.hcalWidth;
        }
    }
    double best = 99, genPt = 0;
    for (auto const& x : gj) {
      double dr = reco::deltaR(x.eta(), x.phi(), j.eta(), j.phi());
      if (dr < best) {
        best = dr;
        genPt = x.pt();
      }
    }
    int sub = j.rawEt() - j.puEt();
    std::cout << "  L1Jet " << i << ": et=" << j.pt() << " eta=" << j.eta() << " phi=" << j.phi() << " ieta=" << j.towerIEta() << " iphi=" << j.towerIPhi()
              << " seedEt=" << 0.5 * j.seedEt() << " rawEt=" << 0.5 * j.rawEt() << " puEt=" << 0.5 * j.puEt() << " rawMinusPu=" << 0.5 * sub
              << " calibration=" << (sub > 0 ? double(j.hwPt()) / sub : 0.) << " hwQual=" << j.hwQual() << " | window: ecalTP=" << wEcal << " hcalTP="
              << wHcal << " layer1=" << wL1 << " layer1Em=" << wEm << " layer1Had=" << wHad;
    if (d.phiRing) {
      std::cout << " | phiRing: ringSum=" << 0.5 * d.ringSum << " rings:";
      for (int s = 0; s < 9; ++s)
        std::cout << " " << 0.5 * d.ring[s];
    } else {
      std::cout << " | donut:";
      for (int s = 0; s < 4; ++s)
        std::cout << " " << stripName(s) << "=" << 0.5 * d.sum[s] << (s == d.dropped ? "(dropped)" : "");
    }
    std::cout << " recomputedPu=" << 0.5 * d.pu << " | genJet: dR=" << best << " pt=" << (best < 0.4 ? genPt : 0.) << "\n";
  }
  const auto& egs = ev.get(egammas_);
  int n = 0;
  for (auto it = egs.begin(0); it != egs.end(0); ++it, ++n)
    std::cout << "  L1EG " << n << ": et=" << it->pt() << " eta=" << it->eta() << " phi=" << it->phi() << " iso=" << it->hwIso() << " hwQual=" << it->hwQual()
              << "\n";
  const auto& taus = ev.get(taus_);
  n = 0;
  for (auto it = taus.begin(0); it != taus.end(0); ++it, ++n)
    std::cout << "  L1Tau " << n << ": et=" << it->pt() << " eta=" << it->eta() << " phi=" << it->phi() << " iso=" << it->hwIso() << "\n";
  std::map<int, const l1t::EtSum*> sum;
  for (auto it = ev.get(sums_).begin(0); it != ev.get(sums_).end(0); ++it)
    sum[it->getType()] = &*it;
  auto sumEt = [&](int type) { return sum.count(type) ? sum[type]->pt() : 0.; };
  auto sumPhi = [&](int type) { return sum.count(type) ? sum[type]->phi() : 0.; };
  auto sumHw = [&](int type) { return sum.count(type) ? sum[type]->hwPt() : 0; };
  std::cout << "  L1Sums: ETT=" << sumEt(l1t::EtSum::kTotalEt) << " ETTEM=" << sumEt(l1t::EtSum::kTotalEtEm) << " HTT=" << sumEt(l1t::EtSum::kTotalHt)
            << " ETM=" << sumEt(l1t::EtSum::kMissingEt) << " ETMphi=" << sumPhi(l1t::EtSum::kMissingEt) << " ETMHF=" << sumEt(l1t::EtSum::kMissingEtHF)
            << " ETMHFphi=" << sumPhi(l1t::EtSum::kMissingEtHF) << " HTM=" << sumEt(l1t::EtSum::kMissingHt) << " HTMphi=" << sumPhi(l1t::EtSum::kMissingHt)
            << " HTMHF=" << sumEt(l1t::EtSum::kMissingHtHF) << " towerCount=" << sumHw(l1t::EtSum::kTowerCount)
            << " minBiasHFP0=" << sumHw(l1t::EtSum::kMinBiasHFP0) << " minBiasHFM0=" << sumHw(l1t::EtSum::kMinBiasHFM0)
            << " minBiasHFP1=" << sumHw(l1t::EtSum::kMinBiasHFP1) << " minBiasHFM1=" << sumHw(l1t::EtSum::kMinBiasHFM1) << "\n";
  std::cout << "  counts: L1Jets=" << jv.size() << " L1EGs=" << egs.size(0) << " L1Taus=" << taus.size(0) << " genJets(pt>" << jetPtMin_ << ")=" << gj.size()
            << "\n";

  // ------------------------------------------------------------------ the pages
  page_.openPdf(pdfName(pdfFile_, ev.id().run(), ev.id().event(), nEvents_++));
  auto png = [&](const char* name) { return savePNG_ ? pngPrefix_ + "_" + name + tag_ + ".png" : std::string(); };
  auto regionLine = [&](const char* r, const Sum& s) {
    page_.text(Form("%s: %d of %d, #Sigma E_{T} %.1f GeV", r, s.nonzero, s.n, s.et), 0.0205);
  };

  // page 1: the ECAL trigger primitives
  drawMap(g, 0, Form("step 1: ECAL trigger primitives, run %u event %llu;#eta;#phi", ev.id().run(), ev.id().event()), "E_{T} of the trigger tower [GeV]");
  drawGenJets(gj, true);
  page_.label(sampleLabel_);
  page_.text("ECAL trigger primitives", 0.026, kGreen + 3);
  page_.text("one E_{T} per tower (5 x 5 crystals in EB)", 0.021);
  page_.text("8 bits, 0.5 GeV per count: max 127.5 GeV", 0.021);
  page_.skip();
  regionLine("EB, |i#eta| #leq 17", tp["EB"]);
  regionLine("EE, |i#eta| 18-28", tp["EE"]);
  regionLine("all", tp["ECAL"]);
  page_.text(Form("largest: %.1f GeV at i#eta %d, i#phi %d", tp["ECAL"].max, tp["ECAL"].maxIeta, tp["ECAL"].maxIphi), 0.0205);
  page_.text(Form("saturated (255 counts): %d", tp["ECAL"].saturated), 0.0205);
  page_.skip();
  page_.text("no ECAL beyond |#eta| = 3 (dotted line)", 0.02, kGray + 2);
  if (!gj.empty())
    page_.text(Form("circles: ak4GenJets, p_{T} > %.0f GeV", jetPtMin_), 0.02, kMagenta + 1);
  page_.print(png("ecaltp"));

  // page 2: the HCAL trigger primitives
  drawMap(g, 1, Form("step 1: HCAL trigger primitives, run %u event %llu;#eta;#phi", ev.id().run(), ev.id().event()), "E_{T} of the trigger tower [GeV]");
  drawGenJets(gj, true);
  page_.label(sampleLabel_);
  page_.text("HCAL trigger primitives", 0.026, kRed + 1);
  page_.text("the depths of a tower summed to one E_{T}", 0.021);
  page_.text("8 bits, 0.5 GeV per count: max 127.5 GeV", 0.021);
  page_.skip();
  regionLine("HB, |i#eta| #leq 16", tp["HB"]);
  regionLine("HE, |i#eta| 17-28", tp["HE"]);
  regionLine("HF, |i#eta| 30-41", tp["HF"]);
  regionLine("all", tp["HCAL"]);
  page_.text(Form("largest: %.1f GeV at i#eta %d, i#phi %d", tp["HCAL"].max, tp["HCAL"].maxIeta, tp["HCAL"].maxIphi), 0.0205);
  page_.text(Form("saturated (255 counts): %d", tp["HCAL"].saturated), 0.0205);
  page_.skip();
  page_.text("HF: 10#circ wide in #phi (20#circ for |i#eta| #geq 40)", 0.02, kGray + 2);
  if (!gj.empty())
    page_.text(Form("circles: ak4GenJets, p_{T} > %.0f GeV", jetPtMin_), 0.02, kMagenta + 1);
  page_.print(png("hcaltp"));

  // page 3: the Layer-1 towers
  drawMap(g, 2, Form("step 2: Layer-1 towers, run %u event %llu;#eta;#phi", ev.id().run(), ev.id().event()), "E_{T} of the tower [GeV]");
  drawGenJets(gj, true);
  page_.label(sampleLabel_);
  page_.text("calorimeter Layer-1", 0.026, kBlue + 1);
  page_.text("ECAL + HCAL of a tower: calibrated, summed", 0.021);
  page_.text("9 bits, 0.5 GeV per count: max 255.5 GeV", 0.021);
  page_.skip();
  regionLine("barrel, |i#eta| #leq 17", l1["barrel"]);
  regionLine("endcap, |i#eta| 18-28", l1["endcap"]);
  regionLine("forward, |i#eta| 30-41", l1["forward"]);
  regionLine("all", l1["all"]);
  page_.skip();
  page_.text("what the calibration did (#Sigma E_{T}, GeV):", 0.021, kBlue + 1);
  page_.text(Form("ECAL  %.1f #rightarrow %.1f", tp["ECAL"].et, l1Em), 0.0205);
  page_.text(Form("HB + HE  %.1f #rightarrow %.1f", tp["HB"].et + tp["HE"].et, l1Had), 0.0205);
  page_.text(Form("HF  %.1f #rightarrow %.1f", tp["HF"].et, l1["forward"].et), 0.0205);
  page_.text(Form("largest tower: %.1f GeV at i#eta %d, i#phi %d", l1["all"].max, l1["all"].maxIeta, l1["all"].maxIphi), 0.0205);
  if (!gj.empty())
    page_.text(Form("circles: ak4GenJets, p_{T} > %.0f GeV", jetPtMin_), 0.02, kMagenta + 1);
  page_.print(png("layer1"));

  // page 4: the Layer-2 jets on the Layer-1 towers
  drawMap(g, 2, Form("step 3: Layer-2 jets on the Layer-1 towers, run %u event %llu;#eta;#phi", ev.id().run(), ev.id().event()), "E_{T} of the tower [GeV]");
  drawGenJets(gj, false);
  for (size_t i = 0; i < jv.size(); ++i) {
    int cj = column(jv[i].towerIEta()), pj = jv[i].towerIPhi();
    if (int(i) < zoomJets_ && donuts[i].phiRing) {  // the phi rings of the leading jets: the 9 eta rings, all around
      page_.line(etaLow(std::max(cj - 4, -kTowersPerSide)), -M_PI, etaLow(std::max(cj - 4, -kTowersPerSide)), M_PI, kAzure + 1, 2);
      page_.line(etaHigh(std::min(cj + 4, kTowersPerSide - 1)), -M_PI, etaHigh(std::min(cj + 4, kTowersPerSide - 1)), M_PI, kAzure + 1, 2);
    } else if (int(i) < zoomJets_ && !par.jetBypassPUS()) {  // the donut: the three strips used in blue, the dropped one dashed
      const Pileup& d = donuts[i];
      for (int s = 0; s < 4; ++s) {
        int color = s == d.dropped ? kGray + 2 : kAzure + 1, style = s == d.dropped ? 2 : 1;
        if (s == 0)
          page_.towerBox(cj - 4, cj + 4, pj + 5, 3, color, 1, style);
        else if (s == 1)
          page_.towerBox(cj - 4, cj + 4, pj - 7, 3, color, 1, style);
        else if (s == 2)
          page_.towerBox(cj + 5, cj + 7, pj - 4, 9, color, 1, style);
        else
          page_.towerBox(cj - 7, cj - 5, pj - 4, 9, color, 1, style);
      }
    }
    page_.towerBox(cj - 4, cj + 4, pj - 4, 9, kRed, int(i) < zoomJets_ ? 3 : 2);
    double x = std::min(std::max(etaLow(std::max(cj - 4, -kTowersPerSide)), -5.1), 4.2);
    double y = towerPhi(pj + 4) + 0.12;
    if (y > 3.0)
      y = towerPhi(pj - 4) - 0.12;
    page_.note(x, y, Form("%.1f", jv[i].pt()), kRed + 1, 0.022);
  }
  page_.label(sampleLabel_);
  page_.text("calorimeter Layer-2: jets", 0.026, kRed);
  page_.text(Form("seed #geq %.0f GeV; 9 x 9 towers around it;", par.jetSeedThreshold()), 0.021);
  page_.text(phiRing ? "pileup: the #phi rings of the window x 9/72;" : "pileup: the 3 quietest donut strips;", 0.021, kAzure + 1);
  page_.text("then the calibration (a lookup table)", 0.021);
  page_.skip();
  page_.text(Form("%zu jets (red windows, E_{T} in GeV)", jv.size()), 0.022, kRed);
  page_.text("   ET   eta   phi  window pileup calib", 0.0185, kGray + 2, 82);
  for (size_t i = 0; i < std::min<size_t>(jv.size(), 12); ++i) {
    int sub = jv[i].rawEt() - jv[i].puEt();
    page_.text(Form("%5.1f %5.2f %5.2f %6.1f %6.1f %5.2f", jv[i].pt(), jv[i].eta(), jv[i].phi(), 0.5 * jv[i].rawEt(), 0.5 * jv[i].puEt(),
                    sub > 0 ? double(jv[i].hwPt()) / sub : 0.),
               0.0185, kBlack, 82);
  }
  page_.skip();
  page_.text(Form("E_{T}^{tot} %.1f GeV, H_{T} %.1f GeV", sumEt(l1t::EtSum::kTotalEt), sumEt(l1t::EtSum::kTotalHt)), 0.021);
  page_.text(Form("E_{T}^{miss} %.1f (with HF %.1f), H_{T}^{miss} %.1f GeV", sumEt(l1t::EtSum::kMissingEt), sumEt(l1t::EtSum::kMissingEtHF),
                  sumEt(l1t::EtSum::kMissingHt)),
             0.021);
  page_.text(Form("e/#gamma candidates: %u, #tau candidates: %u", egs.size(0), taus.size(0)), 0.021);
  page_.skip();
  if (phiRing)
    page_.text("blue lines: the #phi rings of the two leading jets", 0.02, kAzure + 1);
  else {
    page_.text("blue: the donut strips used for the pileup", 0.02, kAzure + 1);
    page_.text("grey dashed: the strip dropped (the largest)", 0.02, kGray + 2);
  }
  if (!gj.empty())
    page_.text(Form("circles: ak4GenJets, p_{T} > %.0f GeV", jetPtMin_), 0.02, kMagenta + 1);
  page_.print(png("layer2"));

  // the zoom pages: the leading jets as the firmware sees them
  for (int i = 0; i < std::min<int>(zoomJets_, jv.size()); ++i) {
    const char* names[] = {"ecaltp", "hcaltp", "layer1"};
    const char* titles[] = {"ECAL trigger primitives", "HCAL trigger primitives", "Layer-1 towers"};
    for (int what = 0; what < 3; ++what) {
      drawZoom(g, what, jv[i], i, donuts[i],
               Form("L1 jet %d (%.1f GeV): %s around the seed, E_{T} in GeV;i#eta of the tower;i#phi of the tower", i + 1, jv[i].pt(), titles[what]));
      const l1t::Jet& j = jv[i];
      const Pileup& d = donuts[i];
      int sub = j.rawEt() - j.puEt();
      page_.label(sampleLabel_);
      page_.text(Form("L1 jet %d: %s", i + 1, titles[what]), 0.025, what == 0 ? kGreen + 3 : what == 1 ? kRed + 1 : kBlue + 1);
      page_.text(Form("seed tower: i#eta %d, i#phi %d (black square)", j.towerIEta(), j.towerIPhi()), 0.021);
      page_.skip();
      page_.text("the arithmetic of Layer-2 (GeV):", 0.022);
      page_.text(Form("seed tower              %6.1f", 0.5 * j.seedEt()), 0.02, kBlack, 82);
      page_.text(Form("9 x 9 window (red)      %6.1f", 0.5 * j.rawEt()), 0.02, kRed, 82);
      if (d.phiRing) {
        page_.text(Form("9 phi rings (blue)      %6.1f", 0.5 * d.ringSum), 0.02, kAzure + 1, 82);
        page_.text(Form("pileup = rings x 9/72   %6.1f", 0.5 * j.puEt()), 0.02, kAzure + 1, 82);
      } else {
        for (int s = 0; s < 4; ++s)
          page_.text(Form("strip %s (3 x 9)      %6.1f%s", stripName(s), 0.5 * d.sum[s], s == d.dropped ? " dropped" : ""), 0.02,
                     s == d.dropped ? kGray + 2 : kAzure + 1, 82);
        page_.text(Form("pileup = 3 quietest     %6.1f", 0.5 * j.puEt()), 0.02, kAzure + 1, 82);
      }
      page_.text(Form("window - pileup         %6.1f", 0.5 * sub), 0.02, kBlack, 82);
      page_.text(Form("x calibration %4.2f      %6.1f", sub > 0 ? double(j.hwPt()) / sub : 0., j.pt()), 0.02, kBlack, 82);
      page_.skip();
      double best = 99, genPt = 0;
      for (auto const& x : gj) {
        double dr = reco::deltaR(x.eta(), x.phi(), j.eta(), j.phi());
        if (dr < best) {
          best = dr;
          genPt = x.pt();
        }
      }
      page_.text(Form("L1 jet: E_{T} %.1f GeV, #eta %.2f, #phi %.2f", j.pt(), j.eta(), j.phi()), 0.021);
      if (best < 0.4)
        page_.text(Form("GenJet (#DeltaR %.2f): p_{T} %.1f GeV", best, genPt), 0.021, kMagenta + 1);
      page_.skip();
      if (what == 1 && std::abs(j.towerIEta()) + 7 >= 30)
        page_.text("an HF primitive spans 2 (4) towers in #phi (frames)", 0.019, kGray + 2);
      if (what == 2 && std::abs(j.towerIEta()) + 7 >= 30)
        page_.text("Layer-1 shares an HF primitive between its towers", 0.019, kGray + 2);
      if (d.phiRing)
        page_.text("blue: the 9 #eta rings go all around in #phi (72 towers)", 0.019, kAzure + 1);
      page_.text("i#eta 29 does not exist in the trigger", 0.019, kGray + 2);
      page_.print(png(Form("jet%d_%s", i + 1, names[what])));
    }
  }
  page_.closePdf();
}

// one of the three tower maps in eta-phi: what = 0 ECAL primitives, 1 HCAL primitives, 2 Layer-1 towers
void L1TCaloAnalyzer::drawMap(const Grid& g, int what, const std::string& title, const std::string& ztitle) {
  page_.begin(true);
  TH2F* h = page_.keep(towerMap(Form("l1tCaloMap%d", what), title.c_str()));
  for (int c = -kTowersPerSide; c < kTowersPerSide; ++c)
    for (int p = 1; p <= kNPhi; ++p) {
      const Cell& cell = g.at(c, p);
      int ieta = ietaOfColumn(c);
      if (what == 0 && cell.ecalRaw > 0)
        fillTower(h, ieta, p, cell.ecalTP);
      else if (what == 1 && cell.hcalRaw > 0)
        fillTower(h, ieta, p, std::max(cell.hcalTP, 0.26f), cell.hcalWidth);
      else if (what == 2 && cell.l1Raw > 0)
        fillTower(h, ieta, p, cell.l1);
    }
  h->SetMinimum(0.25);
  h->SetMaximum(zMax_);
  h->GetZaxis()->SetTitle(ztitle.c_str());
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

void L1TCaloAnalyzer::drawGenJets(const std::vector<reco::GenJet>& gj, bool labels) {
  for (auto const& j : gj) {
    page_.circle(j.eta(), j.phi(), 0.4, kMagenta + 1, 2);
    if (labels)
      page_.note(j.eta(), j.phi() + (j.phi() > 2.5 ? -0.55 : 0.55), Form("GenJet %.0f GeV", j.pt()), kMagenta + 1, 0.022, 22);
  }
}

// 17 x 17 towers around the seed of a jet, one square per tower with its transverse energy written in it
void L1TCaloAnalyzer::drawZoom(const Grid& g, int what, const l1t::Jet& jet, int ijet, const Pileup& d, const std::string& title) {
  page_.begin(true);
  const int half = 8, nb = 2 * half + 1;
  int cj = column(jet.towerIEta()), pj = jet.towerIPhi();
  TH2F* h = page_.keep(new TH2F(Form("l1tCaloZoom%d_%d", ijet, what), title.c_str(), nb, -half - 0.5, half + 0.5, nb, -half - 0.5, half + 0.5));
  h->SetDirectory(nullptr);
  h->SetStats(false);
  for (int dx = -half; dx <= half; ++dx) {
    int c = cj + dx;
    h->GetXaxis()->SetBinLabel(dx + half + 1, validColumn(c) ? Form("%d", ietaOfColumn(c)) : "");
    h->GetYaxis()->SetBinLabel(dx + half + 1, Form("%d", wrapPhi(pj + dx)));
    if (!validColumn(c))
      continue;
    for (int dy = -half; dy <= half; ++dy) {
      const Cell& cell = g.at(c, pj + dy);
      double v = what == 0 ? cell.ecalTP : what == 1 ? cell.hcalTP : cell.l1;
      int raw = what == 0 ? cell.ecalRaw : what == 1 ? cell.hcalRaw : cell.l1Raw;
      if (raw > 0)
        h->SetBinContent(dx + half + 1, dy + half + 1, std::max(v, 0.26));
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
  // the tower grid
  for (int i = -half; i <= half + 1; ++i) {
    page_.line(i - 0.5, -half - 0.5, i - 0.5, half + 0.5, kGray, 1);
    page_.line(-half - 0.5, i - 0.5, half + 0.5, i - 0.5, kGray, 1);
  }
  // an HF primitive covers 2 or 4 towers in phi: frame them
  if (what == 1)
    for (int dx = -half; dx <= half; ++dx) {
      int c = cj + dx;
      if (!validColumn(c) || std::abs(ietaOfColumn(c)) < 30)
        continue;
      for (int dy = -half - 3; dy <= half; ++dy) {
        const Cell& cell = g.at(c, pj + dy);
        double y1 = std::max(dy - 0.45, -half - 0.5), y2 = std::min(dy + cell.hcalWidth - 0.55, half + 0.5);
        if (cell.hcalWidth > 1 && cell.hcalRaw > 0 && y2 > y1)
          page_.box(dx - 0.45, y1, dx + 0.45, y2, kGray + 2, 1);
      }
    }
  // the towers of the pileup estimate (the phi rings: the 9 columns over the full height, and beyond; or the donut
  // strips), the window, the seed
  if (d.phiRing) {
    page_.line(-4.5, -half - 0.5, -4.5, half + 0.5, kAzure + 1, 3);
    page_.line(4.5, -half - 0.5, 4.5, half + 0.5, kAzure + 1, 3);
  }
  for (int s = 0; s < 4 && !d.phiRing; ++s) {
    int color = s == d.dropped ? kGray + 2 : kAzure + 1, style = s == d.dropped ? 2 : 1;
    if (s == 0)
      page_.box(-4.5, 4.5, 4.5, 7.5, color, 3, style);
    else if (s == 1)
      page_.box(-4.5, -7.5, 4.5, -4.5, color, 3, style);
    else if (s == 2 && validColumn(cj + 5))
      page_.box(4.5, -4.5, 7.5, 4.5, color, 3, style);
    else if (s == 3 && validColumn(cj - 5))
      page_.box(-7.5, -4.5, -4.5, 4.5, color, 3, style);
  }
  page_.box(-4.5, -4.5, 4.5, 4.5, kRed, 4);
  page_.box(-0.5, -0.5, 0.5, 0.5, kBlack, 3);
}

DEFINE_FWK_MODULE(L1TCaloAnalyzer);
