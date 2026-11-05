// SimHitAnalyzer: the SimHits of one event drawn on the detector, station by station (JitJet book, SIM chapter).
//
// The particles handed to Geant4 (the primaries: SimTracks with a generator link) leave PSimHits in the
// tracker and the muon chambers and PCaloHits in the calorimeter cells. For every "station" of the journey
// (tracker, ECAL, HCAL, HGCal, muon system ... configured as lists of hit collections) three views are
// drawn per event, each with the detector elements of the station taken from the reco geometry of the
// EventSetup (module centres of the tracker, cell centres of the calorimeters, chamber outlines of the muon
// system) in grey, and the hits on top, coloured by the kind of the primary the hit descends from
// (charged hadron, neutral hadron, photon, electron, muon, other):
//   r-z      longitudinal view, r = sqrt(x^2 + y^2) against z
//   x-y      transverse view of the barrel part of the station (barrel elements and barrel hits only)
//   eta-phi  the hits as seen from the primary vertex
// Every hit is assigned to a primary through the SimTrack / SimVertex parent links (a PSimHit carries the
// Geant4 track id of the track that made it, a PCaloHit the id of the track that entered the calorimeter;
// the ancestor primary of that track is found by walking the vertex parents). Hits of tracks that were not
// kept as SimTracks (delta rays and other secondaries below the persistency thresholds) are "unassigned".
// For a few leading primaries (the highest-pT one of every kind with hits in the station, HIGHLIGHT per kind)
// the hits are connected in time order and labelled, so that a single particle can be followed through the
// layers; their calorimeter cells are circled. Optionally the point where every primary crossed the
// tracker / calorimeter boundary (SimTrack::trackerSurfacePosition) is drawn as a cross (drawCaloEntry).
// Output per station <name> and event: <outputDir>/<name>/<prefix>_<name>_run<R>_event<E>.pdf with the three
// pages (ROOT can keep only one multi-page PDF open, so one PDF per station and event), and with savePNG also
// one PNG per page (<prefix>_<name>_<view>_run<R>_event<E>.png). The per-station hit counts per subdetector,
// layer / station and kind go to std::cout.
// Calorimeter cells are summed per cell over the hits; a cell takes the kind of the primary with the largest
// energy share. HCAL hit ids in the simulation test numbering are relabelled (hcalTestNumbering).
// With crossingFrame = true the analyzer runs inside a DIGI job with pileup mixing and reads the CrossingFrame
// collections of the MixingModule instead of the SIM collections (mix:g4SimHits<Collection>): the signal hits
// are drawn as above, coloured by the kind of their signal primary, and the pileup hits added by the mixing are
// drawn underneath, coloured by their bunch crossing (early bx < 0, in-time bx 0, late bx > 0); a calorimeter
// cell whose energy is dominated by pileup takes the pileup colour. The pileup events are counted per bunch
// crossing from the EncodedEventId of their SimTracks. CrossingFrames are transient, so this mode only works
// in the mixing job itself (JitJetSW.Journey.simHitAnalyzer_cff.addSimHitAnalyzer, runPileup_mcPhase2_BBbar.sh).
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "TCanvas.h"
#include "TGraph.h"
#include "TH2D.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLine.h"
#include "TMarker.h"
#include "TPolyLine.h"
#include "TROOT.h"
#include "TStyle.h"

#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/EcalDetId/interface/EcalSubdetector.h"
#include "DataFormats/ForwardDetId/interface/ForwardSubdetector.h"
#include "DataFormats/ForwardDetId/interface/HGCScintillatorDetId.h"
#include "DataFormats/ForwardDetId/interface/HGCSiliconDetId.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "DataFormats/GeometryVector/interface/LocalPoint.h"
#include "DataFormats/HcalDetId/interface/HcalSubdetector.h"
#include "DataFormats/Math/interface/deltaPhi.h"
#include "DataFormats/MuonDetId/interface/CSCDetId.h"
#include "DataFormats/MuonDetId/interface/DTChamberId.h"
#include "DataFormats/MuonDetId/interface/GEMDetId.h"
#include "DataFormats/MuonDetId/interface/RPCDetId.h"
#include "DataFormats/TrackerCommon/interface/TrackerTopology.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "Geometry/CSCGeometry/interface/CSCChamber.h"
#include "Geometry/CSCGeometry/interface/CSCGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloCellGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloSubdetectorGeometry.h"
#include "Geometry/CommonDetUnit/interface/GeomDet.h"
#include "Geometry/CommonTopologies/interface/GlobalTrackingGeometry.h"
#include "Geometry/DTGeometry/interface/DTChamber.h"
#include "Geometry/DTGeometry/interface/DTGeometry.h"
#include "Geometry/GEMGeometry/interface/GEMChamber.h"
#include "Geometry/GEMGeometry/interface/GEMGeometry.h"
#include "Geometry/HGCalGeometry/interface/HGCalGeometry.h"
#include "Geometry/HcalCommonData/interface/HcalDDDRecConstants.h"
#include "Geometry/HcalCommonData/interface/HcalHitRelabeller.h"
#include "Geometry/RPCGeometry/interface/RPCChamber.h"
#include "Geometry/RPCGeometry/interface/RPCGeometry.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "Geometry/Records/interface/GlobalTrackingGeometryRecord.h"
#include "Geometry/Records/interface/HcalRecNumberingRecord.h"
#include "Geometry/Records/interface/MuonGeometryRecord.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/Records/interface/TrackerTopologyRcd.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "SimDataFormats/CaloHit/interface/PCaloHitContainer.h"
#include "SimDataFormats/CrossingFrame/interface/CrossingFrame.h"
#include "SimDataFormats/CrossingFrame/interface/MixCollection.h"
#include "SimDataFormats/EncodedEventId/interface/EncodedEventId.h"
#include "SimDataFormats/Track/interface/SimTrackContainer.h"
#include "SimDataFormats/TrackingHit/interface/PSimHitContainer.h"
#include "SimDataFormats/Vertex/interface/SimVertexContainer.h"

class SimHitAnalyzer : public edm::one::EDAnalyzer<> {
public:
  explicit SimHitAnalyzer(const edm::ParameterSet& ps);
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  // kind of the primary a hit descends from
  // (the three pileup kinds exist only with crossingFrame: hits of the mixed pileup events by bunch crossing)
  enum Kind { CHARGED_HADRON = 0, NEUTRAL_HADRON, PHOTON, ELECTRON, MUON, OTHER_KIND, UNASSIGNED, PU_EARLY, PU_INTIME, PU_LATE, NKIND };
  static constexpr const char* kKindName[NKIND] = {"charged hadron", "neutral hadron", "photon", "e^{#pm}", "#mu^{#pm}", "other",
                                                   "no stored ancestor", "pileup, early (bx < 0)", "pileup, in-time (bx 0)",
                                                   "pileup, late (bx > 0)"};
  static constexpr int kKindColor[NKIND] = {kBlack,   kMagenta + 2, kGreen + 2, kOrange + 7, kCyan + 2,
                                            kViolet + 1, kGray + 1, kYellow + 2, kAzure - 4,  kRed - 7};
  static constexpr int kKindMarker[NKIND] = {20, 21, 20, 21, 22, 20, 20, 20, 20, 20};
  static bool isPileup(int kind) { return kind >= PU_EARLY; }
  // owner of a hit: >= 0 index of the signal primary it descends from, -1 no stored ancestor,
  // -2 / -3 / -4 a pileup event of an early / in-time / late bunch crossing
  static int pileupOwner(int bx) { return bx < 0 ? -2 : (bx == 0 ? -3 : -4); }

  // subdetector of a hit or of a detector element
  enum Sub { PXB = 0, PXF, TIB, TID, TOB, TEC, DT, CSC, RPC, GEM, EB, EE, ES, HB, HE, HO, HF, HGCEE, HGCHSI, HGCHSC, OTHER_SUB, NSUB };
  static constexpr const char* kSubName[NSUB] = {"PXB", "PXF", "TIB", "TID", "TOB", "TEC", "DT", "CSC", "RPC", "GEM", "EB",
                                                 "EE",  "ES",  "HB",  "HE",  "HO",  "HF",  "HGC EE", "HGC HSi", "HGC HSc", "other"};
  // keys of the geometry groups in the station configuration
  static constexpr const char* kGeoKey[NSUB] = {"tracker", "tracker", "tracker", "tracker", "tracker", "tracker", "DT",
                                                "CSC",     "RPC",     "GEM",     "EB",      "EE",      "ES",      "HB",
                                                "HE",      "HO",      "HF",      "HGCEE",   "HGCHSi",  "HGCHSc",  ""};

  struct Station {
    std::string name, title;
    std::vector<std::pair<edm::InputTag, edm::EDGetTokenT<edm::PSimHitContainer>>> psimTok;
    std::vector<std::pair<edm::InputTag, edm::EDGetTokenT<edm::PCaloHitContainer>>> caloTok;
    std::vector<std::pair<edm::InputTag, edm::EDGetTokenT<CrossingFrame<PSimHit>>>> psimCFTok;  // crossingFrame mode
    std::vector<std::pair<edm::InputTag, edm::EDGetTokenT<CrossingFrame<PCaloHit>>>> caloCFTok;
    bool hasCalo = false;
    std::vector<std::string> geometry;
    double zMax, rMax, xyMax, etaMax;
    bool drawCaloEntry;
  };
  struct Hit {  // a PSimHit, or a calorimeter cell (calo = true, e = summed energy)
    float x, y, z, r, eta, phi, tof, e;
    int sub, layer, prim, kind;
    bool calo, barrel;
  };
  struct GeoPt {
    float x, y, z;
    int sub;
    bool barrel;
  };
  struct GeoPoly {
    std::array<std::array<float, 3>, 4> c;
    int sub;
    bool barrel;
  };
  struct Primary {  // a SimTrack handed to Geant4 (signal event)
    const SimTrack* trk;
    int kind;
    double pt, eta, phi;
    std::string name;
  };
  struct SimEvent {  // the SimTracks and SimVertices of one simulated event: the signal, or one mixed pileup event
    bool signal;
    int bx;
    EncodedEventId id;
    std::vector<const SimTrack*> tracks;
    std::vector<const SimVertex*> vertices;
    std::unordered_map<unsigned int, int> trackIdx;  // Geant4 track id -> index in tracks
    std::vector<int> anc;                             // index in tracks of the ancestor primary of every track (-1: none)
  };
  struct PileupSummary {  // crossingFrame mode: the mixed pileup events per bunch-crossing class (early, in-time, late)
    int nEvents[3] = {}, nPrimaries[3] = {}, nTracks[3] = {};
    int bxMin = 0, bxMax = 0;
    std::map<int, int> eventsPerBx;
  };
  // one drawn page
  struct View {
    SimHitAnalyzer* a;
    double x0, x1, y0, y1;  // frame
    int nLabels = 0;
    std::vector<std::unique_ptr<TObject>> keep;
    std::map<std::string, TGraph*> groups;
    std::unique_ptr<TLegend> leg;
    std::vector<std::unique_ptr<TMarker>> legMarkers;
    View(SimHitAnalyzer* an, const char* title, const char* xt, const char* yt, double x0, double x1, double y0, double y1);
    void point(int color, int style, double size, double x, double y);
    void poly(int color, const std::vector<std::pair<double, double>>& xy);
    void line(int color, int width, double x0, double y0, double x1, double y1);
    void text(double x, double y, const std::string& s, double size, int color = kBlack, int align = 11);
    void label(double x, double y, const std::string& s, int color);
    void legEntry(int color, int style, const std::string& entry);
  };

  void analyze(const edm::Event& ev, const edm::EventSetup& es) override;
  void buildGeometry(const edm::EventSetup& es);
  void drawStation(Station& st, const edm::Event& ev, std::vector<Hit>& hits, const std::vector<Primary>& prims,
                   const math::XYZTLorentzVectorD& pv, const PileupSummary& pu, size_t nSignalHits);
  static void resolveAncestors(SimEvent& e);
  void finish(View& v, Station& st, const edm::Event& ev, const char* view, const std::vector<std::string>& header,
              const std::string& legendTitle);
  static int kindOf(int pdg, double charge);
  static int subOf(const DetId& id);
  static bool isBarrel(int sub, const DetId& id);
  static std::string pdgName(int pdg);
  static double cellSize(double e) {  // 6 size classes, 1 MeV ... 100 GeV
    int k = std::max(0, std::min(5, static_cast<int>(std::floor(std::log10(std::max(e, 1e-6)) + 3.))));
    return 0.35 + 0.33 * k;
  }

  bool crossingFrame_;
  edm::EDGetTokenT<edm::SimTrackContainer> simTracksTok_;
  edm::EDGetTokenT<edm::SimVertexContainer> simVerticesTok_;
  edm::EDGetTokenT<CrossingFrame<SimTrack>> simTracksCFTok_;
  edm::EDGetTokenT<CrossingFrame<SimVertex>> simVerticesCFTok_;
  edm::ESGetToken<GlobalTrackingGeometry, GlobalTrackingGeometryRecord> trackingGeomTok_;
  edm::ESGetToken<TrackerGeometry, TrackerDigiGeometryRecord> trackerGeomTok_;
  edm::ESGetToken<TrackerTopology, TrackerTopologyRcd> trackerTopoTok_;
  edm::ESGetToken<CaloGeometry, CaloGeometryRecord> caloGeomTok_;
  edm::ESGetToken<HcalDDDRecConstants, HcalRecNumberingRecord> hcalRecTok_;
  edm::ESGetToken<DTGeometry, MuonGeometryRecord> dtGeomTok_;
  edm::ESGetToken<CSCGeometry, MuonGeometryRecord> cscGeomTok_;
  edm::ESGetToken<RPCGeometry, MuonGeometryRecord> rpcGeomTok_;
  edm::ESGetToken<GEMGeometry, MuonGeometryRecord> gemGeomTok_;
  std::vector<Station> stations_;
  std::string outputDir_, prefix_, sampleLabel_;
  bool savePNG_, hcalTestNumbering_, printHits_;
  int highlight_, maxGeoPoints_;
  double highlightPtMin_;

  bool geometryBuilt_ = false;
  std::vector<GeoPt> geoPts_;
  std::vector<GeoPoly> geoPolys_;
  std::map<int, int> geoCount_;  // elements per subdetector (before subsampling)
  std::unique_ptr<TCanvas> canvas_;
};

SimHitAnalyzer::SimHitAnalyzer(const edm::ParameterSet& ps)
    : crossingFrame_(ps.getParameter<bool>("crossingFrame")),
      trackingGeomTok_(esConsumes()),
      trackerGeomTok_(esConsumes()),
      trackerTopoTok_(esConsumes()),
      caloGeomTok_(esConsumes()),
      hcalRecTok_(esConsumes()),
      dtGeomTok_(esConsumes()),
      cscGeomTok_(esConsumes()),
      rpcGeomTok_(esConsumes()),
      gemGeomTok_(esConsumes()),
      outputDir_(ps.getParameter<std::string>("outputDir")),
      prefix_(ps.getParameter<std::string>("prefix")),
      sampleLabel_(ps.getParameter<std::string>("sampleLabel")),
      savePNG_(ps.getParameter<bool>("savePNG")),
      hcalTestNumbering_(ps.getParameter<bool>("hcalTestNumbering")),
      printHits_(ps.getParameter<bool>("printHits")),
      highlight_(ps.getParameter<int>("highlight")),
      maxGeoPoints_(ps.getParameter<int>("maxGeoPoints")),
      highlightPtMin_(ps.getParameter<double>("highlightPtMin")) {
  if (crossingFrame_) {
    simTracksCFTok_ = consumes<CrossingFrame<SimTrack>>(ps.getParameter<edm::InputTag>("simTracks"));
    simVerticesCFTok_ = consumes<CrossingFrame<SimVertex>>(ps.getParameter<edm::InputTag>("simVertices"));
  } else {
    simTracksTok_ = consumes<edm::SimTrackContainer>(ps.getParameter<edm::InputTag>("simTracks"));
    simVerticesTok_ = consumes<edm::SimVertexContainer>(ps.getParameter<edm::InputTag>("simVertices"));
  }
  for (auto const& sps : ps.getParameter<std::vector<edm::ParameterSet>>("stations")) {
    Station st;
    st.name = sps.getParameter<std::string>("name");
    st.title = sps.getParameter<std::string>("title");
    for (auto const& t : sps.getParameter<std::vector<edm::InputTag>>("psimHits")) {
      if (crossingFrame_)
        st.psimCFTok.emplace_back(t, consumes<CrossingFrame<PSimHit>>(t));
      else
        st.psimTok.emplace_back(t, consumes<edm::PSimHitContainer>(t));
    }
    for (auto const& t : sps.getParameter<std::vector<edm::InputTag>>("caloHits")) {
      st.hasCalo = true;
      if (crossingFrame_)
        st.caloCFTok.emplace_back(t, consumes<CrossingFrame<PCaloHit>>(t));
      else
        st.caloTok.emplace_back(t, consumes<edm::PCaloHitContainer>(t));
    }
    st.geometry = sps.getParameter<std::vector<std::string>>("geometry");
    st.zMax = sps.getParameter<double>("zMax");
    st.rMax = sps.getParameter<double>("rMax");
    st.xyMax = sps.getParameter<double>("xyMax");
    st.etaMax = sps.getParameter<double>("etaMax");
    st.drawCaloEntry = sps.getParameter<bool>("drawCaloEntry");
    stations_.push_back(std::move(st));
  }
  gROOT->SetBatch(true);
  gStyle->SetOptStat(0);
  gStyle->SetPaperSize(29., 20.);  // A4 landscape width for the 1400 x 900 canvas (see GenPartAnalyzer)
  canvas_ = std::make_unique<TCanvas>("simHitCanvas", "", 1400, 900);
}

void SimHitAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription station;
  station.add<std::string>("name")->setComment("station name, used for the output directory and file names (e.g. TRK)");
  station.add<std::string>("title")->setComment("title printed on the pages");
  station.add<std::vector<edm::InputTag>>("psimHits", {})->setComment("PSimHit collections of the station (tracker, muon); crossingFrame: CrossingFrame<PSimHit>");
  station.add<std::vector<edm::InputTag>>("caloHits", {})->setComment("PCaloHit collections of the station; crossingFrame: CrossingFrame<PCaloHit>");
  station.add<std::vector<std::string>>("geometry", {})
      ->setComment("detector elements drawn in grey: tracker, EB, EE, ES, HB, HE, HO, HF, HGCEE, HGCHSi, HGCHSc, DT, CSC, RPC, GEM");
  station.add<double>("zMax", 1300.)->setComment("z range of the r-z view [cm]");
  station.add<double>("rMax", 800.)->setComment("r range of the r-z view [cm]");
  station.add<double>("xyMax", 800.)->setComment("x, y range of the transverse view [cm]; 0: no x-y page");
  station.add<double>("etaMax", 5.)->setComment("eta range of the eta-phi view");
  station.add<bool>("drawCaloEntry", false)->setComment("draw where every primary crossed the tracker / calorimeter boundary");
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("simTracks", edm::InputTag("g4SimHits"))->setComment("SimTracks (crossingFrame: the CrossingFrame<SimTrack>, mix:g4SimHits)");
  desc.add<edm::InputTag>("simVertices", edm::InputTag("g4SimHits"))->setComment("SimVertices (crossingFrame: the CrossingFrame<SimVertex>, mix:g4SimHits)");
  desc.add<bool>("crossingFrame", false)
      ->setComment("read the CrossingFrame collections of the MixingModule (signal + mixed pileup; only inside the mixing job) "
                   "instead of the SIM collections; all input tags are then CrossingFrame products, e.g. mix:g4SimHitsEcalHitsEB");
  desc.addVPSet("stations", station, {});
  desc.add<std::string>("outputDir", ".")->setComment("<outputDir>/<station>/ receives the PDF and PNGs of the station");
  desc.add<std::string>("prefix", "simHits");
  desc.add<std::string>("sampleLabel", "")->setComment("text printed in the right panel of every page, '|' separates lines");
  desc.add<bool>("savePNG", true)->setComment("also save every page as PNG");
  desc.add<bool>("hcalTestNumbering", true)->setComment("HCAL PCaloHit ids are in the simulation test numbering (relabelled to HcalDetId)");
  desc.add<bool>("printHits", false)->setComment("print every primary with its hit counts per station");
  desc.add<int>("highlight", 1)->setComment("leading primaries per kind whose hits are connected and labelled (0: none)");
  desc.add<double>("highlightPtMin", 2.)->setComment("minimum pT of a highlighted primary [GeV]");
  desc.add<int>("maxGeoPoints", 12000)->setComment("maximum number of drawn detector elements per subdetector (subsampled above)");
  descriptions.addWithDefaultLabel(desc);
}

int SimHitAnalyzer::kindOf(int pdg, double charge) {
  int a = std::abs(pdg);
  if (a == 22)
    return PHOTON;
  if (a == 11)
    return ELECTRON;
  if (a == 13)
    return MUON;
  if (a < 100)
    return OTHER_KIND;
  return charge != 0. ? CHARGED_HADRON : NEUTRAL_HADRON;
}

int SimHitAnalyzer::subOf(const DetId& id) {
  int s = id.subdetId();
  switch (id.det()) {
    case DetId::Tracker:
      return s >= 1 && s <= 6 ? static_cast<int>(PXB) + s - 1 : static_cast<int>(OTHER_SUB);
    case DetId::Muon:
      return s >= 1 && s <= 4 ? static_cast<int>(DT) + s - 1 : static_cast<int>(OTHER_SUB);
    case DetId::Ecal:
      return s == EcalBarrel ? EB : (s == EcalEndcap ? EE : (s == EcalPreshower ? ES : OTHER_SUB));
    case DetId::Hcal:
      return s >= 1 && s <= 4 ? static_cast<int>(HB) + s - 1 : static_cast<int>(OTHER_SUB);
    case DetId::HGCalEE:
      return HGCEE;
    case DetId::HGCalHSi:
      return HGCHSI;
    case DetId::HGCalHSc:
      return HGCHSC;
    default:
      return OTHER_SUB;
  }
}

bool SimHitAnalyzer::isBarrel(int sub, const DetId& id) {
  switch (sub) {
    case PXB:
    case TIB:
    case TOB:
    case DT:
    case EB:
    case HB:
    case HO:
      return true;
    case RPC:
      return RPCDetId(id).region() == 0;
    default:
      return false;
  }
}

std::string SimHitAnalyzer::pdgName(int pdg) {
  static const std::map<int, std::string> names = {
      {11, "e^{-}"},   {13, "#mu^{-}"},   {22, "#gamma"},     {111, "#pi^{0}"},     {211, "#pi^{+}"},    {130, "K_{L}^{0}"},
      {310, "K_{S}^{0}"}, {321, "K^{+}"}, {2212, "p"},        {2112, "n"},          {3122, "#Lambda"},   {3222, "#Sigma^{+}"},
      {3112, "#Sigma^{-}"}, {3322, "#Xi^{0}"}, {3312, "#Xi^{-}"}, {3334, "#Omega^{-}"}};
  auto it = names.find(std::abs(pdg));
  if (it == names.end())
    return Form("pdg %d", pdg);
  std::string n = it->second;
  if (pdg < 0) {
    size_t p = n.find("^{+}");
    if (p != std::string::npos)
      return n.replace(p, 4, "^{-}");
    p = n.find("^{-}");
    if (p != std::string::npos)
      return n.replace(p, 4, "^{+}");
    return "#bar{" + n + "}";
  }
  return n;
}

// ----------------------------------------------------------------------------- geometry (once per job)
void SimHitAnalyzer::buildGeometry(const edm::EventSetup& es) {
  geometryBuilt_ = true;
  auto add = [&](std::vector<GeoPt>& pts, int sub) {  // subsample a subdetector to maxGeoPoints_
    geoCount_[sub] += pts.size();
    size_t stride = std::max<size_t>(1, (pts.size() + maxGeoPoints_ - 1) / maxGeoPoints_);
    for (size_t i = 0; i < pts.size(); i += stride)
      geoPts_.push_back(pts[i]);
  };
  // tracker: module (sensor) centres
  try {
    const TrackerGeometry& tg = es.getData(trackerGeomTok_);
    std::map<int, std::vector<GeoPt>> bySub;
    for (auto const* det : tg.detUnits()) {
      DetId id(det->geographicalId());
      int sub = subOf(id);
      auto p = det->position();
      bySub[sub].push_back({float(p.x()), float(p.y()), float(p.z()), sub, isBarrel(sub, id)});
    }
    for (auto& kv : bySub)
      add(kv.second, kv.first);
  } catch (cms::Exception const& e) {
    std::cout << "SimHitAnalyzer: no TrackerGeometry (" << e.what() << ")\n";
  }
  // calorimeters: cell centres
  try {
    const CaloGeometry& cg = es.getData(caloGeomTok_);
    struct CaloSub {
      DetId::Detector det;
      int subdet, sub;
    };
    const CaloSub subs[] = {{DetId::Ecal, EcalBarrel, EB},        {DetId::Ecal, EcalEndcap, EE},        {DetId::Ecal, EcalPreshower, ES},
                            {DetId::Hcal, HcalBarrel, HB},        {DetId::Hcal, HcalEndcap, HE},        {DetId::Hcal, HcalOuter, HO},
                            {DetId::Hcal, HcalForward, HF},       {DetId::HGCalEE, ForwardEmpty, HGCEE}, {DetId::HGCalHSi, ForwardEmpty, HGCHSI},
                            {DetId::HGCalHSc, ForwardEmpty, HGCHSC}};
    for (auto const& cs : subs) {
      const CaloSubdetectorGeometry* sg = cg.getSubdetectorGeometry(cs.det, cs.subdet);
      if (!sg)
        continue;
      const std::vector<DetId>& ids = sg->getValidDetIds(cs.det, cs.subdet);
      if (ids.empty())
        continue;
      const HGCalGeometry* hg = cs.sub >= HGCEE ? static_cast<const HGCalGeometry*>(sg) : nullptr;
      geoCount_[cs.sub] += ids.size();
      size_t stride = std::max<size_t>(1, (ids.size() + maxGeoPoints_ - 1) / maxGeoPoints_);
      for (size_t i = 0; i < ids.size(); i += stride) {
        GlobalPoint p;
        if (hg) {
          p = hg->getPosition(ids[i]);
        } else {
          auto cell = sg->getGeometry(ids[i]);
          if (!cell.get())
            continue;
          p = cell->getPosition();
        }
        geoPts_.push_back({float(p.x()), float(p.y()), float(p.z()), cs.sub, isBarrel(cs.sub, ids[i])});
      }
    }
  } catch (cms::Exception const& e) {
    std::cout << "SimHitAnalyzer: no CaloGeometry (" << e.what() << ")\n";
  }
  // muon chambers: outlines (the four corners of the chamber surface)
  auto chamber = [&](const GeomDet* det, int sub) {
    const Bounds& b = det->surface().bounds();
    float w = b.width() / 2., l = b.length() / 2.;
    const float lx[4] = {-w, w, w, -w}, ly[4] = {-l, -l, l, l};
    GeoPoly poly;
    for (int k = 0; k < 4; ++k) {
      GlobalPoint p = det->surface().toGlobal(LocalPoint(lx[k], ly[k], 0.));
      poly.c[k] = {{float(p.x()), float(p.y()), float(p.z())}};
    }
    poly.sub = sub;
    poly.barrel = isBarrel(sub, det->geographicalId());
    geoPolys_.push_back(poly);
    ++geoCount_[sub];
  };
  try {
    for (auto const* ch : es.getData(dtGeomTok_).chambers())
      chamber(ch, DT);
  } catch (cms::Exception const& e) {
    std::cout << "SimHitAnalyzer: no DTGeometry (" << e.what() << ")\n";
  }
  try {
    for (auto const* ch : es.getData(cscGeomTok_).chambers())
      chamber(ch, CSC);
  } catch (cms::Exception const& e) {
    std::cout << "SimHitAnalyzer: no CSCGeometry (" << e.what() << ")\n";
  }
  try {
    for (auto const* ch : es.getData(rpcGeomTok_).chambers())
      chamber(ch, RPC);
  } catch (cms::Exception const& e) {
    std::cout << "SimHitAnalyzer: no RPCGeometry (" << e.what() << ")\n";
  }
  try {
    for (auto const* ch : es.getData(gemGeomTok_).chambers())
      chamber(ch, GEM);
  } catch (cms::Exception const& e) {
    std::cout << "SimHitAnalyzer: no GEMGeometry (" << e.what() << ")\n";
  }
  std::cout << "SimHitAnalyzer: detector elements per subdetector:";
  for (auto const& kv : geoCount_)
    std::cout << " " << kSubName[kv.first] << "=" << kv.second;
  std::cout << " (drawn: " << geoPts_.size() << " points, " << geoPolys_.size() << " chamber outlines)\n";
  // extents of the drawn elements and the layer / station numbers, for the documentation
  struct Ext {
    double rMin = 1e9, rMax = 0., zMin = 1e9, zMax = 0.;
    std::set<int> layers;
  };
  std::map<int, Ext> ext;
  auto grow = [&](int sub, double x, double y, double z) {
    Ext& e = ext[sub];
    double r = std::hypot(x, y);
    e.rMin = std::min(e.rMin, r);
    e.rMax = std::max(e.rMax, r);
    e.zMin = std::min(e.zMin, std::abs(z));
    e.zMax = std::max(e.zMax, std::abs(z));
  };
  for (auto const& g : geoPts_)
    grow(g.sub, g.x, g.y, g.z);
  for (auto const& g : geoPolys_)
    for (auto const& c : g.c)
      grow(g.sub, c[0], c[1], c[2]);
  try {
    const TrackerTopology& topo = es.getData(trackerTopoTok_);
    for (auto const* det : es.getData(trackerGeomTok_).detUnits())
      ext[subOf(det->geographicalId())].layers.insert(topo.layer(det->geographicalId()));
  } catch (cms::Exception const&) {
  }
  try {
    const CaloGeometry& cg = es.getData(caloGeomTok_);
    const std::pair<DetId::Detector, int> hg[3] = {{DetId::HGCalEE, HGCEE}, {DetId::HGCalHSi, HGCHSI}, {DetId::HGCalHSc, HGCHSC}};
    for (auto const& h : hg) {
      const CaloSubdetectorGeometry* sg = cg.getSubdetectorGeometry(h.first, ForwardEmpty);
      if (!sg)
        continue;
      for (auto const& id : sg->getValidDetIds(h.first, ForwardEmpty))
        ext[h.second].layers.insert(h.second == HGCHSC ? HGCScintillatorDetId(id).layer() : HGCSiliconDetId(id).layer());
    }
  } catch (cms::Exception const&) {
  }
  try {
    for (auto const* ch : es.getData(dtGeomTok_).chambers())
      ext[DT].layers.insert(DTChamberId(ch->id()).station());
    for (auto const* ch : es.getData(cscGeomTok_).chambers())
      ext[CSC].layers.insert(CSCDetId(ch->id()).station());
    for (auto const* ch : es.getData(rpcGeomTok_).chambers())
      ext[RPC].layers.insert(RPCDetId(ch->id()).station());
    for (auto const* ch : es.getData(gemGeomTok_).chambers())
      ext[GEM].layers.insert(GEMDetId(ch->id()).station());
  } catch (cms::Exception const&) {
  }
  std::cout << "SimHitAnalyzer: extent of the drawn elements (r, |z| in cm) and layers / stations per subdetector:\n";
  for (auto const& kv : ext) {
    const Ext& e = kv.second;
    std::cout << "    " << kSubName[kv.first] << ": r = " << e.rMin << " - " << e.rMax << ", |z| = " << e.zMin << " - " << e.zMax;
    if (!e.layers.empty()) {
      std::cout << ", " << e.layers.size() << " layers/stations:";
      for (int l : e.layers)
        std::cout << " " << l;
    }
    std::cout << "\n";
  }
}

// ----------------------------------------------------------------------------- event
// ancestor primary of every SimTrack of one simulated event through the vertex parent links
void SimHitAnalyzer::resolveAncestors(SimEvent& e) {
  const size_t nt = e.tracks.size();
  e.trackIdx.clear();
  e.trackIdx.reserve(nt);
  for (size_t i = 0; i < nt; ++i)
    e.trackIdx[e.tracks[i]->trackId()] = i;
  e.anc.assign(nt, -2);
  for (size_t i = 0; i < nt; ++i) {
    std::vector<int> path;
    int cur = i, result = -1;
    while (true) {
      if (e.anc[cur] != -2) {
        result = e.anc[cur];
        break;
      }
      path.push_back(cur);
      const SimTrack& t = *e.tracks[cur];
      if (t.isPrimary()) {
        result = cur;
        break;
      }
      int v = t.vertIndex();
      if (v < 0 || static_cast<size_t>(v) >= e.vertices.size() || e.vertices[v]->noParent())
        break;
      auto it = e.trackIdx.find(e.vertices[v]->parentIndex());
      if (it == e.trackIdx.end())
        break;
      cur = it->second;
      if (path.size() > nt)
        break;
    }
    for (int p : path)
      e.anc[p] = result;
  }
}

void SimHitAnalyzer::analyze(const edm::Event& ev, const edm::EventSetup& es) {
  if (!geometryBuilt_)
    buildGeometry(es);

  // the simulated events: the signal (evs[0]) and, in crossingFrame mode, every mixed pileup event, identified
  // by the EncodedEventId (bunch crossing, event number) the MixingModule stamps on its tracks, vertices and hits
  std::vector<SimEvent> evs(1);
  evs[0].signal = true;
  evs[0].bx = 0;
  PileupSummary pu;
  if (!crossingFrame_) {
    for (auto const& t : ev.get(simTracksTok_))
      evs[0].tracks.push_back(&t);
    for (auto const& v : ev.get(simVerticesTok_))
      evs[0].vertices.push_back(&v);
  } else {
    std::map<uint32_t, int> evIdx;  // pileup EncodedEventId -> index in evs
    auto eventOf = [&](const EncodedEventId& id, int bx) {
      auto ins = evIdx.try_emplace(id.rawId(), evs.size());
      if (ins.second) {
        SimEvent e;
        e.signal = false;
        e.bx = bx;
        e.id = id;
        evs.push_back(std::move(e));
      }
      return ins.first->second;
    };
    const CrossingFrame<SimTrack>& cfT = ev.get(simTracksCFTok_);
    MixCollection<SimTrack> colT(&cfT);
    for (auto it = colT.begin(); it != colT.end(); ++it) {
      const SimTrack& t = *it;
      if (it.getTrigger())
        evs[0].tracks.push_back(&t);
      else
        evs[eventOf(t.eventId(), it.bunch())].tracks.push_back(&t);
    }
    const CrossingFrame<SimVertex>& cfV = ev.get(simVerticesCFTok_);
    MixCollection<SimVertex> colV(&cfV);
    for (auto it = colV.begin(); it != colV.end(); ++it) {
      const SimVertex& v = *it;
      if (it.getTrigger())
        evs[0].vertices.push_back(&v);
      else
        evs[eventOf(v.eventId(), it.bunch())].vertices.push_back(&v);
    }
    pu.bxMin = cfT.getBunchRange().first;
    pu.bxMax = cfT.getBunchRange().second;
    for (size_t k = 1; k < evs.size(); ++k) {
      int c = evs[k].bx < 0 ? 0 : (evs[k].bx == 0 ? 1 : 2);
      ++pu.nEvents[c];
      ++pu.eventsPerBx[evs[k].bx];
      pu.nTracks[c] += evs[k].tracks.size();
      for (auto const* t : evs[k].tracks)
        if (t->isPrimary())
          ++pu.nPrimaries[c];
    }
  }
  SimEvent& sig = evs[0];
  resolveAncestors(sig);
  const size_t nt = sig.tracks.size();

  // the signal primaries: index in prims of every signal track's ancestor
  std::vector<Primary> prims;
  std::vector<int> trackToPrim(nt, -1), trackPrim(nt, -1);
  for (size_t i = 0; i < nt; ++i)
    if (sig.tracks[i]->isPrimary()) {
      const SimTrack& t = *sig.tracks[i];
      trackToPrim[i] = prims.size();
      prims.push_back({&t, kindOf(t.type(), t.charge()), t.momentum().pt(), t.momentum().eta(), t.momentum().phi(), pdgName(t.type())});
    }
  for (size_t i = 0; i < nt; ++i)
    trackPrim[i] = sig.anc[i] >= 0 ? trackToPrim[sig.anc[i]] : -1;
  auto primOfTrackId = [&](unsigned int id) {  // signal hits
    auto it = sig.trackIdx.find(id);
    return it == sig.trackIdx.end() ? -1 : trackPrim[it->second];
  };
  auto kindOfOwner = [&](int owner) {
    if (owner >= 0)
      return prims[owner].kind;
    if (owner == -1)
      return static_cast<int>(UNASSIGNED);
    return static_cast<int>(PU_EARLY) + (-owner - 2);
  };

  math::XYZTLorentzVectorD pv;
  for (size_t i = 0; i < nt; ++i)
    if (sig.tracks[i]->isPrimary() && sig.tracks[i]->vertIndex() >= 0 && static_cast<size_t>(sig.tracks[i]->vertIndex()) < sig.vertices.size()) {
      pv = sig.vertices[sig.tracks[i]->vertIndex()]->position();
      break;
    }
  auto dirEta = [&](double x, double y, double z) { return math::XYZVectorD(x - pv.x(), y - pv.y(), z - pv.z()).eta(); };
  auto dirPhi = [&](double x, double y, double z) { return math::XYZVectorD(x - pv.x(), y - pv.y(), z - pv.z()).phi(); };

  const GlobalTrackingGeometry* tgeom = nullptr;
  const TrackerTopology* topo = nullptr;
  const CaloGeometry* cgeom = nullptr;
  const HcalDDDRecConstants* hcons = nullptr;
  try {
    tgeom = &es.getData(trackingGeomTok_);
    topo = &es.getData(trackerTopoTok_);
  } catch (cms::Exception const& e) {
    std::cout << "SimHitAnalyzer: no tracking geometry (" << e.what() << ")\n";
  }
  try {
    cgeom = &es.getData(caloGeomTok_);
    if (hcalTestNumbering_)
      hcons = &es.getData(hcalRecTok_);
  } catch (cms::Exception const& e) {
    std::cout << "SimHitAnalyzer: no calo geometry (" << e.what() << ")\n";
  }

  std::cout << "SimHitAnalyzer run " << ev.id().run() << " event " << ev.id().event() << ": " << nt << " SimTracks, " << prims.size()
            << " primaries handed to Geant4, PV = (" << pv.x() << ", " << pv.y() << ", " << pv.z() << ") cm\n";
  if (crossingFrame_) {
    std::cout << "  pileup mixed in (CrossingFrame, bunch crossings " << pu.bxMin << " .. " << pu.bxMax << "): " << evs.size() - 1
              << " events (early " << pu.nEvents[0] << ", in-time " << pu.nEvents[1] << ", late " << pu.nEvents[2] << "), "
              << pu.nPrimaries[0] + pu.nPrimaries[1] + pu.nPrimaries[2] << " primaries (" << pu.nPrimaries[0] << ", " << pu.nPrimaries[1]
              << ", " << pu.nPrimaries[2] << "), " << pu.nTracks[0] + pu.nTracks[1] + pu.nTracks[2] << " SimTracks; events per bx:";
    for (auto const& kv : pu.eventsPerBx)
      std::cout << " " << kv.first << ":" << kv.second;
    std::cout << "\n";
    for (size_t k = 1; k < evs.size(); ++k) {
      int np = 0;
      const SimVertex* v0 = nullptr;
      for (auto const* t : evs[k].tracks)
        if (t->isPrimary()) {
          ++np;
          if (!v0 && t->vertIndex() >= 0 && static_cast<size_t>(t->vertIndex()) < evs[k].vertices.size())
            v0 = evs[k].vertices[t->vertIndex()];
        }
      std::cout << "    pileup event " << evs[k].id.event() << " in bx " << evs[k].bx << ": " << np << " primaries, " << evs[k].tracks.size()
                << " SimTracks, " << evs[k].vertices.size() << " SimVertices"
                << (v0 ? Form(", vertex z = %.2f cm, t = %.1f ns", v0->position().z(), std::abs(v0->position().t()) < 0.05 ? 0. : v0->position().t()) : "")
                << "\n";
    }
  }

  for (Station& st : stations_) {
    std::vector<Hit> hits;
    int nNoGeom = 0;
    size_t nSignalPSim = 0;
    auto addPSimHit = [&](const PSimHit& hit, int owner) {
      DetId id(hit.detUnitId());
      const GeomDet* det = nullptr;
      if (tgeom) {
        try {
          det = tgeom->idToDet(id);
        } catch (cms::Exception const&) {
        }
      }
      if (!det) {
        ++nNoGeom;
        return;
      }
      GlobalPoint gp = det->surface().toGlobal(hit.localPosition());
      Hit x;
      x.x = gp.x();
      x.y = gp.y();
      x.z = gp.z();
      x.r = gp.perp();
      x.eta = dirEta(gp.x(), gp.y(), gp.z());
      x.phi = dirPhi(gp.x(), gp.y(), gp.z());
      x.tof = hit.timeOfFlight();
      x.e = hit.energyLoss();
      x.sub = subOf(id);
      x.layer = 0;
      if (id.det() == DetId::Tracker && topo)
        x.layer = topo->layer(id);
      else if (x.sub == DT)
        x.layer = DTChamberId(id).station();
      else if (x.sub == CSC)
        x.layer = CSCDetId(id).station();
      else if (x.sub == RPC)
        x.layer = RPCDetId(id).station();
      else if (x.sub == GEM)
        x.layer = GEMDetId(id).station();
      x.prim = owner >= 0 ? owner : -1;
      x.kind = kindOfOwner(owner);
      x.calo = false;
      x.barrel = isBarrel(x.sub, id);
      if (owner >= -1)
        ++nSignalPSim;
      hits.push_back(x);
    };
    if (!crossingFrame_) {
      for (auto const& th : st.psimTok) {
        edm::Handle<edm::PSimHitContainer> h;
        ev.getByToken(th.second, h);
        if (!h.isValid())
          continue;
        for (auto const& hit : *h)
          addPSimHit(hit, primOfTrackId(hit.trackId()));
      }
    } else {
      for (auto const& th : st.psimCFTok) {
        edm::Handle<CrossingFrame<PSimHit>> h;
        ev.getByToken(th.second, h);
        if (!h.isValid())
          continue;
        MixCollection<PSimHit> col(h.product());
        for (auto it = col.begin(); it != col.end(); ++it)
          addPSimHit(*it, it.getTrigger() ? primOfTrackId((*it).trackId()) : pileupOwner(it.bunch()));
      }
    }
    // calorimeter cells: energy summed per cell, kind of the owner (signal primary or pileup class) with the largest share
    struct Cell {
      Hit h;
      std::map<int, double> eOwner;
    };
    std::map<uint32_t, Cell> cells;
    size_t nCaloHits = 0, nSignalCaloHits = 0;
    auto addCaloHit = [&](const PCaloHit& hit, int owner, bool hcal) {
      ++nCaloHits;
      if (owner >= -1)
        ++nSignalCaloHits;
      DetId id(hit.id());
      if (hcal && hcalTestNumbering_ && hcons)
        id = HcalHitRelabeller::relabel(hit.id(), hcons);
      if (id.rawId() == 0) {
        ++nNoGeom;
        return;
      }
      auto it = cells.find(id.rawId());
      if (it == cells.end()) {
        const CaloSubdetectorGeometry* sg = cgeom ? cgeom->getSubdetectorGeometry(id) : nullptr;
        auto cell = sg ? sg->getGeometry(id) : CaloSubdetectorGeometry::CellMayOwnPtr();
        if (!cell.get()) {
          ++nNoGeom;
          return;
        }
        const GlobalPoint& gp = cell->getPosition();
        Cell c;
        c.h.x = gp.x();
        c.h.y = gp.y();
        c.h.z = gp.z();
        c.h.r = gp.perp();
        c.h.eta = dirEta(gp.x(), gp.y(), gp.z());
        c.h.phi = dirPhi(gp.x(), gp.y(), gp.z());
        c.h.tof = 0.;
        c.h.e = 0.;
        c.h.sub = subOf(id);
        c.h.layer = c.h.sub == HGCEE || c.h.sub == HGCHSI ? HGCSiliconDetId(id).layer() : (c.h.sub == HGCHSC ? HGCScintillatorDetId(id).layer() : 0);
        c.h.prim = -1;
        c.h.kind = UNASSIGNED;
        c.h.calo = true;
        c.h.barrel = isBarrel(c.h.sub, id);
        it = cells.emplace(id.rawId(), c).first;
      }
      it->second.h.e += hit.energy();
      it->second.eOwner[owner] += hit.energy();
    };
    if (!crossingFrame_) {
      for (auto const& th : st.caloTok) {
        edm::Handle<edm::PCaloHitContainer> h;
        ev.getByToken(th.second, h);
        if (!h.isValid())
          continue;
        bool hcal = th.first.instance().find("Hcal") != std::string::npos;
        for (auto const& hit : *h)
          addCaloHit(hit, primOfTrackId(hit.geantTrackId()), hcal);
      }
    } else {
      for (auto const& th : st.caloCFTok) {
        edm::Handle<CrossingFrame<PCaloHit>> h;
        ev.getByToken(th.second, h);
        if (!h.isValid())
          continue;
        bool hcal = th.first.instance().find("Hcal") != std::string::npos;
        MixCollection<PCaloHit> col(h.product());
        for (auto it = col.begin(); it != col.end(); ++it)
          addCaloHit(*it, it.getTrigger() ? primOfTrackId((*it).geantTrackId()) : pileupOwner(it.bunch()), hcal);
      }
    }
    size_t nPSim = hits.size();
    size_t nSignalCells = 0;
    for (auto& kv : cells) {
      Cell& c = kv.second;
      double best = -1.;
      int owner = -1;
      for (auto const& pe : c.eOwner)
        if (pe.second > best) {
          best = pe.second;
          owner = pe.first;
        }
      c.h.prim = owner >= 0 ? owner : -1;
      c.h.kind = kindOfOwner(owner);
      if (owner >= -1)
        ++nSignalCells;
      hits.push_back(c.h);
    }
    std::cout << "  station " << st.name << ": " << nPSim << " PSimHits, " << nCaloHits << " PCaloHits in " << cells.size() << " cells"
              << (nNoGeom ? Form(", %d hits without geometry", nNoGeom) : "");
    if (crossingFrame_) {
      std::cout << " (signal: " << nSignalPSim << " PSimHits, " << nSignalCaloHits << " PCaloHits";
      if (!cells.empty())
        std::cout << "; " << nSignalCells << " cells dominated by the signal, " << cells.size() - nSignalCells << " by pileup";
      std::cout << ")";
    }
    std::cout << "\n";
    drawStation(st, ev, hits, prims, pv, pu, nSignalPSim + nSignalCells);
  }
  std::cout.flush();
}

// ----------------------------------------------------------------------------- the three views of a station
void SimHitAnalyzer::drawStation(Station& st, const edm::Event& ev, std::vector<Hit>& hits, const std::vector<Primary>& prims,
                                 const math::XYZTLorentzVectorD& pv, const PileupSummary& pu, size_t nSignalHits) {
  // pileup hits first, so that they are drawn underneath the signal hits (a TGraph per colour, in creation order)
  std::stable_partition(hits.begin(), hits.end(), [](const Hit& h) { return isPileup(h.kind); });
  // counts per subdetector / layer / kind, hits per primary
  std::map<std::pair<int, int>, int> nSubLayer;
  std::map<int, double> eSub;
  int nKind[NKIND] = {}, nSub[NSUB] = {};
  std::map<int, std::vector<int>> hitsOfPrim;
  double eKind[NKIND] = {};
  for (size_t i = 0; i < hits.size(); ++i) {
    const Hit& h = hits[i];
    ++nKind[h.kind];
    ++nSub[h.sub];
    ++nSubLayer[{h.sub, h.layer}];
    if (h.calo) {
      eSub[h.sub] += h.e;
      eKind[h.kind] += h.e;
    }
    if (h.prim >= 0)
      hitsOfPrim[h.prim].push_back(i);
  }
  std::cout << "    hits by subdetector and layer/station:";
  for (auto const& kv : nSubLayer)
    std::cout << " " << kSubName[kv.first.first]
              << (kv.first.second || (kv.first.first >= DT && kv.first.first <= GEM) ? Form("/%d", kv.first.second) : "") << "=" << kv.second;
  std::cout << "\n    hits by kind of the primary:";
  for (int k = 0; k < NKIND; ++k)
    if (nKind[k])
      std::cout << " " << kKindName[k] << "=" << nKind[k] << (eKind[k] > 0. ? Form("(%.3g GeV)", eKind[k]) : "");
  std::cout << "\n    primaries with hits: " << hitsOfPrim.size() << " of " << prims.size() << "\n";
  if (printHits_)
    for (auto const& kv : hitsOfPrim) {
      const Primary& p = prims[kv.first];
      int nh = 0;
      double e = 0.;
      for (int i : kv.second)
        hits[i].calo ? (e += hits[i].e, 0) : ++nh;
      std::cout << "      " << p.name << " pdg " << p.trk->type() << " pT=" << p.pt << " eta=" << p.eta << " phi=" << p.phi
                << " PSimHits=" << nh << " cells=" << kv.second.size() - nh << " E=" << e << "\n";
    }

  // highlighted primaries: the leading ones per kind among those with hits in the station
  std::vector<int> highlighted;
  for (int kind = 0; kind < UNASSIGNED && highlight_ > 0; ++kind) {
    std::vector<int> cand;
    for (auto const& kv : hitsOfPrim)
      if (prims[kv.first].kind == kind && prims[kv.first].pt >= highlightPtMin_)
        cand.push_back(kv.first);
    std::sort(cand.begin(), cand.end(), [&](int a, int b) { return prims[a].pt > prims[b].pt; });
    for (size_t i = 0; i < cand.size() && static_cast<int>(i) < highlight_; ++i)
      highlighted.push_back(cand[i]);
  }
  auto highlightLine = [&](int p) {
    int nh = 0, nc = 0, nhf = 0;
    double e = 0., ehf = 0.;
    for (int i : hitsOfPrim[p]) {
      const Hit& h = hits[i];
      if (!h.calo)
        ++nh;
      else if (h.sub == HF) {
        ++nhf;
        ehf += h.e;
      } else {
        ++nc;
        e += h.e;
      }
    }
    std::string s = Form("%s %.1f GeV (#eta %.2f)", prims[p].name.c_str(), prims[p].pt, prims[p].eta);
    if (nh)
      s += Form(", %d hits", nh);
    if (nc)
      s += Form(", %.2f GeV in %d cells", e, nc);
    if (nhf)
      s += Form(", %.0f (HF units) in %d HF cells", ehf, nhf);
    return s;
  };
  std::vector<std::string> header;
  {
    // the hit counts per subdetector, wrapped into lines of at most ~150 characters
    std::vector<std::string> parts;
    for (int sub = 0; sub < OTHER_SUB; ++sub)
      if (nSub[sub])
        parts.push_back(Form("%s %d%s", kSubName[sub], nSub[sub],
                             eSub.count(sub) ? (sub == HF ? Form(" cells (%.3g, shower-library units)", eSub[sub]) : Form(" cells (%.3g GeV)", eSub[sub])) : ""));
    if (nSub[OTHER_SUB])
      parts.push_back(Form("other %d", nSub[OTHER_SUB]));
    std::string line = Form("%zu hits: ", hits.size());
    for (size_t i = 0; i < parts.size(); ++i) {
      if (line.size() + parts[i].size() > 150) {
        header.push_back(line);
        line = "    ";
      }
      line += parts[i] + (i + 1 < parts.size() ? ", " : "");
    }
    header.push_back(line);
    if (crossingFrame_) {
      int nPU = 0;
      for (int k = 0; k < NKIND; ++k)
        if (isPileup(k))
          nPU += nKind[k];
      std::string perBx;
      for (auto const& kv : pu.eventsPerBx)
        perBx += Form("%s%+d: %d", perBx.empty() ? "" : ", ", kv.first, kv.second);
      header.push_back(Form("signal + pileup (MixingModule): %zu hits of the signal event + %d hits of the %d mixed pileup events (%d pileup primaries), drawn underneath by bunch crossing",
                            nSignalHits, nPU, pu.nEvents[0] + pu.nEvents[1] + pu.nEvents[2], pu.nPrimaries[0] + pu.nPrimaries[1] + pu.nPrimaries[2]));
      header.push_back(Form("pileup events per bunch crossing (Poisson): %s; in-time = bx 0, the hits of bx < 0 / bx > 0 arrive 25 ns x |bx| before / after the signal",
                            perBx.c_str()));
    }
    header.push_back(Form("%zu of the %zu %sprimaries handed to Geant4 left hits here; colour = kind of the primary the hit descends from (ancestor through the SimVertex parents)",
                          hitsOfPrim.size(), prims.size(), crossingFrame_ ? "signal " : ""));
    std::string hl;
    for (int p : highlighted)
      hl += (hl.empty() ? "" : ";  ") + highlightLine(p);
    if (!hl.empty())
      header.push_back("followed (lines in time order, cells circled): " + hl);
  }
  // which geometry groups
  auto drawGeo = [&](int sub) {
    for (auto const& g : st.geometry)
      if (g == kGeoKey[sub])
        return true;
    return false;
  };
  std::string geoText;
  for (auto const& g : st.geometry)
    geoText += (geoText.empty() ? "" : ", ") + g;
  std::string legendTitle = "hits by primary kind";
  auto legend = [&](View& v, int nShown[NKIND], bool withCells) {
    for (int k = 0; k < NKIND; ++k)
      if (nShown[k])
        v.legEntry(kKindColor[k], kKindMarker[k], Form("%s%s (%d)", kKindName[k], crossingFrame_ && !isPileup(k) ? ", signal" : "", nShown[k]));
    v.legEntry(kGray + 1, 20, "detector elements");
    if (!highlighted.empty())
      v.legEntry(kBlack, 24, "followed primary (cells)");
    if (st.drawCaloEntry)
      v.legEntry(kGray + 3, 2, "primary at calo boundary");
    (void)withCells;
  };
  auto labelText = [&](int p) { return Form("%s %.0f GeV", prims[p].name.c_str(), prims[p].pt); };

  // ---- r-z
  {
    View v(this, Form("%s hits by primary kind", st.name.c_str()), "z [cm]", "r [cm]", -st.zMax, st.zMax, 0., st.rMax);
    for (auto const& g : geoPts_)
      if (drawGeo(g.sub))
        v.point(kGray + 1, 20, 0.3, g.z, std::hypot(g.x, g.y));
    for (auto const& g : geoPolys_)
      if (drawGeo(g.sub)) {
        std::vector<std::pair<double, double>> xy;
        for (auto const& c : g.c)
          xy.emplace_back(c[2], std::hypot(c[0], c[1]));
        v.poly(kGray + 1, xy);
      }
    if (st.drawCaloEntry)
      for (auto const& p : prims)
        if (p.trk->crossedBoundary()) {
          auto const& b = p.trk->getPositionAtBoundary();
          v.point(kGray + 3, 2, 0.8, b.z(), std::hypot(b.x(), b.y()));
        }
    int nShown[NKIND] = {};
    for (auto const& h : hits) {
      ++nShown[h.kind];
      v.point(kKindColor[h.kind], kKindMarker[h.kind], h.calo ? cellSize(h.e) : (isPileup(h.kind) ? 0.35 : 0.45), h.z, h.r);
    }
    for (int p : highlighted) {
      std::vector<int> ph;
      for (int i : hitsOfPrim[p])
        if (!hits[i].calo)
          ph.push_back(i);
      std::sort(ph.begin(), ph.end(), [&](int a, int b) { return hits[a].tof < hits[b].tof; });
      int color = kKindColor[prims[p].kind];
      for (size_t k = 1; k < ph.size(); ++k)
        v.line(color, 2, hits[ph[k - 1]].z, hits[ph[k - 1]].r, hits[ph[k]].z, hits[ph[k]].r);
      double lx = 0., ly = -1.;
      for (int i : hitsOfPrim[p]) {
        const Hit& h = hits[i];
        if (h.calo)
          v.point(color, 24, cellSize(h.e) + 0.4, h.z, h.r);
        if (h.r > ly) {
          ly = h.r;
          lx = h.z;
        }
      }
      if (ly >= 0.)
        v.label(lx, ly, labelText(p), color);
    }
    legend(v, nShown, true);
    finish(v, st, ev, "rz", header, legendTitle);
  }
  // ---- x-y (barrel)
  if (st.xyMax > 0.) {
    View v(this, Form("%s hits by primary kind (barrel)", st.name.c_str()), "x [cm]", "y [cm]", -st.xyMax, st.xyMax, -st.xyMax, st.xyMax);
    for (auto const& g : geoPts_)
      if (g.barrel && drawGeo(g.sub))
        v.point(kGray + 1, 20, 0.3, g.x, g.y);
    for (auto const& g : geoPolys_)
      if (g.barrel && drawGeo(g.sub)) {
        std::vector<std::pair<double, double>> xy;
        for (auto const& c : g.c)
          xy.emplace_back(c[0], c[1]);
        v.poly(kGray + 1, xy);
      }
    if (st.drawCaloEntry)
      for (auto const& p : prims)
        if (p.trk->crossedBoundary() && std::abs(p.trk->getPositionAtBoundary().z()) < st.zMax) {
          auto const& b = p.trk->getPositionAtBoundary();
          if (std::hypot(b.x(), b.y()) > 0.9 * std::abs(b.z()) / 3.)  // only the barrel part of the boundary (roughly r > z/3)
            v.point(kGray + 3, 2, 0.8, b.x(), b.y());
        }
    int nShown[NKIND] = {};
    int nBarrel = 0;
    for (auto const& h : hits)
      if (h.barrel) {
        ++nBarrel;
        ++nShown[h.kind];
        v.point(kKindColor[h.kind], kKindMarker[h.kind], h.calo ? cellSize(h.e) : (isPileup(h.kind) ? 0.35 : 0.45), h.x, h.y);
      }
    for (int p : highlighted) {
      std::vector<int> ph;
      for (int i : hitsOfPrim[p])
        if (!hits[i].calo && hits[i].barrel)
          ph.push_back(i);
      std::sort(ph.begin(), ph.end(), [&](int a, int b) { return hits[a].tof < hits[b].tof; });
      int color = kKindColor[prims[p].kind];
      for (size_t k = 1; k < ph.size(); ++k)
        v.line(color, 2, hits[ph[k - 1]].x, hits[ph[k - 1]].y, hits[ph[k]].x, hits[ph[k]].y);
      double lx = 0., ly = 0., lr = -1.;
      for (int i : hitsOfPrim[p]) {
        const Hit& h = hits[i];
        if (!h.barrel)
          continue;
        if (h.calo)
          v.point(color, 24, cellSize(h.e) + 0.4, h.x, h.y);
        if (h.r > lr) {
          lr = h.r;
          lx = h.x;
          ly = h.y;
        }
      }
      if (lr >= 0.)
        v.label(lx, ly, labelText(p), color);
    }
    std::vector<std::string> hdr = header;
    for (size_t k = 0; k < hdr.size(); ++k)
      if (hdr[k].find("primaries handed to Geant4") != std::string::npos) {
        hdr.insert(hdr.begin() + k, Form("%d of the %zu hits are in the barrel elements of the station (shown here); the endcaps are in the r-z and #eta-#phi views",
                                         nBarrel, hits.size()));
        break;
      }
    legend(v, nShown, true);
    finish(v, st, ev, "xy", hdr, legendTitle);
  }
  // ---- eta-phi
  {
    View v(this, Form("%s hits by primary kind", st.name.c_str()), "#eta", "#phi", -st.etaMax, st.etaMax, -M_PI, M_PI);
    if (st.drawCaloEntry)
      for (auto const& p : prims)
        if (p.trk->crossedBoundary()) {
          auto const& b = p.trk->getPositionAtBoundary();
          math::XYZVectorD d(b.x() - pv.x(), b.y() - pv.y(), b.z() - pv.z());
          v.point(kGray + 3, 2, 0.8, d.eta(), d.phi());
        }
    int nShown[NKIND] = {};
    int nOut = 0;
    for (auto const& h : hits) {
      if (std::abs(h.eta) > st.etaMax) {
        ++nOut;
        continue;
      }
      ++nShown[h.kind];
      v.point(kKindColor[h.kind], kKindMarker[h.kind], h.calo ? cellSize(h.e) : (isPileup(h.kind) ? 0.35 : 0.45), h.eta, h.phi);
    }
    for (int p : highlighted) {
      int color = kKindColor[prims[p].kind];
      double se = 0., sp = 0., sw = 0.;
      for (int i : hitsOfPrim[p]) {
        const Hit& h = hits[i];
        if (std::abs(h.eta) > st.etaMax)
          continue;
        if (h.calo)
          v.point(color, 24, cellSize(h.e) + 0.4, h.eta, h.phi);
        double w = h.calo ? std::max(double(h.e), 1e-3) : 1.;
        se += w * h.eta;
        sp += w * reco::deltaPhi(h.phi, prims[p].phi);
        sw += w;
      }
      if (sw > 0.)
        v.label(se / sw, reco::deltaPhi(prims[p].phi + sp / sw, 0.), labelText(p), color);
    }
    std::vector<std::string> hdr = header;
    for (auto& l : hdr)
      if (l.find("primaries handed to Geant4") != std::string::npos)
        l = Form("#eta, #phi of the hit position seen from the primary vertex (%.2f, %.2f, %.2f) cm%s; colour = kind of the ancestor primary",
                 pv.x(), pv.y(), pv.z(), nOut ? Form("; %d hits beyond |#eta| = %.1f not shown", nOut, st.etaMax) : "");
    legend(v, nShown, true);
    finish(v, st, ev, "etaphi", hdr, legendTitle);
  }
  (void)geoText;
}

// ----------------------------------------------------------------------------- drawing
SimHitAnalyzer::View::View(SimHitAnalyzer* an, const char* title, const char* xt, const char* yt, double x0, double x1, double y0, double y1)
    : a(an), x0(x0), x1(x1), y0(y0), y1(y1) {
  TCanvas* c = a->canvas_.get();
  c->Clear();
  c->SetLeftMargin(0.07);
  c->SetRightMargin(0.21);
  c->SetTopMargin(0.17);
  c->SetBottomMargin(0.09);
  c->SetGrid();
  auto frame = std::make_unique<TH2D>("frame", Form(";%s;%s", xt, yt), 10, x0, x1, 10, y0, y1);
  frame->GetXaxis()->SetTitleOffset(1.0);
  frame->GetYaxis()->SetTitleOffset(0.8);
  frame->Draw("axis");
  keep.push_back(std::move(frame));
  leg = std::make_unique<TLegend>(0.80, 0.42, 0.995, 0.80);
  leg->SetBorderSize(0);
  leg->SetTextSize(0.018);
  leg->SetHeader(title);
}

// labels of the followed primaries: stacked upwards so that they do not overlap
void SimHitAnalyzer::View::label(double x, double y, const std::string& s, int color) {
  y += 0.035 * (y1 - y0) * (nLabels++);
  y = std::min(y, y1 - 0.02 * (y1 - y0));
  auto t = std::make_unique<TLatex>(x, y, ("  " + s).c_str());
  t->SetTextSize(0.018);
  t->SetTextColor(color);
  t->SetTextAlign(12);
  t->Draw();
  keep.push_back(std::move(t));
}

void SimHitAnalyzer::View::point(int color, int style, double size, double x, double y) {
  std::string key = Form("%d_%d_%.2f", color, style, size);
  auto it = groups.find(key);
  if (it == groups.end()) {
    auto g = std::make_unique<TGraph>();
    g->SetMarkerColor(color);
    g->SetMarkerStyle(style);
    g->SetMarkerSize(size);
    g->Draw("P same");
    it = groups.emplace(key, g.get()).first;
    keep.push_back(std::move(g));
  }
  it->second->SetPoint(it->second->GetN(), x, y);
}

void SimHitAnalyzer::View::poly(int color, const std::vector<std::pair<double, double>>& xy) {
  std::vector<double> xs, ys;
  for (auto const& p : xy) {
    xs.push_back(p.first);
    ys.push_back(p.second);
  }
  xs.push_back(xy.front().first);
  ys.push_back(xy.front().second);
  auto pl = std::make_unique<TPolyLine>(xs.size(), xs.data(), ys.data());
  pl->SetLineColor(color);
  pl->SetLineWidth(1);
  pl->Draw("L same");
  keep.push_back(std::move(pl));
}

void SimHitAnalyzer::View::line(int color, int width, double x0, double y0, double x1, double y1) {
  auto l = std::make_unique<TLine>(x0, y0, x1, y1);
  l->SetLineColor(color);
  l->SetLineWidth(width);
  l->Draw();
  keep.push_back(std::move(l));
}

void SimHitAnalyzer::View::text(double x, double y, const std::string& s, double size, int color, int align) {
  auto t = std::make_unique<TLatex>(x, y, s.c_str());
  t->SetNDC();
  t->SetTextSize(size);
  t->SetTextColor(color);
  t->SetTextAlign(align);
  t->Draw();
  keep.push_back(std::move(t));
}

void SimHitAnalyzer::View::legEntry(int color, int style, const std::string& entry) {
  auto m = std::make_unique<TMarker>(0, 0, style);
  m->SetMarkerColor(color);
  m->SetMarkerSize(1.4);
  leg->AddEntry(m.get(), entry.c_str(), "p");
  legMarkers.push_back(std::move(m));
}

void SimHitAnalyzer::finish(View& v, Station& st, const edm::Event& ev, const char* view, const std::vector<std::string>& header,
                            const std::string& legendTitle) {
  TCanvas* c = canvas_.get();
  std::string title = Form("%s hits at %s (%s)", crossingFrame_ ? "SIM + pileup" : "SIM", st.name.c_str(), st.title.c_str());
  v.text(0.07, 0.957, title, title.size() > 48 ? 0.024 : 0.03);  // long station titles must not run into the run / event text
  v.text(0.79, 0.955, Form("run %u  lumi %u  event %llu", ev.id().run(), ev.luminosityBlock(), ev.id().event()), 0.022, kBlack, 31);
  if (!sampleLabel_.empty()) {
    size_t start = 0;
    for (int k = 0; start <= sampleLabel_.size() && k < 5; ++k) {
      size_t bar = sampleLabel_.find('|', start);
      std::string line = sampleLabel_.substr(start, bar == std::string::npos ? std::string::npos : bar - start);
      v.text(0.80, 0.925 - 0.026 * k, line, 0.018, k == 0 ? kBlack : kGray + 3);
      if (bar == std::string::npos)
        break;
      start = bar + 1;
    }
  }
  for (size_t k = 0; k < header.size() && k < 6; ++k)
    v.text(0.07, 0.930 - 0.019 * k, header[k], k == 0 ? 0.018 : 0.015, kGray + 3);
  v.leg->Draw();
  if (st.hasCalo) {
    v.text(0.80, 0.39, "marker size of calorimeter cells:", 0.017);
    const double scale[3] = {0.01, 1., 100.};
    for (int k = 0; k < 3; ++k) {
      auto m = std::make_unique<TMarker>(0.815, 0.355 - 0.033 * k, 20);
      m->SetNDC();
      m->SetMarkerColor(kGray + 2);
      m->SetMarkerSize(cellSize(scale[k]));
      m->Draw();
      v.keep.push_back(std::move(m));
      v.text(0.835, 0.35 - 0.033 * k, Form("E = %g GeV", scale[k]), 0.018);
    }
  }
  std::string geo;
  for (auto const& g : st.geometry)
    geo += (geo.empty() ? "" : ", ") + g;
  v.text(0.80, 0.235, "detector elements drawn (reco geometry):", 0.017);
  v.text(0.81, 0.21, geo.empty() ? "none" : geo, 0.016, kGray + 2);
  v.text(0.80, 0.16, "views of this station:", 0.017);
  const char* views[3] = {"rz", "xy", "etaphi"};
  const char* viewNames[3] = {"r-z (longitudinal)", "x-y (barrel, transverse)", "#eta-#phi (from the primary vertex)"};
  for (int k = 0; k < 3; ++k)
    v.text(0.81, 0.135 - 0.028 * k, viewNames[k], 0.017, std::string(views[k]) == view ? kRed : kGray + 2);
  (void)legendTitle;
  c->Update();
  std::filesystem::path dir = std::filesystem::path(outputDir_) / st.name;
  std::filesystem::create_directories(dir);
  std::string pdf = (dir / Form("%s_%s_run%u_event%llu.pdf", prefix_.c_str(), st.name.c_str(), ev.id().run(), ev.id().event())).string();
  // the three views of a station go into one PDF: opened with the first view ("rz"), closed with the last ("etaphi")
  if (std::string(view) == "rz")
    c->Print((pdf + "[").c_str());
  c->Print(pdf.c_str());
  if (std::string(view) == "etaphi")
    c->Print((pdf + "]").c_str());
  if (savePNG_)
    c->SaveAs((dir / Form("%s_%s_%s_run%u_event%llu.png", prefix_.c_str(), st.name.c_str(), view, ev.id().run(), ev.id().event())).string().c_str());
}

DEFINE_FWK_MODULE(SimHitAnalyzer);
