// HLTPathAnalyzer: the beginning and the end of the High Level Trigger for one event (chapter 13, HLT, of the JitJet
// book): the menu, the raw event, the Level-1 seeds and the prescales (step 1), and the decision of every path, the
// datasets and the streams (step 5).
//
//   step 1  the event reaches the HLT as raw data (FEDRawDataCollection "rawDataCollector", one block of bytes per
//           readout board); every path starts with the Level-1 record unpacked from it (hltGtStage2Digis), a seed
//           module (HLTL1TSeed) that asks whether named Level-1 algorithms fired, and a prescaler (HLTPrescaler)
//   step 5  every path that reaches its last filter accepts the event; the datasets are ORs of paths, the streams
//           ORs of datasets
// Pages (one PDF per event, one PNG per page with savePNG):
//   seed     the Level-1 objects as the HLT unpacks them, the objects that seeded the jet paths, the raw-data sizes
//   ladders  the thresholds of the jet paths of the menu and where the event stands (fired / not, and why not)
//   objects  the event as the HLT records it: the trigger objects of the paths that accepted it (hltTriggerSummaryAOD)
// The log has one line per path (accept, its L1 seed and the module that stopped it), the datasets and streams, the
// filters with their objects, the ladders (read by test/common/hltLedger.py).
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include "DataFormats/Common/interface/TriggerResults.h"
#include "DataFormats/FEDRawData/interface/FEDNumbering.h"
#include "DataFormats/FEDRawData/interface/FEDRawDataCollection.h"
#include "DataFormats/HLTReco/interface/TriggerEvent.h"
#include "DataFormats/HLTReco/interface/TriggerTypeDefs.h"
#include "DataFormats/JetReco/interface/CaloJet.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "DataFormats/JetReco/interface/PFJet.h"
#include "DataFormats/L1TGlobal/interface/GlobalAlgBlk.h"
#include "DataFormats/L1Trigger/interface/BXVector.h"
#include "DataFormats/L1Trigger/interface/EGamma.h"
#include "DataFormats/L1Trigger/interface/EtSum.h"
#include "DataFormats/L1Trigger/interface/Jet.h"
#include "DataFormats/L1Trigger/interface/Muon.h"
#include "DataFormats/L1Trigger/interface/Tau.h"
#include "FWCore/Common/interface/TriggerNames.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/Run.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "HLTrigger/HLTcore/interface/HLTConfigProvider.h"

#include "HLTDisplay.h"

namespace {
  using namespace hltdisplay;

  // the subsystems of the readout, by FED number (DataFormats/FEDRawData/interface/FEDNumbering.h)
  const char* subsystem(int fed) {
    auto in = [fed](int a, int b) { return fed >= a && fed <= b; };
    if (in(FEDNumbering::MINSiPixeluTCAFEDID, FEDNumbering::MAXSiPixeluTCAFEDID) || in(FEDNumbering::MINSiPixelFEDID, FEDNumbering::MAXSiPixelFEDID))
      return "pixel";
    if (in(FEDNumbering::MINSiStripFEDID, FEDNumbering::MAXSiStripFEDID))
      return "strip";
    if (in(FEDNumbering::MINECALFEDID, FEDNumbering::MAXECALFEDID))
      return "ECAL";
    if (in(FEDNumbering::MINPreShowerFEDID, FEDNumbering::MAXPreShowerFEDID))
      return "preshower";
    if (in(FEDNumbering::MINHCALuTCAFEDID, FEDNumbering::MAXHCALuTCAFEDID) || in(FEDNumbering::MINHCALFEDID, FEDNumbering::MAXHCALFEDID))
      return "HCAL";
    if (in(FEDNumbering::MINCSCFEDID, FEDNumbering::MAXCSCFEDID) || in(FEDNumbering::MINCSCDDUFEDID, FEDNumbering::MAXCSCDDUFEDID))
      return "CSC";
    if (in(FEDNumbering::MINDTFEDID, FEDNumbering::MAXDTFEDID) || in(FEDNumbering::MINDTUROSFEDID, FEDNumbering::MAXDTUROSFEDID))
      return "DT";
    if (in(FEDNumbering::MINRPCFEDID, FEDNumbering::MAXRPCFEDID))
      return "RPC";
    if (fed >= FEDNumbering::MINGEMFEDID && fed <= FEDNumbering::MINGEMFEDID + 11)
      return "GEM";
    if (in(FEDNumbering::MINTriggerUpgradeFEDID, FEDNumbering::MAXTriggerUpgradeFEDID) || in(FEDNumbering::MINRCTFEDID, FEDNumbering::MAXRCTFEDID) ||
        in(FEDNumbering::MINCalTrigUp, FEDNumbering::MAXCalTrigUp))
      return "Level-1 trigger";
    if (in(FEDNumbering::MINTotemT2FEDID, FEDNumbering::MAXTotemRPTimingVerticalFEDID) || in(FEDNumbering::MINCTPPSPixelsFEDID, FEDNumbering::MAXCTPPSPixelsFEDID))
      return "PPS";
    if (in(FEDNumbering::MINTCDSuTCAFEDID, FEDNumbering::MAXTCDSuTCAFEDID))
      return "TCDS";
    return "other";
  }

  const char* objectType(int id) {
    switch (id) {
      case trigger::TriggerL1Jet: return "L1Jet";
      case trigger::TriggerL1EG: return "L1EG";
      case trigger::TriggerL1Tau: return "L1Tau";
      case trigger::TriggerL1Mu: return "L1Mu";
      case trigger::TriggerL1ETM: return "L1ETM";
      case trigger::TriggerL1ETMHF: return "L1ETMHF";
      case trigger::TriggerL1HTT: return "L1HTT";
      case trigger::TriggerL1HTM: return "L1HTM";
      case trigger::TriggerL1ETT: return "L1ETT";
      case trigger::TriggerJet: return "Jet";
      case trigger::TriggerBJet: return "BJet";
      case trigger::TriggerMET: return "MET";
      case trigger::TriggerTET: return "TET";
      case trigger::TriggerTHT: return "HT";
      case trigger::TriggerMHT: return "MHT";
      case trigger::TriggerTrack: return "Track";
      case trigger::TriggerCluster: return "Cluster";
      case trigger::TriggerPhoton: return "Photon";
      case trigger::TriggerElectron: return "Electron";
      case trigger::TriggerMuon: return "Muon";
      case trigger::TriggerTau: return "Tau";
      default: return "other";
    }
  }

  // how far a path got: the module that stopped it (or the last one, for a path that accepted)
  struct PathResult {
    std::string name, seed, lastModule, lastType, stage;
    bool accept = false, hasSeed = false, seedFired = false;
    int nModules = 0, lastIndex = 0;
  };

  struct Family {
    const char* name;
    const char* pattern;
    const char* what;
    double value = 0;
    std::vector<std::pair<double, std::string>> thresholds;  // threshold, stage reached ("accept", "seed", "prescale", the filter)
  };
}  // namespace

class HLTPathAnalyzer : public edm::one::EDAnalyzer<edm::one::WatchRuns> {
public:
  explicit HLTPathAnalyzer(const edm::ParameterSet&);
  void beginRun(const edm::Run&, const edm::EventSetup&) override;
  void endRun(const edm::Run&, const edm::EventSetup&) override {}
  void analyze(const edm::Event&, const edm::EventSetup&) override;

private:
  std::string processName_;
  edm::EDGetTokenT<FEDRawDataCollection> raw_;
  edm::EDGetTokenT<BXVector<GlobalAlgBlk>> algBlk_;
  edm::EDGetTokenT<BXVector<l1t::Jet>> l1Jets_;
  edm::EDGetTokenT<BXVector<l1t::EGamma>> l1EGs_;
  edm::EDGetTokenT<BXVector<l1t::Tau>> l1Taus_;
  edm::EDGetTokenT<BXVector<l1t::Muon>> l1Muons_;
  edm::EDGetTokenT<edm::TriggerResults> results_;
  edm::EDGetTokenT<trigger::TriggerEvent> summary_;
  edm::EDGetTokenT<reco::PFJetCollection> pfJets_, ak8Jets_;
  edm::EDGetTokenT<reco::CaloJetCollection> caloJets_;
  edm::EDGetTokenT<std::vector<reco::GenJet>> genJets_;
  std::vector<std::string> seedFilters_;
  std::string pdfFile_, pngPrefix_, sampleLabel_;
  bool savePNG_;
  double jetPtMin_;
  int nEvents_ = 0;
  HLTConfigProvider hlt_;
  Page page_;
};

HLTPathAnalyzer::HLTPathAnalyzer(const edm::ParameterSet& ps)
    : processName_(ps.getParameter<std::string>("processName")),
      raw_(consumes(ps.getParameter<edm::InputTag>("rawData"))),
      algBlk_(consumes(ps.getParameter<edm::InputTag>("l1Results"))),
      l1Jets_(consumes(ps.getParameter<edm::InputTag>("l1Jets"))),
      l1EGs_(consumes(ps.getParameter<edm::InputTag>("l1EGammas"))),
      l1Taus_(consumes(ps.getParameter<edm::InputTag>("l1Taus"))),
      l1Muons_(consumes(ps.getParameter<edm::InputTag>("l1Muons"))),
      results_(consumes(ps.getParameter<edm::InputTag>("triggerResults"))),
      summary_(consumes(ps.getParameter<edm::InputTag>("triggerSummary"))),
      pfJets_(consumes(ps.getParameter<edm::InputTag>("pfJets"))),
      ak8Jets_(consumes(ps.getParameter<edm::InputTag>("ak8PFJets"))),
      caloJets_(consumes(ps.getParameter<edm::InputTag>("caloJets"))),
      genJets_(consumes(ps.getParameter<edm::InputTag>("genJets"))),
      seedFilters_(ps.getParameter<std::vector<std::string>>("seedFilters")),
      pdfFile_(ps.getParameter<std::string>("pdfFile")),
      pngPrefix_(ps.getParameter<std::string>("pngPrefix")),
      sampleLabel_(ps.getParameter<std::string>("sampleLabel")),
      savePNG_(ps.getParameter<bool>("savePNG")),
      jetPtMin_(ps.getParameter<double>("jetPtMin")),
      page_("hltPath") {}

void HLTPathAnalyzer::beginRun(const edm::Run& run, const edm::EventSetup& es) {
  bool changed = false;
  if (!hlt_.init(run, es, processName_, changed))
    std::cout << "  HLTConfigProvider: could not read the configuration of process " << processName_ << "\n";
}

void HLTPathAnalyzer::analyze(const edm::Event& ev, const edm::EventSetup&) {
  std::string tag = Form("_run%u_event%llu", ev.id().run(), ev.id().event());
  std::cout << "== HLTPathAnalyzer run:lumi:event " << ev.id().run() << ":" << ev.luminosityBlock() << ":" << ev.id().event() << "\n";

  // ------------------------------------------------------------------ the menu
  const auto& names = hlt_.triggerNames();
  std::map<std::string, int> byPrefix;
  for (auto const& n : names)
    ++byPrefix[n.substr(0, n.find('_'))];
  std::cout << "  menu: table=" << hlt_.tableName() << " process=" << hlt_.processName() << " globalTag=" << hlt_.globalTag() << " paths=" << names.size()
            << " datasets=" << hlt_.datasetNames().size() << " streams=" << hlt_.streamNames().size() << " prescaleColumns=" << hlt_.prescaleSize() << " |";
  for (auto const& kv : byPrefix)
    std::cout << " " << kv.first << "=" << kv.second;
  std::cout << "\n";
  {
    std::map<std::string, int> types;
    std::set<std::string> modules;
    for (unsigned i = 0; i < names.size(); ++i)
      for (auto const& m : hlt_.moduleLabels(i))
        modules.insert(m);
    for (auto const& m : modules)
      ++types[hlt_.moduleEDMType(m)];
    std::cout << "  menu modules: distinct=" << modules.size();
    for (auto const& kv : types)
      std::cout << " " << kv.first << "=" << kv.second;
    std::cout << "\n";
  }

  // ------------------------------------------------------------------ step 1: the raw event
  const auto& raw = ev.get(raw_);
  std::map<std::string, std::pair<int, size_t>> sizes;
  size_t total = 0;
  int nFed = 0;
  for (int fed = 0; fed <= FEDNumbering::lastFEDId(); ++fed) {
    size_t s = raw.FEDData(fed).size();
    if (!s)
      continue;
    ++nFed;
    total += s;
    auto& e = sizes[subsystem(fed)];
    ++e.first;
    e.second += s;
  }
  std::cout << "  rawdata: feds=" << nFed << " bytes=" << total << "\n";
  std::vector<std::pair<size_t, std::string>> bySize;
  for (auto const& kv : sizes) {
    std::cout << "  rawdata " << kv.first << ": feds=" << kv.second.first << " bytes=" << kv.second.second << "\n";
    bySize.emplace_back(kv.second.second, kv.first);
  }
  std::sort(bySize.rbegin(), bySize.rend());

  // ------------------------------------------------------------------ step 1: the Level-1 record as the HLT unpacks it
  const auto& blks = ev.get(algBlk_);
  const GlobalAlgBlk* blk = blks.size(0) ? &*blks.begin(0) : nullptr;
  int nL1 = 0;
  if (blk)
    for (bool b : blk->getAlgoDecisionFinal())
      nL1 += b;
  const auto& l1Jets = ev.get(l1Jets_);
  const auto& l1EGs = ev.get(l1EGs_);
  const auto& l1Taus = ev.get(l1Taus_);
  const auto& l1Muons = ev.get(l1Muons_);
  std::cout << "  L1 unpacked: finalOR=" << (blk ? blk->getFinalOR() : -1) << " algorithmsFired=" << nL1 << " jets=" << l1Jets.size(0) << " egammas=" << l1EGs.size(0)
            << " taus=" << l1Taus.size(0) << " muons=" << l1Muons.size(0) << "\n";
  for (auto it = l1Jets.begin(0); it != l1Jets.end(0); ++it)
    std::cout << "  L1Jet: et=" << it->pt() << " eta=" << it->eta() << " phi=" << it->phi() << "\n";

  // ------------------------------------------------------------------ the decision of every path
  const auto& tr = ev.get(results_);
  const edm::TriggerNames& trNames = ev.triggerNames(tr);
  std::vector<PathResult> paths;
  std::map<std::string, const PathResult*> byName;
  std::map<std::string, int> stageCount;
  for (unsigned i = 0; i < tr.size(); ++i) {
    PathResult p;
    p.name = trNames.triggerName(i);
    p.accept = tr.accept(i);
    unsigned hi = hlt_.triggerIndex(p.name);
    if (hi >= hlt_.size())
      continue;
    const auto& mods = hlt_.moduleLabels(hi);
    p.nModules = mods.size();
    p.lastIndex = tr.index(i);
    p.lastModule = p.lastIndex < int(mods.size()) ? mods[p.lastIndex] : "";
    p.lastType = p.lastModule.empty() ? "" : hlt_.moduleType(p.lastModule);
    const auto& seeds = hlt_.hltL1TSeeds(hi);
    int seedIndex = -1;
    for (unsigned k = 0; k < mods.size(); ++k)
      if (hlt_.moduleType(mods[k]) == "HLTL1TSeed") {
        seedIndex = k;
        break;
      }
    p.hasSeed = seedIndex >= 0;
    p.seed = seeds.empty() ? "" : seeds.front();
    p.seedFired = p.hasSeed && (p.accept || p.lastIndex > seedIndex);
    if (p.accept)
      p.stage = "accept";
    else if (p.lastType == "HLTL1TSeed")
      p.stage = "seed";
    else if (p.lastType == "HLTPrescaler")
      p.stage = "prescale";
    else
      p.stage = "filter";
    if (p.name.rfind("HLT_", 0) == 0 || p.name.rfind("DST_", 0) == 0)
      ++stageCount[p.stage];
    paths.push_back(p);
  }
  for (auto const& p : paths)
    byName[p.name] = &p;
  int nPhys = 0;
  for (auto const& p : paths)
    nPhys += p.name.rfind("HLT_", 0) == 0 || p.name.rfind("DST_", 0) == 0;
  std::cout << "  paths HLT_+DST_: total=" << nPhys << " accept=" << stageCount["accept"] << " seedFailed=" << stageCount["seed"]
            << " prescaled=" << stageCount["prescale"] << " filterFailed=" << stageCount["filter"] << "\n";
  for (auto const& p : paths)
    std::cout << "  path: name=" << p.name << " accept=" << p.accept << " stage=" << p.stage << " seedFired=" << (p.hasSeed ? (p.seedFired ? "1" : "0") : "-")
              << " last=" << p.lastModule << " lastType=" << p.lastType << " lastIndex=" << p.lastIndex << " nModules=" << p.nModules << " seed=\"" << p.seed
              << "\"\n";

  // the datasets (an OR of paths) and the streams (an OR of datasets)
  std::set<std::string> acceptedDatasets;
  auto stem = [](const std::string& n) { return std::regex_replace(n, std::regex("_v[0-9]+$"), ""); };
  std::map<std::string, const PathResult*> byStem;
  for (auto const& p : paths)
    byStem[stem(p.name)] = &p;
  for (auto const& d : hlt_.datasetNames()) {
    std::vector<std::string> fired;
    for (auto const& pn : hlt_.datasetContent(d)) {
      auto it = byStem.find(stem(pn));
      if (it != byStem.end() && it->second->accept)
        fired.push_back(it->second->name);
    }
    if (!fired.empty())
      acceptedDatasets.insert(d);
    std::cout << "  dataset: name=" << d << " accept=" << !fired.empty() << " paths=" << hlt_.datasetContent(d).size() << " fired=" << fired.size() << " |";
    for (auto const& f : fired)
      std::cout << " " << f;
    std::cout << "\n";
  }
  std::vector<std::string> acceptedStreams;
  for (auto const& s : hlt_.streamNames()) {
    std::vector<std::string> ds;
    for (auto const& d : hlt_.streamContent(s))
      if (acceptedDatasets.count(d))
        ds.push_back(d);
    if (!ds.empty())
      acceptedStreams.push_back(s);
    std::cout << "  stream: name=" << s << " accept=" << !ds.empty() << " datasets=" << hlt_.streamContent(s).size() << " |";
    for (auto const& d : ds)
      std::cout << " " << d;
    std::cout << "\n";
  }

  // ------------------------------------------------------------------ the objects of the filters (hltTriggerSummaryAOD)
  const auto& summary = ev.get(summary_);
  const auto& objects = summary.getObjects();
  std::cout << "  summary: collections=" << summary.sizeCollections() << " objects=" << summary.sizeObjects() << " filters=" << summary.sizeFilters() << "\n";
  for (trigger::size_type f = 0; f < summary.sizeFilters(); ++f) {
    const auto& keys = summary.filterKeys(f);
    const auto& ids = summary.filterIds(f);
    std::cout << "  filter: label=" << summary.filterLabel(f) << " type=" << (hlt_.moduleType(std::string(summary.filterLabel(f)))) << " n=" << keys.size() << " |";
    for (size_t k = 0; k < keys.size() && k < 12; ++k) {
      const auto& o = objects.at(keys[k]);
      std::cout << " " << objectType(ids[k]) << "(" << Form("%.1f,%.2f,%.2f", o.pt(), o.eta(), o.phi()) << ")";
    }
    if (keys.size() > 12)
      std::cout << " ...";
    std::cout << "\n";
  }
  auto filterObjects = [&](const std::string& label) {
    std::vector<std::pair<int, const trigger::TriggerObject*>> v;
    auto idx = summary.filterIndex(edm::InputTag(label, "", processName_));
    if (idx >= summary.sizeFilters())
      return v;
    const auto& keys = summary.filterKeys(idx);
    const auto& ids = summary.filterIds(idx);
    for (size_t k = 0; k < keys.size(); ++k)
      v.emplace_back(ids[k], &objects.at(keys[k]));
    return v;
  };
  // the jet filters of the paths of the book, with their cut
  for (auto const& label : {"hltSingleAK4CaloJet80", "hltSinglePFJet110", "hltSingleCaloJet110", "hltSinglePFJet140", "hltSingleCaloJet170", "hltSinglePFJet200",
                             "hltSingleCaloFwdJet110", "hltSinglePFFwdJet140"}) {
    try {
      const auto& pset = hlt_.modulePSet(label);
      std::string line = Form("  cut: label=%s type=%s MinPt=%g MinEta=%g MaxEta=%g input=%s", label, hlt_.moduleType(label).c_str(), pset.getParameter<double>("MinPt"),
                              pset.getParameter<double>("MinEta"), pset.getParameter<double>("MaxEta"), pset.getParameter<edm::InputTag>("inputTag").encode().c_str());
      std::cout << line << "\n";
    } catch (...) {
      std::cout << "  cut: label=" << label << " not in the menu\n";
    }
  }

  // ------------------------------------------------------------------ the ladders of the jet paths
  const auto& pf = ev.get(pfJets_);
  const auto& calo = ev.get(caloJets_);
  auto lead = [](auto const& jets, double etaMin, double etaMax, int rank = 0) {
    std::vector<double> pts;
    for (auto const& j : jets)
      if (std::abs(j.eta()) >= etaMin && std::abs(j.eta()) < etaMax)
        pts.push_back(j.pt());
    std::sort(pts.rbegin(), pts.rend());
    return int(pts.size()) > rank ? pts[rank] : 0.;
  };
  std::cout << "  leading jets: pf=" << Form("%.1f", lead(pf, 0, 5.)) << " pf2=" << Form("%.1f", lead(pf, 0, 5., 1)) << " pfFwd=" << Form("%.1f", lead(pf, 2.7, 5.))
            << " calo=" << Form("%.1f", lead(calo, 0, 5.)) << " calo2=" << Form("%.1f", lead(calo, 0, 5., 1)) << "\n";
  double ht = 0;
  for (auto const& j : pf)
    if (j.pt() > 30 && std::abs(j.eta()) < 2.5)
      ht += j.pt();
  double ave = 0.5 * (lead(pf, 0, 5.) + lead(pf, 0, 5., 1));
  double ak8 = 0;
  if (auto h = ev.getHandle(ak8Jets_); h.isValid())
    ak8 = lead(*h, 0, 5.);
  std::vector<Family> families = {
      {"PFJet", "^HLT_PFJet(\\d+)_v\\d+$", "leading AK4 PF jet, |#eta| < 5", lead(pf, 0, 5.), {}},
      {"PFJetFwd", "^HLT_PFJetFwd(\\d+)_v\\d+$", "leading AK4 PF jet, |#eta| > 2.7", lead(pf, 2.7, 5.), {}},
      {"AK8PFJet", "^HLT_AK8PFJet(\\d+)_v\\d+$", "leading AK8 PF jet", ak8, {}},
      {"DiPFJetAve", "^HLT_DiPFJetAve(\\d+)_v\\d+$", "p_{T} average of the two leading PF jets", ave, {}},
      {"DiPFJetAve_HFJEC", "^HLT_DiPFJetAve(\\d+)_HFJEC_v\\d+$", "the same, one jet |#eta| < 1.4, one > 2.7", ave, {}},
      {"PFHT", "^HLT_PFHT(\\d+)_v\\d+$", "H_{T}: PF jets > 30 GeV, |#eta| < 2.5", ht, {}},
  };
  for (auto& f : families) {
    std::regex re(f.pattern);
    std::smatch m;
    for (auto const& p : paths)
      if (std::regex_match(p.name, m, re))
        f.thresholds.emplace_back(std::stod(m[1]), p.stage == "filter" ? p.lastModule : p.stage);
    std::sort(f.thresholds.begin(), f.thresholds.end());
    std::cout << "  ladder " << f.name << ": value=" << f.value << " thresholds(stage):";
    for (auto const& t : f.thresholds)
      std::cout << " " << t.first << "(" << t.second << ")";
    std::cout << "\n";
  }

  // ------------------------------------------------------------------ page 1: step 1, the seed
  std::vector<reco::GenJet> gj;
  for (auto const& j : ev.get(genJets_))
    if (j.pt() > jetPtMin_)
      gj.push_back(j);
  page_.openPdf(pdfName(pdfFile_, ev.id().run(), ev.id().event(), nEvents_++));
  page_.begin();
  frame(page_, "hltSeedFrame", Form("step 1: the Level-1 objects the HLT unpacks, run %u event %llu", ev.id().run(), ev.id().event()));
  detectorLines(page_);
  genJets(page_, gj, jetPtMin_);
  auto size = [](double et) { return 0.9 + 0.9 * std::log10(1. + et); };
  for (auto it = l1Taus.begin(0); it != l1Taus.end(0); ++it)
    page_.marker(it->eta(), it->phi(), 26, kAzure + 1, size(it->pt()));
  for (auto it = l1EGs.begin(0); it != l1EGs.end(0); ++it)
    page_.marker(it->eta(), it->phi(), 20, kGreen + 2, 0.7 * size(it->pt()));
  for (auto it = l1Jets.begin(0); it != l1Jets.end(0); ++it) {
    page_.marker(it->eta(), it->phi(), 25, kRed, size(it->pt()));
    page_.note(it->eta() + 0.12, it->phi() - 0.16, Form("%.1f", it->pt()), kRed + 1, 0.02);
  }
  for (auto it = l1Muons.begin(0); it != l1Muons.end(0); ++it) {
    page_.marker(it->etaAtVtx(), it->phiAtVtx(), 29, kBlack, 2.2);
    page_.note(it->etaAtVtx() + 0.12, it->phiAtVtx() - 0.16, Form("#mu %.1f", it->pt()), kBlack, 0.02);
  }
  // the objects that the seed modules of the jet paths hand to the rest of the path
  std::vector<std::string> seedLines;
  for (auto const& label : seedFilters_) {
    auto objs = filterObjects(label);
    std::string expr;
    try {
      expr = hlt_.modulePSet(label).getParameter<std::string>("L1SeedsLogicalExpression");
    } catch (...) {
    }
    std::cout << "  seedObjects: label=" << label << " expression=\"" << expr << "\" n=" << objs.size() << " |";
    for (auto const& o : objs)
      std::cout << " " << objectType(o.first) << "(" << Form("%.1f,%.2f,%.2f", o.second->pt(), o.second->eta(), o.second->phi()) << ")";
    std::cout << "\n";
    if (label == seedFilters_.front())
      for (auto const& o : objs)
        if (o.first == trigger::TriggerL1Jet)
          page_.circle(o.second->eta(), o.second->phi(), 0.28, kOrange + 7, 3);
    seedLines.push_back(Form("   %s: %zu object%s", label.c_str(), objs.size(), objs.size() == 1 ? "" : "s"));
  }
  page_.label(sampleLabel_);
  page_.text("the raw event, as the HLT receives it", 0.025, kBlue + 1);
  page_.text(Form("   %d readout boards (FEDs), %.0f kB", nFed, total / 1024.), 0.021);
  for (size_t k = 0; k < bySize.size() && k < 6; ++k)
    page_.text(Form("   %s: %.1f kB (%d FEDs)", bySize[k].second.c_str(), bySize[k].first / 1024., sizes[bySize[k].second].first), 0.019, kGray + 2);
  page_.skip();
  page_.text("the Level-1 record, unpacked", 0.025, kBlue + 1);
  page_.text(Form("   %d algorithms fired, FinalOR %d", nL1, blk ? int(blk->getFinalOR()) : -1), 0.021);
  page_.text(Form("   %u jets (squares), %u #tau, %u e/#gamma, %u #mu", l1Jets.size(0), l1Taus.size(0), l1EGs.size(0), l1Muons.size(0)), 0.021);
  page_.skip();
  page_.text("the seeds of the paths", 0.025, kBlue + 1);
  page_.text(Form("   %d paths; %d stopped by their L1 seed", nPhys, stageCount["seed"]), 0.021);
  page_.text(Form("   %d by a prescale; %d start their reconstruction", stageCount["prescale"], nPhys - stageCount["seed"] - stageCount["prescale"]), 0.021);
  for (auto const& s : seedLines)
    page_.text(s, 0.0185, kOrange + 7);
  page_.skip();
  page_.text(Form("orange circles: the jets of %s", seedFilters_.empty() ? "" : seedFilters_.front().c_str()), 0.019, kOrange + 7);
  page_.text(Form("magenta: ak4GenJets > %.0f GeV", jetPtMin_), 0.019, kGenColor);
  page_.print(savePNG_ ? pngPrefix_ + "_seed" + tag + ".png" : std::string());

  // ------------------------------------------------------------------ page 2: step 5, the ladders
  page_.begin();
  TH2F* lf = page_.keep(new TH2F("hltLadders", Form("step 5: the jet paths of the menu and the event, run %u event %llu", ev.id().run(), ev.id().event()), 10, 0., 1., 10, 0., 1.));
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
  auto stageColor = [](const std::string& s) { return s == "accept" ? kGreen + 2 : s == "seed" ? kGray + 1 : s == "prescale" ? kOrange : kRed + 1; };
  for (auto const& f : families) {
    if (f.thresholds.empty())
      continue;
    double y = 0.95 - (irow + 0.55) * 0.92 / std::max(nRows, 1);
    ++irow;
    double xmax = 1.15 * std::max(f.thresholds.back().first, 1.);
    auto X = [&](double v) { return 0.27 + 0.69 * std::min(v, xmax) / xmax; };
    page_.note(0.012, y + 0.012, f.name, kBlack, 0.024);
    page_.note(0.012, y - 0.018, f.what, kGray + 2, 0.016);
    page_.line(X(0), y, X(xmax), y, kBlack, 1);
    int k = 0;
    for (auto const& t : f.thresholds) {
      int color = stageColor(t.second);
      page_.line(X(t.first), y - 0.014, X(t.first), y + 0.014, color, 3);
      bool up = f.thresholds.size() > 7 && (k++ % 2);
      page_.note(X(t.first), y + (up ? 0.026 : -0.028), Form("%g", t.first), color, 0.017, 22);
    }
    bool off = f.value > xmax;
    page_.marker(X(f.value), y, off ? 34 : 23, kBlue + 1, 1.6);
    page_.note(X(f.value) + (X(f.value) > 0.85 ? -0.012 : 0.012), y + (f.thresholds.size() > 7 ? 0.05 : 0.036), Form("%.1f%s", f.value, off ? " (off scale)" : ""),
               kBlue + 1, 0.019, X(f.value) > 0.85 ? 32 : 12);
  }
  page_.label(sampleLabel_);
  page_.text("the ladders of the jet paths", 0.025, kBlue + 1);
  page_.text("ticks: thresholds in the path name (GeV)", 0.019);
  page_.text("green: accepted; red: stopped by a filter", 0.019);
  page_.text("grey: stopped by the L1 seed", 0.019, kGray + 1);
  page_.text("blue triangle: the event", 0.019, kBlue + 1);
  page_.skip(0.005);
  for (auto const& f : families) {
    if (f.thresholds.empty())
      continue;
    double last = -1, next = -1;
    std::string why;
    for (auto const& t : f.thresholds) {
      if (t.second == "accept")
        last = t.first;
      else if (next < 0) {
        next = t.first;
        why = t.second;
      }
    }
    page_.text(Form("%s: %.1f GeV", f.name, f.value), 0.019);
    std::string s = "   ";
    if (last >= 0)
      s += Form("passes %g", last);
    if (next >= 0)
      s += Form("%sfails %g (%s)", last >= 0 ? ", " : "", next, why.c_str());
    page_.text(s, 0.0165, kGray + 2);
  }
  page_.print(savePNG_ ? pngPrefix_ + "_ladders" + tag + ".png" : std::string());

  // ------------------------------------------------------------------ page 3: the event as the HLT records it
  page_.begin();
  frame(page_, "hltObjectsFrame", Form("step 5: the trigger objects of the accepted paths, run %u event %llu", ev.id().run(), ev.id().event()));
  detectorLines(page_);
  genJets(page_, gj, jetPtMin_, false);
  // the objects of the last saveTags filter of every accepted HLT_ path, drawn once each
  std::set<trigger::size_type> drawnKeys;
  std::map<std::string, int> acceptedFamilies;
  int nAcc = 0;
  for (auto const& p : paths) {
    if (!p.accept || p.name.rfind("HLT_", 0) != 0)
      continue;
    ++nAcc;
    ++acceptedFamilies[std::regex_replace(p.name, std::regex("[0-9]+.*$"), "")];
    const auto& saveTags = hlt_.saveTagsModules(p.name);
    if (saveTags.empty())
      continue;
    auto idx = summary.filterIndex(edm::InputTag(saveTags.back(), "", processName_));
    if (idx >= summary.sizeFilters())
      continue;
    const auto& keys = summary.filterKeys(idx);
    const auto& ids = summary.filterIds(idx);
    for (size_t k = 0; k < keys.size(); ++k) {
      if (!drawnKeys.insert(keys[k]).second)
        continue;
      const auto& o = objects.at(keys[k]);
      int id = ids[k];
      bool ak8 = saveTags.back().find("AK8") != std::string::npos;
      if ((id == trigger::TriggerJet || id == trigger::TriggerBJet) && ak8) {  // a jet of radius 0.8: open, label above
        page_.marker(o.eta(), o.phi(), 25, kOrange + 7, 1.3 * size(o.pt()));
        page_.note(o.eta() - 0.15, o.phi() + 0.2, Form("AK8 %.1f", o.pt()), kOrange + 7, 0.02, 32);
      } else if (id == trigger::TriggerJet || id == trigger::TriggerBJet) {
        page_.marker(o.eta(), o.phi(), 21, kRed, size(o.pt()));
        page_.note(o.eta() + 0.15, o.phi() - 0.2, Form("%.1f", o.pt()), kRed + 1, 0.021);
      } else if (id == trigger::TriggerMuon) {
        page_.marker(o.eta(), o.phi(), 29, kBlack, 2.2);
      } else if (id == trigger::TriggerMET || id == trigger::TriggerMHT) {
        page_.line(-kEtaMax, o.phi(), -kEtaMax + 0.5, o.phi(), kOrange + 7, 3);
      } else if (id > 0) {
        page_.marker(o.eta(), o.phi(), 24, kAzure + 1, 1.2);
      }
    }
  }
  std::cout << "  accepted: paths=" << nAcc << " datasets=" << acceptedDatasets.size() << " streams=" << acceptedStreams.size() << "\n";
  page_.label(sampleLabel_);
  page_.text("what the HLT records", 0.025, kBlue + 1);
  page_.text(Form("   %d HLT paths accept the event", nAcc), 0.022, nAcc ? kGreen + 3 : kRed);
  std::vector<std::pair<int, std::string>> fam;
  for (auto const& kv : acceptedFamilies)
    fam.emplace_back(kv.second, kv.first);
  std::sort(fam.rbegin(), fam.rend());
  for (size_t k = 0; k < fam.size() && k < 9; ++k)
    page_.text(Form("   %s*: %d", fam[k].second.c_str(), fam[k].first), 0.0185, kGray + 2);
  page_.skip();
  // the datasets of the physics streams (the others are calibration, monitoring, express, alignment)
  std::set<std::string> physics;
  for (auto const& s : acceptedStreams)
    if (s.rfind("Physics", 0) == 0 || s.rfind("Scouting", 0) == 0)
      for (auto const& d : hlt_.streamContent(s))
        if (acceptedDatasets.count(d) && d.rfind("Ephemeral", 0) != 0 && d.rfind("Emittance", 0) != 0)
          physics.insert(d);
  page_.text(Form("   %zu datasets, %zu streams; physics:", acceptedDatasets.size(), acceptedStreams.size()), 0.021);
  std::string line = "  ";
  int nl = 0;
  for (auto const& d : physics) {
    if (line.size() + d.size() > 30 && nl < 4) {
      page_.text(line, 0.0185, kGray + 2);
      line = "  ";
      ++nl;
    }
    line += " " + d;
  }
  if (line.size() > 2)
    page_.text(line, 0.0185, kGray + 2);
  page_.skip();
  page_.text("red squares: HLT AK4 jets (p_{T} in GeV)", 0.019, kRed);
  page_.text("open orange squares: HLT AK8 jets", 0.019, kOrange + 7);
  page_.text("orange tick, left edge: the calorimeter MET", 0.019, kOrange + 7);
  page_.text("open circles: other objects", 0.019, kAzure + 1);
  page_.text("magenta: ak4GenJets", 0.019, kGenColor);
  page_.print(savePNG_ ? pngPrefix_ + "_objects" + tag + ".png" : std::string());
  page_.closePdf();
}

DEFINE_FWK_MODULE(HLTPathAnalyzer);
