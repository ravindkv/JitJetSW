// L1TGlobalAnalyzer: the decision of the Level-1 global trigger for one event (chapter 12, L1T, of the JitJet book).
//
//   step 6  the global trigger (simGtStage2Digis) receives the objects of calorimeter Layer-2 (up to 12 jets, 12
//           e/gamma, 12 taus, the energy sums) and of the global muon trigger (up to 8 muons) and evaluates every
//           algorithm of the menu (L1TUtmTriggerMenu of the global tag) on them, in parallel; the event is kept if
//           at least one algorithm fires after prescales and masks (the FinalOR)
// Pages (one PDF per event, one PNG per page with savePNG):
//   objects  the event as the global trigger sees it: a list of objects with (ET, eta, phi), drawn in eta-phi
//   ladders  for the main families of single-object algorithms, the thresholds of the menu and where the event
//            stands with respect to them (the margin by which a seed fires or fails)
// The log lists the menu, the decision, every algorithm that fired with its logical expression and the objects that
// satisfied each of its conditions (from the GlobalObjectMapRecord of the emulator), and the decision of every
// algorithm (read by test/common/l1tLedger.py).
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include "CondFormats/DataRecord/interface/L1TUtmTriggerMenuRcd.h"
#include "CondFormats/L1TObjects/interface/L1TUtmTriggerMenu.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "DataFormats/L1TGlobal/interface/GlobalAlgBlk.h"
#include "DataFormats/L1TGlobal/interface/GlobalObject.h"
#include "DataFormats/L1TGlobal/interface/GlobalObjectMap.h"
#include "DataFormats/L1TGlobal/interface/GlobalObjectMapRecord.h"
#include "DataFormats/L1Trigger/interface/BXVector.h"
#include "DataFormats/L1Trigger/interface/EGamma.h"
#include "DataFormats/L1Trigger/interface/EtSum.h"
#include "DataFormats/L1Trigger/interface/Jet.h"
#include "DataFormats/L1Trigger/interface/Muon.h"
#include "DataFormats/L1Trigger/interface/Tau.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "L1TDisplay.h"

namespace {
  using namespace l1tdisplay;

  // a family of single-threshold algorithms: the names that match the pattern, the threshold being the first group
  struct Family {
    const char* name;
    const char* pattern;
    const char* what;  // the quantity of the event compared with the threshold
    double value = 0;
    std::vector<std::pair<double, bool>> thresholds;  // threshold, fired
  };
}  // namespace

class L1TGlobalAnalyzer : public edm::one::EDAnalyzer<> {
public:
  explicit L1TGlobalAnalyzer(const edm::ParameterSet&);
  void analyze(const edm::Event&, const edm::EventSetup&) override;

private:
  edm::EDGetTokenT<BXVector<GlobalAlgBlk>> algBlk_;
  edm::EDGetTokenT<GlobalObjectMapRecord> objectMap_;
  edm::EDGetTokenT<BXVector<l1t::Jet>> jets_;
  edm::EDGetTokenT<BXVector<l1t::EGamma>> egammas_;
  edm::EDGetTokenT<BXVector<l1t::Tau>> taus_;
  edm::EDGetTokenT<BXVector<l1t::EtSum>> sums_;
  edm::EDGetTokenT<BXVector<l1t::Muon>> muons_;
  edm::EDGetTokenT<std::vector<reco::GenJet>> genJets_;
  edm::EDGetTokenT<BXVector<float>> cicada_;
  edm::ESGetToken<L1TUtmTriggerMenu, L1TUtmTriggerMenuRcd> menu_;
  std::string pdfFile_, pngPrefix_, sampleLabel_;
  bool savePNG_;
  double jetPtMin_, etaMax_;
  int nEvents_ = 0;
  Page page_;
};

L1TGlobalAnalyzer::L1TGlobalAnalyzer(const edm::ParameterSet& ps)
    : algBlk_(consumes(ps.getParameter<edm::InputTag>("algBlk"))),
      objectMap_(consumes(ps.getParameter<edm::InputTag>("algBlk"))),
      jets_(consumes(ps.getParameter<edm::InputTag>("jets"))),
      egammas_(consumes(ps.getParameter<edm::InputTag>("egammas"))),
      taus_(consumes(ps.getParameter<edm::InputTag>("taus"))),
      sums_(consumes(ps.getParameter<edm::InputTag>("etSums"))),
      muons_(consumes(ps.getParameter<edm::InputTag>("muons"))),
      genJets_(consumes(ps.getParameter<edm::InputTag>("genJets"))),
      cicada_(consumes(ps.getParameter<edm::InputTag>("cicadaScore"))),
      menu_(esConsumes()),
      pdfFile_(ps.getParameter<std::string>("pdfFile")),
      pngPrefix_(ps.getParameter<std::string>("pngPrefix")),
      sampleLabel_(ps.getParameter<std::string>("sampleLabel")),
      savePNG_(ps.getParameter<bool>("savePNG")),
      jetPtMin_(ps.getParameter<double>("jetPtMin")),
      etaMax_(ps.getParameter<double>("etaMax")),
      page_("l1tGlobal") {}

void L1TGlobalAnalyzer::analyze(const edm::Event& ev, const edm::EventSetup& es) {
  const L1TUtmTriggerMenu& menu = es.getData(menu_);
  std::string tag = Form("_run%u_event%llu", ev.id().run(), ev.id().event());
  std::cout << "== L1TGlobalAnalyzer run:lumi:event " << ev.id().run() << ":" << ev.luminosityBlock() << ":" << ev.id().event() << "\n";

  // ------------------------------------------------------------------ the objects the global trigger receives (bx 0)
  const auto& jets = ev.get(jets_);
  const auto& egs = ev.get(egammas_);
  const auto& taus = ev.get(taus_);
  const auto& muons = ev.get(muons_);
  const auto& sums = ev.get(sums_);
  std::map<int, const l1t::EtSum*> sum;
  for (auto it = sums.begin(0); it != sums.end(0); ++it)
    sum[it->getType()] = &*it;
  auto sumEt = [&](int type) { return sum.count(type) ? sum[type]->pt() : 0.; };
  auto sumPhi = [&](int type) { return sum.count(type) ? sum[type]->phi() : 0.; };
  double ett = sumEt(l1t::EtSum::kTotalEt), htt = sumEt(l1t::EtSum::kTotalHt), etm = sumEt(l1t::EtSum::kMissingEt), etmhf = sumEt(l1t::EtSum::kMissingEtHF),
         htm = sumEt(l1t::EtSum::kMissingHt), ettem = sumEt(l1t::EtSum::kTotalEtEm);
  std::cout << "  uGT inputs (bx 0): jets=" << jets.size(0) << " egammas=" << egs.size(0) << " taus=" << taus.size(0) << " muons=" << muons.size(0)
            << " etSums=" << sums.size(0) << " | ETT=" << ett << " ETTEM=" << ettem << " HTT=" << htt << " ETM=" << etm << " ETMHF=" << etmhf << " HTM=" << htm
            << " HTMHF=" << sumEt(l1t::EtSum::kMissingHtHF) << "\n";
  // the anomaly score of CICADA (an autoencoder on the 18 x 14 calorimeter regions of Layer-1), if the L1 job kept it
  double cicada = -1;
  if (auto h = ev.getHandle(cicada_); h.isValid() && h->size(0) > 0)
    cicada = h->at(0, 0);
  std::cout << "  CICADA score: " << cicada << (cicada < 0 ? " (not in the file)" : "") << "\n";

  // ------------------------------------------------------------------ the menu and the decision
  std::map<unsigned, const L1TUtmAlgorithm*> algos;
  for (auto const& kv : menu.getAlgorithmMap())
    algos[kv.second.getIndex()] = &kv.second;
  std::cout << "  menu: name=" << menu.getName() << " version=" << menu.getVersion() << " datetime=" << menu.getDatetime() << " scaleSet=" << menu.getScaleSetName()
            << " nAlgorithms=" << algos.size() << " nConditions=" << menu.getConditionMap().size() << " nModules=" << menu.getNmodules() << "\n";
  const auto& blks = ev.get(algBlk_);
  std::cout << "  GlobalAlgBlk: bx range " << blks.getFirstBX() << ".." << blks.getLastBX() << "\n";
  const GlobalAlgBlk* blk = blks.size(0) ? &*blks.begin(0) : nullptr;
  if (!blk) {
    std::cout << "  no GlobalAlgBlk in bx 0\n";
    return;
  }
  const auto& ini = blk->getAlgoDecisionInitial();
  const auto& pre = blk->getAlgoDecisionInterm();
  const auto& fin = blk->getAlgoDecisionFinal();
  auto bit = [](const std::vector<bool>& v, unsigned i) { return i < v.size() && v[i]; };
  int nIni = 0, nPre = 0, nFin = 0;
  for (auto const& kv : algos) {
    nIni += bit(ini, kv.first);
    nPre += bit(pre, kv.first);
    nFin += bit(fin, kv.first);
  }
  std::cout << "  decision: finalOR=" << blk->getFinalOR() << " finalORPreVeto=" << blk->getFinalORPreVeto() << " finalORVeto=" << blk->getFinalORVeto()
            << " prescaleColumn=" << blk->getPreScColumn() << " firedInitial=" << nIni << " firedAfterPrescale=" << nPre << " firedFinal=" << nFin << " of "
            << algos.size() << " algorithms\n";

  // the objects behind every condition that was true, from the object maps of the emulator
  auto describe = [&](l1t::GlobalObject type, int bx, int idx) -> std::string {
    auto obj = [&](const char* n, double et, double eta, double phi) { return std::string(Form("%s[%d](%.1f,%.2f,%.2f)", n, idx, et, eta, phi)); };
    switch (type) {
      case l1t::gtJet:
        if (idx >= 0 && idx < int(jets.size(bx)))
          return obj("Jet", jets.at(bx, idx).pt(), jets.at(bx, idx).eta(), jets.at(bx, idx).phi());
        break;
      case l1t::gtEG:
        if (idx >= 0 && idx < int(egs.size(bx)))
          return obj("EG", egs.at(bx, idx).pt(), egs.at(bx, idx).eta(), egs.at(bx, idx).phi());
        break;
      case l1t::gtTau:
        if (idx >= 0 && idx < int(taus.size(bx)))
          return obj("Tau", taus.at(bx, idx).pt(), taus.at(bx, idx).eta(), taus.at(bx, idx).phi());
        break;
      case l1t::gtMu:
        if (idx >= 0 && idx < int(muons.size(bx)))
          return obj("Mu", muons.at(bx, idx).pt(), muons.at(bx, idx).eta(), muons.at(bx, idx).phi());
        break;
      default:
        break;
    }
    return l1t::GlobalObjectEnumToString(type);
  };
  std::map<int, const GlobalObjectMap*> maps;
  for (auto const& m : ev.get(objectMap_).gtObjectMap())
    maps[m.algoBitNumber()] = &m;
  for (auto const& kv : algos) {
    if (!bit(ini, kv.first))
      continue;
    std::cout << "  fired: bit=" << kv.first << " name=" << kv.second->getName() << " final=" << bit(fin, kv.first) << " expression=\"" << kv.second->getExpression()
              << "\"";
    auto it = maps.find(kv.first);
    if (it != maps.end()) {
      const auto& tokens = it->second->operandTokenVector();
      const auto& combs = it->second->combinationVector();
      const auto& types = it->second->objectTypeVector();
      for (size_t c = 0; c < tokens.size(); ++c) {
        std::cout << " | " << tokens[c].tokenName << "=" << tokens[c].tokenResult;
        if (!tokens[c].tokenResult || c >= combs.size() || c >= types.size())
          continue;
        std::cout << " (" << combs[c].size() << " combination" << (combs[c].size() == 1 ? "" : "s") << ")";
        for (size_t k = 0; k < std::min<size_t>(combs[c].size(), 2); ++k) {
          std::cout << (k ? " or" : ":");
          for (size_t o = 0; o < combs[c][k].size() && o < types[c].size(); ++o)
            std::cout << " " << describe(types[c][o], combs[c][k][o].first, combs[c][k][o].second);
        }
      }
    }
    std::cout << "\n";
  }

  // the conditions of the algorithms that fired, as the menu defines them: the objects and their cuts (threshold in
  // GeV, eta and phi windows, quality and isolation bit masks in "data") and the cuts of the condition (mass, dR ...)
  std::set<std::string> printed;
  for (auto const& kv : algos) {
    auto it = maps.find(kv.first);
    if (!bit(ini, kv.first) || it == maps.end())
      continue;
    for (auto const& token : it->second->operandTokenVector()) {
      auto ic = menu.getConditionMap().find(token.tokenName);
      if (ic == menu.getConditionMap().end() || !printed.insert(token.tokenName).second)
        continue;
      std::cout << "  condition: name=" << token.tokenName << " type=" << ic->second.getType();
      auto cuts = [](const std::vector<L1TUtmCut>& v) {
        for (auto const& c : v) {
          std::cout << " " << c.getName() << "[type " << c.getCutType() << ": " << c.getMinimumValue() << ".." << c.getMaximumValue();
          if (!c.getData().empty())
            std::cout << " data " << c.getData();
          std::cout << "]";
        }
      };
      for (auto const& o : ic->second.getObjects()) {
        std::cout << " | object " << o.getName() << " bx " << o.getBxOffset() << ":";
        cuts(o.getCuts());
      }
      if (!ic->second.getCuts().empty()) {
        std::cout << " | condition cuts:";
        cuts(ic->second.getCuts());
      }
      std::cout << "\n";
    }
  }

  // ------------------------------------------------------------------ the ladders
  auto maxEt = [](const auto& coll, double etaMin, double etaMax, int rank = 0, int minQual = -1) {
    std::vector<double> v;
    for (auto it = coll.begin(0); it != coll.end(0); ++it)
      if (std::abs(it->eta()) >= etaMin && std::abs(it->eta()) < etaMax && it->hwQual() >= minQual)
        v.push_back(it->pt());
    std::sort(v.rbegin(), v.rend());
    return int(v.size()) > rank ? v[rank] : 0.;
  };
  std::vector<Family> families = {
      {"SingleJet", "^L1_SingleJet(\\d+)$", "leading jet, any #eta", maxEt(jets, 0., 9.), {}},
      {"SingleJet..er2p5", "^L1_SingleJet(\\d+)er2p5$", "leading jet, |#eta| < 2.5", maxEt(jets, 0., 2.52), {}},
      {"SingleJet.._FWD3p0", "^L1_SingleJet(\\d+)_FWD3p0$", "leading jet, |#eta| > 3", maxEt(jets, 3.0, 9.), {}},
      {"DoubleJet..er2p5", "^L1_DoubleJet(\\d+)er2p5$", "second jet, |#eta| < 2.5", maxEt(jets, 0., 2.52, 1), {}},
      {"HTT..er", "^L1_HTT(\\d+)er$", "H_{T}: jets > 30 GeV, |#eta| < 2.5", htt, {}},
      {"ETT", "^L1_ETT(\\d+)$", "E_{T}^{tot}", ett, {}},
      {"ETMHF", "^L1_ETMHF(\\d+)$", "E_{T}^{miss} with HF", etmhf, {}},
      {"SingleEG..er2p5", "^L1_SingleEG(\\d+)er2p5$", "leading e/#gamma, |#eta| < 2.5", maxEt(egs, 0., 2.52), {}},
      {"SingleTau..er2p1", "^L1_SingleTau(\\d+)er2p1$", "leading #tau, |#eta| < 2.1", maxEt(taus, 0., 2.13), {}},
      {"SingleMu", "^L1_SingleMu(\\d+)$", "leading #mu, quality #geq 12", maxEt(muons, 0., 9., 0, 12), {}},
  };
  for (auto& f : families) {
    std::regex re(f.pattern);
    std::smatch m;
    for (auto const& kv : algos) {
      const std::string& name = kv.second->getName();
      if (std::regex_match(name, m, re))
        f.thresholds.emplace_back(std::stod(m[1]), bit(ini, kv.first));
    }
    std::sort(f.thresholds.begin(), f.thresholds.end());
    std::cout << "  ladder " << f.name << ": value=" << f.value << " thresholds(fired):";
    for (auto const& t : f.thresholds)
      std::cout << " " << t.first << "(" << t.second << ")";
    std::cout << "\n";
  }
  // every algorithm of the menu with its decision
  for (auto const& kv : algos)
    std::cout << "  algo: bit=" << kv.first << " name=" << kv.second->getName() << " initial=" << bit(ini, kv.first) << " prescaled=" << bit(pre, kv.first)
              << " final=" << bit(fin, kv.first) << "\n";

  // ------------------------------------------------------------------ page 1: the event as the global trigger sees it
  std::vector<reco::GenJet> gj;
  for (auto const& j : ev.get(genJets_))
    if (j.pt() > jetPtMin_)
      gj.push_back(j);
  page_.openPdf(pdfName(pdfFile_, ev.id().run(), ev.id().event(), nEvents_++));
  page_.begin();
  TH2F* frame = page_.keep(
      new TH2F("l1tGlobalFrame", Form("step 6: the objects the global trigger receives, run %u event %llu;#eta;#phi", ev.id().run(), ev.id().event()), 10, -etaMax_, etaMax_, 10, -M_PI, M_PI));
  frame->SetDirectory(nullptr);
  frame->GetYaxis()->SetTitleOffset(0.9);
  frame->Draw();
  page_.etaLines({2.5, 3.0});
  for (auto const& j : gj)
    page_.circle(j.eta(), j.phi(), 0.4, kMagenta + 1, 2);
  auto size = [](double et) { return 0.9 + 0.9 * std::log10(1. + et); };
  for (auto it = jets.begin(0); it != jets.end(0); ++it) {
    page_.marker(it->eta(), it->phi(), 25, kRed, size(it->pt()));
    page_.note(it->eta() + 0.12, it->phi() + 0.13, Form("%.1f", it->pt()), kRed + 1, 0.02);
  }
  for (auto it = taus.begin(0); it != taus.end(0); ++it)
    page_.marker(it->eta(), it->phi(), 26, kAzure + 1, size(it->pt()));
  for (auto it = egs.begin(0); it != egs.end(0); ++it)
    page_.marker(it->eta(), it->phi(), 20, kGreen + 2, 0.7 * size(it->pt()));
  for (auto it = muons.begin(0); it != muons.end(0); ++it) {
    page_.marker(it->eta(), it->phi(), 29, kBlack, 2.2);
    page_.note(it->eta() + 0.12, it->phi() - 0.13, Form("#mu %.1f", it->pt()), kBlack, 0.02);
  }
  // the missing-ET vectors: a tick at their phi on the left edge
  auto arrow = [&](int type, const char* name, int color) {
    if (sumEt(type) <= 0)
      return;
    double phi = wrapToPi(sumPhi(type));
    page_.line(-etaMax_, phi, -etaMax_ + 0.5, phi, color, 3);
    page_.note(-etaMax_ + 0.55, phi, Form("%s %.1f", name, sumEt(type)), color, 0.019);
  };
  arrow(l1t::EtSum::kMissingEtHF, "E_{T}^{miss}(HF)", kOrange + 7);
  arrow(l1t::EtSum::kMissingHt, "H_{T}^{miss}", kViolet + 1);
  page_.label(sampleLabel_);
  page_.text("what the global trigger receives", 0.025, kBlue + 1);
  page_.text(Form("   %u jets (squares, E_{T} in GeV)", jets.size(0)), 0.021, kRed);
  page_.text(Form("   %u #tau candidates (triangles)", taus.size(0)), 0.021, kAzure + 1);
  page_.text(Form("   %u e/#gamma candidates (dots)", egs.size(0)), 0.021, kGreen + 2);
  page_.text(Form("   %u muon%s (stars)", muons.size(0), muons.size(0) == 1 ? "" : "s"), 0.021);
  page_.text(Form("   E_{T}^{tot} %.1f GeV (ECAL only: %.1f)", ett, ettem), 0.021);
  page_.text(Form("   H_{T} %.1f GeV", htt), 0.021);
  page_.text(Form("   E_{T}^{miss} %.1f GeV, with HF %.1f GeV", etm, etmhf), 0.021, kOrange + 7);
  page_.text(Form("   H_{T}^{miss} %.1f GeV", htm), 0.021, kViolet + 1);
  if (cicada >= 0)
    page_.text(Form("   CICADA anomaly score %.1f", cicada), 0.021);
  page_.skip();
  page_.text("the menu", 0.025, kBlue + 1);
  std::string menuName = menu.getName();
  page_.text("   " + menuName, 0.019);
  page_.text(Form("   %zu algorithms, evaluated in parallel", algos.size()), 0.021);
  page_.skip();
  page_.text("the decision", 0.025, kBlue + 1);
  page_.text(Form("   %d algorithms fire", nIni), 0.022);
  page_.text(Form("   %d after prescales and masks", nFin), 0.022);
  page_.text(Form("   FinalOR = %d: the event is %s", int(blk->getFinalOR()), blk->getFinalOR() ? "kept" : "dropped"), 0.024, blk->getFinalOR() ? kGreen + 3 : kRed);
  page_.skip();
  if (!gj.empty())
    page_.text(Form("circles: ak4GenJets, p_{T} > %.0f GeV", jetPtMin_), 0.02, kMagenta + 1);
  page_.text("marker size: log(1 + E_{T}); dotted: |#eta| = 2.5, 3", 0.019, kGray + 2);
  page_.print(savePNG_ ? pngPrefix_ + "_objects" + tag + ".png" : std::string());

  // ------------------------------------------------------------------ page 2: the ladders
  page_.begin();
  TH2F* lf = page_.keep(new TH2F("l1tGlobalLadders", Form("step 6: the thresholds of the menu and the event, run %u event %llu", ev.id().run(), ev.id().event()), 10, 0., 1., 10, 0., 1.));
  lf->SetDirectory(nullptr);
  lf->GetXaxis()->SetLabelSize(0);
  lf->GetYaxis()->SetLabelSize(0);
  lf->GetXaxis()->SetTickLength(0);
  lf->GetYaxis()->SetTickLength(0);
  lf->Draw("AXIS");
  int nRows = 0;
  for (auto const& f : families)
    nRows += !f.thresholds.empty();
  int irow = 0;
  for (auto const& f : families) {
    if (f.thresholds.empty())
      continue;
    double y = 0.95 - (irow + 0.55) * 0.92 / nRows;
    ++irow;
    double xmax = 1.25 * std::max(f.thresholds.back().first, 1.);
    auto X = [&](double v) { return 0.27 + 0.69 * std::min(v, xmax) / xmax; };
    page_.note(0.012, y + 0.012, f.name, kBlack, 0.024);
    page_.note(0.012, y - 0.018, f.what, kGray + 2, 0.016);
    page_.line(X(0), y, X(xmax), y, kBlack, 1);
    int k = 0;
    for (auto const& t : f.thresholds) {
      int color = t.second ? kGreen + 2 : kRed + 1;
      page_.line(X(t.first), y - 0.014, X(t.first), y + 0.014, color, 3);
      // alternate the labels above and below the line when the thresholds are dense
      bool up = f.thresholds.size() > 8 && (k++ % 2);
      page_.note(X(t.first), y + (up ? 0.026 : -0.028), Form("%g", t.first), color, 0.017, 22);
    }
    bool off = f.value > xmax;
    page_.marker(X(f.value), y, off ? 34 : 23, kBlue + 1, 1.6);
    page_.note(X(f.value) + (X(f.value) > 0.85 ? -0.012 : 0.012), y + (f.thresholds.size() > 8 ? 0.05 : 0.036),
               Form("%.1f%s", f.value, off ? " (off scale)" : ""), kBlue + 1, 0.019, X(f.value) > 0.85 ? 32 : 12);
  }
  page_.label(sampleLabel_);
  page_.text("the ladders of the menu", 0.025, kBlue + 1);
  page_.text("ticks: thresholds (GeV); green: fired, red: not", 0.019);
  page_.text("blue triangle: the event", 0.019, kBlue + 1);
  page_.skip(0.005);
  for (auto const& f : families) {
    if (f.thresholds.empty())
      continue;
    double last = -1, next = -1;
    for (auto const& t : f.thresholds) {
      if (t.second)
        last = t.first;
      else if (next < 0)
        next = t.first;
    }
    page_.text(Form("%s: %.1f GeV", f.name, f.value), 0.019);
    std::string s = "   ";
    if (last >= 0)
      s += Form("passes %g by %.1f", last, f.value - last);
    if (next >= 0)
      s += Form("%sfails %g by %.1f", last >= 0 ? ", " : "", next, next - f.value);
    page_.text(s, 0.0175, kGray + 2);
  }
  page_.skip();
  page_.text(Form("%d of %zu algorithms fire; FinalOR = %d", nIni, algos.size(), int(blk->getFinalOR())), 0.021);
  page_.print(savePNG_ ? pngPrefix_ + "_ladders" + tag + ".png" : std::string());
  page_.closePdf();
}

DEFINE_FWK_MODULE(L1TGlobalAnalyzer);
