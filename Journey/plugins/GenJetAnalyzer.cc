// GenJetAnalyzer: eta-phi maps of the generator jets, one per jet collection and event.
//
// The jets of every collection (anti-kT from FastjetJetProducer: ak4GenJets, ak8GenJets, the
// NoNu variants, ...) are drawn over the status 1 genParticles they were clustered from: every
// jet is a circle of radius R around its axis (star marker, label = [index] pT), its
// constituents are filled markers in the jet colour (size growing with pT) and the particles
// that ended up in no jet (below the jet pT threshold, neutrinos for the NoNu collections)
// are grey open markers. The radius, algorithm, input collection and jet pT threshold of
// every collection are read from the producer configuration stored in the file (provenance),
// so they need not be repeated here, and the process that produced the jets is printed.
//
// The hard-process b and bbar are followed to their last same-flavour copy (the quark after
// the shower, before hadronisation; in a sample generated without hadronisation the status 1
// parton) and matched to the nearest jet within R: the b jet is drawn red, the bbar jet blue,
// the quark directions as open stars. pT, eta, phi of both jets, the response
// pT(jet) / pT(quark), the net |pT(b jet) + pT(bbar jet)| and Delta phi are printed on the
// plot and to std::cout, as GenPartAnalyzer does for the quarks and hadrons.
//
// One PDF is written per collection and event (<outputPrefix>_<collection>_run<R>_event<E>.pdf;
// ROOT can keep only one multi-page PDF open at a time) or, if pdfFile is set, all plots go
// as pages into that single PDF. With openPdf = false the file is opened by another module of
// the same job that prints into the same PDF before this one (genJetAnalyzer_cfg.py runs
// GenPartAnalyzer first, so every event gets the generator-stage pages followed by the jet
// pages); closePdf = false leaves the closing to that module. Per collection, the TFileService file gets the jet pT, eta
// and multiplicity, the constituent multiplicity, the b / bbar jet pT, the response, the net
// b + bbar jet pT and etaPhiPt (TH2, eta vs phi weighted by jet pT).
//
// Pileup (signalCollisionId >= 0): when the genParticles collection holds the signal event and
// the pileup collisions mixed into it (GenParticleProducer with useCrossingFrame on the
// CrossingFrame<HepMCProduct> of the MixingModule, genJetPileup_cff.py), every particle carries
// the index of its collision (GenParticle::collisionId, 0 = signal). The particles of the other
// collisions are then drawn as triangles (filled in the jet colour when clustered, open and
// light grey when not), the hard-process b and bbar are looked for in the signal only, the
// pileup pT of every jet (scalar sum of its pileup constituents) is listed next to the jet,
// and the header counts the signal and pileup particles and the pileup collisions.
#include <cmath>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TGraph.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TMarker.h"
#include "TROOT.h"
#include "TStyle.h"

#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "DataFormats/JetReco/interface/GenJetCollection.h"
#include "DataFormats/Math/interface/deltaPhi.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/Provenance/interface/Provenance.h"
#include "FWCore/Common/interface/Provenance.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/Exception.h"

class GenJetAnalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit GenJetAnalyzer(const edm::ParameterSet& ps);
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  using Gens = reco::GenParticleCollection;
  using Jets = reco::GenJetCollection;

  struct Hists {
    TH2D* etaPhiPt = nullptr;
    TH1D *nJets = nullptr, *jetPt = nullptr, *jetEta = nullptr, *nConst = nullptr;
    TH1D *ptBjet = nullptr, *ptBbarJet = nullptr, *ptNet = nullptr, *response = nullptr;
  };
  struct Producer {  // what the FastjetJetProducer was configured with (from the provenance)
    std::string process, algorithm, src;
    double rParam = 0., jetPtMin = 0.;
    bool known = false;
  };
  struct Quark {  // one hard-process quark (b or bbar), its last copy and the matched jet
    bool found = false;
    size_t hard = 0, last = 0;
    int jet = -1;
    double dR = 0.;
  };

  void beginJob() override;
  void analyze(const edm::Event& ev, const edm::EventSetup&) override;
  void endJob() override;

  static bool isIncoming(int status);
  static double markerSize(double pt);
  bool isPileup(const reco::GenParticle& g) const {
    return signalCollisionId_ >= 0 && g.collisionId() != signalCollisionId_;
  }
  Producer producerOf(const edm::Event& ev, const edm::Provenance& prov, const std::string& label) const;
  void draw(size_t k, const edm::Event& ev, const Gens& gens, const Jets& jets, const std::vector<int>& jetOf,
            const std::vector<double>& puPt, const Quark quark[2], const Producer& prod);

  edm::EDGetTokenT<Gens> genParticlesTok_;
  std::vector<edm::InputTag> jetTags_;
  std::vector<edm::EDGetTokenT<Jets>> jetToks_;
  std::string outputPrefix_, pdfFile_, sampleLabel_;
  double etaMax_, labelPtMin_;
  int signalCollisionId_;  // >= 0: the status 1 particles of the other collisions (collisionId) are pileup
  bool savePDF_, printConstituents_, openPdf_, closePdf_;
  bool pdfOpen_ = false;  // the multi-page pdfFile_ is opened on the first page and closed in endJob

  std::unique_ptr<TCanvas> canvas_;
  std::vector<Hists> hists_;
};

GenJetAnalyzer::GenJetAnalyzer(const edm::ParameterSet& ps)
    : genParticlesTok_(consumes<Gens>(ps.getParameter<edm::InputTag>("genParticles"))),
      jetTags_(ps.getParameter<std::vector<edm::InputTag>>("genJets")),
      outputPrefix_(ps.getParameter<std::string>("outputPrefix")),
      pdfFile_(ps.getParameter<std::string>("pdfFile")),
      sampleLabel_(ps.getParameter<std::string>("sampleLabel")),
      etaMax_(ps.getParameter<double>("etaMax")),
      labelPtMin_(ps.getParameter<double>("labelPtMin")),
      signalCollisionId_(ps.getParameter<int>("signalCollisionId")),
      savePDF_(ps.getParameter<bool>("savePDF")),
      printConstituents_(ps.getParameter<bool>("printConstituents")),
      openPdf_(ps.getParameter<bool>("openPdf")),
      closePdf_(ps.getParameter<bool>("closePdf")) {
  usesResource(TFileService::kSharedResource);
  edm::Service<TFileService> fs;
  for (auto const& tag : jetTags_) {
    jetToks_.push_back(consumes<Jets>(tag));
    TFileDirectory dir = fs->mkdir(tag.label());
    const char* t = tag.label().c_str();
    Hists h;
    h.etaPhiPt = dir.make<TH2D>("etaPhiPt", Form("%s;#eta;#phi;#Sigma p_{T}(jet) [GeV]", t), 120, -etaMax_, etaMax_, 64,
                                -M_PI, M_PI);
    h.nJets = dir.make<TH1D>("nJets", Form("%s;number of jets;events", t), 60, -0.5, 59.5);
    h.jetPt = dir.make<TH1D>("jetPt", Form("%s;p_{T}(jet) [GeV];jets", t), 100, 0., 500.);
    h.jetEta = dir.make<TH1D>("jetEta", Form("%s;#eta(jet);jets", t), 60, -etaMax_, etaMax_);
    h.nConst = dir.make<TH1D>("nConstituents", Form("%s;constituents per jet;jets", t), 80, -0.5, 79.5);
    h.ptBjet = dir.make<TH1D>("ptBjet", Form("%s;p_{T}(b jet) [GeV];events", t), 100, 0., 500.);
    h.ptBbarJet = dir.make<TH1D>("ptBbarJet", Form("%s;p_{T}(#bar{b} jet) [GeV];events", t), 100, 0., 500.);
    h.ptNet = dir.make<TH1D>("ptNetBBbarJets", Form("%s;|#vec{p}_{T}(b jet) + #vec{p}_{T}(#bar{b} jet)| [GeV];events", t),
                             100, 0., 200.);
    h.response = dir.make<TH1D>("responseB", Form("%s;p_{T}(jet) / p_{T}(b quark);b and #bar{b} jets", t), 100, 0., 2.);
    hists_.push_back(h);
  }
}

void GenJetAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("genParticles", edm::InputTag("genParticles"));
  desc.add<std::vector<edm::InputTag>>("genJets", {edm::InputTag("ak4GenJets"), edm::InputTag("ak8GenJets")})
      ->setComment("jet collections, one page per collection and event; R, input and pT threshold come from the provenance");
  desc.add<std::string>("outputPrefix", "genJetAnalyzer")
      ->setComment("PDFs are written as <outputPrefix>_<collection>_run<R>_event<E>.pdf");
  desc.add<double>("etaMax", 6.)->setComment("eta axis range; objects beyond are drawn on the border with an x marker");
  desc.add<double>("labelPtMin", 5.)->setComment("jet pT above which [index] pT is written next to the jet axis (b, bbar jets always)");
  desc.add<int>("signalCollisionId", -1)
      ->setComment("collisionId of the signal event when the collection also holds mixed pileup collisions (-1: no pileup)");
  desc.add<bool>("savePDF", true);
  desc.add<std::string>("pdfFile", "")
      ->setComment("if set, all plots are written as pages of this single PDF instead of one PDF per collection and event");
  desc.add<std::string>("sampleLabel", "")
      ->setComment("printed in the right panel of every plot, '|' separates lines (e.g. the generator stage)");
  desc.add<bool>("printConstituents", false)->setComment("print the constituents (genParticles indices) of every jet to stdout");
  desc.add<bool>("openPdf", true)->setComment("open pdfFile before the first page (false: another module of the job opens it)");
  desc.add<bool>("closePdf", true)->setComment("close pdfFile in endJob (false: another module of the job closes it)");
  descriptions.addWithDefaultLabel(desc);
}

bool GenJetAnalyzer::isIncoming(int status) {
  int s = std::abs(status);
  return s == 4 || (s >= 11 && s <= 19) || s == 21 || s == 31 || s == 41 || s == 42 || s == 45 || s == 46 || s == 53 ||
         s == 61;
}

double GenJetAnalyzer::markerSize(double pt) { return 0.6 + 0.9 * std::log10(1. + pt); }

// rParam, jetAlgorithm, jetPtMin and src of the producer, from the configuration stored in the file
GenJetAnalyzer::Producer GenJetAnalyzer::producerOf(const edm::Event& ev, const edm::Provenance& prov,
                                                    const std::string& label) const {
  Producer p;
  p.process = prov.processName();
  try {
    const edm::ParameterSet& ps = edm::parameterSet(prov.stable(), ev.processHistory());
    p.rParam = ps.getParameter<double>("rParam");
    p.jetPtMin = ps.getParameter<double>("jetPtMin");
    p.algorithm = ps.getParameter<std::string>("jetAlgorithm");
    p.src = ps.getParameter<edm::InputTag>("src").label();
    p.known = true;
  } catch (cms::Exception const& e) {
    // not a FastjetJetProducer (or no configuration stored): guess R from the label (ak4 -> 0.4)
    size_t pos = label.find("ak");
    if (pos != std::string::npos && pos + 2 < label.size() && std::isdigit(label[pos + 2]))
      p.rParam = (label[pos + 2] - '0') / 10.;
    p.algorithm = "?";
  }
  return p;
}

// ----------------------------------------------------------------------------- job
void GenJetAnalyzer::beginJob() {
  gROOT->SetBatch(true);
  gStyle->SetOptStat(0);
  gStyle->SetPaperSize(29., 20.);  // see GenPartAnalyzer: fills the landscape page for the 1400 x 900 canvas
  canvas_ = std::make_unique<TCanvas>("genJetCanvas", "", 1400, 900);
}

void GenJetAnalyzer::analyze(const edm::Event& ev, const edm::EventSetup&) {
  edm::Handle<Gens> hGen;
  ev.getByToken(genParticlesTok_, hGen);
  const Gens& gens = *hGen;
  const size_t n = gens.size();

  // 1. the hard-process b and bbar and their last same-flavour copies
  Quark quark[2];  // 0: b, 1: bbar
  size_t nStatus1 = 0, nPileup1 = 0;
  std::set<int> collisions;  // the pileup collisions (collisionId != signal) with a status 1 particle
  for (size_t i = 0; i < n; ++i) {
    if (gens[i].status() == 1) {
      ++nStatus1;
      if (isPileup(gens[i])) {
        ++nPileup1;
        collisions.insert(gens[i].collisionId());
      }
    }
    if (std::abs(gens[i].pdgId()) != 5 || !gens[i].isHardProcess() || isIncoming(gens[i].status()) || gens[i].pt() <= 0. ||
        isPileup(gens[i]))
      continue;
    Quark& q = quark[gens[i].pdgId() > 0 ? 0 : 1];
    if (q.found && gens[q.hard].pt() >= gens[i].pt())
      continue;  // more than one hard b of the same sign: keep the leading one
    q.found = true;
    q.hard = q.last = i;
    for (int guard = 0; guard < 1000; ++guard) {  // follow the same-flavour daughters to the last copy
      const reco::GenParticle& g = gens[q.last];
      size_t next = n;
      for (size_t d = 0; d < g.numberOfDaughters(); ++d) {
        size_t k = g.daughterRef(d).key();
        if (gens[k].pdgId() == g.pdgId() && (next == n || gens[k].pt() > gens[next].pt()))
          next = k;
      }
      if (next == n)
        break;
      q.last = next;
    }
  }

  std::cout << "\n== GenJetAnalyzer run:lumi:event " << ev.id().run() << ":" << ev.luminosityBlock() << ":"
            << ev.id().event() << " nGenParticles=" << n << " nStatus1=" << nStatus1;
  if (signalCollisionId_ >= 0)
    std::cout << " (signal " << nStatus1 - nPileup1 << " + pileup " << nPileup1 << " from " << collisions.size()
              << " collisions)";
  for (int k = 0; k < 2; ++k) {
    std::cout << " | hard " << (k ? "bbar" : "b") << ":";
    if (!quark[k].found) {
      std::cout << " none";
      continue;
    }
    const reco::GenParticle& h = gens[quark[k].hard];
    const reco::GenParticle& l = gens[quark[k].last];
    std::cout << " [" << quark[k].hard << "] pt=" << h.pt() << " -> last copy [" << quark[k].last << "] status=" << l.status()
              << " pt=" << l.pt() << " eta=" << l.eta() << " phi=" << l.phi();
  }
  std::cout << "\n";

  // 2. every jet collection
  for (size_t k = 0; k < jetToks_.size(); ++k) {
    edm::Handle<Jets> hJets;
    ev.getByToken(jetToks_[k], hJets);
    if (!hJets.isValid()) {
      std::cout << "  " << jetTags_[k].encode() << ": not found in the event\n";
      continue;
    }
    const Jets& jets = *hJets;
    Producer prod = producerOf(ev, *hJets.provenance(), jetTags_[k].label());

    // constituents -> genParticles index (the constituent Ptrs point into genParticles; if they do
    // not, e.g. a different collection was used as input, fall back to the nearest status 1 particle)
    std::vector<int> jetOf(n, -1);
    for (size_t j = 0; j < jets.size(); ++j)
      for (auto const& c : jets[j].daughterPtrVector()) {
        if (c.id() == hGen.id() && c.key() < n) {
          jetOf[c.key()] = j;
          continue;
        }
        double bestDR = 1e-3;
        size_t best = n;
        for (size_t i = 0; i < n; ++i)
          if (gens[i].status() == 1 && std::fabs(gens[i].pt() - c->pt()) < 1e-3 * c->pt()) {
            double dr = reco::deltaR(gens[i], *c);
            if (dr < bestDR) {
              bestDR = dr;
              best = i;
            }
          }
        if (best < n)
          jetOf[best] = j;
      }
    size_t nClustered = 0, nClusteredPU = 0;
    std::vector<double> puPt(jets.size(), 0.);  // scalar pT of the pileup constituents of every jet
    for (size_t i = 0; i < n; ++i) {
      if (jetOf[i] < 0)
        continue;
      ++nClustered;
      if (isPileup(gens[i])) {
        ++nClusteredPU;
        puPt[jetOf[i]] += gens[i].pt();
      }
    }

    // the jets matched to the quarks (nearest within R)
    Quark q[2] = {quark[0], quark[1]};
    for (int s = 0; s < 2; ++s) {
      if (!q[s].found)
        continue;
      double bestDR = prod.rParam > 0. ? prod.rParam : 0.4;
      for (size_t j = 0; j < jets.size(); ++j) {
        double dr = reco::deltaR(jets[j], gens[q[s].last]);
        if (dr < bestDR) {
          bestDR = dr;
          q[s].jet = j;
          q[s].dR = dr;
        }
      }
    }

    // histograms and printout
    Hists& h = hists_[k];
    h.nJets->Fill(jets.size());
    for (auto const& j : jets) {
      h.jetPt->Fill(j.pt());
      h.jetEta->Fill(j.eta());
      h.nConst->Fill(j.numberOfDaughters());
      h.etaPhiPt->Fill(j.eta(), j.phi(), j.pt());
    }
    std::cout << "  " << jetTags_[k].label() << " (process " << prod.process;
    if (prod.known)
      std::cout << ", " << prod.algorithm << " R=" << prod.rParam << ", jetPtMin=" << prod.jetPtMin << " GeV, src="
                << prod.src;
    std::cout << "): nJets=" << jets.size() << " nClustered=" << nClustered << "/" << nStatus1 << " status 1 particles";
    if (signalCollisionId_ >= 0)
      std::cout << " (signal " << nClustered - nClusteredPU << "/" << nStatus1 - nPileup1 << ", pileup " << nClusteredPU
                << "/" << nPileup1 << ")";
    std::cout << "\n";
    for (int s = 0; s < 2; ++s) {
      std::cout << "    " << (s ? "bbar" : "b") << " quark -> ";
      if (!q[s].found) {
        std::cout << "no hard quark\n";
        continue;
      }
      if (q[s].jet < 0) {
        std::cout << "no jet within R=" << prod.rParam << "\n";
        continue;
      }
      const reco::GenJet& j = jets[q[s].jet];
      double resp = j.pt() / gens[q[s].last].pt();
      std::cout << "jet [" << q[s].jet << "] pt=" << j.pt() << " eta=" << j.eta() << " phi=" << j.phi() << " nConst="
                << j.numberOfDaughters() << " dR=" << q[s].dR << " pt(jet)/pt(quark)=" << resp;
      if (signalCollisionId_ >= 0)
        std::cout << " pileupPt=" << puPt[q[s].jet] << " (" << 100. * puPt[q[s].jet] / j.pt() << "%)";
      std::cout << "\n";
      (s ? h.ptBbarJet : h.ptBjet)->Fill(j.pt());
      h.response->Fill(resp);
    }
    if (q[0].jet >= 0 && q[1].jet >= 0) {
      math::XYZTLorentzVector net = jets[q[0].jet].p4() + jets[q[1].jet].p4();
      h.ptNet->Fill(net.pt());
      std::cout << "    net b jet + bbar jet: |sum pT|=" << net.pt() << " (px=" << net.px() << " py=" << net.py()
                << ") scalar sum pT=" << jets[q[0].jet].pt() + jets[q[1].jet].pt()
                << " dphi=" << std::fabs(reco::deltaPhi(jets[q[0].jet].phi(), jets[q[1].jet].phi()))
                << (q[0].jet == q[1].jet ? " (same jet)" : "") << "\n";
    }
    for (size_t j = 0; j < jets.size(); ++j) {
      std::cout << "    [" << j << "] pt=" << jets[j].pt() << " eta=" << jets[j].eta() << " phi=" << jets[j].phi()
                << " mass=" << jets[j].mass() << " nConst=" << jets[j].numberOfDaughters();
      if (signalCollisionId_ >= 0)
        std::cout << " pileupPt=" << puPt[j] << " (" << 100. * puPt[j] / jets[j].pt() << "%)";
      if (printConstituents_) {
        std::cout << " constituents(genParticles idx: pdg pt)=";
        for (auto const& c : jets[j].daughterPtrVector())
          std::cout << " " << (c.id() == hGen.id() ? Form("[%u]", (unsigned)c.key()) : "[?]") << ":" << c->pdgId() << ":"
                    << c->pt();
      }
      std::cout << "\n";
    }

    if (savePDF_)
      draw(k, ev, gens, jets, jetOf, puPt, q, prod);
  }
}

// ----------------------------------------------------------------------------- drawing
void GenJetAnalyzer::draw(size_t k, const edm::Event& ev, const Gens& gens, const Jets& jets,
                          const std::vector<int>& jetOf, const std::vector<double>& puPt, const Quark quark[2],
                          const Producer& prod) {
  const size_t n = gens.size();
  const std::string& label = jetTags_[k].label();
  std::vector<std::unique_ptr<TObject>> keep;  // primitives must outlive Print()

  canvas_->Clear();
  canvas_->SetLeftMargin(0.06);
  canvas_->SetRightMargin(0.21);
  canvas_->SetTopMargin(0.19);
  canvas_->SetBottomMargin(0.08);
  canvas_->SetGrid();

  auto frame = std::make_unique<TH2D>(Form("frame%zu", k), ";#eta;#phi", 10, -etaMax_, etaMax_, 10, -M_PI, M_PI);
  frame->GetXaxis()->SetTitleOffset(0.9);
  frame->GetYaxis()->SetTitleOffset(0.6);
  frame->Draw("axis");

  int nClamped = 0;
  auto addPoint = [&](int color, int style, double eta, double phi, double size) {
    bool clamped = std::fabs(eta) > etaMax_;
    if (clamped) {
      eta = eta > 0 ? etaMax_ - 0.05 : -etaMax_ + 0.05;
      ++nClamped;
    }
    auto g = std::make_unique<TGraph>(1);
    g->SetPoint(0, eta, phi);
    g->SetMarkerColor(color);
    g->SetMarkerStyle(clamped ? 5 : style);
    g->SetMarkerSize(size * (clamped ? 1.3 : 1.));
    g->Draw("P same");
    keep.push_back(std::move(g));
    return eta;
  };
  auto label_ = [&](double eta, double phi, const char* s, double size, int color) {  // next to the point (eta, phi)
    const bool left = eta > etaMax_ - 1.2;  // near the right border: to the left of the point, right-aligned
    auto t = std::make_unique<TLatex>(eta + (left ? -0.07 : 0.07), phi, s);
    t->SetTextSize(size);
    t->SetTextColor(color);
    t->SetTextAlign(left ? 32 : 12);
    t->Draw();
    keep.push_back(std::move(t));
  };
  // circle of radius R around (eta0, phi0), wrapped in phi and clipped to the frame
  auto circle = [&](double eta0, double phi0, double R, int color, int width, int style) {
    for (double shift : {0., 2. * M_PI, -2. * M_PI}) {
      double pc = phi0 + shift;
      if (pc + R < -M_PI || pc - R > M_PI)
        continue;
      std::unique_ptr<TGraph> g;
      auto flush = [&]() {
        if (g && g->GetN() > 1) {
          g->SetLineColor(color);
          g->SetLineWidth(width);
          g->SetLineStyle(style);
          g->Draw("L same");
          keep.push_back(std::move(g));
        }
        g.reset();
      };
      for (int p = 0; p <= 90; ++p) {
        double a = 2. * M_PI * p / 90.;
        double e = eta0 + R * std::cos(a), f = pc + R * std::sin(a);
        if (f < -M_PI || f > M_PI || std::fabs(e) > etaMax_) {
          flush();
          continue;
        }
        if (!g)
          g = std::make_unique<TGraph>();
        g->SetPoint(g->GetN(), e, f);
      }
      flush();
    }
  };

  static const int kJetColor[8] = {kOrange + 7, kGreen + 2, kCyan + 2, kViolet + 1, kAzure + 1, kPink + 7, kSpring - 6, kYellow + 2};
  const int sysColor[2] = {kRed, kBlue};
  auto jetColor = [&](int j) {
    if (quark[0].jet == j)
      return sysColor[0];
    if (quark[1].jet == j)
      return sysColor[1];
    return j < 8 ? kJetColor[j] : kGray + 1;
  };
  auto isBJet = [&](int j) { return quark[0].jet == j || quark[1].jet == j; };

  // the status 1 particles: signal / pileup, clustered / not
  const bool withPileup = signalCollisionId_ >= 0;
  size_t nStatus1 = 0, nPileup1 = 0, nUnclustered = 0, nUnclusteredPU = 0;
  std::set<int> collisions;
  for (size_t i = 0; i < n; ++i) {
    if (gens[i].status() != 1 || gens[i].pt() <= 0.)
      continue;
    ++nStatus1;
    nUnclustered += jetOf[i] < 0;
    if (isPileup(gens[i])) {
      ++nPileup1;
      nUnclusteredPU += jetOf[i] < 0;
      collisions.insert(gens[i].collisionId());
    }
  }
  // particles: the pileup underneath the signal, and within each the particles in no jet (grey,
  // open markers) underneath the constituents (filled, jet colour); pileup = triangles, signal = circles
  for (int pass = 0; pass < 4; ++pass) {
    const bool pileup = pass % 2 == 0, clustered = pass >= 2;
    const int style = clustered ? (pileup ? 22 : 20) : (pileup ? 26 : 24);
    for (size_t i = 0; i < n; ++i) {
      if (gens[i].status() != 1 || gens[i].pt() <= 0. || isPileup(gens[i]) != pileup || (jetOf[i] >= 0) != clustered)
        continue;
      const int color = clustered ? jetColor(jetOf[i]) : (pileup ? kGray : kGray + 1);
      addPoint(color, style, gens[i].eta(), gens[i].phi(), markerSize(gens[i].pt()));
    }
  }
  // jets: circle, axis and label (b / bbar jets on top); with pileup there are hundreds of jets, so
  // only the ones above labelPtMin (and the b / bbar jets) get a circle, an axis marker and a label
  double R = prod.rParam > 0. ? prod.rParam : 0.4;
  size_t nDrawn = 0;
  for (int pass = 0; pass < 2; ++pass)
    for (size_t j = 0; j < jets.size(); ++j) {
      if (isBJet(j) != (pass == 1))
        continue;
      int color = jetColor(j);
      bool big = isBJet(j);
      if (withPileup && !big && jets[j].pt() < labelPtMin_)
        continue;
      ++nDrawn;
      circle(jets[j].eta(), jets[j].phi(), R, color, big ? 3 : (j < 8 ? 2 : 1), big ? 1 : (j < 8 ? 1 : 2));
      double eta = addPoint(color, 29, jets[j].eta(), jets[j].phi(), markerSize(jets[j].pt()) * 1.3);
      if (big || jets[j].pt() >= labelPtMin_)
        label_(eta, jets[j].phi() + 0.1, Form("[%zu] %.1f", j, jets[j].pt()), big ? 0.022 : 0.017, color);
    }
  // quark directions
  for (int s = 0; s < 2; ++s) {
    if (!quark[s].found)
      continue;
    const reco::GenParticle& g = gens[quark[s].last];
    double eta = addPoint(sysColor[s], 30, g.eta(), g.phi(), 2.4);
    label_(eta, g.phi() - 0.2, Form("%s quark %.1f", s ? "#bar{b}" : "b", g.pt()), 0.02, sysColor[s]);
  }

  // header
  auto text = [&](double x, double y, const char* s, double size, int color = kBlack, int align = 11) {
    auto t = std::make_unique<TLatex>(x, y, s);
    t->SetNDC();
    t->SetTextSize(size);
    t->SetTextColor(color);
    t->SetTextAlign(align);
    t->Draw();
    keep.push_back(std::move(t));
  };
  std::string title = label + " (process " + prod.process + ")";
  if (prod.known)
    title += Form(": %s R = %.1f, jet p_{T} > %.0f GeV, input %s", prod.algorithm == "AntiKt" ? "anti-k_{T}" : prod.algorithm.c_str(),
                  prod.rParam, prod.jetPtMin, prod.src.c_str());
  text(0.06, 0.955, title.c_str(), title.size() > 95 ? 0.025 : 0.028);  // long input / process names: keep clear of the right panel
  // right panel: event, sample label (one line per '|'-separated part)
  text(0.80, 0.955, Form("run %u  lumi %u  event %llu", ev.id().run(), ev.luminosityBlock(), ev.id().event()), 0.02);
  if (!sampleLabel_.empty()) {
    size_t start = 0;
    for (int l = 0; start <= sampleLabel_.size() && l < 5; ++l) {
      size_t bar = sampleLabel_.find('|', start);
      std::string line = sampleLabel_.substr(start, bar == std::string::npos ? std::string::npos : bar - start);
      text(0.80, 0.928 - 0.025 * l, line.c_str(), 0.018, l == 0 ? kBlack : kGray + 3);
      if (bar == std::string::npos)
        break;
      start = bar + 1;
    }
  }
  std::string info;
  if (withPileup)
    info = Form("%zu jets, %zu drawn (p_{T} > %.0f GeV); clustered: %zu of %zu signal, %zu of %zu pileup (%zu collisions) status 1 particles",
                jets.size(), nDrawn, labelPtMin_, nStatus1 - nPileup1 - (nUnclustered - nUnclusteredPU), nStatus1 - nPileup1,
                nPileup1 - nUnclusteredPU, nPileup1, collisions.size());
  else
    info = Form("%zu jets; %zu of %zu status 1 particles clustered; marker size ~ log p_{T}; label = [jet] p_{T} [GeV] (p_{T} > %.0f GeV)",
                jets.size(), nStatus1 - nUnclustered, nStatus1, labelPtMin_);
  if (nClamped)
    info += withPileup ? Form("; %d beyond |#eta| = %.0f (x)", nClamped, etaMax_)
                       : Form("; %d beyond |#eta| = %.0f on the border (x)", nClamped, etaMax_);
  text(0.06, 0.922, info.c_str(), withPileup ? 0.0165 : 0.018, kGray + 3);
  for (int s = 0; s < 2; ++s) {
    std::string line = s ? "#bar{b}" : "b";
    if (!quark[s].found)
      line += " quark: not found";
    else {
      const reco::GenParticle& g = gens[quark[s].last];
      line += Form(" quark (status %d): p_{T} = %.2f GeV, #eta = %.2f, #phi = %.2f  #rightarrow  ", g.status(), g.pt(), g.eta(),
                   g.phi());
      if (quark[s].jet < 0)
        line += Form("no jet within #DeltaR < %.1f", R);
      else {
        const reco::GenJet& j = jets[quark[s].jet];
        line += Form("jet [%d]: p_{T} = %.2f GeV, #eta = %.2f, #phi = %.2f, n = %zu, #DeltaR = %.3f, p_{T}(jet) / p_{T}(q) = %.3f",
                     quark[s].jet, j.pt(), j.eta(), j.phi(), (size_t)j.numberOfDaughters(), quark[s].dR, j.pt() / g.pt());
      }
    }
    text(0.06, 0.891 - 0.028 * s, line.c_str(), 0.017, sysColor[s]);
  }
  if (quark[0].jet >= 0 && quark[1].jet >= 0) {
    const reco::GenJet& jb = jets[quark[0].jet];
    const reco::GenJet& jbbar = jets[quark[1].jet];
    math::XYZTLorentzVector net = jb.p4() + jbbar.p4();
    std::string line = Form("b jet + #bar{b} jet: |#vec{p}_{T}(b) + #vec{p}_{T}(#bar{b})| = %.2f GeV (p_{x} = %.2f, p_{y} = %.2f),  #Sigma|p_{T}| = %.2f GeV,  #Delta#phi = %.2f,  m = %.1f GeV",
                            net.pt(), net.px(), net.py(), jb.pt() + jbbar.pt(), std::fabs(reco::deltaPhi(jb.phi(), jbbar.phi())),
                            net.mass());
    if (quark[0].jet == quark[1].jet)
      line += "  (same jet)";
    if (withPileup)
      line += Form(",  pileup p_{T} in b / #bar{b} jet = %.1f / %.1f GeV", puPt[quark[0].jet], puPt[quark[1].jet]);
    text(0.06, 0.835, line.c_str(), withPileup ? 0.0175 : 0.019, kMagenta + 2);
  }

  // legend, marker-size scale and the list of the leading jets
  auto leg = std::make_unique<TLegend>(0.80, 0.56, 0.995, 0.80);
  leg->SetBorderSize(0);
  leg->SetTextSize(0.017);
  std::vector<std::unique_ptr<TMarker>> legMarkers;
  auto legEntry = [&](int color, int style, const char* entry) {
    auto m = std::make_unique<TMarker>(0, 0, style);
    m->SetMarkerColor(color);
    m->SetMarkerSize(1.6);
    leg->AddEntry(m.get(), entry, "p");
    legMarkers.push_back(std::move(m));
  };
  if (quark[0].jet >= 0)
    legEntry(kRed, 29, "b jet: axis, circle of radius R");
  if (quark[1].jet >= 0)
    legEntry(kBlue, 29, "#bar{b} jet");
  size_t nOther = 0;
  for (size_t j = 0; j < jets.size(); ++j)
    nOther += !isBJet(j);
  if (nOther)
    legEntry(kOrange + 7, 29, withPileup ? Form("other jets (%zu): p_{T} > %.0f GeV drawn", nOther, labelPtMin_)
                                         : Form("other jets (%zu), leading 8 in colour", nOther));
  if (withPileup) {
    legEntry(kGray + 2, 20, "signal constituents (jet colour)");
    legEntry(kGray + 2, 22, "pileup constituents (jet colour)");
    legEntry(kGray + 1, 24, Form("signal, not in a jet (%zu)", nUnclustered - nUnclusteredPU));
    legEntry(kGray, 26, Form("pileup, not in a jet (%zu)", nUnclusteredPU));
  } else {
    legEntry(kGray + 2, 20, "jet constituents (jet colour)");
    legEntry(kGray + 1, 24, Form("status 1, not in a jet (%zu)", nUnclustered));
  }
  if (quark[0].found || quark[1].found)
    legEntry(kBlack, 30, "b / #bar{b} quark, last copy");
  leg->Draw();

  text(0.80, 0.52, "marker size:", 0.02);
  const double scalePt[3] = {1., 10., 100.};
  for (int s = 0; s < 3; ++s) {
    auto m = std::make_unique<TMarker>(0.815, 0.485 - 0.035 * s, 20);
    m->SetNDC();
    m->SetMarkerColor(kGray + 2);
    m->SetMarkerSize(markerSize(scalePt[s]));
    m->Draw();
    keep.push_back(std::move(m));
    text(0.835, 0.48 - 0.035 * s, Form("p_{T} = %.0f GeV", scalePt[s]), 0.02);
  }
  text(0.80, 0.36, withPileup ? "leading jets: [i] p_{T}  #eta  #phi  n  pileup" : "leading jets: [i] p_{T}  #eta  #phi  n", 0.019);
  const size_t nList = std::min<size_t>(jets.size(), 11);
  for (size_t j = 0; j < nList; ++j) {
    std::string row = Form("[%zu] %.1f  %.2f  %.2f  %zu", j, jets[j].pt(), jets[j].eta(), jets[j].phi(),
                           (size_t)jets[j].numberOfDaughters());
    if (withPileup)
      row += Form("  %.0f%%", 100. * puPt[j] / jets[j].pt());
    text(0.805, 0.335 - 0.026 * j, row.c_str(), 0.017, jetColor(j));
  }
  if (jets.size() > nList)
    text(0.805, 0.335 - 0.026 * nList, Form("... %zu more", jets.size() - nList), 0.017, kGray + 2);

  canvas_->Update();
  if (pdfFile_.empty()) {
    canvas_->Print(Form("%s_%s_run%u_event%llu.pdf", outputPrefix_.c_str(), label.c_str(), ev.id().run(), ev.id().event()));
    return;
  }
  if (!pdfOpen_) {
    if (openPdf_)
      canvas_->Print((pdfFile_ + "[").c_str());
    pdfOpen_ = true;
  }
  canvas_->Print(pdfFile_.c_str());
}

void GenJetAnalyzer::endJob() {
  if (pdfOpen_ && closePdf_)
    canvas_->Print((pdfFile_ + "]").c_str());
}

DEFINE_FWK_MODULE(GenJetAnalyzer);
