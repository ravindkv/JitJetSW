# The signal event under pileup at generator level: the stable particles of nPileup minimum-bias
# collisions of the library are overlaid on the signal event (MixingModule on the HepMC record only,
# JitJetSW/Journey/genJetPileup_cff.py), the anti-kT clustering is run again on the sum, and
# GenJetAnalyzer draws the jets over signal (circles) and pileup (triangles) particles, one page per
# jet collection, with the pileup pT of every jet.
#
#   cd test/08_PU/out && cmsRun ../../common/genJetPileup_cfg.py inputFiles=file:../../07_GenJet/out/step2_GENJET_07_GenJet.root \
#          pileupFiles=file:step1_GEN_MinBias_08_PU.root eventsToProcess=1:1:1 nPileup=68 skipPileup=71 \
#          pdfFile=genJetPileup_08_PU.pdf          (the full command, with every option, is in test/08_PU/run_08_PU.sh)
#
# nPileup collisions are read in sequence from the library file(s) after the first skipPileup
# events (pileupType=fixed; pileupType=poisson draws the number from Poisson(nPileup) with seed
# pileupSeed), so that the in-time collisions of the mixing of chapters 10 and 11 (bunch crossing 0: the
# library events after those of the early bunch crossings) can be
# reproduced exactly. The merged genParticles (collisionId 0 = signal, 1..n = the pileup
# collisions) and the jets are written to edmOutput (if set), the histograms to outputFile.
import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing('analysis')
options.inputFiles = 'file:../../07_GenJet/out/step2_GENJET_07_GenJet.root'  # the signal of chapter 07, from test/08_PU/out
options.outputFile = 'genJetPileup.root'
options.maxEvents = -1
options.register('pileupFiles', 'file:step1_GEN_MinBias_08_PU.root', VarParsing.multiplicity.list, VarParsing.varType.string,
                 'GEN file(s) of the minimum-bias library (generatorSmeared HepMCProduct)')
options.register('eventsToProcess', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'signal event(s), e.g. 1:1:1 (empty: all)')
options.register('nPileup', 60, VarParsing.multiplicity.singleton, VarParsing.varType.int,
                 'pileup collisions overlaid on every signal event (pileupType=fixed) or their mean (poisson)')
options.register('skipPileup', 0, VarParsing.multiplicity.singleton, VarParsing.varType.int,
                 'library events skipped before the first one used')
options.register('pileupType', 'fixed', VarParsing.multiplicity.singleton, VarParsing.varType.string, 'fixed or poisson')
options.register('pileupSeed', 20243, VarParsing.multiplicity.singleton, VarParsing.varType.int,
                 'random seed of the MixingModule (used by pileupType=poisson)')
options.register('jetPtMin', 3.0, VarParsing.multiplicity.singleton, VarParsing.varType.float, 'jet pT threshold of the producers in GeV')
options.register('jets', 'ak4GenJetsPU', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'comma-separated jet collections drawn, one page per collection and event '
                 '(ak4GenJetsPU, ak4GenJetsNoNuPU, ak8GenJetsPU, ak8GenJetsNoNuPU)')
options.register('edmOutput', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'if set, EDM file with the merged genParticles and the jets (the signal file content is dropped)')
options.register('etaMax', 6.0, VarParsing.multiplicity.singleton, VarParsing.varType.float, 'eta axis range of the plots')
options.register('labelPtMin', 5.0, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'jet pT above which the pT value is written next to the jet axis (b, bbar jets always)')
options.register('printConstituents', False, VarParsing.multiplicity.singleton, VarParsing.varType.bool,
                 'print the constituents (genParticlesPU indices) of every jet to stdout')
options.register('pdfFile', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'if set, all plots go as pages into this single PDF instead of one PDF per collection and event')
options.register('sampleLabel', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "text printed in the right panel of every plot, '|' separates lines")
options._tags, options._tagOrder = {}, []  # keep outputFile as given (VarParsing would append _numEvent<N> when maxEvents > 0)
options.parseArguments()

process = cms.Process('GENJETPU')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.cerr.FwkReport.reportEvery = 1
process.load('Configuration.StandardSequences.Services_cff')    # RandomNumberGeneratorService (the MixingModule needs an engine)
process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')         # particle table for GenParticleProducer
process.RandomNumberGeneratorService.mix = cms.PSet(initialSeed=cms.untracked.uint32(options.pileupSeed),
                                                    engineName=cms.untracked.string('MixMaxRng'))

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(options.maxEvents))
process.source = cms.Source('PoolSource', fileNames=cms.untracked.vstring(options.inputFiles))
if options.eventsToProcess:
    process.source.eventsToProcess = cms.untracked.VEventRange(options.eventsToProcess)

# the overlay and the clustering
process.load('JitJetSW.Journey.genJetPileup_cff')
process.mix.input.fileNames = cms.untracked.vstring(options.pileupFiles)
process.mix.input.type = options.pileupType
process.mix.input.nbPileupEvents.averageNumber = float(options.nPileup)
process.mix.input.skipEvents = options.skipPileup
for m in ('ak4GenJetsPU', 'ak8GenJetsPU', 'ak4GenJetsNoNuPU', 'ak8GenJetsNoNuPU'):
    getattr(process, m).jetPtMin = options.jetPtMin

# the analyzer: the merged collection, the signal is collisionId 0
process.TFileService = cms.Service('TFileService', fileName=cms.string(options.outputFile))
process.genJetAnalyzer = cms.EDAnalyzer('GenJetAnalyzer',
    genParticles=cms.InputTag('genParticlesPU'),
    genJets=cms.VInputTag(*[cms.InputTag(j.strip()) for j in options.jets.split(',') if j.strip()]),
    outputPrefix=cms.string(options.outputFile.replace('.root', '')),
    etaMax=cms.double(options.etaMax),
    labelPtMin=cms.double(options.labelPtMin),
    signalCollisionId=cms.int32(0),
    savePDF=cms.bool(True),
    printConstituents=cms.bool(options.printConstituents),
    pdfFile=cms.string(options.pdfFile),
    sampleLabel=cms.string(options.sampleLabel),
    openPdf=cms.bool(True),
    closePdf=cms.bool(True),
)

process.p = cms.Path(process.genJetPileupSequence + process.genJetAnalyzer)
process.schedule = cms.Schedule(process.p)

if options.edmOutput:
    process.out = cms.OutputModule('PoolOutputModule',
        fileName=cms.untracked.string(options.edmOutput),
        outputCommands=cms.untracked.vstring('drop *', 'keep *_genParticles_*_*', 'keep *_generator_*_*',
                                             'keep *_genParticlesPU_*_*', 'keep *_genParticlesForJets*PU_*_*',
                                             'keep *_ak*GenJets*PU_*_*', 'keep *_mix_*_*'),
    )
    process.outPath = cms.EndPath(process.out)
    process.schedule.append(process.outPath)
