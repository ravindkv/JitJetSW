// Keeps only the events whose hard-process incoming partons (Pythia8 status 21) have the
// requested pdg ids, e.g. incomingIds = [2, -2] keeps u ubar -> X and drops d dbar, s sbar, gg, ...
//
// Pythia8 has no per-flavour switch for the incoming partons of a HardQCD process
// (HardQCD:qqbar2bbbar sums over all light q qbar), so the selection has to be done on the
// generated record. Runs on the HepMCProduct right after the generator, before genParticles.
//
// The ids are compared as an unordered list: [2, -2] matches u(beam A) ubar(beam B) and
// ubar(beam A) u(beam B). Set absoluteIds = True to match |pdg| instead (u ubar or ubar u or u u).
#include "FWCore/Framework/interface/global/EDFilter.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "SimDataFormats/GeneratorProducts/interface/HepMCProduct.h"

#include <algorithm>
#include <vector>

class HardProcessIncomingFilter : public edm::global::EDFilter<> {
public:
  explicit HardProcessIncomingFilter(const edm::ParameterSet& ps)
      : hepmcTok_(consumes<edm::HepMCProduct>(ps.getParameter<edm::InputTag>("src"))),
        incomingIds_(ps.getParameter<std::vector<int>>("incomingIds")),
        absoluteIds_(ps.getParameter<bool>("absoluteIds")),
        status_(ps.getParameter<int>("status")) {
    if (absoluteIds_)
      for (int& id : incomingIds_)
        id = std::abs(id);
    std::sort(incomingIds_.begin(), incomingIds_.end());
  }

  bool filter(edm::StreamID, edm::Event& ev, const edm::EventSetup&) const override {
    edm::Handle<edm::HepMCProduct> h;
    ev.getByToken(hepmcTok_, h);
    const HepMC::GenEvent* evt = h->GetEvent();
    std::vector<int> found;
    for (auto it = evt->particles_begin(); it != evt->particles_end(); ++it) {
      const HepMC::GenParticle* p = *it;
      if (std::abs(p->status()) != status_)
        continue;
      found.push_back(absoluteIds_ ? std::abs(p->pdg_id()) : p->pdg_id());
    }
    std::sort(found.begin(), found.end());
    return found == incomingIds_;
  }

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<edm::InputTag>("src", edm::InputTag("generator", "unsmeared"));
    desc.add<std::vector<int>>("incomingIds", {2, -2})->setComment("pdg ids of the hard-process incoming partons, any order");
    desc.add<bool>("absoluteIds", false)->setComment("compare |pdg id| instead of the signed id");
    desc.add<int>("status", 21)->setComment("Pythia8 status code of the hard-process incoming partons");
    descriptions.addWithDefaultLabel(desc);
  }

private:
  const edm::EDGetTokenT<edm::HepMCProduct> hepmcTok_;
  std::vector<int> incomingIds_;
  const bool absoluteIds_;
  const int status_;
};

DEFINE_FWK_MODULE(HardProcessIncomingFilter);
