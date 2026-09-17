// L1Dumper: every Level-1 trigger object (jets, e/gamma, taus, muons, energy sums, calo
// towers and clusters) for every bunch crossing, with hardware and physical values and
// a Delta-R match to the generator objects, plus the uGT algorithm decisions with names.
#include <map>

#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/L1Trigger/interface/Jet.h"
#include "DataFormats/L1Trigger/interface/EGamma.h"
#include "DataFormats/L1Trigger/interface/Tau.h"
#include "DataFormats/L1Trigger/interface/Muon.h"
#include "DataFormats/L1Trigger/interface/EtSum.h"
#include "DataFormats/L1TCalorimeter/interface/CaloTower.h"
#include "DataFormats/L1TCalorimeter/interface/CaloCluster.h"
#include "DataFormats/L1TGlobal/interface/GlobalAlgBlk.h"
#include "CondFormats/L1TObjects/interface/L1TUtmTriggerMenu.h"
#include "CondFormats/DataRecord/interface/L1TUtmTriggerMenuRcd.h"

#include "DumpUtil.h"

using namespace chaindump;

class L1Dumper : public edm::one::EDAnalyzer<> {
public:
  explicit L1Dumper(const edm::ParameterSet& ps);
  void analyze(const edm::Event& ev, const edm::EventSetup& es) override;

private:
  template <class T, class EXTRA>
  void printBX(std::ostream& os, const std::string& name, const Tagged<BXVector<T>>& t, const edm::Event& ev, EXTRA extra,
               const reco::GenParticleCollection* gens, const reco::GenJetCollection* gjets, bool matchJets);

  std::string step_;
  int maxElements_;
  bool printAllAlgos_;
  edm::InputTag genParticlesTag_, genJetsTag_, algBlkTag_;
  edm::EDGetTokenT<reco::GenParticleCollection> genParticlesTok_;
  edm::EDGetTokenT<reco::GenJetCollection> genJetsTok_;
  edm::EDGetTokenT<BXVector<GlobalAlgBlk>> algBlkTok_;
  edm::ESGetToken<L1TUtmTriggerMenu, L1TUtmTriggerMenuRcd> menuTok_;
  std::vector<Tagged<BXVector<l1t::Jet>>> jets_;
  std::vector<Tagged<BXVector<l1t::EGamma>>> egammas_;
  std::vector<Tagged<BXVector<l1t::Tau>>> taus_;
  std::vector<Tagged<BXVector<l1t::Muon>>> muons_;
  std::vector<Tagged<BXVector<l1t::EtSum>>> etSums_;
  std::vector<Tagged<BXVector<l1t::CaloTower>>> towers_;
  std::vector<Tagged<BXVector<l1t::CaloCluster>>> clusters_;
};

L1Dumper::L1Dumper(const edm::ParameterSet& ps)
    : step_(optString(ps, "step", "L1")),
      maxElements_(optInt(ps, "maxElements", -1)),
      printAllAlgos_(optBool(ps, "printAllAlgos", false)),
      genParticlesTag_(optTag(ps, "genParticles")),
      genJetsTag_(optTag(ps, "genJets")),
      algBlkTag_(optTag(ps, "algBlk")) {
  if (!genParticlesTag_.label().empty())
    genParticlesTok_ = consumes<reco::GenParticleCollection>(genParticlesTag_);
  if (!genJetsTag_.label().empty())
    genJetsTok_ = consumes<reco::GenJetCollection>(genJetsTag_);
  if (!algBlkTag_.label().empty()) {
    algBlkTok_ = consumes<BXVector<GlobalAlgBlk>>(algBlkTag_);
    menuTok_ = esConsumes<L1TUtmTriggerMenu, L1TUtmTriggerMenuRcd>();
  }
  auto reg = [&](auto& vec, const char* name) {
    using T = typename std::decay_t<decltype(vec)>::value_type;
    for (auto const& t : optTags(ps, name))
      vec.push_back(T{t, consumes<typename T::product_type>(t)});
  };
  reg(jets_, "jets");
  reg(egammas_, "egammas");
  reg(taus_, "taus");
  reg(muons_, "muons");
  reg(etSums_, "etSums");
  reg(towers_, "caloTowers");
  reg(clusters_, "caloClusters");
}

template <class T, class EXTRA>
void L1Dumper::printBX(std::ostream& os, const std::string& name, const Tagged<BXVector<T>>& t, const edm::Event& ev, EXTRA extra,
                       const reco::GenParticleCollection* gens, const reco::GenJetCollection* gjets, bool matchJets) {
  edm::Handle<BXVector<T>> h;
  ev.getByToken(t.token, h);
  if (!h.isValid()) {
    missing(os, name, t.tag);
    return;
  }
  section(os, name, t.tag, h->size());
  os << "  bxRange=[" << h->getFirstBX() << "," << h->getLastBX() << "]\n";
  for (int bx = h->getFirstBX(); bx <= h->getLastBX(); ++bx) {
    size_t n = h->size(bx);
    if (n == 0)
      continue;
    os << "  bx=" << bx << " n=" << n << "\n";
    size_t i = 0, lim = cap(maxElements_, n);
    for (auto it = h->begin(bx); it != h->end(bx) && i < lim; ++it, ++i) {
      const T& o = *it;
      os << "    [" << i << "] pt=" << o.pt() << " eta=" << o.eta() << " phi=" << o.phi() << " E=" << o.energy() << " hwPt=" << o.hwPt()
         << " hwEta=" << o.hwEta() << " hwPhi=" << o.hwPhi() << " hwQual=" << o.hwQual() << " hwIso=" << o.hwIso();
      extra(os, o);
      if (o.pt() > 0) {
        if (matchJets && gjets)
          os << " " << matchStr(nearestGenJet(o.eta(), o.phi(), *gjets), "genJet", o.pt());
        else if (!matchJets && gens)
          os << " " << matchStr(nearestGen(o.eta(), o.phi(), *gens, false), "gen", o.pt());
      }
      os << "\n";
    }
    truncated(os, lim, n, "    ");
  }
}

namespace {
  const char* etSumName(int t) {
    switch (t) {
      case l1t::EtSum::kTotalEt: return "TotalEt";
      case l1t::EtSum::kTotalHt: return "TotalHt";
      case l1t::EtSum::kMissingEt: return "MissingEt";
      case l1t::EtSum::kMissingHt: return "MissingHt";
      case l1t::EtSum::kTotalEtx: return "TotalEtx";
      case l1t::EtSum::kTotalEty: return "TotalEty";
      case l1t::EtSum::kTotalHtx: return "TotalHtx";
      case l1t::EtSum::kTotalHty: return "TotalHty";
      case l1t::EtSum::kMissingEtHF: return "MissingEtHF";
      case l1t::EtSum::kTotalEtxHF: return "TotalEtxHF";
      case l1t::EtSum::kTotalEtyHF: return "TotalEtyHF";
      case l1t::EtSum::kMinBiasHFP0: return "MinBiasHFP0";
      case l1t::EtSum::kMinBiasHFM0: return "MinBiasHFM0";
      case l1t::EtSum::kMinBiasHFP1: return "MinBiasHFP1";
      case l1t::EtSum::kMinBiasHFM1: return "MinBiasHFM1";
      case l1t::EtSum::kTotalEtHF: return "TotalEtHF";
      case l1t::EtSum::kTotalEtEm: return "TotalEtEm";
      case l1t::EtSum::kTotalHtHF: return "TotalHtHF";
      case l1t::EtSum::kTotalHtxHF: return "TotalHtxHF";
      case l1t::EtSum::kTotalHtyHF: return "TotalHtyHF";
      case l1t::EtSum::kMissingHtHF: return "MissingHtHF";
      case l1t::EtSum::kTowerCount: return "TowerCount";
      case l1t::EtSum::kCentrality: return "Centrality";
      case l1t::EtSum::kAsymEt: return "AsymEt";
      case l1t::EtSum::kAsymHt: return "AsymHt";
      case l1t::EtSum::kAsymEtHF: return "AsymEtHF";
      case l1t::EtSum::kAsymHtHF: return "AsymHtHF";
      default: return "other";
    }
  }
}  // namespace

void L1Dumper::analyze(const edm::Event& ev, const edm::EventSetup& es) {
  std::ostream& os = std::cout;
  eventHeader(os, ev, step_, moduleDescription().moduleLabel());

  const reco::GenParticleCollection* gens = nullptr;
  const reco::GenJetCollection* gjets = nullptr;
  edm::Handle<reco::GenParticleCollection> hg;
  edm::Handle<reco::GenJetCollection> hj;
  if (!genParticlesTag_.label().empty()) {
    ev.getByToken(genParticlesTok_, hg);
    if (hg.isValid())
      gens = hg.product();
  }
  if (!genJetsTag_.label().empty()) {
    ev.getByToken(genJetsTok_, hj);
    if (hj.isValid())
      gjets = hj.product();
  }

  for (auto const& t : jets_)
    printBX(os, "L1 Jets", t, ev, [](std::ostream& o, const l1t::Jet& j) {
      o << " towerIEta=" << j.towerIEta() << " towerIPhi=" << j.towerIPhi() << " rawEt=" << j.rawEt() << " seedEt=" << j.seedEt() << " puEt=" << j.puEt();
    }, gens, gjets, true);
  for (auto const& t : egammas_)
    printBX(os, "L1 EGammas", t, ev, [](std::ostream& o, const l1t::EGamma& e) {
      o << " towerIEta=" << e.towerIEta() << " towerIPhi=" << e.towerIPhi() << " rawEt=" << e.rawEt() << " isoEt=" << e.isoEt() << " footprintEt=" << e.footprintEt() << " nTT=" << e.nTT() << " shape=" << e.shape() << " towerHoE=" << e.towerHoE();
    }, gens, gjets, false);
  for (auto const& t : taus_)
    printBX(os, "L1 Taus", t, ev, [](std::ostream& o, const l1t::Tau& tau) {
      o << " towerIEta=" << tau.towerIEta() << " towerIPhi=" << tau.towerIPhi() << " rawEt=" << tau.rawEt() << " isoEt=" << tau.isoEt() << " nTT=" << tau.nTT() << " hasEM=" << tau.hasEM() << " isMerged=" << tau.isMerged();
    }, gens, gjets, false);
  for (auto const& t : muons_)
    printBX(os, "L1 Muons", t, ev, [](std::ostream& o, const l1t::Muon& m) {
      o << " charge=" << m.charge() << " hwCharge=" << m.hwCharge() << " hwChargeValid=" << m.hwChargeValid() << " tfMuonIndex=" << m.tfMuonIndex()
        << " etaAtVtx=" << m.etaAtVtx() << " phiAtVtx=" << m.phiAtVtx() << " hwEtaAtVtx=" << m.hwEtaAtVtx() << " hwPhiAtVtx=" << m.hwPhiAtVtx()
        << " hwDXY=" << m.hwDXY() << " ptUnconstrained=" << m.ptUnconstrained() << " hwPtUnconstrained=" << m.hwPtUnconstrained();
    }, gens, gjets, false);
  for (auto const& t : etSums_)
    printBX(os, "L1 EtSums", t, ev, [](std::ostream& o, const l1t::EtSum& s) { o << " type=" << s.getType() << "(" << etSumName(s.getType()) << ")"; }, nullptr, nullptr, false);
  for (auto const& t : towers_)
    printBX(os, "L1 CaloTowers", t, ev, [](std::ostream& o, const l1t::CaloTower& c) {
      o << " etEm=" << c.etEm() << " etHad=" << c.etHad() << " hwEtEm=" << c.hwEtEm() << " hwEtHad=" << c.hwEtHad() << " hwEtRatio=" << c.hwEtRatio();
    }, nullptr, nullptr, false);
  for (auto const& t : clusters_)
    printBX(os, "L1 CaloClusters", t, ev, [](std::ostream& o, const l1t::CaloCluster& c) {
      o << " isValid=" << c.isValid() << " hwPtEm=" << c.hwPtEm() << " hwPtHad=" << c.hwPtHad() << " hwSeedPt=" << c.hwSeedPt() << " fgECAL=" << c.fgECAL();
    }, nullptr, nullptr, false);

  if (!algBlkTag_.label().empty()) {
    edm::Handle<BXVector<GlobalAlgBlk>> h;
    ev.getByToken(algBlkTok_, h);
    if (!h.isValid())
      missing(os, "uGT GlobalAlgBlk", algBlkTag_);
    else {
      std::map<unsigned, std::string> names;
      const L1TUtmTriggerMenu& menu = es.getData(menuTok_);
      for (auto const& kv : menu.getAlgorithmMap())
        names[kv.second.getIndex()] = kv.first;
      section(os, "uGT GlobalAlgBlk", algBlkTag_, h->size());
      os << "  menu=" << menu.getName() << " nAlgos=" << names.size() << "\n";
      for (int bx = h->getFirstBX(); bx <= h->getLastBX(); ++bx) {
        for (auto it = h->begin(bx); it != h->end(bx); ++it) {
          const GlobalAlgBlk& b = *it;
          const auto& ini = b.getAlgoDecisionInitial();
          const auto& intm = b.getAlgoDecisionInterm();
          const auto& fin = b.getAlgoDecisionFinal();
          int nIni = 0, nFin = 0;
          for (size_t i = 0; i < fin.size(); ++i) {
            nIni += (i < ini.size() && ini[i]);
            nFin += fin[i];
          }
          os << "  bx=" << bx << " finalOR=" << b.getFinalOR() << " finalORPreVeto=" << b.getFinalORPreVeto() << " finalORVeto=" << b.getFinalORVeto()
             << " prescaleColumn=" << b.getPreScColumn() << " nAlgoInitial=" << nIni << " nAlgoFinal=" << nFin << "\n";
          if (bx == 0 || printAllAlgos_) {
            os << "    columns: bit name initial intermediate(after prescale) final\n";
            for (size_t i = 0; i < fin.size(); ++i) {
              bool any = (i < ini.size() && ini[i]) || (i < intm.size() && intm[i]) || fin[i];
              if (!any && !printAllAlgos_)
                continue;
              auto nm = names.find(i);
              os << "    bit=" << i << " " << (nm != names.end() ? nm->second : std::string("?")) << " initial=" << (i < ini.size() ? ini[i] : 0)
                 << " interm=" << (i < intm.size() ? intm[i] : 0) << " final=" << fin[i] << "\n";
            }
          }
        }
      }
    }
  }
  os.flush();
}

DEFINE_FWK_MODULE(L1Dumper);
