# GenJet step with pileup: the generator final state of the signal event overlaid with the stable
# particles of in-time minimum-bias collisions, and the anti-kT clustering (FastjetJetProducer) run
# again on the sum (test/common/genJetPileup_cfg.py, run by test/08_PU/run_08_PU.sh).
#
# The overlay uses the same MixingModule as the DIGI step of the pileup chapter, restricted to the
# HepMC record: 'mix' reads the generatorSmeared HepMCProduct of the signal event and of N library
# events (EmbeddedRootSource, in sequence from the GEN file of the library, the first skipEvents
# skipped) in the in-time bunch crossing only, and puts them together in a CrossingFrame<HepMCProduct>.
# No SimHit is touched and no digitizer runs. GenParticleProducer with useCrossingFrame = True
# (the mode the heavy-ion workflows use) converts every HepMC event of the frame into one
# genParticles collection, 'genParticlesPU': the signal first (collisionId 0), then the pileup
# collisions (collisionId 1 .. N). The usual InputGenJetsParticleSelector keeps the status 1
# particles of all of them (minus neutrinos for the NoNu variants), and the anti-kT producers
# of the GenJet step run on that: ak4GenJetsPU, ak8GenJetsPU, ak4GenJetsNoNuPU, ak8GenJetsNoNuPU,
# R = 0.4 / 0.8, jet pT > 3 GeV (jetPtMin, rParam as in genJet_cff.py).
import FWCore.ParameterSet.Config as cms

from SimGeneral.MixingModule.mixObjects_cfi import mixHepMCProducts
from PhysicsTools.HepMCCandAlgos.genParticles_cfi import genParticles as _genParticles
from RecoJets.Configuration.GenJetParticles_cff import genParticlesForJets as _genParticlesForJets, \
    genParticlesForJetsNoNu as _genParticlesForJetsNoNu
from RecoJets.JetProducers.ak4GenJets_cfi import ak4GenJets as _ak4GenJets
from RecoJets.JetProducers.ak8GenJets_cfi import ak8GenJets as _ak8GenJets

# the MixingModule of the DIGI step (SimGeneral/MixingModule/python/mix_POISSON_average_cfi.py) with
# no digitizers, one bunch crossing (the in-time one) and the HepMC product as the only mixed object
mix = cms.EDProducer('MixingModule',
    digitizers = cms.PSet(),
    LabelPlayback = cms.string(''),
    maxBunch = cms.int32(0),
    minBunch = cms.int32(0),
    bunchspace = cms.int32(25),
    mixProdStep1 = cms.bool(False),
    mixProdStep2 = cms.bool(False),
    playback = cms.untracked.bool(False),
    useCurrentProcessOnly = cms.bool(False),
    input = cms.SecSource('EmbeddedRootSource',
        type = cms.string('fixed'),                           # 'fixed': exactly averageNumber collisions; 'poisson': Poisson(averageNumber)
        nbPileupEvents = cms.PSet(averageNumber = cms.double(60.)),
        sequential = cms.untracked.bool(True),               # library events in order (no event twice) ...
        skipEvents = cms.untracked.uint32(0),                # ... starting after the first skipEvents
        fileNames = cms.untracked.vstring(),
    ),
    mixObjects = cms.PSet(mixHepMC = cms.PSet(mixHepMCProducts)),
)

# signal + pileup genParticles from the CrossingFrame<HepMCProduct> 'mix:generatorSmeared'
genParticlesPU = _genParticles.clone(
    useCrossingFrame = cms.untracked.bool(True),
    mix = cms.string('mix'),
)

genParticlesForJetsPU = _genParticlesForJets.clone(src = 'genParticlesPU')
genParticlesForJetsNoNuPU = _genParticlesForJetsNoNu.clone(src = 'genParticlesPU')
ak4GenJetsPU = _ak4GenJets.clone(src = 'genParticlesForJetsPU')
ak8GenJetsPU = _ak8GenJets.clone(src = 'genParticlesForJetsPU')
ak4GenJetsNoNuPU = _ak4GenJets.clone(src = 'genParticlesForJetsNoNuPU')
ak8GenJetsNoNuPU = _ak8GenJets.clone(src = 'genParticlesForJetsNoNuPU')

genJetPileupTask = cms.Task(mix, genParticlesPU, genParticlesForJetsPU, genParticlesForJetsNoNuPU,
                            ak4GenJetsPU, ak8GenJetsPU, ak4GenJetsNoNuPU, ak8GenJetsNoNuPU)
genJetPileupSequence = cms.Sequence(genJetPileupTask)
