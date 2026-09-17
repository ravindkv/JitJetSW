// RecoDumper: reconstructed objects (HLT or offline). Vertices, every track (with the
// true TrackingParticle -> genParticle link when an association is available, otherwise a
// Delta-R gen match), every PF candidate, every jet with constituents and gen-jet match,
// METs, leptons/photons/taus (generic reco::Candidate view) and event-level doubles (rho).
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/Common/interface/View.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/HitPattern.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/JetReco/interface/Jet.h"
#include "DataFormats/JetReco/interface/PFJet.h"
#include "DataFormats/JetReco/interface/CaloJet.h"
#include "DataFormats/METReco/interface/MET.h"
#include "DataFormats/METReco/interface/PFMET.h"
#include "DataFormats/METReco/interface/CaloMET.h"
#include "SimDataFormats/Associations/interface/TrackToTrackingParticleAssociator.h"
#include "SimDataFormats/TrackingAnalysis/interface/TrackingParticle.h"

#include "DumpUtil.h"

using namespace chaindump;

class RecoDumper : public edm::one::EDAnalyzer<> {
public:
  explicit RecoDumper(const edm::ParameterSet& ps);
  void analyze(const edm::Event& ev, const edm::EventSetup&) override;

private:
  std::string step_;
  int maxElements_, maxSub_;
  std::string recoToSimTrackLabel_;
  edm::InputTag genParticlesTag_, genJetsTag_, recoToSimTag_;
  edm::EDGetTokenT<reco::GenParticleCollection> genParticlesTok_;
  edm::EDGetTokenT<reco::GenJetCollection> genJetsTok_;
  edm::EDGetTokenT<reco::RecoToSimCollection> recoToSimTok_;
  std::vector<Tagged<reco::VertexCollection>> vertices_;
  std::vector<Tagged<edm::View<reco::Track>>> tracks_;
  std::vector<Tagged<reco::PFCandidateCollection>> pfCands_;
  std::vector<Tagged<edm::View<reco::Jet>>> jets_;
  std::vector<Tagged<edm::View<reco::MET>>> mets_;
  std::vector<Tagged<edm::View<reco::Candidate>>> candidates_;
  std::vector<Tagged<double>> doubles_;
};

RecoDumper::RecoDumper(const edm::ParameterSet& ps)
    : step_(optString(ps, "step", "RECO")),
      maxElements_(optInt(ps, "maxElements", -1)),
      maxSub_(optInt(ps, "maxSub", -1)),
      recoToSimTrackLabel_(optString(ps, "recoToSimTrackLabel", "generalTracks")),
      genParticlesTag_(optTag(ps, "genParticles")),
      genJetsTag_(optTag(ps, "genJets")),
      recoToSimTag_(optTag(ps, "recoToSim")) {
  if (!genParticlesTag_.label().empty())
    genParticlesTok_ = consumes<reco::GenParticleCollection>(genParticlesTag_);
  if (!genJetsTag_.label().empty())
    genJetsTok_ = consumes<reco::GenJetCollection>(genJetsTag_);
  if (!recoToSimTag_.label().empty())
    recoToSimTok_ = consumes<reco::RecoToSimCollection>(recoToSimTag_);
  auto reg = [&](auto& vec, const char* name) {
    using T = typename std::decay_t<decltype(vec)>::value_type;
    for (auto const& t : optTags(ps, name))
      vec.push_back(T{t, consumes<typename T::product_type>(t)});
  };
  reg(vertices_, "vertices");
  reg(tracks_, "tracks");
  reg(pfCands_, "pfCandidates");
  reg(jets_, "jets");
  reg(mets_, "mets");
  reg(candidates_, "candidates");
  reg(doubles_, "doubles");
}

void RecoDumper::analyze(const edm::Event& ev, const edm::EventSetup&) {
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
  const reco::RecoToSimCollection* r2s = nullptr;
  edm::Handle<reco::RecoToSimCollection> hr;
  if (!recoToSimTag_.label().empty()) {
    ev.getByToken(recoToSimTok_, hr);
    if (hr.isValid())
      r2s = hr.product();
    else
      missing(os, "RecoToSim track association", recoToSimTag_);
  }

  for (auto const& t : doubles_) {
    edm::Handle<double> h;
    ev.getByToken(t.token, h);
    if (h.isValid())
      os << "\n== double [" << t.tag.encode() << "] = " << *h << "\n";
    else
      missing(os, "double", t.tag);
  }

  for (auto const& t : vertices_) {
    edm::Handle<reco::VertexCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "Vertices", t.tag);
      continue;
    }
    section(os, "Vertices", t.tag, h->size());
    size_t lim = cap(maxElements_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const reco::Vertex& v = (*h)[i];
      os << "  [" << i << "] x=" << v.x() << " y=" << v.y() << " z=" << v.z() << " xErr=" << v.xError() << " yErr=" << v.yError() << " zErr=" << v.zError()
         << " chi2=" << v.chi2() << " ndof=" << v.ndof() << " nTracks=" << v.tracksSize() << " isFake=" << v.isFake() << " isValid=" << v.isValid();
      if (v.hasRefittedTracks() || v.tracksSize())
        os << " sumPt=" << v.p4().pt();
      os << "\n";
    }
    truncated(os, lim, h->size());
  }

  for (auto const& t : tracks_) {
    edm::Handle<edm::View<reco::Track>> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "Tracks", t.tag);
      continue;
    }
    bool useAssoc = r2s && t.tag.label() == recoToSimTrackLabel_;
    section(os, "Tracks", t.tag, h->size());
    os << "  columns: [index] q pt eta phi ptErr chi2 ndof normChi2 nValidHits nLostHits pixelLayers trackerLayers algo highPurity dxy dz vx vy vz | truth link"
       << (useAssoc ? " (TrackingParticle association)" : " (nearest charged status-1 gen particle)") << "\n";
    size_t lim = cap(maxElements_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const reco::Track& tr = (*h)[i];
      os << "  [" << i << "] q=" << tr.charge() << " pt=" << tr.pt() << " eta=" << tr.eta() << " phi=" << tr.phi() << " ptErr=" << tr.ptError() << " chi2=" << tr.chi2()
         << " ndof=" << tr.ndof() << " normChi2=" << tr.normalizedChi2() << " nValidHits=" << tr.numberOfValidHits() << " nLostHits=" << tr.numberOfLostHits()
         << " pixelLayers=" << tr.hitPattern().pixelLayersWithMeasurement() << " trackerLayers=" << tr.hitPattern().trackerLayersWithMeasurement()
         << " algo=" << tr.algoName() << " highPurity=" << tr.quality(reco::TrackBase::highPurity) << " dxy=" << tr.dxy() << " dz=" << tr.dz() << " " << vtx(tr);
      if (useAssoc) {
        edm::RefToBase<reco::Track> ref = h->refAt(i);
        auto it = r2s->find(ref);
        if (it == r2s->end())
          os << " | tpMatch=none";
        else {
          for (auto const& tpq : it->val) {
            const TrackingParticle& tp = *tpq.first;
            os << " | tpKey=" << tpq.first.key() << " tpPdg=" << tp.pdgId() << " tpPt=" << tp.pt() << " tpEta=" << tp.eta() << " tpPhi=" << tp.phi() << " assocQuality=" << tpq.second
               << " ptRatioTrkOverTP=" << (tp.pt() > 0 ? tr.pt() / tp.pt() : 0.) << " tpGenIdx=";
            if (tp.genParticles().empty())
              os << "-";
            for (size_t g = 0; g < tp.genParticles().size(); ++g) {
              os << (g ? "," : "") << tp.genParticles()[g].key();
              if (tp.genParticles()[g].isAvailable())
                os << "(pdg=" << tp.genParticles()[g]->pdgId() << ",pt=" << tp.genParticles()[g]->pt() << ")";
            }
          }
        }
      } else if (gens)
        os << " | " << matchStr(nearestGen(tr.eta(), tr.phi(), *gens, true), "gen", tr.pt());
      os << "\n";
    }
    truncated(os, lim, h->size());
  }

  for (auto const& t : pfCands_) {
    edm::Handle<reco::PFCandidateCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "PFCandidates", t.tag);
      continue;
    }
    section(os, "PFCandidates", t.tag, h->size());
    os << "  columns: [index] pdg particleId(1=h,2=e,3=mu,4=gamma,5=h0,6=h_HF,7=egamma_HF) q kinematics vertex ecalE hcalE hoE rawEcalE rawHcalE pS1E pS2E trackKey trackPt posAtECAL(eta,phi) nBlockElements | gen match\n";
    size_t lim = cap(maxElements_, h->size());
    double sumPx = 0, sumPy = 0, sumEt = 0;
    for (auto const& c : *h) {
      sumPx += c.px();
      sumPy += c.py();
      sumEt += c.et();
    }
    for (size_t i = 0; i < lim; ++i) {
      const reco::PFCandidate& c = (*h)[i];
      os << "  [" << i << "] pdg=" << c.pdgId() << " particleId=" << c.particleId() << " q=" << c.charge() << " " << kin(c) << " " << vtx(c) << " ecalE=" << c.ecalEnergy()
         << " hcalE=" << c.hcalEnergy() << " hoE=" << c.hoEnergy() << " rawEcalE=" << c.rawEcalEnergy() << " rawHcalE=" << c.rawHcalEnergy() << " pS1E=" << c.pS1Energy()
         << " pS2E=" << c.pS2Energy() << " trackKey=" << (c.trackRef().isNonnull() ? static_cast<long>(c.trackRef().key()) : -1L)
         << " trackPt=" << (c.trackRef().isNonnull() && c.trackRef().isAvailable() ? c.trackRef()->pt() : 0.) << " posAtECAL=(" << c.positionAtECALEntrance().eta() << ","
         << c.positionAtECALEntrance().phi() << ") nBlockElements=" << c.elementsInBlocks().size();
      if (gens)
        os << " | " << matchStr(nearestGen(c.eta(), c.phi(), *gens, c.charge() != 0), "gen", c.pt());
      os << "\n";
    }
    truncated(os, lim, h->size());
    os << "  summary: n=" << h->size() << " sumPx=" << sumPx << " sumPy=" << sumPy << " sumEt=" << sumEt << " MET(pt)=" << std::hypot(sumPx, sumPy) << "\n";
  }

  for (auto const& t : jets_) {
    edm::Handle<edm::View<reco::Jet>> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "Jets", t.tag);
      continue;
    }
    section(os, "Jets", t.tag, h->size());
    size_t lim = cap(maxElements_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const reco::Jet& j = (*h)[i];
      os << "  [" << i << "] " << kin(j) << " area=" << j.jetArea() << " nConst=" << j.numberOfDaughters() << " q=" << j.charge();
      if (auto pf = dynamic_cast<const reco::PFJet*>(&j))
        os << " chHadEF=" << pf->chargedHadronEnergyFraction() << " neHadEF=" << pf->neutralHadronEnergyFraction() << " chEmEF=" << pf->chargedEmEnergyFraction()
           << " neEmEF=" << pf->neutralEmEnergyFraction() << " muEF=" << pf->muonEnergyFraction() << " HFHadEF=" << pf->HFHadronEnergyFraction() << " HFEmEF=" << pf->HFEMEnergyFraction()
           << " chMult=" << pf->chargedMultiplicity() << " neMult=" << pf->neutralMultiplicity() << " chHadMult=" << pf->chargedHadronMultiplicity() << " neHadMult=" << pf->neutralHadronMultiplicity()
           << " photonMult=" << pf->photonMultiplicity() << " eMult=" << pf->electronMultiplicity() << " muMult=" << pf->muonMultiplicity();
      else if (auto cj = dynamic_cast<const reco::CaloJet*>(&j))
        os << " emEF=" << cj->emEnergyFraction() << " hadEF=" << cj->energyFractionHadronic() << " maxEInEmTowers=" << cj->maxEInEmTowers() << " maxEInHadTowers=" << cj->maxEInHadTowers()
           << " n90=" << cj->n90() << " n60=" << cj->n60() << " towersArea=" << cj->towersArea();
      if (gjets)
        os << " | " << matchStr(nearestGenJet(j.eta(), j.phi(), *gjets), "genJet", j.pt());
      os << "\n";
      size_t nc = j.numberOfDaughters(), limc = cap(maxSub_, nc);
      os << "      constituents(key:pdg:pt:eta:phi):";
      for (size_t k = 0; k < limc; ++k) {
        reco::CandidatePtr p = j.daughterPtr(k);
        const reco::Candidate* d = j.daughter(k);
        os << " " << (p.isNonnull() ? std::to_string(p.key()) : std::string("?")) << ":" << (d ? d->pdgId() : 0) << ":" << (d ? d->pt() : 0.) << ":" << (d ? d->eta() : 0.) << ":" << (d ? d->phi() : 0.);
      }
      if (limc < nc)
        os << " ...(" << nc - limc << " more)";
      os << "\n";
    }
    truncated(os, lim, h->size());
  }

  for (auto const& t : mets_) {
    edm::Handle<edm::View<reco::MET>> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "MET", t.tag);
      continue;
    }
    section(os, "MET", t.tag, h->size());
    for (size_t i = 0; i < h->size(); ++i) {
      const reco::MET& m = (*h)[i];
      os << "  [" << i << "] pt=" << m.pt() << " phi=" << m.phi() << " px=" << m.px() << " py=" << m.py() << " sumEt=" << m.sumEt() << " mEtSig=" << m.mEtSig();
      if (auto pf = dynamic_cast<const reco::PFMET*>(&m))
        os << " photonEtFrac=" << pf->photonEtFraction() << " neHadEtFrac=" << pf->neutralHadronEtFraction() << " chHadEtFrac=" << pf->chargedHadronEtFraction()
           << " muEtFrac=" << pf->muonEtFraction() << " HFHadEtFrac=" << pf->HFHadronEtFraction() << " HFEMEtFrac=" << pf->HFEMEtFraction();
      else if (auto cm = dynamic_cast<const reco::CaloMET*>(&m))
        os << " maxEtInEmTowers=" << cm->maxEtInEmTowers() << " maxEtInHadTowers=" << cm->maxEtInHadTowers() << " etFractionHadronic=" << cm->etFractionHadronic() << " emEtFraction=" << cm->emEtFraction();
      os << "\n";
    }
  }

  for (auto const& t : candidates_) {
    edm::Handle<edm::View<reco::Candidate>> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "Candidates", t.tag);
      continue;
    }
    section(os, "Candidates", t.tag, h->size());
    size_t lim = cap(maxElements_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const reco::Candidate& c = (*h)[i];
      os << "  [" << i << "] pdg=" << c.pdgId() << " status=" << c.status() << " q=" << c.charge() << " " << kin(c) << " " << vtx(c) << " nDaughters=" << c.numberOfDaughters();
      candidateExtras(os, c);
      if (gens)
        os << " | " << matchStr(nearestGen(c.eta(), c.phi(), *gens, false), "gen", c.pt());
      os << "\n";
    }
    truncated(os, lim, h->size());
  }
  os.flush();
}

DEFINE_FWK_MODULE(RecoDumper);
