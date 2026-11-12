#ifndef JitJetSW_Journey_HLTDisplay_h
#define JitJetSW_Journey_HLTDisplay_h
// HLTDisplay.h: what the four High Level Trigger analyzers of chapter 13 share (HLTPathAnalyzer, HLTCaloAnalyzer,
// HLTTrackAnalyzer, HLTPFAnalyzer), on top of the page and the tower grid of chapter 12 (L1TDisplay.h):
//   - an empty eta-phi frame of the whole detector (|eta| < 5.2) and of a window around a jet
//   - the generator jets of the signal (ak4GenJets) as magenta circles, the tag of the event, delta R
//   - the jet energy corrections of the HLT (JetCorrectorParametersCollection of the global tag, payload label
//     AK4PFHLT or AK4CaloHLT), evaluated level by level as the correctors of the menu do
#include <cmath>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "CondFormats/JetMETObjects/interface/FactorizedJetCorrector.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectorParameters.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "TH2F.h"

#include "L1TDisplay.h"

namespace hltdisplay {
  using namespace l1tdisplay;

  constexpr double kEtaMax = 5.2;
  constexpr int kGenColor = kMagenta + 1;

  inline double dPhi(double a, double b) { return wrapToPi(a - b); }
  inline double dR(double eta1, double phi1, double eta2, double phi2) {
    return std::hypot(eta1 - eta2, dPhi(phi1, phi2));
  }

  // an empty frame for an eta-phi page, drawn on the pad of the page
  inline TH2F* frame(Page& page, const std::string& name, const std::string& title, double eta1 = -kEtaMax, double eta2 = kEtaMax,
                     double phi1 = -M_PI, double phi2 = M_PI) {
    auto h = page.keep(new TH2F(name.c_str(), title.c_str(), 10, eta1, eta2, 10, phi1, phi2));
    h->SetDirectory(nullptr);
    h->SetStats(false);
    h->GetXaxis()->SetTitle("#eta");
    h->GetYaxis()->SetTitle("#phi");
    h->GetYaxis()->SetTitleOffset(0.8);
    h->Draw("AXIS");
    return h;
  }

  // the detector boundaries: tracker acceptance, barrel-endcap, endcap-HF
  inline void detectorLines(Page& page) {
    page.etaLines({1.479, 3.0}, kGray + 1, 3);
    page.etaLines({2.5}, kAzure - 4, 2);
  }

  // the generator jets of the signal (radius 0.4), with their pT written next to them
  inline void genJets(Page& page, const std::vector<reco::GenJet>& jets, double ptMin, bool labels = true) {
    for (auto const& j : jets) {
      if (j.pt() < ptMin)
        continue;
      page.circle(j.eta(), j.phi(), 0.4, kGenColor, 2, 1);
      if (labels)
        page.note(j.eta() + 0.42, j.phi() + 0.42, Form("gen %.0f", j.pt()), kGenColor, 0.024);
    }
  }

  // the jet energy corrections of the HLT, level by level (L1FastJet, L2Relative, L3Absolute, L2L3Residual):
  // factors[i] is the product of the levels 0..i, as FactorizedJetCorrector::getSubCorrections returns them
  class Corrections {
  public:
    Corrections() = default;
    void set(const JetCorrectorParametersCollection& coll, std::vector<std::string> levels) {
      std::vector<JetCorrectorParameters> pars;
      params_.clear();
      levels_.clear();
      description_.clear();
      for (auto const& l : levels) {
        std::vector<JetCorrectorParametersCollection::key_type> keys;
        coll.validKeys(keys);
        bool found = false;
        for (auto k : keys)
          if (JetCorrectorParametersCollection::findLabel(k) == l)
            found = true;
        if (!found)
          continue;
        pars.push_back(coll[l]);
        params_.push_back(coll[l]);
        levels_.push_back(l);
        const auto& d = coll[l].definitions();
        std::ostringstream o;
        o << "level=" << l << " records=" << coll[l].size() << " formula=\"" << d.formula() << "\" binVar=";
        for (auto const& v : d.binVar())
          o << v << ",";
        o << " parVar=";
        for (auto const& v : d.parVar())
          o << v << ",";
        // the bins of the second binning variable (JetPhi in the PF payload of 2024) for the first eta bin
        if (d.nBinVar() > 1 && coll[l].size() > 0) {
          o << " " << d.binVar(1) << "Bins(eta " << coll[l].record(0).xMin(0) << ".." << coll[l].record(0).xMax(0) << ")=";
          for (unsigned r = 0; r < coll[l].size(); ++r)
            if (coll[l].record(r).xMin(0) == coll[l].record(0).xMin(0))
              o << coll[l].record(r).xMin(1) << ".." << coll[l].record(r).xMax(1) << ",";
          // the eta bins that are split in phi
          int split = 0;
          for (unsigned r = 0; r + 1 < coll[l].size(); ++r)
            split += coll[l].record(r).xMin(0) == coll[l].record(r + 1).xMin(0);
          o << " etaBinsWithSeveralPhiBins=" << split << " | split bins:";
          for (unsigned r = 0; r < coll[l].size(); ++r) {
            const auto& rec = coll[l].record(r);
            bool sameAsNext = r + 1 < coll[l].size() && coll[l].record(r + 1).xMin(0) == rec.xMin(0);
            bool sameAsPrev = r > 0 && coll[l].record(r - 1).xMin(0) == rec.xMin(0);
            if (sameAsNext || sameAsPrev)
              o << " [" << rec.xMin(0) << "," << rec.xMax(0) << "]x[" << rec.xMin(1) << "," << rec.xMax(1) << "]";
          }
        }
        description_.push_back(o.str());
      }
      corrector_ = std::make_unique<FactorizedJetCorrector>(pars);
    }
    // cumulative factors, level by level, for a raw jet
    std::vector<float> factors(double rawPt, double rawE, double eta, double phi, double area, double rho) {
      if (!corrector_)
        return {};
      corrector_->setJetEta(eta);
      corrector_->setJetPhi(phi);
      corrector_->setJetPt(rawPt);
      corrector_->setJetE(rawE);
      corrector_->setJetA(area);
      corrector_->setRho(rho);
      return corrector_->getSubCorrections();
    }
    const std::vector<std::string>& levels() const { return levels_; }
    // one line per level: the formula and its variables, as stored in the payload
    const std::vector<std::string>& description() const { return description_; }

    // the bin of the first level used for a jet: "eta a..b phi c..d"
    std::string bin(double eta, double phi) const {
      if (params_.empty())
        return "";
      const auto& p = params_.front();
      std::vector<float> x = {float(eta)};
      if (p.definitions().nBinVar() > 1)
        x.push_back(float(phi));
      int i = p.binIndex(x);
      if (i < 0)
        return "none";
      const auto& r = p.record(i);
      std::ostringstream o;
      o << "eta[" << r.xMin(0) << "," << r.xMax(0) << "]";
      if (x.size() > 1)
        o << "phi[" << r.xMin(1) << "," << r.xMax(1) << "]";
      return o.str();
    }

  private:
    std::vector<JetCorrectorParameters> params_;
    std::vector<std::string> description_;
    std::unique_ptr<FactorizedJetCorrector> corrector_;
    std::vector<std::string> levels_;
  };

  // "a=1.00 b=2.00": the factors of the levels as a log fragment
  inline std::string factorString(const std::vector<std::string>& levels, const std::vector<float>& f) {
    std::ostringstream s;
    for (size_t i = 0; i < f.size() && i < levels.size(); ++i)
      s << " " << levels[i] << "=" << Form("%.4f", f[i]);
    return s.str();
  }

}  // namespace hltdisplay

#endif
