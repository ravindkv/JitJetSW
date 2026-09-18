#ifndef JitJetSW_Journey_JourneyUtil_h
#define JitJetSW_Journey_JourneyUtil_h
// Small shared helpers for the JitJet per-event text dumps.
// Everything is printed to std::cout; the driver script redirects cmsRun output to the step log.
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/EgammaCandidates/interface/GsfElectron.h"
#include "DataFormats/EgammaCandidates/interface/Photon.h"
#include "DataFormats/RecoCandidate/interface/RecoEcalCandidate.h"
#include "DataFormats/EgammaReco/interface/SuperCluster.h"
#include "DataFormats/GsfTrackReco/interface/GsfTrack.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/MuonReco/interface/MuonPFIsolation.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"

namespace jitjet {

  template <class T>
  struct Tagged {
    using product_type = T;
    edm::InputTag tag;
    edm::EDGetTokenT<T> token;
  };

  inline edm::InputTag optTag(const edm::ParameterSet& ps, const std::string& name) {
    if (ps.existsAs<edm::InputTag>(name, true))
      return ps.getParameter<edm::InputTag>(name);
    if (ps.existsAs<edm::InputTag>(name, false))
      return ps.getUntrackedParameter<edm::InputTag>(name);
    return edm::InputTag();
  }

  inline std::vector<edm::InputTag> optTags(const edm::ParameterSet& ps, const std::string& name) {
    if (ps.existsAs<std::vector<edm::InputTag>>(name, true))
      return ps.getParameter<std::vector<edm::InputTag>>(name);
    if (ps.existsAs<std::vector<edm::InputTag>>(name, false))
      return ps.getUntrackedParameter<std::vector<edm::InputTag>>(name);
    return {};
  }

  inline int optInt(const edm::ParameterSet& ps, const std::string& name, int def) {
    if (ps.existsAs<int>(name, true))
      return ps.getParameter<int>(name);
    if (ps.existsAs<int>(name, false))
      return ps.getUntrackedParameter<int>(name);
    return def;
  }

  inline bool optBool(const edm::ParameterSet& ps, const std::string& name, bool def) {
    if (ps.existsAs<bool>(name, true))
      return ps.getParameter<bool>(name);
    if (ps.existsAs<bool>(name, false))
      return ps.getUntrackedParameter<bool>(name);
    return def;
  }

  inline std::string optString(const edm::ParameterSet& ps, const std::string& name, const std::string& def) {
    if (ps.existsAs<std::string>(name, true))
      return ps.getParameter<std::string>(name);
    if (ps.existsAs<std::string>(name, false))
      return ps.getUntrackedParameter<std::string>(name);
    return def;
  }

  // ---------------------------------------------------------------- formatting
  inline void eventHeader(std::ostream& os, const edm::Event& ev, const std::string& step, const std::string& module) {
    os << std::defaultfloat << std::setprecision(7);
    os << "\n################################################################################################\n"
       << "#### JITJET step=" << step << " module=" << module << " run=" << ev.id().run()
       << " lumi=" << ev.luminosityBlock() << " event=" << ev.id().event() << "\n"
       << "################################################################################################\n";
  }

  inline void section(std::ostream& os, const std::string& name, const edm::InputTag& tag, long n) {
    os << std::defaultfloat << std::setprecision(7) << "\n== " << name << " [" << tag.encode() << "] n=" << n << "\n";
  }

  inline void missing(std::ostream& os, const std::string& name, const edm::InputTag& tag) {
    os << "\n== " << name << " [" << tag.encode() << "] : product not available in this event\n";
  }

  // Number of elements to print given a user cap (negative = unlimited).
  inline size_t cap(int max, size_t n) { return max < 0 ? n : std::min<size_t>(static_cast<size_t>(max), n); }

  inline void truncated(std::ostream& os, size_t printed, size_t total, const std::string& indent = "  ") {
    if (printed < total)
      os << indent << "... " << (total - printed) << " more not printed (raise maxElements/maxSub)\n";
  }

  // Kinematics of anything with pt/eta/phi/mass/energy/px/py/pz (reco::Candidate, LorentzVector-like...).
  template <class T>
  std::string kin(const T& c) {
    std::ostringstream s;
    s << std::setprecision(7) << "pt=" << c.pt() << " eta=" << c.eta() << " phi=" << c.phi() << " m=" << c.mass()
      << " E=" << c.energy() << " px=" << c.px() << " py=" << c.py() << " pz=" << c.pz();
    return s.str();
  }

  // Kinematics for ROOT::Math LorentzVectors (no energy()/mass() with those names).
  template <class LV>
  std::string kinLV(const LV& v) {
    std::ostringstream s;
    s << std::setprecision(7) << "pt=" << v.Pt() << " eta=" << (v.P() > std::abs(v.Pz()) ? v.Eta() : 0.)
      << " phi=" << v.Phi() << " m=" << v.M() << " E=" << v.E() << " px=" << v.Px() << " py=" << v.Py()
      << " pz=" << v.Pz();
    return s.str();
  }

  template <class T>
  std::string vtx(const T& c) {
    std::ostringstream s;
    s << std::setprecision(7) << "vx=" << c.vx() << " vy=" << c.vy() << " vz=" << c.vz();
    return s.str();
  }

  // ------------------------------------------------------------ gen matching
  struct Match {
    int idx = -1;
    double dR = 999.;
    double pt = 0.;
    int pdg = 0;
    int status = 0;
  };

  template <class Coll, class Pred>
  Match nearest(double eta, double phi, const Coll& coll, Pred pred) {
    Match m;
    for (size_t i = 0; i < coll.size(); ++i) {
      const auto& c = coll[i];
      if (!pred(c))
        continue;
      double dr = reco::deltaR(eta, phi, c.eta(), c.phi());
      if (dr < m.dR) {
        m.dR = dr;
        m.idx = i;
        m.pt = c.pt();
        m.pdg = c.pdgId();
        m.status = c.status();
      }
    }
    return m;
  }

  inline std::string matchStr(const Match& m, const char* label, double pt) {
    std::ostringstream s;
    s << std::setprecision(7);
    if (m.idx < 0) {
      s << label << "Match=none";
      return s.str();
    }
    s << label << "Idx=" << m.idx << " " << label << "Pdg=" << m.pdg << " " << label << "Pt=" << m.pt
      << " dR=" << m.dR << " ptRatio=" << (m.pt > 0 ? pt / m.pt : 0.);
    return s.str();
  }

  // Nearest status-1 gen particle (optionally charged only). Neutrinos are skipped.
  inline Match nearestGen(double eta, double phi, const reco::GenParticleCollection& gens, bool chargedOnly) {
    return nearest(eta, phi, gens, [chargedOnly](const reco::GenParticle& g) {
      if (g.status() != 1)
        return false;
      int a = std::abs(g.pdgId());
      if (a == 12 || a == 14 || a == 16)
        return false;
      if (chargedOnly && g.charge() == 0)
        return false;
      return true;
    });
  }

  inline Match nearestGenJet(double eta, double phi, const reco::GenJetCollection& jets) {
    return nearest(eta, phi, jets, [](const reco::GenJet&) { return true; });
  }

  // -------------------------------------------- extra per-type candidate details
  inline void candidateExtras(std::ostream& os, const reco::Candidate& c) {
    if (auto mu = dynamic_cast<const reco::Muon*>(&c)) {
      os << " | muon: global=" << mu->isGlobalMuon() << " tracker=" << mu->isTrackerMuon()
         << " standalone=" << mu->isStandAloneMuon() << " pf=" << mu->isPFMuon()
         << " nMatchedStations=" << mu->numberOfMatchedStations();
      if (mu->innerTrack().isNonnull())
        os << " innerPt=" << mu->innerTrack()->pt() << " innerChi2=" << mu->innerTrack()->normalizedChi2()
           << " innerHits=" << mu->innerTrack()->numberOfValidHits();
      if (mu->globalTrack().isNonnull())
        os << " globalPt=" << mu->globalTrack()->pt() << " globalChi2=" << mu->globalTrack()->normalizedChi2();
      if (mu->outerTrack().isNonnull())
        os << " outerPt=" << mu->outerTrack()->pt();
      const reco::MuonPFIsolation& iso = mu->pfIsolationR04();
      os << " pfIso04(ch,nh,ph,pu)=" << iso.sumChargedHadronPt << "," << iso.sumNeutralHadronEt << ","
         << iso.sumPhotonEt << "," << iso.sumPUPt;
    } else if (auto el = dynamic_cast<const reco::GsfElectron*>(&c)) {
      os << " | electron:";
      if (el->superCluster().isNonnull())
        os << " scE=" << el->superCluster()->energy() << " scRawE=" << el->superCluster()->rawEnergy()
           << " scEta=" << el->superCluster()->eta() << " scPhi=" << el->superCluster()->phi();
      os << " ecalE=" << el->ecalEnergy() << " fbrem=" << el->fbrem() << " sieie5x5=" << el->full5x5_sigmaIetaIeta()
         << " hOverE=" << el->hadronicOverEm() << " dEtaIn=" << el->deltaEtaSuperClusterTrackAtVtx()
         << " dPhiIn=" << el->deltaPhiSuperClusterTrackAtVtx() << " ooEmooP="
         << (el->ecalEnergy() > 0 ? std::abs(1. / el->ecalEnergy() - el->eSuperClusterOverP() / el->ecalEnergy()) : 0.);
      if (el->gsfTrack().isNonnull())
        os << " gsfPt=" << el->gsfTrack()->pt() << " gsfChi2=" << el->gsfTrack()->normalizedChi2()
           << " missInnerHits=" << el->gsfTrack()->hitPattern().numberOfLostHits(reco::HitPattern::MISSING_INNER_HITS);
    } else if (auto ph = dynamic_cast<const reco::Photon*>(&c)) {
      os << " | photon:";
      if (ph->superCluster().isNonnull())
        os << " scE=" << ph->superCluster()->energy() << " scRawE=" << ph->superCluster()->rawEnergy()
           << " scEta=" << ph->superCluster()->eta();
      os << " r9=" << ph->r9() << " hOverE=" << ph->hadronicOverEm() << " sieie5x5=" << ph->full5x5_sigmaIetaIeta()
         << " hasPixelSeed=" << ph->hasPixelSeed() << " isEB=" << ph->isEB() << " isEE=" << ph->isEE();
    } else if (auto rec = dynamic_cast<const reco::RecoEcalCandidate*>(&c)) {
      if (rec->superCluster().isNonnull())
        os << " | ecalCand: scE=" << rec->superCluster()->energy() << " scRawE=" << rec->superCluster()->rawEnergy()
           << " scEta=" << rec->superCluster()->eta() << " nClusters=" << rec->superCluster()->clustersSize();
    }
  }

  inline std::string genFlags(const reco::GenParticle& g) {
    const auto& f = g.statusFlags();
    std::string s;
    auto add = [&s](bool v, const char* n) {
      if (v) {
        if (!s.empty())
          s += ",";
        s += n;
      }
    };
    add(f.isPrompt(), "isPrompt");
    add(f.isDecayedLeptonHadron(), "isDecayedLeptonHadron");
    add(f.isTauDecayProduct(), "isTauDecayProduct");
    add(f.isPromptTauDecayProduct(), "isPromptTauDecayProduct");
    add(f.isDirectTauDecayProduct(), "isDirectTauDecayProduct");
    add(f.isDirectPromptTauDecayProduct(), "isDirectPromptTauDecayProduct");
    add(f.isDirectHadronDecayProduct(), "isDirectHadronDecayProduct");
    add(f.isHardProcess(), "isHardProcess");
    add(f.fromHardProcess(), "fromHardProcess");
    add(f.isHardProcessTauDecayProduct(), "isHardProcessTauDecayProduct");
    add(f.isDirectHardProcessTauDecayProduct(), "isDirectHardProcessTauDecayProduct");
    add(f.fromHardProcessBeforeFSR(), "fromHardProcessBeforeFSR");
    add(f.isFirstCopy(), "isFirstCopy");
    add(f.isLastCopy(), "isLastCopy");
    add(f.isLastCopyBeforeFSR(), "isLastCopyBeforeFSR");
    return s.empty() ? "-" : s;
  }

  // Full gen-particle line, shared by GEN and PAT dumps.
  inline void genParticleLine(std::ostream& os, size_t i, const reco::GenParticle& g, const std::vector<int>* barcodes) {
    os << "  [" << i << "]";
    if (barcodes && i < barcodes->size())
      os << " barcode=" << (*barcodes)[i];
    os << " pdg=" << g.pdgId() << " status=" << g.status() << " q=" << g.charge() << " " << kin(g) << " " << vtx(g)
       << " mothers=";
    for (size_t m = 0; m < g.numberOfMothers(); ++m)
      os << (m ? "," : "") << g.motherRef(m).key();
    if (g.numberOfMothers() == 0)
      os << "-";
    os << " daughters=";
    for (size_t d = 0; d < g.numberOfDaughters(); ++d)
      os << (d ? "," : "") << g.daughterRef(d).key();
    if (g.numberOfDaughters() == 0)
      os << "-";
    os << " flags=" << genFlags(g) << "\n";
  }

}  // namespace jitjet

#endif
