// SimJourney: Geant4 output. Every SimVertex, every SimTrack (linked back to the
// genParticle through the HepMC barcode), every tracker PSimHit and calorimeter PCaloHit.
#include <map>
#include <unordered_map>

#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "SimDataFormats/Track/interface/SimTrackContainer.h"
#include "SimDataFormats/Vertex/interface/SimVertexContainer.h"
#include "SimDataFormats/TrackingHit/interface/PSimHitContainer.h"
#include "SimDataFormats/CaloHit/interface/PCaloHitContainer.h"

#include "JourneyUtil.h"

using namespace jitjet;

class SimJourney : public edm::one::EDAnalyzer<> {
public:
  explicit SimJourney(const edm::ParameterSet& ps);
  void analyze(const edm::Event& ev, const edm::EventSetup&) override;

private:
  std::string step_;
  int maxElements_, maxSub_;
  edm::InputTag simTracksTag_, simVerticesTag_, genParticlesTag_, genBarcodesTag_;
  edm::EDGetTokenT<edm::SimTrackContainer> simTracksTok_;
  edm::EDGetTokenT<edm::SimVertexContainer> simVerticesTok_;
  edm::EDGetTokenT<reco::GenParticleCollection> genParticlesTok_;
  edm::EDGetTokenT<std::vector<int>> genBarcodesTok_;
  std::vector<Tagged<edm::PSimHitContainer>> trackerHits_;
  std::vector<Tagged<edm::PCaloHitContainer>> caloHits_;
};

SimJourney::SimJourney(const edm::ParameterSet& ps)
    : step_(optString(ps, "step", "SIM")),
      maxElements_(optInt(ps, "maxElements", -1)),
      maxSub_(optInt(ps, "maxSub", -1)),
      simTracksTag_(optTag(ps, "simTracks")),
      simVerticesTag_(optTag(ps, "simVertices")),
      genParticlesTag_(optTag(ps, "genParticles")),
      genBarcodesTag_(optTag(ps, "genBarcodes")) {
  if (!simTracksTag_.label().empty())
    simTracksTok_ = consumes<edm::SimTrackContainer>(simTracksTag_);
  if (!simVerticesTag_.label().empty())
    simVerticesTok_ = consumes<edm::SimVertexContainer>(simVerticesTag_);
  if (!genParticlesTag_.label().empty())
    genParticlesTok_ = consumes<reco::GenParticleCollection>(genParticlesTag_);
  if (!genBarcodesTag_.label().empty())
    genBarcodesTok_ = consumes<std::vector<int>>(genBarcodesTag_);
  for (auto const& t : optTags(ps, "trackerHits"))
    trackerHits_.push_back({t, consumes<edm::PSimHitContainer>(t)});
  for (auto const& t : optTags(ps, "caloHits"))
    caloHits_.push_back({t, consumes<edm::PCaloHitContainer>(t)});
}

void SimJourney::analyze(const edm::Event& ev, const edm::EventSetup&) {
  std::ostream& os = std::cout;
  eventHeader(os, ev, step_, moduleDescription().moduleLabel());

  // barcode -> genParticle index
  std::unordered_map<int, int> barcodeToGen;
  const reco::GenParticleCollection* gens = nullptr;
  if (!genParticlesTag_.label().empty()) {
    edm::Handle<reco::GenParticleCollection> h;
    ev.getByToken(genParticlesTok_, h);
    if (h.isValid())
      gens = h.product();
  }
  if (!genBarcodesTag_.label().empty()) {
    edm::Handle<std::vector<int>> hb;
    ev.getByToken(genBarcodesTok_, hb);
    if (hb.isValid())
      for (size_t i = 0; i < hb->size(); ++i)
        barcodeToGen[(*hb)[i]] = i;
  }

  // SimVertices
  const edm::SimVertexContainer* verts = nullptr;
  if (!simVerticesTag_.label().empty()) {
    edm::Handle<edm::SimVertexContainer> h;
    ev.getByToken(simVerticesTok_, h);
    if (h.isValid()) {
      verts = h.product();
      section(os, "SimVertices", simVerticesTag_, verts->size());
      os << "  columns: [index] vertexId x y z(cm) t(ns) parentTrackId processType\n";
      size_t lim = cap(maxElements_, verts->size());
      for (size_t i = 0; i < lim; ++i) {
        const SimVertex& v = (*verts)[i];
        os << "  [" << i << "] vertexId=" << v.vertexId() << " x=" << v.position().x() << " y=" << v.position().y()
           << " z=" << v.position().z() << " t=" << v.position().t() << " parentTrackId=" << (v.noParent() ? -1 : v.parentIndex())
           << " processType=" << v.processType() << "\n";
      }
      truncated(os, lim, verts->size());
    } else
      missing(os, "SimVertices", simVerticesTag_);
  }

  // SimTracks
  std::unordered_map<unsigned int, size_t> trackIdToIdx;
  const edm::SimTrackContainer* tracks = nullptr;
  if (!simTracksTag_.label().empty()) {
    edm::Handle<edm::SimTrackContainer> h;
    ev.getByToken(simTracksTok_, h);
    if (h.isValid()) {
      tracks = h.product();
      for (size_t i = 0; i < tracks->size(); ++i)
        trackIdToIdx[(*tracks)[i].trackId()] = i;
      section(os, "SimTracks", simTracksTag_, tracks->size());
      os << "  columns: [index] trackId pdg charge kinematics vertexIndex(+position) genBarcode->genIdx (+gen kinematics and pt ratio) primary crossedBoundary(+boundary info)\n";
      size_t lim = cap(maxElements_, tracks->size());
      int nLinked = 0;
      for (size_t i = 0; i < lim; ++i) {
        const SimTrack& t = (*tracks)[i];
        os << "  [" << i << "] trackId=" << t.trackId() << " pdg=" << t.type() << " q=" << t.charge() << " "
           << kinLV(t.momentum()) << " vertIndex=" << t.vertIndex();
        if (verts && t.vertIndex() >= 0 && static_cast<size_t>(t.vertIndex()) < verts->size()) {
          const auto& p = (*verts)[t.vertIndex()].position();
          os << " vtx=(" << p.x() << "," << p.y() << "," << p.z() << ")";
        }
        os << " genBarcode=" << (t.noGenpart() ? -1 : t.genpartIndex());
        if (!t.noGenpart()) {
          auto it = barcodeToGen.find(t.genpartIndex());
          if (it != barcodeToGen.end()) {
            os << " genIdx=" << it->second;
            if (gens && static_cast<size_t>(it->second) < gens->size()) {
              const reco::GenParticle& g = (*gens)[it->second];
              os << " genPdg=" << g.pdgId() << " genPt=" << g.pt() << " genEta=" << g.eta() << " genPhi=" << g.phi()
                 << " ptRatio=" << (g.pt() > 0 ? t.momentum().Pt() / g.pt() : 0.);
              ++nLinked;
            }
          } else
            os << " genIdx=?";
        }
        os << " primary=" << t.isPrimary() << " crossedBoundary=" << t.crossedBoundary();
        if (t.crossedBoundary())
          os << " boundaryPos=(" << t.getPositionAtBoundary().x() << "," << t.getPositionAtBoundary().y() << ","
             << t.getPositionAtBoundary().z() << ") boundaryMom=" << kinLV(t.getMomentumAtBoundary())
             << " idAtBoundary=" << t.getIDAtBoundary();
        os << "\n";
      }
      truncated(os, lim, tracks->size());
      os << "  summary: nSimTracks=" << tracks->size() << " nWithGenLink(printed)=" << nLinked << "\n";
    } else
      missing(os, "SimTracks", simTracksTag_);
  }

  auto trackInfo = [&](unsigned int trackId) {
    std::ostringstream s;
    s << std::setprecision(7);
    auto it = trackIdToIdx.find(trackId);
    if (it != trackIdToIdx.end() && tracks) {
      const SimTrack& t = (*tracks)[it->second];
      s << " simTrackIdx=" << it->second << " simPdg=" << t.type() << " simPt=" << t.momentum().Pt();
    } else
      s << " simTrackIdx=?";
    return s.str();
  };

  for (auto const& th : trackerHits_) {
    edm::Handle<edm::PSimHitContainer> h;
    ev.getByToken(th.token, h);
    if (!h.isValid()) {
      missing(os, "PSimHits", th.tag);
      continue;
    }
    section(os, "PSimHits", th.tag, h->size());
    os << "  columns: [index] detUnitId trackId(+simtrack) particleType processType pabs(GeV) tof(ns) eLoss(GeV) entry exit localPos(cm) momentumAtEntry\n";
    size_t lim = cap(maxSub_, h->size());
    double sumE = 0;
    for (auto const& hit : *h)
      sumE += hit.energyLoss();
    for (size_t i = 0; i < lim; ++i) {
      const PSimHit& hit = (*h)[i];
      os << "  [" << i << "] detUnitId=" << hit.detUnitId() << " trackId=" << hit.trackId() << trackInfo(hit.trackId())
         << " particleType=" << hit.particleType() << " processType=" << hit.processType() << " pabs=" << hit.pabs()
         << " tof=" << hit.timeOfFlight() << " eLoss=" << hit.energyLoss() << " entry=(" << hit.entryPoint().x() << ","
         << hit.entryPoint().y() << "," << hit.entryPoint().z() << ") exit=(" << hit.exitPoint().x() << ","
         << hit.exitPoint().y() << "," << hit.exitPoint().z() << ") localPos=(" << hit.localPosition().x() << ","
         << hit.localPosition().y() << "," << hit.localPosition().z() << ") momAtEntry=(" << hit.momentumAtEntry().x()
         << "," << hit.momentumAtEntry().y() << "," << hit.momentumAtEntry().z() << ")\n";
    }
    truncated(os, lim, h->size());
    os << "  summary: nHits=" << h->size() << " sumELoss=" << sumE << "\n";
  }

  for (auto const& th : caloHits_) {
    edm::Handle<edm::PCaloHitContainer> h;
    ev.getByToken(th.token, h);
    if (!h.isValid()) {
      missing(os, "PCaloHits", th.tag);
      continue;
    }
    section(os, "PCaloHits", th.tag, h->size());
    os << "  columns: [index] id(hex) depth energy energyEM energyHad(GeV) time(ns) geantTrackId(+simtrack)\n";
    size_t lim = cap(maxSub_, h->size());
    double sumE = 0;
    for (auto const& hit : *h)
      sumE += hit.energy();
    for (size_t i = 0; i < lim; ++i) {
      const PCaloHit& hit = (*h)[i];
      os << "  [" << i << "] id=0x" << std::hex << hit.id() << std::dec << " depth=" << hit.depth() << " E=" << hit.energy()
         << " Eem=" << hit.energyEM() << " Ehad=" << hit.energyHad() << " t=" << hit.time() << " geantTrackId=" << hit.geantTrackId()
         << trackInfo(hit.geantTrackId()) << "\n";
    }
    truncated(os, lim, h->size());
    os << "  summary: nHits=" << h->size() << " sumE=" << sumE << "\n";
  }
  os.flush();
}

DEFINE_FWK_MODULE(SimJourney);
