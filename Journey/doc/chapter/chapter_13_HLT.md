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
  --conditions auto:phase1_2024_realistic --era Run3_2024 --geometry DB:Extended \
  -n -1 -s DIGI2RAW,HLT:GRun --datatier GEN-SIM-DIGI-RAW --eventcontent FEVTDEBUGHLT --fileout file:step5_HLT_13_HLT.root \
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
| `--conditions auto:phase1_2024_realistic` | the global tag of the book, `150X_mcRun3_2024_realistic_v2`. For the HLT it provides the calibrations of every subdetector, the online beam spot, the Level-1 menu that the HLT re-emulates (`L1TUtmTriggerMenuRcd`), the jet energy corrections of the HLT (`JetCorrectionsRecord` labels `AK4CaloHLT`: `JetCorrectorParametersCollection_Run3Winter24Digi_AK4CaloHLT`, `AK4PFHLT`: `..._Run3Winter24Digi_AK4PFHLT`) and the particle-flow hadron calibration (`PFCalibrationRcd` label `HLT`: `PFCalibration_Run3_2024_online_v1`); `conddb list 150X_mcRun3_2024_realistic_v2 \| grep -i "HLT"` |
| `--era Run3_2024` | the era of the DIGI2RAW packers; the HLT menu itself carries no era modifier |
| `--geometry DB:Extended` | the reconstruction geometry from the database |
| `-n -1` with `eventsToProcess` | all events of the input, restricted to the one event |
| `-s DIGI2RAW,HLT:GRun` | two steps: `DIGI2RAW` (`Configuration/StandardSequences/python/DigiToRaw_cff.py`: the packers of every subdetector and of the Level-1 boards) and the HLT menu `HLTrigger/Configuration/python/HLT_GRun_cff.py` (`/dev/CMSSW_15_0_0/GRun/V62`). `HLT:@relval2024` would be `Fake2` (`Configuration/HLT/python/autoHLT.py`: `'relval2024' : 'Fake2'`, three paths `HLT_Physics`, `HLT_Random`, `HLT_ZeroBias`); `HLT:@relval2025` is `2025v10` (`/frozen/2025/2e34/v1.0/HLT/V2`); the release has no 2024 menu |
| `--datatier GEN-SIM-DIGI-RAW --eventcontent FEVTDEBUGHLT` | the content: the input copied, the raw data (`rawDataCollector`), and the `HLTDebugFEVT` commands of `HLTrigger/Configuration/python/HLTrigger_EventContent_cff.py` (the calo and PF jets, the pixel and full tracks, the pixel vertices, the PF candidates, `hltTriggerSummaryAOD`/`RAW`, the unpacked L1 record) |
| `--customise_commands "..."` | the lines explained below |
| `--python_filename ... --no_exec` | write the configuration and do not run it; the script runs `cmsRun` itself |

The customise lines.

1. `adaptSeedsToL1Menu(process, '<12_L1T/out/l1t_12_L1T.log>')` (`python/hltL1Menu_cff.py`). The GRun menu of the release is
   the 2025 menu; 30 Level-1 algorithms named in its seeds are not in the 2024 Level-1 menu of chapter 12
   (`L1Menu_Collisions2024_v1_3_0`, 408 algorithms, read from the `algo:` lines of the chapter-12 log, Rule E), and
   `HLTL1TSeed` throws `Algorithm L1_X, requested as seed by a HLT path, cannot be matched to a L1 algo name in any
   GlobalObjectMap`. A missing algorithm can never fire: in an OR it is dropped (21 seeds); a seed made only of missing
   algorithms can never fire, and the 37 paths that use it are removed with their entries in the dataset filters and the
   `datasets` / `streams` PSets. 898 paths become 861. The report is printed in the driver log and in
   `out/step5_HLT_13_HLT.log`, and kept in `process.hltL1MenuReport` (`out/menu_13_HLT.txt`, section 1).
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
cmsRun ../step5_HLT_13_HLT_cfg.py > step5_HLT_13_HLT.log 2>&1      # about 75 s: a minute to start, 9 s for the event
```

Or the whole chapter: `./13_HLT/run_13_HLT.sh` (about 1.5 minutes when the HLT file is reused, 3 minutes without;
variables: `REUSE=1` keeps an existing `step5_HLT_13_HLT.root`, `REUSE=0` remakes it, `RUN=0` only writes the
configuration, `ANALYZE=0` skips the analyzers, `PNG=0` skips the PNG pages, `MENU=GRun` the menu of `-s HLT:<MENU>`).

### Walk-through of `step5_HLT_13_HLT_cfg.py`

- `process = cms.Process('HLT', Run3_2024)`: the process name under which the new products are stored (an analysis asks
  for `TriggerResults::HLT`).
- `process.load(...)`: `Services_cff`, `pythiapdt_cfi`, the `MessageLogger`, `EventContent_cff`, `mixNoPU_cfi` (loaded by
  cmsDriver for every step, not used), `GeometryRecoDB_cff`, `MagneticField_cff`, `DigiToRaw_cff` (the packers),
  `HLTrigger.Configuration.HLT_GRun_cff` (the menu: 87 000 lines, the paths, sequences, modules, event-setup producers
  and the `datasets` / `streams` PSets of `/dev/CMSSW_15_0_0/GRun/V62`), `EndOfProcess_cff`,
  `FrontierConditions_GlobalTag_cff`.
- `process.digi2raw_step = cms.Path(process.DigiToRaw)`; then the schedule of the menu (`process.schedule`, from the
  `_cff`), with `digi2raw_step` inserted first and `endjob_step`, `FEVTDEBUGHLToutput_step` appended.
- `customizeHLTforMC(process)` (added by cmsDriver for simulation; in this release it changes nothing) and
  `customiseEarlyDelete` (frees intermediate products as soon as their last consumer has run).
- The customise lines of section 1.

The menu in numbers (`out/menu_13_HLT.txt`, section 1, written by `test/common/hltMenu.py` from the configuration):

```
== 1. The menu
process HLT, table /dev/CMSSW_15_0_0/GRun/V62
paths: 862 | AlCa=11 DQM=3 DST=20 Dataset=99 HLT=697 HLTriggerFinalPath=1 HLTriggerFirstPath=1 MC=26 ScoutingPFOutput=1 Status=2 digi2raw=1
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
adapted to the L1 menu of chapter 12 (python/hltL1Menu_cff.py): 21 seeds changed, 37 paths removed
   seed hltL1DiJetVBFMET: 3 of 5 algorithms not in the L1 menu, dropped from the OR: L1_DoubleJet_70_35_DoubleJet35_Mass_Min600_ETMHF65 L1_DoubleJet_70_35_DoubleJet35_Mass_Min700_ETMHF65 L1_DoubleJet_70_35_DoubleJet35_Ma
   seed hltL1DiJetVBFdoubleJet: 3 of 5 algorithms not in the L1 menu, dropped from the OR: L1_DoubleJet_65_35_DoubleJet35_Mass_Min750_DoubleJetCentral50 L1_DoubleJet_65_35_DoubleJet35_Mass_Min850_DoubleJetCentral50 L1_Do
   seed hltL1DiJetVBFincl: 3 of 5 algorithms not in the L1 menu, dropped from the OR: L1_DoubleJet_110_35_DoubleJet35_Mass_Min1000 L1_DoubleJet_110_35_DoubleJet35_Mass_Min1100 L1_DoubleJet_110_35_DoubleJet35_Mass_Min1200
   seed hltL1DoubleJet8535MassMin600Mu3: 3 of 5 algorithms not in the L1 menu, dropped from the OR: L1_DoubleJet_85_35_DoubleJet35_Mass_Min700_Mu3OQ L1_DoubleJet_85_35_DoubleJet35_Mass_Min800_Mu3OQ L1_DoubleJet_85_35_Dou
   seed hltL1VBFDiJetIsoTau: 2 of 4 algorithms not in the L1 menu, dropped from the OR: L1_DoubleJet45_Mass_Min700_IsoTau45er2p1_RmOvlp_dR0p5 L1_DoubleJet45_Mass_Min800_IsoTau45er2p1_RmOvlp_dR0p5
   seed hltL1VBFLooseIsoEG: 2 of 4 algorithms not in the L1 menu, dropped from the OR: L1_DoubleJet45_Mass_Min700_LooseIsoEG20er2p1_RmOvlp_dR0p2 L1_DoubleJet45_Mass_Min800_LooseIsoEG20er2p1_RmOvlp_dR0p2
   seed hltL1sAXOVVTight: "L1_AXO_VVTight" cannot fire with this L1 menu (missing: L1_AXO_VVTight)
   seed hltL1sAXOVVVTight: "L1_AXO_VVVTight" cannot fire with this L1 menu (missing: L1_AXO_VVVTight)
   seed hltL1sBigORDoubleTauJet: "L1_DoubleTau_Iso34_Iso26_er2p1_Jet55_RmOvlp_dR0p5 OR L1_DoubleTau_Iso38_Iso26_er2p1_Jet55_RmOvlp_dR0p5 OR L1_DoubleTau_Iso40_Iso26_er2p1_Jet55_RmOvlp_dR0p5" cannot fire with this L1 menu
   seed hltL1sCICADAVVTight: "L1_CICADA_VVTight" cannot fire with this L1 menu (missing: L1_CICADA_VVTight)
   seed hltL1sCICADAVVVTight: "L1_CICADA_VVVTight" cannot fire with this L1 menu (missing: L1_CICADA_VVVTight)
   seed hltL1sCICADAVVVVTight: "L1_CICADA_VVVVTight" cannot fire with this L1 menu (missing: L1_CICADA_VVVVTight)
   seed hltL1sDSTRun3AXOMediumPFScoutingTracking: "L1_AXO_Medium" cannot fire with this L1 menu (missing: L1_AXO_Medium)
   seed hltL1sDSTRun3DoubleEGPFScoutingPixelTracking: 3 of 7 algorithms not in the L1 menu, dropped from the OR: L1_DoubleEG15_11_er1p2_dR_Max0p6 L1_DoubleEG16_11_er1p2_dR_Max0p6 L1_DoubleEG17_11_er1p2_dR_Max0p6
   seed hltL1sDoubleEGXer1p2dRMaxY: "L1_DoubleEG15_11_er1p2_dR_Max0p6 OR L1_DoubleEG16_11_er1p2_dR_Max0p6 OR L1_DoubleEG17_11_er1p2_dR_Max0p6" cannot fire with this L1 menu (missing: L1_DoubleEG15_11_er1p2_dR_Max0p6 L1_D
   seed hltL1sQuadJetOrHTTOrMuonHTTOrDiTauOrDiTauJet: 3 of 17 algorithms not in the L1 menu, dropped from the OR: L1_DoubleTau_Iso34_Iso26_er2p1_Jet55_RmOvlp_dR0p5 L1_DoubleTau_Iso38_Iso26_er2p1_Jet55_RmOvlp_dR0p5 L1_Dou
   seed hltL1sSingleMu5Barrel: "L1_SingleMu5_BMTF" cannot fire with this L1 menu (missing: L1_SingleMu5_BMTF)
   seed hltL1sSingleMuHP13: "L1_SingleMu13_SQ14_BMTF" cannot fire with this L1 menu (missing: L1_SingleMu13_SQ14_BMTF)
   seed hltL1sSingleMuScouting: 2 of 3 algorithms not in the L1 menu, dropped from the OR: L1_SingleMu5_BMTF L1_SingleMu13_SQ14_BMTF
   seed hltL1sTauVeryBigOR: 3 of 23 algorithms not in the L1 menu, dropped from the OR: L1_DoubleTau_Iso34_Iso26_er2p1_Jet55_RmOvlp_dR0p5 L1_DoubleTau_Iso38_Iso26_er2p1_Jet55_RmOvlp_dR0p5 L1_DoubleTau_Iso40_Iso26_er2p1_J
   seed hltL1sTripleJet957565VBFIorHTTIorDoubleJetCIorSingleJetorQuadJet95756520: 3 of 12 algorithms not in the L1 menu, dropped from the OR: L1_DoubleJet_110_35_DoubleJet35_Mass_Min1000 L1_DoubleJet_110_35_DoubleJet35_M
   path DST_PFScouting_AXOMedium_v1 removed
   path HLT_DoubleEle10_eta1p22_mMax6_v11 removed
   path HLT_DoubleEle6p5_eta1p22_mMax6_v11 removed
   path HLT_DoubleEle8_eta1p22_mMax6_v11 removed
   path HLT_DoublePNetTauhPFJet26_L2NN_eta2p3_PFJet60_v7 removed
   path HLT_DoublePNetTauhPFJet26_L2NN_eta2p3_PFJet75_v7 removed
   path HLT_DoublePNetTauhPFJet30_Medium_L2NN_eta2p3_v7 removed
   path HLT_DoublePNetTauhPFJet30_Tight_L2NN_eta2p3_v7 removed
   path HLT_IsoMu24_eta2p1_L1HT200_QuadPFJet25_PNet1Tauh0p50_v1 removed
   path HLT_IsoMu24_eta2p1_PNetTauhPFJet130_Loose_L2NN_eta2p3_CrossL1_v7 removed
   path HLT_IsoMu24_eta2p1_PNetTauhPFJet130_Medium_L2NN_eta2p3_CrossL1_v7 removed
   path HLT_IsoMu24_eta2p1_PNetTauhPFJet130_Tight_L2NN_eta2p3_CrossL1_v7 removed
   path HLT_IsoMu24_eta2p1_PNetTauhPFJet26_L2NN_eta2p3_CrossL1_PFJet60_v7 removed
   path HLT_IsoMu24_eta2p1_PNetTauhPFJet26_L2NN_eta2p3_CrossL1_PFJet75_v7 removed
   path HLT_IsoMu24_eta2p1_PNetTauhPFJet26_L2NN_eta2p3_CrossL1_v7 removed
   path HLT_IsoMu24_eta2p1_PNetTauhPFJet30_Medium_L2NN_eta2p3_CrossL1_v7 removed
   path HLT_IsoMu24_eta2p1_PNetTauhPFJet30_Tight_L2NN_eta2p3_CrossL1_v7 removed
   path HLT_IsoMu24_eta2p1_PNetTauhPFJet45_L2NN_eta2p3_CrossL1_v7 removed
   path HLT_L1AXOVVTight_v1 removed
   path HLT_L1AXOVVVTight_v1 removed
   path HLT_L1CICADA_VVTight_v1 removed
   path HLT_L1CICADA_VVVTight_v1 removed
   path HLT_L1CICADA_VVVVTight_v1 removed
   path HLT_L1HT200_QuadPFJet20_PNet1BTag0p50_PNet1Tauh0p50_v1 removed
   path HLT_L1HT200_QuadPFJet25_PNet1BTag0p50_PNet1Tauh0p50_v1 removed
   path HLT_Mu0_Barrel_L1HP13_v1 removed
   path HLT_Mu12_Barrel_L1HP13_IP4_v1 removed
   path HLT_Mu12_Barrel_L1HP13_IP6_v1 removed
   path HLT_Mu4_Barrel_IP4_v1 removed
   path HLT_Mu4_Barrel_IP6_v1 removed
   path HLT_SingleEle8_v10 removed
   path HLT_SinglePNetTauhPFJet130_Loose_L2NN_eta2p3_v7 removed
   path HLT_SinglePNetTauhPFJet130_Medium_L2NN_eta2p3_v7 removed
   path HLT_SinglePNetTauhPFJet130_Tight_L2NN_eta2p3_v7 removed
   path HLT_VBF_DiPFJet45_Mjj650_PNetTauhPFJet45_L2NN_eta2p3_v7 removed
   path HLT_VBF_DiPFJet45_Mjj750_PNetTauhPFJet45_L2NN_eta2p3_v7 removed
   path HLT_VBF_DiPFJet45_Mjj850_PNetTauhPFJet45_L2NN_eta2p3_v1 removed
   dataset filter hltDatasetEGamma: 4 paths removed from its conditions
   dataset filter hltDatasetMuon: 10 paths removed from its conditions
   dataset filter hltDatasetOnlineMonitor: 6 paths removed from its conditions
   dataset filter hltDatasetParkingAnomalyDetection: 5 paths removed from its conditions
   dataset filter hltDatasetParkingHH: 2 paths removed from its conditions
   dataset filter hltDatasetParkingSingleMuon: 5 paths removed from its conditions
   dataset filter hltDatasetParkingVBF: 3 paths removed from its conditions
   dataset filter hltDatasetScoutingPFMonitor: 1 paths removed from its conditions
   dataset filter hltDatasetScoutingPFRun3: 1 paths removed from its conditions
   dataset filter hltDatasetTau: 7 paths removed from its conditions
prescales: no PrescaleService: every HLTPrescaler has prescale 1
HLT_PFJet110: datasets JetMET0 JetMET1 OnlineMonitor; streams DQM PhysicsJetMET0 PhysicsJetMET1
HLT_PFJetFwd140: datasets JetMET0 JetMET1 OnlineMonitor; streams DQM PhysicsJetMET0 PhysicsJetMET1
HLT_DiPFJetAve100_HFJEC: datasets JetMET0 JetMET1 OnlineMonitor; streams DQM PhysicsJetMET0 PhysicsJetMET1
HLT_PFJet40: datasets JetMET0 JetMET1 OnlineMonitor; streams DQM PhysicsJetMET0 PhysicsJetMET1

```

The anatomy of the path of the book (`out/menu_13_HLT.txt`, section 2; a module met again in a later sequence runs once,
so the 209 entries of the path, `nModules=209` in `hlt_13_HLT.log`, are 174 distinct modules):

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

`edmDumpEventContent file:$J/test/13_HLT/out/step5_HLT_13_HLT.root | grep '"HLT"'` lists 2977 products of the process
`HLT` (21.2 MB in all; the input is 12.3 MB). The ones the chapter uses, by step:

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
  globalTag=auto:phase1_2024_realistic era=Run3_2024 geometry=DB:Extended pdfPrefix=hlt_13_HLT savePNG=1 \
  sampleLabel="chapter 13: the High Level Trigger|bbbar, pTHat > 100 GeV, 13.6 TeV|with PU = 60 per bx, bx -1..1" \
  > hlt_13_HLT.log 2>&1                                        # about 30 s
```

Options: `processName=HLT`, `jetPtMin=30` (the ak4GenJets drawn and followed), `dzMax=0.1` (a track belongs to a vertex
within this distance in z, cm), `zMax=256` (the top of the colour scale of the tower maps). The job needs the global tag
(the jet energy corrections `AK4CaloHLT`, `AK4PFHLT`, evaluated level by level with `FactorizedJetCorrector`) and the
geometry (the positions of the calorimeter cells for the rec-hit E_T). `HLTPathAnalyzer` reads the configuration of the
HLT process from the file (`HLTConfigProvider`): the paths, their modules, their seeds, the datasets and streams.

| analyzer | steps | PDF and PNG pages (`hlt_13_HLT_...`) | log lines |
|---|---|---|---|
| `HLTPathAnalyzer` | 1, 5 | `_seed.pdf`; `_seed_seed` (the L1 objects unpacked by the HLT, the objects of `hltL1sSingleJet90`, the raw-data sizes, the fate of the paths), `_seed_ladders` (the thresholds of the jet paths and the stage that stopped each), `_seed_objects` (the objects of the accepted paths from `hltTriggerSummaryAOD`: the display of B4) | `menu:`, `menu modules:`, `rawdata`, `L1 unpacked:`, `L1Jet:`, `paths HLT_+DST_:`, `path:` (every path: accept, stage, seed, the last module and its type), `dataset:`, `stream:`, `summary:`, `filter:` (every filter with its objects), `cut:`, `leading jets:`, `ladder`, `seedObjects:`, `accepted:` |
| `HLTCaloAnalyzer` | 2 | `_calo.pdf`; `_calo_towers` (the towers on the trigger-tower grid with the corrected calo jets), `_calo_jet1`, `_calo_jet2` (17 x 17 towers around the two signal jets, E_T in each, the 9 x 9 window of the L1 jet, the corrections level by level) | `rechits <region>:`, `towers <region>:`, `rho calo`, `jec calo:` (the formulas of the payload), `GenJet:`, `calo jets:`, `CaloJet N:` (raw, factors, recomputed and stored corrected pT, gen match), `zoom gen jet` |
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
menu: table=/dev/CMSSW_15_0_0/GRun/V62 process=HLT globalTag=150X_mcRun3_2024_realistic_v2 paths=862 datasets=99 streams=68 prescaleColumns=0 | AlCa=11 DQM=3 DST=20 Dataset=99 HLT=697 HLTriggerFinalPath=1 HLTriggerFirstPath=1 MC=26 ScoutingPFOutput=1 Status=2 digi2raw=1
menu modules: distinct=4598 =7 EDAnalyzer=1 EDFilter=3294 EDProducer=1296
```

### Step 1 (`ledger_13_HLT.txt`, section 1)

```
-- 1. Step 1: the raw event, the Level-1 record, the seeds and the prescales
raw event: 678 FEDs, 1928.6 kB
   strip               381 FEDs    1001.8 kB  51.9 %
   pixel               108 FEDs     316.0 kB  16.4 %
   HCAL                 44 FEDs     225.4 kB  11.7 %
   ECAL                 54 FEDs     173.3 kB   9.0 %
   preshower            40 FEDs     136.9 kB   7.1 %
   CSC                  36 FEDs      46.9 kB   2.4 %
   Level-1 trigger       7 FEDs      27.1 kB   1.4 %
   GEM                   2 FEDs       0.9 kB   0.0 %
   DT                    3 FEDs       0.3 kB   0.0 %
   RPC                   3 FEDs       0.2 kB   0.0 %
L1 unpacked: finalOR=1 algorithmsFired=57 jets=12 egammas=12 taus=12 muons=1
paths HLT_ and DST_: 717; stopped by the L1 seed 593; by the prescale 0; start their reconstruction 124 (of which 83 stopped by a filter, 41 accept)
seedObjects: label=hltL1sSingleJet90 expression="L1_SingleJet90" n=2 | L1Jet(137.0,-3.74,1.00) L1Jet(106.5,-1.52,-1.98)
seedObjects: label=hltL1sZeroBias expression="L1_ZeroBias" n=0 |
seedObjects: label=hltL1sSingleJet120 expression="L1_SingleJet120" n=1 | L1Jet(137.0,-3.74,1.00)
seedObjects: label=hltL1sSingleJet90Fwd expression="L1_SingleJet90_FWD2p5" n=1 | L1Jet(137.0,-3.74,1.00)
```

The unpacked Level-1 jets and the seeds of the jet paths (`hlt_13_HLT.log`):

```
  L1Jet: et=137 eta=-3.741 phi=1.0005
  L1Jet: et=106.5 eta=-1.5225 phi=-1.97669
  L1Jet: et=75.5 eta=-2.7405 phi=1.0005
  L1Jet: et=56 eta=2.7405 phi=-0.0626853
  L1Jet: et=36.5 eta=-2.7405 phi=-0.584686
  L1Jet: et=22.5 eta=2.7405 phi=-3.02069
  L1Jet: et=18 eta=0.9135 phi=1.6965
  L1Jet: et=17 eta=0.9135 phi=-2.49869
  L1Jet: et=15.5 eta=1.5225 phi=0.9135
  L1Jet: et=14 eta=-0.3045 phi=0.3045
  L1Jet: et=13.5 eta=-2.7405 phi=2.2185
  L1Jet: et=12.5 eta=2.7405 phi=1.0005
  seedObjects: label=hltL1sSingleJet90 expression="L1_SingleJet90" n=2 | L1Jet(137.0,-3.74,1.00) L1Jet(106.5,-1.52,-1.98)
  seedObjects: label=hltL1sZeroBias expression="L1_ZeroBias" n=0 |
  seedObjects: label=hltL1sSingleJet120 expression="L1_SingleJet120" n=1 | L1Jet(137.0,-3.74,1.00)
  seedObjects: label=hltL1sSingleJet90Fwd expression="L1_SingleJet90_FWD2p5" n=1 | L1Jet(137.0,-3.74,1.00)
```

### Step 2 (`ledger_13_HLT.txt`, section 2)

```
-- 2. Step 2: calorimeter rec-hits, towers, calo jets
rec-hits          n      sum E     sum ET
EB             2039      384.9      268.2
EE             4141     2866.7      498.6
HB             1882      476.9      344.9
HE             3271     3282.0      592.3
HF             1728    50755.8     1603.5
towers            n     sum ET         EM        HAD
barrel          844      444.3      235.3      208.9
endcap          807     1028.2      467.1      561.1
forward         790     1613.0      597.2     1015.8
all            2441     3085.5     1299.7     1785.8
rho calo (hltFixedGridRhoFastjetAllCalo): 40.026 GeV per unit area
calo jets: raw=187 corrected=187 corrected>10=29 >30=2 >80=2
jec calo: level=L1FastJet records=82 formula="max(0.0001,1-(z/y)*([0]+([1]*(x))*(1+[2]*log(y))))" binVar=JetEta, parVar=Rho,JetPt,JetA,
jec calo: level=L2Relative records=82 formula="max(0.0001,[0]+[1]/(pow(log10(x),2)+[2])+[3]*exp(-[4]*(log10(x)-[5])*(log10(x)-[5])))" binVar=JetEta, parVar=JetPt,
jec calo: level=L3Absolute records=1 formula="1" binVar=JetEta, parVar=JetPt,
jec calo: level=L2L3Residual records=1 formula="1" binVar=JetEta, parVar=JetPt,
CaloJet 0: rawPt=162.3 eta=-3.73 phi=0.96 area=0.503 towers=17 n90=9 emf=0.26 | L1FastJet=0.7049 L2Relative=0.6988 L3Absolute=0.6988 L2L3Residual=0.6988 jecBin=eta[-3.839,-3.664] | recomputedPt=113.4 correctedPt=113.4 | gen=115.9 dR=0.05
CaloJet 1: rawPt=68.4 eta=-1.57 phi=-2.09 area=0.503 towers=35 n90=21 emf=0.54 | L1FastJet=0.7827 L2Relative=1.2935 L3Absolute=1.2935 L2L3Residual=1.2935 jecBin=eta[-1.653,-1.566] | recomputedPt=88.5 correctedPt=88.5 | gen=117.8 dR=0.02
CaloJet 2: rawPt=41.8 eta=2.98 phi=2.06 area=0.503 towers=18 n90=14 emf=0.44 | L1FastJet=0.2597 L2Relative=0.4560 L3Absolute=0.4560 L2L3Residual=0.4560 jecBin=eta[2.964,3.139] | recomputedPt=19.1 correctedPt=19.1 | gen=- dR=-
CaloJet 3: rawPt=40.8 eta=-2.78 phi=0.89 area=0.503 towers=17 n90=14 emf=0.50 | L1FastJet=0.2297 L2Relative=0.4956 L3Absolute=0.4956 L2L3Residual=0.4956 jecBin=eta[-2.853,-2.65] | recomputedPt=20.2 correctedPt=20.2 | gen=- dR=-
CaloJet 4: rawPt=39.9 eta=2.74 phi=0.46 area=0.503 towers=19 n90=15 emf=0.46 | L1FastJet=0.2172 L2Relative=0.4912 L3Absolute=0.4912 L2L3Residual=0.4912 jecBin=eta[2.65,2.853] | recomputedPt=19.6 correctedPt=19.6 | gen=- dR=-
CaloJet 6: rawPt=39.0 eta=-2.74 phi=-0.71 area=0.503 towers=17 n90=13 emf=0.44 | L1FastJet=0.2044 L2Relative=0.4630 L3Absolute=0.4630 L2L3Residual=0.4630 jecBin=eta[-2.853,-2.65] | recomputedPt=18.1 correctedPt=18.1 | gen=- dR=-
CaloJet 10: rawPt=36.9 eta=2.98 phi=-0.27 area=0.457 towers=19 n90=15 emf=0.34 | L1FastJet=0.2459 L2Relative=0.4451 L3Absolute=0.4451 L2L3Residual=0.4451 jecBin=eta[2.964,3.139] | recomputedPt=16.4 correctedPt=16.4 | gen=- dR=-
CaloJet 11: rawPt=36.5 eta=-2.81 phi=-1.37 area=0.463 towers=17 n90=14 emf=0.55 | L1FastJet=0.2305 L2Relative=0.5139 L3Absolute=0.5139 L2L3Residual=0.5139 jecBin=eta[-2.853,-2.65] | recomputedPt=18.7 correctedPt=18.7 | gen=- dR=-
CaloJet 22: rawPt=30.9 eta=2.35 phi=-2.35 area=0.441 towers=20 n90=16 emf=0.45 | L1FastJet=0.3030 L2Relative=0.5949 L3Absolute=0.5949 L2L3Residual=0.5949 jecBin=eta[2.322,2.5] | recomputedPt=18.4 correctedPt=18.4 | gen=- dR=-
CaloJet 36: rawPt=26.3 eta=-2.26 phi=0.14 area=0.415 towers=18 n90=13 emf=0.43 | L1FastJet=0.3018 L2Relative=0.6639 L3Absolute=0.6639 L2L3Residual=0.6639 jecBin=eta[-2.322,-2.172] | recomputedPt=17.4 correctedPt=17.4 | gen=- dR=-
CaloJet 42: rawPt=24.6 eta=-2.13 phi=-1.77 area=0.433 towers=15 n90=12 emf=0.33 | L1FastJet=0.2907 L2Relative=0.6939 L3Absolute=0.6939 L2L3Residual=0.6939 jecBin=eta[-2.172,-2.043] | recomputedPt=17.1 correctedPt=17.1 | gen=- dR=-
CaloJet 53: rawPt=21.2 eta=-0.32 phi=0.36 area=0.503 towers=21 n90=16 emf=0.65 | L1FastJet=0.5686 L2Relative=1.1526 L3Absolute=1.1526 L2L3Residual=1.1526 jecBin=eta[-0.348,-0.261] | recomputedPt=24.4 correctedPt=24.4 | gen=- dR=-
CaloJet 55: rawPt=21.0 eta=-2.31 phi=-0.42 area=0.292 towers=12 n90=10 emf=0.53 | L1FastJet=0.4180 L2Relative=0.8990 L3Absolute=0.8990 L2L3Residual=0.8990 jecBin=eta[-2.322,-2.172] | recomputedPt=18.9 correctedPt=18.9 | gen=- dR=-
CaloJet 56: rawPt=20.6 eta=-0.85 phi=-2.88 area=0.503 towers=23 n90=17 emf=0.66 | L1FastJet=0.4945 L2Relative=1.0618 L3Absolute=1.0618 L2L3Residual=1.0618 jecBin=eta[-0.879,-0.783] | recomputedPt=21.9 correctedPt=21.9 | gen=- dR=-
CaloJet 59: rawPt=18.4 eta=1.06 phi=2.75 area=0.503 towers=19 n90=14 emf=0.19 | L1FastJet=0.3806 L2Relative=0.9565 L3Absolute=0.9565 L2L3Residual=0.9565 jecBin=eta[1.044,1.131] | recomputedPt=17.6 correctedPt=17.6 | gen=- dR=-
CaloJet 64: rawPt=17.9 eta=1.02 phi=1.75 area=0.503 towers=18 n90=13 emf=0.37 | L1FastJet=0.3844 L2Relative=0.9379 L3Absolute=0.9379 L2L3Residual=0.9379 jecBin=eta[0.957,1.044] | recomputedPt=16.8 correctedPt=16.8 | gen=- dR=-
CaloJet 97: rawPt=11.2 eta=1.16 phi=2.25 area=0.265 towers=9 n90=7 emf=0.65 | L1FastJet=0.5184 L2Relative=1.3652 L3Absolute=1.3652 L2L3Residual=1.3652 jecBin=eta[1.131,1.218] | recomputedPt=15.2 correctedPt=15.2 | gen=- dR=-
zoom gen jet 1: pt=117.8 centre ieta=-19 iphi=49 towersEt(dR<0.4 of the jet axis)=71.7 towersEt(9x9)=80.3
zoom gen jet 2: pt=115.9 centre ieta=-34 iphi=12 towersEt(dR<0.4 of the jet axis)=169.9 towersEt(9x9)=215.6
```

The arithmetic of the chapter: b jet 68.4 x 0.7827 = 53.5, x 1.2935 / 0.7827 = 1.653 -> 88.5; bbar jet 162.3 x 0.7049 =
114.4, x 0.6988 / 0.7049 = 0.991 -> 113.4; pileup jet 41.8 x 0.2597 = 10.9, x 1.756 -> 19.1. 26 raw calo jets above
30 GeV (`CaloJet` lines with `rawPt` > 30), 2 corrected. rho x area: 40.0 x 0.503 = 20 GeV. The 9 x 9 windows of the L1
jets of chapter 12: 71.0 (b) and 190.5 GeV (bbar) (`ledger_12_L1T.txt`, section 3).

### Step 3 (`ledger_13_HLT.txt`, section 3)

```
-- 3. Step 3: tracking and vertices
clusters: pixel=20699 (modules 1752) strip=115932 (modules 13919)
tracks pixel: n=2978 pt>1=1761 pt>10=166 sumPt=38313.1
tracks iter0: n=110 pt>1=44 pt>10=2 sumPt=158.4
tracks doubletRecovery: n=5 pt>1=2 pt>10=0 sumPt=5.4
tracks merged: n=115 pt>1=46 pt>10=2 sumPt=163.7
tracks merged: highPurity=115 meanPixelHits=4.16 meanStripHits=9.44
truth: signal vertex x=0.0277 y=-0.0169 z=3.1119 | in-time pileup collisions=68 (mean 68)
truth: pileup collisions within 1 mm of the signal in z: 0 | z range -9.52 .. 6.53
vertices pixel: n=27
vertices trimmed (seeds of iteration 0): n=1 | z=3.106(signal)
vertices PF: n=1
vertex PF 0: z=3.1114 ndof=103.1 tracks=104 near=63 sumPt=104.8 sumPt2=487.3 sim=signal dzSim=0.0005
signal vertex rank: pixel=0 PF=0 (0 = the first, the primary vertex)
jet tracks: gen jet 1 pt=117.8 eta=-1.59 chargedGenPt=32.6 | tracks dR<0.4: n=5 sumPt=34.0 | from the signal vertex (|dz|<0.1 cm): n=4 sumPt=33.1
jet tracks: gen jet 2 pt=115.9 eta=-3.75 chargedGenPt=44.0 | tracks dR<0.4: n=0 sumPt=0.0 | from the signal vertex (|dz|<0.1 cm): n=0 sumPt=0.0
tracks from the signal vertex (|dz| < 0.1 cm of z=3.111): n=63 sumPt=104.8
vertex pixel 0: z=3.1060 ndof=37.0 tracks=38 near=63 sumPt=104.8 sumPt2=487.3 sim=signal dzSim=0.0060
vertex pixel 1: z=-0.3332 ndof=26.0 tracks=27 near=0 sumPt=0.0 sumPt2=0.0 sim=pu9 dzSim=0.0052
vertex pixel 2: z=-5.4505 ndof=35.0 tracks=36 near=0 sumPt=0.0 sumPt2=0.0 sim=pu30 dzSim=0.0038
vertex pixel 3: z=2.0841 ndof=18.0 tracks=19 near=0 sumPt=0.0 sumPt2=0.0 sim=pu46 dzSim=0.0014
vertex pixel 4: z=0.3606 ndof=27.0 tracks=28 near=0 sumPt=0.0 sumPt2=0.0 sim=pu35 dzSim=0.0002
vertex pixel 5: z=1.2489 ndof=18.0 tracks=19 near=0 sumPt=0.0 sumPt2=0.0 sim=pu31 dzSim=0.0015
vertex pixel 6: z=-2.0501 ndof=17.0 tracks=18 near=0 sumPt=0.0 sumPt2=0.0 sim=pu38 dzSim=0.0004
vertex pixel 7: z=2.8838 ndof=25.0 tracks=26 near=27 sumPt=34.1 sumPt2=59.9 sim=pu34 dzSim=0.0047
```

The pixel vertices in full (`hlt_13_HLT.log`):

```
  vertex pixel 0: z=3.1060 ndof=37.0 tracks=38 near=63 sumPt=104.8 sumPt2=487.3 sim=signal dzSim=0.0060
  vertex pixel 1: z=-0.3332 ndof=26.0 tracks=27 near=0 sumPt=0.0 sumPt2=0.0 sim=pu9 dzSim=0.0052
  vertex pixel 2: z=-5.4505 ndof=35.0 tracks=36 near=0 sumPt=0.0 sumPt2=0.0 sim=pu30 dzSim=0.0038
  vertex pixel 3: z=2.0841 ndof=18.0 tracks=19 near=0 sumPt=0.0 sumPt2=0.0 sim=pu46 dzSim=0.0014
  vertex pixel 4: z=0.3606 ndof=27.0 tracks=28 near=0 sumPt=0.0 sumPt2=0.0 sim=pu35 dzSim=0.0002
  vertex pixel 5: z=1.2489 ndof=18.0 tracks=19 near=0 sumPt=0.0 sumPt2=0.0 sim=pu31 dzSim=0.0015
  vertex pixel 6: z=-2.0501 ndof=17.0 tracks=18 near=0 sumPt=0.0 sumPt2=0.0 sim=pu38 dzSim=0.0004
  vertex pixel 7: z=2.8838 ndof=25.0 tracks=26 near=27 sumPt=34.1 sumPt2=59.9 sim=pu34 dzSim=0.0047
  vertex pixel 8: z=0.2745 ndof=17.0 tracks=18 near=0 sumPt=0.0 sumPt2=0.0 sim=pu37 dzSim=0.0001
  vertex pixel 9: z=-2.8358 ndof=22.0 tracks=23 near=0 sumPt=0.0 sumPt2=0.0 sim=pu50 dzSim=0.0184
  vertex pixel 10: z=5.5124 ndof=10.0 tracks=11 near=0 sumPt=0.0 sumPt2=0.0 sim=pu20 dzSim=0.0022
  vertex pixel 11: z=-0.0668 ndof=26.0 tracks=27 near=0 sumPt=0.0 sumPt2=0.0 sim=pu28 dzSim=0.0086
  vertex pixel 12: z=1.4895 ndof=23.0 tracks=24 near=0 sumPt=0.0 sumPt2=0.0 sim=pu10 dzSim=0.0030
  vertex pixel 13: z=2.5022 ndof=18.0 tracks=19 near=0 sumPt=0.0 sumPt2=0.0 sim=pu22 dzSim=0.0007
  vertex pixel 14: z=3.4953 ndof=7.0 tracks=8 near=1 sumPt=1.1 sumPt2=1.2 sim=pu23 dzSim=0.0023
```

### Step 4 (`ledger_13_HLT.txt`, section 4)

```
-- 4. Step 4: particle flow and PF jets
pfcands all: n=1421 sumPt=2728.4
pfcands h: n=115 sumPt=163.9
pfcands e: n=0 sumPt=0.0
pfcands mu: n=1 sumPt=3.1
pfcands gamma: n=276 sumPt=212.9
pfcands h0: n=224 sumPt=779.3
pfcands h_HF: n=703 sumPt=1482.1
pfcands egamma_HF: n=102 sumPt=87.1
pfcands region tracker: n=568 sumPt=855.8 charged n=108 sumPt=158.8
pfcands region outside: n=68 sumPt=312.2 charged n=8 sumPt=8.2
pfcands region HF: n=785 sumPt=1560.3 charged n=0 sumPt=0.0
rho (hltFixedGridRhoFastjetAll): 36.405 GeV per unit area
pf jets: raw=194 corrected=194 corrected>10=36 >30=2 >30 with a gen jet=2
jec pf: level=L1FastJet records=118 formula="max(0.0001,1-(z/y)*([0]+([1]*(x))*(1+[2]*log(y))))" binVar=JetEta,JetPhi, parVar=Rho,JetPt,JetA, JetPhiBins(eta -5.191..-4.889)=-3.15..3.15, etaBinsWithSeveralPhiBins=36 | split bins: [-1.566,-1.479]x[-3.15,-1.2] [-1.566,-1.479]x[-1.2,-0.8] [-1.566,-1.479]x[-0.8,3.15] [-1.479,-1.392]x[-3.15,-1.2] [-1.479,-1.392]x[-1.2,-0.8] [-1.479,-1.392]x[-0.8,3.15] [-1.392,-1.305]x[-3.1
jec pf: level=L2Relative records=118 formula="max(0.0001,[0]+([1]/(pow(log10(x),2)+[2]))+([3]*exp(-([4]*((log10(x)-[5])*(log10(x)-[5])))))+([6]*exp(-([7]*((log10(x)-[8])*(log10(x)-[8]))))))" binVar=JetEta,JetPhi, parVar=JetPt, JetPhiBins(eta -5.191..-4.889)=-3.15..3.15, etaBinsWithSeveralPhiBins=36 | split bins: [-1.566,-1.479]x[-3.15,-1.2] [-1.566,-1.479]x[-1.2,-0.8] [-1.566,-1.479]x[-0.8,3.15] [-1.479,-1.392]x[-3.1
jec pf: level=L3Absolute records=1 formula="1" binVar=JetEta, parVar=JetPt,
jec pf: level=L2L3Residual records=1 formula="1" binVar=JetEta, parVar=JetPt,
PFJet 0: pt=125.9 rawPt=155.6 eta=-3.77 phi=0.96 area=0.503 n=17 chf=0.00 nhf=1.00 phf=0.00 elf=0.00 muf=0.00 hfhf=1.00 hfemf=0.00 | L1FastJet=0.8246 L2Relative=0.8089 L3Absolute=0.8089 L2L3Residual=0.8089 jecBin=eta[-3.839,-3.664]phi[-3.15,3.15] | recomputedPt=125.9 | gen=115.9 dR=0.05
PFJet 1: pt=80.3 rawPt=77.9 eta=-1.58 phi=-2.08 area=0.503 n=31 chf=0.43 nhf=0.11 phf=0.46 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.8613 L2Relative=1.0308 L3Absolute=1.0308 L2L3Residual=1.0308 jecBin=eta[-1.653,-1.566]phi[-3.15,3.15] | recomputedPt=80.3 | gen=117.8 dR=0.01
PFJet 2: pt=29.0 rawPt=32.0 eta=-2.05 phi=-1.76 area=0.412 n=13 chf=0.24 nhf=0.67 phf=0.09 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.6719 L2Relative=0.9062 L3Absolute=0.9062 L2L3Residual=0.9062 jecBin=eta[-2.172,-2.043]phi[-3.15,3.15] | recomputedPt=29.0 | gen=- dR=-
PFJet 3: pt=25.6 rawPt=39.9 eta=3.21 phi=-0.42 area=0.503 n=14 chf=0.00 nhf=0.98 phf=0.00 elf=0.00 muf=0.00 hfhf=0.82 hfemf=0.02 | L1FastJet=0.5276 L2Relative=0.6408 L3Absolute=0.6408 L2L3Residual=0.6408 jecBin=eta[3.139,3.314]phi[-3.15,3.15] | recomputedPt=25.6 | gen=- dR=-
PFJet 4: pt=23.3 rawPt=34.3 eta=-2.94 phi=0.95 area=0.503 n=12 chf=0.00 nhf=0.74 phf=0.00 elf=0.00 muf=0.00 hfhf=0.21 hfemf=0.26 | L1FastJet=0.3692 L2Relative=0.6799 L3Absolute=0.6799 L2L3Residual=0.6799 jecBin=eta[-2.964,-2.853]phi[-3.15,3.15] | recomputedPt=23.3 | gen=- dR=-
PFJet 5: pt=23.1 rawPt=25.8 eta=1.11 phi=2.45 area=0.503 n=13 chf=0.35 nhf=0.35 phf=0.30 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.7001 L2Relative=0.8954 L3Absolute=0.8954 L2L3Residual=0.8954 jecBin=eta[1.044,1.131]phi[-3.15,3.15] | recomputedPt=23.1 | gen=- dR=-
zoom gen jet 1: pt=117.8 eta=-1.59 | PF jet 80.3 (raw 77.9) dR=0.01 | constituents: h=5/34.0 e=0/0.0 mu=0/0.0 gamma=21/35.7 h0=5/9.0 h_HF=0/0.0 egamma_HF=0/0.0
zoom gen jet 2: pt=115.9 eta=-3.75 | PF jet 125.9 (raw 155.6) dR=0.05 | constituents: h=0/0.0 e=0/0.0 mu=0/0.0 gamma=0/0.0 h0=0/0.0 h_HF=17/156.9 egamma_HF=0/0.0
```

The arithmetic of the chapter: b jet 77.9 x 0.8613 = 67.1 (10.8 GeV subtracted), x 1.0308 / 0.8613 = 1.197 -> 80.3;
bbar jet 155.6 x 0.8246 = 128.3 (27.3 GeV subtracted), x 0.8089 / 0.8246 = 0.981 -> 125.9. The PF jets above 15 GeV
(`hlt_13_HLT.log`, `PFJet` lines; table 13.11):

```
  PFJet 0: pt=125.9 rawPt=155.6 eta=-3.77 phi=0.96 area=0.503 n=17 chf=0.00 nhf=1.00 phf=0.00 elf=0.00 muf=0.00 hfhf=1.00 hfemf=0.00 | L1FastJet=0.8246 L2Relative=0.8089 L3Absolute=0.8089 L2L3Residual=0.8089 jecBin=eta[-3.839,-3.664]phi[-3.15,3.15] | recomputedPt=125.9 | gen=115.9 dR=0.05
  PFJet 1: pt=80.3 rawPt=77.9 eta=-1.58 phi=-2.08 area=0.503 n=31 chf=0.43 nhf=0.11 phf=0.46 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.8613 L2Relative=1.0308 L3Absolute=1.0308 L2L3Residual=1.0308 jecBin=eta[-1.653,-1.566]phi[-3.15,3.15] | recomputedPt=80.3 | gen=117.8 dR=0.01
  PFJet 2: pt=29.0 rawPt=32.0 eta=-2.05 phi=-1.76 area=0.412 n=13 chf=0.24 nhf=0.67 phf=0.09 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.6719 L2Relative=0.9062 L3Absolute=0.9062 L2L3Residual=0.9062 jecBin=eta[-2.172,-2.043]phi[-3.15,3.15] | recomputedPt=29.0 | gen=- dR=-
  PFJet 3: pt=25.6 rawPt=39.9 eta=3.21 phi=-0.42 area=0.503 n=14 chf=0.00 nhf=0.98 phf=0.00 elf=0.00 muf=0.00 hfhf=0.82 hfemf=0.02 | L1FastJet=0.5276 L2Relative=0.6408 L3Absolute=0.6408 L2L3Residual=0.6408 jecBin=eta[3.139,3.314]phi[-3.15,3.15] | recomputedPt=25.6 | gen=- dR=-
  PFJet 4: pt=23.3 rawPt=34.3 eta=-2.94 phi=0.95 area=0.503 n=12 chf=0.00 nhf=0.74 phf=0.00 elf=0.00 muf=0.00 hfhf=0.21 hfemf=0.26 | L1FastJet=0.3692 L2Relative=0.6799 L3Absolute=0.6799 L2L3Residual=0.6799 jecBin=eta[-2.964,-2.853]phi[-3.15,3.15] | recomputedPt=23.3 | gen=- dR=-
  PFJet 5: pt=23.1 rawPt=25.8 eta=1.11 phi=2.45 area=0.503 n=13 chf=0.35 nhf=0.35 phf=0.30 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.7001 L2Relative=0.8954 L3Absolute=0.8954 L2L3Residual=0.8954 jecBin=eta[1.044,1.131]phi[-3.15,3.15] | recomputedPt=23.1 | gen=- dR=-
  PFJet 6: pt=18.7 rawPt=35.7 eta=3.07 phi=2.06 area=0.503 n=11 chf=0.00 nhf=0.91 phf=0.00 elf=0.00 muf=0.00 hfhf=0.60 hfemf=0.09 | L1FastJet=0.3469 L2Relative=0.5251 L3Absolute=0.5251 L2L3Residual=0.5251 jecBin=eta[2.964,3.139]phi[-3.15,3.15] | recomputedPt=18.7 | gen=- dR=-
  PFJet 7: pt=18.7 rawPt=31.4 eta=-2.89 phi=-1.21 area=0.503 n=12 chf=0.01 nhf=0.86 phf=0.00 elf=0.00 muf=0.00 hfhf=0.28 hfemf=0.12 | L1FastJet=0.3082 L2Relative=0.5949 L3Absolute=0.5949 L2L3Residual=0.5949 jecBin=eta[-2.964,-2.853]phi[-3.15,3.15] | recomputedPt=18.7 | gen=- dR=-
  PFJet 8: pt=18.6 rawPt=34.9 eta=-4.23 phi=1.26 area=0.398 n=12 chf=0.00 nhf=1.00 phf=0.00 elf=0.00 muf=0.00 hfhf=1.00 hfemf=0.00 | L1FastJet=0.4683 L2Relative=0.5327 L3Absolute=0.5327 L2L3Residual=0.5327 jecBin=eta[-4.363,-4.191]phi[-3.15,3.15] | recomputedPt=18.6 | gen=- dR=-
  PFJet 9: pt=18.6 rawPt=25.9 eta=-2.21 phi=2.90 area=0.503 n=11 chf=0.16 nhf=0.70 phf=0.04 elf=0.00 muf=0.10 hfhf=0.00 hfemf=0.00 | L1FastJet=0.4994 L2Relative=0.7176 L3Absolute=0.7176 L2L3Residual=0.7176 jecBin=eta[-2.322,-2.172]phi[-3.15,3.15] | recomputedPt=18.6 | gen=- dR=-
  PFJet 10: pt=15.6 rawPt=30.1 eta=4.77 phi=1.67 area=0.503 n=11 chf=0.00 nhf=1.00 phf=0.00 elf=0.00 muf=0.00 hfhf=1.00 hfemf=0.00 | L1FastJet=0.4095 L2Relative=0.5170 L3Absolute=0.5170 L2L3Residual=0.5170 jecBin=eta[4.716,4.889]phi[-3.15,3.15] | recomputedPt=15.6 | gen=- dR=-
  PFJet 11: pt=15.4 rawPt=19.0 eta=1.05 phi=1.71 area=0.494 n=12 chf=0.05 nhf=0.69 phf=0.26 elf=0.00 muf=0.00 hfhf=0.00 hfemf=0.00 | L1FastJet=0.6156 L2Relative=0.8108 L3Absolute=0.8108 L2L3Residual=0.8108 jecBin=eta[1.044,1.131]phi[-3.15,3.15] | recomputedPt=15.4 | gen=- dR=-
```

### Step 5 (`ledger_13_HLT.txt`, section 5)

```
-- 5. Step 5: the decision
HLT_ and DST_ paths that accept the event: 41
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
     HLT_Mu3_PFJet40_v29: the muon of a pileup kaon decay (3.1 GeV) and the b jet
     HLT_CaloMET90_NotCleaned_v15: a calorimeter MET of 133.3 GeV (the neutrino has 37.5)
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
  satisfied by scouting (DST_): 8
     DST_ZeroBias_v12
     DST_Physics_v17
     DST_PFScouting_CICADAVLoose_v3
     DST_PFScouting_CICADALoose_v3
     DST_PFScouting_CICADAMedium_v3
     DST_PFScouting_CICADATight_v3
     DST_PFScouting_CICADAVTight_v3
     DST_PFScouting_ZeroBias_v5

the paths whose L1 seed fired (they start their reconstruction), by kind:
   single jets, dijets                        accepted 16, stopped 14  HLTDiPFJetAveEtaFilter 4, HLT1PFJet 4, HLT1CaloJet 4, HLTDiCaloJetAveFilter 2
   multijets                                  accepted  0, stopped  6  HLT1CaloJet 6
   VBF (two jets far apart in eta)            accepted  0, stopped 13  HLTPFJetVBFFilter 6, HLT1CaloJet 4, HLTCaloJetVBFFilter 3
   missing energy, HT                         accepted  1, stopped 29  HLTMhtFilter 16, HLT1PFMET 5, HLTDeDxFilter 3, HLTHtMhtFilter 2, HLTMuonRecHitClusterFilter 2, HLT1CaloMET 1
   muons                                      accepted  1, stopped 12  HLTMuonL2FromL1TPreFilter 9, HLTMuonL3PreFilter 3
   electrons                                  accepted  0, stopped  2  HLTEgammaL1TMatchFilterRegional 1, HLTEgammaGenericFilter 1
   scouting                                   accepted  8, stopped  0
   no object, L1 seed only, calibration       accepted 15, stopped  0
   others (isolated tracks, PPS, monitoring)  accepted  0, stopped  7  TriggerResultsFilter 3, HLTPixelIsolTrackL1TFilter 2, HLTPPSCalFilter 2
   all                                        accepted 41, stopped 83
other accepted paths (AlCa_, MC_, Dataset_, Status_): 73

the jet paths (ladders: threshold(stage), stage = accept, seed, or the filter that stopped the path)
leading jets: pf=125.9 pf2=80.3 pfFwd=125.9 calo=113.4 calo2=88.5
ladder PFJet: value=125.906 thresholds(stage): 40(accept) 60(accept) 80(accept) 110(accept) 140(hltSinglePFJet140) 200(hltSingleCaloJet170) 260(seed) 320(seed) 400(seed) 450(seed) 500(seed) 550(seed)
ladder PFJetFwd: value=125.906 thresholds(stage): 40(accept) 60(accept) 80(accept) 140(hltSinglePFFwdJet140) 200(hltSingleCaloFwdJet170) 260(seed) 320(seed) 400(seed) 450(seed) 500(seed)
ladder AK8PFJet: value=139.414 thresholds(stage): 40(accept) 60(accept) 80(accept) 140(hltSinglePFJet140AK8) 200(hltSingleCaloJet170AK8) 260(seed) 320(seed) 400(seed) 450(seed) 500(seed) 550(seed)
ladder DiPFJetAve: value=103.108 thresholds(stage): 40(accept) 60(accept) 80(accept) 140(hltDiCaloJetAve110) 200(hltDiCaloJetAve170) 260(seed) 320(seed) 400(seed) 500(seed)
ladder DiPFJetAve_HFJEC: value=103.108 thresholds(stage): 60(hltDiPFJetAve60ForHFJEC) 80(hltDiPFJetAve80ForHFJEC) 100(hltDiPFJetAve100ForHFJEC) 160(hltDiPFJetAve160ForHFJEC) 220(seed) 260(seed) 300(seed)
ladder PFHT: value=80.3103 thresholds(stage): 180(seed) 250(seed) 350(seed) 370(seed) 430(seed) 510(seed) 590(seed) 680(seed) 780(seed) 890(seed) 1050(seed)
cut: label=hltSingleAK4CaloJet80 type=HLT1CaloJet MinPt=80 MinEta=-1 MaxEta=5 input=hltAK4CaloJetsCorrectedIDPassed
cut: label=hltSinglePFJet110 type=HLT1PFJet MinPt=110 MinEta=-1 MaxEta=5 input=hltPFJetsCorrectedMatchedToCaloJets80
cut: label=hltSingleCaloJet110 type=HLT1CaloJet MinPt=110 MinEta=-1 MaxEta=5 input=hltAK4CaloJetsCorrectedIDPassed
cut: label=hltSinglePFJet140 type=HLT1PFJet MinPt=140 MinEta=-1 MaxEta=5 input=hltPFJetsCorrectedMatchedToCaloJets110
cut: label=hltSingleCaloJet170 type=HLT1CaloJet MinPt=170 MinEta=-1 MaxEta=5 input=hltAK4CaloJetsCorrectedIDPassed
cut: label=hltSinglePFJet200 type=HLT1PFJet MinPt=200 MinEta=-1 MaxEta=5 input=hltPFJetsCorrectedMatchedToCaloJets170
cut: label=hltSingleCaloFwdJet110 type=HLT1CaloJet MinPt=110 MinEta=2.7 MaxEta=5 input=hltAK4CaloJetsCorrectedIDPassed
cut: label=hltSinglePFFwdJet140 type=HLT1PFJet MinPt=140 MinEta=2.7 MaxEta=5 input=hltPFJetsCorrectedMatchedToCaloFwdJets110
filter: label=hltSingleAK4CaloJet80 type=HLT1CaloJet n=2 | Jet(113.4,-3.73,0.96) Jet(88.5,-1.57,-2.09)
filter: label=hltSinglePFJet110 type=HLT1PFJet n=1 | Jet(125.9,-3.77,0.96)
filter: label=hltSingleCaloJet110 type=HLT1CaloJet n=1 | Jet(113.4,-3.73,0.96)
filter: label=hltSinglePFJet140 type=HLT1PFJet n=0 |
filter: label=hltSinglePFFwdJet140 type=HLT1PFJet n=0 |
filter: label=hltDiPFJetAve100ForHFJEC type=HLTDiPFJetAveEtaFilter n=1 | Jet(125.9,-3.77,0.96)
filter: label=hltDiCaloJetAve110 type=HLTDiCaloJetAveFilter n=0 |
filter: label=hltMET90 type=HLT1CaloMET n=1 | MET(133.3,0.00,-2.42)
filter: label=hltL2fL1sMu3L1f0L2Filtered0 type=HLTMuonL2FromL1TPreFilter n=1 | Muon(3.1,-2.06,3.03)
filter: label=hltL3fL1sMu3L1f0L2f0L3Filtered3 type=HLTMuonL3PreFilter n=1 | Muon(3.1,-2.03,3.03)
filter: label=hltMu3PFJet40MuCleaned type=HLTPFJetCollectionsFilter n=1 | Jet(80.3,-1.58,-2.08)
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
   Muon0                          1 of 119 paths
   Muon1                          1 of 119 paths
   OnlineMonitor                  26 of 560 paths
   ScoutingPFMonitor              6 of 18 paths
   ScoutingPFRun3                 6 of 18 paths
   TestEnablesEcalHcal            2 of 2 paths
   TestEnablesEcalHcalDQM         2 of 2 paths
   ZeroBias                       3 of 8 paths
streams that accept the event:
   32: ALCALowPtJet ALCALumiPixelsCountsExpress ALCALumiPixelsCountsPrompt ALCAPHISYM Calibration DQM DQMCalibration DQMEventDisplay DQMOnlineBeamspot DQMPPSRandom EcalCalibration Express ExpressAlignment NanoDST PhysicsCommissioning PhysicsEmittanceScan0 PhysicsEmittanceScan1 PhysicsEmittanceScan2 PhysicsHLTPhysics0 PhysicsHLTPhysics1 PhysicsHLTPhysics2 PhysicsHLTPhysics3 PhysicsJetMET0 PhysicsJetMET1 PhysicsMuon0 PhysicsMuon1 PhysicsScoutingPFMonitor PhysicsZeroBias0 PhysicsZeroBias1 PhysicsZeroBias2 PhysicsZeroBias3 ScoutingPF
accepted: paths=33 datasets=47 streams=32
```

The margins of the chapter: `HLT_PFJet110` 125.9 - 110 = 15.9 GeV; `HLT_PFJet140` 140 - 125.9 = 14.1; `HLT_AK8PFJet140`
140 - 139.4 = 0.6; the dijet average (125.9 + 80.3) / 2 = 103.1, of the calo jets (113.4 + 88.5) / 2 = 101.0 < 110;
the HF calibration paths need a tag jet with |eta| < 1.4, the b jet is at |eta| = 1.58; Delta phi of the two PF jets
|0.96 - (-2.08)| = 3.04.

### The ledger (`ledger_13_HLT.txt`, section 6)

```
-- 6. The ledger: what the HLT sees, step by step (nothing is added to or removed from the recorded event)
                                                              GeV
step 2: rec-hits, sum ET (ECAL + HCAL + HF)                3307.5
step 2: calorimeter towers, sum ET                         3085.5
step 2: rho of the calorimeter (per unit area)               40.0
step 3: merged tracks, sum pT                               163.7
step 3: tracks of the signal vertex, sum pT                 104.8
step 4: PF candidates, sum pT                              2728.4
step 4: rho of particle flow (per unit area)                 36.4
the two jets of the signal (generator pT; L1 jet; calo jet raw -> corrected; PF jet raw -> corrected):
   gen 117.8: calo 68.4 -> 88.5, PF 77.9 -> 80.3
   gen 115.9: calo 162.3 -> 113.4, PF 155.6 -> 125.9
L1 jets of chapter 12 (unpacked by the HLT): 137 106.5 75.5
```

### What is quoted from earlier chapters

| number in the chapter | value | source |
|---|---|---|
| L1 algorithms fired, `L1_SingleJet90` margins | 57 of 408; 47.0 GeV (bbar, 137.0), 16.5 GeV (b, 106.5) | `test/12_L1T/out/ledger_12_L1T.txt` |
| L1 objects handed to the global trigger | 12 jets, 12 e/gamma, 12 taus, 1 muon | `test/12_L1T/out/l1t_12_L1T.log` |
| L1 trigger primitives | 3411.5 GeV | `test/12_L1T/out/ledger_12_L1T.txt`, section 1 |
| the muon of a pileup kaon decay | K- of 3.27 GeV, mu- of 2.78 GeV | `test/12_L1T/out/truth_12_L1T.txt` |
| collisions in bx -1, 0, +1 | 71, 68, 54 | `test/11_DIGI/out/ledger_11_DIGI.txt` |
| digis of the event | 551 889 | `test/11_DIGI/out/ledger_11_DIGI.txt` |
| generator jets (visible) | 117.8 (79.8) and 115.9 GeV | `test/07_GenJet/out/` (chapter 7) |
| the neutrino, the electron of the B- decay | 37.5 GeV pT; 10.9 GeV | chapter 6 (`test/06_Decay/out/`) |

### The Phase-2 statements (section B5 of the chapter)

Relative to `$CMSSW_RELEASE_BASE/src`; the conditions with `conddb list 150X_mcRun4_realistic_v1` (the global tag of
`auto:phase2_realistic`, `Configuration/AlCa/python/autoCond.py`, line 111).

| statement | where |
|---|---|
| the Phase-2 menu and its size: 54 entries in the schedule, 32 `HLT_`, 16 `L1T_`, 4 `MC_` | `HLTrigger/Configuration/python/HLT_75e33_cff.py`, line 353 on; `Configuration/HLT/python/autoHLT.py`: `'relvalRun4' : '75e33'` |
| the jet paths and their seeds | `HLT_75e33/paths/HLT_AK4PFPuppiJet520_cfi.py`; `HLT_75e33/modules/hltL1SeedsForPuppiJetFilter_cfi.py` (`PathStatusFilter`, `'pSinglePuppiJet230'`); `hltSingleAK4PFPuppiJet520_cfi.py` (MinPt 520); `hltDoublePFPuppiJets128MaxEta2p4_cfi.py` (MinPt 128, MaxEta 2.4, MinN 2); `hltDoublePFPuppiJets128Eta2p4MaxDeta1p6_cfi.py`; `hltPFPuppiHT1070_cfi.py`; `hltPFPuppiMETTypeOne140_cfi.py` |
| the PUPPI jet corrections | `HLT_75e33/modules/hltAK4PFPuppiJetCorrectorL1_cfi.py` (`AK4PFPuppiHLT`, `L1FastJet`, rho from `hltFixedGridRhoFastjetAll`), `..._L2_cfi.py`, `..._L3_cfi.py`; `hltFixedGridRhoFastjetAll_cfi.py` (`hltParticleFlowTmp`); payload `JetCorrectorParametersCollection_Phase2Spring23ticlv4_MC_AK4PFPuppiHLT_v1`, one IOV inserted 2023-09-21 08:50:47; `conddb dump` of payload `1581021745b44b8b690b85a0b5c09713a30d2baf`: L1FastJet and L3Absolute formula `1`, L2Relative in 19 eta bins |
| PUPPI at the HLT | `HLT_75e33/modules/hltPFPuppi_cfi.py`: `PUProxyValue = hltPixelClustersMultiplicity` (11), `usePUProxyValue = True` (66), regions `etaMin (0.0, 2.5)`, `etaMax (2.5, 3.5)`, `MinNeutralPt (0.5105, 0.821)`, slopes `(9.51e-06, 1.902e-05)`, third region 3.5 to 10.0 with `MinNeutralPt 3.656` and `useCharged False` |
| HF calibration and PF hadron calibration | `HLT_75e33/modules/hltParticleFlowTmpBarrel_cfi.py`: `calibHF_use = False` (121), `calibrationsLabel = ''` (122); `HLT_GRun_cff.py`: `calibrationsLabel = "HLT"` (11366); `PFCalibrationRcd` '' -> `PFCalibration_Run3_mc_offline` (Phase-2 GT); 'HLT' -> `PFCalibration_Run3_2024_online_v1` (`150X_mcRun3_2024_realistic_v2`) |
| tracking without vertex trimming | `HLT_75e33/modules/hltInitialStepSeeds_cfi.py`: `InputVertexCollection = ""`, `usePV = False`; the only `PixelVertexCollectionTrimmer`: `hltPhase2L3FromL1TkMuonTrimmedPixelVertices_cfi.py`; Run 3: `HLT_GRun_cff.py` `hltTrimmedPixelVertices` (`fractionSumPt2 = 0.3`) and `hltIter0PFLowPixelSeedsFromPixelTracks` (`InputVertexCollection = hltTrimmedPixelVertices`) |
| vertices without timing | `HLT_75e33/modules/hltOfflinePrimaryVertices_cfi.py`: `useTiming = cms.bool(False)` (22); `hltUnsortedOfflinePrimaryVertices_cfi.py`: `DA_vect`, no `TrackTimesLabel`; `Configuration/Eras/python/Era_Phase2_cff.py`: `phase2_timing`, `phase2_timing_layer` (17, 24) |
| no calo-jet cut in the Phase-2 jet paths | `HLT_75e33/paths/HLT_AK4PFPuppiJet520_cfi.py`; among the modules of all the paths of `75e33` the jet filters are `HLT1PFJet` on PUPPI jets only, there is no `HLT1CaloJet` |
| comments of the menu | `HLT_75e33_cff.py`: 411 ("Removed temporarily until final decision on L1T tau Phase-2"); `HLT_75e33/modules/hltEgammaHoverEL1Seeded_cfi.py`: 17 (`eThresHB`, the HCAL thresholds of the e/gamma H/E: "Run3 thresholds. Will be overwritten with valid aging customisation"); `HLT_75e33/sequences/HLTInitialStepSequence_cfi.py`: 31-32 ("Probably need to move elsewhere in the final setup") |
| the Phase-2 L1 menu | `Configuration/Applications/python/ConfigBuilder.py`: 1597 (`defaultMenuFile = "step1_2024"`) |
