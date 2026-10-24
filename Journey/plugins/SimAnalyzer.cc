// SimAnalyzer: eta-phi maps of the Geant4 record of the SIM step, five pages per event that follow
// the generator final state through the detector simulation:
//   1 primaries     the status 1 genParticles handed to Geant4, i.e. the SimTracks with a gen link
//                   (SimTrack::genpartIndex = HepMC barcode), at their production. The status 1
//                   particles without SimTrack were cut by the Generator (|eta| > 5.5, p < 40 MeV,
//                   produced outside the decay volume) or not tracked (neutrinos).
//   2 calo surface  the same primaries where they cross the tracker / calorimeter boundary
//                   (SimTrack::trackerSurfacePosition / Momentum, filled by CaloTrkProcessing when a
//                   track enters a calorimeter), with a line from the production direction that
//                   shows the bend in the magnetic field; primaries that never reach the boundary
//                   (decayed, interacted or stopped in the tracker, looping, along the beam line) are
//                   open markers at their production direction.
//   3 secondaries   the SimTracks created by Geant4, at their production vertex, coloured by the
//                   creator process (SimVertex::processType = Geant4 process sub-type): decay, the
//                   electromagnetic processes (ionisation delta rays, bremsstrahlung, pair production,
//                   Compton, photoelectric, annihilation) and the hadronic ones (elastic, inelastic,
//                   capture, at rest). Only the tracks kept by the StackingAction and the sensitive
//                   detectors are SimTracks: decay products and conversions of primaries, tracks with
//                   tracker / muon hits above the persistency thresholds, tracks entering a calorimeter.
//   4 tracker/muon  every PSimHit of the tracker and muon chambers at its global position (geometry
//                   from the EventSetup), coloured by subdetector
//   5 calorimeter   the PCaloHits summed per cell (EB, EE, ES, HB, HE, HO, HF, and for Phase2 the HGCal
//                   cells HGC EE, HGC HSi, HGC HSc; HCAL hit ids are relabelled from the test numbering
//                   of the simulation with HcalHitRelabeller), marker size growing with the cell energy
// The b and bbar of the hard process are followed as in GenPartAnalyzer: the stable decay products
// of the primary b hadron matched to each quark (red / blue) are the primaries of the b / bbar
// system; the secondaries and hits are assigned to the system of their ancestor primary (SimVertex
// parent links for tracks, trackId / geantTrackId for hits). Every page prints the vector sums of
// the system and the net b + bbar, and the same numbers go to std::cout.
// Marker size grows with pT (pages 1-3) or the cell energy (page 5); the pT value is written next to
// the b / bbar system particles and every particle above labelPtMin. One PDF per page and event
// (<outputPrefix>_pageN_<name>_run<R>_event<E>.pdf) or, with pdfFile, all pages in one PDF
// (openPdf / closePdf as in GenPartAnalyzer, so that GenPartAnalyzer can print its generator-stage
// pages first). simStage (1-5) is the SIM stage of the sample (Transport, + B field, + decays, + EM,
// + hadronic physics), highlighted in the right panel; sampleLabel is printed above the legend.
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "TCanvas.h"
#include "TGraph.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLine.h"
#include "TMarker.h"
#include "TROOT.h"
#include "TStyle.h"

#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "DataFormats/GeometryVector/interface/LocalPoint.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/Math/interface/deltaPhi.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "Geometry/CaloGeometry/interface/CaloCellGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloSubdetectorGeometry.h"
#include "Geometry/CommonDetUnit/interface/GeomDet.h"
#include "Geometry/CommonTopologies/interface/GlobalTrackingGeometry.h"
#include "Geometry/HcalCommonData/interface/HcalDDDRecConstants.h"
#include "Geometry/HcalCommonData/interface/HcalHitRelabeller.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "Geometry/Records/interface/GlobalTrackingGeometryRecord.h"
#include "Geometry/Records/interface/HcalRecNumberingRecord.h"
#include "SimDataFormats/CaloHit/interface/PCaloHitContainer.h"
#include "SimDataFormats/Track/interface/SimTrackContainer.h"
#include "SimDataFormats/TrackingHit/interface/PSimHitContainer.h"
#include "SimDataFormats/Vertex/interface/SimVertexContainer.h"

class SimAnalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit SimAnalyzer(const edm::ParameterSet& ps);
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  static constexpr int kNPages = 5;
  static constexpr const char* kPageName[kNPages + 1] = {
      "", "primaries", "caloSurface", "secondaries", "trackerMuonHits", "caloHits"};
  static constexpr const char* kPageTitle[kNPages + 1] = {
      "",
      "1. primaries: status 1 genParticles handed to Geant4",
      "2. transport: primaries at the tracker / calorimeter boundary",
      "3. secondaries: particles created by Geant4, by creator process",
      "4. tracker and muon SimHits (PSimHit)",
      "5. calorimeter SimHits (PCaloHit, energy per cell)"};
  static constexpr const char* kStageName[kNPages] = {
      "1 transport", "2 + B field", "3 + decays in flight", "4 + EM physics", "5 + hadronic physics"};

  // particle kind (pages 1, 2)
  enum Kind { CHARGED_HADRON = 0, NEUTRAL_HADRON, PHOTON, ELECTRON, MUON, NEUTRINO, OTHER_KIND, NOT_PASSED, NKIND };
  static constexpr const char* kKindName[NKIND] = {"charged hadron", "neutral hadron", "photon", "e^{#pm}", "#mu^{#pm}",
                                                   "neutrino",       "other",          "status 1, no SimTrack"};
  static constexpr int kKindColor[NKIND] = {kBlack, kGray + 2, kGreen + 2, kOrange + 7, kCyan + 2, kGray + 1, kViolet + 1, kGray + 1};
  static constexpr int kKindMarker[NKIND] = {20, 24, 20, 21, 22, 27, 28, 2};

  // creator process of a secondary (page 3): Geant4 process sub-type stored in SimVertex::processType
  enum Proc { PRIMARY = 0, DECAY, IONISATION, BREMSSTRAHLUNG, PAIR, COMPTON, PHOTOELECTRIC, ANNIHILATION, EM_OTHER,
              HAD_ELASTIC, HAD_INELASTIC, CAPTURE, AT_REST, HAD_OTHER, OTHER_PROC, NPROC };
  static constexpr const char* kProcName[NPROC] = {"primary (gen)",       "decay",            "ionisation (#delta ray)",
                                                   "bremsstrahlung",      "pair production / conversion", "Compton scattering",
                                                   "photoelectric effect", "annihilation",     "other EM",
                                                   "hadron elastic",      "hadron inelastic", "capture",
                                                   "at rest",             "other hadronic",   "other"};
  static constexpr int kProcColor[NPROC] = {kGray,      kViolet + 1, kOrange + 7, kOrange - 3, kGreen + 2,
                                            kSpring - 6, kTeal - 6,   kGreen + 3,  kGreen + 1,  kCyan + 2,
                                            kAzure + 7,  kAzure - 6,  kAzure + 2,  kAzure + 1,  kGray + 2};
  static constexpr int kProcMarker[NPROC] = {1, 20, 26, 32, 25, 24, 27, 30, 28, 26, 22, 23, 21, 29, 28};
  static constexpr int kProcGroup[NPROC] = {0, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4};  // 1 decay, 2 EM, 3 hadronic, 4 other
  static constexpr const char* kGroupName[5] = {"primary", "decay", "electromagnetic", "hadronic", "other"};

  // subdetector of a hit (pages 4, 5)
  // HGC EE, HGC HSi, HGC HSc: the Phase2 HGCal (DetId::HGCalEE, HGCalHSi, HGCalHSc), replacing EE, ES and HE
  enum Sub { PXB = 0, PXF, TIB, TID, TOB, TEC, DT, CSC, RPC, GEM, EB, EE, ES, HB, HE, HO, HF, HGCEE, HGCHSI, HGCHSC, OTHER_SUB, NSUB };
  static constexpr const char* kSubName[NSUB] = {"PXB", "PXF", "TIB", "TID", "TOB", "TEC", "DT", "CSC", "RPC",
                                                 "GEM", "EB",  "EE",  "ES",  "HB",  "HE",  "HO", "HF",
                                                 "HGC EE", "HGC HSi", "HGC HSc", "other"};
  static constexpr int kSubColor[NSUB] = {kOrange + 7, kOrange - 3, kGreen + 2,  kGreen - 6, kCyan + 2,  kAzure + 7,
                                          kViolet + 1, kViolet - 5, kGray + 2,   kPink - 3,  kGreen + 2, kGreen - 6,
                                          kSpring + 5, kAzure + 7,  kAzure - 3,  kCyan + 2,  kTeal + 3,
                                          kGreen - 3,  kAzure - 7,  kAzure + 4,  kGray + 1};
  static constexpr int kSysColor[2] = {kRed, kBlue};

  struct Sys {  // the b or bbar system on one page
    math::XYZTLorentzVector p4;
    int n = 0;      // members (primaries, secondaries, hits or cells)
    int nAll = 0;   // candidates (e.g. primaries of the system on page 2)
    double e[3] = {0., 0., 0.};  // page 5: ECAL, HCAL (HB, HE, HO) and HF energy
    int nProc[5] = {0, 0, 0, 0, 0};  // page 3: members per process group
  };

  // one eta-phi page: frame, markers (individual or grouped), labels, header, legend, print
  struct Page {
    SimAnalyzer* a;
    int page;
    std::vector<std::unique_ptr<TObject>> keep;
    std::map<std::string, TGraph*> groups;
    std::unique_ptr<TLegend> leg;
    std::vector<std::unique_ptr<TMarker>> legMarkers;
    int nClamped = 0;
    Page(SimAnalyzer* an, int p);
    double clamp(double eta, bool& clamped);
    double point(int color, int style, double size, double eta, double phi);  // own TGraph, returns the drawn eta
    void groupPoint(int color, int style, double size, double eta, double phi);  // one TGraph per (color, style, size)
    void line(int color, double eta0, double phi0, double eta1, double phi1);
    void label(double x, double y, const std::string& s, double size, int color);
    void text(double x, double y, const std::string& s, double size, int color = kBlack, int align = 11);
    void legEntry(int color, int style, const std::string& entry);
    void finish(const edm::Event& ev, const std::vector<std::string>& header, int scaleKind);
  };

  using Coll = reco::GenParticleCollection;
  void beginJob() override;
  void analyze(const edm::Event& ev, const edm::EventSetup& es) override;
  void endJob() override;

  static int bContent(int pdg);
  static std::string pdgName(int pdg);
  static double markerSize(double pt) { return 0.6 + 0.9 * std::log10(1. + pt); }
  static double cellSize(double e) {  // page 5: 6 size classes, 1 MeV ... 100 GeV
    int k = std::max(0, std::min(5, static_cast<int>(std::floor(std::log10(std::max(e, 1e-6)) + 3.))));
    return 0.35 + 0.33 * k;
  }
  static int kindOf(int pdg, double charge);
  static int procOf(unsigned int processType);
  static int subOf(const DetId& id);
  void matchBSystems(const Coll& gens, std::vector<int>& genSys, std::string what[2]) const;
  void descendants(size_t i, const Coll& gens, std::vector<char>& out) const;
  std::string sysLine(const char* name, const Sys& s, int color) const;

  edm::EDGetTokenT<Coll> genParticlesTok_;
  edm::EDGetTokenT<std::vector<int>> genBarcodesTok_;
  edm::EDGetTokenT<edm::SimTrackContainer> simTracksTok_;
  edm::EDGetTokenT<edm::SimVertexContainer> simVerticesTok_;
  std::vector<std::pair<edm::InputTag, edm::EDGetTokenT<edm::PSimHitContainer>>> trackerHitsTok_;
  std::vector<std::pair<edm::InputTag, edm::EDGetTokenT<edm::PCaloHitContainer>>> caloHitsTok_;
  edm::ESGetToken<GlobalTrackingGeometry, GlobalTrackingGeometryRecord> trackingGeomTok_;
  edm::ESGetToken<CaloGeometry, CaloGeometryRecord> caloGeomTok_;
  edm::ESGetToken<HcalDDDRecConstants, HcalRecNumberingRecord> hcalRecTok_;
  std::string outputPrefix_, pdfFile_, sampleLabel_;
  double etaMax_, labelPtMin_;
  bool savePDF_, printTracks_, openPdf_, closePdf_, hcalTestNumbering_;
  int maxPage_, simStage_;
  bool pdfOpen_ = false;

  std::unique_ptr<TCanvas> canvas_;
  TH2D* hEtaPhi_[kNPages + 1] = {};
  TH1D* hN_[kNPages + 1] = {};
  TH1D* hCat_[kNPages + 1] = {};
  TH1D* hPtB_[kNPages + 1] = {};
  TH1D* hPtBbar_[kNPages + 1] = {};
  TH1D* hPtNet_[kNPages + 1] = {};
  TH1D* hEcalB_ = nullptr;
  TH1D* hEcalBbar_ = nullptr;
  TH1D* hHcalB_ = nullptr;
  TH1D* hHcalBbar_ = nullptr;
};

SimAnalyzer::SimAnalyzer(const edm::ParameterSet& ps)
    : genParticlesTok_(consumes<Coll>(ps.getParameter<edm::InputTag>("genParticles"))),
      genBarcodesTok_(consumes<std::vector<int>>(ps.getParameter<edm::InputTag>("genParticles"))),
      simTracksTok_(consumes<edm::SimTrackContainer>(ps.getParameter<edm::InputTag>("simTracks"))),
      simVerticesTok_(consumes<edm::SimVertexContainer>(ps.getParameter<edm::InputTag>("simVertices"))),
      trackingGeomTok_(esConsumes()),
      caloGeomTok_(esConsumes()),
      hcalRecTok_(esConsumes()),
      outputPrefix_(ps.getParameter<std::string>("outputPrefix")),
      pdfFile_(ps.getParameter<std::string>("pdfFile")),
      sampleLabel_(ps.getParameter<std::string>("sampleLabel")),
      etaMax_(ps.getParameter<double>("etaMax")),
      labelPtMin_(ps.getParameter<double>("labelPtMin")),
      savePDF_(ps.getParameter<bool>("savePDF")),
      printTracks_(ps.getParameter<bool>("printTracks")),
      openPdf_(ps.getParameter<bool>("openPdf")),
      closePdf_(ps.getParameter<bool>("closePdf")),
      hcalTestNumbering_(ps.getParameter<bool>("hcalTestNumbering")),
      maxPage_(std::min(std::max(ps.getParameter<int>("maxPage"), 1), kNPages)),
      simStage_(ps.getParameter<int>("simStage")) {
  for (auto const& t : ps.getParameter<std::vector<edm::InputTag>>("trackerHits"))
    trackerHitsTok_.emplace_back(t, consumes<edm::PSimHitContainer>(t));
  for (auto const& t : ps.getParameter<std::vector<edm::InputTag>>("caloHits"))
    caloHitsTok_.emplace_back(t, consumes<edm::PCaloHitContainer>(t));
  usesResource(TFileService::kSharedResource);
  edm::Service<TFileService> fs;
  for (int p = 1; p <= kNPages; ++p) {
    TFileDirectory dir = fs->mkdir(Form("page%d_%s", p, kPageName[p]));
    const char* w = p == 5 ? "#Sigma E [GeV]" : (p == 4 ? "hits" : "#Sigma p_{T} [GeV]");
    hEtaPhi_[p] = dir.make<TH2D>("etaPhi", Form("%s;#eta;#phi;%s", kPageTitle[p], w), 120, -etaMax_, etaMax_, 64, -M_PI, M_PI);
    const char* nName[kNPages + 1] = {"", "nPrimaries", "nReachingBoundary", "nSecondaries", "nHits", "nCells"};
    hN_[p] = dir.make<TH1D>(nName[p], Form("%s;%s;events", kPageTitle[p], nName[p]), 200, 0., p >= 4 ? 20000. : 2000.);
    int nCat = p <= 2 ? static_cast<int>(NKIND) : (p == 3 ? static_cast<int>(NPROC) : static_cast<int>(NSUB));
    hCat_[p] = dir.make<TH1D>(p <= 2 ? "kind" : (p == 3 ? "process" : "subdetector"),
                              Form("%s;;%s", kPageTitle[p], p == 5 ? "#Sigma E [GeV]" : "entries"), nCat, 0., nCat);
    for (int k = 0; k < nCat; ++k)
      hCat_[p]->GetXaxis()->SetBinLabel(k + 1, p <= 2 ? kKindName[k] : (p == 3 ? kProcName[k] : kSubName[k]));
    if (p <= 3) {
      hPtB_[p] = dir.make<TH1D>("ptB", Form("%s;p_{T}(b system) [GeV];events", kPageTitle[p]), 100, 0., 500.);
      hPtBbar_[p] = dir.make<TH1D>("ptBbar", Form("%s;p_{T}(#bar{b} system) [GeV];events", kPageTitle[p]), 100, 0., 500.);
      hPtNet_[p] = dir.make<TH1D>("ptNetBBbar", Form("%s;|#vec{p}_{T}(b) + #vec{p}_{T}(#bar{b})| [GeV];events", kPageTitle[p]),
                                  100, 0., 200.);
    }
    if (p == 5) {
      hEcalB_ = dir.make<TH1D>("eEcalB", "ECAL SimHit energy of the b system;E [GeV];events", 100, 0., 500.);
      hEcalBbar_ = dir.make<TH1D>("eEcalBbar", "ECAL SimHit energy of the #bar{b} system;E [GeV];events", 100, 0., 500.);
      hHcalB_ = dir.make<TH1D>("eHcalB", "HCAL SimHit energy (visible) of the b system;E [GeV];events", 100, 0., 10.);
      hHcalBbar_ = dir.make<TH1D>("eHcalBbar", "HCAL SimHit energy (visible) of the #bar{b} system;E [GeV];events", 100, 0., 10.);
    }
  }
}

void SimAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("genParticles", edm::InputTag("genParticles", "", "GEN"))
      ->setComment("genParticles and (same label) their HepMC barcodes, the SimTrack gen link");
  desc.add<edm::InputTag>("simTracks", edm::InputTag("g4SimHits"));
  desc.add<edm::InputTag>("simVertices", edm::InputTag("g4SimHits"));
  desc.add<std::vector<edm::InputTag>>("trackerHits", {})->setComment("PSimHit collections (tracker and muon) of page 4");
  desc.add<std::vector<edm::InputTag>>("caloHits", {})->setComment("PCaloHit collections of page 5");
  desc.add<std::string>("outputPrefix", "simAnalyzer")
      ->setComment("PDFs are written as <outputPrefix>_pageN_<name>_run<R>_event<E>.pdf");
  desc.add<double>("etaMax", 6.)->setComment("eta axis range; objects beyond are drawn on the border with an x marker");
  desc.add<double>("labelPtMin", 5.)->setComment("pT above which the pT value is written next to the marker (b, bbar always)");
  desc.add<bool>("savePDF", true);
  desc.add<std::string>("pdfFile", "")->setComment("if set, all pages go into this single PDF");
  desc.add<std::string>("sampleLabel", "")->setComment("printed in the right panel, '|' separates lines (e.g. the SIM stage)");
  desc.add<int>("maxPage", kNPages)->setComment("last page to draw: 1 primaries, 2 calo surface, 3 secondaries, 4 tracker/muon hits, 5 calo hits");
  desc.add<int>("simStage", 0)->setComment("SIM stage of the sample, highlighted in the right panel (1 transport ... 5 hadronic physics, 0 none)");
  desc.add<bool>("printTracks", false)->setComment("print every SimTrack with its gen link, ancestor primary, process and system to stdout");
  desc.add<bool>("hcalTestNumbering", true)->setComment("HCAL PCaloHit ids are in the simulation test numbering (relabelled to HcalDetId)");
  desc.add<bool>("openPdf", true)->setComment("open pdfFile before the first page (false: another module of the job opens it)");
  desc.add<bool>("closePdf", true)->setComment("close pdfFile in endJob (false: another module of the job closes it)");
  descriptions.addWithDefaultLabel(desc);
}

// ----------------------------------------------------------------------------- classification
int SimAnalyzer::bContent(int pdg) {  // +n: n b quarks, -n: n anti-b quarks (as in GenPartAnalyzer)
  int a = std::abs(pdg);
  if (a < 100 || a > 9999)
    return 0;
  int q1 = (a / 1000) % 10, q2 = (a / 100) % 10, q3 = (a / 10) % 10;
  if (q1 == 0) {
    if (q2 == 5 && q3 == 5)
      return 0;
    if (q2 == 5)
      return pdg > 0 ? -1 : 1;
    return 0;
  }
  int n = (q1 == 5) + (q2 == 5) + (q3 == 5);
  return pdg > 0 ? n : -n;
}

std::string SimAnalyzer::pdgName(int pdg) {
  static const std::map<int, std::string> names = {
      {11, "e^{-}"},      {12, "#nu_{e}"},    {13, "#mu^{-}"},   {14, "#nu_{#mu}"},  {15, "#tau^{-}"},  {16, "#nu_{#tau}"},
      {22, "#gamma"},     {111, "#pi^{0}"},   {211, "#pi^{+}"},  {130, "K_{L}^{0}"}, {310, "K_{S}^{0}"}, {321, "K^{+}"},
      {2212, "p"},        {2112, "n"},        {3122, "#Lambda"}, {3222, "#Sigma^{+}"}, {3112, "#Sigma^{-}"}, {3322, "#Xi^{0}"},
      {3312, "#Xi^{-}"},  {3334, "#Omega^{-}"}, {411, "D^{+}"},  {421, "D^{0}"},     {431, "D_{s}^{+}"}, {4122, "#Lambda_{c}^{+}"},
      {511, "B^{0}"},     {521, "B^{+}"},     {531, "B_{s}^{0}"}, {541, "B_{c}^{+}"}, {513, "B^{*0}"},   {523, "B^{*+}"},
      {533, "B_{s}^{*0}"}, {5122, "#Lambda_{b}^{0}"}, {5232, "#Xi_{b}^{0}"}, {5132, "#Xi_{b}^{-}"}, {5332, "#Omega_{b}^{-}"}};
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
    p = n.find("^{*+}");
    if (p != std::string::npos)
      return n.replace(p, 5, "^{*-}");
    return "#bar{" + n + "}";
  }
  return n;
}

int SimAnalyzer::kindOf(int pdg, double charge) {
  int a = std::abs(pdg);
  if (a == 22)
    return PHOTON;
  if (a == 11)
    return ELECTRON;
  if (a == 13)
    return MUON;
  if (a == 12 || a == 14 || a == 16)
    return NEUTRINO;
  if (a < 100)
    return OTHER_KIND;
  return charge != 0. ? CHARGED_HADRON : NEUTRAL_HADRON;
}

// Geant4 process sub-types: G4EmProcessSubType, G4HadronicProcessType, G4DecayProcessType
int SimAnalyzer::procOf(unsigned int t) {
  if (t == 0)
    return PRIMARY;
  if (t >= 201 && t <= 239)
    return DECAY;
  if (t <= 50) {
    switch (t) {
      case 2:
        return IONISATION;
      case 3:
        return BREMSSTRAHLUNG;
      case 4:
      case 14:
      case 15:
      case 49:
        return PAIR;
      case 13:
        return COMPTON;
      case 12:
        return PHOTOELECTRIC;
      case 5:
      case 6:
      case 7:
      case 18:
        return ANNIHILATION;
      default:
        return EM_OTHER;
    }
  }
  if (t >= 111 && t <= 199) {
    switch (t) {
      case 111:
        return HAD_ELASTIC;
      case 116:
      case 121:
        return HAD_INELASTIC;
      case 131:
      case 132:
        return CAPTURE;
      case 151:
      case 152:
        return AT_REST;
      default:
        return HAD_OTHER;
    }
  }
  if (t == 310)
    return HAD_OTHER;
  return OTHER_PROC;
}

int SimAnalyzer::subOf(const DetId& id) {
  int s = id.subdetId();
  switch (id.det()) {
    case DetId::Tracker:
      return s >= 1 && s <= 6 ? static_cast<int>(PXB) + s - 1 : static_cast<int>(OTHER_SUB);
    case DetId::Muon:
      return s >= 1 && s <= 4 ? static_cast<int>(DT) + s - 1 : static_cast<int>(OTHER_SUB);
    case DetId::Ecal:
      return s == 1 ? EB : (s == 2 ? EE : (s == 3 ? ES : OTHER_SUB));  // EcalBarrel, EcalEndcap, EcalPreshower
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

void SimAnalyzer::descendants(size_t i, const Coll& gens, std::vector<char>& out) const {
  for (size_t d = 0; d < gens[i].numberOfDaughters(); ++d) {
    size_t k = gens[i].daughterRef(d).key();
    if (out[k])
      continue;
    out[k] = 1;
    descendants(k, gens, out);
  }
}

// b / bbar systems of the status 1 particles, as GenPartAnalyzer stages 4-5: every hard-process quark
// is followed through its same-flavour copies to the last copy, matched to the nearest primary b hadron
// (b content of the right sign, no mother with b content) and the stable descendants of that hadron
// form the system. genSys[i] = 0 (b), 1 (bbar) or -1.
void SimAnalyzer::matchBSystems(const Coll& gens, std::vector<int>& genSys, std::string what[2]) const {
  const size_t n = gens.size();
  std::vector<char> primaryHadron(n, 0);
  for (size_t i = 0; i < n; ++i) {
    if (bContent(gens[i].pdgId()) == 0)
      continue;
    bool fromB = false;
    for (size_t m = 0; m < gens[i].numberOfMothers(); ++m)
      fromB |= bContent(gens[gens[i].motherRef(m).key()].pdgId()) != 0;
    primaryHadron[i] = !fromB;
  }
  std::vector<char> used(n, 0);
  for (int k = 0; k < 2; ++k) {
    int sign = k == 0 ? 1 : -1;
    what[k] = k == 0 ? "b" : "#bar{b}";
    // last same-flavour copies of the hard-process quarks
    std::vector<size_t> last;
    for (size_t i = 0; i < n; ++i) {
      if (gens[i].pdgId() != 5 * sign || !gens[i].isHardProcess() || gens[i].pt() <= 0.)
        continue;
      size_t q = i;
      bool more = true;
      while (more) {
        more = false;
        for (size_t d = 0; d < gens[q].numberOfDaughters(); ++d) {
          size_t c = gens[q].daughterRef(d).key();
          if (gens[c].pdgId() == gens[q].pdgId()) {
            q = c;
            more = true;
            break;
          }
        }
      }
      last.push_back(q);
    }
    std::string hadrons;
    for (size_t q : last) {
      double bestDR = 1e9;
      size_t best = n;
      for (size_t i = 0; i < n; ++i)
        if (primaryHadron[i] && !used[i] && bContent(gens[i].pdgId()) * sign > 0) {
          double dr = reco::deltaR(gens[q], gens[i]);
          if (dr < bestDR) {
            bestDR = dr;
            best = i;
          }
        }
      if (best == n)
        continue;
      used[best] = 1;
      std::vector<char> desc(n, 0);
      descendants(best, gens, desc);
      for (size_t i = 0; i < n; ++i)
        if (desc[i] && gens[i].status() == 1 && genSys[i] < 0)
          genSys[i] = k;
      hadrons += (hadrons.empty() ? "" : ", ") + pdgName(gens[best].pdgId());
      std::cout << "  " << (k == 0 ? "b" : "bbar") << " quark [" << q << "] pt=" << gens[q].pt() << " -> primary hadron ["
                << best << "] pdg=" << gens[best].pdgId() << " pt=" << gens[best].pt() << " dR=" << bestDR << "\n";
    }
    if (!hadrons.empty())
      what[k] += " hadron " + hadrons + " decay products";
  }
}

std::string SimAnalyzer::sysLine(const char* name, const Sys& s, int) const {
  if (s.n == 0 && s.nAll == 0)
    return std::string(name) + ": not found";
  return Form("%s: p_{T} = %.2f GeV, #eta = %.2f, #phi = %.2f", name, s.p4.pt(), s.p4.pt() > 0 ? s.p4.eta() : 0., s.p4.phi());
}

// ----------------------------------------------------------------------------- job
void SimAnalyzer::beginJob() {
  gROOT->SetBatch(true);
  gStyle->SetOptStat(0);
  gStyle->SetPaperSize(29., 20.);  // A4 landscape width for the 1400 x 900 canvas (see GenPartAnalyzer)
  canvas_ = std::make_unique<TCanvas>("simCanvas", "", 1400, 900);
}

void SimAnalyzer::analyze(const edm::Event& ev, const edm::EventSetup& es) {
  const Coll& gens = ev.get(genParticlesTok_);
  const std::vector<int>& barcodes = ev.get(genBarcodesTok_);
  const edm::SimTrackContainer& tracks = ev.get(simTracksTok_);
  const edm::SimVertexContainer& vertices = ev.get(simVerticesTok_);
  const size_t ng = gens.size(), nt = tracks.size();

  std::cout << "\n== SimAnalyzer run:lumi:event " << ev.id().run() << ":" << ev.luminosityBlock() << ":" << ev.id().event()
            << " nGenParticles=" << ng << " nSimTracks=" << nt << " nSimVertices=" << vertices.size() << "\n";

  // 1. gen side: the b / bbar systems and the barcode -> index map
  std::vector<int> genSys(ng, -1);
  std::string what[2];
  matchBSystems(gens, genSys, what);
  std::unordered_map<int, int> barcodeToGen;
  for (size_t i = 0; i < barcodes.size(); ++i)
    barcodeToGen[barcodes[i]] = i;

  // 2. sim side: gen link, ancestor primary, system and creator process of every SimTrack
  std::unordered_map<unsigned int, int> trackIdx;
  for (size_t i = 0; i < nt; ++i)
    trackIdx[tracks[i].trackId()] = i;
  std::vector<int> genIdx(nt, -1), anc(nt, -2), sys(nt, -1), proc(nt, OTHER_PROC), genToTrack(ng, -1);
  for (size_t i = 0; i < nt; ++i) {
    const SimTrack& t = tracks[i];
    if (!t.noGenpart()) {
      auto it = barcodeToGen.find(t.genpartIndex());
      if (it != barcodeToGen.end()) {
        genIdx[i] = it->second;
        if (genToTrack[it->second] < 0)
          genToTrack[it->second] = i;
      }
    }
    int v = t.vertIndex();
    proc[i] = t.isPrimary() ? PRIMARY : (v >= 0 && static_cast<size_t>(v) < vertices.size() ? procOf(vertices[v].processType()) : OTHER_PROC);
  }
  for (size_t i = 0; i < nt; ++i) {
    std::vector<int> path;
    int cur = i, result = -1;
    while (true) {
      if (anc[cur] != -2) {
        result = anc[cur];
        break;
      }
      path.push_back(cur);
      const SimTrack& t = tracks[cur];
      if (t.isPrimary()) {
        result = cur;
        break;
      }
      int v = t.vertIndex();
      if (v < 0 || static_cast<size_t>(v) >= vertices.size() || vertices[v].noParent())
        break;
      auto it = trackIdx.find(vertices[v].parentIndex());
      if (it == trackIdx.end())
        break;
      cur = it->second;
      if (path.size() > nt)
        break;  // cycle guard
    }
    for (int p : path)
      anc[p] = result;
  }
  for (size_t i = 0; i < nt; ++i)
    if (anc[i] >= 0 && genIdx[anc[i]] >= 0)
      sys[i] = genSys[genIdx[anc[i]]];
  auto sysOfTrackId = [&](unsigned int id) {
    auto it = trackIdx.find(id);
    return it == trackIdx.end() ? -1 : sys[it->second];
  };

  // primary vertex (smeared): the vertex of the first primary; hit directions are taken from it
  math::XYZTLorentzVectorD pv;
  bool havePv = false;
  for (size_t i = 0; i < nt && !havePv; ++i)
    if (tracks[i].isPrimary() && tracks[i].vertIndex() >= 0 && static_cast<size_t>(tracks[i].vertIndex()) < vertices.size()) {
      pv = vertices[tracks[i].vertIndex()].position();
      havePv = true;
    }
  auto dirEta = [&](double x, double y, double z) { return math::XYZVectorD(x - pv.x(), y - pv.y(), z - pv.z()).eta(); };
  auto dirPhi = [&](double x, double y, double z) { return math::XYZVectorD(x - pv.x(), y - pv.y(), z - pv.z()).phi(); };

  size_t nStatus1 = 0, nPrimaries = 0, nSecondaries = 0;
  for (size_t i = 0; i < ng; ++i)
    nStatus1 += gens[i].status() == 1;
  for (size_t i = 0; i < nt; ++i)
    (tracks[i].isPrimary() ? nPrimaries : nSecondaries)++;
  if (printTracks_) {
    std::cout << "    columns: [index] trackId pdg pt eta phi vertex(x,y,z) process ancestorPrimary genIdx system crossedBoundary\n";
    for (size_t i = 0; i < nt; ++i) {
      const SimTrack& t = tracks[i];
      int v = t.vertIndex();
      std::cout << "    [" << i << "] trackId=" << t.trackId() << " pdg=" << t.type() << " pt=" << t.momentum().pt()
                << " eta=" << t.momentum().eta() << " phi=" << t.momentum().phi();
      if (v >= 0 && static_cast<size_t>(v) < vertices.size())
        std::cout << " vtx=(" << vertices[v].position().x() << "," << vertices[v].position().y() << ","
                  << vertices[v].position().z() << ")";
      std::cout << " process=" << kProcName[proc[i]] << " anc=" << anc[i] << " genIdx=" << genIdx[i]
                << " system=" << (sys[i] < 0 ? "-" : (sys[i] == 0 ? "b" : "bbar")) << " crossed=" << t.crossedBoundary() << "\n";
    }
  }

  // ------------------------------------------------------------------ page 1: primaries
  const char* sysName[2] = {"b system", "#bar{b} system"};
  if (maxPage_ >= 1) {
    Page P(this, 1);
    Sys S[2];
    int nKind[NKIND] = {}, nEta = 0, nP = 0, nNu = 0, nOtherCut = 0;
    for (size_t g = 0; g < ng; ++g) {
      if (gens[g].status() != 1)
        continue;
      int ti = genToTrack[g];
      int kind = ti >= 0 ? kindOf(gens[g].pdgId(), gens[g].charge()) : NOT_PASSED;
      ++nKind[kind];
      hCat_[1]->Fill(kind);
      double pt = gens[g].pt(), eta = gens[g].eta(), phi = gens[g].phi();
      if (genSys[g] >= 0)
        ++S[genSys[g]].nAll;
      if (ti < 0) {
        int a = std::abs(gens[g].pdgId());
        if (std::fabs(eta) > 5.5)
          ++nEta;
        else if (gens[g].p() < 0.04)
          ++nP;
        else if (a == 12 || a == 14 || a == 16)
          ++nNu;
        else
          ++nOtherCut;
        P.point(kKindColor[NOT_PASSED], kKindMarker[NOT_PASSED], markerSize(pt), eta, phi);
        continue;
      }
      hEtaPhi_[1]->Fill(eta, phi, pt);
      int k = genSys[g];
      if (k >= 0) {
        S[k].p4 += tracks[ti].momentum();
        ++S[k].n;
        continue;  // drawn on top below
      }
      double etaD = P.point(kKindColor[kind], kKindMarker[kind], markerSize(pt), eta, phi);
      if (pt >= labelPtMin_)
        P.label(etaD + 0.05, phi + 0.06, Form("%.1f", pt), 0.016, kKindColor[kind]);
    }
    for (size_t g = 0; g < ng; ++g) {
      int k = genSys[g], ti = genToTrack[g];
      if (k < 0 || ti < 0 || gens[g].status() != 1)
        continue;
      double pt = gens[g].pt(), phi = gens[g].phi();
      double etaD = P.point(kSysColor[k], 20, markerSize(pt), gens[g].eta(), phi);
      if (pt >= labelPtMin_)
        P.label(etaD + 0.05, phi + 0.08, Form("%s %.1f", pdgName(gens[g].pdgId()).c_str(), pt), 0.016, kSysColor[k]);
    }
    math::XYZTLorentzVector net = S[0].p4 + S[1].p4;
    bool both = S[0].n && S[1].n;
    if (both && net.pt() > 0.1) {
      double etaD = P.point(kMagenta, 29, markerSize(net.pt()), net.eta(), net.phi());
      P.label(etaD + 0.05, net.phi() - 0.15, Form("b+#bar{b} %.1f", net.pt()), 0.022, kMagenta);
    }
    hN_[1]->Fill(nPrimaries);
    if (S[0].n)
      hPtB_[1]->Fill(S[0].p4.pt());
    if (S[1].n)
      hPtBbar_[1]->Fill(S[1].p4.pt());
    if (both)
      hPtNet_[1]->Fill(net.pt());
    std::vector<std::string> header;
    header.push_back(Form("%zu primaries (SimTracks with gen link) of %zu status 1 genParticles; %d without SimTrack: "
                          "%d |#eta| > 5.5, %d p < 40 MeV, %d #nu (not tracked), %d other",
                          nPrimaries, nStatus1, nKind[NOT_PASSED], nEta, nP, nNu, nOtherCut));
    header.push_back(havePv ? Form("primary vertex (smeared): x = %.4f, y = %.4f, z = %.3f cm", pv.x(), pv.y(), pv.z())
                            : "primary vertex: not found");
    for (int k = 0; k < 2; ++k)
      header.push_back(sysLine(Form("%s (%s, %d of %d handed to Geant4)", sysName[k], what[k].c_str(), S[k].n, S[k].nAll), S[k], kSysColor[k]));
    if (both)
      header.push_back(Form("net b+#bar{b}: |#vec{p}_{T}(b) + #vec{p}_{T}(#bar{b})| = %.2f GeV,  #Sigma|p_{T}| = %.2f GeV,  #Delta#phi = %.2f",
                            net.pt(), S[0].p4.pt() + S[1].p4.pt(), std::fabs(reco::deltaPhi(S[0].p4.phi(), S[1].p4.phi()))));
    for (int k = 0; k < 2; ++k)
      if (S[k].n)
        P.legEntry(kSysColor[k], 20, Form("%s (%d)", sysName[k], S[k].n));
    if (both)
      P.legEntry(kMagenta, 29, "b + #bar{b} (vector sum)");
    for (int k = 0; k < NKIND; ++k)
      if (nKind[k])
        P.legEntry(kKindColor[k], kKindMarker[k], Form("%s (%d)", kKindName[k], nKind[k]));
    std::cout << "  page 1 (primaries): nPrimaries=" << nPrimaries << " nStatus1=" << nStatus1 << " noSimTrack=" << nKind[NOT_PASSED]
              << " (eta " << nEta << ", p " << nP << ", nu " << nNu << ", other " << nOtherCut << ")";
    if (havePv)
      std::cout << " PV=(" << pv.x() << "," << pv.y() << "," << pv.z() << ")";
    for (int k = 0; k < 2; ++k)
      std::cout << " | " << (k ? "bbar" : "b") << " system: n=" << S[k].n << "/" << S[k].nAll << " pt=" << S[k].p4.pt() << " eta=" << (S[k].n ? S[k].p4.eta() : 0.)
                << " phi=" << S[k].p4.phi();
    std::cout << " | net |sum pT|=" << net.pt() << "\n";
    if (savePDF_)
      P.finish(ev, header, 1);
  }

  // ------------------------------------------------------------------ page 2: calo surface
  if (maxPage_ >= 2) {
    Page P(this, 2);
    Sys S[2];
    int nKind[NKIND] = {}, nCross = 0, nNoCross = 0, nSysNoCross[2] = {0, 0};
    for (int pass = 0; pass < 2; ++pass)
      for (size_t i = 0; i < nt; ++i) {
        const SimTrack& t = tracks[i];
        if (!t.isPrimary() || (pass == 0) != (sys[i] < 0))
          continue;
        int k = sys[i];
        int kind = kindOf(t.type(), t.charge());
        int color = k >= 0 ? kSysColor[k] : kKindColor[kind];
        double eta0 = t.momentum().eta(), phi0 = t.momentum().phi(), pt0 = t.momentum().pt();
        if (k >= 0)
          ++S[k].nAll;
        if (t.crossedBoundary()) {
          const auto& pos = t.trackerSurfacePosition();
          const auto& mom = t.trackerSurfaceMomentum();
          double eta1 = dirEta(pos.x(), pos.y(), pos.z()), phi1 = dirPhi(pos.x(), pos.y(), pos.z()), pt1 = mom.pt();
          if (pass == 0) {
            ++nCross;
            ++nKind[kind];
            hCat_[2]->Fill(kind);
            hEtaPhi_[2]->Fill(eta1, phi1, pt1);
          } else {
            ++S[k].n;
            S[k].p4 += mom;
            ++nCross;
            ++nKind[kind];
            hCat_[2]->Fill(kind);
            hEtaPhi_[2]->Fill(eta1, phi1, pt1);
          }
          if (pt0 >= 1. && std::fabs(eta0) <= etaMax_ && std::fabs(eta1) <= etaMax_)  // loopers below 1 GeV would cover the page
            P.line(color, eta0, phi0, eta1, phi1);
          double etaD = P.point(color, k >= 0 ? 20 : kKindMarker[kind], markerSize(pt1), eta1, phi1);
          if (k >= 0 && pt1 >= labelPtMin_)
            P.label(etaD + 0.05, phi1 + 0.08, Form("%s %.1f", pdgName(t.type()).c_str(), pt1), 0.016, color);
          else if (k < 0 && pt1 >= labelPtMin_)
            P.label(etaD + 0.05, phi1 + 0.06, Form("%.1f", pt1), 0.016, color);
        } else {
          ++nNoCross;
          if (k >= 0)
            ++nSysNoCross[k];
          double etaD = P.point(color, 24, markerSize(pt0), eta0, phi0);
          if (pt0 >= labelPtMin_)
            P.label(etaD + 0.05, phi0 + 0.06, Form("%s%.1f", k >= 0 ? (pdgName(t.type()) + " ").c_str() : "", pt0), 0.016, color);
        }
      }
    math::XYZTLorentzVector net = S[0].p4 + S[1].p4;
    bool both = S[0].n && S[1].n;
    if (both && net.pt() > 0.1) {
      double etaD = P.point(kMagenta, 29, markerSize(net.pt()), net.eta(), net.phi());
      P.label(etaD + 0.05, net.phi() - 0.15, Form("b+#bar{b} %.1f", net.pt()), 0.022, kMagenta);
    }
    hN_[2]->Fill(nCross);
    if (S[0].n)
      hPtB_[2]->Fill(S[0].p4.pt());
    if (S[1].n)
      hPtBbar_[2]->Fill(S[1].p4.pt());
    if (both)
      hPtNet_[2]->Fill(net.pt());
    std::vector<std::string> header;
    header.push_back(Form("%d of %zu primaries reach the tracker / calorimeter boundary (marker at the crossing point seen from the primary "
                          "vertex, size ~ p_{T} there; line from the production direction, p_{T} > 1 GeV)",
                          nCross, nPrimaries));
    header.push_back(Form("%d primaries do not reach it (open marker at the production direction): decayed, stopped or looping in the "
                          "tracker, or along the beam line",
                          nNoCross));
    for (int k = 0; k < 2; ++k)
      header.push_back(sysLine(Form("%s at the boundary (%d of %d primaries)", sysName[k], S[k].n, S[k].nAll), S[k], kSysColor[k]));
    if (both)
      header.push_back(Form("net b+#bar{b} at the boundary: |#vec{p}_{T}(b) + #vec{p}_{T}(#bar{b})| = %.2f GeV,  #Sigma|p_{T}| = %.2f GeV,  #Delta#phi = %.2f",
                            net.pt(), S[0].p4.pt() + S[1].p4.pt(), std::fabs(reco::deltaPhi(S[0].p4.phi(), S[1].p4.phi()))));
    for (int k = 0; k < 2; ++k)
      if (S[k].nAll)
        P.legEntry(kSysColor[k], 20, Form("%s (%d at the boundary)", sysName[k], S[k].n));
    if (both)
      P.legEntry(kMagenta, 29, "b + #bar{b} (vector sum)");
    for (int k = 0; k < NKIND - 1; ++k)
      if (nKind[k])
        P.legEntry(kKindColor[k], kKindMarker[k], Form("%s (%d)", kKindName[k], nKind[k]));
    if (nNoCross)
      P.legEntry(kGray + 2, 24, Form("not reaching the boundary (%d)", nNoCross));
    std::cout << "  page 2 (calo surface): nReaching=" << nCross << " nNotReaching=" << nNoCross;
    for (int k = 0; k < 2; ++k)
      std::cout << " | " << (k ? "bbar" : "b") << " system at the boundary: n=" << S[k].n << "/" << S[k].nAll << " pt=" << S[k].p4.pt()
                << " eta=" << (S[k].n ? S[k].p4.eta() : 0.) << " phi=" << S[k].p4.phi();
    std::cout << " | net |sum pT|=" << net.pt() << "\n";
    if (savePDF_)
      P.finish(ev, header, 1);
  }

  // ------------------------------------------------------------------ page 3: secondaries
  if (maxPage_ >= 3) {
    Page P(this, 3);
    Sys S[2];
    int nProc[NPROC] = {}, nGroup[5] = {};
    for (size_t i = 0; i < nt; ++i)  // primaries in the background
      if (tracks[i].isPrimary())
        P.groupPoint(kGray, 1, 1., tracks[i].momentum().eta(), tracks[i].momentum().phi());
    for (int pass = 0; pass < 2; ++pass)
      for (size_t i = 0; i < nt; ++i) {
        const SimTrack& t = tracks[i];
        if (t.isPrimary() || (pass == 0) != (sys[i] < 0))
          continue;
        int p = proc[i], k = sys[i];
        double pt = t.momentum().pt(), eta = t.momentum().eta(), phi = t.momentum().phi();
        ++nProc[p];
        ++nGroup[kProcGroup[p]];
        hCat_[3]->Fill(p);
        hEtaPhi_[3]->Fill(eta, phi, pt);
        int color = k >= 0 ? kSysColor[k] : kProcColor[p];
        if (k >= 0) {
          ++S[k].n;
          S[k].p4 += t.momentum();
          ++S[k].nProc[kProcGroup[p]];
        }
        double etaD = P.point(color, kProcMarker[p], markerSize(pt), eta, phi);
        if (pt >= labelPtMin_)
          P.label(etaD + 0.05, phi + 0.06, k >= 0 ? Form("%s %.1f", pdgName(t.type()).c_str(), pt) : Form("%.1f", pt), 0.016, color);
      }
    hN_[3]->Fill(nSecondaries);
    if (S[0].n)
      hPtB_[3]->Fill(S[0].p4.pt());
    if (S[1].n)
      hPtBbar_[3]->Fill(S[1].p4.pt());
    std::vector<std::string> header;
    header.push_back(Form("%zu secondaries kept as SimTracks: %d from decays, %d electromagnetic, %d hadronic, %d other",
                          nSecondaries, nGroup[1], nGroup[2], nGroup[3], nGroup[4]));
    header.push_back("marker = creator process of the production vertex (Geant4 process sub-type of the SimVertex); primaries as grey dots");
    for (int k = 0; k < 2; ++k)
      header.push_back(Form("%s: %d secondaries (%d decay, %d EM, %d hadronic), #Sigma|p_{T}| = %.2f GeV", sysName[k], S[k].n,
                            S[k].nProc[1], S[k].nProc[2], S[k].nProc[3], S[k].p4.pt()));
    for (int k = 0; k < 2; ++k)
      if (S[k].n)
        P.legEntry(kSysColor[k], 20, Form("%s (%d)", sysName[k], S[k].n));
    for (int p = 1; p < NPROC; ++p)
      if (nProc[p])
        P.legEntry(kProcColor[p], kProcMarker[p], Form("%s (%d)", kProcName[p], nProc[p]));
    std::cout << "  page 3 (secondaries): nSecondaries=" << nSecondaries << " decay=" << nGroup[1] << " EM=" << nGroup[2]
              << " hadronic=" << nGroup[3] << " other=" << nGroup[4] << " by process:";
    for (int p = 1; p < NPROC; ++p)
      if (nProc[p])
        std::cout << " " << kProcName[p] << "=" << nProc[p];
    for (int k = 0; k < 2; ++k)
      std::cout << " | " << (k ? "bbar" : "b") << " system: n=" << S[k].n << " (decay " << S[k].nProc[1] << ", EM " << S[k].nProc[2]
                << ", had " << S[k].nProc[3] << ")";
    std::cout << "\n";
    if (savePDF_)
      P.finish(ev, header, 1);
  }

  // ------------------------------------------------------------------ page 4: tracker and muon hits
  if (maxPage_ >= 4) {
    Page P(this, 4);
    const GlobalTrackingGeometry* geom = nullptr;
    try {
      geom = &es.getData(trackingGeomTok_);
    } catch (cms::Exception const& e) {
      std::cout << "  page 4: no GlobalTrackingGeometry (" << e.what() << ")\n";
    }
    int nSub[NSUB] = {}, nSys[2] = {0, 0}, nNoGeom = 0, nNoTrack = 0;
    size_t nHits = 0;
    double sumELoss = 0.;
    struct Hit {
      double eta, phi;
      int sub, sys;
    };
    std::vector<Hit> hits;
    for (auto const& th : trackerHitsTok_) {
      edm::Handle<edm::PSimHitContainer> h;
      ev.getByToken(th.second, h);
      if (!h.isValid())
        continue;
      for (auto const& hit : *h) {
        ++nHits;
        sumELoss += hit.energyLoss();
        DetId id(hit.detUnitId());
        const GeomDet* det = nullptr;
        if (geom) {
          try {
            det = geom->idToDet(id);
          } catch (cms::Exception const&) {
          }
        }
        if (!det) {
          ++nNoGeom;
          continue;
        }
        GlobalPoint gp = det->surface().toGlobal(hit.localPosition());
        double eta = dirEta(gp.x(), gp.y(), gp.z()), phi = dirPhi(gp.x(), gp.y(), gp.z());
        int sub = subOf(id), k = sysOfTrackId(hit.trackId());
        if (trackIdx.find(hit.trackId()) == trackIdx.end())
          ++nNoTrack;
        ++nSub[sub];
        hCat_[4]->Fill(sub);
        hEtaPhi_[4]->Fill(eta, phi);
        if (k >= 0)
          ++nSys[k];
        hits.push_back({eta, phi, sub, k});
      }
    }
    for (int pass = 0; pass < 2; ++pass)
      for (auto const& x : hits)
        if ((pass == 0) == (x.sys < 0))
          P.groupPoint(x.sys >= 0 ? kSysColor[x.sys] : kSubColor[x.sub], 20, 0.5, x.eta, x.phi);
    hN_[4]->Fill(nHits);
    int nTracker = 0, nMuon = 0;
    for (int s = PXB; s <= TEC; ++s)
      nTracker += nSub[s];
    for (int s = DT; s <= GEM; ++s)
      nMuon += nSub[s];
    std::vector<std::string> header;
    header.push_back(Form("%zu PSimHits: %d tracker, %d muon chambers; #Sigma energy loss = %.2f MeV; %d hits of tracks not kept as SimTracks%s",
                          nHits, nTracker, nMuon, sumELoss * 1000., nNoTrack,
                          nNoGeom ? Form(", %d without geometry (not drawn)", nNoGeom) : ""));
    header.push_back("#eta, #phi of the hit position seen from the primary vertex");
    for (int k = 0; k < 2; ++k)
      header.push_back(Form("%s: %d hits (tracks descending from its primaries)", sysName[k], nSys[k]));
    for (int k = 0; k < 2; ++k)
      if (nSys[k])
        P.legEntry(kSysColor[k], 20, Form("%s hits (%d)", sysName[k], nSys[k]));
    for (int s = 0; s < NSUB; ++s)
      if (nSub[s])
        P.legEntry(kSubColor[s], 20, Form("%s (%d)", kSubName[s], nSub[s]));
    std::cout << "  page 4 (tracker/muon hits): nHits=" << nHits << " tracker=" << nTracker << " muon=" << nMuon << " sumELoss[GeV]=" << sumELoss
              << " noGeometry=" << nNoGeom << " noSimTrack=" << nNoTrack << " bHits=" << nSys[0] << " bbarHits=" << nSys[1] << " by subdetector:";
    for (int s = 0; s < NSUB; ++s)
      if (nSub[s])
        std::cout << " " << kSubName[s] << "=" << nSub[s];
    std::cout << "\n";
    if (savePDF_)
      P.finish(ev, header, 2);
  }

  // ------------------------------------------------------------------ page 5: calorimeter hits per cell
  if (maxPage_ >= 5) {
    Page P(this, 5);
    const CaloGeometry* geom = nullptr;
    const HcalDDDRecConstants* hcons = nullptr;
    try {
      geom = &es.getData(caloGeomTok_);
      if (hcalTestNumbering_)
        hcons = &es.getData(hcalRecTok_);
    } catch (cms::Exception const& e) {
      std::cout << "  page 5: geometry not available (" << e.what() << ")\n";
    }
    struct Cell {
      int sub;
      double e = 0., eSys[2] = {0., 0.}, eta = 0., phi = 0.;
    };
    std::map<uint32_t, Cell> cells;
    size_t nHits = 0;
    int nNoGeom = 0;
    double eSub[NSUB] = {};
    for (auto const& th : caloHitsTok_) {
      edm::Handle<edm::PCaloHitContainer> h;
      ev.getByToken(th.second, h);
      if (!h.isValid())
        continue;
      bool hcal = th.first.instance().find("Hcal") != std::string::npos;
      for (auto const& hit : *h) {
        ++nHits;
        DetId id(hit.id());
        if (hcal && hcalTestNumbering_ && hcons)
          id = HcalHitRelabeller::relabel(hit.id(), hcons);
        if (id.rawId() == 0) {
          ++nNoGeom;
          continue;
        }
        auto it = cells.find(id.rawId());
        if (it == cells.end()) {
          const CaloSubdetectorGeometry* sg = geom ? geom->getSubdetectorGeometry(id) : nullptr;
          auto cell = sg ? sg->getGeometry(id) : CaloSubdetectorGeometry::CellMayOwnPtr();
          if (!cell.get()) {
            ++nNoGeom;
            continue;
          }
          Cell c;
          c.sub = subOf(id);
          const GlobalPoint& gp = cell->getPosition();
          c.eta = dirEta(gp.x(), gp.y(), gp.z());
          c.phi = dirPhi(gp.x(), gp.y(), gp.z());
          it = cells.emplace(id.rawId(), c).first;
        }
        it->second.e += hit.energy();
        int k = sysOfTrackId(hit.geantTrackId());
        if (k >= 0)
          it->second.eSys[k] += hit.energy();
      }
    }
    Sys S[2];
    int nSub[NSUB] = {};
    for (int pass = 0; pass < 2; ++pass)
      for (auto const& kv : cells) {
        const Cell& c = kv.second;
        int k = c.eSys[0] > 0.5 * c.e ? 0 : (c.eSys[1] > 0.5 * c.e ? 1 : -1);
        if ((pass == 0) != (k < 0))
          continue;
        P.groupPoint(k >= 0 ? kSysColor[k] : kSubColor[c.sub], 20, cellSize(c.e), c.eta, c.phi);
      }
    for (auto const& kv : cells) {
      const Cell& c = kv.second;
      ++nSub[c.sub];
      eSub[c.sub] += c.e;
      hCat_[5]->Fill(c.sub, c.e);
      hEtaPhi_[5]->Fill(c.eta, c.phi, c.e);
      bool ecal = c.sub == EB || c.sub == EE || c.sub == ES || c.sub == HGCEE,
           hcal = c.sub == HB || c.sub == HE || c.sub == HO || c.sub == HGCHSI || c.sub == HGCHSC;
      for (int k = 0; k < 2; ++k) {
        if (c.eSys[k] <= 0.)
          continue;
        ++S[k].n;
        if (ecal)
          S[k].e[0] += c.eSys[k];
        else if (hcal)
          S[k].e[1] += c.eSys[k];
        else if (c.sub == HF)
          S[k].e[2] += c.eSys[k];
      }
    }
    hN_[5]->Fill(cells.size());
    hEcalB_->Fill(S[0].e[0]);
    hEcalBbar_->Fill(S[1].e[0]);
    hHcalB_->Fill(S[0].e[1]);
    hHcalBbar_->Fill(S[1].e[1]);
    std::vector<std::string> header;
    bool hgcal = nSub[HGCEE] + nSub[HGCHSI] + nSub[HGCHSC] > 0;
    if (hgcal)  // Phase2: no EE, ES, HE
      header.push_back(Form("%zu PCaloHits in %zu cells (size ~ cell energy): EB %.1f GeV;  HB %.2f, HO %.3f GeV (visible);  HF %.0f (as stored);  HGCal EE %.3f, HSi %.3f, HSc %.3f GeV%s",
                            nHits, cells.size(), eSub[EB], eSub[HB], eSub[HO], eSub[HF], eSub[HGCEE], eSub[HGCHSI], eSub[HGCHSC],
                            nNoGeom ? Form("; %d hits without geometry", nNoGeom) : ""));
    else
      header.push_back(Form("%zu PCaloHits in %zu cells (size ~ cell energy): EB %.1f, EE %.1f, ES %.2f GeV;  HB %.2f, HE %.2f, HO %.3f GeV (visible);  HF %.0f (as stored)%s",
                            nHits, cells.size(), eSub[EB], eSub[EE], eSub[ES], eSub[HB], eSub[HE], eSub[HO], eSub[HF],
                            nNoGeom ? Form("; %d hits without geometry", nNoGeom) : ""));
    header.push_back(hgcal ? "#eta, #phi of the cell centre seen from the primary vertex; HCAL and HGCal energies are the visible active-layer energies, HF as stored (shower library)"
                           : "#eta, #phi of the cell centre seen from the primary vertex; HCAL energies are the visible scintillator energies, HF as stored (shower library)");
    for (int k = 0; k < 2; ++k)
      header.push_back(Form("%s: E(ECAL) = %.2f GeV, E(HCAL, visible) = %.3f GeV, HF %.0f in %d cells (hits of tracks descending from its primaries)",
                            sysName[k], S[k].e[0], S[k].e[1], S[k].e[2], S[k].n));
    for (int k = 0; k < 2; ++k)
      if (S[k].n)
        P.legEntry(kSysColor[k], 20, Form("%s cells (%d)", sysName[k], S[k].n));
    for (int s = EB; s < OTHER_SUB; ++s)
      if (nSub[s])
        P.legEntry(kSubColor[s], 20, Form("%s (%d cells)", kSubName[s], nSub[s]));
    if (nSub[OTHER_SUB])
      P.legEntry(kSubColor[OTHER_SUB], 20, Form("other (%d cells)", nSub[OTHER_SUB]));
    std::cout << "  page 5 (calo hits): nHits=" << nHits << " nCells=" << cells.size() << " noGeometry=" << nNoGeom << " E by subdetector:";
    for (int s = EB; s < NSUB; ++s)
      if (nSub[s])
        std::cout << " " << kSubName[s] << "=" << eSub[s] << "(" << nSub[s] << ")";
    for (int k = 0; k < 2; ++k)
      std::cout << " | " << (k ? "bbar" : "b") << " system: E_ECAL=" << S[k].e[0] << " E_HCAL=" << S[k].e[1] << " E_HF=" << S[k].e[2]
                << " nCells=" << S[k].n;
    std::cout << "\n";
    if (savePDF_)
      P.finish(ev, header, 3);
  }
  std::cout.flush();
}

// ----------------------------------------------------------------------------- drawing
SimAnalyzer::Page::Page(SimAnalyzer* an, int p) : a(an), page(p) {
  if (!a->savePDF_)
    return;
  TCanvas* c = a->canvas_.get();
  c->Clear();
  c->SetLeftMargin(0.06);
  c->SetRightMargin(0.21);
  c->SetTopMargin(0.19);
  c->SetBottomMargin(0.08);
  c->SetGrid();
  auto frame = std::make_unique<TH2D>(Form("frame%d", page), ";#eta;#phi", 10, -a->etaMax_, a->etaMax_, 10, -M_PI, M_PI);
  frame->GetXaxis()->SetTitleOffset(0.9);
  frame->GetYaxis()->SetTitleOffset(0.6);
  frame->Draw("axis");
  keep.push_back(std::move(frame));
  leg = std::make_unique<TLegend>(0.80, 0.44, 0.995, 0.80);
  leg->SetBorderSize(0);
  leg->SetTextSize(0.018);
}

double SimAnalyzer::Page::clamp(double eta, bool& clamped) {
  clamped = std::fabs(eta) > a->etaMax_;
  if (!clamped)
    return eta;
  ++nClamped;
  return eta > 0 ? a->etaMax_ - 0.05 : -a->etaMax_ + 0.05;
}

double SimAnalyzer::Page::point(int color, int style, double size, double eta, double phi) {
  bool clamped;
  eta = clamp(eta, clamped);
  if (!a->savePDF_)
    return eta;
  auto g = std::make_unique<TGraph>(1);
  g->SetPoint(0, eta, phi);
  g->SetMarkerColor(color);
  g->SetMarkerStyle(clamped ? 5 : style);
  g->SetMarkerSize(size * (clamped ? 1.3 : 1.));
  g->Draw("P same");
  keep.push_back(std::move(g));
  return eta;
}

void SimAnalyzer::Page::groupPoint(int color, int style, double size, double eta, double phi) {
  bool clamped;
  eta = clamp(eta, clamped);
  if (!a->savePDF_)
    return;
  if (clamped) {
    style = 5;
    size *= 1.3;
  }
  std::string key = Form("%d_%d_%.2f", color, style, size);
  auto it = groups.find(key);
  if (it == groups.end()) {
    auto g = std::make_unique<TGraph>();
    g->SetMarkerColor(color);
    g->SetMarkerStyle(style);
    g->SetMarkerSize(size);
    g->Draw("P same");  // painted at Print() with all points added by then
    it = groups.emplace(key, g.get()).first;
    keep.push_back(std::move(g));
  }
  it->second->SetPoint(it->second->GetN(), eta, phi);
}

void SimAnalyzer::Page::line(int color, double eta0, double phi0, double eta1, double phi1) {
  if (!a->savePDF_ || std::fabs(phi1 - phi0) > M_PI)
    return;
  auto l = std::make_unique<TLine>(eta0, phi0, eta1, phi1);
  l->SetLineColorAlpha(color, 0.45);
  l->SetLineWidth(1);
  l->Draw();
  keep.push_back(std::move(l));
}

void SimAnalyzer::Page::label(double x, double y, const std::string& s, double size, int color) {
  if (!a->savePDF_)
    return;
  auto t = std::make_unique<TLatex>(x, y, s.c_str());
  t->SetTextSize(size);
  t->SetTextColor(color);
  t->Draw();
  keep.push_back(std::move(t));
}

void SimAnalyzer::Page::text(double x, double y, const std::string& s, double size, int color, int align) {
  if (!a->savePDF_)
    return;
  auto t = std::make_unique<TLatex>(x, y, s.c_str());
  t->SetNDC();
  t->SetTextSize(size);
  t->SetTextColor(color);
  t->SetTextAlign(align);
  t->Draw();
  keep.push_back(std::move(t));
}

void SimAnalyzer::Page::legEntry(int color, int style, const std::string& entry) {
  if (!a->savePDF_)
    return;
  auto m = std::make_unique<TMarker>(0, 0, style);
  m->SetMarkerColor(color);
  m->SetMarkerSize(1.4);
  leg->AddEntry(m.get(), entry.c_str(), "p");
  legMarkers.push_back(std::move(m));
}

// header lines (first in grey, then the b / bbar / net lines), sample label, legend, marker scale
// (1 pT, 2 none, 3 cell energy) and the SIM stage list; then the page is printed
void SimAnalyzer::Page::finish(const edm::Event& ev, const std::vector<std::string>& header, int scaleKind) {
  if (!a->savePDF_)
    return;
  TCanvas* c = a->canvas_.get();
  text(0.06, 0.957, kPageTitle[page], 0.03);
  text(0.79, 0.955, Form("run %u  lumi %u  event %llu", ev.id().run(), ev.luminosityBlock(), ev.id().event()), 0.022, kBlack, 31);
  if (!a->sampleLabel_.empty()) {
    size_t start = 0;
    for (int k = 0; start <= a->sampleLabel_.size() && k < 5; ++k) {
      size_t bar = a->sampleLabel_.find('|', start);
      std::string line = a->sampleLabel_.substr(start, bar == std::string::npos ? std::string::npos : bar - start);
      text(0.80, 0.925 - 0.026 * k, line, 0.018, k == 0 ? kBlack : kGray + 3);
      if (bar == std::string::npos)
        break;
      start = bar + 1;
    }
  }
  for (size_t k = 0; k < header.size() && k < 5; ++k) {
    std::string line = header[k];
    if (k == 0 && nClamped)
      line += Form("; %d beyond |#eta| = %.0f (x)", nClamped, a->etaMax_);
    int color = kGray + 3;
    if (line.rfind("b system", 0) == 0)
      color = kRed;
    else if (line.rfind("#bar{b} system", 0) == 0)
      color = kBlue;
    else if (line.rfind("net b", 0) == 0)
      color = kMagenta + 2;
    text(0.06, 0.925 - 0.023 * k, line, color == kGray + 3 ? 0.016 : 0.019, color);
  }
  leg->Draw();
  if (scaleKind != 2) {
    text(0.80, 0.41, "marker size:", 0.02);
    const double scale[2][3] = {{1., 10., 100.}, {0.01, 1., 100.}};
    for (int k = 0; k < 3; ++k) {
      double v = scale[scaleKind == 1 ? 0 : 1][k];
      auto m = std::make_unique<TMarker>(0.815, 0.37 - 0.035 * k, 20);
      m->SetNDC();
      m->SetMarkerColor(kGray + 2);
      m->SetMarkerSize(scaleKind == 1 ? markerSize(v) : cellSize(v));
      m->Draw();
      keep.push_back(std::move(m));
      text(0.835, 0.365 - 0.035 * k, scaleKind == 1 ? Form("p_{T} = %.0f GeV", v) : Form("E = %g GeV", v), 0.02);
    }
  }
  text(0.80, 0.25, "SIM stage of this sample:", 0.02);
  for (int k = 0; k < kNPages; ++k)
    text(0.81, 0.22 - 0.03 * k, kStageName[k], 0.018, k + 1 == a->simStage_ ? kRed : (k + 1 < a->simStage_ || a->simStage_ <= 0 ? kGray + 2 : kGray));
  c->Update();
  if (a->pdfFile_.empty()) {
    c->Print(Form("%s_page%d_%s_run%u_event%llu.pdf", a->outputPrefix_.c_str(), page, kPageName[page], ev.id().run(), ev.id().event()));
    return;
  }
  if (!a->pdfOpen_) {
    if (a->openPdf_)
      c->Print((a->pdfFile_ + "[").c_str());
    a->pdfOpen_ = true;
  }
  c->Print(a->pdfFile_.c_str());
}

void SimAnalyzer::endJob() {
  if (pdfOpen_ && closePdf_)
    canvas_->Print((pdfFile_ + "]").c_str());
}

DEFINE_FWK_MODULE(SimAnalyzer);
