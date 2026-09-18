// PatJourney: MiniAOD content. Every packed PF candidate (with the original AOD PF
// candidate it was packed from, to show the packing precision), lost tracks, every pat::Jet
// with all JEC levels, flavour, gen match, b-tag discriminators, user floats and
// constituents, pat::MET with uncertainties, leptons/photons/taus, pruned/packed gen
// particles (mapped back to the AOD genParticles), slimmed gen jets and trigger objects.
#include <map>

#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "DataFormats/Common/interface/View.h"
#include "DataFormats/Common/interface/Association.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include "FWCore/Common/interface/TriggerNames.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/PatCandidates/interface/PackedGenParticle.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/MET.h"
#include "DataFormats/PatCandidates/interface/TriggerObjectStandAlone.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"

#include "JourneyUtil.h"

using namespace jitjet;

class PatJourney : public edm::one::EDAnalyzer<> {
public:
  explicit PatJourney(const edm::ParameterSet& ps);
  void analyze(const edm::Event& ev, const edm::EventSetup&) override;

private:
  std::string step_;
  int maxElements_, maxSub_;
  edm::InputTag genParticlesTag_, genJetsTag_, packedToPFTag_, prunedGenTag_, packedGenTag_, triggerObjectsTag_, triggerResultsTag_;
  edm::EDGetTokenT<reco::GenParticleCollection> genParticlesTok_, prunedGenTok_;
  edm::EDGetTokenT<reco::GenJetCollection> genJetsTok_;
  edm::EDGetTokenT<edm::Association<reco::PFCandidateCollection>> packedToPFTok_;
  edm::EDGetTokenT<pat::PackedGenParticleCollection> packedGenTok_;
  edm::EDGetTokenT<std::vector<pat::TriggerObjectStandAlone>> triggerObjectsTok_;
  edm::EDGetTokenT<edm::TriggerResults> triggerResultsTok_;
  std::vector<Tagged<pat::PackedCandidateCollection>> packed_;
  std::vector<Tagged<reco::VertexCollection>> vertices_;
  std::vector<Tagged<pat::JetCollection>> jets_;
  std::vector<Tagged<pat::METCollection>> mets_;
  std::vector<Tagged<edm::View<reco::Candidate>>> candidates_;
  std::vector<Tagged<reco::GenJetCollection>> slimmedGenJets_;
  std::vector<Tagged<double>> doubles_;
};

PatJourney::PatJourney(const edm::ParameterSet& ps)
    : step_(optString(ps, "step", "PAT")),
      maxElements_(optInt(ps, "maxElements", -1)),
      maxSub_(optInt(ps, "maxSub", -1)),
      genParticlesTag_(optTag(ps, "genParticles")),
      genJetsTag_(optTag(ps, "genJets")),
      packedToPFTag_(optTag(ps, "packedToPF")),
      prunedGenTag_(optTag(ps, "prunedGenParticles")),
      packedGenTag_(optTag(ps, "packedGenParticles")),
      triggerObjectsTag_(optTag(ps, "triggerObjects")),
      triggerResultsTag_(optTag(ps, "triggerResults")) {
  if (!genParticlesTag_.label().empty())
    genParticlesTok_ = consumes<reco::GenParticleCollection>(genParticlesTag_);
  if (!genJetsTag_.label().empty())
    genJetsTok_ = consumes<reco::GenJetCollection>(genJetsTag_);
  if (!packedToPFTag_.label().empty())
    packedToPFTok_ = consumes<edm::Association<reco::PFCandidateCollection>>(packedToPFTag_);
  if (!prunedGenTag_.label().empty())
    prunedGenTok_ = consumes<reco::GenParticleCollection>(prunedGenTag_);
  if (!packedGenTag_.label().empty())
    packedGenTok_ = consumes<pat::PackedGenParticleCollection>(packedGenTag_);
  if (!triggerObjectsTag_.label().empty())
    triggerObjectsTok_ = consumes<std::vector<pat::TriggerObjectStandAlone>>(triggerObjectsTag_);
  if (!triggerResultsTag_.label().empty())
    triggerResultsTok_ = consumes<edm::TriggerResults>(triggerResultsTag_);
  auto reg = [&](auto& vec, const char* name) {
    using T = typename std::decay_t<decltype(vec)>::value_type;
    for (auto const& t : optTags(ps, name))
      vec.push_back(T{t, consumes<typename T::product_type>(t)});
  };
  reg(packed_, "packedCandidates");
  reg(vertices_, "vertices");
  reg(jets_, "jets");
  reg(mets_, "mets");
  reg(candidates_, "candidates");
  reg(slimmedGenJets_, "slimmedGenJets");
  reg(doubles_, "doubles");
}

namespace {
  // exact-ish match of a (pruned/packed) gen copy to the original AOD genParticles
  int originalGenIndex(const reco::Candidate& c, const reco::GenParticleCollection* gens, double relPt) {
    if (!gens)
      return -1;
    int best = -1;
    double bestDR = 1e-3;
    for (size_t i = 0; i < gens->size(); ++i) {
      const auto& g = (*gens)[i];
      if (g.pdgId() != c.pdgId() || g.status() != c.status())
        continue;
      if (std::abs(g.pt() - c.pt()) > relPt * std::max(g.pt(), 1e-3))
        continue;
      double dr = reco::deltaR(g, c);
      if (dr < bestDR) {
        bestDR = dr;
        best = i;
      }
    }
    return best;
  }
}  // namespace

void PatJourney::analyze(const edm::Event& ev, const edm::EventSetup&) {
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

  for (auto const& t : doubles_) {
    edm::Handle<double> h;
    ev.getByToken(t.token, h);
    if (h.isValid())
      os << "\n== double [" << t.tag.encode() << "] = " << *h << "\n";
    else
      missing(os, "double", t.tag);
  }

  // gen copies kept in MiniAOD
  if (!prunedGenTag_.label().empty()) {
    edm::Handle<reco::GenParticleCollection> h;
    ev.getByToken(prunedGenTok_, h);
    if (!h.isValid())
      missing(os, "prunedGenParticles", prunedGenTag_);
    else {
      section(os, "prunedGenParticles", prunedGenTag_, h->size());
      os << "  columns: as GenParticles (mother/daughter indices refer to the pruned collection); origGenIdx = index in the AOD genParticles collection\n";
      size_t lim = cap(maxElements_, h->size());
      for (size_t i = 0; i < lim; ++i) {
        os << "  origGenIdx=" << originalGenIndex((*h)[i], gens, 1e-4);
        genParticleLine(os, i, (*h)[i], nullptr);
      }
      truncated(os, lim, h->size());
    }
  }
  if (!packedGenTag_.label().empty()) {
    edm::Handle<pat::PackedGenParticleCollection> h;
    ev.getByToken(packedGenTok_, h);
    if (!h.isValid())
      missing(os, "packedGenParticles", packedGenTag_);
    else {
      section(os, "packedGenParticles", packedGenTag_, h->size());
      os << "  columns: [index] pdg status q kinematics(packed precision) prunedMotherIdx origGenIdx(+orig pt, ptRatio packed/orig)\n";
      size_t lim = cap(maxElements_, h->size());
      for (size_t i = 0; i < lim; ++i) {
        const pat::PackedGenParticle& p = (*h)[i];
        int orig = originalGenIndex(p, gens, 0.05);
        os << "  [" << i << "] pdg=" << p.pdgId() << " status=" << p.status() << " q=" << p.charge() << " " << kin(p) << " prunedMotherIdx="
           << (p.motherRef().isNonnull() ? static_cast<long>(p.motherRef().key()) : -1L) << " origGenIdx=" << orig;
        if (orig >= 0)
          os << " origPt=" << (*gens)[orig].pt() << " ptRatio=" << p.pt() / (*gens)[orig].pt() << " dEta=" << p.eta() - (*gens)[orig].eta() << " dPhi=" << reco::deltaPhi(p.phi(), (*gens)[orig].phi());
        os << "\n";
      }
      truncated(os, lim, h->size());
    }
  }
  for (auto const& t : slimmedGenJets_) {
    edm::Handle<reco::GenJetCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "slimmedGenJets", t.tag);
      continue;
    }
    section(os, "slimmedGenJets", t.tag, h->size());
    size_t lim = cap(maxElements_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const reco::GenJet& j = (*h)[i];
      os << "  [" << i << "] " << kin(j) << " area=" << j.jetArea() << " emE=" << j.emEnergy() << " hadE=" << j.hadEnergy() << " invisibleE=" << j.invisibleEnergy() << " nConst=" << j.numberOfDaughters();
      if (gjets)
        os << " | " << matchStr(nearestGenJet(j.eta(), j.phi(), *gjets), "aodGenJet", j.pt());
      os << "\n      constituents(packedGenKey:pdg:pt):";
      size_t nc = j.numberOfDaughters(), limc = cap(maxSub_, nc);
      for (size_t k = 0; k < limc; ++k)
        os << " " << (j.daughterPtr(k).isNonnull() ? std::to_string(j.daughterPtr(k).key()) : std::string("?")) << ":" << j.daughter(k)->pdgId() << ":" << j.daughter(k)->pt();
      if (limc < nc)
        os << " ...(" << nc - limc << " more)";
      os << "\n";
    }
    truncated(os, lim, h->size());
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
      os << "  [" << i << "] x=" << v.x() << " y=" << v.y() << " z=" << v.z() << " xErr=" << v.xError() << " yErr=" << v.yError() << " zErr=" << v.zError() << " chi2=" << v.chi2() << " ndof=" << v.ndof()
         << " nTracks=" << v.tracksSize() << " isFake=" << v.isFake() << "\n";
    }
    truncated(os, lim, h->size());
  }

  const edm::Association<reco::PFCandidateCollection>* p2pf = nullptr;
  edm::Handle<edm::Association<reco::PFCandidateCollection>> hp;
  if (!packedToPFTag_.label().empty()) {
    ev.getByToken(packedToPFTok_, hp);
    if (hp.isValid())
      p2pf = hp.product();
    else
      missing(os, "packed->PF association", packedToPFTag_);
  }
  for (auto const& t : packed_) {
    edm::Handle<pat::PackedCandidateCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "PackedCandidates", t.tag);
      continue;
    }
    bool useAssoc = p2pf && t.tag.label() == packedToPFTag_.label();
    section(os, "PackedCandidates", t.tag, h->size());
    os << "  columns: [index] pdg q kinematics vertex dz dxy dzErr dxyErr fromPV pvAssocQuality lostInnerHits highPurity hasTrackDetails nHits nPixelHits trackerLayers trkAlgo ptTrk normChi2 puppiWeight puppiWeightNoLep hcalFraction rawCaloFraction caloFraction isIsoChHad"
       << (useAssoc ? " | original AOD PF candidate (pfKey pfPt ptRatio dEta dPhi)" : "") << " | gen match\n";
    size_t lim = cap(maxElements_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const pat::PackedCandidate& c = (*h)[i];
      os << "  [" << i << "] pdg=" << c.pdgId() << " q=" << c.charge() << " " << kin(c) << " " << vtx(c) << " dz=" << c.dz() << " dxy=" << c.dxy();
      if (c.hasTrackDetails())
        os << " dzErr=" << c.dzError() << " dxyErr=" << c.dxyError();
      else
        os << " dzErr=n/a dxyErr=n/a";
      os << " fromPV=" << c.fromPV() << " pvAssocQuality=" << c.pvAssociationQuality() << " lostInnerHits=" << c.lostInnerHits() << " highPurity=" << c.trackHighPurity()
         << " hasTrackDetails=" << c.hasTrackDetails() << " nHits=" << c.numberOfHits() << " nPixelHits=" << c.numberOfPixelHits() << " trackerLayers=" << c.trackerLayersWithMeasurement()
         << " trkAlgo=" << static_cast<int>(c.trkAlgo()) << " ptTrk=" << c.ptTrk();
      if (c.hasTrackDetails())
        os << " normChi2=" << c.pseudoTrack().normalizedChi2();
      os << " puppiWeight=" << c.puppiWeight() << " puppiWeightNoLep=" << c.puppiWeightNoLep() << " hcalFraction=" << c.hcalFraction() << " rawCaloFraction=" << c.rawCaloFraction()
         << " caloFraction=" << c.caloFraction() << " isIsoChHad=" << c.isIsolatedChargedHadron();
      if (useAssoc) {
        edm::Ref<pat::PackedCandidateCollection> ref(h, i);
        reco::PFCandidateRef pf = (*p2pf)[ref];
        if (pf.isNonnull() && pf.isAvailable())
          os << " | pfKey=" << pf.key() << " pfPt=" << pf->pt() << " ptRatio=" << (pf->pt() > 0 ? c.pt() / pf->pt() : 0.) << " dEta=" << c.eta() - pf->eta() << " dPhi=" << reco::deltaPhi(c.phi(), pf->phi())
             << " dM=" << c.mass() - pf->mass();
        else
          os << " | pfKey=" << (pf.isNonnull() ? static_cast<long>(pf.key()) : -1L);
      }
      if (gens)
        os << " | " << matchStr(nearestGen(c.eta(), c.phi(), *gens, c.charge() != 0), "gen", c.pt());
      os << "\n";
    }
    truncated(os, lim, h->size());
  }

  for (auto const& t : jets_) {
    edm::Handle<pat::JetCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "pat::Jets", t.tag);
      continue;
    }
    section(os, "pat::Jets", t.tag, h->size());
    size_t lim = cap(maxElements_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const pat::Jet& j = (*h)[i];
      os << "  [" << i << "] " << kin(j) << " area=" << j.jetArea() << " nConst=" << j.numberOfDaughters() << " jetCharge=" << j.jetCharge() << " partonFlavour=" << j.partonFlavour()
         << " hadronFlavour=" << j.hadronFlavour() << " nBHadrons=" << j.jetFlavourInfo().getbHadrons().size() << " nCHadrons=" << j.jetFlavourInfo().getcHadrons().size()
         << " isPFJet=" << j.isPFJet() << " isCaloJet=" << j.isCaloJet();
      if (j.isPFJet())
        os << " chHadEF=" << j.chargedHadronEnergyFraction() << " neHadEF=" << j.neutralHadronEnergyFraction() << " chEmEF=" << j.chargedEmEnergyFraction() << " neEmEF=" << j.neutralEmEnergyFraction()
           << " muEF=" << j.muonEnergyFraction() << " HFHadEF=" << j.HFHadronEnergyFraction() << " HFEmEF=" << j.HFEMEnergyFraction() << " chMult=" << j.chargedMultiplicity()
           << " neMult=" << j.neutralMultiplicity() << " chHadMult=" << j.chargedHadronMultiplicity() << " neHadMult=" << j.neutralHadronMultiplicity() << " photonMult=" << j.photonMultiplicity()
           << " eMult=" << j.electronMultiplicity() << " muMult=" << j.muonMultiplicity();
      os << "\n";
      // JEC chain
      os << "      JEC: currentLevel=" << j.currentJECLevel();
      if (j.jecSetsAvailable()) {
        os << " set=" << j.currentJECSet() << " levels(pt|E|factorRelToCurrent):";
        for (auto const& lev : j.availableJECLevels()) {
          try {
            auto p4 = j.correctedP4(lev);
            os << " " << lev << "=" << p4.pt() << "|" << p4.energy() << "|" << j.jecFactor(lev);
          } catch (cms::Exception const&) {
            os << " " << lev << "=?";
          }
        }
      }
      if (j.genJet())
        os << "\n      genJet: pt=" << j.genJet()->pt() << " eta=" << j.genJet()->eta() << " phi=" << j.genJet()->phi() << " m=" << j.genJet()->mass() << " dR=" << reco::deltaR(j, *j.genJet())
           << " ptRatio=" << (j.genJet()->pt() > 0 ? j.pt() / j.genJet()->pt() : 0.) << " ptRatioRaw=" << (j.genJet()->pt() > 0 ? j.correctedP4("Uncorrected").pt() / j.genJet()->pt() : 0.);
      else
        os << "\n      genJet: none";
      if (gjets)
        os << " | " << matchStr(nearestGenJet(j.eta(), j.phi(), *gjets), "nearestGenJet", j.pt());
      if (j.genParton())
        os << " genParton: pdg=" << j.genParton()->pdgId() << " pt=" << j.genParton()->pt() << " eta=" << j.genParton()->eta() << " phi=" << j.genParton()->phi();
      os << "\n      discriminators:";
      for (auto const& d : j.getPairDiscri())
        os << " " << d.first << "=" << d.second;
      os << "\n      userFloats:";
      for (auto const& n : j.userFloatNames())
        os << " " << n << "=" << j.userFloat(n);
      os << "\n      userInts:";
      for (auto const& n : j.userIntNames())
        os << " " << n << "=" << j.userInt(n);
      os << "\n      constituents(packedKey:pdg:pt:eta:phi:puppiWeight):";
      size_t nc = j.numberOfDaughters(), limc = cap(maxSub_, nc);
      for (size_t k = 0; k < limc; ++k) {
        reco::CandidatePtr p = j.daughterPtr(k);
        const reco::Candidate* d = j.daughter(k);
        os << " " << (p.isNonnull() ? std::to_string(p.key()) : std::string("?")) << ":" << (d ? d->pdgId() : 0) << ":" << (d ? d->pt() : 0.) << ":" << (d ? d->eta() : 0.) << ":" << (d ? d->phi() : 0.);
        if (auto pc = dynamic_cast<const pat::PackedCandidate*>(d))
          os << ":" << pc->puppiWeight();
      }
      if (limc < nc)
        os << " ...(" << nc - limc << " more)";
      os << "\n";
    }
    truncated(os, lim, h->size());
  }

  for (auto const& t : mets_) {
    edm::Handle<pat::METCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "pat::MET", t.tag);
      continue;
    }
    section(os, "pat::MET", t.tag, h->size());
    for (size_t i = 0; i < h->size(); ++i) {
      const pat::MET& m = (*h)[i];
      os << "  [" << i << "] pt=" << m.pt() << " phi=" << m.phi() << " px=" << m.px() << " py=" << m.py() << " sumEt=" << m.sumEt() << " significance=" << m.metSignificance()
         << " uncorPt=" << m.uncorPt() << " uncorPhi=" << m.uncorPhi() << " uncorSumEt=" << m.uncorSumEt();
      if (m.genMET())
        os << " genMetPt=" << m.genMET()->pt() << " genMetPhi=" << m.genMET()->phi();
      if (m.isPFMET())
        os << " photonEtFrac=" << m.NeutralEMFraction() << " neHadEtFrac=" << m.NeutralHadEtFraction() << " chHadEtFrac=" << m.ChargedHadEtFraction() << " muEtFrac=" << m.MuonEtFraction();
      os << "\n      shifted(pt,phi):";
      const std::pair<const char*, pat::MET::METUncertainty> uncs[] = {{"JetResUp", pat::MET::JetResUp}, {"JetResDown", pat::MET::JetResDown}, {"JetEnUp", pat::MET::JetEnUp},
                                                                       {"JetEnDown", pat::MET::JetEnDown}, {"MuonEnUp", pat::MET::MuonEnUp}, {"MuonEnDown", pat::MET::MuonEnDown},
                                                                       {"ElectronEnUp", pat::MET::ElectronEnUp}, {"ElectronEnDown", pat::MET::ElectronEnDown}, {"TauEnUp", pat::MET::TauEnUp},
                                                                       {"TauEnDown", pat::MET::TauEnDown}, {"UnclusteredEnUp", pat::MET::UnclusteredEnUp}, {"UnclusteredEnDown", pat::MET::UnclusteredEnDown},
                                                                       {"PhotonEnUp", pat::MET::PhotonEnUp}, {"PhotonEnDown", pat::MET::PhotonEnDown}};
      for (auto const& u : uncs) {
        try {
          os << " " << u.first << "=" << m.shiftedPt(u.second) << "," << m.shiftedPhi(u.second);
        } catch (cms::Exception const&) {
          os << " " << u.first << "=n/a";
        }
      }
      os << "\n      corrected(pt,phi):";
      const std::pair<const char*, pat::MET::METCorrectionLevel> levs[] = {{"Raw", pat::MET::Raw}, {"Type1", pat::MET::Type1}, {"Type01", pat::MET::Type01}, {"TypeXY", pat::MET::TypeXY},
                                                                           {"Type1XY", pat::MET::Type1XY}, {"Type01XY", pat::MET::Type01XY}, {"Type1Smear", pat::MET::Type1Smear}};
      for (auto const& l : levs) {
        try {
          os << " " << l.first << "=" << m.corPt(l.second) << "," << m.corPhi(l.second);
        } catch (cms::Exception const&) {
          os << " " << l.first << "=n/a";
        }
      }
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

  if (!triggerObjectsTag_.label().empty()) {
    edm::Handle<std::vector<pat::TriggerObjectStandAlone>> h;
    ev.getByToken(triggerObjectsTok_, h);
    edm::Handle<edm::TriggerResults> hr;
    if (!triggerResultsTag_.label().empty())
      ev.getByToken(triggerResultsTok_, hr);
    if (!h.isValid())
      missing(os, "TriggerObjectStandAlone", triggerObjectsTag_);
    else {
      section(os, "TriggerObjectStandAlone", triggerObjectsTag_, h->size());
      if (hr.isValid()) {
        const edm::TriggerNames& names = ev.triggerNames(*hr);
        size_t nAcc = 0;
        for (size_t i = 0; i < hr->size(); ++i)
          nAcc += hr->accept(i);
        os << "  TriggerResults[" << triggerResultsTag_.encode() << "] nPaths=" << hr->size() << " nAccepted=" << nAcc << " accepted:";
        for (size_t i = 0; i < hr->size(); ++i)
          if (hr->accept(i))
            os << " " << names.triggerName(i);
        os << "\n";
      }
      size_t lim = cap(maxElements_, h->size());
      for (size_t i = 0; i < lim; ++i) {
        pat::TriggerObjectStandAlone o = (*h)[i];
        if (hr.isValid()) {
          o.unpackNamesAndLabels(ev, *hr);
          o.unpackFilterLabels(ev, *hr);
        }
        os << "  [" << i << "] collection=" << o.collection() << " " << kin(o) << " ids=";
        for (size_t k = 0; k < o.filterIds().size(); ++k)
          os << (k ? "," : "") << o.filterIds()[k];
        os << " filters=";
        for (size_t k = 0; k < o.filterLabels().size(); ++k)
          os << (k ? "," : "") << o.filterLabels()[k];
        os << " paths=";
        auto paths = o.pathNames(false, false);
        for (size_t k = 0; k < paths.size(); ++k)
          os << (k ? "," : "") << paths[k];
        if (gjets && o.hasTriggerObjectType(trigger::TriggerJet))
          os << " | " << matchStr(nearestGenJet(o.eta(), o.phi(), *gjets), "genJet", o.pt());
        else if (gens && o.pt() > 0)
          os << " | " << matchStr(nearestGen(o.eta(), o.phi(), *gens, false), "gen", o.pt());
        os << "\n";
      }
      truncated(os, lim, h->size());
    }
  }
  os.flush();
}

DEFINE_FWK_MODULE(PatJourney);
