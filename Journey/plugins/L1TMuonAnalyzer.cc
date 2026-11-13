// L1TMuonAnalyzer: the muon trigger of one event, step by step (chapter 12, L1T, of the JitJet book).
//
//   step 4  the muon trigger primitives: what a chamber sends upstairs every 25 ns. DT: a phi segment per
//           station with position and bending angle (simDtTriggerPrimitiveDigis); CSC: the anode pattern (ALCT) and
//           the cathode pattern (CLCT) of a chamber correlated into a stub, the LCT (simCscTriggerPrimitiveDigis);
//           RPC: the digis themselves; GEM: pads and pad clusters (simMuonGEMPadDigis, simMuonGEMPadDigiClusters)
//           the muon track finders: BMTF (Kalman, simKBmtfDigis), OMTF (simOmtfDigis), EMTF (simEmtfDigis) link the
//           stubs of several stations into a track and assign its pT; the EMTF emulator stores the stubs it used
//           (EMTFHit, with their position) and its tracks (EMTFTrack)
//   step 5  the global muon trigger (simGmtStage2Digis): the candidates of the 36 track-finder processors sorted,
//           the duplicates found by two neighbouring processors removed, the best 8 sent to the global trigger
// Pages (one PDF per event, one PNG per page with savePNG): eta-phi and r-z of the stubs, the track-finder
// candidates and the final muons, with the generator muons and jets of the signal. The log repeats every number in
// key=value form (read by test/common/l1tLedger.py).
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "DataFormats/CSCDigi/interface/CSCALCTDigiCollection.h"
#include "DataFormats/CSCDigi/interface/CSCCLCTDigiCollection.h"
#include "DataFormats/CSCDigi/interface/CSCCorrelatedLCTDigiCollection.h"
#include "DataFormats/GEMDigi/interface/GEMPadDigiClusterCollection.h"
#include "DataFormats/GEMDigi/interface/GEMPadDigiCollection.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "DataFormats/L1DTTrackFinder/interface/L1MuDTChambPhContainer.h"
#include "DataFormats/L1DTTrackFinder/interface/L1MuDTChambThContainer.h"
#include "DataFormats/L1TMuon/interface/EMTFHit.h"
#include "DataFormats/L1TMuon/interface/EMTFTrack.h"
#include "DataFormats/L1TMuon/interface/L1TMuonSubsystems.h"
#include "DataFormats/L1TMuon/interface/RegionalMuonCand.h"
#include "DataFormats/L1Trigger/interface/BXVector.h"
#include "DataFormats/L1Trigger/interface/Muon.h"
#include "DataFormats/L1Trigger/interface/MuonShower.h"
#include "DataFormats/MuonDetId/interface/DTChamberId.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "Geometry/DTGeometry/interface/DTChamber.h"
#include "Geometry/DTGeometry/interface/DTGeometry.h"
#include "Geometry/Records/interface/MuonGeometryRecord.h"
#include "L1Trigger/L1TMuon/interface/MicroGMTConfiguration.h"

#include "L1TDisplay.h"

namespace {
  using namespace l1tdisplay;

  struct Stub {  // a trigger primitive with a position
    int kind;    // 0 DT, 1 CSC, 2 RPC, 3 GEM
    int bx;
    float eta, phi, r, z;
    int station;
  };
  struct Cand {  // a track-finder candidate
    std::string tf;
    int bx, processor, quality;
    float pt, eta, phi;
  };
  const char* kindName(int k) {
    static const char* n[] = {"DT", "CSC", "RPC", "GEM"};
    return n[k];
  }
  const int kindColor[] = {kBlue + 1, kGreen + 2, kOrange + 7, kViolet + 1};
  const int kindFull[] = {21, 22, 20, 33}, kindOpen[] = {25, 26, 24, 27};
  template <class C>
  int countDigis(const C& coll) {
    int n = 0;
    for (auto it = coll.begin(); it != coll.end(); ++it)
      n += (*it).second.second - (*it).second.first;
    return n;
  }
}  // namespace

class L1TMuonAnalyzer : public edm::one::EDAnalyzer<> {
public:
  explicit L1TMuonAnalyzer(const edm::ParameterSet&);
  void analyze(const edm::Event&, const edm::EventSetup&) override;

private:
  using CandColl = BXVector<l1t::RegionalMuonCand>;
  edm::EDGetTokenT<L1MuDTChambPhContainer> dtPh_;
  edm::EDGetTokenT<L1MuDTChambThContainer> dtTh_;
  edm::EDGetTokenT<CSCALCTDigiCollection> alct_;
  edm::EDGetTokenT<CSCCLCTDigiCollection> clct_;
  edm::EDGetTokenT<CSCCorrelatedLCTDigiCollection> lct_, lctMpc_;
  edm::EDGetTokenT<GEMPadDigiCollection> gemPads_;
  edm::EDGetTokenT<GEMPadDigiClusterCollection> gemClusters_;
  edm::EDGetTokenT<l1t::EMTFHitCollection> emtfHits_;
  edm::EDGetTokenT<l1t::EMTFTrackCollection> emtfTracks_;
  edm::EDGetTokenT<CandColl> bmtf_, kbmtf_, omtf_, emtf_;
  edm::EDGetTokenT<BXVector<l1t::Muon>> muons_;
  edm::EDGetTokenT<BXVector<l1t::MuonShower>> showers_;
  edm::EDGetTokenT<std::vector<reco::GenParticle>> genParticles_;
  edm::EDGetTokenT<std::vector<reco::GenJet>> genJets_;
  edm::ESGetToken<DTGeometry, MuonGeometryRecord> dtGeom_;
  std::string pdfFile_, pngPrefix_, sampleLabel_;
  bool savePNG_;
  double jetPtMin_, genMuonPtMin_;
  int nEvents_ = 0;
  Page page_;
};

L1TMuonAnalyzer::L1TMuonAnalyzer(const edm::ParameterSet& ps)
    : dtPh_(consumes(ps.getParameter<edm::InputTag>("dtTPs"))),
      dtTh_(consumes(ps.getParameter<edm::InputTag>("dtTPs"))),
      alct_(consumes(ps.getParameter<edm::InputTag>("cscTPs"))),
      clct_(consumes(ps.getParameter<edm::InputTag>("cscTPs"))),
      lct_(consumes(ps.getParameter<edm::InputTag>("cscTPs"))),
      lctMpc_(consumes(edm::InputTag(ps.getParameter<edm::InputTag>("cscTPs").label(), "MPCSORTED"))),
      gemPads_(consumes(ps.getParameter<edm::InputTag>("gemPads"))),
      gemClusters_(consumes(ps.getParameter<edm::InputTag>("gemPadClusters"))),
      emtfHits_(consumes(ps.getParameter<edm::InputTag>("emtf"))),
      emtfTracks_(consumes(ps.getParameter<edm::InputTag>("emtf"))),
      bmtf_(consumes(ps.getParameter<edm::InputTag>("bmtfCands"))),
      kbmtf_(consumes(ps.getParameter<edm::InputTag>("kbmtfCands"))),
      omtf_(consumes(ps.getParameter<edm::InputTag>("omtfCands"))),
      emtf_(consumes(ps.getParameter<edm::InputTag>("emtfCands"))),
      muons_(consumes(ps.getParameter<edm::InputTag>("muons"))),
      showers_(consumes(ps.getParameter<edm::InputTag>("muonShowers"))),
      genParticles_(consumes(ps.getParameter<edm::InputTag>("genParticles"))),
      genJets_(consumes(ps.getParameter<edm::InputTag>("genJets"))),
      dtGeom_(esConsumes()),
      pdfFile_(ps.getParameter<std::string>("pdfFile")),
      pngPrefix_(ps.getParameter<std::string>("pngPrefix")),
      sampleLabel_(ps.getParameter<std::string>("sampleLabel")),
      savePNG_(ps.getParameter<bool>("savePNG")),
      jetPtMin_(ps.getParameter<double>("jetPtMin")),
      genMuonPtMin_(ps.getParameter<double>("genMuonPtMin")),
      page_("l1tMuon") {}

void L1TMuonAnalyzer::analyze(const edm::Event& ev, const edm::EventSetup& es) {
  const DTGeometry& dtGeom = es.getData(dtGeom_);
  std::string tag = Form("_run%u_event%llu", ev.id().run(), ev.id().event());
  std::cout << "== L1TMuonAnalyzer run:lumi:event " << ev.id().run() << ":" << ev.luminosityBlock() << ":" << ev.id().event() << "\n";

  // ------------------------------------------------------------------ step 4a: the trigger primitives
  std::vector<Stub> stubs;
  std::map<int, int> nByKind, nByKindBx0;
  const auto& dtPh = *ev.get(dtPh_).getContainer();
  const auto& dtTh = *ev.get(dtTh_).getContainer();
  for (auto const& d : dtPh) {
    // sector 0 is centred on phi = 0; the position inside the sector is in units of 1/4096 rad
    double phi = wrapToPi(d.phi() / 4096. + d.scNum() * M_PI / 6.);
    const DTChamber* ch = dtGeom.chamber(DTChamberId(d.whNum(), d.stNum(), d.scNum() + 1));
    GlobalPoint p = ch ? ch->position() : GlobalPoint(0, 0, 0);
    stubs.push_back({0, d.bxNum(), float(p.eta()), float(phi), float(p.perp()), float(p.z()), d.stNum()});
    std::cout << "  DT TP: wheel=" << d.whNum() << " station=" << d.stNum() << " sector=" << d.scNum() << " bx=" << d.bxNum() << " phi=" << d.phi()
              << " phiB=" << d.phiB() << " quality=" << d.code() << " globalPhi=" << phi << "\n";
  }
  int nAlct = countDigis(ev.get(alct_)), nClct = countDigis(ev.get(clct_)), nLct = countDigis(ev.get(lct_)), nMpc = countDigis(ev.get(lctMpc_));
  int nPads = countDigis(ev.get(gemPads_)), nPadClusters = countDigis(ev.get(gemClusters_));
  // the stubs the EMTF received (CSC LCTs, RPC hits, GEM clusters), with their position; a stub of a chamber at a
  // sector edge is also given to the neighbouring sector (Neighbor = 1). A copy is counted once: it is skipped when the
  // same stub (same chamber, bunch crossing and position) also arrives in its own sector, and counted and drawn when it
  // arrives only as a copy (its own sector did not report it in the read-out window)
  const auto& hits = ev.get(emtfHits_);
  auto sameStub = [](const l1t::EMTFHit& a, const l1t::EMTFHit& b) {
    return a.Subsystem() == b.Subsystem() && a.Endcap() == b.Endcap() && a.Station() == b.Station() && a.Ring() == b.Ring() &&
           a.Chamber() == b.Chamber() && a.BX() == b.BX() && std::abs(a.Eta_sim() - b.Eta_sim()) < 1e-4 &&
           std::abs(a.Phi_sim() - b.Phi_sim()) < 1e-3;
  };
  int nNeighbour = 0, nNoTwin = 0;
  for (auto const& h : hits) {
    std::cout << "  EMTFHit: subsystem=" << (h.Subsystem() >= 0 && h.Subsystem() < 4 ? kindName(h.Subsystem()) : "?") << " endcap=" << h.Endcap()
              << " station=" << h.Station() << " ring=" << h.Ring() << " sector=" << h.Sector() << " chamber=" << h.Chamber() << " bx=" << h.BX()
              << " neighbour=" << h.Neighbor() << " eta=" << h.Eta_sim() << " phi=" << h.Phi_sim() * M_PI / 180. << " rho=" << h.Rho_sim()
              << " z=" << h.Z_sim() << " quality=" << h.Quality() << " pattern=" << h.Pattern() << "\n";
    if (h.Neighbor()) {
      ++nNeighbour;
      bool twin = false;
      for (auto const& o : hits)
        if (!o.Neighbor() && sameStub(h, o))
          twin = true;
      if (twin)
        continue;
      ++nNoTwin;
    }
    if (h.Subsystem() < 1 || h.Subsystem() > 3)
      continue;
    stubs.push_back({h.Subsystem(), h.BX(), h.Eta_sim(), float(wrapToPi(h.Phi_sim() * M_PI / 180.)), h.Rho_sim(), h.Z_sim(), h.Station()});
  }
  for (auto const& s : stubs) {
    ++nByKind[s.kind];
    if (s.bx == 0)
      ++nByKindBx0[s.kind];
  }
  std::cout << "  TP counts: DTphi=" << dtPh.size() << " DTtheta=" << dtTh.size() << " CSC_ALCT=" << nAlct << " CSC_CLCT=" << nClct << " CSC_LCT=" << nLct
            << " CSC_LCT_MPC=" << nMpc << " GEMpads=" << nPads << " GEMpadClusters=" << nPadClusters << "\n";
  std::cout << "  EMTF input stubs: all=" << hits.size() << " neighbourCopies=" << nNeighbour << " copiesWithoutTwin=" << nNoTwin
            << " distinct=" << hits.size() - nNeighbour + nNoTwin << " CSC=" << nByKind[1] << " (bx0 " << nByKindBx0[1] << ") RPC="
            << nByKind[2] << " (bx0 " << nByKindBx0[2] << ") GEM=" << nByKind[3] << " (bx0 " << nByKindBx0[3] << ")\n";

  // ------------------------------------------------------------------ step 4b: the track finders
  const auto& tracks = ev.get(emtfTracks_);
  for (auto const& t : tracks)
    std::cout << "  EMTFTrack: endcap=" << t.Endcap() << " sector=" << t.Sector() << " mode=" << t.Mode() << " bx=" << t.BX() << " pt=" << t.Pt() << " eta="
              << t.Eta() << " phi=" << wrapToPi(t.Phi_glob() * M_PI / 180.) << " charge=" << t.Charge() << " nHits=" << t.NumHits() << " quality="
              << t.GMT_quality() << "\n";
  std::vector<Cand> cands;
  std::map<std::string, int> nCand, nCandBx0;
  auto readCands = [&](const edm::EDGetTokenT<CandColl>& tok, const char* name, bool use) {
    const auto& c = ev.get(tok);
    for (int bx = c.getFirstBX(); bx <= c.getLastBX(); ++bx)
      for (auto it = c.begin(bx); it != c.end(bx); ++it) {
        double pt = (it->hwPt() - 1) * 0.5, eta = it->hwEta() * 0.010875;
        double phi = wrapToPi(l1t::MicroGMTConfiguration::calcGlobalPhi(it->hwPhi(), it->trackFinderType(), it->processor()) * 2. * M_PI / 576.);
        ++nCand[name];
        if (bx == 0)
          ++nCandBx0[name];
        if (use)
          cands.push_back({name, bx, it->processor(), it->hwQual(), float(pt), float(eta), float(phi)});
        std::cout << "  TFCand: tf=" << name << " bx=" << bx << " processor=" << it->processor() << " pt=" << pt << " eta=" << eta << " phi=" << phi
                  << " quality=" << it->hwQual() << " hwPt=" << it->hwPt() << " hwEta=" << it->hwEta() << " hwPhi=" << it->hwPhi() << " sign=" << it->hwSign()
                  << "\n";
      }
  };
  readCands(bmtf_, "BMTF", false);  // the legacy BMTF still runs; the Kalman one feeds the uGMT in Run 3
  readCands(kbmtf_, "kBMTF", true);
  readCands(omtf_, "OMTF", true);
  readCands(emtf_, "EMTF", true);
  std::cout << "  TF counts (all bx / bx 0): BMTF=" << nCand["BMTF"] << "/" << nCandBx0["BMTF"] << " kBMTF=" << nCand["kBMTF"] << "/" << nCandBx0["kBMTF"]
            << " OMTF=" << nCand["OMTF"] << "/" << nCandBx0["OMTF"] << " EMTF=" << nCand["EMTF"] << "/" << nCandBx0["EMTF"] << " EMTFtracks=" << tracks.size()
            << "\n";

  // ------------------------------------------------------------------ step 5: the global muon trigger
  const auto& muons = ev.get(muons_);
  std::vector<std::pair<int, l1t::Muon>> mus;
  for (int bx = muons.getFirstBX(); bx <= muons.getLastBX(); ++bx)
    for (auto it = muons.begin(bx); it != muons.end(bx); ++it) {
      mus.emplace_back(bx, *it);
      std::cout << "  uGMT muon: bx=" << bx << " pt=" << it->pt() << " eta=" << it->eta() << " phi=" << it->phi() << " charge=" << it->charge() << " quality="
                << it->hwQual() << " iso=" << it->hwIso() << " etaAtVtx=" << it->etaAtVtx() << " phiAtVtx=" << it->phiAtVtx() << " tfMuonIndex="
                << it->tfMuonIndex() << "\n";
    }
  const auto& showers = ev.get(showers_);
  int nShowers = 0;
  for (int bx = showers.getFirstBX(); bx <= showers.getLastBX(); ++bx)
    for (auto it = showers.begin(bx); it != showers.end(bx); ++it)
      if (it->isValid())
        ++nShowers;
  std::cout << "  uGMT counts: muons=" << mus.size() << " muonShowers=" << nShowers << "\n";
  // the truth of the signal: muons of the generator, jets
  std::vector<const reco::GenParticle*> genMu;
  for (auto const& p : ev.get(genParticles_))
    if (std::abs(p.pdgId()) == 13 && p.status() == 1 && p.pt() > genMuonPtMin_) {
      genMu.push_back(&p);
      std::cout << "  GenMuon: pt=" << p.pt() << " eta=" << p.eta() << " phi=" << p.phi() << " charge=" << p.charge() << "\n";
    }
  std::vector<reco::GenJet> gj;
  for (auto const& j : ev.get(genJets_))
    if (j.pt() > jetPtMin_)
      gj.push_back(j);
  std::cout << "  truth: genMuons(pt>" << genMuonPtMin_ << ")=" << genMu.size() << " genJets(pt>" << jetPtMin_ << ")=" << gj.size() << "\n";

  // ------------------------------------------------------------------ the pages
  page_.openPdf(pdfName(pdfFile_, ev.id().run(), ev.id().event(), nEvents_++));
  for (int view = 0; view < 2; ++view) {
    page_.begin();
    TH2F* frame =
        view == 0 ? new TH2F("l1tMuonEtaPhi", Form("steps 4, 5: muon trigger, run %u event %llu;#eta;#phi", ev.id().run(), ev.id().event()), 10, -2.8, 2.8, 10, -M_PI, M_PI)
                  : new TH2F("l1tMuonRZ", Form("steps 4, 5: muon trigger, run %u event %llu, r-z;z [cm];r [cm]", ev.id().run(), ev.id().event()), 10, -1200., 1200., 10, 0., 800.);
    frame->SetDirectory(nullptr);
    page_.keep(frame);
    frame->GetYaxis()->SetTitleOffset(view == 0 ? 0.9 : 1.15);
    if (view == 1)
      frame->GetYaxis()->SetLabelSize(0.032);
    frame->Draw();
    if (view == 0) {
      // the regions of the three track finders
      page_.etaLines({0.83, 1.24, 2.4}, kGray + 1, 2);
      page_.note(0., 3.19, "BMTF", kGray + 2, 0.022, 21);
      page_.note(-1.035, 3.19, "OMTF", kGray + 2, 0.022, 21);
      page_.note(1.035, 3.19, "OMTF", kGray + 2, 0.022, 21);
      page_.note(-1.82, 3.19, "EMTF", kGray + 2, 0.022, 21);
      page_.note(1.82, 3.19, "EMTF", kGray + 2, 0.022, 21);
      for (auto const& j : gj)
        page_.circle(j.eta(), j.phi(), 0.4, kMagenta + 1, 2);
    } else {
      // the directions eta = 0.83, 1.24, 2.4 from the origin
      for (double e : {0.83, 1.24, 2.4})
        for (int s = -1; s <= 1; s += 2) {
          double t = 2. * std::atan(std::exp(-e)), zmax = std::min(1200., 800. / std::tan(t));
          page_.line(0., 0., s * zmax, zmax * std::tan(t), kGray + 1, 1, 2);
        }
    }
    for (auto const& s : stubs) {
      double x = view == 0 ? s.eta : s.z, y = view == 0 ? s.phi : s.r;
      if (view == 0 && std::abs(x) > 2.8)
        continue;
      page_.marker(x, y, s.bx == 0 ? kindFull[s.kind] : kindOpen[s.kind], kindColor[s.kind], 1.3);
    }
    for (size_t ic = 0; ic < cands.size(); ++ic) {
      const Cand& c = cands[ic];
      if (view == 0) {
        page_.marker(c.eta, c.phi, 24, kRed + 1, 2.6);
        // candidates of neighbouring processors at the same place (the same muon found twice): one label
        std::string lab = c.tf + Form(" %.1f", c.pt);
        bool first = true;
        for (size_t jc = 0; jc < cands.size(); ++jc) {
          if (jc == ic || std::abs(cands[jc].eta - c.eta) > 0.1 || std::abs(wrapToPi(cands[jc].phi - c.phi)) > 0.1)
            continue;
          if (jc < ic)
            first = false;
          lab += Form(" + %.1f", cands[jc].pt);
        }
        if (first)
          page_.note(c.eta + 0.1, c.phi + (c.phi < -2.6 ? 0.3 : -0.16), lab + " GeV", kRed + 1, 0.02);
      } else {
        double t = 2. * std::atan(std::exp(-std::abs(c.eta))), zmax = std::min(1150., 780. / std::tan(t));
        page_.line(0., 0., (c.eta > 0 ? 1 : -1) * zmax, zmax * std::tan(t), kRed + 1, 1, 1);
      }
    }
    for (auto const& m : mus) {
      if (view == 0) {
        page_.marker(m.second.eta(), m.second.phi(), 29, kBlack, 2.2);
        page_.note(m.second.eta() + 0.1, m.second.phi() + 0.14, Form("uGMT #mu %.1f GeV", m.second.pt()), kBlack, 0.022);
      } else {
        double t = 2. * std::atan(std::exp(-std::abs(m.second.eta()))), zmax = std::min(1150., 780. / std::tan(t));
        page_.line(0., 0., (m.second.eta() > 0 ? 1 : -1) * zmax, zmax * std::tan(t), kBlack, 2, 1);
      }
    }
    if (view == 0)
      for (auto p : genMu)
        page_.marker(p->eta(), p->phi(), 5, kMagenta + 1, 1.8);
    // the right panel
    page_.label(sampleLabel_);
    page_.text("step 4: trigger primitives", 0.025, kBlue + 1);
    page_.text(Form("   DT: %zu #phi segments, %zu #theta segments", dtPh.size(), dtTh.size()), 0.021, kindColor[0]);
    page_.text(Form("   CSC: %d ALCT, %d CLCT #rightarrow %d LCT", nAlct, nClct, nLct), 0.021, kindColor[1]);
    page_.text(Form("   GEM: %d pads, %d pad clusters", nPads, nPadClusters), 0.021, kindColor[3]);
    page_.text("stubs given to the EMTF (drawn):", 0.021);
    for (int k = 1; k <= 3; ++k)
      page_.text(Form("   %s: %d, of which %d in bx 0", kindName(k), nByKind[k], nByKindBx0[k]), 0.021, kindColor[k]);
    page_.text("   full marker: bx 0; open: another bx", 0.019, kGray + 2);
    page_.skip();
    page_.text("step 4: track finders (red circles)", 0.025, kRed + 1);
    page_.text(Form("   BMTF (Kalman): %d   OMTF: %d   EMTF: %d", nCand["kBMTF"], nCand["OMTF"], nCand["EMTF"]), 0.021);
    for (auto const& c : cands)
      page_.text(Form("   %s %d: p_{T} %.1f, #eta %.2f, #phi %.2f, q %d, bx %d", c.tf.c_str(), c.processor, c.pt, c.eta, c.phi, c.quality, c.bx), 0.019,
                 kRed + 1);
    page_.skip();
    page_.text("step 5: global muon trigger (stars)", 0.025, kBlack);
    page_.text(Form("   %zu muon%s sent to the global trigger", mus.size(), mus.size() == 1 ? "" : "s"), 0.021);
    for (auto const& m : mus)
      page_.text(Form("   p_{T} %.1f GeV, #eta %.2f, #phi %.2f, q %d, bx %d", m.second.pt(), m.second.eta(), m.second.phi(), m.second.hwQual(), m.first), 0.019);
    page_.text(Form("   muon showers: %d", nShowers), 0.021);
    page_.skip();
    page_.text(Form("generator muons, p_{T} > %.0f GeV: %zu (crosses)", genMuonPtMin_, genMu.size()), 0.02, kMagenta + 1);
    if (view == 0 && !gj.empty())
      page_.text(Form("circles: ak4GenJets, p_{T} > %.0f GeV", jetPtMin_), 0.02, kMagenta + 1);
    if (view == 1)
      page_.text("dashed: |#eta| = 0.83, 1.24, 2.4", 0.019, kGray + 2);
    page_.print(savePNG_ ? pngPrefix_ + (view == 0 ? "_etaphi" : "_rz") + tag + ".png" : std::string());
  }
  page_.closePdf();
}

DEFINE_FWK_MODULE(L1TMuonAnalyzer);
