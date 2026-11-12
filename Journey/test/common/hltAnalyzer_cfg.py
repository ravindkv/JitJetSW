# The High Level Trigger of one event, step by step (chapter 13 HLT): four analyzers in one job, one PDF each.
#   HLTPathAnalyzer   steps 1 and 5: the menu, the raw event, the Level-1 seeds and prescales; the decision of every
#                     path, the datasets and streams, the trigger objects (pages seed, ladders, objects)
#   HLTCaloAnalyzer   step 2: ECAL/HCAL rec-hits, calorimeter towers, AK4 calo jets and their corrections
#                     (pages towers, jet1, jet2)
#   HLTTrackAnalyzer  step 3: pixel and strip clusters, pixel tracks, iterative tracking, pixel and PF vertices
#                     (pages etaphi, vertices)
#   HLTPFAnalyzer     step 4: particle-flow candidates, AK4 PF jets and their corrections (pages candidates, jet1, jet2)
#
#   cmsRun hltAnalyzer_cfg.py inputFiles=file:step5_HLT_13_HLT.root eventsToProcess=1:1:1 \
#          pdfPrefix=hlt_13_HLT sampleLabel="chapter 13: the High Level Trigger|with PU = 60 per bx"
#
# writes <pdfPrefix>_seed.pdf (HLTPathAnalyzer), _calo.pdf, _track.pdf, _pf.pdf and, with savePNG=1, one PNG per page:
# <pdfPrefix>_seed_{seed,ladders,objects}_run<R>_event<E>.png, _calo_{towers,jet1,jet2}_..., _track_{etaphi,vertices}_...,
# _pf_{candidates,jet1,jet2}_...; every number goes to stdout.
# globalTag, era and geometry are those of the HLT step: the jet energy corrections of the HLT (payloads AK4CaloHLT,
# AK4PFHLT of JetCorrectionsRecord) are read from the conditions, the positions of the calorimeter cells from the geometry.
import importlib
import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing('analysis')
options.inputFiles = 'file:step5_HLT_13_HLT.root'
options.maxEvents = 1
options.register('globalTag', 'auto:phase1_2024_realistic', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'global tag of the HLT step (jet energy corrections of the HLT)')
options.register('era', 'Run3_2024', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'era of the HLT step (Configuration.Eras.Era_<era>_cff)')
options.register('geometry', 'DB:Extended', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'geometry: DB:Extended (GeometryRecoDB) or a Configuration.Geometry.Geometry<name>Reco_cff name')
options.register('processName', 'HLT', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'process name of the HLT step')
options.register('eventsToProcess', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'run:lumi:event to draw, e.g. 1:1:1; empty: the first maxEvents events')
options.register('pdfPrefix', 'hlt', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'prefix of the four PDFs (<prefix>_seed.pdf, _calo.pdf, _track.pdf, _pf.pdf) and of the PNG pages')
options.register('savePNG', True, VarParsing.multiplicity.singleton, VarParsing.varType.bool, 'also save every page as PNG')
options.register('sampleLabel', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "text printed in the right panel of every page, '|' separates lines")
options.register('jetPtMin', 30.0, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'generator jets (ak4GenJets) above this pT are drawn as circles of radius 0.4 and followed')
options.register('dzMax', 0.1, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'a track belongs to a vertex if it is within this distance in z (cm)')
options.register('zMax', 256.0, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'upper end of the colour scale of the tower maps (GeV)')
options._tags, options._tagOrder = {}, []
options.parseArguments()

era = getattr(importlib.import_module('Configuration.Eras.Era_%s_cff' % options.era), options.era)
process = cms.Process('HLTANA', era)
process.load('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.cerr.FwkReport.reportEvery = 1

if options.geometry.startswith('DB:'):
    process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
else:
    process.load('Configuration.Geometry.Geometry%sReco_cff' % options.geometry)
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, options.globalTag, '')

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(options.maxEvents))
process.source = cms.Source('PoolSource', fileNames=cms.untracked.vstring(options.inputFiles))
if options.eventsToProcess:
    process.source.eventsToProcess = cms.untracked.VEventRange(options.eventsToProcess)

hlt = options.processName
common = dict(
    savePNG=cms.bool(options.savePNG),
    sampleLabel=cms.string(options.sampleLabel),
    jetPtMin=cms.double(options.jetPtMin),
    genJets=cms.InputTag('ak4GenJets'),
)
jecLevels = cms.vstring('L1FastJet', 'L2Relative', 'L3Absolute', 'L2L3Residual')
process.hltPathAnalyzer = cms.EDAnalyzer('HLTPathAnalyzer',
    processName=cms.string(hlt),
    rawData=cms.InputTag('rawDataCollector', '', hlt),
    l1Results=cms.InputTag('hltGtStage2Digis', '', hlt),
    l1Jets=cms.InputTag('hltGtStage2Digis', 'Jet', hlt),
    l1EGammas=cms.InputTag('hltGtStage2Digis', 'EGamma', hlt),
    l1Taus=cms.InputTag('hltGtStage2Digis', 'Tau', hlt),
    l1Muons=cms.InputTag('hltGtStage2Digis', 'Muon', hlt),
    triggerResults=cms.InputTag('TriggerResults', '', hlt),
    triggerSummary=cms.InputTag('hltTriggerSummaryAOD', '', hlt),
    pfJets=cms.InputTag('hltAK4PFJetsCorrected', '', hlt),
    ak8PFJets=cms.InputTag('hltAK8PFJetsCorrected', '', hlt),
    caloJets=cms.InputTag('hltAK4CaloJetsCorrected', '', hlt),
    # the seed modules of the jet paths of the book: their objects are drawn (the first one circled)
    seedFilters=cms.vstring('hltL1sSingleJet90', 'hltL1sZeroBias', 'hltL1sSingleJet120', 'hltL1sSingleJet90Fwd'),
    pdfFile=cms.string(options.pdfPrefix + '_seed.pdf'),
    pngPrefix=cms.string(options.pdfPrefix + '_seed'),
    **common
)
process.hltCaloAnalyzer = cms.EDAnalyzer('HLTCaloAnalyzer',
    towers=cms.InputTag('hltTowerMakerForAll', '', hlt),
    ebRecHits=cms.InputTag('hltEcalRecHit', 'EcalRecHitsEB', hlt),
    eeRecHits=cms.InputTag('hltEcalRecHit', 'EcalRecHitsEE', hlt),
    hbheRecHits=cms.InputTag('hltHbhereco', '', hlt),
    hfRecHits=cms.InputTag('hltHfreco', '', hlt),
    rawJets=cms.InputTag('hltAK4CaloJets', '', hlt),
    correctedJets=cms.InputTag('hltAK4CaloJetsCorrected', '', hlt),
    rho=cms.InputTag('hltFixedGridRhoFastjetAllCalo', '', hlt),
    l1Jets=cms.InputTag('hltGtStage2Digis', 'Jet', hlt),
    jecPayload=cms.string('AK4CaloHLT'),
    jecLevels=jecLevels,
    pdfFile=cms.string(options.pdfPrefix + '_calo.pdf'),
    pngPrefix=cms.string(options.pdfPrefix + '_calo'),
    zMax=cms.double(options.zMax),
    **common
)
process.hltTrackAnalyzer = cms.EDAnalyzer('HLTTrackAnalyzer',
    pixelClusters=cms.InputTag('hltSiPixelClusters', '', hlt),
    stripClusters=cms.InputTag('hltSiStripRawToClustersFacility', '', hlt),
    pixelTracks=cms.InputTag('hltPixelTracks', '', hlt),
    iter0Tracks=cms.InputTag('hltIter0PFlowTrackSelectionHighPurity', '', hlt),
    doubletTracks=cms.InputTag('hltDoubletRecoveryPFlowTrackSelectionHighPurity', '', hlt),
    mergedTracks=cms.InputTag('hltMergedTracks', '', hlt),
    pixelVertices=cms.InputTag('hltPixelVertices', '', hlt),
    trimmedPixelVertices=cms.InputTag('hltTrimmedPixelVertices', '', hlt),
    pfVertices=cms.InputTag('hltVerticesPF', '', hlt),
    pileupInfo=cms.InputTag('addPileupInfo'),
    signalVertex=cms.InputTag('genParticles', 'xyz0'),
    dzMax=cms.double(options.dzMax),
    pdfFile=cms.string(options.pdfPrefix + '_track.pdf'),
    pngPrefix=cms.string(options.pdfPrefix + '_track'),
    **common
)
process.hltPFAnalyzer = cms.EDAnalyzer('HLTPFAnalyzer',
    pfCandidates=cms.InputTag('hltParticleFlow', '', hlt),
    rawJets=cms.InputTag('hltAK4PFJets', '', hlt),
    correctedJets=cms.InputTag('hltAK4PFJetsCorrected', '', hlt),
    rho=cms.InputTag('hltFixedGridRhoFastjetAll', '', hlt),
    jecPayload=cms.string('AK4PFHLT'),
    jecLevels=jecLevels,
    pdfFile=cms.string(options.pdfPrefix + '_pf.pdf'),
    pngPrefix=cms.string(options.pdfPrefix + '_pf'),
    **common
)
# one after the other: ROOT keeps a single open PDF, and each analyzer opens and closes its own inside analyze()
process.p = cms.Path(process.hltPathAnalyzer + process.hltCaloAnalyzer + process.hltTrackAnalyzer + process.hltPFAnalyzer)
