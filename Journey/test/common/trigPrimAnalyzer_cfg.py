# The calorimeter trigger primitives of one event (TrigPrimAnalyzer, chapter 11 DIGI): what the DIGI step makes for
# the Level-1 trigger of chapter 12. Pages: the ECAL and the HCAL primitives on the 82 x 72 trigger-tower grid and,
# for every generator jet, the 17 x 17 towers around its axis with the transverse energy written in each tower.
# The log compares the ECAL primitives with the energy the particles of the sample deposited in the crystals
# (PCaloHits per trigger tower), lists the trigger-tower flags against the selective-readout flags they drive, and
# the time samples of the HCAL primitives.
#
#   cmsRun trigPrimAnalyzer_cfg.py inputFiles=file:step3_DIGI_signal_11_DIGI.root eventsToProcess=1:1:1 \
#          pdfFile=trigPrim_signal_11_DIGI.pdf pngPrefix=trigPrim_signal_11_DIGI sampleLabel="(11a) the signal alone"
#
# writes the PDF and, with savePNG=1, <pngPrefix>_{ecaltp,hcaltp,jet<N>_ecaltp,jet<N>_hcaltp}_run<R>_event<E>.png.
# mixed=1 for the sample with pileup mixed in: the file then holds only the SimHits of the signal, and a primitive
# without a deposit behind it is pileup or noise.
import importlib
import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing('analysis')
options.inputFiles = 'file:step3_DIGI_signal_11_DIGI.root'
options.maxEvents = 1
options.register('globalTag', 'auto:phase1_2024_realistic', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'global tag of the DIGI step (the scale of the HCAL trigger primitives)')
options.register('era', 'Run3_2024', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'era of the DIGI step (Configuration.Eras.Era_<era>_cff)')
options.register('geometry', 'DB:Extended', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'geometry: DB:Extended (GeometryRecoDB) or a Configuration.Geometry.Geometry<name>Reco_cff name')
options.register('eventsToProcess', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'run:lumi:event to draw, e.g. 1:1:1; empty: the first maxEvents events')
options.register('pdfFile', 'trigPrim.pdf', VarParsing.multiplicity.singleton, VarParsing.varType.string, 'the multi-page PDF')
options.register('pngPrefix', 'trigPrim', VarParsing.multiplicity.singleton, VarParsing.varType.string, 'prefix of the PNG of every page')
options.register('savePNG', True, VarParsing.multiplicity.singleton, VarParsing.varType.bool, 'also save every page as PNG')
options.register('sampleLabel', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "text printed in the right panel of every page, '|' separates lines")
options.register('mixed', False, VarParsing.multiplicity.singleton, VarParsing.varType.bool,
                 'the sample has pileup mixed in (its SimHits are those of the signal only)')
options.register('jetPtMin', 30.0, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'generator jets (ak4GenJets) above this pT are drawn and get their tower-by-tower pages')
options.register('zMax', 256.0, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'upper end of the colour scale of the tower maps (GeV)')
options._tags, options._tagOrder = {}, []
options.parseArguments()

era = getattr(importlib.import_module('Configuration.Eras.Era_%s_cff' % options.era), options.era)
process = cms.Process('TPANA', era)
process.load('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.cerr.FwkReport.reportEvery = 1

if options.geometry.startswith('DB:'):
    process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
else:
    process.load('Configuration.Geometry.Geometry%sReco_cff' % options.geometry)
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, options.globalTag, '')
# the decoder of the HCAL trigger-primitive scale (CaloTPGTranscoder), an event-setup producer of the DIGI step
process.load('SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff')

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(options.maxEvents))
process.source = cms.Source('PoolSource', fileNames=cms.untracked.vstring(options.inputFiles))
if options.eventsToProcess:
    process.source.eventsToProcess = cms.untracked.VEventRange(options.eventsToProcess)

process.trigPrimAnalyzer = cms.EDAnalyzer('TrigPrimAnalyzer',
    ecalTPs=cms.InputTag('simEcalTriggerPrimitiveDigis'),
    hcalTPs=cms.InputTag('simHcalTriggerPrimitiveDigis'),
    ebSrFlags=cms.InputTag('simEcalDigis', 'ebSrFlags'),
    eeSrFlags=cms.InputTag('simEcalDigis', 'eeSrFlags'),
    ebSimHits=cms.InputTag('g4SimHits', 'EcalHitsEB'),
    eeSimHits=cms.InputTag('g4SimHits', 'EcalHitsEE'),
    genJets=cms.InputTag('ak4GenJets'),
    pdfFile=cms.string(options.pdfFile),
    pngPrefix=cms.string(options.pngPrefix),
    savePNG=cms.bool(options.savePNG),
    sampleLabel=cms.string(options.sampleLabel),
    mixed=cms.bool(options.mixed),
    jetPtMin=cms.double(options.jetPtMin),
    zMax=cms.double(options.zMax),
)
process.p = cms.Path(process.trigPrimAnalyzer)
