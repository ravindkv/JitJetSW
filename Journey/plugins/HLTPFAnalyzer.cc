// HLTPFAnalyzer: step 4 of the High Level Trigger for one event (chapter 13, HLT, of the JitJet book): particle flow
// and the PF jets.
//
//   particle flow links the tracks of step 3 with the calorimeter clusters (ECAL, HCAL, HF, preshower: built from
//   PF rec-hits) into blocks and makes of each block a list of particles (hltParticleFlow: charged hadrons, electrons,
//   muons, photons, neutral hadrons, and in the HF hadronic and electromagnetic deposits); the particles are clustered
//   into anti-kT jets of radius 0.4 (hltAK4PFJets) and corrected (hltAK4PFJetsCorrected: L1FastJet with the energy
//   density hltFixedGridRhoFastjetAll, L2Relative, L3Absolute, L2L3Residual of the payload AK4PFHLT)
// Pages (one PDF per event, one PNG per page with savePNG):
//   candidates  the PF candidates in eta-phi by type, with the corrected PF jets and the generator jets
//   jet<N>      the PF jet of each of the two leading generator jets, constituent by constituent, and the arithmetic
//               of its corrections
// The log has the candidates by type, the energy density, every PF jet with its composition and its corrections
// level by level, and the constituents of the two signal jets (read by test/common/hltLedger.py).
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "CondFormats/JetMETObjects/interface/JetCorrectorParameters.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "DataFormats/JetReco/interface/PFJet.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "JetMETCorrections/Objects/interface/JetCorrectionsRecord.h"

#include "HLTDisplay.h"

namespace {
  using namespace hltdisplay;
  using PF = reco::PFCandidate;

  struct Kind {
    const char* name;
    const char* label;
    int color, marker;
  };
  const Kind& kind(int id) {
    static const std::map<int, Kind> k = {
        {PF::h, {"h", "charged hadron", kBlack, 20}},          {PF::e, {"e", "electron", kAzure + 1, 21}},
        {PF::mu, {"mu", "muon", kRed, 29}},                    {PF::gamma, {"gamma", "photon", kGreen + 2, 20}},
        {PF::h0, {"h0", "neutral hadron", kViolet + 1, 20}},   {PF::h_HF, {"h_HF", "HF hadronic", kOrange + 7, 21}},
        {PF::egamma_HF, {"egamma_HF", "HF electromagnetic", kCyan + 2, 21}}, {PF::X, {"X", "unknown", kGray, 5}},
    };
    auto it = k.find(id);
    return it == k.end() ? k.at(PF::X) : it->second;
  }
  const std::vector<int> kKinds = {PF::h, PF::e, PF::mu, PF::gamma, PF::h0, PF::h_HF, PF::egamma_HF};
}  // namespace

class HLTPFAnalyzer : public edm::one::EDAnalyzer<> {
public:
  explicit HLTPFAnalyzer(const edm::ParameterSet&);
  void analyze(const edm::Event&, const edm::EventSetup&) override;

private:
  edm::EDGetTokenT<reco::PFCandidateCollection> cands_;
  edm::EDGetTokenT<reco::PFJetCollection> rawJets_, corrJets_;
  edm::EDGetTokenT<double> rho_;
  edm::EDGetTokenT<std::vector<reco::GenJet>> genJets_;
  edm::ESGetToken<JetCorrectorParametersCollection, JetCorrectionsRecord> jec_;
  std::vector<std::string> levels_;
  std::string pdfFile_, pngPrefix_, sampleLabel_;
  bool savePNG_;
  double jetPtMin_;
  int nEvents_ = 0;
  Page page_;
  Corrections corrections_;
};

HLTPFAnalyzer::HLTPFAnalyzer(const edm::ParameterSet& ps)
    : cands_(consumes(ps.getParameter<edm::InputTag>("pfCandidates"))),
      rawJets_(consumes(ps.getParameter<edm::InputTag>("rawJets"))),
      corrJets_(consumes(ps.getParameter<edm::InputTag>("correctedJets"))),
      rho_(consumes(ps.getParameter<edm::InputTag>("rho"))),
      genJets_(consumes(ps.getParameter<edm::InputTag>("genJets"))),
      jec_(esConsumes(edm::ESInputTag("", ps.getParameter<std::string>("jecPayload")))),
      levels_(ps.getParameter<std::vector<std::string>>("jecLevels")),
      pdfFile_(ps.getParameter<std::string>("pdfFile")),
      pngPrefix_(ps.getParameter<std::string>("pngPrefix")),
      sampleLabel_(ps.getParameter<std::string>("sampleLabel")),
      savePNG_(ps.getParameter<bool>("savePNG")),
      jetPtMin_(ps.getParameter<double>("jetPtMin")),
      page_("hltPF") {}

void HLTPFAnalyzer::analyze(const edm::Event& ev, const edm::EventSetup& es) {
  std::string tag = Form("_run%u_event%llu", ev.id().run(), ev.id().event());
  std::cout << "== HLTPFAnalyzer run:lumi:event " << ev.id().run() << ":" << ev.luminosityBlock() << ":" << ev.id().event() << "\n";
  corrections_.set(es.getData(jec_), levels_);

  // ------------------------------------------------------------------ the candidates
  const auto& cands = ev.get(cands_);
  std::map<int, std::pair<int, double>> byKind;
  double sumAll = 0;
  for (auto const& c : cands) {
    auto& e = byKind[c.particleId()];
    ++e.first;
    e.second += c.pt();
    sumAll += c.pt();
  }
  std::cout << "  pfcands all: n=" << cands.size() << " sumPt=" << Form("%.1f", sumAll) << "\n";
  for (int id : kKinds)
    std::cout << "  pfcands " << kind(id).name << ": n=" << byKind[id].first << " sumPt=" << Form("%.1f", byKind[id].second) << "\n";
  // by region: the tracker covers |eta| < 2.5, the HF 3 < |eta| < 5
  for (auto const& [name, lo, hi] : std::vector<std::tuple<const char*, double, double>>{{"tracker", 0., 2.5}, {"outside", 2.5, 3.0}, {"HF", 3.0, 9.}}) {
    int n = 0, nCh = 0;
    double s = 0, sCh = 0;
    for (auto const& c : cands)
      if (std::abs(c.eta()) >= lo && std::abs(c.eta()) < hi) {
        ++n;
        s += c.pt();
        if (c.charge() != 0) {
          ++nCh;
          sCh += c.pt();
        }
      }
    std::cout << "  pfcands region " << name << ": n=" << n << " sumPt=" << Form("%.1f", s) << " charged n=" << nCh << " sumPt=" << Form("%.1f", sCh) << "\n";
  }

  // ------------------------------------------------------------------ the PF jets and their corrections
  double rho = ev.get(rho_);
  std::cout << "  rho (hltFixedGridRhoFastjetAll): " << Form("%.3f", rho) << " GeV per unit area\n";
  std::cout << "  jec payload levels:";
  for (auto const& l : corrections_.levels())
    std::cout << " " << l;
  std::cout << "\n";
  for (auto const& d : corrections_.description())
    std::cout << "  jec pf: " << d << "\n";
  const auto& raw = ev.get(rawJets_);
  const auto& corr = ev.get(corrJets_);
  std::vector<reco::GenJet> gj;
  for (auto const& j : ev.get(genJets_))
    if (j.pt() > jetPtMin_)
      gj.push_back(j);
  int n10 = 0, n30 = 0, n30m = 0, nRaw10 = 0, nRaw30 = 0;
  for (auto const& j : raw) {
    nRaw10 += j.pt() > 10;
    nRaw30 += j.pt() > 30;
  }
  for (auto const& j : corr) {
    n10 += j.pt() > 10;
    n30 += j.pt() > 30;
    bool m = false;
    for (auto const& g : gj)
      m |= dR(g.eta(), g.phi(), j.eta(), j.phi()) < 0.4;
    n30m += j.pt() > 30 && m;
  }
  std::cout << "  pf jets: raw=" << raw.size() << " raw>10=" << nRaw10 << " raw>30=" << nRaw30 << " corrected=" << corr.size() << " corrected>10=" << n10
            << " >30=" << n30 << " >30 with a gen jet=" << n30m << "\n";
  auto rawOf = [&](const reco::PFJet& cj) -> const reco::PFJet* {
    for (auto const& x : raw)
      if (dR(x.eta(), x.phi(), cj.eta(), cj.phi()) < 0.01)
        return &x;
    return nullptr;
  };
  auto genOf = [&](double eta, double phi, double& dr) -> const reco::GenJet* {
    const reco::GenJet* g = nullptr;
    dr = 9;
    for (auto const& x : gj)
      if (double d = dR(x.eta(), x.phi(), eta, phi); d < 0.4 && d < dr) {
        dr = d;
        g = &x;
      }
    return g;
  };
  // every jet above 10 GeV before or after the corrections (the corrections can lower a jet below 10 GeV)
  int k = 0;
  for (auto const& j : corr) {
    const reco::PFJet* r = rawOf(j);
    if (!r || (j.pt() < 10 && r->pt() < 10))
      continue;
    auto f = corrections_.factors(r->pt(), r->energy(), r->eta(), r->phi(), r->jetArea(), rho);
    double dr;
    const reco::GenJet* g = genOf(j.eta(), j.phi(), dr);
    std::cout << "  PFJet " << k++ << ": pt=" << Form("%.1f", j.pt()) << " rawPt=" << Form("%.1f", r->pt()) << " eta=" << Form("%.2f", j.eta()) << " phi=" << Form("%.2f", j.phi())
              << " area=" << Form("%.3f", r->jetArea()) << " n=" << r->nConstituents() << " chf=" << Form("%.2f", r->chargedHadronEnergyFraction())
              << " nhf=" << Form("%.2f", r->neutralHadronEnergyFraction()) << " phf=" << Form("%.2f", r->photonEnergyFraction())
              << " elf=" << Form("%.2f", r->electronEnergyFraction()) << " muf=" << Form("%.2f", r->muonEnergyFraction())
              << " hfhf=" << Form("%.2f", r->HFHadronEnergyFraction()) << " hfemf=" << Form("%.2f", r->HFEMEnergyFraction()) << " |"
              << factorString(corrections_.levels(), f) << " jecBin=" << corrections_.bin(r->eta(), r->phi()) << " | recomputedPt=" << Form("%.1f", r->pt() * (f.empty() ? 1. : f.back()))
              << " | gen=" << (g ? Form("%.1f", g->pt()) : "-") << " dR=" << (g ? Form("%.2f", dr) : "-") << "\n";
  }

  // ------------------------------------------------------------------ page 1: the candidates
  page_.openPdf(pdfName(pdfFile_, ev.id().run(), ev.id().event(), nEvents_++));
  page_.begin();
  frame(page_, "hltPFFrame", Form("step 4: the PF candidates (hltParticleFlow) and AK4 PF jets, run %u event %llu", ev.id().run(), ev.id().event()));
  detectorLines(page_);
  genJets(page_, gj, jetPtMin_, false);
  for (int id : {PF::h0, PF::gamma, PF::h_HF, PF::egamma_HF, PF::e, PF::h, PF::mu})
    for (auto const& c : cands)
      if (c.particleId() == id)
        page_.marker(c.eta(), c.phi(), kind(id).marker, kind(id).color, 0.3 + 0.55 * std::log10(1. + c.pt()));
  int nDrawn = 0;
  for (auto const& j : corr) {
    if (j.pt() < 20)
      continue;
    ++nDrawn;
    page_.circle(j.eta(), j.phi(), 0.4, kRed, 2, 2);
    page_.note(j.eta() + 0.3, j.phi() - 0.42, Form("%.0f", j.pt()), kRed + 1, 0.022);
  }
  page_.label(sampleLabel_);
  page_.text(Form("%zu PF candidates, #Sigmap_{T} = %.0f GeV", cands.size(), sumAll), 0.022, kBlue + 1);
  for (int id : kKinds)
    page_.text(Form("   %s: %d, %.0f GeV", kind(id).label, byKind[id].first, byKind[id].second), 0.019, kind(id).color);
  page_.skip();
  page_.text(Form("#rho = %.1f GeV per unit area", rho), 0.021);
  page_.text("AK4 PF jets, corrected", 0.025, kBlue + 1);
  page_.text(Form("   %d above 10 GeV, %d above 30 GeV (raw: %d, %d)", n10, n30, nRaw10, nRaw30), 0.021);
  page_.text(Form("   %d above 30 GeV have a gen jet", n30m), 0.021);
  page_.text(Form("   red dashed circles: the %d above 20 GeV", nDrawn), 0.019, kRed);
  page_.text("magenta: ak4GenJets; size: log(1 + p_{T})", 0.019, kGenColor);
  page_.print(savePNG_ ? pngPrefix_ + "_candidates" + tag + ".png" : std::string());

  // ------------------------------------------------------------------ pages 2, 3: the two signal jets, constituent by constituent
  for (size_t ig = 0; ig < gj.size() && ig < 2; ++ig) {
    const auto& g = gj[ig];
    const reco::PFJet* cj = nullptr;
    double dr = 0.4;
    for (auto const& x : corr)
      if (double d = dR(x.eta(), x.phi(), g.eta(), g.phi()); d < dr) {
        dr = d;
        cj = &x;
      }
    double ceta = cj ? cj->eta() : g.eta(), cphi = cj ? cj->phi() : g.phi();
    page_.begin();
    frame(page_, Form("hltPFZoom%zu", ig), Form("step 4: the PF jet of generator jet %zu (p_{T} %.1f GeV)", ig + 1, g.pt()), ceta - 1.0, ceta + 1.0, cphi - 1.0, cphi + 1.0);
    // the phi coordinate of the page is continuous around the jet
    auto Y = [&](double phi) { return cphi + dPhi(phi, cphi); };
    page_.circle(g.eta(), Y(g.phi()), 0.4, kGenColor, 2);
    if (cj)
      page_.circle(cj->eta(), Y(cj->phi()), 0.4, kRed, 3, 2);
    for (double e : {-3.0, -2.5, 2.5, 3.0})  // the detector boundaries inside the window only (a TLine is not clipped)
      if (e > ceta - 1.0 && e < ceta + 1.0)
        page_.line(e, cphi - 1.0, e, cphi + 1.0, kGray + 1, 1, 3);
    std::map<int, std::pair<int, double>> inJet;
    const reco::PFJet* r = cj ? rawOf(*cj) : nullptr;
    std::vector<const reco::PFCandidate*> constituents;
    if (r)
      for (auto const& c : r->getPFConstituents())
        constituents.push_back(c.get());
    for (auto const& c : cands) {
      double d = dR(c.eta(), c.phi(), ceta, cphi);
      if (d > 1.0)
        continue;
      bool in = std::find_if(constituents.begin(), constituents.end(), [&](const reco::PFCandidate* x) { return x && std::abs(x->pt() - c.pt()) < 1e-4 && std::abs(x->eta() - c.eta()) < 1e-4; }) != constituents.end();
      const Kind& kk = kind(c.particleId());
      page_.marker(c.eta(), Y(c.phi()), in ? kk.marker : (kk.marker == 20 ? 24 : 25), kk.color, 0.5 + 0.8 * std::log10(1. + c.pt()));
      if (in && c.pt() > 4)
        page_.note(c.eta() + 0.03, Y(c.phi()) + 0.045, Form("%.1f", c.pt()), kk.color, 0.019);
      if (in) {
        ++inJet[c.particleId()].first;
        inJet[c.particleId()].second += c.pt();
      }
    }
    std::cout << "  zoom gen jet " << ig + 1 << ": pt=" << Form("%.1f", g.pt()) << " eta=" << Form("%.2f", g.eta()) << " | PF jet "
              << (cj ? Form("%.1f (raw %.1f) dR=%.2f", cj->pt(), r ? r->pt() : -1., dr) : "none") << " | constituents:";
    for (int id : kKinds)
      std::cout << " " << kind(id).name << "=" << inJet[id].first << "/" << Form("%.1f", inJet[id].second);
    std::cout << "\n";
    page_.label(sampleLabel_);
    page_.text(Form("generator jet %zu: p_{T} %.1f GeV, #eta %.2f", ig + 1, g.pt(), g.eta()), 0.021, kGenColor);
    if (cj && r) {
      page_.text(Form("the AK4 PF jet: %d constituents", int(r->nConstituents())), 0.025, kBlue + 1);
      for (int id : kKinds)
        if (inJet[id].first)
          page_.text(Form("   %d %s: %.1f GeV", inJet[id].first, kind(id).label, inJet[id].second), 0.019, kind(id).color);
      page_.skip();
      auto f = corrections_.factors(r->pt(), r->energy(), r->eta(), r->phi(), r->jetArea(), rho);
      page_.text("its corrections", 0.025, kBlue + 1);
      page_.text(Form("   raw p_{T} %.1f GeV, area %.2f", r->pt(), r->jetArea()), 0.020);
      page_.text(Form("   #rho #times area = %.1f GeV", rho * r->jetArea()), 0.020);
      for (size_t l = 0; l < f.size() && l < corrections_.levels().size(); ++l)
        page_.text(Form("   after %s: %.1f GeV (#times%.3f)", corrections_.levels()[l].c_str(), r->pt() * f[l], l ? f[l] / f[l - 1] : f[l]), 0.019);
      page_.text(Form("   corrected p_{T} %.1f GeV", cj->pt()), 0.022, kRed + 1);
    } else {
      page_.text("no PF jet within #DeltaR < 0.4", 0.021, kRed);
    }
    page_.skip();
    page_.text("full markers: constituents; open: other candidates", 0.017, kGray + 2);
    page_.print(savePNG_ ? pngPrefix_ + Form("_jet%zu", ig + 1) + tag + ".png" : std::string());
  }
  page_.closePdf();
}

DEFINE_FWK_MODULE(HLTPFAnalyzer);
