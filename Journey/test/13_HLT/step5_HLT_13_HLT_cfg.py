# Auto generated configuration file
# using: 
# Revision: 1.19 
# Source: /local/reps/CMSSW/CMSSW/Configuration/Applications/python/ConfigBuilder.py,v 
# with command line options: step5 --filein file:/afs/cern.ch/work/r/rverma/public/JME/JitJet/CMSSW_15_0_5/src/JitJetSW/Journey/test/12_L1T/out/step4_L1_12_L1T.root --conditions auto:phase1_2024_realistic --era Run3_2024 --geometry DB:Extended -n -1 -s DIGI2RAW,HLT:GRun --datatier GEN-SIM-DIGI-RAW --eventcontent FEVTDEBUGHLT --fileout file:step5_HLT_13_HLT.root --customise_commands from JitJetSW.Journey.hltL1Menu_cff import adaptSeedsToL1Menu; process = adaptSeedsToL1Menu(process, '/afs/cern.ch/work/r/rverma/public/JME/JitJet/CMSSW_15_0_5/src/JitJetSW/Journey/test/12_L1T/out/l1t_12_L1T.log')\nprocess.FEVTDEBUGHLToutput.outputCommands.extend(['keep *_hltFixedGridRhoFastjetAll_*_*', 'keep *_hltFixedGridRhoFastjetAllCalo_*_*', 'keep *_hltVerticesPF_*_*', 'keep *_hltDoubletRecoveryPFlowTrackSelectionHighPurity_*_*', 'keep recoPFClusters_hltParticleFlowCluster*_*_*', 'keep recoPFRecHits_hltParticleFlowRecHit*_*_*', 'keep EcalRecHitsSorted_hltEcalRecHit_*_*', 'keep HBHERecHitsSorted_hltHbhereco_*_*', 'keep HFRecHitsSorted_hltHfreco_*_*', 'keep SiPixelClusteredmNewDetSetVector_hltSiPixelClusters_*_*', 'keep SiStripClusteredmNewDetSetVector_hltSiStripRawToClustersFacility_*_*', 'keep recoRecoChargedCandidates_*_*_HLT', 'keep recoMuons_*_*_HLT', 'keep recoTracks_hltL2Muons_*_*', 'keep triggerTriggerFilterObjectWithRefs_*_*_HLT'])\nprocess.load('L1Trigger.L1TZDC.l1tZDCEtSums_cfi'); process.load('L1Trigger.L1TTwinMux.simTwinMuxDigis_cfi'); process.load('L1Trigger.Configuration.SimL1TechnicalTriggers_cff')\nprocess.digi2raw_step.insert(0, process.simGtExtFakeStage2Digis); process.digi2raw_step.insert(0, process.simTwinMuxDigis); process.digi2raw_step.insert(0, process.l1tZDCEtSums)\nfrom L1Trigger.L1TTwinMux.fakeTwinMuxParams_cff import twinmuxParamsSource, fakeTwinMuxParams; process.twinmuxParamsSource = twinmuxParamsSource; process.fakeTwinMuxParams = fakeTwinMuxParams\ndel process.ctppsPixelRawData\nprocess.source.eventsToProcess = cms.untracked.VEventRange('1:1:1') --python_filename /afs/cern.ch/work/r/rverma/public/JME/JitJet/CMSSW_15_0_5/src/JitJetSW/Journey/test/13_HLT/step5_HLT_13_HLT_cfg.py --no_exec
import FWCore.ParameterSet.Config as cms

from Configuration.Eras.Era_Run3_2024_cff import Run3_2024

process = cms.Process('HLT',Run3_2024)

# import of standard configurations
process.load('Configuration.StandardSequences.Services_cff')
process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.EventContent.EventContent_cff')
process.load('SimGeneral.MixingModule.mixNoPU_cfi')
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')
process.load('Configuration.StandardSequences.DigiToRaw_cff')
process.load('HLTrigger.Configuration.HLT_GRun_cff')
process.load('Configuration.StandardSequences.EndOfProcess_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(-1),
    output = cms.optional.untracked.allowed(cms.int32,cms.PSet)
)

# Input source
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring('file:/afs/cern.ch/work/r/rverma/public/JME/JitJet/CMSSW_15_0_5/src/JitJetSW/Journey/test/12_L1T/out/step4_L1_12_L1T.root'),
    secondaryFileNames = cms.untracked.vstring()
)

process.options = cms.untracked.PSet(
    IgnoreCompletely = cms.untracked.vstring(),
    Rethrow = cms.untracked.vstring(),
    TryToContinue = cms.untracked.vstring(),
    accelerators = cms.untracked.vstring('*'),
    allowUnscheduled = cms.obsolete.untracked.bool,
    canDeleteEarly = cms.untracked.vstring(),
    deleteNonConsumedUnscheduledModules = cms.untracked.bool(True),
    dumpOptions = cms.untracked.bool(False),
    emptyRunLumiMode = cms.obsolete.untracked.string,
    eventSetup = cms.untracked.PSet(
        forceNumberOfConcurrentIOVs = cms.untracked.PSet(
            allowAnyLabel_=cms.required.untracked.uint32
        ),
        numberOfConcurrentIOVs = cms.untracked.uint32(0)
    ),
    fileMode = cms.untracked.string('FULLMERGE'),
    forceEventSetupCacheClearOnNewRun = cms.untracked.bool(False),
    holdsReferencesToDeleteEarly = cms.untracked.VPSet(),
    makeTriggerResults = cms.obsolete.untracked.bool,
    modulesToCallForTryToContinue = cms.untracked.vstring(),
    modulesToIgnoreForDeleteEarly = cms.untracked.vstring(),
    numberOfConcurrentLuminosityBlocks = cms.untracked.uint32(0),
    numberOfConcurrentRuns = cms.untracked.uint32(1),
    numberOfStreams = cms.untracked.uint32(0),
    numberOfThreads = cms.untracked.uint32(1),
    printDependencies = cms.untracked.bool(False),
    sizeOfStackForThreadsInKB = cms.optional.untracked.uint32,
    throwIfIllegalParameter = cms.untracked.bool(True),
    wantSummary = cms.untracked.bool(False)
)

# Production Info
process.configurationMetadata = cms.untracked.PSet(
    annotation = cms.untracked.string('step5 nevts:-1'),
    name = cms.untracked.string('Applications'),
    version = cms.untracked.string('$Revision: 1.19 $')
)

# Output definition

process.FEVTDEBUGHLToutput = cms.OutputModule("PoolOutputModule",
    dataset = cms.untracked.PSet(
        dataTier = cms.untracked.string('GEN-SIM-DIGI-RAW'),
        filterName = cms.untracked.string('')
    ),
    fileName = cms.untracked.string('file:step5_HLT_13_HLT.root'),
    outputCommands = process.FEVTDEBUGHLTEventContent.outputCommands,
    splitLevel = cms.untracked.int32(0)
)

# Additional output definition

# Other statements
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase1_2024_realistic', '')

# Path and EndPath definitions
process.digi2raw_step = cms.Path(process.DigiToRaw)
process.endjob_step = cms.EndPath(process.endOfProcess)
process.FEVTDEBUGHLToutput_step = cms.EndPath(process.FEVTDEBUGHLToutput)

# Schedule definition
# process.schedule imported from cff in HLTrigger.Configuration
process.schedule.insert(0, process.digi2raw_step)
process.schedule.extend([process.endjob_step,process.FEVTDEBUGHLToutput_step])
from PhysicsTools.PatAlgos.tools.helpers import associatePatAlgosToolsTask
associatePatAlgosToolsTask(process)

# customisation of the process.

# Automatic addition of the customisation function from HLTrigger.Configuration.customizeHLTforMC
from HLTrigger.Configuration.customizeHLTforMC import customizeHLTforMC 

#call to customisation function customizeHLTforMC imported from HLTrigger.Configuration.customizeHLTforMC
process = customizeHLTforMC(process)

# End of customisation functions


# Customisation from command line

from JitJetSW.Journey.hltL1Menu_cff import adaptSeedsToL1Menu; process = adaptSeedsToL1Menu(process, '/afs/cern.ch/work/r/rverma/public/JME/JitJet/CMSSW_15_0_5/src/JitJetSW/Journey/test/12_L1T/out/l1t_12_L1T.log')
process.FEVTDEBUGHLToutput.outputCommands.extend(['keep *_hltFixedGridRhoFastjetAll_*_*', 'keep *_hltFixedGridRhoFastjetAllCalo_*_*', 'keep *_hltVerticesPF_*_*', 'keep *_hltDoubletRecoveryPFlowTrackSelectionHighPurity_*_*', 'keep recoPFClusters_hltParticleFlowCluster*_*_*', 'keep recoPFRecHits_hltParticleFlowRecHit*_*_*', 'keep EcalRecHitsSorted_hltEcalRecHit_*_*', 'keep HBHERecHitsSorted_hltHbhereco_*_*', 'keep HFRecHitsSorted_hltHfreco_*_*', 'keep SiPixelClusteredmNewDetSetVector_hltSiPixelClusters_*_*', 'keep SiStripClusteredmNewDetSetVector_hltSiStripRawToClustersFacility_*_*', 'keep recoRecoChargedCandidates_*_*_HLT', 'keep recoMuons_*_*_HLT', 'keep recoTracks_hltL2Muons_*_*', 'keep triggerTriggerFilterObjectWithRefs_*_*_HLT'])
process.load('L1Trigger.L1TZDC.l1tZDCEtSums_cfi'); process.load('L1Trigger.L1TTwinMux.simTwinMuxDigis_cfi'); process.load('L1Trigger.Configuration.SimL1TechnicalTriggers_cff')
process.digi2raw_step.insert(0, process.simGtExtFakeStage2Digis); process.digi2raw_step.insert(0, process.simTwinMuxDigis); process.digi2raw_step.insert(0, process.l1tZDCEtSums)
from L1Trigger.L1TTwinMux.fakeTwinMuxParams_cff import twinmuxParamsSource, fakeTwinMuxParams; process.twinmuxParamsSource = twinmuxParamsSource; process.fakeTwinMuxParams = fakeTwinMuxParams
del process.ctppsPixelRawData
process.source.eventsToProcess = cms.untracked.VEventRange('1:1:1')
# Add early deletion of temporary data products to reduce peak memory need
from Configuration.StandardSequences.earlyDeleteSettings_cff import customiseEarlyDelete
process = customiseEarlyDelete(process)
# End adding early deletion
