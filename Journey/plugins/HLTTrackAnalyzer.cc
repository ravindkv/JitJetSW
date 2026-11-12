// HLTTrackAnalyzer: step 3 of the High Level Trigger for one event (chapter 13, HLT, of the JitJet book): tracking
// and vertices.
//
//   the pixel and strip raw data are unpacked into clusters (hltSiPixelClusters, hltSiStripRawToClustersFacility);
//   pixel tracks are built from four (or three) pixel hits with the cellular automaton (hltPixelTracks) and clustered
//   in z into pixel vertices (hltPixelVertices); the full tracks are then built in two iterations seeded by the pixel
//   tracks (iteration 0, hltIter0PFlowTrackSelectionHighPurity, with mkFit) and by pixel pairs (doublet recovery,
//   hltDoubletRecoveryPFlowTrackSelectionHighPurity) and merged (hltMergedTracks); the vertices of particle flow are
//   fitted from them (hltVerticesPF)
// Pages (one PDF per event, one PNG per page with savePNG):
//   etaphi    the merged tracks in eta-phi, those of the vertex of the signal in black, with the generator jets
//   vertices  the vertices along the beam (z), pixel and PF, with the simulated collisions of the bunch crossing
//             (PileupSummaryInfo) and the simulated vertex of the signal (genParticles:xyz0)
// The log has the clusters, the tracks of each iteration, every vertex with the simulated collision nearest to it,
// and the tracks inside the two generator jets (read by test/common/hltLedger.py).
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "DataFormats/Common/interface/DetSetVectorNew.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "DataFormats/Math/interface/Point3D.h"
#include "DataFormats/SiPixelCluster/interface/SiPixelCluster.h"
#include "DataFormats/SiStripCluster/interface/SiStripCluster.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h"

#include "HLTDisplay.h"

namespace {
  using namespace hltdisplay;

  struct VertexInfo {
    double z = 0, ndof = 0, sumPt2 = 0, sumPt = 0;
    int nTracks = 0, nNear = 0;
    int sim = -2;  // -1: the signal; k >= 0: the k-th in-time pileup collision
    double dzSim = 99;
  };
}  // namespace

class HLTTrackAnalyzer : public edm::one::EDAnalyzer<> {
public:
  explicit HLTTrackAnalyzer(const edm::ParameterSet&);
  void analyze(const edm::Event&, const edm::EventSetup&) override;

private:
  edm::EDGetTokenT<edmNew::DetSetVector<SiPixelCluster>> pixelClusters_;
  edm::EDGetTokenT<edmNew::DetSetVector<SiStripCluster>> stripClusters_;
  edm::EDGetTokenT<reco::TrackCollection> pixelTracks_, iter0Tracks_, doubletTracks_, mergedTracks_;
  edm::EDGetTokenT<reco::VertexCollection> pixelVertices_, trimmedVertices_, pfVertices_;
  edm::EDGetTokenT<std::vector<PileupSummaryInfo>> pileup_;
  edm::EDGetTokenT<math::XYZPointF> signalVertex_;
  edm::EDGetTokenT<std::vector<reco::GenJet>> genJets_;
  std::string pdfFile_, pngPrefix_, sampleLabel_;
  bool savePNG_;
  double jetPtMin_, dzMax_;
  int nEvents_ = 0;
  Page page_;
};

HLTTrackAnalyzer::HLTTrackAnalyzer(const edm::ParameterSet& ps)
    : pixelClusters_(consumes(ps.getParameter<edm::InputTag>("pixelClusters"))),
      stripClusters_(consumes(ps.getParameter<edm::InputTag>("stripClusters"))),
      pixelTracks_(consumes(ps.getParameter<edm::InputTag>("pixelTracks"))),
      iter0Tracks_(consumes(ps.getParameter<edm::InputTag>("iter0Tracks"))),
      doubletTracks_(consumes(ps.getParameter<edm::InputTag>("doubletTracks"))),
      mergedTracks_(consumes(ps.getParameter<edm::InputTag>("mergedTracks"))),
      pixelVertices_(consumes(ps.getParameter<edm::InputTag>("pixelVertices"))),
      trimmedVertices_(consumes(ps.getParameter<edm::InputTag>("trimmedPixelVertices"))),
      pfVertices_(consumes(ps.getParameter<edm::InputTag>("pfVertices"))),
      pileup_(consumes(ps.getParameter<edm::InputTag>("pileupInfo"))),
      signalVertex_(consumes(ps.getParameter<edm::InputTag>("signalVertex"))),
      genJets_(consumes(ps.getParameter<edm::InputTag>("genJets"))),
      pdfFile_(ps.getParameter<std::string>("pdfFile")),
      pngPrefix_(ps.getParameter<std::string>("pngPrefix")),
      sampleLabel_(ps.getParameter<std::string>("sampleLabel")),
      savePNG_(ps.getParameter<bool>("savePNG")),
      jetPtMin_(ps.getParameter<double>("jetPtMin")),
      dzMax_(ps.getParameter<double>("dzMax")),
      page_("hltTrack") {}

void HLTTrackAnalyzer::analyze(const edm::Event& ev, const edm::EventSetup&) {
  std::string tag = Form("_run%u_event%llu", ev.id().run(), ev.id().event());
  std::cout << "== HLTTrackAnalyzer run:lumi:event " << ev.id().run() << ":" << ev.luminosityBlock() << ":" << ev.id().event() << "\n";

  // ------------------------------------------------------------------ the clusters
  size_t nPix = 0, nStrip = 0, nPixMod = 0, nStripMod = 0;
  if (auto h = ev.getHandle(pixelClusters_); h.isValid()) {
    nPix = h->dataSize();
    nPixMod = h->size();
  }
  if (auto h = ev.getHandle(stripClusters_); h.isValid()) {
    nStrip = h->dataSize();
    nStripMod = h->size();
  }
  std::cout << "  clusters: pixel=" << nPix << " (modules " << nPixMod << ") strip=" << nStrip << " (modules " << nStripMod << ")\n";

  // ------------------------------------------------------------------ the tracks of every iteration
  auto summary = [](const char* name, const reco::TrackCollection& t) {
    double s = 0;
    int n1 = 0, n10 = 0;
    std::vector<int> hits;
    for (auto const& x : t) {
      s += x.pt();
      n1 += x.pt() > 1;
      n10 += x.pt() > 10;
    }
    std::cout << "  tracks " << name << ": n=" << t.size() << " pt>1=" << n1 << " pt>10=" << n10 << " sumPt=" << Form("%.1f", s) << "\n";
    return s;
  };
  const auto& pixT = ev.get(pixelTracks_);
  const auto& it0 = ev.get(iter0Tracks_);
  const auto& dbl = ev.get(doubletTracks_);
  const auto& trk = ev.get(mergedTracks_);
  summary("pixel", pixT);
  summary("iter0", it0);
  summary("doubletRecovery", dbl);
  double sumMerged = summary("merged", trk);
  {
    int hp = 0, nPixHits = 0, nStripHits = 0;
    for (auto const& t : trk) {
      hp += t.quality(reco::TrackBase::highPurity);
      nPixHits += t.hitPattern().numberOfValidPixelHits();
      nStripHits += t.hitPattern().numberOfValidStripHits();
    }
    std::cout << "  tracks merged: highPurity=" << hp << " meanPixelHits=" << Form("%.2f", trk.empty() ? 0. : double(nPixHits) / trk.size())
              << " meanStripHits=" << Form("%.2f", trk.empty() ? 0. : double(nStripHits) / trk.size()) << "\n";
  }

  // ------------------------------------------------------------------ the truth: where the collisions were
  const auto signal = ev.get(signalVertex_);
  std::vector<float> puZ;
  int nPU = 0;
  double trueNPU = 0;
  for (auto const& p : ev.get(pileup_))
    if (p.getBunchCrossing() == 0) {
      puZ = p.getPU_zpositions();
      nPU = p.getPU_NumInteractions();
      trueNPU = p.getTrueNumInteractions();
    }
  std::cout << "  truth: signal vertex x=" << Form("%.4f", signal.x()) << " y=" << Form("%.4f", signal.y()) << " z=" << Form("%.4f", signal.z())
            << " | in-time pileup collisions=" << nPU << " (mean " << trueNPU << ")\n";
  {
    std::vector<float> s = puZ;
    std::sort(s.begin(), s.end());
    int within1 = 0;
    for (float z : s)
      within1 += std::abs(z - signal.z()) < 0.1;
    std::cout << "  truth: pileup collisions within 1 mm of the signal in z: " << within1;
    if (!s.empty())
      std::cout << " | z range " << Form("%.2f", s.front()) << " .. " << Form("%.2f", s.back());
    std::cout << "\n";
  }
  auto nearestSim = [&](double z, double& dz) {
    int best = -1;
    dz = std::abs(z - signal.z());
    for (size_t k = 0; k < puZ.size(); ++k)
      if (double d = std::abs(z - puZ[k]); d < dz) {
        dz = d;
        best = k;
      }
    return best;
  };

  // ------------------------------------------------------------------ the vertices
  auto vertexInfo = [&](const reco::VertexCollection& vs) {
    std::vector<VertexInfo> out;
    for (auto const& v : vs) {
      if (v.isFake())
        continue;
      VertexInfo vi;
      vi.z = v.z();
      vi.ndof = v.ndof();
      vi.nTracks = v.tracksSize();
      // the tracks near the vertex in z (the refs of the vertex may point to a collection that is not in the file)
      for (auto const& t : trk)
        if (std::abs(t.dz(v.position())) < dzMax_) {
          ++vi.nNear;
          vi.sumPt += t.pt();
          vi.sumPt2 += t.pt() * t.pt();
        }
      vi.sim = nearestSim(v.z(), vi.dzSim);
      out.push_back(vi);
    }
    return out;
  };
  const auto pixV = vertexInfo(ev.get(pixelVertices_));
  const auto pfV = vertexInfo(ev.get(pfVertices_));
  // the pixel vertices kept to seed the full tracking: sum pT^2 at least fractionSumPt2 of the first one
  const auto trimV = vertexInfo(ev.get(trimmedVertices_));
  std::cout << "  vertices trimmed (seeds of iteration 0): n=" << trimV.size() << " |";
  for (auto const& v : trimV)
    std::cout << " z=" << Form("%.3f", v.z) << "(" << (v.sim == -1 ? std::string("signal") : Form("pu%d", v.sim)) << ")";
  std::cout << "\n";
  int signalRankPix = -1, signalRankPF = -1;
  for (auto const& [name, vs, rank] : std::vector<std::tuple<const char*, const std::vector<VertexInfo>*, int*>>{{"pixel", &pixV, &signalRankPix}, {"PF", &pfV, &signalRankPF}}) {
    std::cout << "  vertices " << name << ": n=" << vs->size() << "\n";
    for (size_t i = 0; i < vs->size(); ++i) {
      auto const& v = (*vs)[i];
      if (v.sim == -1 && v.dzSim < 0.1 && *rank < 0)
        *rank = i;
      if (i < 15 || v.sim == -1)
        std::cout << "  vertex " << name << " " << i << ": z=" << Form("%.4f", v.z) << " ndof=" << Form("%.1f", v.ndof) << " tracks=" << v.nTracks
                  << " near=" << v.nNear << " sumPt=" << Form("%.1f", v.sumPt) << " sumPt2=" << Form("%.1f", v.sumPt2) << " sim="
                  << (v.sim == -1 ? std::string("signal") : Form("pu%d", v.sim)) << " dzSim=" << Form("%.4f", v.dzSim) << "\n";
    }
  }
  std::cout << "  signal vertex rank: pixel=" << signalRankPix << " PF=" << signalRankPF << " (0 = the first, the primary vertex)\n";
  // the reconstructed vertex closest to the simulated signal vertex
  const VertexInfo* sigV = nullptr;
  for (auto const& v : pfV)
    if (!sigV || std::abs(v.z - signal.z()) < std::abs(sigV->z - signal.z()))
      sigV = &v;
  double zSig = sigV ? sigV->z : signal.z();

  // ------------------------------------------------------------------ the tracks of the two signal jets
  std::vector<reco::GenJet> gj;
  for (auto const& j : ev.get(genJets_))
    if (j.pt() > jetPtMin_)
      gj.push_back(j);
  for (size_t ig = 0; ig < gj.size(); ++ig) {
    const auto& g = gj[ig];
    int n = 0, nSig = 0;
    double s = 0, sSig = 0, charged = 0;
    for (auto const& c : g.getGenConstituents())
      if (c->charge() != 0)
        charged += c->pt();
    for (auto const& t : trk)
      if (dR(t.eta(), t.phi(), g.eta(), g.phi()) < 0.4) {
        ++n;
        s += t.pt();
        if (std::abs(t.vz() - zSig) < dzMax_) {
          ++nSig;
          sSig += t.pt();
        }
      }
    std::cout << "  jet tracks: gen jet " << ig + 1 << " pt=" << Form("%.1f", g.pt()) << " eta=" << Form("%.2f", g.eta()) << " chargedGenPt=" << Form("%.1f", charged)
              << " | tracks dR<0.4: n=" << n << " sumPt=" << Form("%.1f", s) << " | from the signal vertex (|dz|<" << dzMax_ << " cm): n=" << nSig
              << " sumPt=" << Form("%.1f", sSig) << "\n";
  }

  // ------------------------------------------------------------------ page 1: the tracks in eta-phi
  page_.openPdf(pdfName(pdfFile_, ev.id().run(), ev.id().event(), nEvents_++));
  page_.begin();
  frame(page_, "hltTrackFrame", Form("step 3: the HLT tracks (hltMergedTracks), run %u event %llu", ev.id().run(), ev.id().event()));
  detectorLines(page_);
  genJets(page_, gj, jetPtMin_);
  int nSigTracks = 0;
  double sumSig = 0;
  for (auto const& t : trk) {
    bool fromSignal = std::abs(t.vz() - zSig) < dzMax_;
    nSigTracks += fromSignal;
    if (fromSignal)
      sumSig += t.pt();
    double size = 0.35 + 0.6 * std::log10(1. + t.pt());
    page_.marker(t.eta(), t.phi(), 20, fromSignal ? kBlack : kGray + 1, fromSignal ? size * 1.2 : size);
    if (fromSignal && t.pt() > 9)
      page_.note(t.eta() + 0.08, t.phi() + 0.1, Form("%.0f", t.pt()), kBlack, 0.019);
  }
  std::cout << "  tracks from the signal vertex (|dz| < " << dzMax_ << " cm of z=" << Form("%.3f", zSig) << "): n=" << nSigTracks << " sumPt=" << Form("%.1f", sumSig) << "\n";
  page_.label(sampleLabel_);
  page_.text("hits", 0.025, kBlue + 1);
  page_.text(Form("   %zu pixel clusters, %zu strip clusters", nPix, nStrip), 0.020);
  page_.skip();
  page_.text("tracks", 0.025, kBlue + 1);
  page_.text(Form("   pixel tracks: %zu", pixT.size()), 0.020);
  page_.text(Form("   iteration 0 (from pixel tracks): %zu", it0.size()), 0.020);
  page_.text(Form("   doublet recovery: %zu", dbl.size()), 0.020);
  page_.text(Form("   merged: %zu, #Sigmap_{T} = %.0f GeV", trk.size(), sumMerged), 0.020);
  page_.skip();
  page_.text("vertices", 0.025, kBlue + 1);
  page_.text(Form("   pixel: %zu; kept as seeds: %zu; PF: %zu", pixV.size(), trimV.size(), pfV.size()), 0.020);
  page_.text(Form("   simulated in time: %d + the signal", nPU), 0.020);
  page_.text(Form("   the signal vertex is PF vertex number %d", signalRankPF), 0.020, kGenColor);
  page_.skip();
  page_.text(Form("black: %d tracks of the signal vertex (%.0f GeV)", nSigTracks, sumSig), 0.019);
  page_.text("grey: the other tracks; size: log(1 + p_{T})", 0.019, kGray + 2);
  page_.text("dashed: the tracker ends at |#eta| = 2.5", 0.019, kAzure - 4);
  page_.print(savePNG_ ? pngPrefix_ + "_etaphi" + tag + ".png" : std::string());

  // ------------------------------------------------------------------ page 2: the vertices along the beam
  page_.begin();
  double zmin = -20, zmax = 20, ymax = 5;
  for (auto const& v : pfV)
    ymax = std::max(ymax, double(v.nNear));
  for (auto const& v : pixV)
    ymax = std::max(ymax, double(v.nTracks));
  ymax *= 1.15;
  TH2F* f = page_.keep(new TH2F("hltVertexFrame", Form("step 3: the vertices along the beam, run %u event %llu;z [cm];tracks", ev.id().run(), ev.id().event()), 10, zmin,
                                zmax, 10, -0.12 * ymax, ymax));
  f->SetDirectory(nullptr);
  f->SetStats(false);
  f->GetYaxis()->SetTitleOffset(0.9);
  f->Draw("AXIS");
  page_.line(zmin, 0, zmax, 0, kBlack, 1);
  for (float z : puZ)
    page_.line(z, -0.11 * ymax, z, -0.04 * ymax, kAzure + 1, 2);
  page_.marker(signal.z(), -0.075 * ymax, 29, kGenColor, 2.4);
  for (auto const& v : pfV)
    page_.line(v.z, 0, v.z, v.nNear, v.sim == -1 && v.dzSim < 0.1 ? kGenColor : kRed + 1, 3);
  for (auto const& v : pixV)
    page_.marker(v.z, v.nTracks, 24, kBlack, 0.9);
  for (auto const& v : trimV)
    page_.marker(v.z, v.nTracks, 20, kBlack, 1.1);
  page_.label(sampleLabel_);
  page_.text("along the beam line", 0.025, kBlue + 1);
  page_.text("blue ticks: the simulated in-time collisions", 0.019, kAzure + 1);
  page_.text("magenta star: the simulated signal vertex", 0.019, kGenColor);
  page_.text(Form("   z = %.2f cm", signal.z()), 0.019, kGenColor);
  page_.text(Form("red bars: %zu PF vertices, height = tracks", pfV.size()), 0.019, kRed + 1);
  page_.text(Form("   within %.1f mm in z", 10 * dzMax_), 0.019, kRed + 1);
  page_.text(Form("circles: %zu pixel vertices (pixel tracks)", pixV.size()), 0.019);
  page_.text(Form("   full: the %zu that seed the full tracking", trimV.size()), 0.019);
  page_.skip();
  page_.text("the first PF vertices", 0.025, kBlue + 1);
  for (size_t i = 0; i < pfV.size() && i < 8; ++i) {
    auto const& v = pfV[i];
    page_.text(Form("   %zu: z %.2f, %d tracks, %s", i, v.z, v.nNear, v.sim == -1 ? "the signal" : Form("pileup (#Deltaz %.2f)", v.dzSim)), 0.0185,
               v.sim == -1 ? kGenColor : kBlack);
  }
  page_.print(savePNG_ ? pngPrefix_ + "_vertices" + tag + ".png" : std::string());
  page_.closePdf();
}

DEFINE_FWK_MODULE(HLTTrackAnalyzer);
