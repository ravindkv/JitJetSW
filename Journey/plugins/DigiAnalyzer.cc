// DigiAnalyzer: the digis of one event on the detector (chapter 11, DIGI, of the JitJet book).
//
// Reads the digi collections written by the DIGI step (pdigi_valid) of a GEN-SIM-DIGI file and draws
// them in two views, eta-phi (seen from the signal vertex) and r-z, with a right-hand panel of numbers:
//   tracker   pixel digis (DetSetVector<PixelDigi>, position of the pixel from the module topology) and
//             strip digis (DetSetVector<SiStripDigi>, ZeroSuppressed, position of the strip); through the
//             PixelDigiSimLink / StripDigiSimLink of every channel the digi is attributed to the signal
//             (EncodedEventId bx 0, event 0), to in-time pileup (bx 0, event > 0), to out-of-time pileup
//             (bx != 0) or to nothing (no link: an electronics-noise digi above threshold)
//   ECAL      EB / EE data frames (10 samples, amplitude = max sample - pedestal of the first 3 samples,
//             in ADC counts at gain 12) and ES frames (3 samples)
//   HCAL      QIE11 frames of HB / HE and QIE10 frames of HF (amplitude = sum of the ADC codes of the
//             samples around the signal time slice minus the pedestal), HO frames
//   muon      DT (wire), CSC (strip and wire group), RPC and GEM (strip) digis, attributed through their
//             own sim links where they exist (DTDigiSimLink, StripDigiSimLink of the CSC strips,
//             RPCDigiSimLink, GEMDigiSimLink)
// Every subdetector is counted (digis, and for the linked ones the signal / in-time / out-of-time /
// unlinked split; for the calorimeters the summed amplitude) and the counts go to std::cout, so that
// the signal-only, pileup-only and signal+pileup DIGI jobs of the chapter can be put side by side.
// The generator jets of the signal (ak4GenJets above jetPtMin) are drawn as circles of radius 0.4 so
// that the digis can be compared with the jets of the GenJet chapter.
//
// One multi-page PDF per job (pdfFile; one PNG per page with savePNG), the counts in the log.
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/CSCDigi/interface/CSCStripDigiCollection.h"
#include "DataFormats/CSCDigi/interface/CSCWireDigiCollection.h"
#include "DataFormats/Common/interface/DetSetVector.h"
#include "DataFormats/DTDigi/interface/DTDigiCollection.h"
#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/EcalDetId/interface/EBDetId.h"
#include "DataFormats/EcalDetId/interface/EEDetId.h"
#include "DataFormats/EcalDetId/interface/ESDetId.h"
#include "DataFormats/EcalDigi/interface/EcalDigiCollections.h"
#include "DataFormats/GEMDigi/interface/GEMDigiCollection.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "DataFormats/GeometryVector/interface/LocalPoint.h"
#include "DataFormats/HcalDetId/interface/HcalDetId.h"
#include "DataFormats/HcalDigi/interface/HcalDigiCollections.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "DataFormats/Math/interface/Point3D.h"
#include "DataFormats/Math/interface/deltaPhi.h"
#include "DataFormats/MuonDetId/interface/CSCDetId.h"
#include "DataFormats/MuonDetId/interface/DTLayerId.h"
#include "DataFormats/MuonDetId/interface/GEMDetId.h"
#include "DataFormats/MuonDetId/interface/RPCDetId.h"
#include "DataFormats/RPCDigi/interface/RPCDigiCollection.h"
#include "DataFormats/SiPixelDigi/interface/PixelDigi.h"
#include "DataFormats/SiStripDigi/interface/SiStripDigi.h"
#include "DataFormats/TrackerCommon/interface/TrackerTopology.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "Geometry/CSCGeometry/interface/CSCGeometry.h"
#include "Geometry/CSCGeometry/interface/CSCLayer.h"
#include "Geometry/CaloGeometry/interface/CaloCellGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloSubdetectorGeometry.h"
#include "Geometry/CommonDetUnit/interface/GeomDet.h"
#include "Geometry/CommonDetUnit/interface/PixelGeomDetUnit.h"
#include "Geometry/TrackerGeometryBuilder/interface/StripGeomDetUnit.h"
#include "Geometry/CommonTopologies/interface/PixelTopology.h"
#include "Geometry/CommonTopologies/interface/StripTopology.h"
#include "Geometry/DTGeometry/interface/DTGeometry.h"
#include "Geometry/DTGeometry/interface/DTLayer.h"
#include "Geometry/DTGeometry/interface/DTTopology.h"
#include "Geometry/GEMGeometry/interface/GEMEtaPartition.h"
#include "Geometry/GEMGeometry/interface/GEMGeometry.h"
#include "Geometry/RPCGeometry/interface/RPCGeometry.h"
#include "Geometry/RPCGeometry/interface/RPCRoll.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "Geometry/Records/interface/MuonGeometryRecord.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/Records/interface/TrackerTopologyRcd.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "SimDataFormats/DigiSimLinks/interface/DTDigiSimLinkCollection.h"
#include "SimDataFormats/GEMDigiSimLink/interface/GEMDigiSimLink.h"
#include "SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h"
#include "SimDataFormats/RPCDigiSimLink/interface/RPCDigiSimLink.h"
#include "SimDataFormats/TrackerDigiSimLink/interface/PixelDigiSimLink.h"
#include "SimDataFormats/TrackerDigiSimLink/interface/StripDigiSimLink.h"

#include "TCanvas.h"
#include "TEllipse.h"
#include "TGraph.h"
#include "TH2F.h"
#include "TLatex.h"
#include "TLine.h"
#include "TMarker.h"
#include "TPad.h"
#include "TROOT.h"
#include "TStyle.h"

namespace {

  // origin of a digi: who made it, from the sim links
  enum Origin { kSignal = 0, kInTime, kEarly, kLate, kNoLink, kNoLinkInfo, kNOrigin };
  const char* originName(int o) {
    static const char* n[] = {"signal", "pileup in-time (bx 0)", "pileup early (bx < 0)", "pileup late (bx > 0)", "no link (noise)", "no link information"};
    return n[o];
  }
  int originColor(int o) {
    static const int c[] = {kBlack, kAzure + 1, kOrange - 3, kRed - 7, kGray + 1, kGray + 1};
    return c[o];
  }
  int originOf(const EncodedEventId& id) {
    if (id.bunchCrossing() < 0)
      return kEarly;
    if (id.bunchCrossing() > 0)
      return kLate;
    return id.event() == 0 ? kSignal : kInTime;
  }

  struct Digi {
    float eta, phi, r, z;
    float amp;      // ADC amplitude above pedestal (calorimeters), 1 for the others
    int origin;
  };

  struct SubdetCount {
    long n = 0;
    long byOrigin[kNOrigin] = {0, 0, 0, 0, 0, 0};
    double amp = 0, ampByOrigin[kNOrigin] = {0, 0, 0, 0, 0, 0};
    void add(const Digi& d) {
      ++n;
      ++byOrigin[d.origin];
      amp += d.amp;
      ampByOrigin[d.origin] += d.amp;
    }
  };

  double etaOf(const GlobalPoint& p, const math::XYZPointF& v) {
    double dx = p.x() - v.x(), dy = p.y() - v.y(), dz = p.z() - v.z();
    double r = std::sqrt(dx * dx + dy * dy);
    return r > 0 ? std::asinh(dz / r) : (dz > 0 ? 10. : -10.);
  }
  double phiOf(const GlobalPoint& p, const math::XYZPointF& v) { return std::atan2(p.y() - v.y(), p.x() - v.x()); }

}  // namespace

class DigiAnalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit DigiAnalyzer(const edm::ParameterSet&);
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  void endJob() override;

private:
  template <class T>
  edm::EDGetTokenT<T> tok(const edm::ParameterSet& ps, const char* name) {
    return consumes<T>(ps.getParameter<edm::InputTag>(name));
  }
  void store(std::map<std::string, std::vector<Digi>>& digis, std::map<std::string, SubdetCount>& counts, const std::string& sub,
             const GlobalPoint& p, const math::XYZPointF& v, float amp, int origin) {
    Digi d{float(etaOf(p, v)), float(phiOf(p, v)), float(p.perp()), float(p.z()), amp, origin};
    digis[sub].push_back(d);
    counts[sub].add(d);
  }
  void draw(const edm::Event& ev, const std::map<std::string, std::vector<Digi>>& digis, const std::map<std::string, SubdetCount>& counts,
            const std::vector<std::pair<float, float>>& jets, const std::vector<std::string>& puLines, const math::XYZPointF& v);

  // event data
  edm::EDGetTokenT<edm::DetSetVector<PixelDigi>> pixelDigis_;
  edm::EDGetTokenT<edm::DetSetVector<PixelDigiSimLink>> pixelLinks_;
  edm::EDGetTokenT<edm::DetSetVector<SiStripDigi>> stripDigis_;
  edm::EDGetTokenT<edm::DetSetVector<StripDigiSimLink>> stripLinks_;
  edm::EDGetTokenT<EBDigiCollection> ebDigis_;
  edm::EDGetTokenT<EEDigiCollection> eeDigis_;
  edm::EDGetTokenT<ESDigiCollection> esDigis_;
  edm::EDGetTokenT<QIE11DigiCollection> hbheDigis_;
  edm::EDGetTokenT<QIE10DigiCollection> hfDigis_;
  edm::EDGetTokenT<HODigiCollection> hoDigis_;
  edm::EDGetTokenT<DTDigiCollection> dtDigis_;
  edm::EDGetTokenT<DTDigiSimLinkCollection> dtLinks_;
  edm::EDGetTokenT<CSCStripDigiCollection> cscStripDigis_;
  edm::EDGetTokenT<edm::DetSetVector<StripDigiSimLink>> cscStripLinks_;
  edm::EDGetTokenT<CSCWireDigiCollection> cscWireDigis_;
  edm::EDGetTokenT<edm::DetSetVector<StripDigiSimLink>> cscWireLinks_;
  edm::EDGetTokenT<RPCDigiCollection> rpcDigis_;
  edm::EDGetTokenT<edm::DetSetVector<RPCDigiSimLink>> rpcLinks_;
  edm::EDGetTokenT<GEMDigiCollection> gemDigis_;
  edm::EDGetTokenT<edm::DetSetVector<GEMDigiSimLink>> gemLinks_;
  edm::EDGetTokenT<std::vector<PileupSummaryInfo>> pileup_;
  edm::EDGetTokenT<math::XYZPointF> vertex_;
  edm::EDGetTokenT<std::vector<reco::GenJet>> genJets_;
  // geometry
  edm::ESGetToken<TrackerGeometry, TrackerDigiGeometryRecord> trackerGeom_;
  edm::ESGetToken<TrackerTopology, TrackerTopologyRcd> trackerTopo_;
  edm::ESGetToken<CaloGeometry, CaloGeometryRecord> caloGeom_;
  edm::ESGetToken<DTGeometry, MuonGeometryRecord> dtGeom_;
  edm::ESGetToken<CSCGeometry, MuonGeometryRecord> cscGeom_;
  edm::ESGetToken<RPCGeometry, MuonGeometryRecord> rpcGeom_;
  edm::ESGetToken<GEMGeometry, MuonGeometryRecord> gemGeom_;
  // options
  std::string pdfFile_, pngPrefix_, sampleLabel_;
  bool savePNG_, pdfOpen_ = false;
  double etaMax_, jetPtMin_, ecalMinAmp_, hcalMinAmp_;
  int hcalSignalTS_, hcalNSumTS_, maxDrawn_;
  std::unique_ptr<TCanvas> canvas_;
  TPad *pad_ = nullptr, *side_ = nullptr;
};

DigiAnalyzer::DigiAnalyzer(const edm::ParameterSet& ps)
    : pixelDigis_(tok<edm::DetSetVector<PixelDigi>>(ps, "pixelDigis")),
      pixelLinks_(tok<edm::DetSetVector<PixelDigiSimLink>>(ps, "pixelLinks")),
      stripDigis_(tok<edm::DetSetVector<SiStripDigi>>(ps, "stripDigis")),
      stripLinks_(tok<edm::DetSetVector<StripDigiSimLink>>(ps, "stripLinks")),
      ebDigis_(tok<EBDigiCollection>(ps, "ebDigis")),
      eeDigis_(tok<EEDigiCollection>(ps, "eeDigis")),
      esDigis_(tok<ESDigiCollection>(ps, "esDigis")),
      hbheDigis_(tok<QIE11DigiCollection>(ps, "hbheDigis")),
      hfDigis_(tok<QIE10DigiCollection>(ps, "hfDigis")),
      hoDigis_(tok<HODigiCollection>(ps, "hoDigis")),
      dtDigis_(tok<DTDigiCollection>(ps, "dtDigis")),
      dtLinks_(tok<DTDigiSimLinkCollection>(ps, "dtLinks")),
      cscStripDigis_(tok<CSCStripDigiCollection>(ps, "cscStripDigis")),
      cscStripLinks_(tok<edm::DetSetVector<StripDigiSimLink>>(ps, "cscStripLinks")),
      cscWireDigis_(tok<CSCWireDigiCollection>(ps, "cscWireDigis")),
      cscWireLinks_(tok<edm::DetSetVector<StripDigiSimLink>>(ps, "cscWireLinks")),
      rpcDigis_(tok<RPCDigiCollection>(ps, "rpcDigis")),
      rpcLinks_(tok<edm::DetSetVector<RPCDigiSimLink>>(ps, "rpcLinks")),
      gemDigis_(tok<GEMDigiCollection>(ps, "gemDigis")),
      gemLinks_(tok<edm::DetSetVector<GEMDigiSimLink>>(ps, "gemLinks")),
      pileup_(tok<std::vector<PileupSummaryInfo>>(ps, "pileupSummary")),
      vertex_(tok<math::XYZPointF>(ps, "vertex")),
      genJets_(tok<std::vector<reco::GenJet>>(ps, "genJets")),
      trackerGeom_(esConsumes()),
      trackerTopo_(esConsumes()),
      caloGeom_(esConsumes()),
      dtGeom_(esConsumes()),
      cscGeom_(esConsumes()),
      rpcGeom_(esConsumes()),
      gemGeom_(esConsumes()),
      pdfFile_(ps.getParameter<std::string>("pdfFile")),
      pngPrefix_(ps.getParameter<std::string>("pngPrefix")),
      sampleLabel_(ps.getParameter<std::string>("sampleLabel")),
      savePNG_(ps.getParameter<bool>("savePNG")),
      etaMax_(ps.getParameter<double>("etaMax")),
      jetPtMin_(ps.getParameter<double>("jetPtMin")),
      ecalMinAmp_(ps.getParameter<double>("ecalMinAmp")),
      hcalMinAmp_(ps.getParameter<double>("hcalMinAmp")),
      hcalSignalTS_(ps.getParameter<int>("hcalSignalTS")),
      hcalNSumTS_(ps.getParameter<int>("hcalNSumTS")),
      maxDrawn_(ps.getParameter<int>("maxDrawn")) {
  usesResource(TFileService::kSharedResource);
  gROOT->SetBatch(true);
  gStyle->SetOptStat(0);
  gStyle->SetPaperSize(29., 20.);
  canvas_ = std::make_unique<TCanvas>("digiCanvas", "", 1400, 900);
  pad_ = new TPad("digiPad", "", 0., 0., 0.72, 1.);
  side_ = new TPad("digiSide", "", 0.72, 0., 1., 1.);
  pad_->SetLeftMargin(0.08);
  pad_->SetRightMargin(0.02);
  canvas_->cd();
  pad_->Draw();
  side_->Draw();
}

void DigiAnalyzer::analyze(const edm::Event& ev, const edm::EventSetup& es) {
  const TrackerGeometry& tg = es.getData(trackerGeom_);
  const TrackerTopology& tt = es.getData(trackerTopo_);
  const CaloGeometry& cg = es.getData(caloGeom_);
  const DTGeometry& dtg = es.getData(dtGeom_);
  const CSCGeometry& cscg = es.getData(cscGeom_);
  const RPCGeometry& rpcg = es.getData(rpcGeom_);
  const GEMGeometry& gemg = es.getData(gemGeom_);

  math::XYZPointF vtx(0, 0, 0);
  {
    edm::Handle<math::XYZPointF> h;
    ev.getByToken(vertex_, h);
    if (h.isValid())
      vtx = *h;
  }
  std::map<std::string, std::vector<Digi>> digis;
  std::map<std::string, SubdetCount> counts;

  // ---------------------------------------------------------------- tracker
  const char* trkNames[] = {"", "PXB", "PXF", "TIB", "TID", "TOB", "TEC"};
  {
    edm::Handle<edm::DetSetVector<PixelDigi>> hd;
    edm::Handle<edm::DetSetVector<PixelDigiSimLink>> hl;
    ev.getByToken(pixelDigis_, hd);
    ev.getByToken(pixelLinks_, hl);
    if (hd.isValid()) {
      for (auto const& ds : *hd) {
        DetId id(ds.id);
        const auto* du = dynamic_cast<const PixelGeomDetUnit*>(tg.idToDetUnit(id));
        if (!du)
          continue;
        std::map<int, std::pair<float, EncodedEventId>> best;  // channel -> (largest fraction, event id)
        if (hl.isValid()) {
          auto it = hl->find(ds.id);
          if (it != hl->end())
            for (auto const& l : *it)
              if (!best.count(l.channel()) || l.fraction() > best[l.channel()].first)
                best[l.channel()] = {l.fraction(), l.eventId()};
        }
        const std::string sub = trkNames[id.subdetId()];
        for (auto const& d : ds.data) {
          LocalPoint lp = du->specificTopology().localPosition(MeasurementPoint(d.row() + 0.5, d.column() + 0.5));
          GlobalPoint gp = du->surface().toGlobal(lp);
          int origin = hl.isValid() ? (best.count(d.channel()) ? originOf(best[d.channel()].second) : kNoLink) : kNoLinkInfo;
          store(digis, counts, sub, gp, vtx, 1.f, origin);
        }
      }
    }
  }
  {
    edm::Handle<edm::DetSetVector<SiStripDigi>> hd;
    edm::Handle<edm::DetSetVector<StripDigiSimLink>> hl;
    ev.getByToken(stripDigis_, hd);
    ev.getByToken(stripLinks_, hl);
    if (hd.isValid()) {
      for (auto const& ds : *hd) {
        DetId id(ds.id);
        const auto* du = dynamic_cast<const StripGeomDetUnit*>(tg.idToDetUnit(id));
        if (!du)
          continue;
        std::map<int, std::pair<float, EncodedEventId>> best;
        if (hl.isValid()) {
          auto it = hl->find(ds.id);
          if (it != hl->end())
            for (auto const& l : *it)
              if (!best.count(l.channel()) || l.fraction() > best[l.channel()].first)
                best[l.channel()] = {l.fraction(), l.eventId()};
        }
        const std::string sub = trkNames[id.subdetId()];
        for (auto const& d : ds.data) {
          LocalPoint lp = du->specificTopology().localPosition(d.strip() + 0.5f);
          GlobalPoint gp = du->surface().toGlobal(lp);
          int origin = hl.isValid() ? (best.count(d.strip()) ? originOf(best[d.strip()].second) : kNoLink) : kNoLinkInfo;
          store(digis, counts, sub, gp, vtx, 1.f, origin);
        }
      }
    }
  }
  (void)tt;

  // ---------------------------------------------------------------- ECAL
  auto ecalFrame = [&](const std::string& sub, const DetId& id, int nSamples, int npedSamples, auto adcOf) {
    auto cell = cg.getGeometry(id);
    if (!cell)
      return;
    float ped = 0;
    int nped = std::min(npedSamples, nSamples);
    for (int i = 0; i < nped; ++i)
      ped += adcOf(i);
    ped /= std::max(1, nped);
    float amp = 0;
    for (int i = 0; i < nSamples; ++i)
      amp = std::max(amp, float(adcOf(i)) - ped);
    if (amp < ecalMinAmp_)
      return;
    store(digis, counts, sub, cell->getPosition(), vtx, amp, kNoLinkInfo);
  };
  {
    edm::Handle<EBDigiCollection> h;
    ev.getByToken(ebDigis_, h);
    if (h.isValid())
      for (auto it = h->begin(); it != h->end(); ++it) {
        EBDataFrame df(*it);
        ecalFrame("EB", df.id(), df.size(), 3, [&](int i) {  // gain 12 equivalent counts
          int g = df.sample(i).gainId();
          return df.sample(i).adc() * (g == 1 ? 1. : g == 2 ? 2. : g == 3 ? 12. : 1.);
        });
      }
  }
  {
    edm::Handle<EEDigiCollection> h;
    ev.getByToken(eeDigis_, h);
    if (h.isValid())
      for (auto it = h->begin(); it != h->end(); ++it) {
        EEDataFrame df(*it);
        ecalFrame("EE", df.id(), df.size(), 3, [&](int i) {
          int g = df.sample(i).gainId();
          return df.sample(i).adc() * (g == 1 ? 1. : g == 2 ? 2. : g == 3 ? 12. : 1.);
        });
      }
  }
  {
    edm::Handle<ESDigiCollection> h;
    ev.getByToken(esDigis_, h);
    if (h.isValid())
      for (auto it = h->begin(); it != h->end(); ++it) {
        ESDataFrame df(*it);
        ecalFrame("ES", df.id(), df.size(), 1, [&](int i) { return double(df.sample(i).adc()); });
      }
  }

  // ---------------------------------------------------------------- HCAL
  auto hcalFrame = [&](const HcalDetId& id, int nSamples, int soi, auto adcOf) {
    auto cell = cg.getGeometry(id);
    if (!cell)
      return;
    if (hcalSignalTS_ >= 0)
      soi = hcalSignalTS_;  // the configuration overrides the sample of interest of the frame
    float ped = 0;
    int nped = 0;
    for (int i = 0; i < std::min(nSamples, soi); ++i) {
      ped += adcOf(i);
      ++nped;
    }
    ped = nped ? ped / nped : 0;
    float amp = 0;
    for (int i = soi; i < std::min(nSamples, soi + hcalNSumTS_); ++i)
      amp += adcOf(i) - ped;
    if (amp < hcalMinAmp_)
      return;
    static const char* names[] = {"", "HB", "HE", "HO", "HF"};
    int s = id.subdet();
    store(digis, counts, (s >= 1 && s <= 4) ? names[s] : "HCALother", cell->getPosition(), vtx, amp, kNoLinkInfo);
  };
  {
    edm::Handle<QIE11DigiCollection> h;
    ev.getByToken(hbheDigis_, h);
    if (h.isValid())
      for (auto it = h->begin(); it != h->end(); ++it) {
        QIE11DataFrame df = *it;
        hcalFrame(HcalDetId(df.id()), df.samples(), df.presamples(), [&](int i) { return double(df[i].adc()); });
      }
  }
  {
    edm::Handle<QIE10DigiCollection> h;
    ev.getByToken(hfDigis_, h);
    if (h.isValid())
      for (auto it = h->begin(); it != h->end(); ++it) {
        QIE10DataFrame df = *it;
        hcalFrame(HcalDetId(df.id()), df.samples(), df.presamples(), [&](int i) { return double(df[i].adc()); });
      }
  }
  {
    edm::Handle<HODigiCollection> h;
    ev.getByToken(hoDigis_, h);
    if (h.isValid())
      for (auto const& df : *h)
        hcalFrame(df.id(), df.size(), df.presamples(), [&](int i) { return double(df.sample(i).adc()); });
  }

  // ---------------------------------------------------------------- muon system
  {
    edm::Handle<DTDigiCollection> hd;
    edm::Handle<DTDigiSimLinkCollection> hl;
    ev.getByToken(dtDigis_, hd);
    ev.getByToken(dtLinks_, hl);
    if (hd.isValid()) {
      for (auto it = hd->begin(); it != hd->end(); ++it) {
        const DTLayerId& id = (*it).first;
        const DTLayer* layer = dtg.layer(id);
        if (!layer)
          continue;
        std::map<std::pair<int, int>, EncodedEventId> links;  // (wire, digi number) -> event id
        if (hl.isValid()) {
          auto lr = hl->get(id);
          for (auto l = lr.first; l != lr.second; ++l)
            links[{l->wire(), l->number()}] = l->eventId();
        }
        auto range = (*it).second;
        for (auto d = range.first; d != range.second; ++d) {
          float x = layer->specificTopology().wirePosition(d->wire());
          GlobalPoint gp = layer->surface().toGlobal(LocalPoint(x, 0, 0));
          int origin = kNoLinkInfo;
          if (hl.isValid()) {
            auto f = links.find({d->wire(), d->number()});
            origin = f != links.end() ? originOf(f->second) : kNoLink;
          }
          store(digis, counts, "DT", gp, vtx, 1.f, origin);
        }
      }
    }
  }
  auto cscLinks = [&](const edm::Handle<edm::DetSetVector<StripDigiSimLink>>& hl, uint32_t rawId, std::map<int, std::pair<float, EncodedEventId>>& best) {
    if (!hl.isValid())
      return;
    auto it = hl->find(rawId);
    if (it == hl->end())
      return;
    for (auto const& l : *it)
      if (!best.count(l.channel()) || l.fraction() > best[l.channel()].first)
        best[l.channel()] = {l.fraction(), l.eventId()};
  };
  {
    edm::Handle<CSCStripDigiCollection> hd;
    edm::Handle<edm::DetSetVector<StripDigiSimLink>> hl;
    ev.getByToken(cscStripDigis_, hd);
    ev.getByToken(cscStripLinks_, hl);
    if (hd.isValid()) {
      for (auto it = hd->begin(); it != hd->end(); ++it) {
        const CSCDetId& id = (*it).first;
        const CSCLayer* layer = cscg.layer(id);
        if (!layer)
          continue;
        std::map<int, std::pair<float, EncodedEventId>> best;
        cscLinks(hl, id.rawId(), best);
        auto range = (*it).second;
        for (auto d = range.first; d != range.second; ++d) {
          GlobalPoint gp = layer->centerOfStrip(d->getStrip());
          int origin = hl.isValid() ? (best.count(d->getStrip()) ? originOf(best[d->getStrip()].second) : kNoLink) : kNoLinkInfo;
          store(digis, counts, "CSCstrip", gp, vtx, 1.f, origin);
        }
      }
    }
  }
  {
    edm::Handle<CSCWireDigiCollection> hd;
    edm::Handle<edm::DetSetVector<StripDigiSimLink>> hl;
    ev.getByToken(cscWireDigis_, hd);
    ev.getByToken(cscWireLinks_, hl);
    if (hd.isValid()) {
      for (auto it = hd->begin(); it != hd->end(); ++it) {
        const CSCDetId& id = (*it).first;
        const CSCLayer* layer = cscg.layer(id);
        if (!layer)
          continue;
        std::map<int, std::pair<float, EncodedEventId>> best;
        cscLinks(hl, id.rawId(), best);
        auto range = (*it).second;
        for (auto d = range.first; d != range.second; ++d) {
          GlobalPoint gp = layer->centerOfWireGroup(d->getWireGroup());
          int origin = hl.isValid() ? (best.count(d->getWireGroup()) ? originOf(best[d->getWireGroup()].second) : kNoLink) : kNoLinkInfo;
          store(digis, counts, "CSCwire", gp, vtx, 1.f, origin);
        }
      }
    }
  }
  {
    edm::Handle<RPCDigiCollection> hd;
    edm::Handle<edm::DetSetVector<RPCDigiSimLink>> hl;
    ev.getByToken(rpcDigis_, hd);
    ev.getByToken(rpcLinks_, hl);
    if (hd.isValid()) {
      for (auto it = hd->begin(); it != hd->end(); ++it) {
        const RPCDetId& id = (*it).first;
        const RPCRoll* roll = rpcg.roll(id);
        if (!roll)
          continue;
        std::map<std::pair<int, int>, EncodedEventId> links;  // (strip, bx)
        if (hl.isValid()) {
          auto f = hl->find(id.rawId());
          if (f != hl->end())
            for (auto const& l : *f)
              links[{int(l.getStrip()), int(l.getBx())}] = l.getEventId();
        }
        auto range = (*it).second;
        for (auto d = range.first; d != range.second; ++d) {
          GlobalPoint gp = roll->toGlobal(roll->centreOfStrip(d->strip()));
          int origin = kNoLinkInfo;
          if (hl.isValid()) {
            auto f = links.find({d->strip(), d->bx()});
            origin = f != links.end() ? originOf(f->second) : kNoLink;
          }
          store(digis, counts, "RPC", gp, vtx, 1.f, origin);
        }
      }
    }
  }
  {
    edm::Handle<GEMDigiCollection> hd;
    edm::Handle<edm::DetSetVector<GEMDigiSimLink>> hl;
    ev.getByToken(gemDigis_, hd);
    ev.getByToken(gemLinks_, hl);
    if (hd.isValid()) {
      for (auto it = hd->begin(); it != hd->end(); ++it) {
        const GEMDetId& id = (*it).first;
        const GEMEtaPartition* part = gemg.etaPartition(id);
        if (!part)
          continue;
        std::map<std::pair<int, int>, EncodedEventId> links;
        if (hl.isValid()) {
          auto f = hl->find(id.rawId());
          if (f != hl->end())
            for (auto const& l : *f)
              links[{int(l.getStrip()), l.getBx()}] = l.getEventId();
        }
        auto range = (*it).second;
        for (auto d = range.first; d != range.second; ++d) {
          GlobalPoint gp = part->toGlobal(part->centreOfStrip(d->strip()));
          int origin = kNoLinkInfo;
          if (hl.isValid()) {
            auto f = links.find({d->strip(), d->bx()});
            origin = f != links.end() ? originOf(f->second) : kNoLink;
          }
          store(digis, counts, "GEM", gp, vtx, 1.f, origin);
        }
      }
    }
  }

  // ---------------------------------------------------------------- pileup summary, jets
  std::vector<std::string> puLines;
  {
    edm::Handle<std::vector<PileupSummaryInfo>> h;
    ev.getByToken(pileup_, h);
    if (h.isValid())
      for (auto const& p : *h)
        puLines.push_back(Form("bx %d: %d pileup events", p.getBunchCrossing(), p.getPU_NumInteractions()));
  }
  std::vector<std::pair<float, float>> jets;
  {
    edm::Handle<std::vector<reco::GenJet>> h;
    ev.getByToken(genJets_, h);
    if (h.isValid())
      for (auto const& j : *h)
        if (j.pt() > jetPtMin_)
          jets.push_back({float(j.eta()), float(j.phi())});
  }

  // ---------------------------------------------------------------- the log
  std::cout << "== DigiAnalyzer run:lumi:event " << ev.id().run() << ":" << ev.luminosityBlock() << ":" << ev.id().event()
            << " vertex=(" << vtx.x() << "," << vtx.y() << "," << vtx.z() << ") cm";
  for (auto const& l : puLines)
    std::cout << " | " << l;
  std::cout << "\n";
  const char* order[] = {"PXB", "PXF", "TIB", "TID", "TOB", "TEC", "EB", "EE", "ES", "HB", "HE", "HO", "HF", "DT", "CSCstrip", "CSCwire", "RPC", "GEM"};
  long total = 0;
  for (auto const& name : order) {
    auto it = counts.find(name);
    if (it == counts.end())
      continue;
    const SubdetCount& c = it->second;
    total += c.n;
    std::cout << "  " << name << ": nDigis=" << c.n;
    if (c.amp > 0 && c.byOrigin[kNoLinkInfo] == c.n)
      std::cout << " sumAmplitude[ADC]=" << c.amp;
    if (c.byOrigin[kNoLinkInfo] != c.n)
      std::cout << " signal=" << c.byOrigin[kSignal] << " inTime=" << c.byOrigin[kInTime] << " early=" << c.byOrigin[kEarly] << " late=" << c.byOrigin[kLate]
                << " noLink=" << c.byOrigin[kNoLink];
    std::cout << "\n";
  }
  std::cout << "  total digis=" << total << " genJets(pt>" << jetPtMin_ << ")=" << jets.size() << "\n";

  draw(ev, digis, counts, jets, puLines, vtx);
}

void DigiAnalyzer::draw(const edm::Event& ev, const std::map<std::string, std::vector<Digi>>& digis, const std::map<std::string, SubdetCount>& counts,
                        const std::vector<std::pair<float, float>>& jets, const std::vector<std::string>& puLines, const math::XYZPointF& vtx) {
  TCanvas* c = canvas_.get();
  const char* order[] = {"PXB", "PXF", "TIB", "TID", "TOB", "TEC", "EB", "EE", "ES", "HB", "HE", "HO", "HF", "DT", "CSCstrip", "CSCwire", "RPC", "GEM"};
  auto group = [](const std::string& s) {  // 0 tracker, 1 ECAL, 2 HCAL, 3 muon
    if (s == "EB" || s == "EE" || s == "ES")
      return 1;
    if (s == "HB" || s == "HE" || s == "HO" || s == "HF")
      return 2;
    if (s == "DT" || s.rfind("CSC", 0) == 0 || s == "RPC" || s == "GEM")
      return 3;
    return 0;
  };
  const int groupColor[] = {kBlack, kGreen + 2, kRed + 1, kCyan + 2};
  const char* groupName[] = {"tracker (pixel + strip)", "ECAL (EB, EE, ES)", "HCAL (HB, HE, HO, HF)", "muon (DT, CSC, RPC, GEM)"};

  for (int view = 0; view < 2; ++view) {
    TPad* pad = pad_;
    TPad* side = side_;
    pad->Clear();
    side->Clear();
    pad->cd();
    std::vector<std::unique_ptr<TObject>> keep;
    TH2F* frame;
    if (view == 0)
      frame = new TH2F("frameEtaPhi", Form("digis of run %u event %llu, #eta-#phi from the signal vertex;#eta;#phi", ev.id().run(), ev.id().event()), 10, -etaMax_, etaMax_, 10, -M_PI, M_PI);
    else
      frame = new TH2F("frameRZ", Form("digis of run %u event %llu, r-z;z [cm];r [cm]", ev.id().run(), ev.id().event()), 10, -1200., 1200., 10, 0., 800.);
    keep.emplace_back(frame);
    frame->Draw();
    bool thinned = false;
    // tracker and muon digis as graphs per origin, calorimeter digis as markers sized by amplitude
    for (auto const& name : order) {
      auto it = digis.find(name);
      if (it == digis.end())
        continue;
      int g = group(name);
      if (g == 1 || g == 2) {
        const std::string sub = name;
        for (auto const& d : it->second) {
          double x = view == 0 ? d.eta : d.z, y = view == 0 ? d.phi : d.r;
          double size = 0.25 + 0.35 * std::log10(1. + d.amp);  // 1 ADC -> 0.36, 100 ADC -> 0.95, 10000 -> 1.65
          auto m = std::make_unique<TMarker>(x, y, sub == "ES" ? 20 : sub == "HF" ? 23 : g == 1 ? 21 : 22);
          m->SetMarkerColor(sub == "ES" ? kGreen - 6 : sub == "HF" ? kRed + 2 : groupColor[g]);
          m->SetMarkerSize(sub == "ES" ? 0.5 * size : size);
          m->Draw();
          keep.push_back(std::move(m));
        }
      } else {
        std::vector<double> xs[kNOrigin], ys[kNOrigin];
        // every k-th digi of the subdetector is drawn so that at most maxDrawn points go into the page (the counts stay exact)
        long k = (maxDrawn_ > 0 && long(it->second.size()) > maxDrawn_) ? (long(it->second.size()) + maxDrawn_ - 1) / maxDrawn_ : 1;
        thinned = thinned || k > 1;
        long i = 0;
        for (auto const& d : it->second) {
          if (i++ % k)
            continue;
          xs[d.origin].push_back(view == 0 ? d.eta : d.z);
          ys[d.origin].push_back(view == 0 ? d.phi : d.r);
        }
        for (int o = 0; o < kNOrigin; ++o) {
          if (xs[o].empty())
            continue;
          auto gr = std::make_unique<TGraph>(xs[o].size(), xs[o].data(), ys[o].data());
          gr->SetMarkerStyle(g == 3 ? 24 : 20);
          gr->SetMarkerSize(g == 3 ? 0.6 : 0.25);
          gr->SetMarkerColor(o == kNoLinkInfo ? groupColor[g] : originColor(o));
          gr->Draw("P same");
          keep.push_back(std::move(gr));
        }
      }
    }
    if (view == 0)
      for (auto const& j : jets) {
        auto e = std::make_unique<TEllipse>(j.first, j.second, 0.4, 0.4);
        e->SetFillStyle(0);
        e->SetLineColor(kMagenta + 1);
        e->SetLineWidth(2);
        e->Draw();
        keep.push_back(std::move(e));
      }
    // the right panel
    side->cd();
    double y = 0.96;
    auto text = [&](const std::string& s, double size = 0.024, int color = kBlack) {
      auto t = std::make_unique<TLatex>(0.04, y, s.c_str());
      t->SetNDC();
      t->SetTextSize(size);
      t->SetTextColor(color);
      t->SetTextFont(42);
      t->Draw();
      keep.push_back(std::move(t));
      y -= size * 1.12;
    };
    std::string label = sampleLabel_, line;
    size_t pos;
    while ((pos = label.find('|')) != std::string::npos) {
      text(label.substr(0, pos), 0.026);
      label = label.substr(pos + 1);
    }
    text(label, 0.026);
    y -= 0.008;
    text(Form("vertex z = %.2f cm", vtx.z()));
    for (auto const& l : puLines)
      text(l);
    y -= 0.01;
    for (int g = 0; g < 4; ++g) {
      long n = 0;
      double amp = 0;
      long byO[kNOrigin] = {0, 0, 0, 0, 0, 0};
      for (auto const& name : order) {
        if (group(name) != g)
          continue;
        auto it = counts.find(name);
        if (it == counts.end())
          continue;
        n += it->second.n;
        amp += it->second.amp;
        for (int o = 0; o < kNOrigin; ++o)
          byO[o] += it->second.byOrigin[o];
      }
      text(Form("%s: %ld digis", groupName[g], n), 0.024, groupColor[g]);
      if (g == 1 || g == 2)
        text(Form("   sum amplitude %.0f ADC", amp), 0.02);
      else if (byO[kNoLinkInfo] != n)
        for (int o = 0; o < kNoLinkInfo; ++o)
          if (byO[o])
            text(Form("   %s: %ld", originName(o), byO[o]), 0.02, originColor(o));
      std::string subs;
      for (auto const& name : order) {
        if (group(name) != g)
          continue;
        auto it = counts.find(name);
        if (it != counts.end())
          subs += Form("%s%s %ld", subs.empty() ? "   " : ", ", name, it->second.n);
      }
      if (!subs.empty())
        text(subs, 0.018, kGray + 2);
    }
    y -= 0.01;
    text(Form("circles: ak4GenJets of the signal, p_{T} > %.0f GeV", jetPtMin_), 0.02, kMagenta + 1);
    text("calorimeter marker size: log(1 + ADC); ES light green, HF dark red", 0.018, kGray + 2);
    if (thinned)
      text(Form("tracker / muon digis thinned for drawing (at most %d per subdetector)", maxDrawn_), 0.02, kGray + 2);
    c->cd();
    c->Update();
    if (!pdfFile_.empty()) {
      if (!pdfOpen_) {
        c->Print((pdfFile_ + "[").c_str());
        pdfOpen_ = true;
      }
      c->Print(pdfFile_.c_str());
    }
    if (savePNG_)
      c->SaveAs(Form("%s_%s_run%u_event%llu.png", pngPrefix_.c_str(), view == 0 ? "etaphi" : "rz", ev.id().run(), ev.id().event()));
    pad->Clear();
    side->Clear();
    keep.clear();
  }
}

void DigiAnalyzer::endJob() {
  if (pdfOpen_)
    canvas_->Print((pdfFile_ + "]").c_str());
}

DEFINE_FWK_MODULE(DigiAnalyzer);
