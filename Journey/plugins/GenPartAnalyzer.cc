// GenPartAnalyzer: eta-phi maps of the generator record, one per generation stage.
//
// Every genParticle is drawn as a marker at (eta, phi) whose size grows with pT (the pT
// value is written next to the b, bbar and every particle above labelPtMin). One PDF is
// written per stage and event (<outputPrefix>_stageN_<name>_run<R>_event<E>.pdf; ROOT can
// keep only one multi-page PDF open at a time), or, if pdfFile is set, all plots go as pages
// into that single PDF (per event the drawn stages; with openPdf / closePdf = false the
// file is opened / closed by another module of the same job that prints into the same PDF,
// e.g. GenJetAnalyzer, which then appends its pages after the stages of every event), plus TH2 (eta, phi, weight pT)
// per stage in the TFileService file. maxStage limits the stages that are analysed, for a
// sample generated with later Pythia8 levels switched off (ME only: 1, + parton shower: 2,
// + MPI: 3, + hadronisation: 5); drawStages picks the analysed stages that get a page (empty: all of
// them; the others are still analysed and printed, so that a chapter shows only the page of its own
// stage while the legend keeps the stages of the sample); sampleLabel is printed in the right panel
// of every plot, '|' separates lines.
//
// The Pythia8 status codes kept in genParticles (Pythia8 manual, "Particle Properties")
// give the stage in which each particle (or copy of a particle) was produced:
//   1 hard process (ME)       status 21-29           (23 = outgoing b, bbar)
//   2 parton shower           status 41-49 ISR, 51-59 FSR (of the hard process: an initial-state emission belongs to the
//                            scattering its backward chain of incoming partons leads to, and one that leads to an MPI is
//                            produced in stage 3)
//   3 MPI / beam remnants     status 31-39 MPI, 61-69 primordial kT + beam remnants; a final parton written with
//                            status 1 (hadronisation off: the copy that carries the primordial kT, Pythia8 62/63)
//   4 hadronisation           status 71-79 (partons entering the strings) and the
//                            primary hadrons: status 1/2 with a status 71-79 mother
//   5 hadron decays           status 1/2 with a decayed hadron/lepton (status 1/2) mother
// A particle is drawn in the plot of stage N if it exists at the end of stage N: it was
// produced in a stage <= N, all its outgoing ancestors were produced in stages <= N and none
// of its daughters was produced in a stage <= N (it has not yet been replaced by a later
// copy, split, hadronised or decayed). Incoming partons and the beams (status 4, 21, 31,
// 41, 42, 45, 46, 53, 61) are along the beam axis (pT = 0) and are never drawn.
//
// The marker colour gives the origin of the particle: the process that created it. A copy
// (status 44, 52, 62, ...: same pdg id, single daughter of its mother) and the radiator of an
// FSR branching (status 51, first daughter of its mother with the same pdg id) inherit the
// origin of their mother, so a quark produced in an MPI stays "MPI" after it radiated and
// received its primordial kT.
//
// In a record with multiparton interactions (status 31-39) a parton is also attributed to the SCATTERING it belongs to
// (scatteringOf): the hard process, one of the MPI, or a beam remnant, following its mothers up through copies,
// radiators and emissions and, from an incoming parton, down the chain of incoming partons to its scattering (the walk
// of the FWLite script of chapter 04). The marker then shows both: the ISR and FSR of the hard process keep their
// colours ("hard-process ISR / FSR", the same partons on the page of stage 2 and on the page of stage 3), everything
// that belongs to an MPI is cyan (squares the outgoing partons of the interactions, triangles their ISR and FSR), the
// remnants grey; the header of stage 3 counts the drawn partons by scattering. Without MPI nothing changes (ISR, FSR).
//
// The b and bbar of the hard process are followed through the stages: their same-flavour
// copies (stages 1-3), the primary b hadron nearest in Delta R to the last quark copy
// (stage 4) and the stable decay products of that hadron (stage 5). The pT of each and the
// net (vector sum) pT of b + bbar are printed on every plot and to std::cout.
//
// With coneR > 0 (default 0: off) the cone Delta R < coneR around each of the two systems is added: every particle
// alive at the stage except neutrinos within coneR of the system direction (what a jet of radius coneR would hold).
// The cone is drawn as a dashed circle with its pT, its n and pT are added to the header and printed to std::cout
// ("  cone ..."), and the fuller cone (at least two particles) is drawn again, magnified, in an inset placed in the
// corner of the frame that hides the least: every particle of it with its name and pT, which the main frame cannot
// separate (their labels are not written in the main frame); the other cone gets a second inset, in another corner,
// when it holds at least three particles (the two clouds of decay products of stage 5). A neutrino is drawn in the inset
// with its label in parentheses: it is not counted in the cone. In a record with MPI the cone of stage 3 also gives what of
// it belongs to the underlying event (UE: the partons of the MPI and the beam remnants, scalar sum of their pT).
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TGraph.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLine.h"
#include "TMarker.h"
#include "TPad.h"
#include "TROOT.h"
#include "TStyle.h"

#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/Math/interface/deltaPhi.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

class GenPartAnalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit GenPartAnalyzer(const edm::ParameterSet& ps);
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  // origin of a particle (the process that created it); each maps to one of the 5 stages
  // MPIISR, MPIFSR: an ISR / FSR emission that belongs to an MPI (only for drawing, see scatteringOf)
  enum Origin { BEAM = 0, HARD, ISR, FSR, MPI, MPIISR, MPIFSR, REMNANT, STRING, HADRON, DECAY, OTHER, NORIG };
  enum Scattering { SC_NONE = 0, SC_HARD = 1, SC_MPI = 3, SC_REMNANT = 6 };
  static constexpr int kNStages = 5;
  static constexpr const char* kStageName[kNStages + 1] = {
      "", "hardProcess", "partonShower", "mpiBeamRemnants", "hadronisation", "hadronDecays"};
  static constexpr const char* kStageTitle[kNStages + 1] = {"",
                                                            "1. hard process (ME): q#bar{q} #rightarrow b#bar{b}",
                                                            "2. + parton shower (ISR + FSR)",
                                                            "3. + MPI / beam remnants",
                                                            "4. + hadronisation (primary hadrons)",
                                                            "5. + hadron decays (= genParticles status 1)"};
  static constexpr const char* kOriginName[NORIG] = {"beam",          "hard process",  "ISR",           "FSR",
                                                     "MPI",           "MPI ISR",       "MPI FSR",       "beam remnant",
                                                     "string parton", "primary hadron", "decay product", "other"};
  // the names in a record with MPI: the ISR and FSR of the hard process apart from those of the MPI
  static constexpr const char* kOriginNameMPI[NORIG] = {"beam",          "hard process",   "hard-process ISR", "hard-process FSR",
                                                        "MPI parton",    "MPI ISR",        "MPI FSR",          "beam remnant",
                                                        "string parton", "primary hadron", "decay product",    "other"};
  static constexpr int kOriginColor[NORIG] = {kBlack,     kBlack,    kOrange + 7, kGreen + 2,  kCyan + 2, kCyan + 2,
                                              kCyan + 2,  kGray + 2, kViolet + 1, kViolet + 1, kGray + 1, kBlack};
  static constexpr int kOriginMarker[NORIG] = {1, 24, 26, 32, 25, 26, 32, 27, 24, 20, 20, 24};

  struct Sys {  // one of the two b systems (b or bbar) at a given stage
    std::vector<size_t> idx;
    math::XYZTLorentzVector p4;
    std::string what;   // long description for the header
    std::string label;  // short label next to the marker (per particle at stage 5)
    bool hasCone = false;            // coneR > 0 and the system exists
    std::vector<size_t> coneIdx;     // the particles within coneR of p4
    math::XYZTLorentzVector coneP4;  // their sum
    bool hasUE = false;              // stage 3 of a record with MPI: the underlying-event part of the cone is given
    size_t ueN = 0;                  // the cone particles from an MPI or a beam remnant
    double uePt = 0.;                // the scalar sum of their pT
  };

  using Coll = reco::GenParticleCollection;

  void beginJob() override;
  void analyze(const edm::Event& ev, const edm::EventSetup&) override;
  void endJob() override;

  static bool isIncoming(int status);
  static bool isParton(int pdg);  // quark, gluon, diquark, string / cluster
  static Origin originOfStatus(int status);
  static int stageOfOrigin(int o);
  static int bContent(int pdg);  // +n: n b quarks, -n: n anti-b quarks, 0: none
  void classify(size_t i, const Coll& gens, const std::vector<char>& inc, std::vector<int>& stage,
                std::vector<int>& origin) const;
  bool lineageOk(size_t i, int stage, const Coll& gens, const std::vector<int>& st, const std::vector<char>& inc,
                 std::vector<char>& memo) const;
  int scatteringStage(size_t k, const Coll& gens, int depth = 0) const;
  int scatteringOf(size_t i, const Coll& gens, const std::vector<char>& inc, std::vector<int>& memo) const;
  void descendants(size_t i, const Coll& gens, std::vector<char>& out) const;
  static std::string pdgName(int pdg);
  static double markerSize(double pt);
  void draw(int stage, const edm::Event& ev, const Coll& gens, const std::vector<char>& alive,
            const std::vector<int>& origin, const std::vector<Sys>& systems);
  void drawZoom(int stage, const Coll& gens, const std::vector<char>& alive, const std::vector<int>& origin,
                const std::vector<int>& member, const std::vector<Sys>& systems, int k,
                std::vector<std::pair<double, double>>& usedCorners);

  edm::EDGetTokenT<Coll> genParticlesTok_;
  std::string outputPrefix_, pdfFile_, sampleLabel_;
  double etaMax_, labelPtMin_, coneR_;
  bool savePDF_, printParticles_, openPdf_, closePdf_;
  int maxStage_;
  std::vector<int> drawStages_;  // the stages that get a page; empty: every analysed stage
  bool pdfOpen_ = false;  // the multi-page pdfFile_ is opened on the first page and closed in endJob
  // the event being drawn: the record has multiparton interactions (status 31-39), their number, and the scattering
  // of every particle (Scattering)
  bool hasMPI_ = false;
  int nMPI_ = 0;
  std::vector<int> scatter_;

  std::unique_ptr<TCanvas> canvas_;
  TH2D* hEtaPhiPt_[kNStages + 1] = {};
  TH1D* hPtB_[kNStages + 1] = {};
  TH1D* hPtBbar_[kNStages + 1] = {};
  TH1D* hPtNet_[kNStages + 1] = {};
};

GenPartAnalyzer::GenPartAnalyzer(const edm::ParameterSet& ps)
    : genParticlesTok_(consumes<Coll>(ps.getParameter<edm::InputTag>("genParticles"))),
      outputPrefix_(ps.getParameter<std::string>("outputPrefix")),
      pdfFile_(ps.getParameter<std::string>("pdfFile")),
      sampleLabel_(ps.getParameter<std::string>("sampleLabel")),
      etaMax_(ps.getParameter<double>("etaMax")),
      labelPtMin_(ps.getParameter<double>("labelPtMin")),
      coneR_(ps.getParameter<double>("coneR")),
      savePDF_(ps.getParameter<bool>("savePDF")),
      printParticles_(ps.getParameter<bool>("printParticles")),
      openPdf_(ps.getParameter<bool>("openPdf")),
      closePdf_(ps.getParameter<bool>("closePdf")),
      maxStage_(std::min(std::max(ps.getParameter<int>("maxStage"), 1), kNStages)),
      drawStages_(ps.getParameter<std::vector<int>>("drawStages")) {
  usesResource(TFileService::kSharedResource);
  edm::Service<TFileService> fs;
  for (int s = 1; s <= kNStages; ++s) {
    TFileDirectory dir = fs->mkdir(Form("stage%d_%s", s, kStageName[s]));
    hEtaPhiPt_[s] = dir.make<TH2D>("etaPhiPt", Form("%s;#eta;#phi;#Sigma p_{T} [GeV]", kStageTitle[s]), 120, -etaMax_,
                                   etaMax_, 64, -M_PI, M_PI);
    hPtB_[s] = dir.make<TH1D>("ptB", Form("%s;p_{T}(b) [GeV];events", kStageTitle[s]), 100, 0., 500.);
    hPtBbar_[s] = dir.make<TH1D>("ptBbar", Form("%s;p_{T}(#bar{b}) [GeV];events", kStageTitle[s]), 100, 0., 500.);
    hPtNet_[s] = dir.make<TH1D>("ptNetBBbar",
                                Form("%s;|#vec{p}_{T}(b) + #vec{p}_{T}(#bar{b})| [GeV];events", kStageTitle[s]), 100,
                                0., 200.);
  }
}

void GenPartAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("genParticles", edm::InputTag("genParticles"));
  desc.add<std::string>("outputPrefix", "genPartAnalyzer")
      ->setComment("PDFs are written as <outputPrefix>_stageN_<name>_run<R>_event<E>.pdf");
  desc.add<double>("etaMax", 6.)->setComment("eta axis range; particles beyond are drawn on the border with an x marker");
  desc.add<double>("labelPtMin", 5.)->setComment("pT above which the pT value is written next to the marker (b, bbar always)");
  desc.add<double>("coneR", 0.)
      ->setComment("if > 0: the cone Delta R < coneR around the b and the bbar system (circle, pT, inset of the fuller cone; with MPI, its UE part at stage 3)");
  desc.add<bool>("savePDF", true);
  desc.add<std::string>("pdfFile", "")
      ->setComment("if set, all plots are written as pages of this single PDF instead of one PDF per stage and event");
  desc.add<std::string>("sampleLabel", "")
      ->setComment("printed in the right panel of every plot, '|' separates lines (e.g. the generator stage)");
  desc.add<int>("maxStage", kNStages)
      ->setComment("last stage to analyse and draw: 1 hard process, 2 parton shower, 3 MPI, 4 hadronisation, 5 decays");
  desc.add<std::vector<int>>("drawStages", {})
      ->setComment("the analysed stages that get a page (empty: all); the others are analysed and printed, but not drawn");
  desc.add<bool>("printParticles", false)->setComment("print every drawn particle of every stage to stdout");
  desc.add<bool>("openPdf", true)->setComment("open pdfFile before the first page (false: another module of the job opens it)");
  desc.add<bool>("closePdf", true)->setComment("close pdfFile in endJob (false: another module of the job closes it)");
  descriptions.addWithDefaultLabel(desc);
}

// ----------------------------------------------------------------------------- classification
bool GenPartAnalyzer::isIncoming(int status) {
  int s = std::abs(status);
  return s == 4 || (s >= 11 && s <= 19) || s == 21 || s == 31 || s == 41 || s == 42 || s == 45 || s == 46 || s == 53 ||
         s == 61;
}

bool GenPartAnalyzer::isParton(int pdg) {
  int a = std::abs(pdg);
  return (a >= 1 && a <= 8) || a == 21 || (a >= 90 && a <= 92) ||
         (a >= 1103 && a <= 5503 && (a / 10) % 10 == 0 && (a % 10 == 1 || a % 10 == 3));
}

GenPartAnalyzer::Origin GenPartAnalyzer::originOfStatus(int status) {
  int s = std::abs(status);
  if (s == 4 || (s >= 11 && s <= 19))
    return BEAM;
  if (s >= 21 && s <= 29)
    return HARD;
  if (s >= 31 && s <= 39)
    return MPI;
  if (s >= 41 && s <= 49)
    return ISR;
  if (s >= 51 && s <= 59)
    return FSR;
  if (s >= 61 && s <= 69)
    return REMNANT;
  if (s >= 71 && s <= 79)
    return STRING;
  if (s >= 81 && s <= 89)
    return HADRON;
  if (s >= 91 && s <= 99)
    return DECAY;
  return OTHER;
}

int GenPartAnalyzer::stageOfOrigin(int o) {
  switch (o) {
    case BEAM:
      return 0;
    case HARD:
    case OTHER:
      return 1;
    case ISR:
    case FSR:
      return 2;
    case MPI:
    case MPIISR:
    case MPIFSR:
    case REMNANT:
      return 3;
    case STRING:
    case HADRON:
      return 4;
    default:
      return 5;
  }
}

// stage: when this entry of the record was produced (status code); status 1/2 particles carry no
// code any more, so primary hadrons are the daughters of partons (status 71-79 or a ministring), decay products
// the daughters of decayed (status 2) particles, anything else (e.g. a final-state photon radiated
// in the shower, the final copy of a hard-process lepton) is produced together with its mother.
// origin: the process that created the particle, inherited by copies and FSR radiators (see header).
void GenPartAnalyzer::classify(size_t i, const Coll& gens, const std::vector<char>& inc, std::vector<int>& stage,
                               std::vector<int>& origin) const {
  if (stage[i] >= 0)
    return;
  stage[i] = 5;  // guard against cycles
  origin[i] = DECAY;
  const reco::GenParticle& g = gens[i];
  int s = std::abs(g.status());
  if (s == 1 && isParton(g.pdgId())) {
    // a final parton of a record that is not hadronised: the copy of a parton that received the primordial kT with the
    // beam remnants (Pythia8 status 62/63, written as status 1), produced in stage 3 with the origin of what it copies
    stage[i] = 3;
    origin[i] = REMNANT;
    if (g.numberOfMothers() == 1) {
      size_t k = g.motherRef(0).key();
      if (!inc[k] && isParton(gens[k].pdgId())) {
        classify(k, gens, inc, stage, origin);
        origin[i] = origin[k];
      }
    }
    return;
  }
  if (s == 1 || s == 2) {
    bool fromString = false, fromDecay = false;
    int best = -1, bestStage = -1;
    for (size_t m = 0; m < g.numberOfMothers(); ++m) {
      size_t k = g.motherRef(m).key();
      int ms = std::abs(gens[k].status());
      if ((ms >= 71 && ms <= 79) || isParton(gens[k].pdgId()))
        fromString = true;  // partons of a ministring (status 62/63) hadronise without status 71-79 copies
      else if (ms == 1 || ms == 2)
        fromDecay = true;
      else {
        classify(k, gens, inc, stage, origin);
        if (stage[k] > bestStage) {
          bestStage = stage[k];
          best = k;
        }
      }
    }
    if (fromString) {
      stage[i] = 4;
      origin[i] = HADRON;
    } else if (fromDecay) {
      stage[i] = 5;
      origin[i] = DECAY;
    } else if (best >= 0) {
      stage[i] = std::max(bestStage, 3);  // stage 0 (beam) mother: a diffractively scattered proton
      origin[i] = inc[best] ? REMNANT : origin[best];
    } else {
      stage[i] = 1;
      origin[i] = OTHER;
    }
    return;
  }
  Origin o = originOfStatus(s);
  stage[i] = stageOfOrigin(o);
  origin[i] = o;
  // an initial-state emission hangs on an incoming parton (status 41), which lineageOk does not follow: it is produced
  // with the scattering that its chain of incoming partons leads to, in stage 3 when that is a multiparton interaction
  if (o == ISR && !inc[i] && g.numberOfMothers() == 1 && inc[g.motherRef(0).key()])
    stage[i] = std::max(stage[i], scatteringStage(g.motherRef(0).key(), gens));
  // copy or FSR radiator: inherit the origin of the mother
  if (g.numberOfMothers() == 1) {
    size_t k = g.motherRef(0).key();
    const reco::GenParticle& m = gens[k];
    bool copy = m.numberOfDaughters() == 1 && m.pdgId() == g.pdgId();
    bool radiator = s == 51 && m.numberOfDaughters() == 2 && m.daughterRef(0).key() == i && m.pdgId() == g.pdgId();
    if ((copy || radiator) && !inc[k]) {
      classify(k, gens, inc, stage, origin);
      origin[i] = origin[k];
    }
  }
}

// true if no outgoing ancestor of i was produced after 'stage' (the walk stops at incoming partons,
// whose own history runs backwards through ISR to the beams)
bool GenPartAnalyzer::lineageOk(size_t i, int stage, const Coll& gens, const std::vector<int>& st,
                                const std::vector<char>& inc, std::vector<char>& memo) const {
  if (memo[i] != 2)
    return memo[i];
  memo[i] = 1;  // guard against cycles
  const reco::GenParticle& g = gens[i];
  for (size_t m = 0; m < g.numberOfMothers(); ++m) {
    size_t k = g.motherRef(m).key();
    if (inc[k])
      continue;
    if (st[k] > stage || !lineageOk(k, stage, gens, st, inc, memo)) {
      memo[i] = 0;
      return false;
    }
  }
  return true;
}

// the stage of the scattering an incoming parton evolves into, following the backward chain of the initial-state
// shower down the record (incoming copies and recoilers, status 41, 42, 45, 46, 53, 61): 1 for the hard process
// (status 21-29), 3 for a multiparton interaction (status 31-39), 0 if none is found
int GenPartAnalyzer::scatteringStage(size_t k, const Coll& gens, int depth) const {
  if (depth > 100)
    return 0;
  const reco::GenParticle& g = gens[k];
  for (size_t d = 0; d < g.numberOfDaughters(); ++d) {
    int s = std::abs(gens[g.daughterRef(d).key()].status());
    if (s >= 21 && s <= 29)
      return 1;
    if (s >= 31 && s <= 39)
      return 3;
  }
  for (size_t d = 0; d < g.numberOfDaughters(); ++d) {
    size_t c = g.daughterRef(d).key();
    int s = std::abs(gens[c].status());
    if (s == 41 || s == 42 || s == 45 || s == 46 || s == 53 || s == 61)
      if (int r = scatteringStage(c, gens, depth + 1))
        return r;
  }
  return 0;
}

// the scattering a particle belongs to (Scattering): an entry of the hard process (status 21-29) or of an MPI (31-39)
// is its own; an incoming parton belongs to the scattering its chain of incoming partons leads to (scatteringStage); a
// parton whose mother is a beam proton is a beam remnant; anything else belongs to the scattering of its mother (copies,
// radiators, emissions, the final copies with the primordial kT). Hadrons and decay products end at SC_NONE or at the
// scattering of their first mother; only partons are drawn by it.
int GenPartAnalyzer::scatteringOf(size_t i, const Coll& gens, const std::vector<char>& inc, std::vector<int>& memo) const {
  if (memo[i] >= 0)
    return memo[i];
  memo[i] = SC_NONE;  // guard against cycles
  const reco::GenParticle& g = gens[i];
  int s = std::abs(g.status()), r = SC_NONE;
  if (s >= 21 && s <= 29)
    r = SC_HARD;
  else if (s >= 31 && s <= 39)
    r = SC_MPI;
  else if (inc[i]) {
    int st = (s == 4 || (s >= 11 && s <= 19)) ? 0 : scatteringStage(i, gens);
    r = st == 1 ? SC_HARD : (st == 3 ? SC_MPI : SC_NONE);
  } else
    for (size_t m = 0; m < g.numberOfMothers() && r == SC_NONE; ++m) {
      size_t k = g.motherRef(m).key();
      int ms = std::abs(gens[k].status());
      r = (ms == 4 || (ms >= 11 && ms <= 19)) ? SC_REMNANT : scatteringOf(k, gens, inc, memo);
    }
  memo[i] = r;
  return r;
}

void GenPartAnalyzer::descendants(size_t i, const Coll& gens, std::vector<char>& out) const {
  const reco::GenParticle& g = gens[i];
  for (size_t d = 0; d < g.numberOfDaughters(); ++d) {
    size_t k = g.daughterRef(d).key();
    if (out[k])
      continue;
    out[k] = 1;
    descendants(k, gens, out);
  }
}

int GenPartAnalyzer::bContent(int pdg) {
  int a = std::abs(pdg);
  if (a < 100 || a > 9999)
    return 0;
  int q1 = (a / 1000) % 10, q2 = (a / 100) % 10, q3 = (a / 10) % 10;
  if (q1 == 0) {  // meson: a positive id (B+ = u bbar, B0 = d bbar, ...) contains an anti-b
    if (q2 == 5 && q3 == 5)
      return 0;  // bottomonium
    if (q2 == 5)
      return pdg > 0 ? -1 : 1;
    return 0;
  }
  int n = (q1 == 5) + (q2 == 5) + (q3 == 5);  // baryon: a positive id contains quarks
  return pdg > 0 ? n : -n;
}

std::string GenPartAnalyzer::pdgName(int pdg) {
  static const std::map<int, std::string> names = {
      {1, "d"},           {2, "u"},           {3, "s"},           {4, "c"},           {5, "b"},
      {6, "t"},           {11, "e^{-}"},      {12, "#nu_{e}"},    {13, "#mu^{-}"},    {14, "#nu_{#mu}"},
      {15, "#tau^{-}"},   {16, "#nu_{#tau}"}, {21, "g"},          {22, "#gamma"},     {111, "#pi^{0}"},
      {211, "#pi^{+}"},   {113, "#rho^{0}"},  {213, "#rho^{+}"},  {221, "#eta"},      {223, "#omega"},
      {331, "#eta'"},     {130, "K_{L}^{0}"}, {310, "K_{S}^{0}"}, {311, "K^{0}"},     {321, "K^{+}"},
      {313, "K^{*0}"},    {323, "K^{*+}"},    {333, "#phi"},      {2212, "p"},        {2112, "n"},
      {3122, "#Lambda"},  {3222, "#Sigma^{+}"}, {3112, "#Sigma^{-}"}, {3212, "#Sigma^{0}"}, {3322, "#Xi^{0}"},
      {3312, "#Xi^{-}"},  {3334, "#Omega^{-}"}, {411, "D^{+}"},   {421, "D^{0}"},     {431, "D_{s}^{+}"},
      {413, "D^{*+}"},    {423, "D^{*0}"},    {433, "D_{s}^{*+}"}, {441, "#eta_{c}"}, {443, "J/#psi"},
      {4122, "#Lambda_{c}^{+}"}, {511, "B^{0}"}, {521, "B^{+}"},  {531, "B_{s}^{0}"}, {541, "B_{c}^{+}"},
      {513, "B^{*0}"},    {523, "B^{*+}"},    {533, "B_{s}^{*0}"}, {553, "#Upsilon"}, {5122, "#Lambda_{b}^{0}"},
      {5232, "#Xi_{b}^{0}"}, {5132, "#Xi_{b}^{-}"}, {5332, "#Omega_{b}^{-}"}};
  auto it = names.find(std::abs(pdg));
  if (it == names.end())
    return Form("pdg %d", pdg);
  std::string n = it->second;
  if (pdg < 0) {
    // antiparticle: flip the charge sign, or add a bar
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

double GenPartAnalyzer::markerSize(double pt) { return 0.6 + 0.9 * std::log10(1. + pt); }

// ----------------------------------------------------------------------------- job
void GenPartAnalyzer::beginJob() {
  gROOT->SetBatch(true);
  gStyle->SetOptStat(0);
  // ROOT writes an A4 (landscape) page with the canvas scaled to the paper width and a CropBox around
  // it; 29 cm wide fills the page for the 1400 x 900 canvas
  gStyle->SetPaperSize(29., 20.);
  canvas_ = std::make_unique<TCanvas>("genPartCanvas", "", 1400, 900);
}

void GenPartAnalyzer::analyze(const edm::Event& ev, const edm::EventSetup&) {
  edm::Handle<Coll> h;
  ev.getByToken(genParticlesTok_, h);
  const Coll& gens = *h;
  const size_t n = gens.size();

  // 1. production stage and origin of every particle
  std::vector<int> stage(n, -1), origin(n, -1);
  std::vector<char> inc(n, 0);
  size_t nStatus1 = 0;
  for (size_t i = 0; i < n; ++i) {
    inc[i] = isIncoming(gens[i].status()) || gens[i].pt() <= 0.;
    nStatus1 += gens[i].status() == 1;
  }
  for (size_t i = 0; i < n; ++i)
    classify(i, gens, inc, stage, origin);

  // 1b. with MPI in the record: the scattering of every particle, and the origin drawn (ISR / FSR of an MPI apart)
  int nStatus31 = 0;
  for (size_t i = 0; i < n; ++i)
    nStatus31 += std::abs(gens[i].status()) == 31;
  nMPI_ = nStatus31 / 2;
  hasMPI_ = nMPI_ > 0;
  scatter_.assign(n, -1);
  std::vector<int> shown(origin);
  if (hasMPI_)
    for (size_t i = 0; i < n; ++i) {
      scatteringOf(i, gens, inc, scatter_);
      if (scatter_[i] == SC_MPI && origin[i] == ISR)
        shown[i] = MPIISR;
      else if (scatter_[i] == SC_MPI && origin[i] == FSR)
        shown[i] = MPIFSR;
    }

  // 2. the hard-process b quarks and their same-flavour copies
  std::vector<size_t> hardB, hardBbar;
  for (size_t i = 0; i < n; ++i)
    if (std::abs(gens[i].pdgId()) == 5 && gens[i].isHardProcess() && !inc[i])
      (gens[i].pdgId() > 0 ? hardB : hardBbar).push_back(i);
  auto chainOf = [&](const std::vector<size_t>& seeds) {
    std::vector<char> chain(n, 0);
    std::vector<size_t> todo(seeds);
    while (!todo.empty()) {
      size_t i = todo.back();
      todo.pop_back();
      if (chain[i])
        continue;
      chain[i] = 1;
      for (size_t d = 0; d < gens[i].numberOfDaughters(); ++d) {
        size_t k = gens[i].daughterRef(d).key();
        if (gens[k].pdgId() == gens[i].pdgId())
          todo.push_back(k);
      }
    }
    return chain;
  };
  std::vector<char> chainB = chainOf(hardB), chainBbar = chainOf(hardBbar);

  std::cout << "\n== GenPartAnalyzer run:lumi:event " << ev.id().run() << ":" << ev.luminosityBlock() << ":"
            << ev.id().event() << " nGenParticles=" << n << " nStatus1=" << nStatus1 << " hard b idx=";
  for (size_t i : hardB)
    std::cout << i << " ";
  std::cout << "hard bbar idx=";
  for (size_t i : hardBbar)
    std::cout << i << " ";
  std::cout << "\n";

  std::vector<size_t> bHadron, bbarHadron;  // primary b hadrons matched to the b / bbar chains (stage 4)
  std::vector<char> bHadronDesc(n, 0), bbarHadronDesc(n, 0);

  for (int s = 1; s <= maxStage_; ++s) {
    // 3. particles alive at the end of stage s
    std::vector<char> lineage(n, 2), alive(n, 0);
    for (size_t i = 0; i < n; ++i) {
      if (inc[i] || stage[i] > s || !lineageOk(i, s, gens, stage, inc, lineage))
        continue;
      bool replaced = false;
      for (size_t d = 0; d < gens[i].numberOfDaughters() && !replaced; ++d)
        replaced = stage[gens[i].daughterRef(d).key()] <= s;
      alive[i] = !replaced;
    }

    // 4. the b and bbar systems at this stage
    std::vector<Sys> sys(2);
    sys[0].what = sys[0].label = "b";
    sys[1].what = sys[1].label = "#bar{b}";
    if (s <= 3) {
      for (size_t i = 0; i < n; ++i)
        if (alive[i]) {
          if (chainB[i])
            sys[0].idx.push_back(i);
          if (chainBbar[i])
            sys[1].idx.push_back(i);
        }
      for (auto& S : sys)
        if (!S.idx.empty())
          S.what += Form(" quark (status %d)", gens[S.idx.front()].status());
    } else if (s == 4) {
      // match every last quark copy to the nearest primary hadron with the right b content
      auto match = [&](const std::vector<char>& chain, int sign, std::vector<size_t>& out, std::vector<char>& desc) {
        std::vector<size_t> lastCopies;
        for (size_t i = 0; i < n; ++i)
          if (chain[i]) {
            bool hasSameFlavourDaughter = false;
            for (size_t d = 0; d < gens[i].numberOfDaughters(); ++d)
              hasSameFlavourDaughter |= gens[gens[i].daughterRef(d).key()].pdgId() == gens[i].pdgId();
            if (!hasSameFlavourDaughter)
              lastCopies.push_back(i);
          }
        std::vector<char> used(n, 0);
        for (size_t q : lastCopies) {
          double bestDR = 1e9;
          size_t best = n;
          for (size_t i = 0; i < n; ++i)
            if (alive[i] && !used[i] && bContent(gens[i].pdgId()) * sign > 0) {
              double dr = reco::deltaR(gens[q], gens[i]);
              if (dr < bestDR) {
                bestDR = dr;
                best = i;
              }
            }
          if (best < n) {
            used[best] = 1;
            out.push_back(best);
            descendants(best, gens, desc);
            std::cout << "  stage 4: " << (sign > 0 ? "b" : "bbar") << " quark [" << q << "] pt=" << gens[q].pt()
                      << " -> primary hadron [" << best << "] pdg=" << gens[best].pdgId() << " pt=" << gens[best].pt()
                      << " dR=" << bestDR << "\n";
          }
        }
      };
      match(chainB, +1, bHadron, bHadronDesc);
      match(chainBbar, -1, bbarHadron, bbarHadronDesc);
      sys[0].idx = bHadron;
      sys[1].idx = bbarHadron;
      for (auto& S : sys)
        if (!S.idx.empty()) {
          S.label = pdgName(gens[S.idx.front()].pdgId());
          S.what += " hadron " + S.label;
        }
    } else {
      for (size_t i = 0; i < n; ++i)
        if (alive[i]) {
          if (bHadronDesc[i])
            sys[0].idx.push_back(i);
          if (bbarHadronDesc[i])
            sys[1].idx.push_back(i);
        }
      for (size_t k = 0; k < 2; ++k) {
        auto const& had = k == 0 ? bHadron : bbarHadron;
        if (!had.empty())
          sys[k].what += " hadron " + pdgName(gens[had.front()].pdgId()) + Form(" decay products (n=%zu)", sys[k].idx.size());
      }
    }
    for (auto& S : sys)
      for (size_t i : S.idx)
        S.p4 += gens[i].p4();
    // the cones around the two systems: the particles alive at this stage, except neutrinos, within coneR of the axis
    if (coneR_ > 0)
      for (auto& S : sys) {
        if (S.idx.empty())
          continue;
        S.hasCone = true;
        for (size_t i = 0; i < n; ++i) {
          int a = std::abs(gens[i].pdgId());
          if (alive[i] && a != 12 && a != 14 && a != 16 &&
              reco::deltaR(S.p4.eta(), S.p4.phi(), gens[i].eta(), gens[i].phi()) < coneR_) {
            S.coneIdx.push_back(i);
            S.coneP4 += gens[i].p4();
            if (scatter_[i] == SC_MPI || scatter_[i] == SC_REMNANT) {
              ++S.ueN;
              S.uePt += gens[i].pt();
            }
          }
        }
        S.hasUE = hasMPI_ && s == 3;
        std::sort(S.coneIdx.begin(), S.coneIdx.end(), [&](size_t a, size_t b) { return gens[a].pt() > gens[b].pt(); });
      }

    // 5. histograms and printout
    size_t nAlive = 0;
    for (size_t i = 0; i < n; ++i)
      if (alive[i]) {
        ++nAlive;
        hEtaPhiPt_[s]->Fill(gens[i].eta(), gens[i].phi(), gens[i].pt());
      }
    math::XYZTLorentzVector net = sys[0].p4 + sys[1].p4;
    if (!sys[0].idx.empty())
      hPtB_[s]->Fill(sys[0].p4.pt());
    if (!sys[1].idx.empty())
      hPtBbar_[s]->Fill(sys[1].p4.pt());
    if (!sys[0].idx.empty() && !sys[1].idx.empty())
      hPtNet_[s]->Fill(net.pt());
    std::cout << "  stage " << s << " (" << kStageName[s] << "): nParticles=" << nAlive;
    for (auto const& S : sys) {
      std::cout << " | " << S.what << ":";
      if (S.idx.empty()) {
        std::cout << " none";
        continue;
      }
      std::cout << " pt=" << S.p4.pt() << " eta=" << S.p4.eta() << " phi=" << S.p4.phi() << " idx=[";
      for (size_t k = 0; k < S.idx.size() && k < 8; ++k)
        std::cout << (k ? "," : "") << S.idx[k];
      std::cout << (S.idx.size() > 8 ? ",...]" : "]");
    }
    std::cout << " | net b+bbar: |sum pT|=" << net.pt() << " (px=" << net.px() << " py=" << net.py()
              << ") scalar sum pT=" << sys[0].p4.pt() + sys[1].p4.pt() << "\n";
    if (hasMPI_ && s <= 3) {  // the drawn particles by scattering and by the origin drawn
      std::map<int, int> bySc;
      std::vector<int> byOrigin(NORIG, 0);
      for (size_t i = 0; i < n; ++i)
        if (alive[i]) {
          ++bySc[scatter_[i]];
          ++byOrigin[shown[i]];
        }
      std::cout << "  by scattering (stage " << s << "): hard process " << bySc[SC_HARD] << ", MPI " << bySc[SC_MPI]
                << " (of " << nMPI_ << " interactions), beam remnants " << bySc[SC_REMNANT] << ", none " << bySc[SC_NONE]
                << " | by origin drawn:";
      for (int o = 0; o < NORIG; ++o)
        if (byOrigin[o])
          std::cout << " " << kOriginNameMPI[o] << " " << byOrigin[o] << ";";
      std::cout << " (the b and bbar copies are counted under their origin)\n";
    }
    if (coneR_ > 0) {
      std::cout << "  cone dR<" << coneR_ << " (stage " << s << ")";
      for (size_t k = 0; k < 2; ++k) {
        auto const& S = sys[k];
        if (!S.hasCone)
          continue;
        double scalar = 0;
        for (size_t i : S.coneIdx)
          scalar += gens[i].pt();
        std::cout << " | around " << (k == 0 ? "b" : "bbar") << ": n=" << S.coneIdx.size() << " pt=" << S.coneP4.pt()
                  << " eta=" << S.coneP4.eta() << " phi=" << S.coneP4.phi() << " scalar sum pT=" << scalar;
        if (S.hasUE) {  // the cone without the underlying event: the vector sum of its hard-process partons
          math::XYZTLorentzVector hard;
          for (size_t i : S.coneIdx)
            if (scatter_[i] == SC_HARD)
              hard += gens[i].p4();
          std::cout << " hard process: n=" << S.coneIdx.size() - S.ueN << " pt=" << hard.pt()
                    << " scalar sum pT=" << scalar - S.uePt << " UE (MPI + remnants): n=" << S.ueN
                    << " scalar sum pT=" << S.uePt;
        }
        // the scattering of a member only up to stage 3: a hadron comes from a string that joins partons of several
        // scatterings, so it has none
        bool tagSc = hasMPI_ && s <= 3;
        std::cout << " members [idx]pdg:pt:dR" << (tagSc ? ":scattering(1 hard, 3 MPI, 6 remnant)" : "") << " =";
        for (size_t i : S.coneIdx) {
          std::cout << " [" << i << "]" << gens[i].pdgId() << ":" << gens[i].pt() << ":"
                    << reco::deltaR(S.p4.eta(), S.p4.phi(), gens[i].eta(), gens[i].phi());
          if (tagSc)
            std::cout << ":" << scatter_[i];
        }
      }
      std::cout << "\n";
    }
    if (printParticles_) {
      std::cout << "    columns: [index] pdg status origin pt eta phi mothers daughters\n";
      for (size_t i = 0; i < n; ++i) {
        if (!alive[i])
          continue;
        const reco::GenParticle& g = gens[i];
        std::cout << "    [" << i << "] pdg=" << g.pdgId() << " status=" << g.status() << " origin=" << kOriginName[origin[i]]
                  << " pt=" << g.pt() << " eta=" << g.eta() << " phi=" << g.phi() << " mothers=";
        for (size_t m = 0; m < g.numberOfMothers(); ++m)
          std::cout << (m ? "," : "") << g.motherRef(m).key();
        std::cout << " daughters=";
        for (size_t d = 0; d < g.numberOfDaughters(); ++d)
          std::cout << (d ? "," : "") << g.daughterRef(d).key();
        std::cout << "\n";
      }
    }

    if (savePDF_ && (drawStages_.empty() || std::find(drawStages_.begin(), drawStages_.end(), s) != drawStages_.end()))
      draw(s, ev, gens, alive, shown, sys);
  }
}

// ----------------------------------------------------------------------------- drawing
void GenPartAnalyzer::draw(int stage, const edm::Event& ev, const Coll& gens, const std::vector<char>& alive,
                           const std::vector<int>& origin, const std::vector<Sys>& sys) {
  const size_t n = gens.size();
  std::vector<std::unique_ptr<TObject>> keep;  // primitives must outlive Print()

  canvas_->Clear();
  canvas_->SetLeftMargin(0.06);
  canvas_->SetRightMargin(0.21);
  canvas_->SetTopMargin(0.19);
  canvas_->SetBottomMargin(0.08);
  canvas_->SetGrid();

  auto frame = std::make_unique<TH2D>(Form("frame%d", stage), ";#eta;#phi", 10, -etaMax_, etaMax_, 10, -M_PI, M_PI);
  frame->GetXaxis()->SetTitleOffset(0.9);
  frame->GetYaxis()->SetTitleOffset(0.6);
  frame->Draw("axis");

  // membership in the b / bbar systems
  std::vector<int> member(n, -1);
  for (int k = 0; k < 2; ++k)
    for (size_t i : sys[k].idx)
      member[i] = k;

  std::vector<std::unique_ptr<TGraph>> graphs;
  std::vector<int> nOrigin(NORIG, 0);
  int nClamped = 0;
  auto addPoint = [&](int color, int style, double eta, double phi, double pt) {
    bool clamped = std::fabs(eta) > etaMax_;
    if (clamped) {
      eta = eta > 0 ? etaMax_ - 0.05 : -etaMax_ + 0.05;
      ++nClamped;
    }
    auto g = std::make_unique<TGraph>(1);
    g->SetPoint(0, eta, phi);
    g->SetMarkerColor(color);
    g->SetMarkerStyle(clamped ? 5 : style);
    g->SetMarkerSize(markerSize(pt) * (clamped ? 1.3 : 1.));
    g->Draw("P same");
    graphs.push_back(std::move(g));
    return eta;
  };
  auto label = [&](double x, double y, const char* s, double size, int color) {
    auto t = std::make_unique<TLatex>(x, y, s);
    t->SetTextSize(size);
    t->SetTextColor(color);
    t->Draw();
    keep.push_back(std::move(t));
  };
  // the cone shown magnified in the inset: the fuller one, if it holds at least two particles
  int zoomK = -1;
  for (int k = 0; k < 2; ++k)
    if (sys[k].hasCone && sys[k].coneIdx.size() >= 2 &&
        (zoomK < 0 || sys[k].coneIdx.size() > sys[zoomK].coneIdx.size()))
      zoomK = k;
  // ... and the other one as well when it holds at least three (two insets, in two corners: the stage-5 cones of the
  // complete event, each a cloud of decay products)
  std::vector<int> zoomKs;
  if (zoomK >= 0) {
    zoomKs.push_back(zoomK);
    if (sys[1 - zoomK].hasCone && sys[1 - zoomK].coneIdx.size() >= 3)
      zoomKs.push_back(1 - zoomK);
  }
  std::vector<char> inZoom(n, 0);  // labelled in the inset, not in the main frame
  for (int k : zoomKs) {
    for (size_t i : sys[k].coneIdx)
      inZoom[i] = 1;
    for (size_t i : sys[k].idx)  // the members the inset window shows (+-1.25 coneR), the neutrinos among them
      if (reco::deltaR(sys[k].p4.eta(), sys[k].p4.phi(), gens[i].eta(), gens[i].phi()) < 1.25 * coneR_)
        inZoom[i] = 1;
  }
  // others first, b / bbar on top
  for (size_t i = 0; i < n; ++i) {
    if (!alive[i] || member[i] >= 0)
      continue;
    int o = origin[i];
    ++nOrigin[o];
    double eta = addPoint(kOriginColor[o], kOriginMarker[o], gens[i].eta(), gens[i].phi(), gens[i].pt());
    if (gens[i].pt() >= labelPtMin_ && !inZoom[i])
      label(eta + 0.05, gens[i].phi() + 0.06, Form("%.1f", gens[i].pt()), 0.016, kOriginColor[o]);
  }
  const int sysColor[2] = {kRed, kBlue};
  for (int k = 0; k < 2; ++k)
    for (size_t i : sys[k].idx) {
      double eta = addPoint(sysColor[k], 20, gens[i].eta(), gens[i].phi(), gens[i].pt());
      if (stage == 5 && (gens[i].pt() < labelPtMin_ || inZoom[i]))  // the decay products of a magnified cone: in the inset
        continue;
      std::string name = stage == 5 ? pdgName(gens[i].pdgId()) : sys[k].label;
      label(eta + 0.05, gens[i].phi() + 0.08, Form("%s %.1f", name.c_str(), gens[i].pt()), stage == 5 ? 0.016 : 0.022,
            sysColor[k]);
    }
  // the cones: dashed circles (split where they cross phi = +-pi) and their pT below them
  auto circle = [&](double eta0, double phi0, double r, int color) {
    std::vector<double> xs, ys;
    auto flush = [&]() {
      if (xs.size() > 1) {
        auto g = std::make_unique<TGraph>(xs.size(), xs.data(), ys.data());
        g->SetLineColor(color);
        g->SetLineStyle(2);
        g->SetLineWidth(2);
        g->Draw("L same");
        graphs.push_back(std::move(g));
      }
      xs.clear();
      ys.clear();
    };
    for (int k = 0; k <= 96; ++k) {
      double t = 2 * M_PI * k / 96, x = eta0 + r * std::cos(t), y = reco::deltaPhi(phi0 + r * std::sin(t), 0.);
      if (std::fabs(x) > etaMax_) {
        flush();
        continue;
      }
      if (!ys.empty() && std::fabs(y - ys.back()) > M_PI)
        flush();
      xs.push_back(x);
      ys.push_back(y);
    }
    flush();
  };
  for (int k = 0; k < 2; ++k)
    if (sys[k].hasCone) {
      double eta0 = sys[k].p4.eta(), phi0 = sys[k].p4.phi();
      circle(eta0, phi0, coneR_, sysColor[k]);
      double y = phi0 - coneR_ - 0.22 > -M_PI + 0.05 ? phi0 - coneR_ - 0.22 : phi0 + coneR_ + 0.08;
      std::string cl = Form("cone %.1f: %.1f GeV (n = %zu", coneR_, sys[k].coneP4.pt(), sys[k].coneIdx.size());
      cl += sys[k].hasUE ? Form("; UE %.1f GeV)", sys[k].uePt) : ")";
      label(std::max(eta0 - coneR_, -etaMax_ + 0.05), y, cl.c_str(), 0.018, sysColor[k]);
    }
  // net b + bbar
  math::XYZTLorentzVector net = sys[0].p4 + sys[1].p4;
  bool haveBoth = !sys[0].idx.empty() && !sys[1].idx.empty();
  if (haveBoth && net.pt() > 0.1) {
    double eta = addPoint(kMagenta, 29, net.eta(), net.phi(), net.pt());
    if (coneR_ > 0) {  // left of the star, clear of the cone circles (right of it when the frame ends on the left)
      bool left = eta - 1.3 > -etaMax_;
      auto t = std::make_unique<TLatex>(left ? eta - 0.12 : eta + 0.10, net.phi() - 0.15, Form("b+#bar{b} %.1f", net.pt()));
      t->SetTextAlign(left ? 31 : 11);
      t->SetTextSize(0.022);
      t->SetTextColor(kMagenta);
      t->Draw();
      keep.push_back(std::move(t));
    } else
      label(eta + 0.05, net.phi() - 0.15, Form("b+#bar{b} %.1f", net.pt()), 0.022, kMagenta);
  }

  // header: title, event, b / bbar summary
  auto text = [&](double x, double y, const char* s, double size, int color = kBlack, int align = 11) {
    auto t = std::make_unique<TLatex>(x, y, s);
    t->SetNDC();
    t->SetTextSize(size);
    t->SetTextColor(color);
    t->SetTextAlign(align);
    t->Draw();
    keep.push_back(std::move(t));
  };
  text(0.06, 0.955, kStageTitle[stage], 0.034);
  text(0.79, 0.955, Form("run %u  lumi %u  event %llu", ev.id().run(), ev.luminosityBlock(), ev.id().event()), 0.022,
       kBlack, 31);
  // sample label in the right panel above the legend, one line per '|'-separated part
  if (!sampleLabel_.empty()) {
    size_t start = 0;
    for (int k = 0; start <= sampleLabel_.size() && k < 5; ++k) {
      size_t bar = sampleLabel_.find('|', start);
      std::string line = sampleLabel_.substr(start, bar == std::string::npos ? std::string::npos : bar - start);
      text(0.80, 0.925 - 0.026 * k, line.c_str(), 0.018, k == 0 ? kBlack : kGray + 3);
      if (bar == std::string::npos)
        break;
      start = bar + 1;
    }
  }
  size_t nAlive = 0;
  for (size_t i = 0; i < n; ++i)
    nAlive += alive[i];
  std::string info = Form("%zu particles drawn (marker size ~ log p_{T}; label = p_{T} [GeV] for p_{T} > %.0f GeV)", nAlive,
                          labelPtMin_);
  if (hasMPI_ && stage == 2)  // the shower of the hard process alone: the MPI and their showers come with stage 3
    info = Form("%zu particles drawn: the hard process and its own ISR + FSR (the %d MPI and their showers: stage 3)", nAlive,
                nMPI_);
  else if (hasMPI_ && stage == 3) {
    int nSc[7] = {};
    for (size_t i = 0; i < n; ++i)
      if (alive[i] && scatter_[i] >= 0 && scatter_[i] < 7)
        ++nSc[scatter_[i]];
    info = Form("%zu particles drawn: hard process + showers %d, MPI + showers %d, beam remnants %d", nAlive, nSc[SC_HARD],
                nSc[SC_MPI], nSc[SC_REMNANT]);
    if (nSc[SC_NONE])
      info += Form(", unattributed %d", nSc[SC_NONE]);
  }
  if (nClamped)
    info += Form(", %d with |#eta| > %.0f drawn on the border (x)", nClamped, etaMax_);
  text(0.06, 0.922, info.c_str(), 0.02, kGray + 3);
  for (int k = 0; k < 2; ++k) {
    std::string line = sys[k].what + ": ";
    if (sys[k].idx.empty())
      line += "not found";
    else
      line += Form("p_{T} = %.2f GeV, #eta = %.2f, #phi = %.2f", sys[k].p4.pt(), sys[k].p4.eta(), sys[k].p4.phi());
    if (sys[k].hasCone)
      line += Form(";   cone #DeltaR < %.1f: n = %zu, p_{T} = %.2f GeV", coneR_, sys[k].coneIdx.size(), sys[k].coneP4.pt());
    if (sys[k].hasUE)
      line += Form(" (UE %.2f GeV)", sys[k].uePt);
    text(0.06, 0.891 - 0.028 * k, line.c_str(), 0.022, sysColor[k]);
  }
  if (haveBoth) {
    std::string line = Form("net b+#bar{b}: |#vec{p}_{T}(b) + #vec{p}_{T}(#bar{b})| = %.2f GeV  (p_{x} = %.2f, p_{y} = %.2f)",
                            net.pt(), net.px(), net.py());
    line += Form(",  #Sigma|p_{T}| = %.2f GeV,  #Delta#phi(b,#bar{b}) = %.2f", sys[0].p4.pt() + sys[1].p4.pt(),
                 std::fabs(reco::deltaPhi(sys[0].p4.phi(), sys[1].p4.phi())));
    text(0.06, 0.835, line.c_str(), 0.022, kMagenta + 2);
  }

  // legend (only the origins present) and marker-size scale
  auto leg = std::make_unique<TLegend>(0.80, 0.50, 0.995, 0.80);
  leg->SetBorderSize(0);
  leg->SetTextSize(0.02);
  leg->SetHeader("particle origin (number drawn)");
  std::vector<std::unique_ptr<TMarker>> legMarkers;
  auto legEntry = [&](int color, int style, const char* entry) {
    auto m = std::make_unique<TMarker>(0, 0, style);
    m->SetMarkerColor(color);
    m->SetMarkerSize(1.4);
    leg->AddEntry(m.get(), entry, "p");
    legMarkers.push_back(std::move(m));
  };
  if (!sys[0].idx.empty())
    legEntry(kRed, 20, Form("b system (%zu)", sys[0].idx.size()));
  if (!sys[1].idx.empty())
    legEntry(kBlue, 20, Form("#bar{b} system (%zu)", sys[1].idx.size()));
  if (haveBoth)
    legEntry(kMagenta, 29, "b + #bar{b} (vector sum)");
  for (int o = 0; o < NORIG; ++o)
    if (nOrigin[o])
      legEntry(kOriginColor[o], kOriginMarker[o], Form("%s (%d)", (hasMPI_ ? kOriginNameMPI : kOriginName)[o], nOrigin[o]));
  std::unique_ptr<TLine> coneLine;
  if (coneR_ > 0) {
    coneLine = std::make_unique<TLine>();
    coneLine->SetLineStyle(2);
    coneLine->SetLineWidth(2);
    coneLine->SetLineColor(kGray + 2);
    leg->AddEntry(coneLine.get(), Form("cone #DeltaR < %.1f", coneR_), "l");
  }
  leg->Draw();

  text(0.80, 0.46, "marker size:", 0.02);
  const double scalePt[3] = {1., 10., 100.};
  for (int k = 0; k < 3; ++k) {
    auto m = std::make_unique<TMarker>(0.815, 0.42 - 0.04 * k, 20);
    m->SetNDC();
    m->SetMarkerColor(kGray + 2);
    m->SetMarkerSize(markerSize(scalePt[k]));
    m->Draw();
    keep.push_back(std::move(m));
    text(0.835, 0.415 - 0.04 * k, Form("p_{T} = %.0f GeV", scalePt[k]), 0.02);
  }
  text(0.80, 0.26, "stages:", 0.02);
  const char* flow[5] = {"1 hard process", "2 ISR + FSR", "3 MPI / remnants", "4 hadronisation", "5 hadron decays"};
  for (int k = 0; k < 5; ++k)  // stages beyond maxStage (not generated in this sample) in light grey
    text(0.81, 0.23 - 0.03 * k, flow[k], 0.018, k + 1 == stage ? kRed : (k < maxStage_ ? kGray + 2 : kGray));

  std::vector<std::pair<double, double>> usedCorners;
  for (int k : zoomKs)
    drawZoom(stage, gens, alive, origin, member, sys, k, usedCorners);

  canvas_->Update();
  if (pdfFile_.empty()) {
    canvas_->Print(Form("%s_stage%d_%s_run%u_event%llu.pdf", outputPrefix_.c_str(), stage, kStageName[stage],
                        ev.id().run(), ev.id().event()));
    return;
  }
  if (!pdfOpen_) {
    if (openPdf_)
      canvas_->Print((pdfFile_ + "[").c_str());
    pdfOpen_ = true;
  }
  canvas_->Print(pdfFile_.c_str());
}

// the inset with the cone of system k magnified: a TPad in a corner of the frame, its primitives owned by ROOT
// (kCanDelete: deleted with the pad by the canvas_->Clear() of the next page). The window spans +-1.25 coneR around the
// axis, plus a column on the right where the name and pT of every particle are written, joined to it by a thin line.
void GenPartAnalyzer::drawZoom(int stage, const Coll& gens, const std::vector<char>& alive, const std::vector<int>& origin,
                               const std::vector<int>& member, const std::vector<Sys>& sys, int k,
                               std::vector<std::pair<double, double>>& usedCorners) {
  const size_t n = gens.size();
  const int sysColor[2] = {kRed, kBlue};
  const double eta0 = sys[k].p4.eta(), phi0 = sys[k].p4.phi(), Z = 1.25 * coneR_;
  // the corner of the frame (canvas NDC) that hides the least: never over a system or its cone
  const double fx1 = 0.06, fx2 = 0.79, fy1 = 0.08, fy2 = 0.81, w = 0.27, h = 0.32, gap = 0.006;
  auto toEta = [&](double x) { return -etaMax_ + (x - fx1) / (fx2 - fx1) * 2 * etaMax_; };
  auto toPhi = [&](double y) { return -M_PI + (y - fy1) / (fy2 - fy1) * 2 * M_PI; };
  double best = 1e30, bx = fx2 - gap - w, by = fy2 - gap - h;
  for (double cx : {fx2 - gap - w, fx1 + gap})
    for (double cy : {fy2 - gap - h, fy1 + gap}) {
      if (std::find(usedCorners.begin(), usedCorners.end(), std::make_pair(cx, cy)) != usedCorners.end())
        continue;  // taken by the inset of the other cone
      double e1 = toEta(cx), e2 = toEta(cx + w), p1 = toPhi(cy), p2 = toPhi(cy + h), score = 0;
      for (size_t i = 0; i < n; ++i) {
        double e = std::clamp(gens[i].eta(), -etaMax_ + 0.05, etaMax_ - 0.05);
        if (alive[i] && e > e1 && e < e2 && gens[i].phi() > p1 && gens[i].phi() < p2)
          score += 1. + gens[i].pt();
      }
      for (auto const& T : sys)
        if (!T.idx.empty() && T.p4.eta() + coneR_ > e1 && T.p4.eta() - coneR_ < e2 && T.p4.phi() + coneR_ > p1 &&
            T.p4.phi() - coneR_ < p2)
          score += 1e6;
      if (score < best) {
        best = score;
        bx = cx;
        by = cy;
      }
    }
  usedCorners.emplace_back(bx, by);
  canvas_->cd();
  auto* pad = new TPad(Form("zoom%d_%d", stage, k), "", bx, by, bx + w, by + h);
  pad->SetBit(kCanDelete);
  pad->SetFillColor(kWhite);
  pad->SetBorderMode(0);
  pad->SetMargin(0.10, 0.03, 0.12, 0.13);  // left, right, bottom, top: the frame is 1.5 times wider than high
  pad->SetGrid(0, 0);
  pad->Draw();
  pad->cd();
  auto own = [](TObject* o) {
    o->SetBit(kCanDelete);
    return o;
  };
  auto* fr = new TH2D(Form("zoomFrame%d_%d", stage, k), ";#eta;#phi", 10, eta0 - Z, eta0 + 2 * Z, 10, phi0 - Z, phi0 + Z);
  fr->SetDirectory(nullptr);
  own(fr);
  for (TAxis* a : {fr->GetXaxis(), fr->GetYaxis()}) {
    a->SetLabelSize(0.06);
    a->SetTitleSize(0.07);
    a->SetNdivisions(505);
  }
  fr->GetXaxis()->SetTitleOffset(0.7);
  fr->GetYaxis()->SetTitleOffset(0.55);
  fr->Draw("axis");
  // the border of the inset, in the colour of the system
  const double bxs[5] = {0.002, 0.998, 0.998, 0.002, 0.002}, bys[5] = {0.002, 0.002, 0.998, 0.998, 0.002};
  for (int j = 0; j < 4; ++j) {
    auto* l = new TLine(bxs[j], bys[j], bxs[j + 1], bys[j + 1]);
    l->SetNDC();
    l->SetLineColor(sysColor[k]);
    l->SetLineWidth(2);
    own(l)->Draw();
  }
  // the cone
  std::vector<double> cx(97), cy(97);
  for (int j = 0; j <= 96; ++j) {
    cx[j] = eta0 + coneR_ * std::cos(2 * M_PI * j / 96);
    cy[j] = phi0 + coneR_ * std::sin(2 * M_PI * j / 96);
  }
  auto* gc = new TGraph(97, cx.data(), cy.data());
  gc->SetLineColor(sysColor[k]);
  gc->SetLineStyle(2);
  gc->SetLineWidth(2);
  own(gc)->Draw("L");
  // the particles within 1.25 coneR (phi unwrapped around the axis), the softer first
  struct P {
    size_t i;
    double x, y;
    int color;
  };
  std::vector<P> ps;
  for (size_t i = 0; i < n; ++i) {
    if (!alive[i])
      continue;
    double x = gens[i].eta(), y = phi0 + reco::deltaPhi(gens[i].phi(), phi0);
    if (std::hypot(x - eta0, y - phi0) < Z)
      ps.push_back({i, x, y, member[i] >= 0 ? sysColor[member[i]] : kOriginColor[origin[i]]});
  }
  std::sort(ps.begin(), ps.end(), [&](const P& a, const P& b) { return gens[a.i].pt() < gens[b.i].pt(); });
  for (auto const& p : ps) {
    auto* g = new TGraph(1);
    g->SetPoint(0, p.x, p.y);
    g->SetMarkerColor(p.color);
    g->SetMarkerStyle(member[p.i] >= 0 ? 20 : kOriginMarker[origin[p.i]]);
    g->SetMarkerSize(markerSize(gens[p.i].pt()));
    own(g)->Draw("P");
  }
  // the labels: the 12 hardest, in a column right of the cone, ordered in phi and kept apart
  std::sort(ps.begin(), ps.end(), [&](const P& a, const P& b) { return gens[a.i].pt() > gens[b.i].pt(); });
  size_t nMore = ps.size() > 12 ? ps.size() - 12 : 0;
  ps.resize(ps.size() - nMore);
  std::sort(ps.begin(), ps.end(), [](const P& a, const P& b) { return a.y > b.y; });
  const double top = phi0 + 0.9 * Z, bottom = phi0 - (nMore ? 0.76 : 0.9) * Z, lx = eta0 + 1.08 * Z;
  double dy = 0.19 * Z;
  if (ps.size() > 1)
    dy = std::min(dy, (top - bottom) / (ps.size() - 1));
  std::vector<double> ly(ps.size());
  for (size_t j = 0; j < ps.size(); ++j)
    ly[j] = std::min(ps[j].y, j ? ly[j - 1] - dy : top);
  for (size_t j = ps.size(); j-- > 0;)
    ly[j] = std::max(ly[j], j + 1 < ps.size() ? ly[j + 1] + dy : bottom);
  for (size_t j = 0; j < ps.size(); ++j) {
    auto* l = new TLine(ps[j].x, ps[j].y, lx - 0.04 * Z, ly[j]);
    l->SetLineColor(kGray + 1);
    l->SetLineWidth(1);
    own(l)->Draw();
    int a = std::abs(gens[ps[j].i].pdgId());
    bool nu = a == 12 || a == 14 || a == 16;  // drawn, not counted in the cone: in parentheses
    auto* t = new TLatex(lx, ly[j], Form(nu ? "(%s %.1f)" : "%s %.1f", pdgName(gens[ps[j].i].pdgId()).c_str(),
                                         gens[ps[j].i].pt()));
    t->SetTextAlign(12);
    t->SetTextSize(0.055);
    t->SetTextColor(ps[j].color);
    own(t)->Draw();
  }
  std::string ttl = Form("%s cone #DeltaR < %.1f: n = %zu, p_{T} = %.1f GeV", sys[k].label.c_str(), coneR_,
                         sys[k].coneIdx.size(), sys[k].coneP4.pt());
  if (sys[k].hasUE)
    ttl += Form(" (UE %.1f)", sys[k].uePt);
  if (nMore) {  // under the column of labels
    auto* more = new TLatex(lx, phi0 - 0.93 * Z, Form("+%zu more", nMore));
    more->SetTextAlign(12);
    more->SetTextSize(0.05);
    more->SetTextColor(kGray + 2);
    own(more)->Draw();
  }
  bool longTitle = sys[k].hasUE;  // smaller and further left, to stay inside the inset
  auto* title = new TLatex(longTitle ? 0.03 : 0.10, 0.92, ttl.c_str());
  title->SetNDC();
  title->SetTextSize(longTitle ? 0.052 : 0.058);
  title->SetTextColor(sysColor[k]);
  own(title)->Draw();
  canvas_->cd();
}

void GenPartAnalyzer::endJob() {
  if (pdfOpen_ && closePdf_)
    canvas_->Print((pdfFile_ + "]").c_str());
}

DEFINE_FWK_MODULE(GenPartAnalyzer);
