// GenJourney: prints, for every event, the generator information, the HepMC primary
// vertex before/after smearing, every genParticle (with barcodes, mothers, daughters,
// status flags), every GenJet with its constituents and the GenMETs.
#include <map>

#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/HepMCProduct.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "DataFormats/METReco/interface/GenMET.h"
#include "DataFormats/METReco/interface/GenMETCollection.h"
#include "HepMC/GenEvent.h"
#include "HepMC/GenVertex.h"
#include "HepMC/GenParticle.h"

#include "JourneyUtil.h"

using namespace jitjet;

class GenJourney : public edm::one::EDAnalyzer<> {
public:
  explicit GenJourney(const edm::ParameterSet& ps);
  void analyze(const edm::Event& ev, const edm::EventSetup&) override;

private:
  void printHepMC(std::ostream& os, const std::string& name, const edm::InputTag& tag, const edm::HepMCProduct& prod) const;

  std::string step_;
  int maxElements_, maxSub_;
  bool printHepMCParticles_;
  edm::InputTag genInfoTag_, hepmcSmearedTag_, hepmcRawTag_, genParticlesTag_, genBarcodesTag_;
  edm::EDGetTokenT<GenEventInfoProduct> genInfoTok_;
  edm::EDGetTokenT<edm::HepMCProduct> hepmcSmearedTok_, hepmcRawTok_;
  edm::EDGetTokenT<reco::GenParticleCollection> genParticlesTok_;
  edm::EDGetTokenT<std::vector<int>> genBarcodesTok_;
  std::vector<Tagged<reco::GenJetCollection>> genJets_;
  std::vector<Tagged<reco::GenMETCollection>> genMETs_;
};

GenJourney::GenJourney(const edm::ParameterSet& ps)
    : step_(optString(ps, "step", "GEN")),
      maxElements_(optInt(ps, "maxElements", -1)),
      maxSub_(optInt(ps, "maxSub", -1)),
      printHepMCParticles_(optBool(ps, "printHepMCParticles", false)),
      genInfoTag_(optTag(ps, "genEventInfo")),
      hepmcSmearedTag_(optTag(ps, "hepmcSmeared")),
      hepmcRawTag_(optTag(ps, "hepmcRaw")),
      genParticlesTag_(optTag(ps, "genParticles")),
      genBarcodesTag_(optTag(ps, "genBarcodes")) {
  if (!genInfoTag_.label().empty())
    genInfoTok_ = consumes<GenEventInfoProduct>(genInfoTag_);
  if (!hepmcSmearedTag_.label().empty())
    hepmcSmearedTok_ = consumes<edm::HepMCProduct>(hepmcSmearedTag_);
  if (!hepmcRawTag_.label().empty())
    hepmcRawTok_ = consumes<edm::HepMCProduct>(hepmcRawTag_);
  if (!genParticlesTag_.label().empty())
    genParticlesTok_ = consumes<reco::GenParticleCollection>(genParticlesTag_);
  if (!genBarcodesTag_.label().empty())
    genBarcodesTok_ = consumes<std::vector<int>>(genBarcodesTag_);
  for (auto const& t : optTags(ps, "genJets"))
    genJets_.push_back({t, consumes<reco::GenJetCollection>(t)});
  for (auto const& t : optTags(ps, "genMETs"))
    genMETs_.push_back({t, consumes<reco::GenMETCollection>(t)});
}

void GenJourney::printHepMC(std::ostream& os, const std::string& name, const edm::InputTag& tag, const edm::HepMCProduct& prod) const {
  const HepMC::GenEvent* evt = prod.GetEvent();
  if (!evt) {
    os << "\n== " << name << " [" << tag.encode() << "] : empty HepMC event\n";
    return;
  }
  os << "\n== " << name << " [" << tag.encode() << "] eventNumber=" << evt->event_number()
     << " signalProcessId=" << evt->signal_process_id() << " nVertices=" << evt->vertices_size()
     << " nParticles=" << evt->particles_size() << " eventScale=" << evt->event_scale()
     << " alphaQCD=" << evt->alphaQCD() << " alphaQED=" << evt->alphaQED()
     << " vtxSmearingApplied=" << prod.isVtxGenApplied() << " vtxBoostApplied=" << prod.isVtxBoostApplied() << "\n";
  const HepMC::GenVertex* pv = evt->signal_process_vertex();
  if (!pv && evt->vertices_begin() != evt->vertices_end())
    pv = *(evt->vertices_begin());
  if (pv)
    os << "  primaryVertex(mm): x=" << pv->position().x() << " y=" << pv->position().y() << " z=" << pv->position().z()
       << " t=" << pv->position().t() << " (cm: " << pv->position().x() / 10. << ", " << pv->position().y() / 10.
       << ", " << pv->position().z() / 10. << ")\n";
  os << "  weights:";
  for (size_t i = 0; i < evt->weights().size(); ++i)
    os << " " << evt->weights()[i];
  os << "\n";
  if (evt->pdf_info())
    os << "  pdf: id1=" << evt->pdf_info()->id1() << " id2=" << evt->pdf_info()->id2() << " x1=" << evt->pdf_info()->x1()
       << " x2=" << evt->pdf_info()->x2() << " scalePDF=" << evt->pdf_info()->scalePDF()
       << " pdf1=" << evt->pdf_info()->pdf1() << " pdf2=" << evt->pdf_info()->pdf2() << "\n";
  if (printHepMCParticles_) {
    size_t n = 0, ntot = evt->particles_size();
    size_t lim = cap(maxElements_, ntot);
    for (auto it = evt->particles_begin(); it != evt->particles_end() && n < lim; ++it, ++n) {
      const HepMC::GenParticle* p = *it;
      os << "  [" << n << "] barcode=" << p->barcode() << " pdg=" << p->pdg_id() << " status=" << p->status()
         << " px=" << p->momentum().px() << " py=" << p->momentum().py() << " pz=" << p->momentum().pz()
         << " E=" << p->momentum().e() << " m=" << p->generated_mass() << " pt=" << p->momentum().perp();
      if (p->production_vertex())
        os << " prodVtx(mm)=(" << p->production_vertex()->position().x() << "," << p->production_vertex()->position().y()
           << "," << p->production_vertex()->position().z() << ") prodVtxBarcode=" << p->production_vertex()->barcode();
      if (p->end_vertex())
        os << " endVtxBarcode=" << p->end_vertex()->barcode();
      os << "\n";
    }
    truncated(os, lim, ntot);
  }
}

void GenJourney::analyze(const edm::Event& ev, const edm::EventSetup&) {
  std::ostream& os = std::cout;
  eventHeader(os, ev, step_, moduleDescription().moduleLabel());

  if (!genInfoTag_.label().empty()) {
    edm::Handle<GenEventInfoProduct> h;
    ev.getByToken(genInfoTok_, h);
    if (h.isValid()) {
      os << "\n== GenEventInfoProduct [" << genInfoTag_.encode() << "] weight=" << h->weight()
         << " weightProduct=" << h->weightProduct() << " signalProcessID=" << h->signalProcessID()
         << " qScale=" << h->qScale() << " alphaQCD=" << h->alphaQCD() << " alphaQED=" << h->alphaQED()
         << " nMEPartons=" << h->nMEPartons() << " nMEPartonsFiltered=" << h->nMEPartonsFiltered() << "\n";
      os << "  weights:";
      for (double w : h->weights())
        os << " " << w;
      os << "\n";
      if (h->hasPDF())
        os << "  pdf: id=(" << h->pdf()->id.first << "," << h->pdf()->id.second << ") x=(" << h->pdf()->x.first << ","
           << h->pdf()->x.second << ") xPDF=(" << h->pdf()->xPDF.first << "," << h->pdf()->xPDF.second
           << ") scalePDF=" << h->pdf()->scalePDF << "\n";
      if (h->hasBinningValues()) {
        os << "  binningValues:";
        for (double b : h->binningValues())
          os << " " << b;
        os << "\n";
      }
    } else
      missing(os, "GenEventInfoProduct", genInfoTag_);
  }

  if (!hepmcRawTag_.label().empty()) {
    edm::Handle<edm::HepMCProduct> h;
    ev.getByToken(hepmcRawTok_, h);
    if (h.isValid())
      printHepMC(os, "HepMC (before vertex smearing)", hepmcRawTag_, *h);
    else
      missing(os, "HepMC (before vertex smearing)", hepmcRawTag_);
  }
  if (!hepmcSmearedTag_.label().empty()) {
    edm::Handle<edm::HepMCProduct> h;
    ev.getByToken(hepmcSmearedTok_, h);
    if (h.isValid())
      printHepMC(os, "HepMC (after vertex smearing)", hepmcSmearedTag_, *h);
    else
      missing(os, "HepMC (after vertex smearing)", hepmcSmearedTag_);
  }

  const reco::GenParticleCollection* gens = nullptr;
  if (!genParticlesTag_.label().empty()) {
    edm::Handle<reco::GenParticleCollection> h;
    ev.getByToken(genParticlesTok_, h);
    const std::vector<int>* barcodes = nullptr;
    edm::Handle<std::vector<int>> hb;
    if (!genBarcodesTag_.label().empty()) {
      ev.getByToken(genBarcodesTok_, hb);
      if (hb.isValid())
        barcodes = hb.product();
    }
    if (h.isValid()) {
      gens = h.product();
      section(os, "GenParticles", genParticlesTag_, gens->size());
      os << "  columns: [index] barcode pdg status charge pt eta phi m E px py pz vx vy vz(cm) mothers(indices) daughters(indices) statusFlags\n";
      size_t lim = cap(maxElements_, gens->size());
      for (size_t i = 0; i < lim; ++i)
        genParticleLine(os, i, (*gens)[i], barcodes);
      truncated(os, lim, gens->size());
      // summary of status-1 content
      double sumPx = 0, sumPy = 0, sumE = 0, sumEvis = 0;
      int nStable = 0, nCharged = 0, nNu = 0;
      for (auto const& g : *gens) {
        if (g.status() != 1)
          continue;
        ++nStable;
        sumPx += g.px();
        sumPy += g.py();
        sumE += g.energy();
        int a = std::abs(g.pdgId());
        if (a == 12 || a == 14 || a == 16)
          ++nNu;
        else
          sumEvis += g.energy();
        if (g.charge() != 0)
          ++nCharged;
      }
      os << "  summary status1: n=" << nStable << " nCharged=" << nCharged << " nNeutrinos=" << nNu
         << " sumPx=" << sumPx << " sumPy=" << sumPy << " sumE=" << sumE << " sumEvisible=" << sumEvis
         << " |sumPt|=" << std::hypot(sumPx, sumPy) << "\n";
    } else
      missing(os, "GenParticles", genParticlesTag_);
  }

  for (auto const& tj : genJets_) {
    edm::Handle<reco::GenJetCollection> h;
    ev.getByToken(tj.token, h);
    if (!h.isValid()) {
      missing(os, "GenJets", tj.tag);
      continue;
    }
    section(os, "GenJets", tj.tag, h->size());
    size_t lim = cap(maxElements_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const reco::GenJet& j = (*h)[i];
      os << "  [" << i << "] " << kin(j) << " area=" << j.jetArea() << " emE=" << j.emEnergy() << " hadE=" << j.hadEnergy()
         << " invisibleE=" << j.invisibleEnergy() << " auxE=" << j.auxiliaryEnergy() << " nConst=" << j.numberOfDaughters()
         << " charge=" << j.charge() << "\n";
      size_t nc = j.numberOfDaughters();
      size_t limc = cap(maxSub_, nc);
      os << "      constituents(key:pdg:pt:eta:phi):";
      for (size_t k = 0; k < limc; ++k) {
        reco::CandidatePtr p = j.daughterPtr(k);
        os << " " << (p.isNonnull() ? std::to_string(p.key()) : std::string("?")) << ":" << j.daughter(k)->pdgId() << ":"
           << j.daughter(k)->pt() << ":" << j.daughter(k)->eta() << ":" << j.daughter(k)->phi();
      }
      if (limc < nc)
        os << " ...(" << nc - limc << " more)";
      os << "\n";
    }
    truncated(os, lim, h->size());
  }

  for (auto const& tm : genMETs_) {
    edm::Handle<reco::GenMETCollection> h;
    ev.getByToken(tm.token, h);
    if (!h.isValid()) {
      missing(os, "GenMET", tm.tag);
      continue;
    }
    section(os, "GenMET", tm.tag, h->size());
    for (size_t i = 0; i < h->size(); ++i) {
      const reco::GenMET& m = (*h)[i];
      os << "  [" << i << "] pt=" << m.pt() << " phi=" << m.phi() << " px=" << m.px() << " py=" << m.py() << " sumEt=" << m.sumEt()
         << " NeutralEMEt=" << m.NeutralEMEt() << " ChargedEMEt=" << m.ChargedEMEt() << " NeutralHadEt=" << m.NeutralHadEt()
         << " ChargedHadEt=" << m.ChargedHadEt() << " MuonEt=" << m.MuonEt() << " InvisibleEt=" << m.InvisibleEt() << "\n";
    }
  }
  os.flush();
}

DEFINE_FWK_MODULE(GenJourney);
