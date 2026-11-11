# The Level-1 trigger of one event, step by step (chapter 12 L1T): three analyzers in one job, one PDF each.
#   L1TCaloAnalyzer    steps 1-3: ECAL and HCAL trigger primitives, calorimeter Layer-1 towers, Layer-2 jets
#                      (eta-phi pages of the 82 x 72 trigger towers + the towers around the leading jets)
#   L1TMuonAnalyzer    steps 4-5: muon trigger primitives, track finders (BMTF, OMTF, EMTF), global muon trigger
#   L1TGlobalAnalyzer  step 6: the objects the global trigger receives, the menu, the decision, the thresholds
#
#   cmsRun l1tAnalyzer_cfg.py inputFiles=file:step4_L1_12_L1T.root eventsToProcess=1:1:1 \
#          pdfPrefix=l1t_12_L1T sampleLabel="chapter 12: the Level-1 trigger of 2024|with PU = 60 per bx"
#
# writes <pdfPrefix>_calo.pdf, <pdfPrefix>_muon.pdf, <pdfPrefix>_global.pdf and, with savePNG=1, one PNG per page:
# <pdfPrefix>_calo_{ecaltp,hcaltp,layer1,layer2,jet<N>_{ecaltp,hcaltp,layer1}}_run<R>_event<E>.png,
# <pdfPrefix>_muon_{etaphi,rz}_..., <pdfPrefix>_global_{objects,ladders}_...; every number goes to stdout.
# globalTag, era and geometry are those of the L1 step: the menu, the calorimeter-trigger parameters and the HCAL
# trigger-primitive scale are read from the conditions, the DT chamber positions from the geometry.
import importlib
import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing('analysis')
options.inputFiles = 'file:step4_L1_12_L1T.root'
options.maxEvents = 1
options.register('globalTag', 'auto:phase1_2024_realistic', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'global tag of the L1 step (menu, calorimeter-trigger parameters, HCAL TP scale)')
options.register('era', 'Run3_2024', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'era of the L1 step (Configuration.Eras.Era_<era>_cff)')
options.register('geometry', 'DB:Extended', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'geometry: DB:Extended (GeometryRecoDB) or a Configuration.Geometry.Geometry<name>Reco_cff name')
options.register('eventsToProcess', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'run:lumi:event to draw, e.g. 1:1:1; empty: the first maxEvents events')
options.register('pdfPrefix', 'l1t', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'prefix of the three PDFs (<prefix>_calo.pdf, _muon.pdf, _global.pdf) and of the PNG pages')
options.register('savePNG', True, VarParsing.multiplicity.singleton, VarParsing.varType.bool, 'also save every page as PNG')
options.register('sampleLabel', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "text printed in the right panel of every page, '|' separates lines")
options.register('jetPtMin', 30.0, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'generator jets (ak4GenJets) above this pT are drawn as circles of radius 0.4')
options.register('genMuonPtMin', 1.0, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'generator muons (status 1) above this pT are listed and drawn')
options.register('zoomJets', 2, VarParsing.multiplicity.singleton, VarParsing.varType.int,
                 'number of leading L1 jets drawn tower by tower (three pages each)')
options.register('zMax', 256.0, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'upper end of the colour scale of the tower maps (GeV); a Layer-1 tower saturates at 255.5 GeV')
options._tags, options._tagOrder = {}, []
options.parseArguments()

era = getattr(importlib.import_module('Configuration.Eras.Era_%s_cff' % options.era), options.era)
process = cms.Process('L1TANA', era)
process.load('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.cerr.FwkReport.reportEvery = 1

if options.geometry.startswith('DB:'):
    process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
else:
    process.load('Configuration.Geometry.Geometry%sReco_cff' % options.geometry)
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, options.globalTag, '')
# the decoder of the HCAL trigger-primitive scale (CaloTPGTranscoder) and the HCAL trigger-tower geometry: event-setup
# producers that the DIGI step loads with the HCAL trigger-primitive emulator
process.load('SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff')

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(options.maxEvents))
process.source = cms.Source('PoolSource', fileNames=cms.untracked.vstring(options.inputFiles))
if options.eventsToProcess:
    process.source.eventsToProcess = cms.untracked.VEventRange(options.eventsToProcess)

common = dict(
    savePNG=cms.bool(options.savePNG),
    sampleLabel=cms.string(options.sampleLabel),
    jetPtMin=cms.double(options.jetPtMin),
    genJets=cms.InputTag('ak4GenJets'),
)
process.l1tCaloAnalyzer = cms.EDAnalyzer('L1TCaloAnalyzer',
    ecalTPs=cms.InputTag('simEcalTriggerPrimitiveDigis'),
    hcalTPs=cms.InputTag('simHcalTriggerPrimitiveDigis'),
    layer1Towers=cms.InputTag('simCaloStage2Layer1Digis'),
    jets=cms.InputTag('simCaloStage2Digis'),
    egammas=cms.InputTag('simCaloStage2Digis'),
    taus=cms.InputTag('simCaloStage2Digis'),
    etSums=cms.InputTag('simCaloStage2Digis'),
    pdfFile=cms.string(options.pdfPrefix + '_calo.pdf'),
    pngPrefix=cms.string(options.pdfPrefix + '_calo'),
    zoomJets=cms.int32(options.zoomJets),
    zMax=cms.double(options.zMax),
    **common
)
process.l1tMuonAnalyzer = cms.EDAnalyzer('L1TMuonAnalyzer',
    dtTPs=cms.InputTag('simDtTriggerPrimitiveDigis'),
    cscTPs=cms.InputTag('simCscTriggerPrimitiveDigis'),
    gemPads=cms.InputTag('simMuonGEMPadDigis'),
    gemPadClusters=cms.InputTag('simMuonGEMPadDigiClusters'),
    emtf=cms.InputTag('simEmtfDigis'),
    bmtfCands=cms.InputTag('simBmtfDigis', 'BMTF'),
    kbmtfCands=cms.InputTag('simKBmtfDigis', 'BMTF'),
    omtfCands=cms.InputTag('simOmtfDigis', 'OMTF'),
    emtfCands=cms.InputTag('simEmtfDigis', 'EMTF'),
    muons=cms.InputTag('simGmtStage2Digis'),
    muonShowers=cms.InputTag('simGmtShowerDigis'),
    genParticles=cms.InputTag('genParticles'),
    genMuonPtMin=cms.double(options.genMuonPtMin),
    pdfFile=cms.string(options.pdfPrefix + '_muon.pdf'),
    pngPrefix=cms.string(options.pdfPrefix + '_muon'),
    **common
)
process.l1tGlobalAnalyzer = cms.EDAnalyzer('L1TGlobalAnalyzer',
    algBlk=cms.InputTag('simGtStage2Digis'),
    jets=cms.InputTag('simCaloStage2Digis'),
    egammas=cms.InputTag('simCaloStage2Digis'),
    taus=cms.InputTag('simCaloStage2Digis'),
    etSums=cms.InputTag('simCaloStage2Digis'),
    muons=cms.InputTag('simGmtStage2Digis'),
    etaMax=cms.double(5.2),
    cicadaScore=cms.InputTag('simCaloStage2Layer1Summary', 'CICADAScore'),
    pdfFile=cms.string(options.pdfPrefix + '_global.pdf'),
    pngPrefix=cms.string(options.pdfPrefix + '_global'),
    **common
)
# one after the other: ROOT keeps a single open PDF, and each analyzer opens and closes its own inside analyze()
process.p = cms.Path(process.l1tCaloAnalyzer + process.l1tMuonAnalyzer + process.l1tGlobalAnalyzer)
