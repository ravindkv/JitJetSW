// TriggerJourney: HLT path results (all paths, with the index of the module that stopped
// the path) and the full trigger summary: every trigger object with kinematics, every
// collection and every filter with the objects it accepted, Delta-R matched to gen.
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Common/interface/TriggerNames.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include "DataFormats/HLTReco/interface/TriggerEvent.h"
#include "DataFormats/HLTReco/interface/TriggerObject.h"

#include "JourneyUtil.h"

using namespace jitjet;

class TriggerJourney : public edm::one::EDAnalyzer<> {
public:
  explicit TriggerJourney(const edm::ParameterSet& ps);
  void analyze(const edm::Event& ev, const edm::EventSetup&) override;

private:
  std::string step_;
  int maxElements_;
  bool onlyAccepted_;
  edm::InputTag resultsTag_, summaryTag_, genParticlesTag_, genJetsTag_;
  edm::EDGetTokenT<edm::TriggerResults> resultsTok_;
  edm::EDGetTokenT<trigger::TriggerEvent> summaryTok_;
  edm::EDGetTokenT<reco::GenParticleCollection> genParticlesTok_;
  edm::EDGetTokenT<reco::GenJetCollection> genJetsTok_;
};

TriggerJourney::TriggerJourney(const edm::ParameterSet& ps)
    : step_(optString(ps, "step", "HLT")),
      maxElements_(optInt(ps, "maxElements", -1)),
      onlyAccepted_(optBool(ps, "onlyAcceptedPaths", false)),
      resultsTag_(optTag(ps, "triggerResults")),
      summaryTag_(optTag(ps, "triggerSummary")),
      genParticlesTag_(optTag(ps, "genParticles")),
      genJetsTag_(optTag(ps, "genJets")) {
  if (!resultsTag_.label().empty())
    resultsTok_ = consumes<edm::TriggerResults>(resultsTag_);
  if (!summaryTag_.label().empty())
    summaryTok_ = consumes<trigger::TriggerEvent>(summaryTag_);
  if (!genParticlesTag_.label().empty())
    genParticlesTok_ = consumes<reco::GenParticleCollection>(genParticlesTag_);
  if (!genJetsTag_.label().empty())
    genJetsTok_ = consumes<reco::GenJetCollection>(genJetsTag_);
}

void TriggerJourney::analyze(const edm::Event& ev, const edm::EventSetup&) {
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

  if (!resultsTag_.label().empty()) {
    edm::Handle<edm::TriggerResults> h;
    ev.getByToken(resultsTok_, h);
    if (!h.isValid())
      missing(os, "TriggerResults", resultsTag_);
    else {
      const edm::TriggerNames& names = ev.triggerNames(*h);
      size_t nAcc = 0;
      for (size_t i = 0; i < h->size(); ++i)
        nAcc += h->accept(i);
      section(os, "TriggerResults", resultsTag_, h->size());
      os << "  nAccepted=" << nAcc << " anyAccepted=" << h->accept() << (onlyAccepted_ ? " (only accepted paths listed)" : "") << "\n";
      os << "  columns: [index] path wasrun accept error lastModuleIndex(index of module that made the decision)\n";
      for (size_t i = 0; i < h->size(); ++i) {
        if (onlyAccepted_ && !h->accept(i))
          continue;
        os << "  [" << i << "] " << names.triggerName(i) << " wasrun=" << h->wasrun(i) << " accept=" << h->accept(i) << " error=" << h->error(i) << " lastModule=" << h->index(i) << "\n";
      }
    }
  }

  if (!summaryTag_.label().empty()) {
    edm::Handle<trigger::TriggerEvent> h;
    ev.getByToken(summaryTok_, h);
    if (!h.isValid())
      missing(os, "TriggerEvent (hltTriggerSummaryAOD)", summaryTag_);
    else {
      const trigger::TriggerEvent& te = *h;
      section(os, "TriggerEvent (hltTriggerSummaryAOD)", summaryTag_, te.sizeObjects());
      os << "  usedProcessName=" << te.usedProcessName() << " nCollections=" << te.sizeCollections() << " nObjects=" << te.sizeObjects() << " nFilters=" << te.sizeFilters() << "\n";
      const trigger::TriggerObjectCollection& objs = te.getObjects();
      // collections: object key ranges
      os << "  -- collections (objects [firstKey,lastKey))\n";
      trigger::size_type first = 0;
      for (trigger::size_type c = 0; c < te.sizeCollections(); ++c) {
        os << "    [" << c << "] " << te.collectionTag(c).encode() << " keys=[" << first << "," << te.collectionKey(c) << ") n=" << te.collectionKey(c) - first << "\n";
        first = te.collectionKey(c);
      }
      os << "  -- objects\n  columns: [key] id(trigger type) pt eta phi m genMatch\n";
      size_t lim = cap(maxElements_, objs.size());
      for (size_t k = 0; k < lim; ++k) {
        const trigger::TriggerObject& o = objs[k];
        os << "    [" << k << "] id=" << o.id() << " pt=" << o.pt() << " eta=" << o.eta() << " phi=" << o.phi() << " m=" << o.mass();
        int aid = std::abs(o.id());
        bool isJetLike = (aid == trigger::TriggerJet || aid == trigger::TriggerBJet || aid == trigger::TriggerL1Jet || aid == trigger::TriggerL1CenJet || aid == trigger::TriggerL1ForJet || aid == trigger::TriggerL1TauJet);
        if (o.pt() > 0) {
          if (isJetLike && gjets)
            os << " " << matchStr(nearestGenJet(o.eta(), o.phi(), *gjets), "genJet", o.pt());
          else if (!isJetLike && gens && aid != trigger::TriggerMET && aid != trigger::TriggerTHT && aid != trigger::TriggerL1ETM && aid != trigger::TriggerL1HTT && aid != trigger::TriggerL1ETT && aid != trigger::TriggerMHT && aid != trigger::TriggerTET)
            os << " " << matchStr(nearestGen(o.eta(), o.phi(), *gens, false), "gen", o.pt());
        }
        os << "\n";
      }
      truncated(os, lim, objs.size(), "    ");
      os << "  -- filters (objects passing each filter)\n";
      for (trigger::size_type f = 0; f < te.sizeFilters(); ++f) {
        const trigger::Keys& keys = te.filterKeys(f);
        const trigger::Vids& ids = te.filterIds(f);
        os << "    [" << f << "] " << te.filterTag(f).encode() << " nObjects=" << keys.size() << " objects(key:id:pt:eta:phi)=";
        for (size_t k = 0; k < keys.size(); ++k) {
          const trigger::TriggerObject& o = objs[keys[k]];
          os << (k ? " " : "") << keys[k] << ":" << (k < ids.size() ? ids[k] : 0) << ":" << o.pt() << ":" << o.eta() << ":" << o.phi();
        }
        os << "\n";
      }
    }
  }
  os.flush();
}

DEFINE_FWK_MODULE(TriggerJourney);
