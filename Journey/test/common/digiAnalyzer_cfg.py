# The digis of one event drawn on the detector (DigiAnalyzer, chapter 11 DIGI): an eta-phi page and an r-z page
# with every pixel, strip, ECAL, HCAL and muon digi of the event, the tracker and muon digis attributed through
# their DigiSimLinks to the signal, to in-time pileup, to out-of-time pileup or to noise, the calorimeter digis
# sized by their amplitude above pedestal; the counts per subdetector go to stdout.
#
#   cmsRun digiAnalyzer_cfg.py inputFiles=file:step3_DIGI_mixed_11_DIGI.root eventsToProcess=1:1:1 \
#          pdfFile=digiAnalyzer_mixed_11_DIGI.pdf pngPrefix=digiAnalyzer_mixed_11_DIGI sampleLabel="(a+b) signal + pileup|PU 60"
#
# globalTag, era and geometry are those of the DIGI step (the positions of the digis come from the reco geometry).
import importlib
import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing('analysis')
options.inputFiles = 'file:step3_DIGI_signal_11_DIGI.root'
options.outputFile = 'digiAnalyzer.root'
options.maxEvents = 1
options.register('globalTag', 'auto:phase1_2024_realistic', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'global tag of the geometry used for the digi positions (as in the DIGI step)')
options.register('era', 'Run3_2024', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'era of the DIGI step (Configuration.Eras.Era_<era>_cff)')
options.register('geometry', 'DB:Extended', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'geometry: DB:Extended (GeometryRecoDB) or a Configuration.Geometry.Geometry<name>Reco_cff name')
options.register('eventsToProcess', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'run:lumi:event to draw, e.g. 1:1:1; empty: the first maxEvents events')
options.register('pdfFile', 'digiAnalyzer.pdf', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'the multi-page PDF (eta-phi page, r-z page per event)')
options.register('pngPrefix', 'digiAnalyzer', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'prefix of the PNG of every page (<prefix>_etaphi_run<R>_event<E>.png, ..._rz_...)')
options.register('savePNG', True, VarParsing.multiplicity.singleton, VarParsing.varType.bool, 'also save every page as PNG')
options.register('sampleLabel', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "text printed in the right panel of every page, '|' separates lines")
options.register('etaMax', 5.2, VarParsing.multiplicity.singleton, VarParsing.varType.float, 'eta axis range of the eta-phi page')
options.register('jetPtMin', 30.0, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'generator jets (ak4GenJets) above this pT are drawn as circles of radius 0.4')
options.register('ecalMinAmp', 0.0, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'ECAL digis with an amplitude (max sample - pedestal, ADC counts) below this are not drawn or counted')
options.register('hcalMinAmp', 0.0, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'HCAL digis with an amplitude (sum of the signal time slices - pedestal, ADC counts) below this are not drawn or counted')
options.register('hcalSignalTS', -1, VarParsing.multiplicity.singleton, VarParsing.varType.int,
                 'first time slice of the HCAL signal (-1: the sample of interest flagged in the frame; the pedestal is estimated from the earlier slices)')
options.register('maxDrawn', 8000, VarParsing.multiplicity.singleton, VarParsing.varType.int,
                 'tracker / muon digis drawn per subdetector (every k-th beyond that; the counts stay exact; 0: all)')
options.register('hcalNSumTS', 2, VarParsing.multiplicity.singleton, VarParsing.varType.int, 'HCAL time slices summed for the amplitude')
options._tags, options._tagOrder = {}, []
options.parseArguments()

era = getattr(importlib.import_module('Configuration.Eras.Era_%s_cff' % options.era), options.era)
process = cms.Process('DIGIANA', era)
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

process.TFileService = cms.Service('TFileService', fileName=cms.string(options.outputFile))

process.digiAnalyzer = cms.EDAnalyzer('DigiAnalyzer',
    pixelDigis=cms.InputTag('simSiPixelDigis'),
    pixelLinks=cms.InputTag('simSiPixelDigis'),
    stripDigis=cms.InputTag('simSiStripDigis', 'ZeroSuppressed'),
    stripLinks=cms.InputTag('simSiStripDigis'),
    ebDigis=cms.InputTag('simEcalDigis', 'ebDigis'),
    eeDigis=cms.InputTag('simEcalDigis', 'eeDigis'),
    esDigis=cms.InputTag('simEcalPreshowerDigis'),
    hbheDigis=cms.InputTag('simHcalDigis', 'HBHEQIE11DigiCollection'),
    hfDigis=cms.InputTag('simHcalDigis', 'HFQIE10DigiCollection'),
    hoDigis=cms.InputTag('simHcalDigis'),
    dtDigis=cms.InputTag('simMuonDTDigis'),
    dtLinks=cms.InputTag('simMuonDTDigis'),
    cscStripDigis=cms.InputTag('simMuonCSCDigis', 'MuonCSCStripDigi'),
    cscStripLinks=cms.InputTag('simMuonCSCDigis', 'MuonCSCStripDigiSimLinks'),
    cscWireDigis=cms.InputTag('simMuonCSCDigis', 'MuonCSCWireDigi'),
    cscWireLinks=cms.InputTag('simMuonCSCDigis', 'MuonCSCWireDigiSimLinks'),
    rpcDigis=cms.InputTag('simMuonRPCDigis'),
    rpcLinks=cms.InputTag('simMuonRPCDigis', 'RPCDigiSimLink'),
    gemDigis=cms.InputTag('simMuonGEMDigis'),
    gemLinks=cms.InputTag('simMuonGEMDigis', 'GEM'),
    pileupSummary=cms.InputTag('addPileupInfo'),
    vertex=cms.InputTag('genParticles', 'xyz0'),
    genJets=cms.InputTag('ak4GenJets'),
    pdfFile=cms.string(options.pdfFile),
    pngPrefix=cms.string(options.pngPrefix),
    savePNG=cms.bool(options.savePNG),
    sampleLabel=cms.string(options.sampleLabel),
    etaMax=cms.double(options.etaMax),
    jetPtMin=cms.double(options.jetPtMin),
    ecalMinAmp=cms.double(options.ecalMinAmp),
    hcalMinAmp=cms.double(options.hcalMinAmp),
    hcalSignalTS=cms.int32(options.hcalSignalTS),
    hcalNSumTS=cms.int32(options.hcalNSumTS),
    maxDrawn=cms.int32(options.maxDrawn),
)
process.p = cms.Path(process.digiAnalyzer)
