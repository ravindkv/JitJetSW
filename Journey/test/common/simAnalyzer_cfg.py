# Eta-phi maps of the Geant4 record of a SIM step (SimAnalyzer), five pages per event:
# 1 primaries handed to Geant4, 2 the primaries at the tracker/calorimeter boundary, 3 secondaries
# by creator process, 4 tracker and muon SimHits, 5 calorimeter SimHits per cell.
#
#   cmsRun simAnalyzer_cfg.py                                      # reads output_step2_SIM.root
#   cmsRun simAnalyzer_cfg.py inputFiles=file:other.root maxEvents=3 printTracks=True
#
# Writes simAnalyzer_pageN_<name>_run<R>_event<E>.pdf per page and event, simAnalyzer.root with the
# histograms per page, and prints the per-page counts and the b / bbar system sums to stdout.
# The hit positions come from the reco geometry of the EventSetup (globalTag, era, geometry: GeometryRecoDB
# for DB:Extended, otherwise Configuration.Geometry.Geometry<geometry>Reco_cff, e.g. ExtendedRun4D110 for Phase2).
# With a Phase2 era the Phase2 hit collections are read: the inner and outer tracker both write
# TrackerHitsPixelBarrel/Endcap, the endcap calorimeters are the HGCal (HGCHitsEE, HGCHitsHEfront, HGCHitsHEback),
# EcalHitsEE/ES and the HE of HcalHits do not exist (runCMSDrivers_mcPhase2_BBbar_SIM.sh).
#
# For the stage samples of runCMSDrivers_mc2024_BBbar_SIM.sh, collect the pages in one PDF and mark the
# stage, e.g. for the Transport_B sample (B field on, no decays, no EM or hadronic physics):
#   cmsRun simAnalyzer_cfg.py inputFiles=file:SIM/step2_SIM_Transport_B_out.root maxEvents=1 simStage=2 \
#          pdfFile=SIM/simAnalyzer_Transport_B_out.pdf sampleLabel="sample Transport_B|B on, Decay off|EM off, Had off"
# With genStages > 0 (default 5) GenPartAnalyzer runs first in the same job, so that every event starts
# with the eta-phi maps of the generator-record stages 1..genStages (as in genJetAnalyzer_cfg.py).
import importlib
import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing('analysis')
options.inputFiles = 'file:output_step2_SIM.root'
options.outputFile = 'simAnalyzer.root'
options.maxEvents = -1
options.register('globalTag', 'auto:phase1_2024_realistic', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'global tag of the geometry used for the hit positions (as in the SIM step)')
options.register('era', 'Run3_2024', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'era of the SIM step (Configuration.Eras.Era_<era>_cff); a Phase2* era selects the Phase2 hit collections')
options.register('geometry', 'DB:Extended', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'geometry of the SIM step: DB:Extended (GeometryRecoDB) or the name of a Configuration.Geometry.Geometry<name>Reco_cff, e.g. ExtendedRun4D110')
options.register('genStages', 5, VarParsing.multiplicity.singleton, VarParsing.varType.int,
                 'generator-record stages drawn by GenPartAnalyzer before the SIM pages of every event (0: none)')
options.register('maxPage', 5, VarParsing.multiplicity.singleton, VarParsing.varType.int,
                 'last SIM page to draw (1 primaries, 2 calo surface, 3 secondaries, 4 tracker/muon hits, 5 calo hits)')
options.register('simStage', 0, VarParsing.multiplicity.singleton, VarParsing.varType.int,
                 'SIM stage of the sample, highlighted in the right panel (1 transport, 2 + B, 3 + decays, 4 + EM, 5 + hadronic)')
options.register('etaMax', 6.0, VarParsing.multiplicity.singleton, VarParsing.varType.float, 'eta axis range of the plots')
options.register('labelPtMin', 5.0, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'pT above which the pT value is written next to every particle (b, bbar systems always)')
options.register('printTracks', False, VarParsing.multiplicity.singleton, VarParsing.varType.bool,
                 'print every SimTrack with its gen link, ancestor primary, creator process and system to stdout')
options.register('hcalTestNumbering', True, VarParsing.multiplicity.singleton, VarParsing.varType.bool,
                 'HCAL PCaloHit ids are in the simulation test numbering (Run 2/3 default), relabelled to HcalDetId')
options.register('pdfFile', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'if set, all plots go as pages into this single PDF instead of one PDF per page and event')
options.register('sampleLabel', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "text printed in the right panel of every plot, '|' separates lines, e.g. the SIM stage")
options.register('eventsToProcess', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'run:lumi:event (or a range) to analyse, e.g. 1:1:1; empty: the first maxEvents events')
options._tags, options._tagOrder = {}, []  # keep outputFile as given (VarParsing would append _numEvent<N> when maxEvents > 0)
options.parseArguments()

era = getattr(importlib.import_module('Configuration.Eras.Era_%s_cff' % options.era), options.era)
process = cms.Process('SIMANA', era)
process.load('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.cerr.FwkReport.reportEvery = 1

# reco geometry (tracker, muon, calorimeters) and conditions for the hit positions
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

prefix = options.outputFile.replace('.root', '')
if options.genStages > 0:
    process.genPartAnalyzer = cms.EDAnalyzer('GenPartAnalyzer',
        genParticles=cms.InputTag('genParticles', '', 'GEN'),
        outputPrefix=cms.string(prefix),
        etaMax=cms.double(options.etaMax),
        labelPtMin=cms.double(options.labelPtMin),
        savePDF=cms.bool(True),
        printParticles=cms.bool(False),
        maxStage=cms.int32(options.genStages),
        pdfFile=cms.string(options.pdfFile),
        sampleLabel=cms.string(options.sampleLabel),
        openPdf=cms.bool(True),    # opens the shared multi-page PDF (first module to print)
        closePdf=cms.bool(False),  # SimAnalyzer closes it
    )

# the PSimHit and PCaloHit collections of the SIM step (as dumped by SimJourney, jitJet_cff)
if options.era.startswith('Phase2'):
    # Phase2 (e.g. ExtendedRun4D110): inner and outer tracker share the pixel collection names; no ME0; the
    # endcap calorimeters are the HGCal; the MTD hits (FastTimerHitsBarrel/Endcap) are not in the tracking geometry
    tracker = ['TrackerHitsPixelBarrelLowTof', 'TrackerHitsPixelBarrelHighTof', 'TrackerHitsPixelEndcapLowTof',
               'TrackerHitsPixelEndcapHighTof', 'MuonDTHits', 'MuonCSCHits', 'MuonRPCHits', 'MuonGEMHits']
    calo = ['EcalHitsEB', 'HcalHits', 'HGCHitsEE', 'HGCHitsHEfront', 'HGCHitsHEback']
else:
    tracker = ['TrackerHitsPixelBarrelLowTof', 'TrackerHitsPixelBarrelHighTof', 'TrackerHitsPixelEndcapLowTof',
               'TrackerHitsPixelEndcapHighTof', 'TrackerHitsTIBLowTof', 'TrackerHitsTIBHighTof', 'TrackerHitsTIDLowTof',
               'TrackerHitsTIDHighTof', 'TrackerHitsTOBLowTof', 'TrackerHitsTOBHighTof', 'TrackerHitsTECLowTof',
               'TrackerHitsTECHighTof', 'MuonDTHits', 'MuonCSCHits', 'MuonRPCHits', 'MuonGEMHits']
    calo = ['EcalHitsEB', 'EcalHitsEE', 'EcalHitsES', 'HcalHits']
process.simAnalyzer = cms.EDAnalyzer('SimAnalyzer',
    genParticles=cms.InputTag('genParticles', '', 'GEN'),
    simTracks=cms.InputTag('g4SimHits'),
    simVertices=cms.InputTag('g4SimHits'),
    trackerHits=cms.VInputTag(*['g4SimHits:' + x for x in tracker]),
    caloHits=cms.VInputTag(*['g4SimHits:' + x for x in calo]),
    outputPrefix=cms.string(prefix),
    etaMax=cms.double(options.etaMax),
    labelPtMin=cms.double(options.labelPtMin),
    savePDF=cms.bool(True),
    printTracks=cms.bool(options.printTracks),
    hcalTestNumbering=cms.bool(options.hcalTestNumbering),
    maxPage=cms.int32(options.maxPage),
    simStage=cms.int32(options.simStage),
    pdfFile=cms.string(options.pdfFile),
    sampleLabel=cms.string(options.sampleLabel),
    openPdf=cms.bool(options.genStages <= 0),
    closePdf=cms.bool(True),
)

process.p = cms.Path(process.simAnalyzer)
if options.genStages > 0:
    # the framework orders the modules of a Path only across EDFilters, so an always-true filter keeps
    # the generator-stage pages of every event in front of the SIM pages (see genJetAnalyzer_cfg.py)
    process.genPartPagesFirst = cms.EDFilter('HLTBool', result=cms.bool(True))
    process.p = cms.Path(process.genPartAnalyzer + process.genPartPagesFirst + process.simAnalyzer)
