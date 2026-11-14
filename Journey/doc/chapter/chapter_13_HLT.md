# Chapter 13, HLT: technical companion

The High Level Trigger of the ONE event, in five steps. One sample: the event as CMS sees it, the signal with its pileup
(PU = 60 per bunch crossing, bx -1..1; Rule H of the Constitution: from chapter 12 on only the mixed event is followed).
Script: `test/13_HLT/run_13_HLT.sh`; everything it produces is in `test/13_HLT/out/`. The chapter text is
`doc/chapter/chapter_13_HLT.tex`, a checkpoint chapter (Rule G: B1, B2, one section per step, B4, B5). All commands below
are run from `test/` of an initialized CMSSW_15_0_5 (`cmsenv`), with `J=$CMSSW_BASE/src/JitJetSW/Journey`.

## 1. The cmsDriver.py commands

The script runs `cmsDriver.py` once through `run_driver` of `test/common/jitjet_common.sh`. The command, from the header
of the generated configuration (`test/13_HLT/step5_HLT_13_HLT_cfg.py`, "with command line options"; the cmsDriver output
is in `out/step5_HLT_13_HLT_cfg.driver.log`):

```bash
cd $J/test
cmsDriver.py step5 --filein file:$J/test/12_L1T/out/step4_L1_12_L1T.root \
  --conditions auto:phase1_2025_realistic --era Run3_2025 --geometry DB:Extended \
  -n -1 -s DIGI2RAW,HLT:@relval2025 --datatier GEN-SIM-DIGI-RAW --eventcontent FEVTDEBUGHLT --fileout file:step5_HLT_13_HLT.root \
  --customise_commands "from JitJetSW.Journey.hltL1Menu_cff import adaptSeedsToL1Menu; process = adaptSeedsToL1Menu(process, '$J/test/12_L1T/out/l1t_12_L1T.log')
process.FEVTDEBUGHLToutput.outputCommands.extend(['keep *_hltFixedGridRhoFastjetAll_*_*', 'keep *_hltFixedGridRhoFastjetAllCalo_*_*', 'keep *_hltVerticesPF_*_*', 'keep *_hltDoubletRecoveryPFlowTrackSelectionHighPurity_*_*', 'keep recoPFClusters_hltParticleFlowCluster*_*_*', 'keep recoPFRecHits_hltParticleFlowRecHit*_*_*', 'keep EcalRecHitsSorted_hltEcalRecHit_*_*', 'keep HBHERecHitsSorted_hltHbhereco_*_*', 'keep HFRecHitsSorted_hltHfreco_*_*', 'keep SiPixelClusteredmNewDetSetVector_hltSiPixelClusters_*_*', 'keep SiStripClusteredmNewDetSetVector_hltSiStripRawToClustersFacility_*_*', 'keep recoRecoChargedCandidates_*_*_HLT', 'keep recoMuons_*_*_HLT', 'keep recoTracks_hltL2Muons_*_*', 'keep triggerTriggerFilterObjectWithRefs_*_*_HLT'])
process.load('L1Trigger.L1TZDC.l1tZDCEtSums_cfi'); process.load('L1Trigger.L1TTwinMux.simTwinMuxDigis_cfi'); process.load('L1Trigger.Configuration.SimL1TechnicalTriggers_cff')
process.digi2raw_step.insert(0, process.simGtExtFakeStage2Digis); process.digi2raw_step.insert(0, process.simTwinMuxDigis); process.digi2raw_step.insert(0, process.l1tZDCEtSums)
from L1Trigger.L1TTwinMux.fakeTwinMuxParams_cff import twinmuxParamsSource, fakeTwinMuxParams; process.twinmuxParamsSource = twinmuxParamsSource; process.fakeTwinMuxParams = fakeTwinMuxParams
del process.ctppsPixelRawData
process.source.eventsToProcess = cms.untracked.VEventRange('1:1:1')" \
  --python_filename $J/test/13_HLT/step5_HLT_13_HLT_cfg.py --no_exec
```

(the script passes the customise lines joined with `\n`; one line per customise is what cmsDriver executes, so an import
and its use share a line).

| argument | meaning |
|---|---|
| `step5` | the name of the configuration (free; `step4` was the L1 of chapter 12) |
| `--filein file:...` | the input (Rule E): the L1 file of chapter 12, the digis of chapter 11 with the Level-1 products |
| `--conditions auto:phase1_2025_realistic` | the global tag of the CMS chapters (Rule I), `150X_mcRun3_2025_realistic_v1`. For the HLT it provides the calibrations of every subdetector, the online beam spot (`BeamSpotOnlineHLTObjectsRcd`: `BeamSpotOnlineObjects_Realistic25ns_13p6TeVCollisions_Winter25_mc_v1`; 2024: `..._Early2024_v1_mc`), the Level-1 menu that the HLT re-emulates (`L1TUtmTriggerMenuRcd`: `L1Menu_Collisions2025_v1_0_0_xml`), the hadronic particle-flow thresholds (`HcalPFCutsRcd`: `HcalPFCuts_2025_mc`, 2024: `HcalPFCuts_2023_V1.0_mc`; read by the 26 modules of the menu with `usePFThresholdsFromDB = True`), the jet energy corrections of the HLT (`JetCorrectionsRecord` labels `AK4CaloHLT`: `JetCorrectorParametersCollection_Run3Winter24Digi_AK4CaloHLT`, `AK4PFHLT`: `..._Run3Winter24Digi_AK4PFHLT`, the same payloads as in the 2024 global tag) and the particle-flow hadron calibration (`PFCalibrationRcd` label `HLT`: `PFCalibration_Run3_2024_online_v1`, also unchanged); `conddb list 150X_mcRun3_2025_realistic_v1 \| grep -i "HLT"`. The other records of the 2025 tag that the HLT reads and that differ from 2024 are those of chapter 11: ECAL pedestals, laser corrections and channel status, HCAL pedestals, gains (`HcalRespCorrs`) and SiPM parameters, the pixel templates, generic errors and Lorentz angle (`..._phase1_EoR3_HV800_Tr2000...`), the strip bad components, the tracker alignment (`TrackerAlignment_Run3MC_2025_EOY_v1`) |
| `--era Run3_2025` | the era of the DIGI2RAW packers; the HLT menu itself carries no era modifier (the five modifiers that `Run3_2025` adds to `Run3_2024` act on the offline local reconstruction, the Level-1 emulation and the DQM: `gemRecHits`, `cscTriggerPrimitiveDigis`, `simCaloStage2Layer1Summary`, `L1TStage2uGTTiming`, the offline pixel clusterizer, the offline ECAL rec-hits; neither on the packers nor on the `_cff` of the menu) |
| `--geometry DB:Extended` | the reconstruction geometry from the database |
| `-n -1` with `eventsToProcess` | all events of the input, restricted to the one event |
| `-s DIGI2RAW,HLT:@relval2025` | two steps: `DIGI2RAW` (`Configuration/StandardSequences/python/DigiToRaw_cff.py`: the packers of every subdetector and of the Level-1 boards) and the HLT menu that the release gives the 2025 simulation: `Configuration/HLT/python/autoHLT.py` maps `'relval2025'` to `'2025v10'`, the file `HLTrigger/Configuration/python/HLT_2025v10_cff.py`, the frozen 2025 menu `/frozen/2025/2e34/v1.0/HLT/V2` (the RelVal workflows of 2025 in `Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py` use the same `'HLTmenu': '@relval2025'`). It is the physics menu `GRun` of the release frozen: `diff HLT_2025v10_cff.py HLT_GRun_cff.py` gives 10 lines, the two header comments, the `tableName` (`/dev/CMSSW_15_0_0/GRun/V62` in `GRun`) and the menu-type argument of the last two calls, `customizeHLTforAll(fragment, "2025v10")` and `customizeHLTforCMSSW(fragment, "2025v10")`, which neither function uses (`HLTrigger/Configuration/python/customizeHLTforALL.py`, `customizeHLTforCMSSW.py`). Every module, path, seed and threshold is that of `GRun`, which the 2024 version of this chapter ran. For comparison: `HLT:@relval2024` is `Fake2` (three paths `HLT_Physics`, `HLT_Random`, `HLT_ZeroBias`); the release has no 2024 physics menu |
| `--datatier GEN-SIM-DIGI-RAW --eventcontent FEVTDEBUGHLT` | the content: the input copied, the raw data (`rawDataCollector`), and the `HLTDebugFEVT` commands of `HLTrigger/Configuration/python/HLTrigger_EventContent_cff.py` (the calo and PF jets, the pixel and full tracks, the pixel vertices, the PF candidates, `hltTriggerSummaryAOD`/`RAW`, the unpacked L1 record) |
| `--customise_commands "..."` | the lines explained below |
| `--python_filename ... --no_exec` | write the configuration and do not run it; the script runs `cmsRun` itself |

The customise lines.

1. `adaptSeedsToL1Menu(process, '<12_L1T/out/l1t_12_L1T.log>')` (`python/hltL1Menu_cff.py`). A check that every Level-1
   algorithm named in a seed of the menu (`HLTL1TSeed`, `L1SeedsLogicalExpression`) is in the Level-1 menu of chapter 12
   (`L1Menu_Collisions2025_v1_0_0`, 465 algorithms, read from the `algo:` lines of the chapter-12 log, Rule E); a
   missing algorithm would stop the job (`HLTL1TSeed` throws `Algorithm L1_X, requested as seed by a HLT path, cannot be
   matched to a L1 algo name in any GlobalObjectMap`). With the 2025 Level-1 menu the report is
   `hltL1Menu_cff: 465 L1 algorithms in the menu of chapter 12; 0 seeds changed, 0 paths removed`: the menu runs
   complete, all 898 paths (`out/step5_HLT_13_HLT_cfg.driver.log`, `out/step5_HLT_13_HLT.log`, `out/menu_13_HLT.txt`,
   section 1). With the 2024 Level-1 menu (`L1Menu_Collisions2024_v1_3_0`, 408 algorithms) the same call found 30
   algorithms of the seeds missing, dropped them from 21 OR seeds and removed the 37 paths whose seed could never fire
   (898 paths became 861; section 4, "The 2024 and the 2025 settings"). The call stays as the guard of that agreement.
2. `process.FEVTDEBUGHLToutput.outputCommands.extend([...])`: products the analyzers need that `FEVTDEBUGHLT` does not
   keep: the two energy densities rho, the PF vertices, the second tracking iteration, the PF clusters and rec-hits, the
   calorimeter rec-hits, the pixel and strip clusters, the HLT muons, and the `TriggerFilterObjectWithRefs` of every filter.
3. and 4. The packers of `DIGI2RAW` read three products of the L1 job that the `FEVTDEBUG` content of chapter 12 does not
   keep (in production L1 and DIGI2RAW run in the same job). Without them the job stops with `ProductNotFound` (first
   `l1tZDCEtSums` for `gtStage2Raw`, then `simTwinMuxDigis` for `bmtfStage2Raw` and `omtfStage2Raw`, then
   `simGtExtFakeStage2Digis` for `gtStage2Raw`). They are remade at the head of `digi2raw_step` with the configuration of
   the L1 job (checked: the `dumpPython()` of the three modules is identical to that of `test/12_L1T/step4_L1_12_L1T_cfg.py`);
   the TwinMux needs its event-setup parameters (`L1TTwinMuxParamsRcd`, `fakeTwinMuxParams` of
   `L1Trigger/L1TTwinMux/python/fakeTwinMuxParams_cff.py`, as in the L1 job).
5. `del process.ctppsPixelRawData`: the PPS (forward proton spectrometer) is not simulated in this chain, there are no PPS
   pixel digis (`RPixDetDigitizer`) to pack.
6. `eventsToProcess = '1:1:1'`: the event of the book.

## 2. The cmsRun on the python config

```bash
cd $J/test/13_HLT/out
cmsRun ../step5_HLT_13_HLT_cfg.py > step5_HLT_13_HLT.log 2>&1      # about 60 s: 53 s to start, 6 s for the event
```

Or the whole chapter: `./13_HLT/run_13_HLT.sh` (106 s without reuse on 12 November 2026: 12 s for cmsDriver, 61 s for
the HLT job, 33 s for the analyzers and the two scripts; variables: `REUSE=1` keeps an existing `step5_HLT_13_HLT.root`,
`REUSE=0` remakes it, `RUN=0` only writes the configuration, `ANALYZE=0` skips the analyzers, `PNG=0` skips the PNG
pages, `MENU=@relval2025` the menu of `-s HLT:<MENU>`; `MENU=GRun` gives the same modules under the table name
`/dev/CMSSW_15_0_0/GRun/V62`).

### Walk-through of `step5_HLT_13_HLT_cfg.py`

- `process = cms.Process('HLT', Run3_2025)`: the process name under which the new products are stored (an analysis asks
  for `TriggerResults::HLT`).
- `process.load(...)`: `Services_cff`, `pythiapdt_cfi`, the `MessageLogger`, `EventContent_cff`, `mixNoPU_cfi` (loaded by
  cmsDriver for every step, not used), `GeometryRecoDB_cff`, `MagneticField_cff`, `DigiToRaw_cff` (the packers),
  `HLTrigger.Configuration.HLT_2025v10_cff` (the menu: 87 551 lines, the paths, sequences, modules, event-setup producers
  and the `datasets` / `streams` PSets of `/frozen/2025/2e34/v1.0/HLT/V2`), `EndOfProcess_cff`,
  `FrontierConditions_GlobalTag_cff`.
- `process.digi2raw_step = cms.Path(process.DigiToRaw)`; then the schedule of the menu (`process.schedule`, from the
  `_cff`), with `digi2raw_step` inserted first and `endjob_step`, `FEVTDEBUGHLToutput_step` appended.
- `customizeHLTforMC(process)` (added by cmsDriver for simulation; in this release it changes nothing) and
  `customiseEarlyDelete` (frees intermediate products as soon as their last consumer has run).
- The customise lines of section 1.

The menu in numbers (`out/menu_13_HLT.txt`, section 1, written by `test/common/hltMenu.py` from the configuration):

```
== 1. The menu
process HLT, table /frozen/2025/2e34/v1.0/HLT/V2
paths: 899 | AlCa=11 DQM=3 DST=21 Dataset=99 HLT=733 HLTriggerFinalPath=1 HLTriggerFirstPath=1 MC=26 ScoutingPFOutput=1 Status=2 digi2raw=1
endpaths: HLTAnalyzerEndpath endjob_step FEVTDEBUGHLToutput_step
datasets: 99
streams: 68
modules: 1337 producers, 3473 filters, 3 analyzers; 402 C++ types
event-setup: 220 producers, 16 sources
   HLTL1TSeed               224
   HLTPrescaler             893
   HLT1PFJet                155
   HLT1CaloJet              111
   TriggerResultsFilter     48
adapted to the L1 menu of chapter 12 (python/hltL1Menu_cff.py): 0 seeds changed, 0 paths removed
prescales: no PrescaleService: every HLTPrescaler has prescale 1
HLT_PFJet110: datasets JetMET0 JetMET1 OnlineMonitor; streams DQM PhysicsJetMET0 PhysicsJetMET1
HLT_PFJetFwd140: datasets JetMET0 JetMET1 OnlineMonitor; streams DQM PhysicsJetMET0 PhysicsJetMET1
HLT_DiPFJetAve100_HFJEC: datasets JetMET0 JetMET1 OnlineMonitor; streams DQM PhysicsJetMET0 PhysicsJetMET1
HLT_PFJet40: datasets JetMET0 JetMET1 OnlineMonitor; streams DQM PhysicsJetMET0 PhysicsJetMET1

```

`paths: 899` is the 898 paths of the menu and `digi2raw_step`; 733 `HLT_` and 21 `DST_` paths, 754 in all, are the
physics and scouting paths that the chapter counts. The module counts are those of the configuration and are the same
as in 2024, when the 37 removed paths left their modules defined (the modules that run, as `HLTConfigProvider` sees
them, are in the `menu modules:` line of section 4: 4783 distinct, against 4598 with the 861 paths of 2024).

The anatomy of the path of the book (`out/menu_13_HLT.txt`, section 2; a module met again in a later sequence runs once,
so the 209 entries of the path, `nModules=209` in `hlt_13_HLT.log`, are 174 distinct modules; sections 2 and 3 of
`menu_13_HLT.txt` are identical to those of the 2024 run with `GRun`, the path is the same module for module):

```
-- HLT_PFJet110_v13: 174 modules
     0 hltTriggerType                                             HLTTriggerTypeFilter
     1 hltGtStage2Digis                                           L1TRawToDigi
     2 hltGtStage2ObjectMap                                       L1TGlobalProducer
     3 hltOnlineMetaDataDigis                                     OnlineMetaDataRawToDigi
     4 hltOnlineBeamSpot                                          BeamSpotOnlineProducer
     5 hltL1sSingleJet90                                          HLTL1TSeed                                   seed "L1_SingleJet90"
     6 hltPrePFJet110                                             HLTPrescaler
     7 hltEcalDigisLegacy                                         EcalRawToDigi
     8 hltEcalDigisSoA                                            EcalRawToDigiPortable@alpaka
     9 hltEcalDigis                                               EcalDigisFromPortableProducer
    10 hltEcalUncalibRecHitSoA                                    EcalUncalibRecHitProducerPortable@alpaka
    11 hltEcalUncalibRecHit                                       EcalUncalibRecHitSoAToLegacy
    12 hltEcalDetIdToBeRecovered                                  EcalDetIdToBeRecoveredProducer
    13 hltEcalRecHit                                              EcalRecHitProducer
    14 hltHcalDigis                                               HcalRawToDigi
    15 hltHcalDigisSoA                                            HcalDigisSoAProducer@alpaka
    16 hltHbheRecoSoA                                             HBHERecHitProducerPortable@alpaka
    17 hltHbhereco                                                HcalRecHitSoAToLegacy
    18 hltHfprereco                                               HFPreReconstructor
    19 hltHfreco                                                  HFPhase1Reconstructor
    20 hltHoreco                                                  HcalHitReconstructor
    21 hltTowerMakerForAll                                        CaloTowersCreator
    22 hltAK4CaloJets                                             FastjetJetProducer
    23 hltAK4CaloJetsIDPassed                                     HLTCaloJetIDProducer
    24 hltFixedGridRhoFastjetAllCalo                              FixedGridRhoProducerFastjet
    25 hltAK4CaloFastJetCorrector                                 L1FastjetCorrectorProducer
    26 hltAK4CaloRelativeCorrector                                LXXXCorrectorProducer
    27 hltAK4CaloAbsoluteCorrector                                LXXXCorrectorProducer
    28 hltAK4CaloResidualCorrector                                LXXXCorrectorProducer
    29 hltAK4CaloCorrector                                        ChainedJetCorrectorProducer
    30 hltAK4CaloJetsCorrected                                    CorrectedCaloJetProducer
    31 hltAK4CaloJetsCorrectedIDPassed                            CorrectedCaloJetProducer
    32 hltSingleAK4CaloJet80                                      HLT1CaloJet                                  MinPt 80 MinEta -1 MaxEta 5 input hltAK4CaloJetsCorrectedIDPassed
    33 hltAK4CaloJetsPF                                           FastjetJetProducer
    34 hltAK4CaloJetsPFEt5                                        EtMinCaloJetSelector
    35 hltMuonDTDigis                                             DTuROSRawToDigi
    36 hltDt1DRecHits                                             DTRecHitProducer
    37 hltDt4DSegments                                            DTRecSegment4DProducer
    38 hltMuonCSCDigis                                            CSCDCCUnpacker
    39 hltCsc2DRecHits                                            CSCRecHitDProducer
    40 hltCscSegments                                             CSCSegmentProducer
    41 hltMuonRPCDigisCPPF                                        RPCAMCRawToDigi
    42 hltOmtfDigis                                               OmtfUnpacker
    43 hltMuonRPCDigisTwinMux                                     RPCTwinMuxRawToDigi
    44 hltMuonRPCDigis                                            RPCDigiMerger
    45 hltRpcRecHits                                              RPCRecHitProducer
    46 hltMuonGEMDigis                                            GEMRawToDigiModule
    47 hltGemRecHits                                              GEMRecHitProducer
    48 hltGemSegments                                             GEMSegmentProducer
    49 hltL2OfflineMuonSeeds                                      MuonSeedGenerator
    50 hltL2MuonSeeds                                             L2MuonSeedGeneratorFromL1T
    51 hltL2Muons                                                 L2MuonProducer
    52 hltL2MuonCandidates                                        L2MuonCandidateProducer
    53 hltOnlineBeamSpotDevice                                    BeamSpotDeviceProducer@alpaka
    54 hltSiPixelClustersSoA                                      SiPixelRawToClusterPhase1@alpaka
    55 hltSiPixelClusters                                         SiPixelDigisClustersFromSoAAlpakaPhase1
    56 hltSiPixelDigiErrors                                       SiPixelDigiErrorsFromSoAAlpaka
    57 hltSiPixelRecHitsSoA                                       SiPixelRecHitAlpakaPhase1@alpaka
    58 hltSiPixelRecHits                                          SiPixelRecHitFromSoAAlpakaPhase1
    59 hltSiStripExcludedFEDListProducer                          SiStripExcludedFEDListProducer
    60 hltSiStripRawToClustersFacility                            SiStripClusterizerFromRaw
    61 hltMeasurementTrackerEvent                                 MeasurementTrackerEventProducer
    62 hltSiStripRecHits                                          SiStripRecHitConverter
    63 hltIterL3OISeedsFromL2Muons                                TSGForOIDNN
    64 hltIterL3OITrackCandidates                                 CkfTrackCandidateMaker
    65 hltIterL3OIMuCtfWithMaterialTracks                         TrackProducer
    66 hltIterL3OIMuonTrackCutClassifier                          TrackCutClassifier
    67 hltIterL3OIMuonTrackSelectionHighPurity                    TrackCollectionFilterCloner
    68 hltL3MuonsIterL3OI                                         L3MuonProducer
    69 hltIterL3OIL3MuonsLinksCombination                         L3TrackLinksCombiner
    70 hltIterL3OIL3Muons                                         L3TrackCombiner
    71 hltIterL3OIL3MuonCandidates                                L3MuonCandidateProducer
    72 hltL2SelectorForL3IO                                       HLTMuonL2SelectorForL3IO
    73 hltPixelTracksSoA                                          CAHitNtupletAlpakaPhase1@alpaka
    74 hltPixelTracks                                             PixelTrackProducerFromSoAAlpakaPhase1
    75 hltPixelVerticesSoA                                        PixelVertexProducerAlpakaPhase1@alpaka
    76 hltPixelVertices                                           PixelVertexProducerFromSoAAlpaka
    77 hltTrimmedPixelVertices                                    PixelVertexCollectionTrimmer
    78 hltIterL3MuonPixelTracksTrackingRegions                    MuonTrackingRegionByPtEDProducer
    79 hltPixelTracksInRegionL2                                   TrackSelectorByRegion
    80 hltIter0IterL3MuonPixelSeedsFromPixelTracks                SeedGeneratorFromProtoTracksEDProducer
    81 hltIter0IterL3MuonPixelSeedsFromPixelTracksFiltered        MuonHLTSeedMVAClassifier
    82 hltIter0IterL3MuonCkfTrackCandidates                       CkfTrackCandidateMaker
    83 hltIter0IterL3MuonCtfWithMaterialTracks                    TrackProducer
    84 hltIter0IterL3MuonTrackCutClassifier                       TrackCutClassifier
    85 hltIter0IterL3MuonTrackSelectionHighPurity                 TrackCollectionFilterCloner
    86 hltL3MuonsIterL3IO                                         L3MuonProducer
    87 hltIterL3MuonsFromL2LinksCombination                       L3TrackLinksCombiner
    88 hltL1MuonsPt0                                              HLTL1TMuonSelector
    89 hltIterL3FromL1MuonPixelTracksTrackingRegions              L1MuonSeededTrackingRegionsEDProducer
    90 hltPixelTracksInRegionL1                                   TrackSelectorByRegion
    91 hltIter0IterL3FromL1MuonPixelSeedsFromPixelTracks          SeedGeneratorFromProtoTracksEDProducer
    92 hltIter0IterL3FromL1MuonPixelSeedsFromPixelTracksFiltered  MuonHLTSeedMVAClassifier
    93 hltIter0IterL3FromL1MuonCkfTrackCandidates                 CkfTrackCandidateMaker
    94 hltIter0IterL3FromL1MuonCtfWithMaterialTracks              TrackProducer
    95 hltIter0IterL3FromL1MuonTrackCutClassifier                 TrackCutClassifier
    96 hltIter0IterL3FromL1MuonTrackSelectionHighPurity           TrackCollectionFilterCloner
    97 hltIter3IterL3FromL1MuonClustersRefRemoval                 TrackClusterRemover
    98 hltIter3IterL3FromL1MuonMaskedMeasurementTrackerEvent      MaskedMeasurementTrackerEventProducer
    99 hltIter3IterL3FromL1MuonPixelLayersAndRegions              PixelInactiveAreaTrackingRegionsSeedingLayersProducer
   100 hltIter3IterL3FromL1MuonTrackingRegions                    L1MuonSeededTrackingRegionsEDProducer
   101 hltIter3IterL3FromL1MuonPixelClusterCheck                  ClusterCheckerEDProducer
   102 hltIter3IterL3FromL1MuonPixelHitDoublets                   HitPairEDProducer
   103 hltIter3IterL3FromL1MuonPixelSeeds                         SeedCreatorFromRegionConsecutiveHitsEDProducer
   104 hltIter3IterL3FromL1MuonPixelSeedsFiltered                 MuonHLTSeedMVAClassifier
   105 hltIter3IterL3FromL1MuonCkfTrackCandidates                 CkfTrackCandidateMaker
   106 hltIter3IterL3FromL1MuonCtfWithMaterialTracks              TrackProducer
   107 hltIter3IterL3FromL1MuonTrackCutClassifier                 TrackCutClassifier
   108 hltIter3IterL3FromL1MuonTrackSelectionHighPurity           TrackCollectionFilterCloner
   109 hltIter03IterL3FromL1MuonMerged                            TrackListMerger
   110 hltIterL3MuonMerged                                        TrackListMerger
   111 hltIterL3MuonAndMuonFromL1Merged                           TrackListMerger
   112 hltIterL3GlbMuon                                           L3MuonProducer
   113 hltIterL3MuonsNoID                                         MuonIdProducer
   114 hltIterL3Muons                                             MuonIDFilterProducerForHLT
   115 hltL3MuonsIterL3Links                                      MuonLinksProducer
   116 hltIterL3MuonTracks                                        HLTMuonTrackSelector
   117 hltIterL3MuonCandidates                                    L3MuonCandidateProducerFromMuons
   118 hltIter0PFLowPixelSeedsFromPixelTracks                     SeedGeneratorFromProtoTracksEDProducer
   119 hltIter0PFlowCkfTrackCandidatesMkFitSiPixelHits            MkFitSiPixelHitConverter
   120 hltIter0PFlowCkfTrackCandidatesMkFitSiStripHits            MkFitSiStripHitConverter
   121 hltIter0PFlowCkfTrackCandidatesMkFitEventOfHits            MkFitEventOfHitsProducer
   122 hltIter0PFlowCkfTrackCandidatesMkFitSeeds                  MkFitSeedConverter
   123 hltIter0PFlowCkfTrackCandidatesMkFit                       MkFitProducer
   124 hltIter0PFlowCkfTrackCandidates                            MkFitOutputConverter
   125 hltIter0PFlowCtfWithMaterialTracks                         TrackProducer
   126 hltIter0PFlowTrackCutClassifier                            TrackCutClassifier
   127 hltIter0PFlowTrackSelectionHighPurity                      TrackCollectionFilterCloner
   128 hltDoubletRecoveryClustersRefRemoval                       TrackClusterRemover
   129 hltDoubletRecoveryMaskedMeasurementTrackerEvent            MaskedMeasurementTrackerEventProducer
   130 hltDoubletRecoveryPixelLayersAndRegions                    PixelInactiveAreaTrackingRegionsSeedingLayersProducer
   131 hltDoubletRecoveryPFlowPixelClusterCheck                   ClusterCheckerEDProducer
   132 hltDoubletRecoveryPFlowPixelHitDoublets                    HitPairEDProducer
   133 hltDoubletRecoveryPFlowPixelSeeds                          SeedCreatorFromRegionConsecutiveHitsEDProducer
   134 hltDoubletRecoveryPFlowCkfTrackCandidates                  CkfTrackCandidateMaker
   135 hltDoubletRecoveryPFlowCtfWithMaterialTracks               TrackProducer
   136 hltDoubletRecoveryPFlowTrackCutClassifier                  TrackCutClassifier
   137 hltDoubletRecoveryPFlowTrackSelectionHighPurity            TrackCollectionFilterCloner
   138 hltMergedTracks                                            TrackListMerger
   139 hltPFMuonMerging                                           TrackListMerger
   140 hltMuonLinks                                               MuonLinksProducerForHLT
   141 hltMuons                                                   MuonIdProducer
   142 hltEcalPreshowerDigis                                      ESRawToDigi
   143 hltEcalPreshowerRecHit                                     ESRecHitProducer
   144 hltParticleFlowRecHitECALUnseeded                          PFRecHitProducer
   145 hltParticleFlowRecHitHF                                    PFRecHitProducer
   146 hltParticleFlowRecHitPSUnseeded                            PFRecHitProducer
   147 hltParticleFlowClusterECALUncorrectedUnseeded              PFClusterProducer
   148 hltParticleFlowClusterPSUnseeded                           PFClusterProducer
   149 hltParticleFlowClusterECALUnseeded                         CorrectedECALPFClusterProducer
   150 hltParticleFlowRecHitHBHESoA                               PFRecHitSoAProducerHCAL@alpaka
   151 hltParticleFlowRecHitHBHE                                  LegacyPFRecHitProducer
   152 hltParticleFlowClusterHBHESoA                              PFClusterSoAProducer@alpaka
   153 hltParticleFlowClusterHBHE                                 LegacyPFClusterProducer
   154 hltParticleFlowClusterHCAL                                 PFMultiDepthClusterProducer
   155 hltParticleFlowClusterHF                                   PFClusterProducer
   156 hltLightPFTracks                                           LightPFTrackProducer
   157 hltParticleFlowBlock                                       PFBlockProducer
   158 hltParticleFlow                                            PFProducer
   159 hltAK4PFJets                                               FastjetJetProducer
   160 hltAK4PFJetsLooseID                                        HLTPFJetIDProducer
   161 hltAK4PFJetsTightID                                        HLTPFJetIDProducer
   162 hltFixedGridRhoFastjetAll                                  FixedGridRhoProducerFastjet
   163 hltAK4PFFastJetCorrector                                   L1FastjetCorrectorProducer
   164 hltAK4PFRelativeCorrector                                  LXXXCorrectorProducer
   165 hltAK4PFAbsoluteCorrector                                  LXXXCorrectorProducer
   166 hltAK4PFResidualCorrector                                  LXXXCorrectorProducer
   167 hltAK4PFCorrector                                          ChainedJetCorrectorProducer
   168 hltAK4PFJetsCorrected                                      CorrectedPFJetProducer
   169 hltAK4PFJetsLooseIDCorrected                               CorrectedPFJetProducer
   170 hltAK4PFJetsTightIDCorrected                               CorrectedPFJetProducer
   171 hltPFJetsCorrectedMatchedToCaloJets80                      HLTPFJetsMatchedToFilteredCaloJetsProducer   maxDeltaR 0.5
   172 hltSinglePFJet110                                          HLT1PFJet                                    MinPt 110 MinEta -1 MaxEta 5 input hltPFJetsCorrectedMatchedToCaloJets80
   173 hltBoolEnd                                                 HLTBool

```

The parameters of the modules the chapter describes are in `out/menu_13_HLT.txt`, section 3 (every parameter, step by
step), for example the tower thresholds:

```
-- hltTowerMakerForAll (CaloTowersCreator)
   AllowMissingInputs                                   False
   EBGrid                                               []
   EBSumThreshold                                       0.2
   EBThreshold                                          0.07
   EBWeight                                             1.0
   EBWeights                                            []
   EEGrid                                               []
   EESumThreshold                                       0.45
   EEThreshold                                          0.3
   EEWeight                                             1.0
   EEWeights                                            []
   EcalRecHitSeveritiesToBeExcluded                     ['kTime', 'kWeird', 'kBad']
   EcalRecHitThresh                                     False
   EcalSeveritiesToBeUsedInBadTowers                    []
   EcutTower                                            -1000.0
   HBGrid                                               []
   HBThreshold                                          0.3
   HBThreshold1                                         0.4
   HBThreshold2                                         0.3
   HBWeight                                             1.0
   HBWeights                                            []
   HEDGrid                                              []
   HEDThreshold                                         0.2
   HEDThreshold1                                        0.1
   HEDWeight                                            1.0
   HEDWeights                                           []
   HESGrid                                              []
   HESThreshold                                         0.2
   HESThreshold1                                        0.1
   HESWeight                                            1.0
   HESWeights                                           []
   HF1Grid                                              []
   HF1Threshold                                         0.5
   HF1Weight                                            1.0
   HF1Weights                                           []
   HF2Grid                                              []
   HF2Threshold                                         0.85
   HF2Weight                                            1.0
   HF2Weights                                           []
```

### The content of the output ROOT file

`edmDumpEventContent file:$J/test/13_HLT/out/step5_HLT_13_HLT.root | grep '"HLT"'` lists 3121 products of the process
`HLT` (20.8 MB in all; the input is 12.3 MB; in 2024, with the 37 paths removed, 2977 products and 21.2 MB). The ones
the chapter uses, by step:

| step | product | what it is |
|---|---|---|
| 1 | `FEDRawDataCollection rawDataCollector` | the raw event: one block of bytes per FED (made by DIGI2RAW) |
| 1 | `BXVector<GlobalAlgBlk>`, `BXVector<l1t::Jet / EGamma / Tau / Muon / EtSum> hltGtStage2Digis` (instances `Jet`, `EGamma`, ...) | the Level-1 record unpacked from FED 1404 (`L1TRawToDigi`, `stage2::GTSetup`); the instances `Jet2` ... `Jet6` are further copies unpacked from the same record |
| 1 | `GlobalObjectMapRecord hltGtStage2ObjectMap` | the global trigger re-emulated on the unpacked objects: which objects satisfied each algorithm (read by `HLTL1TSeed`) |
| 2 | `EcalRecHitsSorted hltEcalRecHit:EcalRecHitsEB/EE`, `HBHERecHitsSorted hltHbhereco`, `HFRecHitsSorted hltHfreco` | the calorimeter rec-hits |
| 2 | `CaloTowersSorted hltTowerMakerForAll` | the calorimeter towers |
| 2 | `vector<reco::CaloJet> hltAK4CaloJets`, `hltAK4CaloJetsIDPassed`, `hltAK4CaloJetsCorrected`, `hltAK4CaloJetsCorrectedIDPassed`; `double hltFixedGridRhoFastjetAllCalo` | the AK4 calo jets, raw and corrected; rho of the towers |
| 3 | `SiPixelClusteredmNewDetSetVector hltSiPixelClusters`, `SiStripClusteredmNewDetSetVector hltSiStripRawToClustersFacility` | the clusters |
| 3 | `vector<reco::Track> hltPixelTracks`, `hltIter0PFlowTrackSelectionHighPurity`, `hltDoubletRecoveryPFlowTrackSelectionHighPurity`, `hltMergedTracks` | pixel tracks, the two iterations, the merged tracks |
| 3 | `vector<reco::Vertex> hltPixelVertices`, `hltTrimmedPixelVertices`, `hltVerticesPF` | pixel vertices, the ones kept for the tracking, the PF vertices |
| 4 | `vector<reco::PFCandidate> hltParticleFlow`; `vector<reco::PFCluster> hltParticleFlowCluster*` | the PF candidates and clusters |
| 4 | `vector<reco::PFJet> hltAK4PFJets`, `hltAK4PFJetsCorrected` (and the AK8 ones); `double hltFixedGridRhoFastjetAll` | the AK4 PF jets, raw and corrected; rho of the PF candidates |
| 4 | `vector<reco::Muon> hltMuons`, `vector<reco::RecoChargedCandidate> hltIterL3MuonCandidates`, `vector<reco::Track> hltL2Muons` | the HLT muons (the pileup kaon decay) |
| 5 | `edm::TriggerResults TriggerResults` | one bit per path (accept, and the index of the last module run) |
| 5 | `trigger::TriggerEvent hltTriggerSummaryAOD`, `trigger::TriggerEventWithRefs hltTriggerSummaryRAW`, `trigger::TriggerFilterObjectWithRefs <filter>` | the objects of every filter with `saveTags` (AOD: copies; RAW and the filter objects: references) |

## 3. The analyzer configs

One job runs four analyzers, one after the other (`test/common/hltAnalyzer_cfg.py`; the plugins are
`plugins/HLTPathAnalyzer.cc`, `HLTCaloAnalyzer.cc`, `HLTTrackAnalyzer.cc`, `HLTPFAnalyzer.cc`, with the page, the
tower grid of chapter 12 and the jet corrections level by level in `plugins/HLTDisplay.h` on top of `L1TDisplay.h`):

```bash
cd $J/test/13_HLT/out
cmsRun $J/test/common/hltAnalyzer_cfg.py inputFiles=file:step5_HLT_13_HLT.root maxEvents=1 eventsToProcess=1:1:1 \
  globalTag=auto:phase1_2025_realistic era=Run3_2025 geometry=DB:Extended pdfPrefix=hlt_13_HLT savePNG=1 \
  sampleLabel="chapter 13: the High Level Trigger|bbbar, pTHat > 100 GeV, 13.6 TeV|with PU = 60 per bx, bx -1..1" \
  > hlt_13_HLT.log 2>&1                                        # about 20 s
```

Options: `processName=HLT`, `jetPtMin=30` (the ak4GenJets drawn and followed), `dzMax=0.1` (a track belongs to a vertex
within this distance in z, cm), `zMax=256` (the top of the colour scale of the tower maps). The job needs the global tag
(the jet energy corrections `AK4CaloHLT`, `AK4PFHLT`, evaluated level by level with `FactorizedJetCorrector`) and the
geometry (the positions of the calorimeter cells for the rec-hit E_T). `HLTPathAnalyzer` reads the configuration of the
HLT process from the file (`HLTConfigProvider`): the paths, their modules, their seeds, the datasets and streams.

| analyzer | steps | PDF and PNG pages (`hlt_13_HLT_...`) | log lines |
|---|---|---|---|
| `HLTPathAnalyzer` | 1, 5 | `_seed.pdf`; `_seed_seed` (the L1 objects unpacked by the HLT, the objects of `hltL1sSingleJet90`, the raw-data sizes, the fate of the paths), `_seed_ladders` (the thresholds of the jet paths and the stage that stopped each), `_seed_objects` (the objects of the accepted paths from `hltTriggerSummaryAOD`: the display of B4) | `menu:`, `menu modules:`, `rawdata`, `L1 unpacked:`, `L1Jet:`, `paths HLT_+DST_:`, `path:` (every path: accept, stage, seed, the last module and its type), `dataset:`, `stream:`, `summary:`, `filter:` (every filter with its objects), `cut:`, `leading jets:`, `ladder`, `seedObjects:`, `accepted:` |
| `HLTCaloAnalyzer` | 2 | `_calo.pdf`; `_calo_towers` (the towers on the trigger-tower grid with the corrected calo jets), `_calo_jet1`, `_calo_jet2` (17 x 17 towers around the two signal jets, E_T in each, the 9 x 9 window of the L1 jet nearest to the calo jet, the corrections level by level) | `rechits <region>:`, `towers <region>:`, `rho calo`, `jec calo:` (the formulas of the payload), `GenJet:`, `calo jets:`, `CaloJet N:` (raw, factors, recomputed and stored corrected pT, gen match), `zoom gen jet` |
| `HLTTrackAnalyzer` | 3 | `_track.pdf`; `_track_etaphi` (the merged tracks, those of the signal vertex in black), `_track_vertices` (pixel, kept and PF vertices along z with the simulated collisions) | `clusters:`, `tracks <iteration>:`, `truth:` (the simulated vertices, `addPileupInfo`, `genParticles:xyz0`), `vertices <collection>:`, `vertex <collection> N:` (with the nearest simulated collision), `vertices trimmed`, `signal vertex rank:`, `jet tracks:` |
| `HLTPFAnalyzer` | 4 | `_pf.pdf`; `_pf_candidates` (the PF candidates by kind with the corrected PF jets), `_pf_jet1`, `_pf_jet2` (the constituents of the two signal PF jets and their corrections) | `pfcands <kind>:`, `pfcands region`, `rho`, `jec pf:` (formulas, the eta x phi bins), `pf jets:`, `PFJet N:` (composition, factors, `jecBin`, gen match), `zoom gen jet` |

Each analyzer opens and closes its PDF inside its own `analyze()` (ROOT has one global PDF), so the four can share a job.
The `CaloJet` and `PFJet` lines check the menu: the corrections recomputed from the payload give the pT of
`hltAK4CaloJetsCorrected` and `hltAK4PFJetsCorrected` exactly (`recomputedPt`). The PF payload is binned in eta and phi
(`JetPhi`): the corrector must be given the phi of the jet, without it no bin is found and every factor is 1.

The pages (11 PNG):

| page | figure of the chapter |
|---|---|
| `hlt_13_HLT_seed_seed` | 13.2: step 1, the Level-1 objects the HLT unpacks |
| `hlt_13_HLT_calo_towers` | 13.3: step 2, the towers and the calo jets |
| `hlt_13_HLT_calo_jet1`, `_calo_jet2` | 13.4: step 2, the towers around the two signal jets |
| `hlt_13_HLT_track_etaphi`, `_track_vertices` | 13.5: step 3 |
| `hlt_13_HLT_pf_candidates` | 13.6: step 4, the PF candidates |
| `hlt_13_HLT_pf_jet1`, `_pf_jet2` | 13.7: step 4, the two signal jets particle by particle |
| `hlt_13_HLT_seed_ladders` | 13.8: step 5, the ladders of the jet paths |
| `hlt_13_HLT_seed_objects` | 13.11: the display of B4 |

Two scripts then read the configuration and the log:

```bash
python3 $J/test/common/hltMenu.py $J/test/13_HLT/step5_HLT_13_HLT_cfg.py > menu_13_HLT.txt   # the menu as configured
python3 $J/test/common/hltLedger.py . 13_HLT > ledger_13_HLT.txt                             # the ledger, step by step
```

## 4. The tables

Every number of the chapter, by step, copied from `test/13_HLT/out/`.

### The event and the menu (`ledger_13_HLT.txt`, head)

```
== hltLedger 13_HLT
event: run:lumi:event 1:1:1
menu: table=/frozen/2025/2e34/v1.0/HLT/V2 process=HLT globalTag=150X_mcRun3_2025_realistic_v1 paths=899 datasets=99 streams=68 prescaleColumns=0 | AlCa=11 DQM=3 DST=21 Dataset=99 HLT=733 HLTriggerFinalPath=1 HLTriggerFirstPath=1 MC=26 ScoutingPFOutput=1 Status=2 digi2raw=1
menu modules: distinct=4783 =7 EDAnalyzer=1 EDFilter=3474 EDProducer=1301
```

### Step 1 (`ledger_13_HLT.txt`, section 1)

```
-- 1. Step 1: the raw event, the Level-1 record, the seeds and the prescales
raw event: 678 FEDs, 1861.5 kB
   strip               381 FEDs     997.5 kB  53.6 %
   pixel               108 FEDs     306.7 kB  16.5 %
   HCAL                 44 FEDs     240.6 kB  12.9 %
   preshower            40 FEDs     137.4 kB   7.4 %
   ECAL                 54 FEDs     103.9 kB   5.6 %
   CSC                  36 FEDs      46.9 kB   2.5 %
   Level-1 trigger       7 FEDs      27.1 kB   1.5 %
   GEM                   2 FEDs       0.9 kB   0.0 %
   DT                    3 FEDs       0.3 kB   0.0 %
   RPC                   3 FEDs       0.2 kB   0.0 %
L1 unpacked: finalOR=1 algorithmsFired=63 jets=9 egammas=12 taus=12 muons=1
paths HLT_ and DST_: 754; stopped by the L1 seed 652; by the prescale 0; start their reconstruction 102 (of which 62 stopped by a filter, 40 accept)
seedObjects: label=hltL1sSingleJet90 expression="L1_SingleJet90" n=2 | L1Jet(136.0,-3.74,1.00) L1Jet(108.5,-1.52,-1.98)
seedObjects: label=hltL1sZeroBias expression="L1_ZeroBias" n=0 |
seedObjects: label=hltL1sSingleJet120 expression="L1_SingleJet120" n=1 | L1Jet(136.0,-3.74,1.00)
seedObjects: label=hltL1sSingleJet90Fwd expression="L1_SingleJet90_FWD2p5" n=1 | L1Jet(136.0,-3.74,1.00)
```

The unpacked Level-1 jets and the seeds of the jet paths (`hlt_13_HLT.log`):

```
  L1Jet: et=136 eta=-3.741 phi=1.0005
  L1Jet: et=108.5 eta=-1.5225 phi=-1.97669
  L1Jet: et=23 eta=0.9135 phi=1.6965
  L1Jet: et=20 eta=-0.3045 phi=0.3045
  L1Jet: et=17.5 eta=1.5225 phi=0.9135
  L1Jet: et=15 eta=0.9135 phi=-2.49869
  L1Jet: et=14 eta=-2.262 phi=-1.80269
  L1Jet: et=13 eta=1.0875 phi=2.7405
  L1Jet: et=9.5 eta=2.3925 phi=-1.54169
  seedObjects: label=hltL1sSingleJet90 expression="L1_SingleJet90" n=2 | L1Jet(136.0,-3.74,1.00) L1Jet(108.5,-1.52,-1.98)
  seedObjects: label=hltL1sZeroBias expression="L1_ZeroBias" n=0 |
  seedObjects: label=hltL1sSingleJet120 expression="L1_SingleJet120" n=1 | L1Jet(136.0,-3.74,1.00)
  seedObjects: label=hltL1sSingleJet90Fwd expression="L1_SingleJet90_FWD2p5" n=1 | L1Jet(136.0,-3.74,1.00)
```

### Step 2 (`ledger_13_HLT.txt`, section 2)

```
-- 2. Step 2: calorimeter rec-hits, towers, calo jets
rec-hits          n      sum E     sum ET
EB              956      208.3      142.6
EE             2515     2135.3      378.7
HB             2162      671.3      493.4
HE             3832     3028.3      578.0
HF             1728    50809.4     1606.2
towers            n     sum ET         EM        HAD
barrel          492      342.8      136.1      206.8
endcap          771      882.9      360.9      522.0
forward         788     1617.2      593.3     1023.9
all            2051     2842.9     1090.3     1752.6
rho calo (hltFixedGridRhoFastjetAllCalo): 36.349 GeV per unit area
calo jets: raw=190 corrected=190 corrected>10=35 >30=2 >80=2
jec calo: level=L1FastJet records=82 formula="max(0.0001,1-(z/y)*([0]+([1]*(x))*(1+[2]*log(y))))" binVar=JetEta, parVar=Rho,JetPt,JetA,
jec calo: level=L2Relative records=82 formula="max(0.0001,[0]+[1]/(pow(log10(x),2)+[2])+[3]*exp(-[4]*(log10(x)-[5])*(log10(x)-[5])))" binVar=JetEta, parVar=JetPt,
jec calo: level=L3Absolute records=1 formula="1" binVar=JetEta, parVar=JetPt,
jec calo: level=L2L3Residual records=1 formula="1" binVar=JetEta, parVar=JetPt,
CaloJet 0: rawPt=163.5 eta=-3.72 phi=0.96 area=0.503 towers=17 n90=9 emf=0.27 | L1FastJet=0.7335 L2Relative=0.7243 L3Absolute=0.7243 L2L3Residual=0.7243 jecBin=eta[-3.839,-3.664] | recomputedPt=118.5 correctedPt=118.5 | gen=115.9 dR=0.05
CaloJet 1: rawPt=70.9 eta=-1.58 phi=-2.08 area=0.503 towers=33 n90=19 emf=0.56 | L1FastJet=0.8091 L2Relative=1.3162 L3Absolute=1.3162 L2L3Residual=1.3162 jecBin=eta[-1.653,-1.566] | recomputedPt=93.3 correctedPt=93.3 | gen=117.8 dR=0.01
CaloJet 5: rawPt=36.1 eta=4.57 phi=-1.23 area=0.503 towers=14 n90=12 emf=0.36 | L1FastJet=0.3374 L2Relative=0.4622 L3Absolute=0.4622 L2L3Residual=0.4622 jecBin=eta[4.538,4.716] | recomputedPt=16.7 correctedPt=16.7 | gen=- dR=-
CaloJet 7: rawPt=35.7 eta=2.77 phi=0.51 area=0.503 towers=17 n90=12 emf=0.53 | L1FastJet=0.2248 L2Relative=0.5222 L3Absolute=0.5222 L2L3Residual=0.5222 jecBin=eta[2.65,2.853] | recomputedPt=18.6 correctedPt=18.6 | gen=- dR=-
CaloJet 8: rawPt=35.1 eta=-2.72 phi=-0.71 area=0.503 towers=16 n90=12 emf=0.43 | L1FastJet=0.2206 L2Relative=0.5041 L3Absolute=0.5041 L2L3Residual=0.5041 jecBin=eta[-2.853,-2.65] | recomputedPt=17.7 correctedPt=17.7 | gen=- dR=-
CaloJet 9: rawPt=34.8 eta=-2.79 phi=3.12 area=0.503 towers=18 n90=13 emf=0.63 | L1FastJet=0.2154 L2Relative=0.4972 L3Absolute=0.4972 L2L3Residual=0.4972 jecBin=eta[-2.853,-2.65] | recomputedPt=17.3 correctedPt=17.3 | gen=- dR=-
CaloJet 18: rawPt=31.1 eta=-2.82 phi=0.89 area=0.457 towers=16 n90=13 emf=0.48 | L1FastJet=0.2270 L2Relative=0.5335 L3Absolute=0.5335 L2L3Residual=0.5335 jecBin=eta[-2.853,-2.65] | recomputedPt=16.6 correctedPt=16.6 | gen=- dR=-
CaloJet 21: rawPt=30.8 eta=2.38 phi=-1.73 area=0.501 towers=17 n90=13 emf=0.59 | L1FastJet=0.2809 L2Relative=0.5606 L3Absolute=0.5606 L2L3Residual=0.5606 jecBin=eta[2.322,2.5] | recomputedPt=17.3 correctedPt=17.3 | gen=- dR=-
CaloJet 32: rawPt=26.1 eta=2.15 phi=0.89 area=0.484 towers=18 n90=13 emf=0.53 | L1FastJet=0.3073 L2Relative=0.6969 L3Absolute=0.6969 L2L3Residual=0.6969 jecBin=eta[2.043,2.172] | recomputedPt=18.2 correctedPt=18.2 | gen=- dR=-
CaloJet 41: rawPt=22.7 eta=-2.13 phi=-1.76 area=0.414 towers=14 n90=10 emf=0.29 | L1FastJet=0.3468 L2Relative=0.8084 L3Absolute=0.8084 L2L3Residual=0.8084 jecBin=eta[-2.172,-2.043] | recomputedPt=18.4 correctedPt=18.4 | gen=- dR=-
CaloJet 52: rawPt=19.0 eta=-0.35 phi=0.36 area=0.503 towers=19 n90=15 emf=0.67 | L1FastJet=0.5691 L2Relative=1.1964 L3Absolute=1.1964 L2L3Residual=1.1964 jecBin=eta[-0.435,-0.348] | recomputedPt=22.7 correctedPt=22.7 | gen=- dR=-
CaloJet 54: rawPt=18.9 eta=-0.83 phi=-2.78 area=0.503 towers=23 n90=18 emf=0.56 | L1FastJet=0.5094 L2Relative=1.1070 L3Absolute=1.1070 L2L3Residual=1.1070 jecBin=eta[-0.879,-0.783] | recomputedPt=20.9 correctedPt=20.9 | gen=- dR=-
zoom gen jet 1: pt=117.8 centre ieta=-19 iphi=49 towersEt(dR<0.4 of the jet axis)=72.7 towersEt(9x9)=77.7 L1 jet 108.5 ieta=-18 iphi=50
zoom gen jet 2: pt=115.9 centre ieta=-34 iphi=12 towersEt(dR<0.4 of the jet axis)=168.9 towersEt(9x9)=215.9 L1 jet 136.0 ieta=-34 iphi=12
```

The arithmetic of the chapter: b jet 70.9 x 0.8091 = 57.4 (13.5 GeV subtracted), x 1.3162 / 0.8091 = 1.627 -> 93.3;
bbar jet 163.5 x 0.7335 = 119.9 (43.6 GeV subtracted), x 0.7243 / 0.7335 = 0.987 -> 118.5. The section lists the calo
jets above 15 GeV after the corrections; the leading jet of the pileup before them is `CaloJet 2` of `hlt_13_HLT.log`,
not listed here:

```
  CaloJet 2: rawPt=39.5 eta=3.66 phi=-0.36 area=0.503 towers=16 n90=13 emf=0.35 | L1FastJet=0.2602 L2Relative=0.3169 L3Absolute=0.3169 L2L3Residual=0.3169 jecBin=eta[3.489,3.664] | recomputedPt=12.5 correctedPt=12.5 | gen=- dR=-
```

39.5 x 0.2602 = 10.3, x 0.3169 / 0.2602 = 1.218 -> 12.5. 23 raw calo jets above 30 GeV (`CaloJet` lines of
`hlt_13_HLT.log` with `rawPt` > 30; the log has 143 `CaloJet` lines), 2 corrected. rho x area: 36.3 x 0.503 = 18.3 GeV.
The 9 x 9 windows of the L1 jets of chapter 12: 74.5 (b) and 189.0 GeV (bbar) (`ledger_12_L1T.txt`, section 3); the HLT
towers in the same windows (`plugins/HLTCaloAnalyzer.cc` takes the L1 jet of `hltGtStage2Digis` nearest to the calo
jet): 77.7 and 215.9 GeV (`zoom gen jet` lines). For the bbar jet the calo jet and the L1 jet are in the same tower
(ieta -34, iphi 12); for the b jet the calo jet is at ieta -19, iphi 49, one tower away in both from the seed of the
L1 jet (ieta -18, iphi 50); centred on the calo jet, as the analyzer did before 13 November 2026, the window held 79.5 GeV.

### Step 3 (`ledger_13_HLT.txt`, section 3)

```
-- 3. Step 3: tracking and vertices
clusters: pixel=20925 (modules 1766) strip=115249 (modules 13859)
tracks pixel: n=2981 pt>1=1771 pt>10=156 sumPt=18982.2
tracks iter0: n=108 pt>1=42 pt>10=3 sumPt=165.4
tracks doubletRecovery: n=8 pt>1=7 pt>10=0 sumPt=14.1
tracks merged: n=116 pt>1=49 pt>10=3 sumPt=179.5
tracks merged: highPurity=116 meanPixelHits=3.97 meanStripHits=9.34
truth: signal vertex x=0.0277 y=-0.0169 z=3.1119 | in-time pileup collisions=68 (mean 68)
truth: pileup collisions within 1 mm of the signal in z: 0 | z range -9.52 .. 6.53
vertices pixel: n=31
vertices trimmed (seeds of iteration 0): n=1 | z=3.107(signal)
vertices PF: n=1
vertex PF 0: z=3.1112 ndof=95.4 tracks=107 near=59 sumPt=115.6 sumPt2=627.0 sim=signal dzSim=0.0007
signal vertex rank: pixel=0 PF=0 (0 = the first, the primary vertex)
jet tracks: gen jet 1 pt=117.8 eta=-1.59 chargedGenPt=32.6 | tracks dR<0.4: n=7 sumPt=50.1 | from the signal vertex (|dz|<0.1 cm): n=6 sumPt=49.2
jet tracks: gen jet 2 pt=115.9 eta=-3.75 chargedGenPt=44.0 | tracks dR<0.4: n=0 sumPt=0.0 | from the signal vertex (|dz|<0.1 cm): n=0 sumPt=0.0
tracks from the signal vertex (|dz| < 0.1 cm of z=3.111): n=61 sumPt=121.8
vertex pixel 0: z=3.1066 ndof=39.0 tracks=40 near=61 sumPt=121.8 sumPt2=653.8 sim=signal dzSim=0.0053
vertex pixel 1: z=-0.3305 ndof=21.0 tracks=22 near=0 sumPt=0.0 sumPt2=0.0 sim=pu9 dzSim=0.0079
vertex pixel 2: z=-5.4560 ndof=30.0 tracks=31 near=0 sumPt=0.0 sumPt2=0.0 sim=pu30 dzSim=0.0017
vertex pixel 3: z=2.0815 ndof=21.0 tracks=22 near=0 sumPt=0.0 sumPt2=0.0 sim=pu42 dzSim=0.0012
vertex pixel 4: z=0.3620 ndof=26.0 tracks=27 near=0 sumPt=0.0 sumPt2=0.0 sim=pu35 dzSim=0.0016
vertex pixel 5: z=-2.0472 ndof=12.0 tracks=13 near=0 sumPt=0.0 sumPt2=0.0 sim=pu51 dzSim=0.0003
vertex pixel 6: z=-2.8665 ndof=17.0 tracks=18 near=0 sumPt=0.0 sumPt2=0.0 sim=pu29 dzSim=0.0009
vertex pixel 7: z=2.4981 ndof=24.0 tracks=25 near=0 sumPt=0.0 sumPt2=0.0 sim=pu22 dzSim=0.0034
```

The pixel vertices in full (`hlt_13_HLT.log`):

```
  vertex pixel 0: z=3.1066 ndof=39.0 tracks=40 near=61 sumPt=121.8 sumPt2=653.8 sim=signal dzSim=0.0053
  vertex pixel 1: z=-0.3305 ndof=21.0 tracks=22 near=0 sumPt=0.0 sumPt2=0.0 sim=pu9 dzSim=0.0079
  vertex pixel 2: z=-5.4560 ndof=30.0 tracks=31 near=0 sumPt=0.0 sumPt2=0.0 sim=pu30 dzSim=0.0017
  vertex pixel 3: z=2.0815 ndof=21.0 tracks=22 near=0 sumPt=0.0 sumPt2=0.0 sim=pu42 dzSim=0.0012
  vertex pixel 4: z=0.3620 ndof=26.0 tracks=27 near=0 sumPt=0.0 sumPt2=0.0 sim=pu35 dzSim=0.0016
  vertex pixel 5: z=-2.0472 ndof=12.0 tracks=13 near=0 sumPt=0.0 sumPt2=0.0 sim=pu51 dzSim=0.0003
  vertex pixel 6: z=-2.8665 ndof=17.0 tracks=18 near=0 sumPt=0.0 sumPt2=0.0 sim=pu29 dzSim=0.0009
  vertex pixel 7: z=2.4981 ndof=24.0 tracks=25 near=0 sumPt=0.0 sumPt2=0.0 sim=pu22 dzSim=0.0034
  vertex pixel 8: z=2.8856 ndof=19.0 tracks=20 near=27 sumPt=30.3 sumPt2=41.7 sim=pu34 dzSim=0.0064
  vertex pixel 9: z=1.4886 ndof=27.0 tracks=28 near=0 sumPt=0.0 sumPt2=0.0 sim=pu10 dzSim=0.0039
  vertex pixel 10: z=-0.0608 ndof=16.0 tracks=17 near=0 sumPt=0.0 sumPt2=0.0 sim=pu59 dzSim=0.0043
  vertex pixel 11: z=3.4951 ndof=11.0 tracks=12 near=2 sumPt=2.6 sumPt2=3.4 sim=pu23 dzSim=0.0021
  vertex pixel 12: z=1.2480 ndof=16.0 tracks=17 near=0 sumPt=0.0 sumPt2=0.0 sim=pu31 dzSim=0.0024
  vertex pixel 13: z=-5.3849 ndof=1.0 tracks=2 near=0 sumPt=0.0 sumPt2=0.0 sim=pu7 dzSim=0.0389
  vertex pixel 14: z=-2.8111 ndof=14.0 tracks=15 near=0 sumPt=0.0 sumPt2=0.0 sim=pu50 dzSim=0.0063
```

### Step 4 (`ledger_13_HLT.txt`, section 4)

```
-- 4. Step 4: particle flow and PF jets
pfcands all: n=1366 sumPt=2655.8
pfcands h: n=117 sumPt=181.9
pfcands e: n=0 sumPt=0.0
pfcands mu: n=1 sumPt=3.0
pfcands gamma: n=238 sumPt=196.3
pfcands h0: n=209 sumPt=702.5
pfcands h_HF: n=700 sumPt=1486.0
pfcands egamma_HF: n=101 sumPt=86.0
pfcands region tracker: n=515 sumPt=833.1 charged n=110 sumPt=176.7
pfcands region outside: n=68 sumPt=259.1 charged n=8 sumPt=8.3
pfcands region HF: n=783 sumPt=1563.6 charged n=0 sumPt=0.0
rho (hltFixedGridRhoFastjetAll): 36.743 GeV per unit area
pf jets: raw=192 raw>10=103 raw>30=13 corrected=192 corrected>10=30 >30=2 >30 with a gen jet=2
jec pf: level=L1FastJet records=118 formula="max(0.0001,1-(z/y)*([0]+([1]*(x))*(1+[2]*log(y))))" binVar=JetEta,JetPhi, parVar=Rho,JetPt,JetA, JetPhiBins(eta -5.191..-4.889)=-3.15..3.15, etaBinsWithSeveralPhiBins=36 | split bins: [-1.566,-1.479]x[-3.15,-1.2] [-1.566,-1.479]x[-1.2,-0.8] [-1.566,-1.479]x[-0.8,3.15] [-1.479,-1.392]x[-3.15,-1.2] [-1.479,-1.392]x[-1.2,-0.8] [-1.479,-1.392]x[-0.8,3.15] [-1.392,-1.305]x[-3.1
jec pf: level=L2Relative records=118 formula="max(0.0001,[0]+([1]/(pow(log10(x),2)+[2]))+([3]*exp(-([4]*((log10(x)-[5])*(log10(x)-[5])))))+([6]*exp(-([7]*((log10(x)-[8])*(log10(x)-[8]))))))" binVar=JetEta,JetPhi, parVar=JetPt, JetPhiBins(eta -5.191..-4.889)=-3.15..3.15, etaBinsWithSeveralPhiBins=36 | split bins: [-1.566,-1.479]x[-3.15,-1.2] [-1.566,-1.479]x[-1.2,-0.8] [-1.566,-1.479]x[-0.8,3.15] [-1.479,-1.392]x[-3.1
jec pf: level=L3Absolute records=1 formula="1" binVar=JetEta, parVar=JetPt,
jec pf: level=L2L3Residual records=1 formula="1" binVar=JetEta, parVar=JetPt,
PFJet 0: pt=126.7 rawPt=156.8 eta=-3.77 phi=0.96 area=0.503 n=17 chf=0.00 nhf=1.00 phf=0.00 elf=0.00 muf=0.00 hfhf=1.00 hfemf=0.00 | L1FastJet=0.8241 L2Relative=0.8081 L3Absolute=0.8081 L2L3Residual=0.8081 jecBin=eta[-3.839,-3.664]phi[-3.15,3.15] | recomputedPt=126.7 | gen=115.9 dR=0.05
PFJet 1: pt=86.7 rawPt=83.7 eta=-1.58 phi=-2.08 area=0.503 n=29 chf=0.59 nhf=0.07 phf=0.34 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.8685 L2Relative=1.0353 L3Absolute=1.0353 L2L3Residual=1.0353 jecBin=eta[-1.653,-1.566]phi[-3.15,3.15] | recomputedPt=86.7 | gen=117.8 dR=0.01
PFJet 2: pt=21.2 rawPt=24.2 eta=1.11 phi=2.43 area=0.503 n=12 chf=0.37 nhf=0.31 phf=0.32 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.6804 L2Relative=0.8758 L3Absolute=0.8758 L2L3Residual=0.8758 jecBin=eta[1.044,1.131]phi[-3.15,3.15] | recomputedPt=21.2 | gen=- dR=-
zoom gen jet 1: pt=117.8 eta=-1.59 | PF jet 86.7 (raw 83.7) dR=0.01 | constituents: h=7/50.1 e=0/0.0 mu=0/0.0 gamma=20/28.4 h0=2/5.8 h_HF=0/0.0 egamma_HF=0/0.0
zoom gen jet 2: pt=115.9 eta=-3.75 | PF jet 126.7 (raw 156.8) dR=0.05 | constituents: h=0/0.0 e=0/0.0 mu=0/0.0 gamma=0/0.0 h0=0/0.0 h_HF=17/158.1 egamma_HF=0/0.0
```

The arithmetic of the chapter: b jet 83.7 x 0.8685 = 72.7 (11.0 GeV subtracted), x 1.0353 / 0.8685 = 1.192 -> 86.7;
bbar jet 156.8 x 0.8241 = 129.2 (27.6 GeV subtracted), x 0.8081 / 0.8241 = 0.981 -> 126.7. 13 raw PF jets above 30 GeV
(the `pf jets:` line, `pf jets: raw=192 raw>10=103 raw>30=13 corrected=192 corrected>10=30 >30=2 >30 with a gen jet=2`; the `PFJet` lines list every jet above 10 GeV before or after
the corrections: 156.8, 83.7, 38.9, 37.7, 36.9, 34.7, 34.6, 32.6, 31.8, 31.5, 31.4, 31.2, 30.8 raw), 2 corrected. The PF jets above 15 GeV after the
corrections (`hlt_13_HLT.log`, `PFJet` lines; the table of the PF jets in step 4 of the chapter):

```
  PFJet 0: pt=126.7 rawPt=156.8 eta=-3.77 phi=0.96 area=0.503 n=17 chf=0.00 nhf=1.00 phf=0.00 elf=0.00 muf=0.00 hfhf=1.00 hfemf=0.00 | L1FastJet=0.8241 L2Relative=0.8081 L3Absolute=0.8081 L2L3Residual=0.8081 jecBin=eta[-3.839,-3.664]phi[-3.15,3.15] | recomputedPt=126.7 | gen=115.9 dR=0.05
  PFJet 1: pt=86.7 rawPt=83.7 eta=-1.58 phi=-2.08 area=0.503 n=29 chf=0.59 nhf=0.07 phf=0.34 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.8685 L2Relative=1.0353 L3Absolute=1.0353 L2L3Residual=1.0353 jecBin=eta[-1.653,-1.566]phi[-3.15,3.15] | recomputedPt=86.7 | gen=117.8 dR=0.01
  PFJet 2: pt=21.2 rawPt=24.2 eta=1.11 phi=2.43 area=0.503 n=12 chf=0.37 nhf=0.31 phf=0.32 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.6804 L2Relative=0.8758 L3Absolute=0.8758 L2L3Residual=0.8758 jecBin=eta[1.044,1.131]phi[-3.15,3.15] | recomputedPt=21.2 | gen=- dR=-
  PFJet 3: pt=19.6 rawPt=37.7 eta=3.41 phi=1.94 area=0.503 n=15 chf=0.00 nhf=0.95 phf=0.00 elf=0.00 muf=0.00 hfhf=0.95 hfemf=0.05 | L1FastJet=0.4269 L2Relative=0.5206 L3Absolute=0.5206 L2L3Residual=0.5206 jecBin=eta[3.314,3.489]phi[-3.15,3.15] | recomputedPt=19.6 | gen=- dR=-
  PFJet 4: pt=18.3 rawPt=34.7 eta=4.61 phi=-1.23 area=0.503 n=14 chf=0.00 nhf=1.00 phf=0.00 elf=0.00 muf=0.00 hfhf=1.00 hfemf=0.00 | L1FastJet=0.4211 L2Relative=0.5263 L3Absolute=0.5263 L2L3Residual=0.5263 jecBin=eta[4.538,4.716]phi[-3.15,3.15] | recomputedPt=18.3 | gen=- dR=-
  PFJet 5: pt=18.0 rawPt=34.6 eta=-4.22 phi=1.25 area=0.397 n=12 chf=0.00 nhf=1.00 phf=0.00 elf=0.00 muf=0.00 hfhf=1.00 hfemf=0.00 | L1FastJet=0.4584 L2Relative=0.5222 L3Absolute=0.5222 L2L3Residual=0.5222 jecBin=eta[-4.363,-4.191]phi[-3.15,3.15] | recomputedPt=18.0 | gen=- dR=-
  PFJet 6: pt=16.7 rawPt=29.9 eta=4.78 phi=1.67 area=0.464 n=11 chf=0.00 nhf=1.00 phf=0.00 elf=0.00 muf=0.00 hfhf=1.00 hfemf=0.00 | L1FastJet=0.4454 L2Relative=0.5605 L3Absolute=0.5605 L2L3Residual=0.5605 jecBin=eta[4.716,4.889]phi[-3.15,3.15] | recomputedPt=16.7 | gen=- dR=-
  PFJet 7: pt=16.3 rawPt=19.7 eta=1.02 phi=1.69 area=0.493 n=12 chf=0.05 nhf=0.73 phf=0.22 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.6417 L2Relative=0.8266 L3Absolute=0.8266 L2L3Residual=0.8266 jecBin=eta[0.957,1.044]phi[-3.15,3.15] | recomputedPt=16.3 | gen=- dR=-
  PFJet 8: pt=16.1 rawPt=38.9 eta=3.69 phi=-0.36 area=0.503 n=16 chf=0.00 nhf=1.00 phf=0.00 elf=0.00 muf=0.00 hfhf=1.00 hfemf=0.00 | L1FastJet=0.3565 L2Relative=0.4142 L3Absolute=0.4142 L2L3Residual=0.4142 jecBin=eta[3.664,3.839]phi[-3.15,3.15] | recomputedPt=16.1 | gen=- dR=-
```

### Step 5 (`ledger_13_HLT.txt`, section 5)

```
-- 5. Step 5: the decision
HLT_ and DST_ paths that accept the event: 40
  satisfied by signal jets: 16
     HLT_DiPFJetAve40_v27
     HLT_DiPFJetAve60_v27
     HLT_DiPFJetAve80_v27
     HLT_AK8PFJet40_v29
     HLT_AK8PFJet60_v28
     HLT_AK8PFJet80_v29
     HLT_PFJet40_v34
     HLT_PFJet60_v34
     HLT_PFJet80_v34
     HLT_PFJet110_v13
     HLT_PFJetFwd40_v32
     HLT_PFJetFwd60_v32
     HLT_PFJetFwd80_v31
     HLT_AK8PFJetFwd40_v28
     HLT_AK8PFJetFwd60_v27
     HLT_AK8PFJetFwd80_v27
  satisfied by a pileup object: 2
     HLT_Mu3_PFJet40_v29: the muon of a pileup kaon decay (3.1 GeV at L2, 3.0 GeV at L3) and the b jet
     HLT_CaloMET90_NotCleaned_v15: a calorimeter MET of 138.5 GeV (the neutrino has 37.5)
  satisfied by the L1 seed alone: 3
     HLT_L1SingleMuCosmics_v9: the L1 seed alone (the pileup muon)
     HLT_L1ETMHadSeeds_v11: the L1 seed alone (ETMHF)
     HLT_L1AlwaysTrue_v1: the L1 seed alone (always true)
  satisfied by no object (zero bias, random, calibration, monitoring): 12
     HLT_EphemeralPhysics_v10
     HLT_EphemeralZeroBias_v10
     HLT_EcalCalibration_v4
     HLT_HcalCalibration_v6
     HLT_HcalNZS_v22
     HLT_HcalPhiSym_v24
     HLT_Random_v3
     HLT_Physics_v15
     HLT_ZeroBias_v14
     HLT_ZeroBias_Alignment_v9
     HLT_ZeroBias_Beamspot_v17
     HLT_PPSRandom_v1
  satisfied by scouting (DST_): 7
     DST_ZeroBias_v12
     DST_Physics_v17
     DST_PFScouting_CICADAVLoose_v3
     DST_PFScouting_CICADALoose_v3
     DST_PFScouting_CICADAMedium_v3
     DST_PFScouting_CICADATight_v3
     DST_PFScouting_ZeroBias_v5

the paths whose L1 seed fired (they start their reconstruction), by kind:
   single jets, dijets                        accepted 16, stopped 14  HLTDiPFJetAveEtaFilter 4, HLT1PFJet 4, HLT1CaloJet 4, HLTDiCaloJetAveFilter 2
   multijets                                  accepted  0, stopped  0  
   VBF (two jets far apart in eta)            accepted  0, stopped  0  
   missing energy, HT                         accepted  1, stopped 29  HLTMhtFilter 16, HLT1PFMET 5, HLTDeDxFilter 3, HLTHtMhtFilter 2, HLTMuonRecHitClusterFilter 2, HLT1CaloMET 1
   muons                                      accepted  1, stopped 12  HLTMuonL2FromL1TPreFilter 9, HLTMuonL3PreFilter 3
   electrons                                  accepted  0, stopped  0  
   scouting                                   accepted  7, stopped  0  
   no object, L1 seed only, calibration       accepted 15, stopped  0  
   others (isolated tracks, PPS, monitoring)  accepted  0, stopped  7  TriggerResultsFilter 3, HLTPixelIsolTrackL1TFilter 2, HLTPPSCalFilter 2
   all                                        accepted 40, stopped 62
other accepted paths (AlCa_, MC_, Dataset_, Status_): 75

the jet paths (ladders: threshold(stage), stage = accept, seed, or the filter that stopped the path)
leading jets: pf=126.7 pf2=86.7 pfFwd=126.7 calo=118.5 calo2=93.3
ladder PFJet: value=126.723 thresholds(stage): 40(accept) 60(accept) 80(accept) 110(accept) 140(hltSinglePFJet140) 200(hltSingleCaloJet170) 260(seed) 320(seed) 400(seed) 450(seed) 500(seed) 550(seed)
ladder PFJetFwd: value=126.723 thresholds(stage): 40(accept) 60(accept) 80(accept) 140(hltSinglePFFwdJet140) 200(hltSingleCaloFwdJet170) 260(seed) 320(seed) 400(seed) 450(seed) 500(seed)
ladder AK8PFJet: value=139.141 thresholds(stage): 40(accept) 60(accept) 80(accept) 140(hltSinglePFJet140AK8) 200(hltSingleCaloJet170AK8) 260(seed) 320(seed) 400(seed) 450(seed) 500(seed) 550(seed)
ladder DiPFJetAve: value=106.709 thresholds(stage): 40(accept) 60(accept) 80(accept) 140(hltDiCaloJetAve110) 200(hltDiCaloJetAve170) 260(seed) 320(seed) 400(seed) 500(seed)
ladder DiPFJetAve_HFJEC: value=106.709 thresholds(stage): 60(hltDiPFJetAve60ForHFJEC) 80(hltDiPFJetAve80ForHFJEC) 100(hltDiPFJetAve100ForHFJEC) 160(hltDiPFJetAve160ForHFJEC) 220(seed) 260(seed) 300(seed)
ladder PFHT: value=86.695 thresholds(stage): 180(seed) 250(seed) 350(seed) 370(seed) 430(seed) 510(seed) 590(seed) 680(seed) 780(seed) 890(seed) 1050(seed)
cut: label=hltSingleAK4CaloJet80 type=HLT1CaloJet MinPt=80 MinEta=-1 MaxEta=5 input=hltAK4CaloJetsCorrectedIDPassed
cut: label=hltSinglePFJet110 type=HLT1PFJet MinPt=110 MinEta=-1 MaxEta=5 input=hltPFJetsCorrectedMatchedToCaloJets80
cut: label=hltSingleCaloJet110 type=HLT1CaloJet MinPt=110 MinEta=-1 MaxEta=5 input=hltAK4CaloJetsCorrectedIDPassed
cut: label=hltSinglePFJet140 type=HLT1PFJet MinPt=140 MinEta=-1 MaxEta=5 input=hltPFJetsCorrectedMatchedToCaloJets110
cut: label=hltSingleCaloJet170 type=HLT1CaloJet MinPt=170 MinEta=-1 MaxEta=5 input=hltAK4CaloJetsCorrectedIDPassed
cut: label=hltSinglePFJet200 type=HLT1PFJet MinPt=200 MinEta=-1 MaxEta=5 input=hltPFJetsCorrectedMatchedToCaloJets170
cut: label=hltSingleCaloFwdJet110 type=HLT1CaloJet MinPt=110 MinEta=2.7 MaxEta=5 input=hltAK4CaloJetsCorrectedIDPassed
cut: label=hltSinglePFFwdJet140 type=HLT1PFJet MinPt=140 MinEta=2.7 MaxEta=5 input=hltPFJetsCorrectedMatchedToCaloFwdJets110
filter: label=hltSingleAK4CaloJet80 type=HLT1CaloJet n=2 | Jet(118.5,-3.72,0.96) Jet(93.3,-1.58,-2.08)
filter: label=hltSinglePFJet110 type=HLT1PFJet n=1 | Jet(126.7,-3.77,0.96)
filter: label=hltSingleCaloJet110 type=HLT1CaloJet n=1 | Jet(118.5,-3.72,0.96)
filter: label=hltSinglePFJet140 type=HLT1PFJet n=0 |
filter: label=hltSinglePFFwdJet140 type=HLT1PFJet n=0 |
filter: label=hltDiPFJetAve100ForHFJEC type=HLTDiPFJetAveEtaFilter n=1 | Jet(126.7,-3.77,0.96)
filter: label=hltDiCaloJetAve110 type=HLTDiCaloJetAveFilter n=0 |
filter: label=hltMET90 type=HLT1CaloMET n=1 | MET(138.5,0.00,-2.69)
filter: label=hltL2fL1sMu3L1f0L2Filtered0 type=HLTMuonL2FromL1TPreFilter n=1 | Muon(3.1,-2.06,3.03)
filter: label=hltL3fL1sMu3L1f0L2f0L3Filtered3 type=HLTMuonL3PreFilter n=1 | Muon(3.0,-2.04,3.03)
filter: label=hltMu3PFJet40MuCleaned type=HLTPFJetCollectionsFilter n=1 | Jet(86.7,-1.58,-2.08)
path: name=HLT_PFJet110_v13 accept=1 stage=accept seedFired=1 last=hltBoolEnd lastType=HLTBool lastIndex=208 nModules=209 seed="L1_SingleJet90"
path: name=HLT_PFJet140_v32 accept=0 stage=filter seedFired=1 last=hltSinglePFJet140 lastType=HLT1PFJet lastIndex=207 nModules=209 seed="L1_SingleJet90"
path: name=HLT_PFJetFwd140_v31 accept=0 stage=filter seedFired=1 last=hltSinglePFFwdJet140 lastType=HLT1PFJet lastIndex=207 nModules=209 seed="L1_SingleJet90_FWD2p5"
path: name=HLT_DiPFJetAve100_HFJEC_v30 accept=0 stage=filter seedFired=1 last=hltDiPFJetAve100ForHFJEC lastType=HLTDiPFJetAveEtaFilter lastIndex=208 nModules=210 seed="L1_ZeroBias OR L1_SingleJet60_FWD2p5 OR L1_SingleJet
path: name=HLT_PFJet40_v34 accept=1 stage=accept seedFired=1 last=hltBoolEnd lastType=HLTBool lastIndex=208 nModules=209 seed="L1_ZeroBias"
path: name=HLT_Mu3_PFJet40_v29 accept=1 stage=accept seedFired=1 last=hltBoolEnd lastType=HLTBool lastIndex=274 nModules=275 seed="L1_SingleMu3 OR L1_Mu3_Jet30er2p5"

datasets that accept the event:
   AlCaLowPtJet                   2 of 2 paths
   AlCaLumiPixelsCountsExpress    1 of 1 paths
   AlCaLumiPixelsCountsPrompt     2 of 2 paths
   AlCaP0                         1 of 4 paths
   AlCaPhiSym                     1 of 1 paths
   Cosmics                        1 of 1 paths
   DQMOnlineBeamspot              1 of 2 paths
   DQMPPSRandom                   1 of 1 paths
   EcalLaser                      1 of 1 paths
   EmittanceScan0                 1 of 1 paths
   EmittanceScan1                 1 of 1 paths
   EmittanceScan2                 1 of 1 paths
   EmittanceScan3                 1 of 1 paths
   EmittanceScan4                 1 of 1 paths
   EmittanceScan5                 1 of 1 paths
   EphemeralHLTPhysics0           1 of 1 paths
   EphemeralHLTPhysics1           1 of 1 paths
   EphemeralHLTPhysics2           1 of 1 paths
   EphemeralHLTPhysics3           1 of 1 paths
   EphemeralHLTPhysics4           1 of 1 paths
   EphemeralHLTPhysics5           1 of 1 paths
   EphemeralHLTPhysics6           1 of 1 paths
   EphemeralHLTPhysics7           1 of 1 paths
   EphemeralZeroBias0             1 of 1 paths
   EphemeralZeroBias1             1 of 1 paths
   EphemeralZeroBias2             1 of 1 paths
   EphemeralZeroBias3             1 of 1 paths
   EphemeralZeroBias4             1 of 1 paths
   EphemeralZeroBias5             1 of 1 paths
   EphemeralZeroBias6             1 of 1 paths
   EphemeralZeroBias7             1 of 1 paths
   EventDisplay                   1 of 3 paths
   ExpressAlignment               1 of 2 paths
   ExpressPhysics                 4 of 12 paths
   HLTPhysics                     1 of 1 paths
   HcalNZS                        2 of 2 paths
   JetMET0                        18 of 150 paths
   JetMET1                        18 of 150 paths
   L1Accept                       2 of 2 paths
   MonteCarlo                     19 of 26 paths
   Muon0                          1 of 129 paths
   Muon1                          1 of 129 paths
   OnlineMonitor                  26 of 566 paths
   ScoutingPFMonitor              5 of 19 paths
   ScoutingPFRun3                 5 of 19 paths
   TestEnablesEcalHcal            2 of 2 paths
   TestEnablesEcalHcalDQM         2 of 2 paths
   ZeroBias                       3 of 8 paths
streams that accept the event:
   33: ALCALowPtJet ALCALumiPixelsCountsExpress ALCALumiPixelsCountsPrompt ALCAP0 ALCAPHISYM Calibration DQM DQMCalibration DQMEventDisplay DQMOnlineBeamspot DQMPPSRandom EcalCalibration Express ExpressAlignment NanoDST PhysicsCommissioning PhysicsEmittanceScan0 PhysicsEmittanceScan1 PhysicsEmittanceScan2 PhysicsHLTPhysics0 PhysicsHLTPhysics1 PhysicsHLTPhysics2 PhysicsHLTPhysics3 PhysicsJetMET0 PhysicsJetMET1 PhysicsMuon0 PhysicsMuon1 PhysicsScoutingPFMonitor PhysicsZeroBias0 PhysicsZeroBias1 PhysicsZeroBias2 PhysicsZeroBias3 ScoutingPF
accepted: paths=33 datasets=48 streams=33
```

The margins of the chapter: `HLT_PFJet110` 126.7 - 110 = 16.7 GeV; `HLT_PFJet140` 140 - 126.7 = 13.3; `HLT_AK8PFJet140`
140 - 139.1 = 0.9; the dijet average (126.7 + 86.7) / 2 = 106.7, of the calo jets (118.5 + 93.3) / 2 = 105.9 < 110;
the HF calibration paths need a tag jet with |eta| < 1.4, the b jet is at |eta| = 1.58; Delta phi of the two PF jets
|0.96 - (-2.08)| = 3.04. The objects of the two paths accepted on a pileup object are in the `filter:` lines: the
calorimeter missing transverse energy of `HLT_CaloMET90_NotCleaned` is the `hltMET90` object, 138.5 GeV at phi = -2.69
(the reason lines of the ledger read both values from these `filter:` lines; until 12 November 2026 `hltLedger.py`
had the 2024 values written into it); the muon of
`HLT_Mu3_PFJet40` has 3.1 GeV in the muon chambers (`hltL2fL1sMu3L1f0L2Filtered0`) and 3.0 GeV with the tracker
(`hltL3fL1sMu3L1f0L2f0L3Filtered3`; 3.1 and 3.1 in 2024).

### The ledger (`ledger_13_HLT.txt`, section 6)

```
-- 6. The ledger: what the HLT sees, step by step (nothing is added to or removed from the recorded event)
                                                              GeV
step 2: rec-hits, sum ET (ECAL + HCAL + HF)                3198.9
step 2: calorimeter towers, sum ET                         2842.9
step 2: rho of the calorimeter (per unit area)               36.3
step 3: merged tracks, sum pT                               179.5
step 3: tracks of the signal vertex, sum pT                 121.8
step 4: PF candidates, sum pT                              2655.8
step 4: rho of particle flow (per unit area)                 36.7
the two jets of the signal (generator pT; L1 jet; calo jet raw -> corrected; PF jet raw -> corrected):
   gen 117.8: calo 70.9 -> 93.3, PF 83.7 -> 86.7
   gen 115.9: calo 163.5 -> 118.5, PF 156.8 -> 126.7
L1 jets of chapter 12 (unpacked by the HLT): 136 108.5 23
```

### What is quoted from earlier chapters

| number in the chapter | value | source |
|---|---|---|
| L1 algorithms fired, `L1_SingleJet90` margins | 63 of 465; 46.0 GeV (bbar, 136.0), 18.5 GeV (b, 108.5) | `test/12_L1T/out/ledger_12_L1T.txt` |
| L1 objects handed to the global trigger | 9 jets, 12 e/gamma, 12 taus, 1 muon | `test/12_L1T/out/l1t_12_L1T.log` |
| L1 trigger primitives | 2793.0 GeV | `test/12_L1T/out/ledger_12_L1T.txt`, section 1 |
| L1 jet windows (9 x 9 Layer-1 towers) | 74.5 (b), 189.0 GeV (bbar) | `test/12_L1T/out/ledger_12_L1T.txt`, section 3 |
| the muon of a pileup kaon decay | K- of 3.27 GeV, mu- of 2.78 GeV | `test/12_L1T/out/truth_12_L1T.txt` |
| collisions in bx -1, 0, +1 | 71, 68, 54 | `test/11_DIGI/out/ledger_11_DIGI.txt` |
| digis of the event | 544 407 | `test/11_DIGI/out/ledger_11_DIGI.txt` |
| EB data frames of the signal alone (zero suppression of 2025) | 156 (2024: 399) | `test/11_DIGI/out/ledger_11_DIGI.txt` |
| generator jets (visible) | 117.8 (79.8) and 115.9 GeV | `test/07_GenJet/out/` (chapter 7) |
| the neutrino, the electron of the B- decay | 37.5 GeV pT; 5.0 GeV pT (10.9 GeV energy) | chapter 6 (`test/06_Decay/out/ledger_06_Decay.txt`) |

### The Phase-2 statements (section B5 of the chapter)

Read in the release of the book, CMSSW_15_0_5, relative to `$CMSSW_RELEASE_BASE/src` (not the development branch); the
conditions with `conddb list 150X_mcRun4_realistic_v1` (the global tag of `auto:phase2_realistic`,
`Configuration/AlCa/python/autoCond.py`, line 111; the `auto:phase2_realistic_T*` keys of
`Configuration/AlCa/python/autoCondPhase2.py` add only tracker tags, `activeKeys`, line 150) and `conddb dump`.

| statement | where |
|---|---|
| PU 200, the Phase-2 detectors | `Configuration/PyReleaseValidation/python/relval_steps.py`: 4617 (`Run4`: `AVE_200_BX_25ns`; Run 3: `Run3_Flat55To75_PoissonOOTPU`, 4613); `Configuration/Eras/python/Era_Phase2_cff.py`: 24 (`phase2_tracker`, `phase2_hgcal`, `phase2_timing`, `phase2_timing_layer`, ...) |
| the Phase-2 menu and its size: 54 entries in the schedule, 32 `HLT_`, 16 `L1T_`, 4 `MC_`, `HLTriggerFinalPath`, `HLTAnalyzerEndpath` | `HLTrigger/Configuration/python/HLT_75e33_cff.py`, line 353 on; `Configuration/HLT/python/autoHLT.py`: `'relvalRun4' : '75e33'` |
| the jet paths and their seeds | `HLT_75e33/paths/HLT_AK4PFPuppiJet520_cfi.py`; `HLT_75e33/modules/hltL1SeedsForPuppiJetFilter_cfi.py` (`PathStatusFilter`, `'pSinglePuppiJet230'`); `hltSingleAK4PFPuppiJet520_cfi.py` (MinPt 520); `hltDoublePFPuppiJets128MaxEta2p4_cfi.py` (MinPt 128, MaxEta 2.4, MinN 2); `hltDoublePFPuppiJets128Eta2p4MaxDeta1p6_cfi.py`; `hltPFPuppiHT1070_cfi.py`; `hltPFPuppiMETTypeOne140_cfi.py`; seeds of the jet paths `pSinglePuppiJet230`, `pDoublePuppiJet112_112`, `pPuppiHT400 and pQuadJet70_55_40_40` (`hltL1SeedsFor*Jet*_cfi.py`) |
| the Level-1 jet seeds stop at abs(eta) = 2.4 | `L1Trigger/Configuration/python/Phase2GTMenus/SeedDefinitions/step1_2024/l1tGTMenuObjects_cff.py`: 30-34 (`l1tGTsc4Jet`, `CL2JetsSC4`, eta -2.4 to 2.4), used by the three seeds in `l1tGTMenu_hadr_metSeeds_cff.py`: 23-27, 30-39, 76-91; our L1 jet 136.0 GeV at eta -3.74 (`ledger_13_HLT.txt`, section 1) |
| no raw data for the tracker and the HGCAL | `Configuration/StandardSequences/python/DigiToRaw_cff.py`: 22 (the task, no HGCAL packer), 38 ("Remove siPixelRawData until we have phase2 pixel digis"), 41 (`phase2_tracker` drops `siPixelRawData`, `SiStripDigiToRaw`, "# FIXME"); `HLT_75e33/modules/hltSiPixelClusters_cfi.py`: 23 (`simSiPixelDigis:Pixel`); `hltSiPhase2Clusters_cfi.py`: 5 (`mix:Tracker`); `hltHgcalDigis_cfi.py`: 3-6 (`HGCalRawToDigiFake`, `simHGCalUnsuppressedDigis`); `HLT_75e33/sequences/HLTRawToDigiSequence_cfi.py`: 10 (HGCAL, ECAL, HCAL, CSC, DT, GEM; the last five from `rawDataCollector`); Level-1 inputs `l1tGTProducer`, `l1tLayer1EG`, `l1tTkMuonsGmt`, ... in `HLT_75e33/modules/`; `relval_steps.py`: 4728 (`DIGI:pdigi_valid,L1TrackTrigger,L1,L1P2GT,DIGI2RAW,HLT`); our raw event: strip 997.5 + pixel 306.7 of 1861.5 kB (`ledger_13_HLT.txt`, section 1) |
| the PUPPI jet corrections | `HLT_75e33/modules/hltAK4PFPuppiJetCorrectorL1_cfi.py` (`AK4PFPuppiHLT`, `L1FastJet`, rho from `hltFixedGridRhoFastjetAll`), `..._L2_cfi.py`, `..._L3_cfi.py`; `hltFixedGridRhoFastjetAll_cfi.py` (`hltParticleFlowTmp` = `hltParticleFlowTmpBarrel` + `hltPfTICL`); payload `JetCorrectorParametersCollection_Phase2Spring23ticlv4_MC_AK4PFPuppiHLT_v1`, one IOV inserted 2023-09-21 08:50:47; `conddb dump` of payload `1581021745b44b8b690b85a0b5c09713a30d2baf`: L1FastJet and L3Absolute formula `1`, L2Relative in 19 eta bins with edges -5.2, -3.0, -2.9, -2.7, -2.5, -1.8, -1.3, -0.9, -0.4, 0, 0.4, 0.8, 0.9, 1.3, 1.8, 2.5, 2.7, 2.9, 3.0, 5.2 (our bbar PF jet at eta -3.77, `ledger_13_HLT.txt`, section 4); Run 3: `AK4CaloHLT` 82 records, `AK4PFHLT` 118 (`ledger_13_HLT.txt`, sections 2 and 4); `ticl_v5`: `HLT_75e33/sequences/HLTIterTICLSequence_cfi.py`: 13-14 |
| the other jet corrections of the menu | `HLT_75e33/modules/hltAK8PFPuppiJetCorrectorL1_cfi.py`: 4 (`AK8PFPuppi`, also L2, L3) -> `JetCorrectorParametersCollection_Winter22Run3_V1_MC_AK8PFPuppi`; `hltAK4PFJetCorrectorL1_cfi.py` (`AK4PF`, tau paths and `MC_JME`) -> `JetCorrectorParametersCollection_PhaseIIFall17_V5_MC_AK4PF`; the AK8 PUPPI jets in `HLT_75e33/sequences/HLTPFPuppiJMEReconstruction_cfi.py`, run by `MC_JME` through `HLTJMESequence` |
| PUPPI at the HLT | `HLT_75e33/modules/hltPFPuppi_cfi.py`: `PUProxyValue = hltPixelClustersMultiplicity` (11), `usePUProxyValue = True` (66), regions `etaMin (0.0, 2.5)`, `etaMax (2.5, 3.5)`, `MinNeutralPt (0.5105, 0.821)` (22), slopes `(9.51e-06, 1.902e-05)` (23), third region 3.5 to 10.0 with `MinNeutralPt 3.656` (41), `MinNeutralPtSlope 5.072e-05` (42) and `useCharged False` (54); the threshold `cur_NeutralPtMin + iPUProxy * cur_NeutralPtSlope`: `CommonTools/PileupAlgos/interface/PuppiAlgo.h`: 33 (offline comment "Slope vs #pv": `src/PuppiAlgo.cc`: 13); 17 HF particles in the bbar PF jet (`ledger_13_HLT.txt`, section 4) |
| HF calibration, PF hadron calibration, HCAL thresholds | `HLT_75e33/modules/hltParticleFlowTmpBarrel_cfi.py`: `calibHF_use = False` (121), `calibrationsLabel = ''` (122); `HLT_2025v10_cff.py`: `hltParticleFlow`, `calibrationsLabel = "HLT"` (11366, the same line in `HLT_GRun_cff.py`), `calibHF_use = False` (11377); `PFCalibrationRcd` '' -> `PFCalibration_Run3_mc_offline` (Phase-2 GT); 'HLT' -> `PFCalibration_Run3_2024_online_v1` (`150X_mcRun3_2025_realistic_v1`, as in `150X_mcRun3_2024_realistic_v2`); `hltParticleFlowRecHitHBHE_cfi.py`: `usePFThresholdsFromDB = True` (31; also `hltParticleFlowClusterHBHE_cfi.py` 165, `hltParticleFlowClusterHCAL_cfi.py` 41, `hltParticleFlowBlock_cfi.py` 16), the DB value replaces the python one: `RecoParticleFlow/PFClusterProducer/interface/PFRecHitQTests.h` (`cutsFromDB ? item->noiseThreshold() : thresholds_`); `HcalPFCutsRcd`: `HcalPFCuts_2023_V1.0_mc` (Phase-2 GT and `150X_mcRun3_2024_realistic_v2`), `HcalPFCuts_2025_mc` (`150X_mcRun3_2025_realistic_v1`) |
| tracking without vertex trimming | `HLT_75e33/modules/hltInitialStepSeeds_cfi.py`: `InputVertexCollection = ""` (5), `usePV = False` (13); the only `PixelVertexCollectionTrimmer`: `hltPhase2L3FromL1TkMuonTrimmedPixelVertices_cfi.py` (in `HLTPhase2L3FromL1TkSequence_cfi.py`, muons); Run 3: `HLT_2025v10_cff.py` `hltTrimmedPixelVertices` (line 8854) (`fractionSumPt2 = 0.3`) and `hltIter0PFLowPixelSeedsFromPixelTracks` (`InputVertexCollection = hltTrimmedPixelVertices`) |
| vertices and particle flow without timing | `HLT_75e33/modules/hltUnsortedOfflinePrimaryVertices_cfi.py`: `DA_vect` (16), no `TrackTimesLabel`; `hltOfflinePrimaryVertices_cfi.py`: `useTiming = cms.bool(False)` (22, in `assignment`), `jets = hltAk4CaloJetsForTrk` (25); `Configuration/Eras/python/Era_Phase2_cff.py`: `phase2_timing`, `phase2_timing_layer` (17-18, 24); offline: `RecoVertex/Configuration/python/RecoVertex_phase2_timing_cff.py`: `DA2D_vect` (5), `TrackTimesLabel` (12), `useTiming = True` (33), switched on in `RecoVertex_cff.py` (78, 81); no MTD producer among the `EDProducer`s of `HLT_75e33/modules/`; `hltPfTICL_cfi.py`: `useMTDTiming = False` (32), time inputs `tofPID` (29, 31); `hltTiclCandidate_cfi.py`: 27; `hltTiclTrackstersMerge_cfi.py`: 49 |
| no calo-jet cut in the Phase-2 jet paths | `HLT_75e33/paths/HLT_AK4PFPuppiJet520_cfi.py`; among the modules of `HLT_75e33/` the jet filters are `HLT1PFJet` (10) and `HLT2PFJetPFJet` (1) on PUPPI jets; the one `HLT2CaloJetCaloJet` cuts Level-1 jets in `L1T_DoublePFPuppiJets112_2p4_DEta1p6`; `hltAk4CaloJetsForTrk` is read only by `hltOfflinePrimaryVertices_cfi.py` |
| comments of the menu | `HLT_75e33_cff.py`: 411 ("Removed temporarily until final decision on L1T tau Phase-2", above `L1T_DoubleNNTau52`, `L1T_SingleNNTau150`); `HLT_75e33/modules/hltEgammaHoverEL1Seeded_cfi.py`: 17 (`eThresHB`: "Run3 thresholds. Will be overwritten with valid aging customisation"; the same in `hltEgammaHoverEUnseeded_cfi.py`), 21 (`usePFThresholdsFromDB = True`; read in `RecoEgamma/EgammaHLTProducers/plugins/EgammaHLTHcalVarProducerFromRecHit.cc` 107, 125 and `RecoEgamma/EgammaIsolationAlgos/src/EgammaHcalIsolation.cc` 154-158); `HLT_75e33/sequences/HLTInitialStepSequence_cfi.py`: 31-32 ("Probably need to move elsewhere in the final setup", in `_HLTInitialStepSequenceLST`, used under `trackingLST`, 44-45) |
| the Phase-2 Level-1 menus | `Configuration/Applications/python/ConfigBuilder.py`: 1597 (`defaultMenuFile = "step1_2024"`); `L1Trigger/Configuration/python/Phase2GTMenus/step1_2023.py`, `step1_2024.py`, `step1_2024_explicitSeeds.py`: line 1 "# The first prototype menu"; the 19 Level-1 algorithms named by the `PathStatusFilter`s of `HLT_75e33/modules/` are defined (`p... = cms.Path`) in all three `SeedDefinitions/` directories; `pSinglePuppiJet230`: `SeedDefinitions/step1_2023/l1tGTMenu_hadr_metSeeds_cff.py` 25 (`regionsMinPt=cms.vdouble(160,108)`, eta bounds 0, 1.5), `SeedDefinitions/step1_2024/l1tGTMenu_hadr_metSeeds_cff.py` 25 (`get_object_thrs(230, "CL2JetsSC4", "default")`) |
| the second Phase-2 HLT menu | `HLTrigger/Configuration/python/HLT_75e33_timing_cff.py` (schedule line 341: 41 entries, 23 `HLT_`, 16 `L1T_`, the two end paths); `Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py`: 1703 (`HLT:75e33_timing`) |

### The 2024 and the 2025 settings

The chapter was first written on 11 November 2026 with the settings of 2024 and rerun on 12 November 2026 with those of
2025 (Rule I of the Constitution; the 2024 files were kept aside for the comparison). What changed in the job:

- **The menu.** 2024: `-s DIGI2RAW,HLT:GRun`, `/dev/CMSSW_15_0_0/GRun/V62`, whose seeds name 30 Level-1 algorithms
  that the 2024 Level-1 menu of chapter 12 (`L1Menu_Collisions2024_v1_3_0`, 408 algorithms) does not have;
  `hltL1Menu_cff` dropped them from 21 OR seeds and removed 37 paths (898 -> 861; 697 `HLT_` and 20 `DST_` paths
  left, 717). 2025: `-s DIGI2RAW,HLT:@relval2025`, `2025v10`, `/frozen/2025/2e34/v1.0/HLT/V2`, the same modules as
  `GRun` V62 (section 1), on the 2025 Level-1 menu `L1Menu_Collisions2025_v1_0_0` (465 algorithms): 0 seeds changed,
  0 paths removed, all 898 paths run (733 `HLT_`, 21 `DST_`, 754). The 37 paths are back: among them the `L1CICADA`
  and `L1AXO` paths of the tightest thresholds, the PNet tau paths, the barrel-muon paths with `L1_SingleMu5_BMTF` and
  `L1_SingleMu13_SQ14_BMTF`, `HLT_SingleEle8` (the full list is in the 2024 version of `menu_13_HLT.txt`, section 1).
- **The global tag** (`conddb list`): `150X_mcRun3_2024_realistic_v2` -> `150X_mcRun3_2025_realistic_v1`. Unchanged
  for the HLT: the jet energy corrections (`AK4CaloHLT`, `AK4PFHLT`, `AK8CaloHLT`, `AK8PFHLT`: `..._Run3Winter24Digi_...`),
  the particle-flow hadron calibration (`PFCalibrationRcd` label `HLT`: `PFCalibration_Run3_2024_online_v1`), the ECAL
  particle-flow thresholds (`EcalPFRecHitThresholds_34sigma_TL235`), the pixel gains for the HLT. Changed: the online
  beam spot (`..._Early2024_v1_mc` -> `..._Winter25_mc_v1`), the HCAL particle-flow cuts (`HcalPFCuts_2023_V1.0_mc` ->
  `HcalPFCuts_2025_mc`), the Level-1 menu, and the detector conditions of chapter 11 (ECAL pedestals, laser and
  channel status, HCAL pedestals, gains and SiPM parameters, pixel templates, generic errors and Lorentz angle, strip
  bad components, tracker alignment and surface deformations).
- **The input.** The digis of chapter 11 and the Level-1 record of chapter 12 are those of 2025: 544 407 digis
  (551 889), 156 EB frames for the signal alone (399) after the zero suppression of the 2025 selective-readout payload
  (`EcalSRSettings_2024_EBZS9.5_EEZS6.0_DCC2_mc`, low-interest threshold in the barrel 0.3325 GeV, 2024 0.2275 GeV);
  63 Level-1 algorithms fired of 465 (57 of 408), 9 Level-1 jets (12): no jet seeded in tower ring 28, so no VBF, no
  `HTMHF` and no multijet seed fired.

The numbers, step by step (2024 from the files of 11 November 2026, 2025 from `out/`; GeV unless stated):

| | 2024 | 2025 | why (verified, or "not established") |
|---|---|---|---|
| raw event, kB (678 FEDs) | 1928.6 | 1861.5 | |
| ECAL, kB | 173.3 | 103.9 | fewer ECAL frames: chapter 11, mixed EB 2050 -> 964, EE 4144 -> 2520 |
| HCAL, kB | 225.4 | 240.6 | more HB/HE frames after the zero suppression: 5839 -> 6545 (chapter 11, FWLite extras) |
| strip / pixel / preshower, kB | 1001.8 / 316.0 / 136.9 | 997.5 / 306.7 / 137.4 | chapter 11: mixed pixel digis 80 346 -> 77 980; why: not established |
| Level-1: algorithms fired / jets unpacked | 57 / 12 | 63 / 9 | chapter 12 |
| `HLT_` + `DST_` paths / stopped by the seed / started | 717 / 593 / 124 | 754 / 652 / 102 | 37 paths back, none of them started; 22 fewer paths start |
| started: jets / multijets / VBF / MET, HT / muons / electrons | 30 / 6 / 13 / 30 / 13 / 2 | 30 / 0 / 0 / 30 / 13 / 0 | multijet seeds are ORs with `L1_DoubleJet_110_35_DoubleJet35_Mass_Min800`, a VBF algorithm that fired on a ring-28 jet in 2024; the electron paths are seeded by `L1_SingleEG8er2p5`, fired by an e/gamma of 8.0 in 2024, 7.5 in 2025 |
| started: scouting / no object / others | 8 / 15 / 7 | 7 / 15 / 7 | `L1_CICADA_VTight` does not fire in 2025 (score 88.3 with `CICADAModel_v2p2p0`; the 2025 menu fires VLoose to Tight, thresholds 50 to 80) |
| stopped by a filter / accepted | 83 / 41 | 62 / 40 | the 21 VBF, multijet and electron paths that started and failed in 2024 do not start; `DST_PFScouting_CICADAVTight` |
| other paths accepted (`AlCa_`, `MC_`, `Dataset_`, `Status_`) | 73 | 75 | `AlCa_EcalPi0EBonly` (seeded by `L1_ZeroBias` among others) accepts, and with it `Dataset_AlCaP0`; why the pi0 filter passes: not established |
| rec-hits EB / EE, n | 2039 / 4141 | 956 / 2515 | the ECAL frames of chapter 11 (above) |
| rec-hits HB / HE / HF, n | 1882 / 3271 / 1728 | 2162 / 3832 / 1728 | the HCAL frames of chapter 11 (above) |
| rec-hits, sum E_T | 3307.5 | 3198.9 | EB 268.2 -> 142.6, EE 498.6 -> 378.7, HB 344.9 -> 493.4, HE 592.3 -> 578.0, HF 1603.5 -> 1606.2 |
| towers barrel / endcap / HF, n | 844 / 807 / 790 | 492 / 771 / 788 | fewer barrel rec-hits above the tower thresholds |
| towers, sum E_T | 3085.5 | 2842.9 | barrel 444.3 -> 342.8, endcap 1028.2 -> 882.9, HF 1613.0 -> 1617.2 |
| rho calo / rho PF, per unit area | 40.0 / 36.4 | 36.3 / 36.7 | rho calo: fewer towers with energy; not established in detail |
| calo jets raw / raw above 30 / corrected above 30 | 187 / 26 / 2 | 190 / 23 / 2 | |
| b calo jet: raw, x L1FastJet, x L2Relative -> corrected | 68.4, 0.783, 1.653 -> 88.5 | 70.9, 0.809, 1.627 -> 93.3 | same payload `Run3Winter24Digi_AK4CaloHLT`; the factors follow from raw pT and rho |
| bbar calo jet | 162.3, 0.705, 0.991 -> 113.4 | 163.5, 0.734, 0.987 -> 118.5 | idem; the subtraction 47.9 -> 43.6 follows from rho |
| pixel clusters / strip clusters | 20 699 / 115 932 | 20 925 / 115 249 | pixel templates, Lorentz angle, alignment of 2025: not established in detail |
| pixel tracks / pixel vertices / kept | 2978 / 27 / 1 | 2981 / 31 / 1 | idem; the signal is the first vertex in both |
| full tracks (iteration 0 + doublet recovery) | 115 (110 + 5) | 116 (108 + 8) | |
| PF vertex: tracks, distance to the simulated signal | 104, 5 um | 107, 7 um | |
| tracks within 1 mm of the signal vertex, sum pT | 63, 104.8 | 61, 121.8 | |
| tracks in the b jet (dR < 0.4), sum pT | 5, 34.0 | 7, 50.1 | two more: the electron of the B- decay and an 11.2 GeV track (below) |
| PF candidates, sum pT | 1421, 2728.4 | 1366, 2655.8 | photons 276 -> 238, neutral hadrons 224 -> 209 |
| b PF jet: raw, x L1FastJet, x L2Relative -> corrected | 77.9, 0.861, 1.197 -> 80.3 | 83.7, 0.869, 1.192 -> 86.7 | constituents: charged 5 / 34.0 -> 7 / 50.1, photons 21 / 35.7 -> 20 / 28.4, neutral hadrons 5 / 9.0 -> 2 / 5.8 |
| bbar PF jet | 155.6, 0.825, 0.981 -> 125.9 | 156.8, 0.824, 0.981 -> 126.7 | 17 HF hadronic particles in both; the HF rec-hits hardly change |
| `HLT_PFJet110` | accepted by 15.9 | accepted by 16.7 | on the bbar jet in both |
| `HLT_PFJet140` / `HLT_AK8PFJet140` | fail by 14.1 / 0.6 | fail by 13.3 / 0.9 | AK8 jet 139.4 -> 139.1 |
| dijet average PF / calo | 103.1 / 101.0 | 106.7 / 105.9 | `HLT_DiPFJetAve140` stopped by the calo cut at 110 in both; HF calibration paths fail on the tag jet in both |
| `hltMET90` (calorimeter MET) | 133.3 | 138.5 | `HLT_CaloMET90_NotCleaned` accepts in both |
| HLT muon, L2 / L3 | 3.1 / 3.1 | 3.1 / 3.0 | the same pileup kaon decay (chapter 12) |
| datasets / streams | 47 / 32 | 48 / 33 | `AlCaP0` and the stream `ALCAP0` added |
| products of the process `HLT` / file size | 2977 / 21.2 MB | 3121 / 20.8 MB | the 37 paths back |

The tracks of the b jet (FWLite on `hltMergedTracks` within dR < 0.4 of the generator jet, eta = -1.589,
phi = -2.084, in the two files; the charged generator particles above 1 GeV in the same cone for comparison):

```
== 2024
  pt=  6.94 +-  0.14 eta=-1.603 phi=-2.080 dz=  3.119 dR=0.01 pixel=3 strip=12 chi2/ndf=1.32
  pt=  2.13 +-  0.03 eta=-1.688 phi=-1.823 dz=  3.156 dR=0.28 pixel=4 strip=13 chi2/ndf=0.78
  pt=  0.89 +-  0.03 eta=-1.766 phi=-1.954 dz=  3.354 dR=0.22 pixel=5 strip=2 chi2/ndf=0.20
  pt= 13.10 +-  0.21 eta=-1.544 phi=-2.081 dz=  3.091 dR=0.04 pixel=4 strip=17 chi2/ndf=0.80
  pt= 10.94 +-  0.16 eta=-1.532 phi=-2.066 dz=  3.084 dR=0.06 pixel=5 strip=15 chi2/ndf=0.88
== 2025
  pt=  6.99 +-  0.13 eta=-1.602 phi=-2.080 dz=  3.078 dR=0.01 pixel=3 strip=11 chi2/ndf=1.56
  pt=  2.10 +-  0.03 eta=-1.685 phi=-1.822 dz=  3.126 dR=0.28 pixel=3 strip=19 chi2/ndf=1.40
  pt=  0.89 +-  0.03 eta=-1.766 phi=-1.955 dz=  3.366 dR=0.22 pixel=5 strip=2 chi2/ndf=0.16
  pt=  5.03 +-  0.11 eta=-1.431 phi=-2.210 dz=  3.013 dR=0.20 pixel=3 strip=3 chi2/ndf=1.11
  pt= 12.97 +-  0.22 eta=-1.544 phi=-2.081 dz=  3.091 dR=0.04 pixel=4 strip=18 chi2/ndf=0.82
  pt= 11.21 +-  0.40 eta=-1.565 phi=-2.086 dz=  3.101 dR=0.02 pixel=3 strip=6 chi2/ndf=0.53
  pt= 10.95 +-  0.17 eta=-1.532 phi=-2.066 dz=  3.083 dR=0.06 pixel=5 strip=15 chi2/ndf=0.85
== generator (status 1, charged, pT > 1 GeV, dR < 0.4)
  pi+ pt=2.14 eta=-1.685 phi=-1.822 | e- pt=4.95 eta=-1.431 phi=-2.210 | pi- pt=11.00 eta=-1.533 phi=-2.065 | pi+ pt=13.31 eta=-1.545 phi=-2.081
```

The 5.03 GeV track of 2025 is the electron of the B- decay (4.95 GeV), which had no track in 2024. The 11.21 GeV track
has 3 pixel and 6 strip hits, lies 0.04 from the 11.0 GeV pion and has no generator particle of its own: a duplicate of
the pion track or a secondary particle (the generator record holds only the particles of the generator); which of the
two is not established. The 6.9 GeV track at eta = -1.60 has no charged generator particle above 1 GeV either, in both
years. Why the tracking finds the two tracks in 2025 and not in 2024 (the pixel conditions, the alignment) is not
established. The PF b jet is 86.7 GeV, 9 % above the 79.8 GeV of visible particles of chapter 7, where it was within
a per cent in 2024 (80.3 GeV); the generator jet with its neutrino has 117.8 GeV.
