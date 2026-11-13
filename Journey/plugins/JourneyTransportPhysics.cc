// JourneyTransportPhysics: Geant4 physics list with transportation only, i.e. no decays, no
// electromagnetic and no hadronic processes. Every Geant4 particle is constructed so that the
// generator primaries exist in the particle table; the only process is the transportation that
// G4VModularPhysicsList adds itself, so the particles follow straight lines (UseMagneticField =
// False) or helices in the field, cross the sensitive volumes without energy loss and leave the
// detector or are stopped by the time cut (MaxTrackTime).
//
// Used by the transport-only SIM of chapter 09 (test/09_G4/run_09_G4.sh) and the stage B of chapter 10
// (test/10_SIM/run_10_SIM.sh: the field on, no physics), which cannot
// be obtained from the standard lists: FTFP_BERT_EMM with EMPhysics and HadPhysics off still
// registers G4DecayPhysics, and the Geant4 UI commands of G4Commands are applied before the
// processes exist (PreInit state), so '/process/inactivate Decay' has no effect.
// Selected with process.g4SimHits.Physics.type = 'SimG4Core/Physics/JourneyTransportPhysics'
// (the factory adds the SimG4Core/Physics/ prefix to every physics list).
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "SimG4Core/Physics/interface/PhysicsList.h"
#include "SimG4Core/Physics/interface/PhysicsListFactory.h"

#include "G4BaryonConstructor.hh"
#include "G4BosonConstructor.hh"
#include "G4IonConstructor.hh"
#include "G4LeptonConstructor.hh"
#include "G4MesonConstructor.hh"
#include "G4ShortLivedConstructor.hh"
#include "G4VPhysicsConstructor.hh"

// all particles, no processes (G4DecayPhysics::ConstructParticle without the decay process)
class JourneyAllParticles : public G4VPhysicsConstructor {
public:
  JourneyAllParticles() : G4VPhysicsConstructor("JourneyAllParticles") {}
  void ConstructParticle() override {
    G4BosonConstructor bosons;
    bosons.ConstructParticle();
    G4LeptonConstructor leptons;
    leptons.ConstructParticle();
    G4MesonConstructor mesons;
    mesons.ConstructParticle();
    G4BaryonConstructor baryons;
    baryons.ConstructParticle();
    G4IonConstructor ions;
    ions.ConstructParticle();
    G4ShortLivedConstructor shortLived;
    shortLived.ConstructParticle();
  }
  void ConstructProcess() override {}
};

class JourneyTransportPhysics : public PhysicsList {
public:
  explicit JourneyTransportPhysics(const edm::ParameterSet& p) : PhysicsList(p) {
    RegisterPhysics(new JourneyAllParticles());
    edm::LogVerbatim("PhysicsList") << "JourneyTransportPhysics constructed: transportation only "
                                    << "(no decay, no electromagnetic and no hadronic physics)";
  }
};

DEFINE_PHYSICSLIST(JourneyTransportPhysics);
