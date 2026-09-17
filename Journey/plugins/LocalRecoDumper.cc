// LocalRecoDumper: local reconstruction (the bridge between digis and physics objects):
// tracker pixel/strip clusters, ECAL/HCAL rechits, PF rechits, PF clusters, ECAL
// superclusters and calo towers.
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/Common/interface/DetSetVectorNew.h"
#include "DataFormats/SiPixelCluster/interface/SiPixelCluster.h"
#include "DataFormats/SiStripCluster/interface/SiStripCluster.h"
#include "DataFormats/EcalRecHit/interface/EcalRecHitCollections.h"
#include "DataFormats/EcalDetId/interface/EBDetId.h"
#include "DataFormats/EcalDetId/interface/EEDetId.h"
#include "DataFormats/HcalRecHit/interface/HcalRecHitCollections.h"
#include "DataFormats/ParticleFlowReco/interface/PFRecHit.h"
#include "DataFormats/ParticleFlowReco/interface/PFRecHitFwd.h"
#include "DataFormats/ParticleFlowReco/interface/PFCluster.h"
#include "DataFormats/ParticleFlowReco/interface/PFClusterFwd.h"
#include "DataFormats/EgammaReco/interface/SuperCluster.h"
#include "DataFormats/EgammaReco/interface/SuperClusterFwd.h"
#include "DataFormats/CaloTowers/interface/CaloTowerCollection.h"

#include "DumpUtil.h"

using namespace chaindump;

class LocalRecoDumper : public edm::one::EDAnalyzer<> {
public:
  explicit LocalRecoDumper(const edm::ParameterSet& ps);
  void analyze(const edm::Event& ev, const edm::EventSetup&) override;

private:
  std::string step_;
  int maxElements_, maxSub_;
  std::vector<Tagged<edmNew::DetSetVector<SiPixelCluster>>> pixelClusters_;
  std::vector<Tagged<edmNew::DetSetVector<SiStripCluster>>> stripClusters_;
  std::vector<Tagged<EcalRecHitCollection>> ecalRecHits_;
  std::vector<Tagged<HBHERecHitCollection>> hbheRecHits_;
  std::vector<Tagged<HFRecHitCollection>> hfRecHits_;
  std::vector<Tagged<HORecHitCollection>> hoRecHits_;
  std::vector<Tagged<reco::PFRecHitCollection>> pfRecHits_;
  std::vector<Tagged<reco::PFClusterCollection>> pfClusters_;
  std::vector<Tagged<reco::SuperClusterCollection>> superClusters_;
  std::vector<Tagged<CaloTowerCollection>> caloTowers_;
};

LocalRecoDumper::LocalRecoDumper(const edm::ParameterSet& ps)
    : step_(optString(ps, "step", "RECO")), maxElements_(optInt(ps, "maxElements", -1)), maxSub_(optInt(ps, "maxSub", -1)) {
  auto reg = [&](auto& vec, const char* name) {
    using T = typename std::decay_t<decltype(vec)>::value_type;
    for (auto const& t : optTags(ps, name))
      vec.push_back(T{t, consumes<typename T::product_type>(t)});
  };
  reg(pixelClusters_, "pixelClusters");
  reg(stripClusters_, "stripClusters");
  reg(ecalRecHits_, "ecalRecHits");
  reg(hbheRecHits_, "hbheRecHits");
  reg(hfRecHits_, "hfRecHits");
  reg(hoRecHits_, "hoRecHits");
  reg(pfRecHits_, "pfRecHits");
  reg(pfClusters_, "pfClusters");
  reg(superClusters_, "superClusters");
  reg(caloTowers_, "caloTowers");
}

void LocalRecoDumper::analyze(const edm::Event& ev, const edm::EventSetup&) {
  std::ostream& os = std::cout;
  eventHeader(os, ev, step_, moduleDescription().moduleLabel());

  for (auto const& t : pixelClusters_) {
    edm::Handle<edmNew::DetSetVector<SiPixelCluster>> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "SiPixelClusters", t.tag);
      continue;
    }
    section(os, "SiPixelClusters", t.tag, h->dataSize());
    os << "  nDetUnits=" << h->size() << "\n";
    size_t nd = 0, limDet = cap(maxElements_, h->size());
    for (auto const& ds : *h) {
      if (nd++ >= limDet)
        break;
      os << "  det=" << ds.detId() << " n=" << ds.size() << "\n";
      size_t k = 0, lim = cap(maxSub_, ds.size());
      for (auto const& cl : ds) {
        if (k >= lim)
          break;
        os << "    [" << k++ << "] x=" << cl.x() << " y=" << cl.y() << " charge=" << cl.charge() << " size=" << cl.size() << " sizeX=" << cl.sizeX() << " sizeY=" << cl.sizeY()
           << " minRow=" << cl.minPixelRow() << " maxRow=" << cl.maxPixelRow() << " minCol=" << cl.minPixelCol() << " maxCol=" << cl.maxPixelCol() << " pixels(row:col:adc)=";
        for (int p = 0; p < cl.size(); ++p)
          os << (p ? " " : "") << cl.pixel(p).x << ":" << cl.pixel(p).y << ":" << cl.pixel(p).adc;
        os << "\n";
      }
      truncated(os, lim, ds.size(), "    ");
    }
    truncated(os, limDet, h->size());
  }

  for (auto const& t : stripClusters_) {
    edm::Handle<edmNew::DetSetVector<SiStripCluster>> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "SiStripClusters", t.tag);
      continue;
    }
    section(os, "SiStripClusters", t.tag, h->dataSize());
    os << "  nDetUnits=" << h->size() << "\n";
    size_t nd = 0, limDet = cap(maxElements_, h->size());
    for (auto const& ds : *h) {
      if (nd++ >= limDet)
        break;
      os << "  det=" << ds.detId() << " n=" << ds.size() << "\n";
      size_t k = 0, lim = cap(maxSub_, ds.size());
      for (auto const& cl : ds) {
        if (k >= lim)
          break;
        os << "    [" << k++ << "] firstStrip=" << cl.firstStrip() << " barycenter=" << cl.barycenter() << " charge=" << cl.charge() << " size=" << cl.size() << " amplitudes=";
        auto amps = cl.amplitudes();
        for (size_t a = 0; a < amps.size(); ++a)
          os << (a ? "," : "") << static_cast<int>(amps[a]);
        os << "\n";
      }
      truncated(os, lim, ds.size(), "    ");
    }
    truncated(os, limDet, h->size());
  }

  for (auto const& t : ecalRecHits_) {
    edm::Handle<EcalRecHitCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "EcalRecHits", t.tag);
      continue;
    }
    section(os, "EcalRecHits", t.tag, h->size());
    double sumE = 0;
    for (auto const& rh : *h)
      sumE += rh.energy();
    os << "  sumE=" << sumE << "\n";
    size_t lim = cap(maxSub_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const EcalRecHit& rh = (*h)[i];
      os << "  [" << i << "] ";
      if (rh.id().subdetId() == EcalBarrel) {
        EBDetId id(rh.id());
        os << "EB ieta=" << id.ieta() << " iphi=" << id.iphi();
      } else if (rh.id().subdetId() == EcalEndcap) {
        EEDetId id(rh.id());
        os << "EE ix=" << id.ix() << " iy=" << id.iy() << " zside=" << id.zside();
      }
      os << " rawId=" << rh.id().rawId() << " E=" << rh.energy() << " Eerr=" << rh.energyError() << " t=" << rh.time() << " tErr=" << rh.timeError() << " flags=" << rh.flagsBits()
         << " good=" << rh.checkFlag(EcalRecHit::kGood) << " chi2=" << rh.chi2() << "\n";
    }
    truncated(os, lim, h->size());
  }

  auto printHcal = [&](const std::string& name, auto const& tagged) {
    using COLL = typename std::decay_t<decltype(tagged)>::product_type;
    edm::Handle<COLL> h;
    ev.getByToken(tagged.token, h);
    if (!h.isValid()) {
      missing(os, name, tagged.tag);
      return;
    }
    section(os, name, tagged.tag, h->size());
    double sumE = 0;
    for (auto const& rh : *h)
      sumE += rh.energy();
    os << "  sumE=" << sumE << "\n";
    size_t lim = cap(maxSub_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const auto& rh = (*h)[i];
      os << "  [" << i << "] " << rh.id() << " rawId=" << rh.id().rawId() << " E=" << rh.energy() << " t=" << rh.time() << " flags=" << rh.flags();
      if constexpr (std::is_same_v<COLL, HBHERecHitCollection>)
        os << " eraw=" << rh.eraw() << " eaux=" << rh.eaux() << " chi2=" << rh.chi2() << " timeFalling=" << rh.timeFalling();
      os << "\n";
    }
    truncated(os, lim, h->size());
  };
  for (auto const& t : hbheRecHits_)
    printHcal("HBHERecHits", t);
  for (auto const& t : hfRecHits_)
    printHcal("HFRecHits", t);
  for (auto const& t : hoRecHits_)
    printHcal("HORecHits", t);

  for (auto const& t : pfRecHits_) {
    edm::Handle<reco::PFRecHitCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "PFRecHits", t.tag);
      continue;
    }
    section(os, "PFRecHits", t.tag, h->size());
    size_t lim = cap(maxSub_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const reco::PFRecHit& rh = (*h)[i];
      os << "  [" << i << "] detId=" << rh.detId() << " layer=" << rh.layer() << " depth=" << rh.depth() << " E=" << rh.energy() << " t=" << rh.time() << " eta=" << rh.position().eta()
         << " phi=" << rh.position().phi() << " x=" << rh.position().x() << " y=" << rh.position().y() << " z=" << rh.position().z() << " nNeighbours=" << rh.neighbours().size() << "\n";
    }
    truncated(os, lim, h->size());
  }

  for (auto const& t : pfClusters_) {
    edm::Handle<reco::PFClusterCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "PFClusters", t.tag);
      continue;
    }
    section(os, "PFClusters", t.tag, h->size());
    size_t lim = cap(maxElements_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const reco::PFCluster& c = (*h)[i];
      os << "  [" << i << "] E=" << c.energy() << " correctedE=" << c.correctedEnergy() << " pt=" << c.pt() << " eta=" << c.eta() << " phi=" << c.phi() << " x=" << c.x() << " y=" << c.y() << " z=" << c.z()
         << " layer=" << c.layer() << " depth=" << c.depth() << " seedDetId=" << c.seed().rawId() << " time=" << c.time() << " nRecHits=" << c.recHitFractions().size() << "\n";
      size_t nr = c.recHitFractions().size(), limr = cap(maxSub_, nr);
      os << "      rechits(detId:fraction:E):";
      for (size_t r = 0; r < limr; ++r) {
        const auto& f = c.recHitFractions()[r];
        os << " " << (f.recHitRef().isNonnull() && f.recHitRef().isAvailable() ? f.recHitRef()->detId() : 0u) << ":" << f.fraction() << ":"
           << (f.recHitRef().isNonnull() && f.recHitRef().isAvailable() ? f.recHitRef()->energy() : 0.);
      }
      if (limr < nr)
        os << " ...(" << nr - limr << " more)";
      os << "\n";
    }
    truncated(os, lim, h->size());
  }

  for (auto const& t : superClusters_) {
    edm::Handle<reco::SuperClusterCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "SuperClusters", t.tag);
      continue;
    }
    section(os, "SuperClusters", t.tag, h->size());
    size_t lim = cap(maxElements_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const reco::SuperCluster& sc = (*h)[i];
      os << "  [" << i << "] E=" << sc.energy() << " rawE=" << sc.rawEnergy() << " correctedE=" << sc.correctedEnergy() << " preshowerE=" << sc.preshowerEnergy() << " eta=" << sc.eta() << " phi=" << sc.phi()
         << " etaWidth=" << sc.etaWidth() << " phiWidth=" << sc.phiWidth() << " nClusters=" << sc.clustersSize() << " seedE=" << (sc.seed().isNonnull() ? sc.seed()->energy() : 0.) << "\n";
    }
    truncated(os, lim, h->size());
  }

  for (auto const& t : caloTowers_) {
    edm::Handle<CaloTowerCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "CaloTowers", t.tag);
      continue;
    }
    section(os, "CaloTowers", t.tag, h->size());
    size_t lim = cap(maxSub_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const CaloTower& ct = (*h)[i];
      os << "  [" << i << "] " << ct.id() << " ieta=" << ct.ieta() << " iphi=" << ct.iphi() << " et=" << ct.et() << " eta=" << ct.eta() << " phi=" << ct.phi() << " emE=" << ct.emEnergy()
         << " hadE=" << ct.hadEnergy() << " outerE=" << ct.outerEnergy() << " emEt=" << ct.emEt() << " hadEt=" << ct.hadEt() << " nConstituents=" << ct.constituentsSize() << "\n";
    }
    truncated(os, lim, h->size());
  }
  os.flush();
}

DEFINE_FWK_MODULE(LocalRecoDumper);
