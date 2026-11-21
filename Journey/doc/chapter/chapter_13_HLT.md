# Chapter 13, HLT: technical companion

The High Level Trigger of the ONE event, in five steps. One sample: the event as CMS sees it, the signal with its pileup
(PU = 60 per bunch crossing, bx -12..3, 995 collisions of which 64 in time; Rule H of the Constitution: from chapter 12 on
only the mixed event is followed). On 21 November 2026 the chapter was rerun on bx -12..3 (before: bx -1..1, 71 / 68 / 54
collisions); the outputs of the earlier version are kept in `test/tmp/out_bx-1to1/13_HLT/`.
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
| `-s DIGI2RAW,HLT:@relval2025` | two steps: `DIGI2RAW` (`Configuration/StandardSequences/python/DigiToRaw_cff.py`: the packers of every subdetector and of the Level-1 boards) and the HLT menu that the release gives the 2025 simulation: `Configuration/HLT/python/autoHLT.py` maps `'relval2025'` to `'2025v10'`, the file `HLTrigger/Configuration/python/HLT_2025v10_cff.py`, the frozen 2025 menu `/frozen/2025/2e34/v1.0/HLT/V2` (the RelVal workflows of 2025 in `Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py` use the same `'HLTmenu': '@relval2025'`). It is the physics menu `GRun` of the release frozen: `diff HLT_2025v10_cff.py HLT_GRun_cff.py` gives 10 lines, the two header comments, the `tableName` (`/dev/CMSSW_15_0_0/GRun/V62` in `GRun`) and the menu-type argument of the last two calls, `customizeHLTforAll(fragment, "2025v10")` and `customizeHLTforCMSSW(fragment, "2025v10")`, which neither function uses (`HLTrigger/Configuration/python/customizeHLTforALL.py`, `customizeHLTforCMSSW.py`). Every module, path, seed and threshold is that of `GRun`, which the 2024 comparison of the chapter runs (`run_13_HLT_2024.sh`, section 4). For comparison: `HLT:@relval2024` is `Fake2` (three paths `HLT_Physics`, `HLT_Random`, `HLT_ZeroBias`); the release has no 2024 physics menu |
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
   section 1). With the 2024 Level-1 menu (`L1Menu_Collisions2024_v1_3_0`, 408 algorithms) the same call finds 30
   algorithms of the seeds missing, drops them from 21 OR seeds and removes the 37 paths whose seed can never fire
   (898 paths become 861; `test/13_HLT/run_13_HLT_2024.sh`, section 4, "The 2024 and the 2025 settings"). The call
   stays as the guard of that agreement.
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
cmsRun ../step5_HLT_13_HLT_cfg.py > step5_HLT_13_HLT.log 2>&1      # about 50 s: 41 s to start, 8 s for the event
```

Or the whole chapter: `./13_HLT/run_13_HLT.sh` (81 s without reuse on 21 November 2026: 9 s for cmsDriver, 51 s for
the HLT job, 21 s for the analyzers and the two scripts; variables: `REUSE=1` keeps an existing `step5_HLT_13_HLT.root`,
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
`menu_13_HLT.txt` are identical to those of the 2024 run with `GRun`, `out2024/menu_13_HLT.txt`: the path is the same module for module):

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
`HLT` (39.4 MB in all; the input is 31.0 MB, the digis and truth of the 995 collisions; in 2024, with the 37 paths
removed, 2977 products and 39.4 MB). The ones
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
| 4 | `vector<reco::Muon> hltMuons`, `vector<reco::RecoChargedCandidate> hltIterL3MuonCandidates`, `vector<reco::Track> hltL2Muons` | the HLT muons (none in this event; with bx -1..1, the decay of a pileup kaon) |
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
  sampleLabel="chapter 13: the High Level Trigger|bbbar, pTHat > 100 GeV, 13.6 TeV|with PU = 60 per bx, bx -12..3" \
  > hlt_13_HLT.log 2>&1                                        # about 15 s
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
raw event: 678 FEDs, 1734.1 kB
   strip               381 FEDs     902.8 kB  52.1 %
   pixel               108 FEDs     263.0 kB  15.2 %
   HCAL                 44 FEDs     252.5 kB  14.6 %
   preshower            40 FEDs     155.6 kB   9.0 %
   CSC                  36 FEDs      77.9 kB   4.5 %
   ECAL                 54 FEDs      53.3 kB   3.1 %
   Level-1 trigger       7 FEDs      26.8 kB   1.5 %
   GEM                   2 FEDs       1.7 kB   0.1 %
   DT                    3 FEDs       0.5 kB   0.0 %
   RPC                   3 FEDs       0.2 kB   0.0 %
L1 unpacked: finalOR=1 algorithmsFired=54 jets=4 egammas=9 taus=12 muons=0
paths HLT_ and DST_: 754; stopped by the L1 seed 656; by the prescale 0; start their reconstruction 98 (of which 65 stopped by a filter, 33 accept)
seedObjects: label=hltL1sSingleJet90 expression="L1_SingleJet90" n=2 | L1Jet(122.5,-3.74,1.00) L1Jet(93.5,-1.52,-1.98)
seedObjects: label=hltL1sZeroBias expression="L1_ZeroBias" n=0 |
seedObjects: label=hltL1sSingleJet120 expression="L1_SingleJet120" n=1 | L1Jet(122.5,-3.74,1.00)
seedObjects: label=hltL1sSingleJet90Fwd expression="L1_SingleJet90_FWD2p5" n=1 | L1Jet(122.5,-3.74,1.00)
```

The unpacked Level-1 jets and the seeds of the jet paths (`hlt_13_HLT.log`):

```
  L1Jet: et=122.5 eta=-3.741 phi=1.0005
  L1Jet: et=93.5 eta=-1.5225 phi=-1.97669
  L1Jet: et=39 eta=0.6525 phi=-0.410685
  L1Jet: et=28 eta=0.3915 phi=-2.84669
  seedObjects: label=hltL1sSingleJet90 expression="L1_SingleJet90" n=2 | L1Jet(122.5,-3.74,1.00) L1Jet(93.5,-1.52,-1.98)
  seedObjects: label=hltL1sZeroBias expression="L1_ZeroBias" n=0 |
  seedObjects: label=hltL1sSingleJet120 expression="L1_SingleJet120" n=1 | L1Jet(122.5,-3.74,1.00)
  seedObjects: label=hltL1sSingleJet90Fwd expression="L1_SingleJet90_FWD2p5" n=1 | L1Jet(122.5,-3.74,1.00)
```

### Step 2 (`ledger_13_HLT.txt`, section 2)

```
-- 2. Step 2: calorimeter rec-hits, towers, calo jets
rec-hits          n      sum E     sum ET
EB              762      173.1      126.7
EE              623      439.5      109.5
HB             2367      660.6      478.6
HE             4233     2690.3      512.1
HF             1728    40617.7     1329.6
towers            n     sum ET         EM        HAD
barrel          420      303.4      120.0      183.4
endcap          750      554.8      100.8      454.0
forward         788     1337.4      471.1      866.2
all            1958     2195.6      691.9     1503.7
rho calo (hltFixedGridRhoFastjetAllCalo): 26.544 GeV per unit area
calo jets: raw=192 corrected=192 corrected>10=27 >30=4 >80=2
jec calo: level=L1FastJet records=82 formula="max(0.0001,1-(z/y)*([0]+([1]*(x))*(1+[2]*log(y))))" binVar=JetEta, parVar=Rho,JetPt,JetA,
jec calo: level=L2Relative records=82 formula="max(0.0001,[0]+[1]/(pow(log10(x),2)+[2])+[3]*exp(-[4]*(log10(x)-[5])*(log10(x)-[5])))" binVar=JetEta, parVar=JetPt,
jec calo: level=L3Absolute records=1 formula="1" binVar=JetEta, parVar=JetPt,
jec calo: level=L2L3Residual records=1 formula="1" binVar=JetEta, parVar=JetPt,
CaloJet 0: rawPt=152.5 eta=-3.72 phi=1.00 area=0.503 towers=16 n90=8 emf=0.29 | L1FastJet=0.7940 L2Relative=0.7835 L3Absolute=0.7835 L2L3Residual=0.7835 jecBin=eta[-3.839,-3.664] | recomputedPt=119.5 correctedPt=119.5 | gen=115.9 dR=0.03
CaloJet 1: rawPt=64.7 eta=-1.57 phi=-2.07 area=0.503 towers=30 n90=18 emf=0.54 | L1FastJet=0.8517 L2Relative=1.3986 L3Absolute=1.3986 L2L3Residual=1.3986 jecBin=eta[-1.653,-1.566] | recomputedPt=90.4 correctedPt=90.4 | gen=117.8 dR=0.02
CaloJet 2: rawPt=38.6 eta=-3.53 phi=-0.08 area=0.503 towers=15 n90=12 emf=0.41 | L1FastJet=0.4405 L2Relative=0.5351 L3Absolute=0.5351 L2L3Residual=0.5351 jecBin=eta[-3.664,-3.489] | recomputedPt=20.6 correctedPt=20.6 | gen=- dR=-
CaloJet 6: rawPt=30.3 eta=0.44 phi=-2.88 area=0.503 towers=21 n90=15 emf=0.75 | L1FastJet=0.7804 L2Relative=1.3813 L3Absolute=1.3813 L2L3Residual=1.3813 jecBin=eta[0.435,0.522] | recomputedPt=41.8 correctedPt=41.8 | gen=- dR=-
CaloJet 11: rawPt=25.3 eta=0.68 phi=-0.46 area=0.503 towers=15 n90=9 emf=0.25 | L1FastJet=0.7339 L2Relative=1.3799 L3Absolute=1.3799 L2L3Residual=1.3799 jecBin=eta[0.609,0.696] | recomputedPt=34.9 correctedPt=34.9 | gen=- dR=-
CaloJet 21: rawPt=21.8 eta=-2.44 phi=-1.11 area=0.503 towers=17 n90=13 emf=0.08 | L1FastJet=0.3244 L2Relative=0.7168 L3Absolute=0.7168 L2L3Residual=0.7168 jecBin=eta[-2.5,-2.322] | recomputedPt=15.6 correctedPt=15.6 | gen=- dR=-
CaloJet 35: rawPt=18.4 eta=-2.13 phi=1.08 area=0.503 towers=15 n90=11 emf=0.16 | L1FastJet=0.3287 L2Relative=0.8171 L3Absolute=0.8171 L2L3Residual=0.8171 jecBin=eta[-2.172,-2.043] | recomputedPt=15.1 correctedPt=15.1 | gen=- dR=-
CaloJet 47: rawPt=15.6 eta=-2.15 phi=-1.93 area=0.370 towers=12 n90=9 emf=0.12 | L1FastJet=0.4383 L2Relative=1.0585 L3Absolute=1.0585 L2L3Residual=1.0585 jecBin=eta[-2.172,-2.043] | recomputedPt=16.5 correctedPt=16.5 | gen=- dR=-
CaloJet 48: rawPt=15.6 eta=1.06 phi=2.12 area=0.503 towers=15 n90=11 emf=0.29 | L1FastJet=0.5403 L2Relative=1.3032 L3Absolute=1.3032 L2L3Residual=1.3032 jecBin=eta[1.044,1.131] | recomputedPt=20.3 correctedPt=20.3 | gen=- dR=-
CaloJet 53: rawPt=14.9 eta=-2.14 phi=1.57 area=0.369 towers=13 n90=11 emf=0.22 | L1FastJet=0.4209 L2Relative=1.0372 L3Absolute=1.0372 L2L3Residual=1.0372 jecBin=eta[-2.172,-2.043] | recomputedPt=15.5 correctedPt=15.5 | gen=- dR=-
CaloJet 58: rawPt=13.5 eta=1.26 phi=2.79 area=0.476 towers=13 n90=9 emf=0.25 | L1FastJet=0.5200 L2Relative=1.3467 L3Absolute=1.3467 L2L3Residual=1.3467 jecBin=eta[1.218,1.305] | recomputedPt=18.1 correctedPt=18.1 | gen=- dR=-
CaloJet 65: rawPt=12.3 eta=-1.20 phi=0.98 area=0.503 towers=14 n90=11 emf=0.11 | L1FastJet=0.4454 L2Relative=1.2318 L3Absolute=1.2318 L2L3Residual=1.2318 jecBin=eta[-1.218,-1.131] | recomputedPt=15.1 correctedPt=15.1 | gen=- dR=-
CaloJet 77: rawPt=10.3 eta=1.00 phi=-2.98 area=0.324 towers=11 n90=9 emf=0.46 | L1FastJet=0.6034 L2Relative=1.5045 L3Absolute=1.5045 L2L3Residual=1.5045 jecBin=eta[0.957,1.044] | recomputedPt=15.5 correctedPt=15.5 | gen=- dR=-
zoom gen jet 1: pt=117.8 centre ieta=-19 iphi=49 towersEt(dR<0.4 of the jet axis)=66.5 towersEt(9x9)=68.3 L1 jet 93.5 ieta=-18 iphi=50
zoom gen jet 2: pt=115.9 centre ieta=-34 iphi=12 towersEt(dR<0.4 of the jet axis)=152.9 towersEt(9x9)=192.7 L1 jet 122.5 ieta=-34 iphi=12
```

The arithmetic of the chapter: b jet 64.7 x 0.8517 = 55.1 (9.6 GeV subtracted), x 1.3986 / 0.8517 = 1.642 -> 90.4;
bbar jet 152.5 x 0.7940 = 121.1 (31.4 GeV subtracted), x 0.7835 / 0.7940 = 0.987 -> 119.5. The section lists the calo
jets above 15 GeV after the corrections; the pileup jet of the HF in the table of the chapter is `CaloJet 2`, the two
barrel jets of pileup that stay above 30 GeV are `CaloJet 6` and `CaloJet 11` (`hlt_13_HLT.log`):

```
  CaloJet 2: rawPt=38.6 eta=-3.53 phi=-0.08 area=0.503 towers=15 n90=12 emf=0.41 | L1FastJet=0.4405 L2Relative=0.5351 L3Absolute=0.5351 L2L3Residual=0.5351 jecBin=eta[-3.664,-3.489] | recomputedPt=20.6 correctedPt=20.6 | gen=- dR=-
  CaloJet 6: rawPt=30.3 eta=0.44 phi=-2.88 area=0.503 towers=21 n90=15 emf=0.75 | L1FastJet=0.7804 L2Relative=1.3813 L3Absolute=1.3813 L2L3Residual=1.3813 jecBin=eta[0.435,0.522] | recomputedPt=41.8 correctedPt=41.8 | gen=- dR=-
  CaloJet 11: rawPt=25.3 eta=0.68 phi=-0.46 area=0.503 towers=15 n90=9 emf=0.25 | L1FastJet=0.7339 L2Relative=1.3799 L3Absolute=1.3799 L2L3Residual=1.3799 jecBin=eta[0.609,0.696] | recomputedPt=34.9 correctedPt=34.9 | gen=- dR=-
```

38.6 x 0.4405 = 17.0, x 0.5351 / 0.4405 = 1.215 -> 20.6; the barrel ones 30.3 x 1.3813 = 41.8 and 25.3 x 1.3799 =
34.9 (L1FastJet x 0.780 and x 0.734, then L2Relative x 1.770 and x 1.880: in the barrel the relative correction is
larger than the subtraction). 7 raw calo jets above
30 GeV (`CaloJet` lines of `hlt_13_HLT.log` with `rawPt` > 30: 152.5, 64.7, 38.6, 33.9, 30.8, 30.4, 30.3; the log has
115 `CaloJet` lines), 4 corrected. rho x area: 26.5 x 0.503 = 13.4 GeV. The 9 x 9 windows of the L1 jets of chapter 12:
56.0 (b) and 170.0 GeV (bbar) (the `window:` part of the `L1Jet` lines of `test/12_L1T/out/l1t_12_L1T.log`); the HLT
towers in the same windows (`plugins/HLTCaloAnalyzer.cc` takes the L1 jet of `hltGtStage2Digis` nearest to the calo
jet): 68.3 and 192.7 GeV (`zoom gen jet` lines). For the bbar jet the calo jet and the L1 jet are in the same tower
(ieta -34, iphi 12); for the b jet the calo jet is at ieta -19, iphi 49, one tower away in both from the seed of the
L1 jet (ieta -18, iphi 50). The trigger primitives of chapter 12 (`test/12_L1T/out/ledger_12_L1T.txt`, section 1):
1804.0 GeV in all, HCAL barrel + endcap 155.0 + 156.0 = 311.0 GeV, against the HLT rec-hits of HB and HE, 478.6 +
512.1 = 990.7 GeV; ECAL primitives 428.5 GeV against 126.7 + 109.5 = 236.2 GeV of ECAL rec-hits.

### Step 3 (`ledger_13_HLT.txt`, section 3)

```
-- 3. Step 3: tracking and vertices
clusters: pixel=17842 (modules 1752) strip=99265 (modules 13729)
tracks pixel: n=2251 pt>1=1264 pt>10=99 sumPt=58543.8
tracks iter0: n=262 pt>1=137 pt>10=3 sumPt=400.4
tracks doubletRecovery: n=8 pt>1=3 pt>10=0 sumPt=8.3
tracks merged: n=270 pt>1=140 pt>10=3 sumPt=408.8
tracks merged: highPurity=270 meanPixelHits=4.17 meanStripHits=9.03
truth: signal vertex x=0.0277 y=-0.0169 z=3.1119 | in-time pileup collisions=64 (mean 64)
truth: pileup collisions within 1 mm of the signal in z: 2 | z range -6.52 .. 8.33
vertices pixel: n=27
vertices trimmed (seeds of iteration 0): n=3 | z=3.109(signal) z=2.248(pu13) z=2.560(pu17)
vertices PF: n=3
vertex PF 0: z=3.1145 ndof=98.7 tracks=76 near=65 sumPt=117.0 sumPt2=618.0 sim=signal dzSim=0.0026
vertex PF 1: z=2.2475 ndof=125.9 tracks=81 near=90 sumPt=137.3 sumPt2=305.0 sim=pu13 dzSim=0.0005
vertex PF 2: z=2.5628 ndof=64.0 tracks=67 near=38 sumPt=66.3 sumPt2=190.0 sim=pu17 dzSim=0.0019
signal vertex rank: pixel=0 PF=0 (0 = the first, the primary vertex)
jet tracks: gen jet 1 pt=117.8 eta=-1.59 chargedGenPt=32.6 | tracks dR<0.4: n=9 sumPt=52.1 | from the signal vertex (|dz|<0.1 cm): n=5 sumPt=44.5
jet tracks: gen jet 2 pt=115.9 eta=-3.75 chargedGenPt=44.0 | tracks dR<0.4: n=0 sumPt=0.0 | from the signal vertex (|dz|<0.1 cm): n=0 sumPt=0.0
tracks from the signal vertex (|dz| < 0.1 cm of z=3.115): n=67 sumPt=122.8
vertex pixel 0: z=3.1093 ndof=48.0 tracks=49 near=66 sumPt=122.3 sumPt2=645.6 sim=signal dzSim=0.0027
vertex pixel 1: z=2.2485 ndof=54.0 tracks=55 near=91 sumPt=138.3 sumPt2=306.0 sim=pu13 dzSim=0.0015
vertex pixel 2: z=2.5597 ndof=31.0 tracks=32 near=37 sumPt=65.6 sumPt2=189.4 sim=pu17 dzSim=0.0012
vertex pixel 3: z=-1.8336 ndof=52.0 tracks=53 near=0 sumPt=0.0 sumPt2=0.0 sim=pu25 dzSim=0.0023
vertex pixel 4: z=-3.6209 ndof=25.0 tracks=26 near=0 sumPt=0.0 sumPt2=0.0 sim=pu34 dzSim=0.0015
vertex pixel 5: z=7.2919 ndof=23.0 tracks=24 near=0 sumPt=0.0 sumPt2=0.0 sim=pu12 dzSim=0.0146
vertex pixel 6: z=-3.5398 ndof=20.0 tracks=21 near=0 sumPt=0.0 sumPt2=0.0 sim=pu51 dzSim=0.0007
vertex pixel 7: z=-1.4983 ndof=16.0 tracks=17 near=0 sumPt=0.0 sumPt2=0.0 sim=pu48 dzSim=0.0169
```

The pixel vertices in full (`hlt_13_HLT.log`; `pu<k>` is in-time collision k of `addPileupInfo`, counted from 0, library
event 747 + k: `PileupSummaryInfo::getPU_EventID()` of bunch crossing 0 gives 747 for k = 0, 760 for k = 13, 764 for
k = 17, 772 for k = 25):

```
  vertex pixel 0: z=3.1093 ndof=48.0 tracks=49 near=66 sumPt=122.3 sumPt2=645.6 sim=signal dzSim=0.0027
  vertex pixel 1: z=2.2485 ndof=54.0 tracks=55 near=91 sumPt=138.3 sumPt2=306.0 sim=pu13 dzSim=0.0015
  vertex pixel 2: z=2.5597 ndof=31.0 tracks=32 near=37 sumPt=65.6 sumPt2=189.4 sim=pu17 dzSim=0.0012
  vertex pixel 3: z=-1.8336 ndof=52.0 tracks=53 near=0 sumPt=0.0 sumPt2=0.0 sim=pu25 dzSim=0.0023
  vertex pixel 4: z=-3.6209 ndof=25.0 tracks=26 near=0 sumPt=0.0 sumPt2=0.0 sim=pu34 dzSim=0.0015
  vertex pixel 5: z=7.2919 ndof=23.0 tracks=24 near=0 sumPt=0.0 sumPt2=0.0 sim=pu12 dzSim=0.0146
  vertex pixel 6: z=-3.5398 ndof=20.0 tracks=21 near=0 sumPt=0.0 sumPt2=0.0 sim=pu51 dzSim=0.0007
  vertex pixel 7: z=-1.4983 ndof=16.0 tracks=17 near=0 sumPt=0.0 sumPt2=0.0 sim=pu48 dzSim=0.0169
  vertex pixel 8: z=5.8032 ndof=10.0 tracks=11 near=0 sumPt=0.0 sumPt2=0.0 sim=pu7 dzSim=0.0018
  vertex pixel 9: z=-2.3705 ndof=11.0 tracks=12 near=0 sumPt=0.0 sumPt2=0.0 sim=pu50 dzSim=0.0029
  vertex pixel 10: z=-3.2284 ndof=7.0 tracks=8 near=0 sumPt=0.0 sumPt2=0.0 sim=pu63 dzSim=0.0098
  vertex pixel 11: z=2.8727 ndof=17.0 tracks=18 near=26 sumPt=25.4 sumPt2=36.4 sim=pu56 dzSim=0.0048
  vertex pixel 12: z=-1.2748 ndof=14.0 tracks=15 near=0 sumPt=0.0 sumPt2=0.0 sim=pu19 dzSim=0.0030
  vertex pixel 13: z=-1.1996 ndof=7.0 tracks=8 near=0 sumPt=0.0 sumPt2=0.0 sim=pu53 dzSim=0.0102
  vertex pixel 14: z=1.5578 ndof=11.0 tracks=12 near=0 sumPt=0.0 sumPt2=0.0 sim=pu29 dzSim=0.0053
```

The trimming (`hltTrimmedPixelVertices`, `PixelVertexCollectionTrimmer`: `fractionSumPt2 = 0.3`, `minSumPt2 = 0`,
`maxVtx = 100`, `PVcomparer = HLTPSetPvClusterComparerForIT`: `track_pt_min = 1.0`, `track_pt_max = 20.0`,
`track_chi2_max = 20.0`, `track_prob_min = -1.0`; `HLT_2025v10_cff.py`, lines 149-154 and 8854-8860) keeps the first
three: `RecoVertex/PixelVertexFinding/src/PVClusterComparer.cc`, `pTSquaredSum`, skips tracks below `track_pt_min` and
above `track_chi2_max` in chi2/ndf and counts a track above `track_pt_max` as 20 GeV. The `sumPt2` of the log lines is
the analyzer's own sum, without the cap: by it vertex 2 has 189.4 / 645.6 = 0.29 of the first, by the menu's at least
0.3. The two pileup collisions kept are library events 760 (z = 2.247 cm in `addPileupInfo`; 754 primaries and 2669
`SimTrack`s, the largest event of the library, chapter 10) and 764 (z = 2.561 cm; 317 primaries). The in-time collisions
within 1 mm of the signal (z = 3.112 cm): two (`truth:` line), k = 33 at z = 3.048 and k = 3 at 3.206 (FWLite on
`addPileupInfo`). The tracks of the two barrel pileup jets (FWLite on `hltMergedTracks`, Delta R < 0.4): around
(0.44, -2.84) 11 tracks with 28.1 GeV, 5.86 / 5.23 / 4.49 / 4.42 / 3.14 GeV at z = 2.56, 2.60, 2.56, 2.57, 2.54 cm
(library event 764); around (0.65, -0.45) 6 tracks with 9.4 GeV from z = 2.25 (760) and 2.56 (764).

### Step 4 (`ledger_13_HLT.txt`, section 4)

```
-- 4. Step 4: particle flow and PF jets
pfcands all: n=1392 sumPt=2337.8
pfcands h: n=261 sumPt=388.0
pfcands e: n=0 sumPt=0.0
pfcands mu: n=0 sumPt=0.0
pfcands gamma: n=192 sumPt=160.1
pfcands h0: n=140 sumPt=489.4
pfcands h_HF: n=693 sumPt=1215.2
pfcands egamma_HF: n=106 sumPt=85.1
pfcands region tracker: n=539 sumPt=815.8 charged n=244 sumPt=369.4
pfcands region outside: n=74 sumPt=233.7 charged n=17 sumPt=18.6
pfcands region HF: n=779 sumPt=1288.2 charged n=0 sumPt=0.0
rho (hltFixedGridRhoFastjetAll): 31.711 GeV per unit area
pf jets: raw=209 raw>10=93 raw>30=6 corrected=209 corrected>10=18 >30=4 >30 with a gen jet=2
jec pf: level=L1FastJet records=118 formula="max(0.0001,1-(z/y)*([0]+([1]*(x))*(1+[2]*log(y))))" binVar=JetEta,JetPhi, parVar=Rho,JetPt,JetA, JetPhiBins(eta -5.191..-4.889)=-3.15..3.15, etaBinsWithSeveralPhiBins=36 | split bins: [-1.566,-1.479]x[-3.15,-1.2] [-1.566,-1.479]x[-1.2,-0.8] [-1.566,-1.479]x[-0.8,3.15] [-1.479,-1.392]x[-3.15,-1.2] [-1.479,-1.392]x[-1.2,-0.8] [-1.479,-1.392]x[-0.8,3.15] [-1.392,-1.305]x[-3.1
jec pf: level=L2Relative records=118 formula="max(0.0001,[0]+([1]/(pow(log10(x),2)+[2]))+([3]*exp(-([4]*((log10(x)-[5])*(log10(x)-[5])))))+([6]*exp(-([7]*((log10(x)-[8])*(log10(x)-[8]))))))" binVar=JetEta,JetPhi, parVar=JetPt, JetPhiBins(eta -5.191..-4.889)=-3.15..3.15, etaBinsWithSeveralPhiBins=36 | split bins: [-1.566,-1.479]x[-3.15,-1.2] [-1.566,-1.479]x[-1.2,-0.8] [-1.566,-1.479]x[-0.8,3.15] [-1.479,-1.392]x[-3.1
jec pf: level=L3Absolute records=1 formula="1" binVar=JetEta, parVar=JetPt,
jec pf: level=L2L3Residual records=1 formula="1" binVar=JetEta, parVar=JetPt,
PFJet 0: pt=120.9 rawPt=146.5 eta=-3.76 phi=1.00 area=0.503 n=17 chf=0.00 nhf=0.99 phf=0.00 elf=0.00 muf=0.00 hfhf=0.99 hfemf=0.01 | L1FastJet=0.8390 L2Relative=0.8254 L3Absolute=0.8254 L2L3Residual=0.8254 jecBin=eta[-3.839,-3.664]phi[-3.15,3.15] | recomputedPt=120.9 | gen=115.9 dR=0.02
PFJet 1: pt=83.8 rawPt=79.5 eta=-1.57 phi=-2.08 area=0.503 n=22 chf=0.64 nhf=0.00 phf=0.36 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.8820 L2Relative=1.0533 L3Absolute=1.0533 L2L3Residual=1.0533 jecBin=eta[-1.653,-1.566]phi[-3.15,3.15] | recomputedPt=83.8 | gen=117.8 dR=0.02
PFJet 2: pt=42.3 rawPt=41.8 eta=0.44 phi=-2.84 area=0.503 n=24 chf=0.67 nhf=0.00 phf=0.33 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.8595 L2Relative=1.0126 L3Absolute=1.0126 L2L3Residual=1.0126 jecBin=eta[0.435,0.522]phi[-3.15,3.15] | recomputedPt=42.3 | gen=- dR=-
PFJet 3: pt=33.1 rawPt=33.5 eta=0.65 phi=-0.45 area=0.503 n=15 chf=0.28 nhf=0.58 phf=0.14 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.8324 L2Relative=0.9896 L3Absolute=0.9896 L2L3Residual=0.9896 jecBin=eta[0.609,0.696]phi[-3.15,3.15] | recomputedPt=33.1 | gen=- dR=-
PFJet 4: pt=20.8 rawPt=22.4 eta=1.23 phi=2.45 area=0.503 n=9 chf=0.62 nhf=0.23 phf=0.15 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.7070 L2Relative=0.9317 L3Absolute=0.9317 L2L3Residual=0.9317 jecBin=eta[1.218,1.305]phi[-3.15,3.15] | recomputedPt=20.8 | gen=- dR=-
PFJet 5: pt=20.7 rawPt=20.4 eta=1.22 phi=2.94 area=0.366 n=10 chf=0.33 nhf=0.50 phf=0.17 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.7683 L2Relative=1.0132 L3Absolute=1.0132 L2L3Residual=1.0132 jecBin=eta[1.218,1.305]phi[-3.15,3.15] | recomputedPt=20.7 | gen=- dR=-
zoom gen jet 1: pt=117.8 eta=-1.59 | PF jet 83.8 (raw 79.5) dR=0.02 | constituents: h=9/52.1 e=0/0.0 mu=0/0.0 gamma=13/27.8 h0=0/0.0 h_HF=0/0.0 egamma_HF=0/0.0
zoom gen jet 2: pt=115.9 eta=-3.75 | PF jet 120.9 (raw 146.5) dR=0.02 | constituents: h=0/0.0 e=0/0.0 mu=0/0.0 gamma=0/0.0 h0=0/0.0 h_HF=16/146.2 egamma_HF=1/1.3
```

The arithmetic of the chapter: b jet 79.5 x 0.8820 = 70.1 (9.4 GeV subtracted), x 1.0533 / 0.8820 = 1.194 -> 83.8;
bbar jet 146.5 x 0.8390 = 122.9 (23.6 GeV subtracted; rho x area = 31.7 x 0.503 = 15.9 GeV, the rest is the
parametrisation of L1FastJet), x 0.8254 / 0.8390 = 0.984 -> 120.9. 6 raw PF jets above 30 GeV (the `pf jets:` line;
146.5, 79.5, 41.8, 36.8, 33.5, 32.5 raw), 4 corrected. The PF jets above 15 GeV after the corrections (`hlt_13_HLT.log`,
`PFJet` lines; the table of the PF jets in step 4 of the chapter):

```
  PFJet 0: pt=120.9 rawPt=146.5 eta=-3.76 phi=1.00 area=0.503 n=17 chf=0.00 nhf=0.99 phf=0.00 elf=0.00 muf=0.00 hfhf=0.99 hfemf=0.01 | L1FastJet=0.8390 L2Relative=0.8254 L3Absolute=0.8254 L2L3Residual=0.8254 jecBin=eta[-3.839,-3.664]phi[-3.15,3.15] | recomputedPt=120.9 | gen=115.9 dR=0.02
  PFJet 1: pt=83.8 rawPt=79.5 eta=-1.57 phi=-2.08 area=0.503 n=22 chf=0.64 nhf=0.00 phf=0.36 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.8820 L2Relative=1.0533 L3Absolute=1.0533 L2L3Residual=1.0533 jecBin=eta[-1.653,-1.566]phi[-3.15,3.15] | recomputedPt=83.8 | gen=117.8 dR=0.02
  PFJet 2: pt=42.3 rawPt=41.8 eta=0.44 phi=-2.84 area=0.503 n=24 chf=0.67 nhf=0.00 phf=0.33 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.8595 L2Relative=1.0126 L3Absolute=1.0126 L2L3Residual=1.0126 jecBin=eta[0.435,0.522]phi[-3.15,3.15] | recomputedPt=42.3 | gen=- dR=-
  PFJet 3: pt=33.1 rawPt=33.5 eta=0.65 phi=-0.45 area=0.503 n=15 chf=0.28 nhf=0.58 phf=0.14 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.8324 L2Relative=0.9896 L3Absolute=0.9896 L2L3Residual=0.9896 jecBin=eta[0.609,0.696]phi[-3.15,3.15] | recomputedPt=33.1 | gen=- dR=-
  PFJet 4: pt=20.8 rawPt=22.4 eta=1.23 phi=2.45 area=0.503 n=9 chf=0.62 nhf=0.23 phf=0.15 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.7070 L2Relative=0.9317 L3Absolute=0.9317 L2L3Residual=0.9317 jecBin=eta[1.218,1.305]phi[-3.15,3.15] | recomputedPt=20.8 | gen=- dR=-
  PFJet 5: pt=20.7 rawPt=20.4 eta=1.22 phi=2.94 area=0.366 n=10 chf=0.33 nhf=0.50 phf=0.17 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.7683 L2Relative=1.0132 L3Absolute=1.0132 L2L3Residual=1.0132 jecBin=eta[1.218,1.305]phi[-3.15,3.15] | recomputedPt=20.7 | gen=- dR=-
  PFJet 6: pt=19.3 rawPt=36.8 eta=-3.59 phi=-0.07 area=0.503 n=15 chf=0.00 nhf=0.94 phf=0.00 elf=0.00 muf=0.00 hfhf=0.94 hfemf=0.06 | L1FastJet=0.4353 L2Relative=0.5240 L3Absolute=0.5240 L2L3Residual=0.5240 jecBin=eta[-3.664,-3.489]phi[-3.15,3.15] | recomputedPt=19.3 | gen=- dR=-
  PFJet 7: pt=16.8 rawPt=22.7 eta=-2.10 phi=1.03 area=0.503 n=7 chf=0.20 nhf=0.64 phf=0.16 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.5248 L2Relative=0.7386 L3Absolute=0.7386 L2L3Residual=0.7386 jecBin=eta[-2.172,-2.043]phi[-3.15,3.15] | recomputedPt=16.8 | gen=- dR=-
  PFJet 8: pt=16.6 rawPt=21.1 eta=-2.31 phi=-1.11 area=0.439 n=12 chf=0.22 nhf=0.66 phf=0.12 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.5427 L2Relative=0.7848 L3Absolute=0.7848 L2L3Residual=0.7848 jecBin=eta[-2.322,-2.172]phi[-3.15,3.15] | recomputedPt=16.6 | gen=- dR=-
  PFJet 9: pt=15.0 rawPt=17.3 eta=-1.23 phi=1.04 area=0.503 n=8 chf=0.31 nhf=0.60 phf=0.09 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.6303 L2Relative=0.8673 L3Absolute=0.8673 L2L3Residual=0.8673 jecBin=eta[-1.305,-1.218]phi[-0.8,3.15] | recomputedPt=15.0 | gen=- dR=-
```

### Step 5 (`ledger_13_HLT.txt`, section 5)

```
-- 5. Step 5: the decision
HLT_ and DST_ paths that accept the event: 33
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
  satisfied by a pileup object: 0
  satisfied by the L1 seed alone: 2
     HLT_L1ETMHadSeeds_v11: the L1 seed alone (ETMHF)
     HLT_L1AlwaysTrue_v1: the L1 seed alone (always true)
  satisfied by no object (zero bias, random, calibration, monitoring): 11
     HLT_EphemeralPhysics_v10
     HLT_EphemeralZeroBias_v10
     HLT_EcalCalibration_v4
     HLT_HcalCalibration_v6
     HLT_HcalNZS_v22
     HLT_Random_v3
     HLT_Physics_v15
     HLT_ZeroBias_v14
     HLT_ZeroBias_Alignment_v9
     HLT_ZeroBias_Beamspot_v17
     HLT_PPSRandom_v1
  satisfied by scouting (DST_): 4
     DST_ZeroBias_v12
     DST_Physics_v17
     DST_PFScouting_CICADAVLoose_v3
     DST_PFScouting_ZeroBias_v5

the paths whose L1 seed fired (they start their reconstruction), by kind:
   single jets, dijets                        accepted 16, stopped 14  HLTDiPFJetAveEtaFilter 4, HLT1PFJet 4, HLT1CaloJet 4, HLTDiCaloJetAveFilter 2
   multijets                                  accepted  0, stopped  0  
   VBF (two jets far apart in eta)            accepted  0, stopped  3  HLT1CaloMET 3
   missing energy, HT                         accepted  0, stopped 41  HLT1CaloMET 25, HLTHtMhtFilter 9, HLTMhtFilter 5, HLTMuonRecHitClusterFilter 2
   muons                                      accepted  0, stopped  0  
   electrons                                  accepted  0, stopped  0  
   scouting                                   accepted  4, stopped  0  
   no object, L1 seed only, calibration       accepted 13, stopped  0  
   others (isolated tracks, PPS, monitoring)  accepted  0, stopped  7  TriggerResultsFilter 3, HLTPixelIsolTrackL1TFilter 2, HLTPPSCalFilter 2
   all                                        accepted 33, stopped 65
other accepted paths (AlCa_, MC_, Dataset_, Status_): 71

the jet paths (ladders: threshold(stage), stage = accept, seed, or the filter that stopped the path)
leading jets: pf=120.9 pf2=83.8 pfFwd=120.9 calo=119.5 calo2=90.4
ladder PFJet: value=120.935 thresholds(stage): 40(accept) 60(accept) 80(accept) 110(accept) 140(hltSinglePFJet140) 200(hltSingleCaloJet170) 260(seed) 320(seed) 400(seed) 450(seed) 500(seed) 550(seed)
ladder PFJetFwd: value=120.935 thresholds(stage): 40(accept) 60(accept) 80(accept) 140(hltSinglePFFwdJet140) 200(hltSingleCaloFwdJet170) 260(seed) 320(seed) 400(seed) 450(seed) 500(seed)
ladder AK8PFJet: value=125.82 thresholds(stage): 40(accept) 60(accept) 80(accept) 140(hltSinglePFJet140AK8) 200(hltSingleCaloJet170AK8) 260(seed) 320(seed) 400(seed) 450(seed) 500(seed) 550(seed)
ladder DiPFJetAve: value=102.345 thresholds(stage): 40(accept) 60(accept) 80(accept) 140(hltDiCaloJetAve110) 200(hltDiCaloJetAve170) 260(seed) 320(seed) 400(seed) 500(seed)
ladder DiPFJetAve_HFJEC: value=102.345 thresholds(stage): 60(hltDiPFJetAve60ForHFJEC) 80(hltDiPFJetAve80ForHFJEC) 100(hltDiPFJetAve100ForHFJEC) 160(hltDiPFJetAve160ForHFJEC) 220(seed) 260(seed) 300(seed)
ladder PFHT: value=159.239 thresholds(stage): 180(hltPFHT180Jet30) 250(seed) 350(seed) 370(seed) 430(seed) 510(hltHT460Jet30) 590(hltHT515Jet30) 680(hltHT580Jet30) 780(hltHT680Jet30) 890(hltHT790Jet30) 1050(hltHT900Jet30)
cut: label=hltSingleAK4CaloJet80 type=HLT1CaloJet MinPt=80 MinEta=-1 MaxEta=5 input=hltAK4CaloJetsCorrectedIDPassed
cut: label=hltSinglePFJet110 type=HLT1PFJet MinPt=110 MinEta=-1 MaxEta=5 input=hltPFJetsCorrectedMatchedToCaloJets80
cut: label=hltSingleCaloJet110 type=HLT1CaloJet MinPt=110 MinEta=-1 MaxEta=5 input=hltAK4CaloJetsCorrectedIDPassed
cut: label=hltSinglePFJet140 type=HLT1PFJet MinPt=140 MinEta=-1 MaxEta=5 input=hltPFJetsCorrectedMatchedToCaloJets110
cut: label=hltSingleCaloJet170 type=HLT1CaloJet MinPt=170 MinEta=-1 MaxEta=5 input=hltAK4CaloJetsCorrectedIDPassed
cut: label=hltSinglePFJet200 type=HLT1PFJet MinPt=200 MinEta=-1 MaxEta=5 input=hltPFJetsCorrectedMatchedToCaloJets170
cut: label=hltSingleCaloFwdJet110 type=HLT1CaloJet MinPt=110 MinEta=2.7 MaxEta=5 input=hltAK4CaloJetsCorrectedIDPassed
cut: label=hltSinglePFFwdJet140 type=HLT1PFJet MinPt=140 MinEta=2.7 MaxEta=5 input=hltPFJetsCorrectedMatchedToCaloFwdJets110
filter: label=hltSingleAK4CaloJet80 type=HLT1CaloJet n=2 | Jet(119.5,-3.72,1.00) Jet(90.4,-1.57,-2.07)
filter: label=hltSinglePFJet110 type=HLT1PFJet n=1 | Jet(120.9,-3.76,1.00)
filter: label=hltSingleCaloJet110 type=HLT1CaloJet n=1 | Jet(119.5,-3.72,1.00)
filter: label=hltSinglePFJet140 type=HLT1PFJet n=0 |
filter: label=hltSinglePFFwdJet140 type=HLT1PFJet n=0 |
filter: label=hltDiPFJetAve100ForHFJEC type=HLTDiPFJetAveEtaFilter n=1 | Jet(120.9,-3.76,1.00)
filter: label=hltDiCaloJetAve110 type=HLTDiCaloJetAveFilter n=0 |
filter: label=hltMET90 type=HLT1CaloMET n=0 |
path: name=HLT_PFJet110_v13 accept=1 stage=accept seedFired=1 last=hltBoolEnd lastType=HLTBool lastIndex=208 nModules=209 seed="L1_SingleJet90"
path: name=HLT_PFJet140_v32 accept=0 stage=filter seedFired=1 last=hltSinglePFJet140 lastType=HLT1PFJet lastIndex=207 nModules=209 seed="L1_SingleJet90"
path: name=HLT_PFJetFwd140_v31 accept=0 stage=filter seedFired=1 last=hltSinglePFFwdJet140 lastType=HLT1PFJet lastIndex=207 nModules=209 seed="L1_SingleJet90_FWD2p5"
path: name=HLT_DiPFJetAve100_HFJEC_v30 accept=0 stage=filter seedFired=1 last=hltDiPFJetAve100ForHFJEC lastType=HLTDiPFJetAveEtaFilter lastIndex=208 nModules=210 seed="L1_ZeroBias OR L1_SingleJet60_FWD2p5 OR L1_SingleJet
path: name=HLT_PFJet40_v34 accept=1 stage=accept seedFired=1 last=hltBoolEnd lastType=HLTBool lastIndex=208 nModules=209 seed="L1_ZeroBias"
path: name=HLT_Mu3_PFJet40_v29 accept=0 stage=seed seedFired=0 last=hltL1sSingleMu3IorMu3Jet30er2p5 lastType=HLTL1TSeed lastIndex=5 nModules=275 seed="L1_SingleMu3 OR L1_Mu3_Jet30er2p5"

datasets that accept the event:
   AlCaLowPtJet                   2 of 2 paths
   AlCaLumiPixelsCountsExpress    1 of 1 paths
   AlCaLumiPixelsCountsPrompt     2 of 2 paths
   AlCaP0                         1 of 4 paths
   AlCaPhiSym                     1 of 1 paths
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
   HcalNZS                        1 of 2 paths
   JetMET0                        17 of 150 paths
   JetMET1                        17 of 150 paths
   L1Accept                       2 of 2 paths
   MonteCarlo                     18 of 26 paths
   OnlineMonitor                  22 of 566 paths
   ScoutingPFMonitor              2 of 19 paths
   ScoutingPFRun3                 2 of 19 paths
   TestEnablesEcalHcal            2 of 2 paths
   TestEnablesEcalHcalDQM         2 of 2 paths
   ZeroBias                       3 of 8 paths
streams that accept the event:
   31: ALCALowPtJet ALCALumiPixelsCountsExpress ALCALumiPixelsCountsPrompt ALCAP0 ALCAPHISYM Calibration DQM DQMCalibration DQMEventDisplay DQMOnlineBeamspot DQMPPSRandom EcalCalibration Express ExpressAlignment NanoDST PhysicsCommissioning PhysicsEmittanceScan0 PhysicsEmittanceScan1 PhysicsEmittanceScan2 PhysicsHLTPhysics0 PhysicsHLTPhysics1 PhysicsHLTPhysics2 PhysicsHLTPhysics3 PhysicsJetMET0 PhysicsJetMET1 PhysicsScoutingPFMonitor PhysicsZeroBias0 PhysicsZeroBias1 PhysicsZeroBias2 PhysicsZeroBias3 ScoutingPF
accepted: paths=29 datasets=45 streams=31
```

The margins of the chapter: `HLT_PFJet110` 120.9 - 110 = 10.9 GeV; `HLT_PFJet140` 140 - 120.9 = 19.1; `HLT_AK8PFJet140`
140 - 125.8 = 14.2; the dijet average (120.9 + 83.8) / 2 = 102.3, of the calo jets (119.5 + 90.4) / 2 = 105.0 < 110;
the HF calibration paths need a tag jet with |eta| < 1.4, the b jet is at |eta| = 1.57; Delta phi of the two PF jets
|1.00 - (-2.08)| = 3.08. No path accepts on a pileup object: the calorimeter missing transverse energy is the
`hltMET50` / `hltMET60` object, 66.4 GeV at phi = -2.40 (`filter:` lines), below `hltMET70` ... `hltMET110` of the 28
missing-energy and VBF paths that stop on it; the PF MET of `MC_PFMET` is 55.4 GeV (`hltPFMETOpenFilter`); the calo
HT of `hltHT460Jet30` ... is 168.2 GeV, the PF HT of `hltPFHT180Jet30` 159.2 GeV; the MHT of the calo jets
(`hltMHT50`) 24.1 GeV. `HLT_Mu3_PFJet40`, `HLT_L1SingleMuCosmics` and `HLT_HcalPhiSym` (seeded by e/gamma or muon
algorithms) stop at their seed: there is no Level-1 muon. Until 21 November 2026 (bx -1..1) `HLT_Mu3_PFJet40` and
`HLT_CaloMET90_NotCleaned` accepted on a pileup kaon muon (3.1 GeV in the muon chambers, 3.0 GeV with the tracker) and
on a calorimeter MET of 138.5 GeV; `hltLedger.py` keeps the two classifications, and `HLT_PFHT*` counts since then
under "missing energy, HT" (its regex missed it; the seed `L1_HTT120er` did not fire before).

### The ledger (`ledger_13_HLT.txt`, section 6)

```
-- 6. The ledger: what the HLT sees, step by step (nothing is added to or removed from the recorded event)
                                                              GeV
step 2: rec-hits, sum ET (ECAL + HCAL + HF)                2556.5
step 2: calorimeter towers, sum ET                         2195.6
step 2: rho of the calorimeter (per unit area)               26.5
step 3: merged tracks, sum pT                               408.8
step 3: tracks of the signal vertex, sum pT                 122.8
step 4: PF candidates, sum pT                              2337.8
step 4: rho of particle flow (per unit area)                 31.7
the two jets of the signal (generator pT; L1 jet; calo jet raw -> corrected; PF jet raw -> corrected):
   gen 117.8: calo 64.7 -> 90.4, PF 79.5 -> 83.8
   gen 115.9: calo 152.5 -> 119.5, PF 146.5 -> 120.9
L1 jets of chapter 12 (unpacked by the HLT): 122.5 93.5 39
```

### What is quoted from earlier chapters

| number in the chapter | value | source |
|---|---|---|
| L1 algorithms fired, `L1_SingleJet90` margins | 54 of 465; 32.5 GeV (bbar, 122.5), 3.5 GeV (b, 93.5) | `test/12_L1T/out/ledger_12_L1T.txt` |
| L1 objects handed to the global trigger | 4 jets (122.5, 93.5, 39.0, 28.0), 9 e/gamma, 12 taus, no muon | `test/12_L1T/out/l1t_12_L1T.log` |
| L1 algorithms: 8 without an object, 22 satisfied by the two signal jets alone, 24 others | the lists | `test/12_L1T/out2024/compare_12_L1T.txt`, section (6), and section 6 of the ledger |
| L1 energy sums: HTT 132.5, ETMHF 78.5 GeV; CICADA score 55.1 (threshold of `L1_CICADA_VLoose` 50) | | `test/12_L1T/out/ledger_12_L1T.txt`, sections 4 and 6 |
| L1 trigger primitives | 1804.0 GeV (HB + HE 311.0, ECAL 428.5) | `test/12_L1T/out/ledger_12_L1T.txt`, section 1 |
| L1 jet windows (9 x 9 Layer-1 towers) | 56.0 (b), 170.0 GeV (bbar) | `test/12_L1T/out/l1t_12_L1T.log`, `L1Jet` lines, `window: layer1=` |
| collisions in bx -12 ... +3 | 746 early, 64 in time (library events 747-810), 185 late | `test/11_DIGI/out/ledger_11_DIGI.txt`, `test/08_PU/out/draw_08_PU.txt` |
| the largest collision of the library, library event 760: 754 primaries | | chapter 10 (`test/10_SIM/out/step2_SIM_pileup_10_SIM.root`, FWLite) |
| digis of the event; ECAL frames EB / EE | 475 110; 766 / 623 | `test/11_DIGI/out/ledger_11_DIGI.txt` |
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
| the Level-1 jet seeds stop at abs(eta) = 2.4 | `L1Trigger/Configuration/python/Phase2GTMenus/SeedDefinitions/step1_2024/l1tGTMenuObjects_cff.py`: 30-34 (`l1tGTsc4Jet`, `CL2JetsSC4`, eta -2.4 to 2.4), used by the three seeds in `l1tGTMenu_hadr_metSeeds_cff.py`: 23-27, 30-39, 76-91; our L1 jet 122.5 GeV at eta -3.74 (`ledger_13_HLT.txt`, section 1) |
| no raw data for the tracker and the HGCAL | `Configuration/StandardSequences/python/DigiToRaw_cff.py`: 22 (the task, no HGCAL packer), 38 ("Remove siPixelRawData until we have phase2 pixel digis"), 41 (`phase2_tracker` drops `siPixelRawData`, `SiStripDigiToRaw`, "# FIXME"); `HLT_75e33/modules/hltSiPixelClusters_cfi.py`: 23 (`simSiPixelDigis:Pixel`); `hltSiPhase2Clusters_cfi.py`: 5 (`mix:Tracker`); `hltHgcalDigis_cfi.py`: 3-6 (`HGCalRawToDigiFake`, `simHGCalUnsuppressedDigis`); `HLT_75e33/sequences/HLTRawToDigiSequence_cfi.py`: 10 (HGCAL, ECAL, HCAL, CSC, DT, GEM; the last five from `rawDataCollector`); Level-1 inputs `l1tGTProducer`, `l1tLayer1EG`, `l1tTkMuonsGmt`, ... in `HLT_75e33/modules/`; `relval_steps.py`: 4728 (`DIGI:pdigi_valid,L1TrackTrigger,L1,L1P2GT,DIGI2RAW,HLT`); our raw event: strip 902.8 + pixel 263.0 of 1734.1 kB (`ledger_13_HLT.txt`, section 1) |
| the PUPPI jet corrections | `HLT_75e33/modules/hltAK4PFPuppiJetCorrectorL1_cfi.py` (`AK4PFPuppiHLT`, `L1FastJet`, rho from `hltFixedGridRhoFastjetAll`), `..._L2_cfi.py`, `..._L3_cfi.py`; `hltFixedGridRhoFastjetAll_cfi.py` (`hltParticleFlowTmp` = `hltParticleFlowTmpBarrel` + `hltPfTICL`); payload `JetCorrectorParametersCollection_Phase2Spring23ticlv4_MC_AK4PFPuppiHLT_v1`, one IOV inserted 2023-09-21 08:50:47; `conddb dump` of payload `1581021745b44b8b690b85a0b5c09713a30d2baf`: L1FastJet and L3Absolute formula `1`, L2Relative in 19 eta bins with edges -5.2, -3.0, -2.9, -2.7, -2.5, -1.8, -1.3, -0.9, -0.4, 0, 0.4, 0.8, 0.9, 1.3, 1.8, 2.5, 2.7, 2.9, 3.0, 5.2 (our bbar PF jet at eta -3.76, `ledger_13_HLT.txt`, section 4); Run 3: `AK4CaloHLT` 82 records, `AK4PFHLT` 118 (`ledger_13_HLT.txt`, sections 2 and 4); `ticl_v5`: `HLT_75e33/sequences/HLTIterTICLSequence_cfi.py`: 13-14 |
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
2025 (Rule I of the Constitution). Since 21 November 2026, when the chapter moved to bx -12..3, the 2024 run is remade
next to the 2025 one by `test/13_HLT/run_13_HLT_2024.sh` (Rule I3; about two minutes):

```bash
cd $J/test
./12_L1T/run_12_L1T_2024.sh       # first: the 2024 DIGI and L1 of the same event -> test/12_L1T/out2024/
./13_HLT/run_13_HLT_2024.sh       # DIGI2RAW + HLT:GRun on 12_L1T/out2024/step4_L1_12_L1T.root -> test/13_HLT/out2024/
```

The script is `run_13_HLT.sh` with `YEAR=2024` (`auto:phase1_2024_realistic` = `150X_mcRun3_2024_realistic_v2`,
`Run3_2024`), `OUT=test/13_HLT/out2024`, the configuration `test/13_HLT/step5_HLT_2024_13_HLT_cfg.py`, the same customise
lines, and the menu `-s DIGI2RAW,HLT:GRun` (`MENU=GRun`): the release has no 2024 physics menu (`HLT:@relval2024` is
`Fake2`), so the 2024 run takes `/dev/CMSSW_15_0_0/GRun/V62`, module for module the `2025v10` of the chapter, and
`hltL1Menu_cff.adaptSeedsToL1Menu` reads the `algo:` lines of `test/12_L1T/out2024/l1t_12_L1T.log` (the 2024 Level-1
menu). The four analyzers, `hltMenu.py` and `hltLedger.py` write the same files as in `out/`. What changes in the job:

- **The menu.** 2024: 30 algorithms of the seeds are not in `L1Menu_Collisions2024_v1_3_0` (408 algorithms);
  `hltL1Menu_cff` drops them from 21 OR seeds and removes 37 paths (898 -> 861; 697 `HLT_` and 20 `DST_` paths, 717;
  `out2024/step5_HLT_13_HLT.log`: `hltL1Menu_cff: 408 L1 algorithms in the menu of chapter 12; 21 seeds changed, 37 paths
  removed`; the list in `out2024/menu_13_HLT.txt`, section 1). 2025: `2025v10` on `L1Menu_Collisions2025_v1_0_0` (465
  algorithms): 0 seeds changed, 0 paths removed, 898 paths (733 `HLT_`, 21 `DST_`, 754). The 37 paths are the
  `L1CICADA` and `L1AXO` paths of the tightest thresholds, the PNet tau paths, the barrel-muon paths with
  `L1_SingleMu5_BMTF` and `L1_SingleMu13_SQ14_BMTF`, `HLT_SingleEle8` and others; none of them starts in 2025.
- **The global tag** (`conddb list`): `150X_mcRun3_2024_realistic_v2` -> `150X_mcRun3_2025_realistic_v1`. Unchanged
  for the HLT: the jet energy corrections (`AK4CaloHLT`, `AK4PFHLT`, `AK8CaloHLT`, `AK8PFHLT`: `..._Run3Winter24Digi_...`),
  the particle-flow hadron calibration (`PFCalibrationRcd` label `HLT`: `PFCalibration_Run3_2024_online_v1`), the ECAL
  particle-flow thresholds (`EcalPFRecHitThresholds_34sigma_TL235`), the pixel gains for the HLT. Changed: the online
  beam spot (`..._Early2024_v1_mc` -> `..._Winter25_mc_v1`), the HCAL particle-flow cuts (`HcalPFCuts_2023_V1.0_mc` ->
  `HcalPFCuts_2025_mc`), the Level-1 menu, and the detector conditions of chapter 11 (ECAL pedestals, laser and
  channel status, HCAL pedestals, gains and SiPM parameters, pixel templates, generic errors and Lorentz angle, strip
  bad components, tracker alignment and surface deformations).
- **The input.** The digis and the Level-1 record of the same SimHits and collisions, with the conditions of each year
  (chapters 11 and 12, `test/12_L1T/out2024/compare_12_L1T.txt`): Level-1 30 of 408 algorithms (2025: 54 of 465), 4 jets
  in both (120.0, 97.5, 40.0, 32.0 against 122.5, 93.5, 39.0, 28.0), HTT 169.5 (132.5), ETMHF 69.0 (78.5: `L1_ETMHF70`
  fires in 2025 only), CICADA score 82.3 against the 113 of `L1_CICADA_VLoose` in 2024 (55.1 against 50 in 2025).

The numbers, step by step (`out2024/` and `out/`, both with bx -12..3; GeV unless stated):

| | 2024 | 2025 | why (verified, or "not established") |
|---|---|---|---|
| raw event, kB (678 FEDs) | 1767.5 | 1734.1 | |
| ECAL, kB | 84.1 | 53.3 | fewer ECAL frames: the barrel zero suppression of 2025 (chapter 11) |
| HCAL / strip / pixel / preshower, kB | 240.2 / 907.3 / 273.9 / 154.8 | 252.5 / 902.8 / 263.0 / 155.6 | |
| Level-1: algorithms fired / jets unpacked | 30 / 4 | 54 / 4 | chapter 12 |
| `HLT_` + `DST_` paths / stopped by the seed / started | 717 / 648 / 69 | 754 / 656 / 98 | 37 paths back, none of them started; 29 more paths start |
| started: jets / VBF / MET, HT / others | 30 / 3 / 13 / 8 | 30 / 3 / 41 / 7 | 2025: `L1_ETMHF70` opens 25 `HLT1CaloMET` paths, 2 `DTCluster` and `HLT_CaloMHT90`; 2024: `L1_HTT160er` fired (HTT 169.5) |
| started: scouting / no object | 3 / 12 | 4 / 13 | 2025: `DST_PFScouting_CICADAVLoose`, `HLT_L1ETMHadSeeds` |
| stopped by a filter / accepted | 38 / 31 | 65 / 33 | |
| other paths accepted (`AlCa_`, `MC_`, `Dataset_`, `Status_`) | 69 | 71 | `AlCa_EcalPi0EBonly` (seeded by `L1_ZeroBias` among others) accepts in 2025 only, and with it `Dataset_AlCaP0`; why the pi0 filter passes: not established |
| rec-hits EB / EE, n | 1481 / 1001 | 762 / 623 | the ECAL frames of chapter 11 |
| rec-hits HB / HE / HF, n | 2146 / 3822 / 1728 | 2367 / 4233 / 1728 | the HCAL frames of chapter 11 (SiPM dark current of 2025) |
| rec-hits, sum E_T | 2584.6 | 2556.5 | EB 221.0 -> 126.7, EE 177.9 -> 109.5, HB 332.2 -> 478.6, HE 532.0 -> 512.1, HF 1321.5 -> 1329.6 |
| towers barrel / endcap / HF, n | 703 / 792 / 788 | 420 / 750 / 788 | fewer barrel ECAL rec-hits above the tower thresholds |
| towers, sum E_T | 2370.2 | 2195.6 | barrel 373.5 -> 303.4, endcap 666.8 -> 554.8, HF 1329.9 -> 1337.4 |
| rho calo / rho PF, per unit area | 29.9 / 32.2 | 26.5 / 31.7 | |
| calorimeter MET (`hltMETOpen`) / PF MET (`hltPFMETOpenFilter`) | 86.2 / 70.3 | 66.4 / 55.4 | not established in detail |
| calo jets raw above 30 / corrected above 30 | 9 / 4 | 7 / 4 | |
| b calo jet: raw, x L1FastJet, x L2Relative -> corrected | 64.8, 0.832, 1.650 -> 88.9 | 64.7, 0.852, 1.642 -> 90.4 | same payload `Run3Winter24Digi_AK4CaloHLT`; the factors follow from raw pT and rho |
| bbar calo jet | 153.0, 0.769, 0.989 -> 116.4 | 152.5, 0.794, 0.987 -> 119.5 | idem; the subtraction 35.4 -> 31.4 follows from rho |
| pixel clusters / strip clusters | 17 861 / 99 879 | 17 842 / 99 265 | |
| pixel tracks / pixel vertices / kept | 2276 / 33 / 2 | 2251 / 27 / 3 | |
| the first pixel vertex | library event 760 (z = 2.249 cm), the signal second | the signal, then 760 and 764 | in 2024 vertex 760 has a track of about 120 GeV (the analyzer's uncapped `sumPt2` is 15 386, against 665 for the signal); the menu counts it as 20 GeV, and why the vertex still comes first in 2024 is not established |
| full tracks (iteration 0 + doublet recovery) / PF vertices | 216 (215 + 1) / 2 | 270 (262 + 8) / 3 | |
| tracks within 1 mm of the signal vertex, sum pT | 69, 129.2 | 67, 122.8 | |
| tracks in the b jet (dR < 0.4), sum pT | 8, 51.3 | 9, 52.1 | below |
| PF candidates, sum pT | 1444, 2388.2 | 1392, 2337.8 | photons 247 -> 192, neutral hadrons 184 -> 140, charged 212 -> 261 |
| b PF jet: raw, x L1FastJet, x L2Relative -> corrected | 78.7, 0.887, 1.202 -> 83.9 | 79.5, 0.882, 1.194 -> 83.8 | constituents: charged 8 / 51.3 -> 9 / 52.1, photons 16 / 24.4 -> 13 / 27.8, neutral hadrons 2 / 3.5 -> 0 |
| bbar PF jet | 146.9, 0.837, 0.984 -> 121.0 | 146.5, 0.839, 0.984 -> 120.9 | 16 HF hadronic + 1 HF electromagnetic particles in both |
| barrel pileup PF jets | 32.2 (0.63, -0.42), 28.6 (0.45, -2.87) | 33.1 (0.65, -0.45), 42.3 (0.44, -2.84) | |
| `HLT_PFJet110` | accepted by 11.0 | accepted by 10.9 | on the bbar jet in both |
| `HLT_PFJet140` / `HLT_AK8PFJet140` | fail by 19.0 / 16.4 | fail by 19.1 / 14.2 | AK8 jet 123.6 -> 125.8 |
| dijet average PF / calo | 102.4 / 102.7 | 102.3 / 105.0 | `HLT_DiPFJetAve140` stopped by the calo cut at 110 in both; HF calibration paths fail on the tag jet in both |
| datasets / streams | 44 / 30 | 45 / 31 | `AlCaP0` and the stream `ALCAP0` added |
| products of the process `HLT` / file size | 2977 / 39.4 MB | 3121 / 39.4 MB | the 37 paths back |

The tracks of the b jet (FWLite on `hltMergedTracks` within dR < 0.4 of the generator jet, eta = -1.589,
phi = -2.084, in the two files; `dz` is the z of the track's reference point, cm):

```
== 2024
  pt=  7.01 +-  0.13 eta=-1.603 phi=-2.080 dz=  3.121 dR=0.01 pixel=3 strip=10 chi2/ndf=1.54
  pt=  2.13 +-  0.03 eta=-1.686 phi=-1.823 dz=  3.134 dR=0.28 pixel=4 strip=13 chi2/ndf=1.12
  pt=  0.55 +-  0.01 eta=-1.602 phi=-2.188 dz=  2.890 dR=0.10 pixel=4 strip=21 chi2/ndf=1.02
  pt=  5.65 +-  0.11 eta=-1.565 phi=-2.077 dz=  3.114 dR=0.02 pixel=3 strip=5 chi2/ndf=0.98
  pt= 13.04 +-  0.21 eta=-1.544 phi=-2.081 dz=  3.092 dR=0.04 pixel=4 strip=16 chi2/ndf=1.40
  pt=  0.60 +-  0.02 eta=-1.591 phi=-2.362 dz=  3.060 dR=0.28 pixel=4 strip=1 chi2/ndf=0.74
  pt= 11.30 +-  0.36 eta=-1.565 phi=-2.086 dz=  3.107 dR=0.02 pixel=3 strip=6 chi2/ndf=0.49
  pt= 11.01 +-  0.17 eta=-1.532 phi=-2.066 dz=  3.081 dR=0.06 pixel=5 strip=13 chi2/ndf=0.99
== 2025
  pt=  6.97 +-  0.14 eta=-1.603 phi=-2.080 dz=  3.116 dR=0.01 pixel=3 strip=11 chi2/ndf=1.31
  pt=  2.12 +-  0.03 eta=-1.686 phi=-1.822 dz=  3.134 dR=0.28 pixel=4 strip=14 chi2/ndf=1.01
  pt=  0.54 +-  0.01 eta=-1.601 phi=-2.187 dz=  2.873 dR=0.10 pixel=4 strip=21 chi2/ndf=0.84
  pt=  5.03 +-  0.11 eta=-1.431 phi=-2.210 dz=  3.008 dR=0.20 pixel=3 strip=3 chi2/ndf=0.51
  pt=  0.74 +-  0.02 eta=-1.582 phi=-2.348 dz=  2.907 dR=0.26 pixel=4 strip=3 chi2/ndf=0.41
  pt= 13.20 +-  0.20 eta=-1.544 phi=-2.081 dz=  3.091 dR=0.04 pixel=4 strip=16 chi2/ndf=0.90
  pt= 11.19 +-  0.35 eta=-1.565 phi=-2.086 dz=  3.099 dR=0.02 pixel=3 strip=6 chi2/ndf=0.32
  pt=  1.31 +-  0.02 eta=-1.526 phi=-1.927 dz=  2.736 dR=0.17 pixel=4 strip=15 chi2/ndf=0.82
  pt= 11.03 +-  0.18 eta=-1.532 phi=-2.066 dz=  3.091 dR=0.06 pixel=5 strip=15 chi2/ndf=1.08
```

The 5.03 GeV track of 2025 at eta = -1.431 is the electron of the B- decay (4.95 GeV, chapter 6); it has no track in
2024. The 11.2 GeV track has 3 pixel and 6 strip hits, lies 0.04 from the 11.0 GeV pion and has no generator particle of
its own, in both years: a duplicate of the pion track or a secondary particle (the generator record holds only the
particles of the generator); which of the two is not established. The soft tracks at z = 2.74 to 2.91 cm come from the
pileup vertices that iteration 0 also seeds in 2025 (2024: one, at z = 2.89 cm). The PF b jet is 83.8 GeV in 2025,
83.9 in 2024, 5 % above the 79.8 GeV of visible particles of chapter 7; the generator jet with its neutrino has 117.8 GeV.
The comparison made with bx -1..1 (11 and 12 November 2026) is in the git history of this companion; the 2025 outputs
of that time are kept in `test/tmp/out_bx-1to1/13_HLT/out/`.
