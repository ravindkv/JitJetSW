// DigiDumper: every digi of every subdetector (tracker pixel/strip, ECAL, HCAL, muon),
// the digi->SimTrack links, ECAL/HCAL trigger primitives, TrackingParticles/Vertices,
// pileup summary and (for RAW) the size of every FED payload.
#include <unordered_map>

#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/Common/interface/DetSetVector.h"
#include "DataFormats/SiPixelDigi/interface/PixelDigi.h"
#include "DataFormats/SiStripDigi/interface/SiStripDigi.h"
#include "SimDataFormats/TrackerDigiSimLink/interface/PixelDigiSimLink.h"
#include "SimDataFormats/TrackerDigiSimLink/interface/StripDigiSimLink.h"
#include "DataFormats/EcalDigi/interface/EcalDigiCollections.h"
#include "DataFormats/EcalDetId/interface/EBDetId.h"
#include "DataFormats/EcalDetId/interface/EEDetId.h"
#include "DataFormats/EcalDetId/interface/ESDetId.h"
#include "DataFormats/HcalDigi/interface/HcalDigiCollections.h"
#include "DataFormats/HcalDetId/interface/HcalDetId.h"
#include "DataFormats/DTDigi/interface/DTDigiCollection.h"
#include "DataFormats/CSCDigi/interface/CSCWireDigiCollection.h"
#include "DataFormats/CSCDigi/interface/CSCStripDigiCollection.h"
#include "DataFormats/CSCDigi/interface/CSCComparatorDigiCollection.h"
#include "DataFormats/RPCDigi/interface/RPCDigiCollection.h"
#include "DataFormats/GEMDigi/interface/GEMDigiCollection.h"
#include "SimDataFormats/TrackingAnalysis/interface/TrackingParticle.h"
#include "SimDataFormats/TrackingAnalysis/interface/TrackingVertex.h"
#include "SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h"
#include "DataFormats/FEDRawData/interface/FEDRawDataCollection.h"
#include "DataFormats/FEDRawData/interface/FEDNumbering.h"

#include "DumpUtil.h"

using namespace chaindump;

class DigiDumper : public edm::one::EDAnalyzer<> {
public:
  explicit DigiDumper(const edm::ParameterSet& ps);
  void analyze(const edm::Event& ev, const edm::EventSetup&) override;

private:
  template <class DIGI, class PRINT>
  void printDetSetVector(std::ostream& os, const std::string& name, const Tagged<edm::DetSetVector<DIGI>>& t, const edm::Event& ev, PRINT printer);
  template <class COLL, class PRINT>
  void printMuonDigis(std::ostream& os, const std::string& name, const Tagged<COLL>& t, const edm::Event& ev, PRINT printer);

  std::string step_;
  int maxElements_, maxSub_;
  std::vector<Tagged<edm::DetSetVector<PixelDigi>>> pixelDigis_;
  std::vector<Tagged<edm::DetSetVector<PixelDigiSimLink>>> pixelLinks_;
  std::vector<Tagged<edm::DetSetVector<SiStripDigi>>> stripDigis_;
  std::vector<Tagged<edm::DetSetVector<StripDigiSimLink>>> stripLinks_;
  std::vector<Tagged<EBDigiCollection>> ebDigis_;
  std::vector<Tagged<EEDigiCollection>> eeDigis_;
  std::vector<Tagged<ESDigiCollection>> esDigis_;
  std::vector<Tagged<QIE11DigiCollection>> qie11Digis_;
  std::vector<Tagged<QIE10DigiCollection>> qie10Digis_;
  std::vector<Tagged<HODigiCollection>> hoDigis_;
  std::vector<Tagged<EcalTrigPrimDigiCollection>> ecalTPs_;
  std::vector<Tagged<HcalTrigPrimDigiCollection>> hcalTPs_;
  std::vector<Tagged<DTDigiCollection>> dtDigis_;
  std::vector<Tagged<CSCWireDigiCollection>> cscWireDigis_;
  std::vector<Tagged<CSCStripDigiCollection>> cscStripDigis_;
  std::vector<Tagged<CSCComparatorDigiCollection>> cscCompDigis_;
  std::vector<Tagged<RPCDigiCollection>> rpcDigis_;
  std::vector<Tagged<GEMDigiCollection>> gemDigis_;
  std::vector<Tagged<TrackingParticleCollection>> trackingParticles_;
  std::vector<Tagged<TrackingVertexCollection>> trackingVertices_;
  std::vector<Tagged<std::vector<PileupSummaryInfo>>> pileup_;
  std::vector<Tagged<FEDRawDataCollection>> raw_;
};

DigiDumper::DigiDumper(const edm::ParameterSet& ps)
    : step_(optString(ps, "step", "DIGI")), maxElements_(optInt(ps, "maxElements", -1)), maxSub_(optInt(ps, "maxSub", -1)) {
  auto reg = [&](auto& vec, const char* name) {
    using T = typename std::decay_t<decltype(vec)>::value_type;
    for (auto const& t : optTags(ps, name))
      vec.push_back(T{t, consumes<typename T::product_type>(t)});
  };
  reg(pixelDigis_, "pixelDigis");
  reg(pixelLinks_, "pixelDigiSimLinks");
  reg(stripDigis_, "stripDigis");
  reg(stripLinks_, "stripDigiSimLinks");
  reg(ebDigis_, "ebDigis");
  reg(eeDigis_, "eeDigis");
  reg(esDigis_, "esDigis");
  reg(qie11Digis_, "hcalQIE11Digis");
  reg(qie10Digis_, "hcalQIE10Digis");
  reg(hoDigis_, "hoDigis");
  reg(ecalTPs_, "ecalTrigPrims");
  reg(hcalTPs_, "hcalTrigPrims");
  reg(dtDigis_, "dtDigis");
  reg(cscWireDigis_, "cscWireDigis");
  reg(cscStripDigis_, "cscStripDigis");
  reg(cscCompDigis_, "cscComparatorDigis");
  reg(rpcDigis_, "rpcDigis");
  reg(gemDigis_, "gemDigis");
  reg(trackingParticles_, "trackingParticles");
  reg(trackingVertices_, "trackingVertices");
  reg(pileup_, "pileupSummary");
  reg(raw_, "fedRawData");
}

template <class DIGI, class PRINT>
void DigiDumper::printDetSetVector(std::ostream& os, const std::string& name, const Tagged<edm::DetSetVector<DIGI>>& t, const edm::Event& ev, PRINT printer) {
  edm::Handle<edm::DetSetVector<DIGI>> h;
  ev.getByToken(t.token, h);
  if (!h.isValid()) {
    missing(os, name, t.tag);
    return;
  }
  size_t ntot = 0;
  for (auto const& ds : *h)
    ntot += ds.size();
  section(os, name, t.tag, ntot);
  os << "  nDetUnits=" << h->size() << " nDigis=" << ntot << "\n";
  size_t nd = 0, limDet = cap(maxElements_, h->size());
  for (auto const& ds : *h) {
    if (nd >= limDet)
      break;
    os << "  det=" << ds.id << " (0x" << std::hex << ds.id << std::dec << ") n=" << ds.size() << "\n";
    size_t k = 0, lim = cap(maxSub_, ds.size());
    for (auto const& d : ds.data) {
      if (k >= lim)
        break;
      os << "    [" << k << "] ";
      printer(os, d);
      os << "\n";
      ++k;
    }
    truncated(os, lim, ds.size(), "    ");
    ++nd;
  }
  truncated(os, limDet, h->size());
}

template <class COLL, class PRINT>
void DigiDumper::printMuonDigis(std::ostream& os, const std::string& name, const Tagged<COLL>& t, const edm::Event& ev, PRINT printer) {
  edm::Handle<COLL> h;
  ev.getByToken(t.token, h);
  if (!h.isValid()) {
    missing(os, name, t.tag);
    return;
  }
  size_t ntot = 0, ndet = 0;
  for (auto it = h->begin(); it != h->end(); ++it) {
    ++ndet;
    ntot += std::distance((*it).second.first, (*it).second.second);
  }
  section(os, name, t.tag, ntot);
  os << "  nDetUnits=" << ndet << " nDigis=" << ntot << "\n";
  size_t nd = 0, limDet = cap(maxElements_, ndet);
  for (auto it = h->begin(); it != h->end() && nd < limDet; ++it, ++nd) {
    const auto& id = (*it).first;
    auto range = (*it).second;
    os << "  det=" << id << " rawId=" << id.rawId() << " n=" << std::distance(range.first, range.second) << "\n";
    size_t k = 0;
    for (auto d = range.first; d != range.second; ++d, ++k) {
      if (maxSub_ >= 0 && k >= static_cast<size_t>(maxSub_)) {
        os << "    ... more not printed\n";
        break;
      }
      os << "    [" << k << "] ";
      printer(os, *d);
      os << "\n";
    }
  }
  truncated(os, limDet, ndet);
}

void DigiDumper::analyze(const edm::Event& ev, const edm::EventSetup&) {
  std::ostream& os = std::cout;
  eventHeader(os, ev, step_, moduleDescription().moduleLabel());

  // --- pileup
  for (auto const& t : pileup_) {
    edm::Handle<std::vector<PileupSummaryInfo>> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "PileupSummaryInfo", t.tag);
      continue;
    }
    section(os, "PileupSummaryInfo", t.tag, h->size());
    for (auto const& p : *h)
      os << "  bx=" << p.getBunchCrossing() << " nPU=" << p.getPU_NumInteractions() << " trueNumInteractions=" << p.getTrueNumInteractions() << "\n";
  }

  // --- tracking truth
  for (auto const& t : trackingVertices_) {
    edm::Handle<TrackingVertexCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "TrackingVertices", t.tag);
      continue;
    }
    section(os, "TrackingVertices", t.tag, h->size());
    size_t lim = cap(maxElements_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const TrackingVertex& v = (*h)[i];
      os << "  [" << i << "] x=" << v.position().x() << " y=" << v.position().y() << " z=" << v.position().z() << " t=" << v.position().t()
         << " inVolume=" << v.inVolume() << " nGenVertices=" << v.nGenVertices() << " nSourceTracks=" << v.nSourceTracks()
         << " nDaughterTracks=" << v.nDaughterTracks() << " eventId(bx,evt)=" << v.eventId().bunchCrossing() << "," << v.eventId().event() << "\n";
    }
    truncated(os, lim, h->size());
  }
  for (auto const& t : trackingParticles_) {
    edm::Handle<TrackingParticleCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "TrackingParticles", t.tag);
      continue;
    }
    section(os, "TrackingParticles", t.tag, h->size());
    os << "  columns: [index] pdg charge kinematics vertex eventId nHits nTrackerHits nTrackerLayers genParticleKeys(->genIdx) simTrackIds parentVertex nDecayVertices\n";
    size_t lim = cap(maxElements_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const TrackingParticle& tp = (*h)[i];
      os << "  [" << i << "] pdg=" << tp.pdgId() << " q=" << tp.charge() << " " << kin(tp) << " " << vtx(tp)
         << " eventId(bx,evt)=" << tp.eventId().bunchCrossing() << "," << tp.eventId().event() << " nHits=" << tp.numberOfHits()
         << " nTrackerHits=" << tp.numberOfTrackerHits() << " nTrackerLayers=" << tp.numberOfTrackerLayers() << " genIdx=";
      if (tp.genParticles().empty())
        os << "-";
      for (size_t g = 0; g < tp.genParticles().size(); ++g) {
        const auto& ref = tp.genParticles()[g];
        os << (g ? "," : "") << ref.key();
        if (ref.isAvailable())
          os << "(pdg=" << ref->pdgId() << ",pt=" << ref->pt() << ",ptRatio=" << (ref->pt() > 0 ? tp.pt() / ref->pt() : 0.) << ")";
      }
      os << " simTrackIds=";
      for (size_t s = 0; s < tp.g4Tracks().size(); ++s)
        os << (s ? "," : "") << tp.g4Tracks()[s].trackId();
      if (tp.parentVertex().isNonnull())
        os << " parentVtx=(" << tp.parentVertex()->position().x() << "," << tp.parentVertex()->position().y() << "," << tp.parentVertex()->position().z() << ")";
      os << " nDecayVertices=" << tp.decayVertices().size() << "\n";
    }
    truncated(os, lim, h->size());
  }

  // --- tracker digis and links
  for (auto const& t : pixelDigis_)
    printDetSetVector<PixelDigi>(os, "PixelDigis", t, ev, [](std::ostream& o, const PixelDigi& d) {
      o << "row=" << d.row() << " col=" << d.column() << " adc=" << d.adc() << " channel=" << d.channel() << " flag=" << d.flag();
    });
  for (auto const& t : pixelLinks_)
    printDetSetVector<PixelDigiSimLink>(os, "PixelDigiSimLinks", t, ev, [](std::ostream& o, const PixelDigiSimLink& d) {
      o << "channel=" << d.channel() << " (row=" << PixelDigi::channelToPixel(d.channel()).first << " col=" << PixelDigi::channelToPixel(d.channel()).second
        << ") simTrackId=" << d.SimTrackId() << " fraction=" << d.fraction() << " tofBin=" << d.TofBin() << " eventId(bx,evt)=" << d.eventId().bunchCrossing() << "," << d.eventId().event();
    });
  for (auto const& t : stripDigis_)
    printDetSetVector<SiStripDigi>(os, "SiStripDigis", t, ev, [](std::ostream& o, const SiStripDigi& d) { o << "strip=" << d.strip() << " adc=" << d.adc(); });
  for (auto const& t : stripLinks_)
    printDetSetVector<StripDigiSimLink>(os, "StripDigiSimLinks", t, ev, [](std::ostream& o, const StripDigiSimLink& d) {
      o << "channel=" << d.channel() << " simTrackId=" << d.SimTrackId() << " fraction=" << d.fraction() << " tofBin=" << d.TofBin() << " eventId(bx,evt)=" << d.eventId().bunchCrossing() << "," << d.eventId().event();
    });

  // --- ECAL digis
  auto printEcal = [&](const std::string& name, auto const& tagged, auto idPrinter) {
    using COLL = typename std::decay_t<decltype(tagged)>::product_type;
    edm::Handle<COLL> h;
    ev.getByToken(tagged.token, h);
    if (!h.isValid()) {
      missing(os, name, tagged.tag);
      return;
    }
    section(os, name, tagged.tag, h->size());
    os << "  columns: [index] detId samples(adc:gainId ...)\n";
    size_t lim = cap(maxSub_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      typename COLL::Digi df((*h)[i]);
      os << "  [" << i << "] ";
      idPrinter(os, df);
      os << " nSamples=" << df.size() << " samples=";
      for (int s = 0; s < df.size(); ++s)
        os << (s ? " " : "") << df.sample(s).adc() << ":" << df.sample(s).gainId();
      os << "\n";
    }
    truncated(os, lim, h->size());
  };
  for (auto const& t : ebDigis_)
    printEcal("EBDigis", t, [](std::ostream& o, const EBDataFrame& df) { EBDetId id(df.id()); o << "EBDetId=" << id.rawId() << " ieta=" << id.ieta() << " iphi=" << id.iphi(); });
  for (auto const& t : eeDigis_)
    printEcal("EEDigis", t, [](std::ostream& o, const EEDataFrame& df) { EEDetId id(df.id()); o << "EEDetId=" << id.rawId() << " ix=" << id.ix() << " iy=" << id.iy() << " zside=" << id.zside(); });
  for (auto const& t : esDigis_) {
    edm::Handle<ESDigiCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "ESDigis", t.tag);
      continue;
    }
    section(os, "ESDigis", t.tag, h->size());
    size_t lim = cap(maxSub_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      ESDataFrame df((*h)[i]);
      ESDetId id(df.id());
      os << "  [" << i << "] ESDetId=" << id.rawId() << " zside=" << id.zside() << " plane=" << id.plane() << " six=" << id.six() << " siy=" << id.siy() << " strip=" << id.strip() << " samples=";
      for (int s = 0; s < df.size(); ++s)
        os << (s ? " " : "") << df.sample(s).adc();
      os << "\n";
    }
    truncated(os, lim, h->size());
  }

  // --- HCAL digis
  for (auto const& t : qie11Digis_) {
    edm::Handle<QIE11DigiCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "HcalQIE11Digis", t.tag);
      continue;
    }
    section(os, "HcalQIE11Digis (HB/HE)", t.tag, h->size());
    os << "  columns: [index] HcalDetId samples(adc:tdc:capid:soi ...)\n";
    size_t lim = cap(maxSub_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      QIE11DataFrame df((*h)[i]);
      HcalDetId id(df.detid());
      os << "  [" << i << "] " << id << " rawId=" << id.rawId() << " flavor=" << df.flavor() << " nSamples=" << df.samples() << " samples=";
      for (int s = 0; s < df.samples(); ++s)
        os << (s ? " " : "") << df[s].adc() << ":" << df[s].tdc() << ":" << df[s].capid() << ":" << df[s].soi();
      os << "\n";
    }
    truncated(os, lim, h->size());
  }
  for (auto const& t : qie10Digis_) {
    edm::Handle<QIE10DigiCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "HcalQIE10Digis", t.tag);
      continue;
    }
    section(os, "HcalQIE10Digis (HF)", t.tag, h->size());
    os << "  columns: [index] HcalDetId samples(adc:le_tdc:te_tdc:capid:soi ...)\n";
    size_t lim = cap(maxSub_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      QIE10DataFrame df((*h)[i]);
      HcalDetId id(df.detid());
      os << "  [" << i << "] " << id << " rawId=" << id.rawId() << " nSamples=" << df.samples() << " samples=";
      for (int s = 0; s < df.samples(); ++s)
        os << (s ? " " : "") << df[s].adc() << ":" << df[s].le_tdc() << ":" << df[s].te_tdc() << ":" << df[s].capid() << ":" << df[s].soi();
      os << "\n";
    }
    truncated(os, lim, h->size());
  }
  for (auto const& t : hoDigis_) {
    edm::Handle<HODigiCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "HODigis", t.tag);
      continue;
    }
    section(os, "HODigis", t.tag, h->size());
    size_t lim = cap(maxSub_, h->size());
    for (size_t i = 0; i < lim; ++i) {
      const HODataFrame& df = (*h)[i];
      os << "  [" << i << "] " << df.id() << " rawId=" << df.id().rawId() << " presamples=" << df.presamples() << " samples(adc:capid)=";
      for (int s = 0; s < df.size(); ++s)
        os << (s ? " " : "") << df.sample(s).adc() << ":" << df.sample(s).capid();
      os << "\n";
    }
    truncated(os, lim, h->size());
  }

  // --- trigger primitives
  for (auto const& t : ecalTPs_) {
    edm::Handle<EcalTrigPrimDigiCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "EcalTriggerPrimitives", t.tag);
      continue;
    }
    size_t nNonZero = 0;
    for (auto const& tp : *h)
      if (tp.compressedEt() > 0)
        ++nNonZero;
    section(os, "EcalTriggerPrimitives", t.tag, h->size());
    os << "  nWithEt>0=" << nNonZero << " (only those are listed)\n";
    size_t k = 0, lim = cap(maxSub_, nNonZero);
    for (auto const& tp : *h) {
      if (tp.compressedEt() == 0)
        continue;
      if (k >= lim)
        break;
      os << "  [" << k << "] " << tp.id() << " compressedEt=" << tp.compressedEt() << " fineGrain=" << tp.fineGrain() << " ttFlag=" << tp.ttFlag() << " samples(raw)=";
      for (int s = 0; s < tp.size(); ++s)
        os << (s ? " " : "") << tp.sample(s).raw();
      os << "\n";
      ++k;
    }
    truncated(os, lim, nNonZero);
  }
  for (auto const& t : hcalTPs_) {
    edm::Handle<HcalTrigPrimDigiCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "HcalTriggerPrimitives", t.tag);
      continue;
    }
    size_t nNonZero = 0;
    for (auto const& tp : *h)
      if (tp.SOI_compressedEt() > 0)
        ++nNonZero;
    section(os, "HcalTriggerPrimitives", t.tag, h->size());
    os << "  nWithEt>0=" << nNonZero << " (only those are listed)\n";
    size_t k = 0, lim = cap(maxSub_, nNonZero);
    for (auto const& tp : *h) {
      if (tp.SOI_compressedEt() == 0)
        continue;
      if (k >= lim)
        break;
      os << "  [" << k << "] " << tp.id() << " SOI_compressedEt=" << tp.SOI_compressedEt() << " SOI_fineGrain=" << tp.SOI_fineGrain() << " presamples=" << tp.presamples() << " samples(compressedEt:fineGrain)=";
      for (int s = 0; s < tp.size(); ++s)
        os << (s ? " " : "") << tp.sample(s).compressedEt() << ":" << tp.sample(s).fineGrain();
      os << "\n";
      ++k;
    }
    truncated(os, lim, nNonZero);
  }

  // --- muon digis
  for (auto const& t : dtDigis_)
    printMuonDigis(os, "DTDigis", t, ev, [](std::ostream& o, const DTDigi& d) { o << "wire=" << d.wire() << " number=" << d.number() << " time(ns)=" << d.time() << " countsTDC=" << d.countsTDC(); });
  for (auto const& t : cscWireDigis_)
    printMuonDigis(os, "CSCWireDigis", t, ev, [](std::ostream& o, const CSCWireDigi& d) { o << "wireGroup=" << d.getWireGroup() << " timeBin=" << d.getTimeBin() << " timeBinWord=" << d.getTimeBinWord(); });
  for (auto const& t : cscStripDigis_)
    printMuonDigis(os, "CSCStripDigis", t, ev, [](std::ostream& o, const CSCStripDigi& d) {
      o << "strip=" << d.getStrip() << " adc=";
      auto adcs = d.getADCCounts();
      for (size_t i = 0; i < adcs.size(); ++i)
        o << (i ? "," : "") << adcs[i];
    });
  for (auto const& t : cscCompDigis_)
    printMuonDigis(os, "CSCComparatorDigis", t, ev, [](std::ostream& o, const CSCComparatorDigi& d) { o << "strip=" << d.getStrip() << " comparator=" << d.getComparator() << " timeBin=" << d.getTimeBin(); });
  for (auto const& t : rpcDigis_)
    printMuonDigis(os, "RPCDigis", t, ev, [](std::ostream& o, const RPCDigi& d) { o << "strip=" << d.strip() << " bx=" << d.bx(); });
  for (auto const& t : gemDigis_)
    printMuonDigis(os, "GEMDigis", t, ev, [](std::ostream& o, const GEMDigi& d) { o << "strip=" << d.strip() << " bx=" << d.bx(); });

  // --- raw data
  for (auto const& t : raw_) {
    edm::Handle<FEDRawDataCollection> h;
    ev.getByToken(t.token, h);
    if (!h.isValid()) {
      missing(os, "FEDRawDataCollection", t.tag);
      continue;
    }
    size_t total = 0, nfed = 0;
    for (int fed = 0; fed <= FEDNumbering::MAXFEDID; ++fed)
      if (h->FEDData(fed).size()) {
        ++nfed;
        total += h->FEDData(fed).size();
      }
    section(os, "FEDRawDataCollection", t.tag, nfed);
    os << "  totalBytes=" << total << "\n";
    for (int fed = 0; fed <= FEDNumbering::MAXFEDID; ++fed)
      if (h->FEDData(fed).size())
        os << "  fedId=" << fed << " bytes=" << h->FEDData(fed).size() << "\n";
  }
  os.flush();
}

DEFINE_FWK_MODULE(DigiDumper);
