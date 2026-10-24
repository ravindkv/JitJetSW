# The SimHits of one event drawn on the detector, station by station (SimHitAnalyzer): for every station
# (TRK, ECAL, HCAL, HGCAL, MUON and ALL) an r-z view, a transverse (barrel) view and an eta-phi view of the
# hits, coloured by the kind of the primary (the particle handed to Geant4) the hit descends from, with the
# detector elements of the station from the reco geometry in grey and the leading primary of every kind
# followed through the layers. Output: <outputDir>/<station>/simHits_<station>.pdf (+ one PNG per page).
#
#   cmsRun simHitAnalyzer_cfg.py inputFiles=file:SIM_Phase2/step2_SIM_Transport_B_Decay_EM_Had_out.root \
#          outputDir=SIM_Phase2 era=Phase2C17I13M9 globalTag=auto:phase2_realistic_T33 geometry=ExtendedRun4D110 \
#          eventsToProcess=1:1:1 sampleLabel="sample Transport_B_Decay_EM_Had|Phase2 D110"
#
# globalTag, era and geometry are those of the SIM step (see simAnalyzer_cfg.py); with a Phase2* era the
# Phase2 collections and stations are used (HGCal instead of EE, ES and HE), otherwise the Run 3 ones.
import importlib
import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing('analysis')
options.inputFiles = 'file:SIM_Phase2/step2_SIM_Transport_B_Decay_EM_Had_out.root'
options.maxEvents = 1
options.register('globalTag', 'auto:phase2_realistic_T33', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'global tag of the geometry used for the hit positions (as in the SIM step)')
options.register('era', 'Phase2C17I13M9', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'era of the SIM step (Configuration.Eras.Era_<era>_cff); a Phase2* era selects the Phase2 collections')
options.register('geometry', 'ExtendedRun4D110', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'geometry of the SIM step: DB:Extended (GeometryRecoDB) or a Configuration.Geometry.Geometry<name>Reco_cff name')
options.register('eventsToProcess', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'run:lumi:event (or a range) to draw, e.g. 1:1:1; empty: the first maxEvents events')
options.register('outputDir', 'SIM_Phase2', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'base directory: <outputDir>/<station>/ receives the PDF and PNGs of every station')
options.register('stations', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'comma-separated subset of the stations (default all: TRK,ECAL,HCAL,HGCAL,MUON,ALL)')
options.register('sampleLabel', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "text printed in the right panel of every page, '|' separates lines")
options.register('highlight', 1, VarParsing.multiplicity.singleton, VarParsing.varType.int,
                 'leading primaries per kind whose hits are connected and labelled (0: none)')
options.register('highlightPtMin', 2.0, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'minimum pT of a followed primary [GeV]')
options.register('printHits', False, VarParsing.multiplicity.singleton, VarParsing.varType.bool,
                 'print every primary with its hit counts per station')
options.register('savePNG', True, VarParsing.multiplicity.singleton, VarParsing.varType.bool, 'also save every page as PNG')
options.register('hcalTestNumbering', True, VarParsing.multiplicity.singleton, VarParsing.varType.bool,
                 'HCAL PCaloHit ids are in the simulation test numbering, relabelled to HcalDetId')
options._tags, options._tagOrder = {}, []
options.parseArguments()

era = getattr(importlib.import_module('Configuration.Eras.Era_%s_cff' % options.era), options.era)
process = cms.Process('SIMHITS', era)
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

def tags(names):
    return cms.VInputTag(*['g4SimHits:' + n for n in names])

phase2 = options.era.startswith('Phase2')
pixel = ['TrackerHitsPixelBarrelLowTof', 'TrackerHitsPixelBarrelHighTof', 'TrackerHitsPixelEndcapLowTof', 'TrackerHitsPixelEndcapHighTof']
strip = [] if phase2 else ['TrackerHitsTIBLowTof', 'TrackerHitsTIBHighTof', 'TrackerHitsTIDLowTof', 'TrackerHitsTIDHighTof',
                           'TrackerHitsTOBLowTof', 'TrackerHitsTOBHighTof', 'TrackerHitsTECLowTof', 'TrackerHitsTECHighTof']
muon = ['MuonDTHits', 'MuonCSCHits', 'MuonRPCHits', 'MuonGEMHits']
ecal = ['EcalHitsEB'] if phase2 else ['EcalHitsEB', 'EcalHitsEE', 'EcalHitsES']
hgcal = ['HGCHitsEE', 'HGCHitsHEfront', 'HGCHitsHEback'] if phase2 else []
ecalGeo = ['EB'] if phase2 else ['EB', 'EE', 'ES']
hcalGeo = ['HB', 'HO', 'HF'] if phase2 else ['HB', 'HE', 'HO', 'HF']

def station(name, title, psim=[], calo=[], geometry=[], zMax=1300., rMax=800., xyMax=800., etaMax=5., caloEntry=False):
    return cms.PSet(name=cms.string(name), title=cms.string(title), psimHits=tags(psim), caloHits=tags(calo),
                    geometry=cms.vstring(*geometry), zMax=cms.double(zMax), rMax=cms.double(rMax), xyMax=cms.double(xyMax),
                    etaMax=cms.double(etaMax), drawCaloEntry=cms.bool(caloEntry))

stations = [
    station('TRK', 'tracker', psim=pixel + strip, geometry=['tracker'], zMax=300., rMax=125., xyMax=125., etaMax=4.5),
    station('ECAL', 'electromagnetic calorimeter' + (' (EB + HGCal EE)' if phase2 else ' (EB, EE, ES)'),
            calo=ecal + (['HGCHitsEE'] if phase2 else []), geometry=['tracker'] + ecalGeo + (['HGCEE'] if phase2 else []),
            zMax=420., rMax=200., xyMax=200., etaMax=3.2, caloEntry=True),
    station('HCAL', 'hadron calorimeter' + (' (HB, HO, HF + HGCal HSi, HSc)' if phase2 else ' (HB, HE, HO, HF)'),
            calo=['HcalHits'] + (['HGCHitsHEfront', 'HGCHitsHEback'] if phase2 else []),
            geometry=['tracker'] + ecalGeo + hcalGeo + (['HGCEE', 'HGCHSi', 'HGCHSc'] if phase2 else []),
            zMax=1300., rMax=450., xyMax=450., etaMax=5.2),
]
if phase2:
    stations.append(station('HGCAL', 'high-granularity endcap calorimeter (EE, HSi, HSc)', calo=hgcal,
                            geometry=['HGCEE', 'HGCHSi', 'HGCHSc'], zMax=560., rMax=280., xyMax=0., etaMax=3.2, caloEntry=True))
stations += [
    station('MUON', 'muon system (DT, CSC, RPC, GEM)', psim=muon, geometry=['DT', 'CSC', 'RPC', 'GEM'],
            zMax=1150., rMax=760., xyMax=760., etaMax=3.0),
    station('ALL', 'all subdetectors', psim=pixel + strip + muon, calo=ecal + ['HcalHits'] + hgcal,
            geometry=['tracker'] + ecalGeo + hcalGeo + (['HGCEE', 'HGCHSi', 'HGCHSc'] if phase2 else []) + ['DT', 'CSC', 'RPC', 'GEM'],
            zMax=1300., rMax=800., xyMax=800., etaMax=5.2),
]
if options.stations:
    wanted = options.stations.split(',')
    stations = [s for s in stations if s.name.value() in wanted]

process.simHitAnalyzer = cms.EDAnalyzer('SimHitAnalyzer',
    simTracks=cms.InputTag('g4SimHits'),
    simVertices=cms.InputTag('g4SimHits'),
    stations=cms.VPSet(*stations),
    outputDir=cms.string(options.outputDir),
    prefix=cms.string('simHits'),
    sampleLabel=cms.string(options.sampleLabel),
    savePNG=cms.bool(options.savePNG),
    hcalTestNumbering=cms.bool(options.hcalTestNumbering),
    printHits=cms.bool(options.printHits),
    highlight=cms.int32(options.highlight),
    highlightPtMin=cms.double(options.highlightPtMin),
    maxGeoPoints=cms.int32(12000),
)
process.p = cms.Path(process.simHitAnalyzer)
