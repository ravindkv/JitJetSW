# GenJet step: anti-kT clustering (FastjetJetProducer) of the generator final state.
#
# Scheduled by cmsDriver.py as a USER step on the output of the GEN step:
#   cmsDriver.py step2 --filein file:step1_GEN_out.root -s USER:JitJetSW/Journey/genJet_cff.genJetSequence ...
# (runCMSDrivers_mc2024_BBbar_GEN.sh does this for every generator stage).
#
# The modules are the standard ones of Configuration/StandardSequences/Generator_cff.py (pgen
# already runs them inside the GEN step, so step 1 contains the same collections): the input
# selector keeps the status 1 genParticles (minus neutrinos for the NoNu variants; when the
# sample was generated with hadronisation off, the status 1 particles are the final partons,
# so parton jets are clustered) and the producers run anti-kT with R = 0.4 / 0.8 and a jet
# pT threshold of 3 GeV. jetPtMin and rParam can be changed with --customise_commands, e.g.
#   process.ak4GenJets.jetPtMin = 10.
import FWCore.ParameterSet.Config as cms

from RecoJets.Configuration.GenJetParticles_cff import genParticlesForJets, genParticlesForJetsNoNu
from RecoJets.JetProducers.ak4GenJets_cfi import ak4GenJets
from RecoJets.JetProducers.ak8GenJets_cfi import ak8GenJets

ak4GenJetsNoNu = ak4GenJets.clone(src='genParticlesForJetsNoNu')
ak8GenJetsNoNu = ak8GenJets.clone(src='genParticlesForJetsNoNu')

genJetInputTask = cms.Task(genParticlesForJets, genParticlesForJetsNoNu)
genJetTask = cms.Task(genJetInputTask, ak4GenJets, ak8GenJets, ak4GenJetsNoNu, ak8GenJetsNoNu)
genJetSequence = cms.Sequence(genJetTask)
