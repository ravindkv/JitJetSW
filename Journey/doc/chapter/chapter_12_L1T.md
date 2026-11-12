# Chapter 12, L1T: technical companion

The Level-1 trigger of the ONE event, in six steps. One sample: the event as CMS sees it, the signal with its pileup
(PU = 60 per bunch crossing, bx -1..1). Chapter 11 (DIGI) is the last stage at which the signal and the pileup exist
apart (Rule H of the Constitution): there is no `signal`, `pileup` or `mixed` in the file names of this chapter, and
what the chapter says about the signal alone is quoted from chapter 11. Script: `test/12_L1T/run_12_L1T.sh`;
everything it produces is in `test/12_L1T/out/`. The chapter text is `doc/chapter/chapter_12_L1T.tex`, a checkpoint
chapter (Rule G of the Constitution: B1, B2, one section per step, B4, B5). All commands below are run from `test/` of
an initialized CMSSW_15_0_5 (`cmsenv`), with `J=$CMSSW_BASE/src/JitJetSW/Journey`.

## 1. The cmsDriver.py commands

The script runs `cmsDriver.py` once through `run_driver` of `test/common/jitjet_common.sh`. The command, copied from
the header of the generated configuration (`test/12_L1T/step4_L1_12_L1T_cfg.py`, "with command line options"; the
cmsDriver output is in `out/step4_L1_12_L1T_cfg.driver.log`):

```bash
cd $J/test
cmsDriver.py step4 --filein file:$J/test/11_DIGI/out/step3_DIGI_mixed_11_DIGI.root \
  --conditions auto:phase1_2025_realistic --era Run3_2025 --geometry DB:Extended \
  -n -1 -s L1 --datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG --fileout file:step4_L1_12_L1T.root \
  --customise_commands "process.load('SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff')\nprocess.FEVTDEBUGoutput.outputCommands.append('keep *_simCaloStage2Layer1Summary_*_*')\nprocess.source.eventsToProcess = cms.untracked.VEventRange('1:1:1')" \
  --python_filename $J/test/12_L1T/step4_L1_12_L1T_cfg.py --no_exec
```

| argument | meaning |
|---|---|
| `step4` | the name of the configuration (free; `step1` was GEN, `step2` SIM, `step3` DIGI) |
| `--filein file:...` | the input (Rule E): the DIGI file of chapter 11 of the mixed event (the signal with its pileup; Rule H of the Constitution). It holds the digis and, made in the same DIGI job, the calorimeter trigger primitives `simEcalTriggerPrimitiveDigis` and `simHcalTriggerPrimitiveDigis` (step 1 of the chapter) |
| `--conditions auto:phase1_2025_realistic` | the global tag of the CMS chapters of the book (Rule I of the Constitution), `150X_mcRun3_2025_realistic_v1` (`Configuration/AlCa/python/autoCond.py`). For the L1 it provides the menu (`L1TUtmTriggerMenuRcd`: `L1Menu_Collisions2025_v1_0_0_xml`, 465 algorithms; the 2024 tag had `L1Menu_Collisions2024_v1_3_0_xml`, 408), the calorimeter-trigger parameters (`L1TCaloParamsRcd`: `L1TCaloParams_static_2024_v0_1_mc`, the same payload as in 2024), the muon track-finder parameters (`L1TMuonBarrelParamsRcd`, `L1TMuonOverlapParamsRcd`, `L1TMuonEndCapParamsRcd`, `L1TMuonEndCapForestRcd`, `L1TMuonGlobalParamsRcd`, all as in 2024) and pass-through prescales (`L1TGlobalPrescalesVetos_passThrough_mc`); list them with `conddb list 150X_mcRun3_2025_realistic_v1 \| grep -E "L1T\|EcalTPG\|HcalTP"`. Of these 34 records 11 differ from the 2024 tag: the menu, 9 of the 19 `EcalTPG*` payloads and `HcalTPChannelParameters`; the last 10 were read by the DIGI job of chapter 11, which made the primitives (section 4, "The 2024 and the 2025 settings") |
| `--era Run3_2025` | `Run3_2024` plus five modifiers (`Configuration/Eras/python/Era_Run3_2025_cff.py`). From 2024: the stage-2 emulator (`stage2L1Trigger`) with its yearly modifiers up to `stage2L1Trigger_2024` (OMTF patterns, Kalman BMTF quality), `run3_GEM` (GEM pads and the Run-3 CSC primitives), `run3_common` (muon showers, HCAL saturation fix). New in 2025 and read by this step: `stage2L1Trigger_2025` sets the CICADA model to `CICADAModel_v2p2p0` (2024: `CICADAModel_v2p1p2`; `L1Trigger/L1TCaloLayer1/python/simCaloStage2Layer1Summary_cfi.py`, lines 54 and 61-64), and `run3_CSC_2025` gives the two CSC trigger-primitive producers the 2025 shower thresholds (`showerPSet_2025`, `L1Trigger/CSCTriggerPrimitives/python/params/showerParams.py`, line 98) under the name `showerParams` (`cscTriggerPrimitiveDigis_cfi.py`, lines 77-80), while the producer reads `showerParam` (`L1Trigger/CSCTriggerPrimitives/src/CSCBaseboard.cc`, line 6): the 2025 thresholds are in the configuration and are not used. `run3_GEM_2025`, `run3_SiPixel_2025` and `ecal_cctiming` act only in the reconstruction |
| `--geometry DB:Extended` | the reconstruction geometry from the database: the muon trigger-primitive emulators and the track finders need the chamber geometry |
| `-n -1` with `eventsToProcess` | all events of the input, restricted to the one event |
| `-s L1` | the step: the sequence `SimL1Emulator` of `Configuration/StandardSequences/python/SimL1Emulator_cff.py` (which imports `L1Trigger/Configuration/python/SimL1Emulator_cff.py`); section 2 |
| `--datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG` | the output tier and content: the input is copied (generator, SIM, digis, truth) and the L1 collections are kept (`keep *_sim*Digis_*_*` of the SIM content and the `L1TriggerFEVTDEBUG` commands of `L1Trigger/Configuration/python/L1Trigger_EventContent_cff.py`), all but the CICADA score (customise line 2) |
| `--fileout file:...` | the output ROOT file (written in `out/`, where `cmsRun` is started) |
| `--customise_commands "..."` | three python lines appended to the configuration, explained below |
| `--python_filename ... --no_exec` | write the configuration and do not run it; the script runs `cmsRun` itself |

The three customise lines.

1. `process.load('SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff')`. Without it the job stops at the first run
   with `Cannot find EventSetup module to produce data of type "HcalTrigTowerGeometry" in record "CaloGeometryRecord"`
   (module `simCaloStage2Layer1Digis`). `-s L1` alone is not a production configuration: in production the L1 runs in
   the same job as the DIGI step (`-s DIGI,L1,DIGI2RAW`), and `Digi_cff` brings, with the HCAL trigger-primitive
   emulator, three event-setup producers that Layer-1 needs: `HcalTrigTowerGeometryESProducer` (which HCAL cells make
   a trigger tower), `CaloTPGTranscoder` (the scale of the 8-bit HCAL primitive, used by Layer-1 to build its lookup
   tables) and `HcalTPGCoderULUT`. The line loads them without running the emulator again.
2. `process.FEVTDEBUGoutput.outputCommands.append('keep *_simCaloStage2Layer1Summary_*_*')`. The anomaly score of
   CICADA (`simCaloStage2Layer1Summary:CICADAScore`, a `BXVector<float>`) is read by the global trigger inside the job
   but is not in the event content; the line keeps it so that `L1TGlobalAnalyzer` can print it.
3. `process.source.eventsToProcess = cms.untracked.VEventRange('1:1:1')`: the event of the book; the DIGI file of
   chapter 11 holds only that event, the line is a safeguard.

## 2. The cmsRun on the python config

```bash
cd $J/test/12_L1T/out
cmsRun ../step4_L1_12_L1T_cfg.py > step4_L1_12_L1T.log 2>&1      # about 30 s, most of it reading the conditions
```

Or the whole chapter: `./12_L1T/run_12_L1T.sh` (68 s with the L1 job on 12 November 2026, under a minute when it is reused;
variables: `REUSE=1` keeps an existing `step4_L1_12_L1T.root`, `REUSE=0` remakes it, `RUN=0` only writes the
configuration, `ANALYZE=0` skips the analyzers, `PNG=0` skips the PNG pages). The input (Rule E, Rule H2) is the mixed
file of chapter 11 only; the `signal` and `pileup` DIGI files of chapter 11 are not read by any later chapter.

### Walk-through of `step4_L1_12_L1T_cfg.py`

- `process = cms.Process('L1', Run3_2025)`: the process name under which the new products are stored.
- `process.load(...)`: `Services_cff`, `pythiapdt_cfi`, the `MessageLogger`, `EventContent_cff`, `mixNoPU_cfi` (loaded
  by cmsDriver for every step, not used), `GeometryRecoDB_cff`, `MagneticField_cff`,
  `Configuration.StandardSequences.SimL1Emulator_cff` (the emulator), `EndOfProcess_cff`,
  `FrontierConditions_GlobalTag_cff`.
- `process.source`: `PoolSource` on the DIGI file of chapter 11.
- `process.FEVTDEBUGoutput`: the `PoolOutputModule` with the `FEVTDEBUGEventContent` commands.
- `process.L1simulation_step = cms.Path(process.SimL1Emulator)`, then `endjob_step` and the output `EndPath`.
- The three customise lines of section 1.

What `SimL1Emulator` runs (`out/settings_12_L1T.txt`, section 1; the full parameter sets are in its section 2, the
two trigger-primitive emulators of the DIGI job, read from `test/11_DIGI/step3_DIGI_mixed_11_DIGI_cfg.py`, in its
section 3):

```
== 1. The modules of SimL1Emulator (21), in the order of the hardware chain
   simCaloStage2Layer1Digis           L1TCaloLayer1
   simCaloStage2Layer1Summary         L1TCaloSummary_CICADA_vXp1p2
   simCaloStage2Digis                 L1TStage2Layer2Producer
   simDtTriggerPrimitiveDigis         DTTrigProd
   simCscTriggerPrimitiveDigis        CSCTriggerPrimitivesProducer
   simCscTriggerPrimitiveDigisRun3    CSCTriggerPrimitivesProducer
   simMuonGEMPadDigis                 GEMPadDigiProducer
   simMuonGEMPadDigiClusters          GEMPadDigiClusterProducer
   simTwinMuxDigis                    L1TTwinMuxProducer
   simBmtfDigis                       L1TMuonBarrelTrackProducer
   simKBmtfStubs                      L1TMuonBarrelKalmanStubProducer
   simKBmtfDigis                      L1TMuonBarrelKalmanTrackProducer
   simOmtfDigis                       L1TMuonOverlapPhase1TrackProducer
   simEmtfDigis                       L1TMuonEndCapTrackProducer
   simEmtfShowers                     L1TMuonEndCapShowerProducer
   simGmtCaloSumDigis                 L1TMuonCaloSumProducer
   simGmtStage2Digis                  L1TMuonProducer
   simGmtShowerDigis                  L1TMuonShowerProducer
   simGtStage2Digis                   L1TGlobalProducer
   l1tZDCEtSums                       L1TZDCProducer
   simGtExtFakeStage2Digis            L1TExtCondProducer
```

`simBmtfDigis` (the pre-2021 barrel track finder) still runs but the global muon trigger reads `simKBmtfDigis:BMTF`
(`barrelTFInput`, set under `stage2L1Trigger_2021`). `simCscTriggerPrimitiveDigisRun3` is the CSC emulator with the
Run-3 pattern lookup tables switched on; `simEmtfDigis` reads `simCscTriggerPrimitiveDigis:MPCSORTED`.
`simGtExtFakeStage2Digis` makes the external signals (beam pick-ups) all true: this is why `L1_ZeroBias` and the
`L1_Bptx*` algorithms fire on every simulated event, and so do the `_BptxAND` algorithms of the 2025 menu whenever
their object is there. `l1tZDCEtSums` are the ZDC sums for heavy-ion menus. The module list is the one of 2024; the
parameters differ in two places only (`settings_12_L1T.txt` against the 2024 file): `CICADAModelVersion` of
`simCaloStage2Layer1Summary` (`CICADAModel_v2p1p2` -> `CICADAModel_v2p2p0`), and a new `showerParams` set in the two
CSC producers next to the `showerParam` they read (section 1, the `--era` row).

Where the numbers of the trigger are. The python holds almost none of them (`out/settings_12_L1T.txt`); the trigger is
configured from the database, and the analyzers print what they can reach of it into `out/l1t_12_L1T.log`:
`caloParams:` lines (the calorimeter-trigger parameters of `L1TCaloParamsRcd`, with the Layer-1 scale-factor tables),
`menu:` and `condition:` lines (the menu of `L1TUtmTriggerMenuRcd` and the cuts of the conditions that were true).

### The content of the output ROOT file

`edmDumpEventContent file:$J/test/12_L1T/out/step4_L1_12_L1T.root | grep '"L1"'` (12.3 MB: the 12.2 MB of the input
plus about 0.1 MB). The content of the chapter-11 file is copied. New under the process `L1`, in the order of the
steps:

| step | product | what it is |
|---|---|---|
| 1 | (process `DIGI`) `EcalTriggerPrimitiveDigi simEcalTriggerPrimitiveDigis`, `HcalTriggerPrimitiveDigi simHcalTriggerPrimitiveDigis` | the calorimeter trigger primitives: 4032 ECAL and 4824 HCAL (4032 HB/HE + 792 HF) per event |
| 2 | `BXVector<l1t::CaloTower> simCaloStage2Layer1Digis` | the 5760 Layer-1 towers of bx 0: `hwPt` (9 bits, 0.5 GeV), `hwEtRatio`, `hwQual`, and for convenience `hwEtEm`, `hwEtHad` (the calibrated inputs); `vector<L1CaloRegion> simCaloStage2Layer1Digis`: the 4 x 4 tower regions, the input of CICADA |
| 2 | `BXVector<float> simCaloStage2Layer1Summary:CICADAScore` | the anomaly score (kept by the customise line) |
| 3 | `BXVector<l1t::Jet / EGamma / Tau / EtSum> simCaloStage2Digis` | what Layer-2 sends to the global trigger: at most 12 jets, 12 e/gamma, 12 taus, the sums. A jet carries `rawEt` (the 9 x 9 window), `seedEt`, `puEt` (the pileup estimate), `towerIEta`, `towerIPhi`. The same types with the instance `MP` are the objects inside the main processor before the final sort, with `BXVector<l1t::CaloTower>` and `BXVector<l1t::CaloCluster>` |
| 4 | `L1MuDTChambPhContainer`, `L1MuDTChambThContainer simDtTriggerPrimitiveDigis` | DT phi and theta segments |
| 4 | `CSCALCTDigi`, `CSCCLCTDigi`, `CSCCorrelatedLCTDigi simCscTriggerPrimitiveDigis` (also `:MPCSORTED`), `CSCShowerDigi` | CSC anode patterns, cathode patterns, stubs, hadronic-shower flags |
| 4 | `GEMPadDigi simMuonGEMPadDigis`, `GEMPadDigiCluster simMuonGEMPadDigiClusters`, `GEMCoPadDigi simCscTriggerPrimitiveDigis` | GEM pads, pad clusters, coincidence pads |
| 4 | `BXVector<l1t::RegionalMuonCand> simKBmtfDigis:BMTF`, `simOmtfDigis:OMTF`, `simEmtfDigis:EMTF` (and `simBmtfDigis:BMTF`) | the candidates of the track finders (`hwPt`, `hwEta`, `hwPhi` local to the processor, `hwQual`, `processor`) |
| 4 | `vector<l1t::EMTFHit>`, `vector<l1t::EMTFTrack> simEmtfDigis`; `vector<L1MuBMTrack> simBmtfDigis`; `BXVector<L1MuKBMTrack> simKBmtfDigis` | the stubs the EMTF used, with their position, and its tracks (mode, pT, the stubs of each track); the barrel tracks |
| 5 | `BXVector<l1t::Muon> simGmtStage2Digis` | the at most 8 muons sent to the global trigger, with the coordinates at the vertex; the instances `imdMuonsBMTF`, `imdMuonsOMTFNeg/Pos`, `imdMuonsEMTFNeg/Pos` are the intermediate lists per track finder |
| 5 | `BXVector<l1t::MuonShower> simGmtShowerDigis`, `BXVector<l1t::RegionalMuonShower> simEmtfShowers:EMTF` | the hadronic-shower triggers of the CSCs (Run 3) |
| 6 | `BXVector<GlobalAlgBlk> simGtStage2Digis` | the decision: 512 bits before prescale (`getAlgoDecisionInitial`), after prescale, final; `getFinalOR` |
| 6 | `GlobalObjectMapRecord simGtStage2Digis` | per algorithm, the conditions and the object combinations that satisfied them (the emulator only) |

## 3. The analyzer configs

One job runs three analyzers, one after the other (`test/common/l1tAnalyzer_cfg.py`; the plugins are
`plugins/L1TCaloAnalyzer.cc`, `L1TMuonAnalyzer.cc`, `L1TGlobalAnalyzer.cc`, with the tower grid and the page layout
shared in `plugins/L1TDisplay.h`):

```bash
cd $J/test/12_L1T/out
cmsRun $J/test/common/l1tAnalyzer_cfg.py inputFiles=file:step4_L1_12_L1T.root maxEvents=1 eventsToProcess=1:1:1 \
  globalTag=auto:phase1_2025_realistic era=Run3_2025 geometry=DB:Extended pdfPrefix=l1t_12_L1T savePNG=1 \
  sampleLabel="chapter 12: the Level-1 trigger of 2025|bbbar, pTHat > 100 GeV, 13.6 TeV|with PU = 60 per bx, bx -1..1" \
  > l1t_12_L1T.log 2>&1                                        # under a minute
```

Options: `jetPtMin=30` (the ak4GenJets drawn as circles), `genMuonPtMin=1`, `zoomJets=2` (the leading L1 jets drawn
tower by tower), `zMax=256` (the top of the colour scale, GeV). The job needs the conditions (menu, calorimeter-trigger
parameters, HCAL primitive scale: it loads `hcaltpdigi_cff` like the L1 job) and the geometry (DT chambers).

| analyzer | steps | PDF and PNG pages (`l1t_12_L1T_...`) | log lines |
|---|---|---|---|
| `L1TCaloAnalyzer` | 1, 2, 3 | `_calo.pdf`; `_calo_ecaltp`, `_calo_hcaltp` (step 1: the primitives on the 82 x 72 tower grid), `_calo_layer1` (step 2), `_calo_layer2` (step 3: the jet windows on the towers, the jet table), and for each of the two leading jets `_calo_jet<N>_ecaltp`, `_hcaltp`, `_layer1`: the 17 x 17 towers around the seed with the ET written in each, the 9 x 9 window, the towers of the pileup estimate, the arithmetic | `caloParams:`, `TP <region>:`, `Layer1 <region>:`, `ring ieta=`, `Layer1 top tower`, `GenJet:`, `L1Jet N:` (with the window sums and the recomputed pileup estimate), `L1EG`, `L1Tau`, `L1Sums:`, `counts:` |
| `L1TMuonAnalyzer` | 4, 5 | `_muon.pdf`; `_muon_etaphi`, `_muon_rz`: stubs, track-finder candidates, uGMT muons | `DT TP:`, `EMTFHit:`, `TP counts:`, `EMTF input stubs:`, `EMTFTrack:`, `TFCand:`, `TF counts`, `uGMT muon:`, `GenMuon:` |
| `L1TGlobalAnalyzer` | 6 | `_global.pdf`; `_global_objects` (the event as the global trigger receives it), `_global_ladders` (the thresholds of the single-object families and the event) | `uGT inputs`, `CICADA score:`, `menu:`, `decision:`, `fired:` (expression, conditions, the objects behind them), `condition:` (the cuts of the menu), `ladder`, `algo:` (all 465) |

Each analyzer opens and closes its PDF inside its own `analyze()` (ROOT has one global PDF), so the three can share a
job. The `L1Jet` lines check the emulator: the pileup estimate recomputed from the towers (`recomputedPu`) equals the
`puEt` stored in the jet for all nine jets of the event. The `Layer1 top tower` lines print, besides the energies,
the 16-bit tower word as Layer-2 receives it (`tower word: et=... ratio=... flags=...`: 9 bits of ET in counts of
0.5 GeV, 3 bits of E/H ratio, the flag bits), and the `fired:` lines the object map of every algorithm that fired.

The pages (14 PNG; the jet pages are numbered by L1 jet, `jet1` = the leading one, the bbar jet in the HF, `jet2` =
the b jet):

| page | figure of the chapter |
|---|---|
| `l1t_12_L1T_calo_ecaltp`, `_calo_hcaltp` | 12.2: step 1, the primitives on the tower grid |
| `l1t_12_L1T_calo_jet2_ecaltp`, `_jet2_hcaltp` | 12.3: step 1, the b jet tower by tower |
| `l1t_12_L1T_calo_jet1_hcaltp`, `_jet1_layer1` | step 2, the bbar jet in the HF: primitives and Layer-1 towers |
| `l1t_12_L1T_calo_layer1` | step 2, the Layer-1 towers |
| `l1t_12_L1T_calo_jet2_layer1`, `_jet1_layer1` | step 3, the two signal jets as the firmware sees them |
| `l1t_12_L1T_calo_layer2` | step 3, the nine jets on the towers |
| `l1t_12_L1T_muon_etaphi`, `_muon_rz` | steps 4 and 5, the muon trigger |
| `l1t_12_L1T_global_ladders` | step 6, the thresholds of the menu and the event |
| `l1t_12_L1T_global_objects` | the display of B4: the event as the global trigger receives it |
| `l1t_12_L1T_calo_jet1_ecaltp` | not used (the HF has no ECAL) |

Three scripts then read the logs and the files:

```bash
python3 $J/test/common/l1tSettings.py $J/test/12_L1T/step4_L1_12_L1T_cfg.py $J/test/11_DIGI/step3_DIGI_mixed_11_DIGI_cfg.py > settings_12_L1T.txt
python3 $J/test/common/l1tTruth.py . 12_L1T > truth_12_L1T.txt      # FWLite: the TrackingParticles behind every L1 muon
python3 $J/test/common/l1tLedger.py . 12_L1T > ledger_12_L1T.txt    # the ledger, step by step
```

## 4. The tables

Every number of the chapter, by step. Source files in `test/12_L1T/out/`. The numbers about the signal alone are
those of chapter 11 and are listed at the end of this section with their source.

### The event, the menu, the calorimeter-trigger parameters (`ledger_12_L1T.txt`, head)

```
== l1tLedger 12_L1T
event: run:lumi:event 1:1:1
menu L1Menu_Collisions2025_v1_0_0, 465 algorithms, 481 conditions; finalOR=1 fired=63
caloParams: towerLsbSum=0.5 towerLsbE=0.5 towerLsbH=0.5 towerNBitsSum=9 jetLsb=0.5 jetSeedThreshold=4 jetNeighbourThreshold=0 jetBypassPUS=0 jetPUSUsePhiRing=1 jetPUSType=ChunkyDonut jetCalibrationTyp
caloParams etSum (index: etaMin etaMax etThreshold): 0: 1 28 0; 1: 1 26 30; 2: 1 28 0; 3: 1 26 30; 4: 1 28 0;
jet pileup estimate: PhiRing1
```

`jetPUSUsePhiRing=1` selects the phi-ring pileup subtraction although `jetPUSType` still reads `ChunkyDonut`
(`Stage2Layer2JetAlgorithmFirmwareImp1::create`). `etSum` index 0 = ETT, 1 = HTT, 2 = ETM, 3 = HTM, 4 = ETMHF: towers
up to ieta 28, jets up to ieta 26 and above 30 GeV. The `caloParams` lines of the log are identical to those of the
2024 run: the same payload `L1TCaloParams_static_2024_v0_1_mc` in both global tags. The menu line of the log (the
comment of the payload reads "First official L1T menu for the 2025 pp data taking"; the scale set is still that of
May 2024):

```
  menu: name=L1Menu_Collisions2025_v1_0_0 version=0.13 datetime= scaleSet=scales_2024_05_15 nAlgorithms=465 nConditions=481 nModules=0
```

### Steps 1 and 2 (`ledger_12_L1T.txt`, sections 1 and 2)

```
-- 1. Steps 1 and 2: the transverse energy the trigger sees
                                   ET > 0 sum ET [GeV] max ET [GeV]
ECAL primitives, EB                   683        468.0          5.5
ECAL primitives, EE                   494        579.5          7.0
ECAL primitives, all                 1177       1047.5          7.0
HCAL primitives, HB                   233        183.0          7.5
HCAL primitives, HE                   328        241.5         10.0
HCAL primitives, HF                   741       1321.0         90.5
HCAL primitives, all                 1302       1745.5         90.5
Layer-1 towers, barrel                738        576.5          7.5
Layer-1 towers, endcap                557        509.5         14.0
Layer-1 towers, forward              1266       1095.0         45.0
Layer-1 towers, all                  2561       2181.0         45.0
all primitives (of 8856)             2479       2793.0
what Layer-1 changes (sum ET in -> out, difference):
  ECAL (EB + EE)             1047.5 ->    740.5 (-307.0)
  HCAL (HB + HE)              424.5 ->    345.5 (-79.0)
  HF                         1321.0 ->   1095.0 (-226.0)
  all                        2793.0 ->   2181.0 (-612.0)
Layer1 top tower 0: ieta=-34 iphi=11 et=45 ecalTP=0 hcalTP=90.5 calibratedEm=0 calibratedHad=0 | tower word: et=90 ratio=0 flags=5
Layer1 top tower 1: ieta=-34 iphi=12 et=45 ecalTP=0 hcalTP=90.5 calibratedEm=0 calibratedHad=0 | tower word: et=90 ratio=0 flags=5
Layer1 top tower 2: ieta=-18 iphi=50 et=14 ecalTP=3.5 hcalTP=10 calibratedEm=4 calibratedHad=10 | tower word: et=28 ratio=1 flags=0
Layer1 top tower 3: ieta=-19 iphi=48 et=10.5 ecalTP=7 hcalTP=3.5 calibratedEm=7 calibratedHad=3.5 | tower word: et=21 ratio=1 flags=2
Layer1 top tower 4: ieta=-33 iphi=11 et=7.5 ecalTP=0 hcalTP=15.5 calibratedEm=0 calibratedHad=0 | tower word: et=15 ratio=0 flags=5
Layer1 top tower 5: ieta=-33 iphi=12 et=7.5 ecalTP=0 hcalTP=15.5 calibratedEm=0 calibratedHad=0 | tower word: et=15 ratio=0 flags=5
Layer1 top tower 6: ieta=11 iphi=20 et=7.5 ecalTP=0 hcalTP=7.5 calibratedEm=0 calibratedHad=7.5 | tower word: et=15 ratio=0 flags=1
Layer1 top tower 7: ieta=-4 iphi=4 et=6 ecalTP=5.5 hcalTP=0 calibratedEm=6 calibratedHad=0 | tower word: et=12 ratio=0 flags=3
Layer1 top tower 8: ieta=-19 iphi=49 et=6 ecalTP=6 hcalTP=0 calibratedEm=6 calibratedHad=0 | tower word: et=12 ratio=0 flags=3
Layer1 top tower 9: ieta=-33 iphi=13 et=6 ecalTP=0 hcalTP=12 calibratedEm=0 calibratedHad=0 | tower word: et=12 ratio=0 flags=5
```

Derived for the chapter: the barrel (EB + HB) holds 468.0 + 183.0 = 651.0 GeV of primitives; the HF and the endcap
ECAL together 1321.0 + 579.5 = 1900.5 GeV of the 2793.0 (68 %). No tower of tower ring 28 is among the ten largest
Layer-1 towers (in 2024 seven of them were in rings 27 and 28). In a
`Layer1 top tower` line `et`, `ecalTP`, `hcalTP`, `calibratedEm`, `calibratedHad` are in GeV; the tower word is in
counts: `et=28` is 14.0 GeV; `flags` is the upper four bits of the word (bit 12 of the word = 1, 13 = 2, 14 = 4,
15 = 8), so `flags=3` is bits 12 and 13 and `flags=5` bits 12 and 14.

The Layer-1 scale factors of the global tag (`l1t_12_L1T.log`; one row per ET bin of the primitive, one column per
tower ring):

```
  caloParams layer1ECalScaleETBins (14): 3 6 9 12 15 20 25 30 35 40 45 55 70 256
  caloParams layer1HCalScaleETBins (14): 1 6 9 12 15 20 25 30 35 40 45 55 70 256
  caloParams layer1HFScaleETBins (13): 6 9 12 15 20 25 30 35 40 45 55 70 256
  caloParams layer1ECalScaleFactors (392 values, columns: |ieta| = 1..28)
    ET < 3: 1.12 1.13 1.13 1.12 1.12 1.12 1.13 1.12 1.13 1.12 1.13 1.13 1.14 1.13 1.13 1.13 1.14 1.26 1.11 1.2 1.21 1.22 1.19 1.2 1.19 0 0 0
    ET < 6: 1.12 1.13 1.13 1.12 1.12 1.12 1.13 1.12 1.13 1.12 1.13 1.13 1.14 1.13 1.13 1.13 1.14 1.26 1.11 1.2 1.21 1.22 1.19 1.2 1.19 1.22 0 0
    ET < 9: 1.08 1.09 1.08 1.08 1.11 1.08 1.09 1.09 1.09 1.09 1.15 1.09 1.1 1.1 1.1 1.1 1.1 1.23 1.07 1.15 1.14 1.16 1.14 1.14 1.15 1.14 1.14 0
    ET < 12: 1.06 1.06 1.06 1.06 1.06 1.06 1.06 1.06 1.07 1.07 1.07 1.07 1.07 1.08 1.07 1.09 1.08 1.17 1.06 1.11 1.1 1.13 1.1 1.1 1.11 1.11 1.11 1.09
    ET < 15: 1.04 1.05 1.04 1.05 1.04 1.05 1.06 1.06 1.05 1.05 1.05 1.06 1.06 1.06 1.06 1.06 1.07 1.15 1.04 1.09 1.09 1.1 1.09 1.09 1.1 1.1 1.1 1.08
    ET < 20: 1.04 1.03 1.04 1.04 1.04 1.04 1.04 1.04 1.04 1.04 1.04 1.04 1.05 1.06 1.04 1.05 1.05 1.13 1.03 1.07 1.08 1.08 1.08 1.07 1.07 1.09 1.08 1.07
    ET < 25: 1.03 1.03 1.03 1.03 1.03 1.03 1.03 1.03 1.03 1.03 1.04 1.04 1.05 1.05 1.05 1.05 1.05 1.12 1.03 1.06 1.06 1.08 1.07 1.07 1.06 1.08 1.07 1.06
    ET < 30: 1.03 1.03 1.03 1.03 1.03 1.03 1.03 1.03 1.03 1.03 1.03 1.04 1.04 1.04 1.04 1.04 1.03 1.1 1.02 1.05 1.06 1.06 1.06 1.06 1.05 1.06 1.06 1.06
    ET < 35: 1.02 1.02 1.02 1.02 1.02 1.03 1.03 1.03 1.03 1.03 1.03 1.03 1.03 1.04 1.03 1.03 1.02 1.07 1.02 1.04 1.04 1.05 1.06 1.05 1.05 1.06 1.06 1.05
    ET < 40: 1.02 1.02 1.02 1.02 1.02 1.02 1.02 1.02 1.02 1.03 1.03 1.03 1.03 1.03 1.03 1.03 1.03 1.09 1.02 1.04 1.05 1.05 1.05 1.05 1.04 1.05 1.06 1.05
    ET < 45: 1.02 1.02 1.02 1.02 1.02 1.02 1.02 1.02 1.02 1.02 1.02 1.02 1.03 1.03 1.03 1.03 1.03 1.08 1.01 1.04 1.04 1.05 1.05 1.04 1.04 1.05 1.06 1.05
    ET < 55: 1.01 1.01 1.01 1.01 1.01 1.01 1.02 1.01 1.02 1.02 1.02 1.02 1.03 1.03 1.03 1.03 1.03 1.06 1.01 1.04 1.04 1.05 1.04 1.03 1.03 1.04 1.05 1.04
    ET < 70: 1.01 1 1.01 1.01 1.01 1.01 1.01 1 1.01 1.02 1.01 1.01 1.02 1.02 1.02 1.02 1.03 1.04 1.01 1.03 1.03 1.03 1.03 1.03 1.03 1.03 1 1.01
    ET < 256: 1.02 1 1 1.02 1 1.01 1.01 1 1 1.02 1.01 1.01 1.02 1.02 1.02 1.02 1.02 1.04 1.01 1.03 1.03 1.03 1.03 1.02 1.02 1.02 1 1.01
  caloParams layer1HCalScaleFactors (392 values, columns: |ieta| = 1..28)
    ET < 1: 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1
    ET < 6: 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
    ... (1 for every higher bin)
  caloParams layer1HFScaleFactors (156 values, columns: |ieta| = 30..41)
    ET < 6: 1 1 1 1 1 1 1 1 1 1 1 1
    ... (1 for every bin)
```

Layer-1 in addition shares an HF primitive between the 2 (|ieta| 30-39) or 4 (40, 41) towers it covers, dividing by 2
or 4 and truncating to 0.5 GeV (`L1TCaloLayer1FetchLUTs.cc`, "Divide by two (since two duplicate towers are sent)").

Ring by ring (the bar chart of the chapter is drawn from this table). Rings 26 to 28 (both sides) receive 445.5 GeV,
321.5 of it from the ECAL, and keep 134.5 GeV; in rings 27 and 28 the Layer-1 sum equals the HCAL sum: no ECAL
primitive of these rings reaches the 6 GeV (27) or 9 GeV (28) below which the scale factor is 0 (the largest EE
primitive of the event is 7.0 GeV). The barrel rings receive 655.0 GeV and keep 576.5 GeV:

```
-- 2. Ring by ring (72 towers in phi): ECAL primitives + HCAL primitives -> Layer-1 towers, sum ET in GeV
 ieta     ecalTP     hcalTP     layer1
  -41        0.0       36.5       24.0
  -40        0.0       38.5       26.0
  -39        0.0       42.5       35.0
  -38        0.0       41.0       34.0
  -37        0.0       50.0       41.0
  -36        0.0       64.5       58.0
  -35        0.0       52.0       39.0
  -34        0.0      148.5      137.0
  -33        0.0       76.5       67.0
  -32        0.0       50.0       41.0
  -31        0.0       48.0       36.0
  -30        0.0       36.5       29.0
  -28       95.0       46.0       46.0
  -27       36.0        9.0        9.0
  -26       41.5       14.0       17.5
  -25       36.5        9.0       46.0
  -24       22.0        7.0       29.0
  -23       16.0        4.0       20.0
  -22       19.0        1.0       20.5
  -21        7.5        5.0       12.5
  -20       15.0        3.0       18.0
  -19       31.5       10.5       42.5
  -18        8.0       17.5       26.5
  -17       14.5        2.5       17.0
  -16        7.0        3.5       10.5
  -15        9.0       13.0       19.5
  -14        8.0        4.0        9.5
  -13       13.5        3.5       16.5
  -12       15.0        5.5       19.5
  -11       13.0        4.0       15.0
  -10       19.5        3.0       20.5
   -9       21.5        3.5       21.5
   -8       10.5        4.0       13.5
   -7       11.5        4.0       11.5
   -6       16.5        9.5       22.0
   -5       14.5        7.0       19.0
   -4       20.0       10.0       27.0
   -3       19.0        8.0       24.0
   -2       12.0        9.0       19.0
   -1       16.0        8.0       19.5
    1       18.5        2.0       20.5
    2       17.0        5.0       17.0
    3       18.0        6.0       18.0
    4       13.0        4.0       14.0
    5       13.5        5.5       16.0
    6       14.0        4.5       15.5
    7       12.5        2.0       13.5
    8       12.5        1.5       12.5
    9        7.0        0.0        7.0
   10       12.5        5.0       15.0
   11       14.0       15.5       27.0
   12       12.0        7.0       15.0
   13       18.0        7.5       24.0
   14       12.0        5.5       14.5
   15       10.0        9.5       15.0
   16       11.0        3.0       14.0
   17       11.5        1.5       13.0
   18        0.0        5.0        5.0
   19        5.0        4.0        9.0
   20        7.0        4.5       11.5
   21        8.0        4.0       12.0
   22        8.5        3.0       11.5
   23       15.5       12.0       27.5
   24       29.5        6.0       36.5
   25       29.0       18.0       47.0
   26       44.0       11.0       18.0
   27       37.0        8.0        8.0
   28       68.0       36.0       36.0
   30        0.0       38.5       28.0
   31        0.0       46.0       38.0
   32        0.0       45.0       36.0
   33        0.0       57.0       51.0
   34        0.0       54.0       46.0
   35        0.0       57.0       48.0
   36        0.0       62.0       55.0
   37        0.0       61.0       53.0
   38        0.0       63.5       54.0
   39        0.0       49.5       41.0
   40        0.0       54.0       42.0
   41        0.0       49.0       36.0
```

### Step 3 (`ledger_12_L1T.txt`, sections 3 and 4)

```
-- 3. Step 3: the Layer-2 jets (GeV). window = sum of the 9 x 9 Layer-1 towers; pileup = the estimate subtracted;
      calib = the calibration factor; ET = (window - pileup) x calib; ecalTP / hcalTP = the primitives in the window
9 L1 jets; GenJets: pt 117.8 eta -1.59 phi -2.08, pt 115.9 eta -3.75 phi 1.01
       ET    eta    phi  ieta  iphi   seed  window  pileup  win-pu  calib  ecalTP  hcalTP   GenJet
    136.0  -3.74   1.00   -34    12   45.0   189.0    60.0   129.0   1.05     0.0   199.8    115.9
          phi rings of the 9 eta columns of the window (72 towers each): 34 41 58 39 137 67 41 36 29; sum 482; x 9/72 = 60 (the emulator: 60.0)
    108.5  -1.52  -1.98   -18    50   14.0    74.5    22.0    52.5   2.07    46.0    28.0    117.8
          phi rings of the 9 eta columns of the window (72 towers each): 20.5 12.5 18 42.5 26.5 17 10.5 19.5 9.5; sum 176.5; x 9/72 = 22 (the emulator: 22.0)
     23.0   0.91   1.70    11    20    7.5    25.0    17.5     7.5   3.07    16.5    11.5        -
     20.0  -0.30   0.30    -4     4    6.0    28.5    22.0     6.5   3.08    23.0    10.0        -
     17.5   1.52   0.91    18    11    4.0    17.0    13.0     4.0   4.38     8.5    10.0        -
     15.0   0.91  -2.50    11    44    4.5    21.0    17.5     3.5   4.29    15.5     8.5        -
     14.0  -2.26  -1.80   -25    52    4.0    32.5    28.5     4.0   3.50    31.0    20.5        -
     13.0   1.09   2.74    13    32    4.0    20.5    18.0     2.5   5.20    12.5    11.0        -
      9.5   2.39  -1.54    26    55    4.0    32.5    31.0     1.5   6.33    34.5    14.5        -
   all jets: sum of the windows 440.5, of the pileup estimates 229.5, of the jet ET 356.5

-- 4. Step 3: what Layer-2 sends to the global trigger
jets 9, e/gamma 12, taus 12
ETT (total ET, towers |ieta| <= 28)        1086.0
ETTEM (ECAL only)                           730.0
HTT (jets > 30 GeV, |ieta| <= 26)           108.5
ETM                                          60.5
ETMHF (with HF)                              79.0
HTM                                         108.0
HTMHF                                        34.5
tower count                                1295.0
L1EG 0: et=7.5 eta=-0.3045 phi=0.3045 iso=0 hwQual=7
L1EG 1: et=7 eta=-0.696 phi=-2.71619 iso=3 hwQual=7
L1EG 2: et=5.5 eta=1.131 phi=2.088 iso=0 hwQual=7
L1EG 3: et=5 eta=0.957 phi=2.3055 iso=0 hwQual=7
L1EG 4: et=4 eta=-0.261 phi=-1.54169 iso=0 hwQual=7
L1EG 5: et=3.5 eta=2.3925 phi=-0.0626853 iso=0 hwQual=7
L1EG 6: et=3.5 eta=-1.131 phi=-0.454185 iso=0 hwQual=7
L1EG 7: et=3.5 eta=0.522 phi=-3.02069 iso=3 hwQual=7
L1EG 8: et=3.5 eta=-1.914 phi=2.8275 iso=0 hwQual=7
L1EG 9: et=3.5 eta=-0.87 phi=2.7405 iso=3 hwQual=7
L1EG 10: et=2.5 eta=1.0875 phi=-2.23769 iso=0 hwQual=7
L1EG 11: et=2.5 eta=0.3045 phi=0.6525 iso=0 hwQual=7
L1Tau 0: et=58.5 eta=-1.566 phi=-1.93319 iso=1
L1Tau 1: et=15 eta=-2.2185 phi=-1.75919 iso=1
L1Tau 2: et=14 eta=0.9135 phi=1.653 iso=0
L1Tau 3: et=13.5 eta=2.1315 phi=1.044 iso=1
L1Tau 4: et=12.5 eta=1.9575 phi=-0.628186 iso=0
L1Tau 5: et=12.5 eta=-2.262 phi=2.958 iso=0
L1Tau 6: et=12.5 eta=-0.261 phi=1.218 iso=1
L1Tau 7: et=11.5 eta=2.2185 phi=-2.36819 iso=1
L1Tau 8: et=11.5 eta=-0.3045 phi=0.261 iso=0
L1Tau 9: et=11 eta=-0.696 phi=-2.71619 iso=1
L1Tau 10: et=11 eta=1.044 phi=2.697 iso=1
L1Tau 11: et=11 eta=1.131 phi=2.088 iso=0
```

The generator jets and the directions of the sums (`l1t_12_L1T.log`):

```
  GenJet: pt=117.788 eta=-1.58908 phi=-2.08403 energy=301.021
  GenJet: pt=115.944 eta=-3.74635 phi=1.00816 energy=2457.41
  L1Sums: ETT=1086 ETTEM=730 HTT=108.5 ETM=60.5 ETMphi=1.044 ETMHF=79 ETMHFphi=-2.32469 HTM=108 HTMphi=1.131 HTMHF=34.5 towerCount=1295 minBiasHFP0=15 minBiasHFM0=15 minBiasHFP1=0 minBiasHFM1=0
```

Sums of the chapter's ledger table: windows of the nine jets 440.5 GeV; pileup estimates 229.5 GeV; jet ET
356.5 GeV, of which the two signal jets 136.0 + 108.5 = 244.5 GeV. The b jet: 74.5 / 8 = 9.3 GeV of its own window
in its pileup estimate of 22.0 GeV; dR to the generator jet sqrt((1.5225 - 1.5891)^2 + (1.9767 - 2.0840)^2) = 0.13.
The seven other jets are 9.5 to 23.0 GeV, all with |eta| < 2.4, none seeded in tower ring 28. HTMHF is the imbalance
of the two signal jets: the vector sum of (136.0, phi 1.0005) and (108.5, phi -1.9767) is 34.0 GeV (the trigger:
34.5). Invariant mass of the two signal jets, M^2/2 = ET1 ET2 (cosh(deta) - cos(dphi)) with (136.0, -3.741, 1.0005)
and (108.5, -1.5225, -1.9767): 83 200 GeV^2, M = 408 GeV. They are the only two jets above 35 GeV, so every
`DoubleJet_..._DoubleJet35_Mass_Min...` algorithm fails (the lowest cut, MASS_MIN_500, is 125 000 GeV^2). In 2024
the HF jet and the ring-28 jet of 56.0 GeV at eta = 2.74 made 2 501 000 GeV^2, M = 2237 GeV.

The generator truth quoted with the jets is that of the earlier chapters, read from the same file: the b jet has
117.8 GeV with its neutrino and 79.8 GeV without (`ak4GenJetsNoNu`, chapter 07); the neutrino is the missing
transverse energy of the event, 37.5 GeV at phi = -2.10 (`genMetTrue`, chapter 06).

### Steps 4 and 5 (`ledger_12_L1T.txt`, section 5; `truth_12_L1T.txt`)

```
-- 5. Steps 4 and 5: the muon trigger
DT phi segments                                   0
DT theta segments                                 0
CSC anode patterns (ALCT)                        11
CSC cathode patterns (CLCT)                       8
CSC stubs (LCT)                                   8
GEM pads                                         11
GEM pad clusters                                  7
stubs given to the EMTF                          16
   of which neighbour copies                      4
EMTF tracks                                       2
BMTF (Kalman) candidates, all bx / bx 0         0/0
OMTF candidates                                 0/0
EMTF candidates                                 2/2
uGMT muons                                        1
muon showers                                      0
EMTF input stubs: all=16 neighbourCopies=4 CSC=8 (bx0 6) RPC=4 (bx0 2) GEM=0 (bx0 0)
EMTFHit: subsystem=CSC endcap=1 station=1 ring=4 sector=2 chamber=11 bx=0 neighbour=0 eta=2.35422 phi=1.71534 rho=118.004 z=615.68 quality=3 pattern=8
EMTFHit: subsystem=RPC endcap=-1 station=3 ring=2 sector=1 chamber=7 bx=-2 neighbour=0 eta=-1.483 phi=1.0718 rho=466.691 z=-975.2 quality=1 pattern=0
EMTFHit: subsystem=CSC endcap=-1 station=1 ring=4 sector=6 chamber=2 bx=0 neighbour=1 eta=-2.20308 phi=0.198793 rho=131.146 z=-586.38 quality=5 pattern=6
EMTFHit: subsystem=RPC endcap=-1 station=1 ring=2 sector=3 chamber=15 bx=0 neighbour=0 eta=-1.28302 phi=2.35879 rho=435.962 z=-725.95 quality=1 pattern=0
EMTFHit: subsystem=CSC endcap=-1 station=1 ring=1 sector=3 chamber=20 bx=0 neighbour=0 eta=-2.08396 phi=-2.96624 rho=148.229 z=-586.38 quality=7 pattern=6
EMTFHit: subsystem=CSC endcap=-1 station=2 ring=1 sector=3 chamber=10 bx=0 neighbour=0 eta=-2.07297 phi=-2.87505 rho=208.388 z=-815.062 quality=3 pattern=10
EMTFHit: subsystem=CSC endcap=-1 station=1 ring=4 sector=4 chamber=25 bx=-1 neighbour=0 eta=-2.29134 phi=-2.14766 rho=125.815 z=-615.68 quality=3 pattern=9
EMTFHit: subsystem=RPC endcap=-1 station=1 ring=2 sector=4 chamber=25 bx=0 neighbour=0 eta=-1.39743 phi=-2.12451 rho=382.323 z=-725.95 quality=4 pattern=0
EMTFHit: subsystem=CSC endcap=-1 station=3 ring=1 sector=4 chamber=11 bx=0 neighbour=0 eta=-2.06941 phi=-2.85873 rho=237.143 z=-924.138 quality=3 pattern=10
EMTFHit: subsystem=CSC endcap=-1 station=4 ring=1 sector=4 chamber=11 bx=0 neighbour=0 eta=-2.06722 phi=-2.85873 rho=260.817 z=-1014.1 quality=3 pattern=10
EMTFHit: subsystem=CSC endcap=-1 station=1 ring=1 sector=3 chamber=20 bx=0 neighbour=1 eta=-2.08396 phi=-2.96624 rho=148.229 z=-586.38 quality=7 pattern=6
EMTFHit: subsystem=CSC endcap=-1 station=2 ring=1 sector=3 chamber=10 bx=0 neighbour=1 eta=-2.07297 phi=-2.87505 rho=208.388 z=-815.062 quality=3 pattern=10
EMTFHit: subsystem=RPC endcap=-1 station=3 ring=3 sector=6 chamber=34 bx=-3 neighbour=0 eta=-1.35977 phi=-0.5318 rho=533.618 z=-970.8 quality=1 pattern=0
EMTFHit: subsystem=CSC endcap=-1 station=1 ring=4 sector=6 chamber=33 bx=-1 neighbour=0 eta=-2.34761 phi=-0.766924 rho=118.801 z=-615.68 quality=3 pattern=3
EMTFHit: subsystem=CSC endcap=-1 station=1 ring=4 sector=6 chamber=2 bx=0 neighbour=0 eta=-2.20308 phi=0.198793 rho=131.146 z=-586.38 quality=5 pattern=6
EMTFHit: subsystem=RPC endcap=-1 station=4 ring=3 sector=5 chamber=32 bx=0 neighbour=1 eta=-1.24382 phi=-0.869931 rho=667.652 z=-1061.76 quality=1 pattern=0
EMTFTrack: endcap=-1 sector=3 mode=12 bx=0 pt=2 eta=-2.08277 phi=-2.87514 charge=-1 nHits=2 quality=7
EMTFTrack: endcap=-1 sector=4 mode=15 bx=0 pt=3 eta=-2.08277 phi=-2.87514 charge=-1 nHits=4 quality=15
TFCand: tf=EMTF bx=0 processor=2 pt=2 eta=-2.088 phi=-2.87979 quality=7 hwPt=5 hwEta=-192 hwPhi=96 sign=1
TFCand: tf=EMTF bx=0 processor=3 pt=3 eta=-2.088 phi=-2.87979 quality=15 hwPt=7 hwEta=-192 hwPhi=0 sign=1
uGMT muon: bx=0 pt=3 eta=-2.088 phi=-2.87989 charge=-1 quality=15 iso=3 etaAtVtx=-2.06625 phiAtVtx=3.09787 tfMuonIndex=99
truth: genMuons(pt>1)=0 genJets(pt>30)=2
```

The EMTF received 16 stubs of which 4 are neighbour copies: 12 distinct ones, 8 CSC and 4 RPC, 8 of them in bunch
crossing 0 (6 CSC, 2 RPC). Steps 4 and 5 are identical, line for line, to those of the 2024 run. `pt = (hwPt - 1) x 0.5 GeV`, `eta = hwEta x 0.010875`, global phi =
(processor x 96 + hwPhi + 24) x 2 pi / 576 for the OMTF and EMTF (`l1t::MicroGMTConfiguration::calcGlobalPhi`). EMTF
mode: bit 8 ME1, 4 ME2, 2 ME3, 1 ME4. The extrapolation to the vertex moves the muon from phi = -2.880 to 3.098, that
is by 2 pi - 2.880 - 3.098 = 0.31.

The truth behind the L1 muon (`truth_12_L1T.txt`, in full):

```
== l1tTruth 12_L1T
1 L1 muon(s); 45282 TrackingParticles
  L1 muon bx 0: pt 3.0 eta -2.088 phi -2.880 (at the vertex: eta -2.066 phi 3.098) charge -1 quality 15
    charged pdgId    211 pt   2.38 eta -2.091 phi  3.087 charge +1 | bx 1, collision 33 (0 = the signal) | produced at r 0.03 cm, z 1.6 cm | hits 25 (tracker 25) | dR 0.03
    charged pdgId   -321 pt   3.27 eta -2.035 phi  3.026 charge -1 | bx 0, collision 21 (0 = the signal) | produced at r 0.03 cm, z 5.5 cm | hits 19 (tracker 19) | dR 0.08
    charged pdgId    211 pt   2.58 eta -1.986 phi  3.130 charge +1 | bx 1, collision 14 (0 = the signal) | produced at r 0.03 cm, z 3.7 cm | hits 25 (tracker 25) | dR 0.09
    charged pdgId  -2212 pt   3.93 eta -1.965 phi  3.081 charge -1 | bx 1, collision 9 (0 = the signal) | produced at r 0.03 cm, z -3.8 cm | hits 10 (tracker 10) | dR 0.10
    charged pdgId  -2212 pt   1.97 eta -2.049 phi -3.062 charge -1 | bx 1, collision 9 (0 = the signal) | produced at r 29.88 cm, z -108.6 cm | hits 14 (tracker 14) | dR 0.12
    charged pdgId   -321 pt   2.40 eta -2.018 phi  2.978 charge -1 | bx 0, collision 36 (0 = the signal) | produced at r 0.02 cm, z 0.3 cm | hits 24 (tracker 24) | dR 0.13
    MUON    pdgId     13 pt   2.78 eta -2.083 phi -3.043 charge -1 | bx 0, collision 21 (0 = the signal) | produced at r 58.52 cm, z -214.4 cm | hits 35 (tracker 6) | dR 0.14
    charged pdgId  -2212 pt   2.29 eta -2.269 phi -3.082 charge -1 | bx 0, collision 60 (0 = the signal) | produced at r 0.03 cm, z -0.1 cm | hits 18 (tracker 18) | dR 0.23
    charged pdgId     11 pt   1.97 eta -1.809 phi  3.065 charge -1 | bx 0, collision 36 (0 = the signal) | produced at r 11.55 cm, z -34.0 cm | hits 22 (tracker 22) | dR 0.26
    charged pdgId   -211 pt   2.55 eta -1.850 phi  2.954 charge -1 | bx 0, collision 36 (0 = the signal) | produced at r 27.20 cm, z -77.6 cm | hits 20 (tracker 20) | dR 0.26
    charged pdgId    211 pt   2.10 eta -2.207 phi  2.853 charge +1 | bx 1, collision 33 (0 = the signal) | produced at r 75.41 cm, z -299.5 cm | hits 0 (tracker 0) | dR 0.28
```

The muon is produced at r = 58.52 cm, z = -214.4 cm; a straight line from the K- vertex (z = 5.5 cm) at eta = -2.035
reaches r = (214.4 + 5.5) / sinh(2.035) = 58.5 cm at that z: the muon is the daughter of the kaon. Its 35 hits are 6 in
the tracker and 29 in the muon chambers. "Collision 21" is the 21st of the 68 in-time pileup collisions (0 is the
signal).

### Step 6 (`ledger_12_L1T.txt`, section 6)

```
-- 6. Step 6: the decision of the global trigger
FinalOR 1; 63 of 465 algorithms fire (63 after prescales and masks); CICADA score: 88.2617
uGT inputs (bx 0): jets=9 egammas=12 taus=12 muons=1 etSums=19 | ETT=1086 ETTEM=730 HTT=108.5 ETM=60.5 ETMHF=79 HTM=108 HTMHF=34.5
the 63 algorithms that fire, with their expression:
     0 L1_SingleMuCosmics                                         MU0
     3 L1_SingleMuCosmics_EMTF                                    MU0[MU-INDEX_EMTF_NEG,MU-INDEX_EMTF_POS]
     4 L1_SingleMuOpen                                            MU0[MU-QLTY_OPEN]
     7 L1_SingleMuOpen_EMTF                                       MU0[MU-INDEX_EMTF_NEG,MU-INDEX_EMTF_POS,MU-QLTY_OPEN]
     8 L1_SingleMu0_DQ                                            MU0[MU-QLTY_DBLE]
    11 L1_SingleMu0_EMTF                                          MU0[MU-INDEX_EMTF_NEG,MU-INDEX_EMTF_POS,MU-QLTY_SNGL]
    19 L1_SingleMu3                                               MU3[MU-QLTY_SNGL]
   137 L1_Mu3_Jet30er2p5                                          MU3[MU-QLTY_SNGL] AND JET30[JET-ETA_2p52]
   296 L1_SingleJet35                                             JET35
   298 L1_SingleJet60er1p3to2p5                                   JET60[JET-ETA_2p52] AND  NOT JET60[JET-ETA_1p3]
   301 L1_SingleJet60                                             JET60
   302 L1_SingleJet90                                             JET90
   303 L1_SingleJet120                                            JET120
   307 L1_SingleJet35er2p5                                        JET35[JET-ETA_2p52]
   313 L1_SingleJet35_FWD2p5                                      JET35[JET-ETA_FWD_2p48_NEG] OR JET35[JET-ETA_FWD_2p48_POS]
   314 L1_SingleJet60_FWD2p5                                      JET60[JET-ETA_FWD_2p48_NEG] OR JET60[JET-ETA_FWD_2p48_POS]
   315 L1_SingleJet90_FWD2p5                                      JET90[JET-ETA_FWD_2p48_NEG] OR JET90[JET-ETA_FWD_2p48_POS]
   316 L1_SingleJet120_FWD2p5                                     JET120[JET-ETA_FWD_2p48_NEG] OR JET120[JET-ETA_FWD_2p48_POS]
   317 L1_SingleJet35_FWD3p0                                      JET35[JET-ETA_FWD_3p00_NEG] OR JET35[JET-ETA_FWD_3p00_POS]
   318 L1_SingleJet120_FWD3p0                                     JET120[JET-ETA_FWD_3p00_NEG] OR JET120[JET-ETA_FWD_3p00_POS]
   319 L1_SingleJet8erHE                                          JET8[JET-ETA_1p392_2p958_NEG,JET-ETA_1p392_2p958_POS]
   320 L1_SingleJet10erHE                                         JET10[JET-ETA_1p392_2p958_NEG,JET-ETA_1p392_2p958_POS]
   321 L1_SingleJet12erHE                                         JET12[JET-ETA_1p392_2p958_NEG,JET-ETA_1p392_2p958_POS]
   400 L1_ETMHF70                                                 ETMHF70
   409 L1_ETMHF70_HTT60er                                         ETMHF70 AND HTT60
   428 L1_CICADA_VLoose                                           CICADA[CICADA-CSCORE_50]
   429 L1_CICADA_Loose                                            CICADA[CICADA-CSCORE_60]
   430 L1_CICADA_Medium                                           CICADA[CICADA-CSCORE_70]
   431 L1_CICADA_Tight                                            CICADA[CICADA-CSCORE_80]
   444 L1_SingleJet8_BptxAND                                      JET8 AND EXT_ZeroBias_BPTX_AND_VME
   445 L1_SingleJet12_BptxAND                                     JET12 AND EXT_ZeroBias_BPTX_AND_VME
   446 L1_SingleJet16_BptxAND                                     JET16 AND EXT_ZeroBias_BPTX_AND_VME
   447 L1_SingleJet20_BptxAND                                     JET20 AND EXT_ZeroBias_BPTX_AND_VME
   448 L1_SingleJet24_BptxAND                                     JET24 AND EXT_ZeroBias_BPTX_AND_VME
   449 L1_SingleJet8er1p3_BptxAND                                 JET8[JET-ETA_1p3] AND EXT_ZeroBias_BPTX_AND_VME
   450 L1_SingleJet12er1p3_BptxAND                                JET12[JET-ETA_1p3] AND EXT_ZeroBias_BPTX_AND_VME
   451 L1_SingleJet16er1p3_BptxAND                                JET16[JET-ETA_1p3] AND EXT_ZeroBias_BPTX_AND_VME
   452 L1_SingleJet20er1p3_BptxAND                                JET20[JET-ETA_1p3] AND EXT_ZeroBias_BPTX_AND_VME
   454 L1_SingleJet8                                              JET8
   455 L1_SingleJet8er1p3                                         JET8[JET-ETA_1p3]
   456 L1_SingleJet8er2p13                                        JET8[JET-ETA_2p13]
   457 L1_SingleEG5_BptxAND                                       EG5 AND EXT_ZeroBias_BPTX_AND_VME
   458 L1_SingleEG7_BptxAND                                       EG7 AND EXT_ZeroBias_BPTX_AND_VME
   459 L1_SingleEG5                                               EG5
   460 L1_SingleEG7                                               EG7
   461 L1_SingleEG5er1p52                                         EG5[EG-ETA_1p52_HASHFIX]
   462 L1_SingleEG7er1p52                                         EG7[EG-ETA_1p52_HASHFIX]
   463 L1_DoubleEG2_BptxAND                                       comb{EG2,EG2} AND EXT_ZeroBias_BPTX_AND_VME
   464 L1_DoubleEG2er2p13_BptxAND                                 comb{EG2[EG-ETA_2p13],EG2[EG-ETA_2p13]} AND EXT_ZeroBias_BPTX_AND_VME
   465 L1_DoubleEG3_BptxAND                                       comb{EG3,EG3} AND EXT_ZeroBias_BPTX_AND_VME
   466 L1_DoubleEG3er2p13_BptxAND                                 comb{EG3[EG-ETA_2p13],EG3[EG-ETA_2p13]} AND EXT_ZeroBias_BPTX_AND_VME
   467 L1_DoubleEG4_BptxAND                                       comb{EG4,EG4} AND EXT_ZeroBias_BPTX_AND_VME
   468 L1_DoubleEG4er2p13_BptxAND                                 comb{EG4[EG-ETA_2p13],EG4[EG-ETA_2p13]} AND EXT_ZeroBias_BPTX_AND_VME
   469 L1_DoubleEG2                                               comb{EG2,EG2}
   470 L1_DoubleEG2er2p13                                         comb{EG2[EG-ETA_2p13],EG2[EG-ETA_2p13]}
   472 L1_AlwaysTrue                                              EXT_ZeroBias_BPTX_AND_VME OR ( NOT EXT_ZeroBias_BPTX_AND_VME)
   473 L1_ZeroBias                                                EXT_ZeroBias_BPTX_AND_VME
   474 L1_ZeroBias_copy                                           EXT_ZeroBias_BPTX_AND_VME
   475 L1_MinimumBiasHF0_AND_BptxAND                              (MBT0HFP1 AND MBT0HFM1) AND EXT_ZeroBias_BPTX_AND_VME
   476 L1_MinimumBiasHF0                                          MBT0HFP1 AND MBT0HFM1
   478 L1_BptxOR                                                  EXT_BPTX_OR_VME
   480 L1_BptxPlus                                                EXT_BPTX_B1_VME
   481 L1_BptxMinus                                               EXT_BPTX_B2_VME
the objects behind the conditions that are true (at most two combinations per condition):
  L1_SingleMuCosmics                             | SingleMU_i0=1 (1 combination): Mu[0](3.0,-2.09,-2.88)
  L1_SingleMuCosmics_EMTF                        | SingleMU_i3=1 (1 combination): Mu[0](3.0,-2.09,-2.88)
  L1_SingleMuOpen                                | SingleMU_i4=1 (1 combination): Mu[0](3.0,-2.09,-2.88)
  L1_SingleMuOpen_EMTF                           | SingleMU_i7=1 (1 combination): Mu[0](3.0,-2.09,-2.88)
  L1_SingleMu0_DQ                                | SingleMU_i8=1 (1 combination): Mu[0](3.0,-2.09,-2.88)
  L1_SingleMu0_EMTF                              | SingleMU_i11=1 (1 combination): Mu[0](3.0,-2.09,-2.88)
  L1_SingleMu3                                   | SingleMU_i12=1 (1 combination): Mu[0](3.0,-2.09,-2.88)
  L1_Mu3_Jet30er2p5                              | SingleMU_i12=1 (1 combination): Mu[0](3.0,-2.09,-2.88) | SingleJET_i103=1 (1 combination): Jet[1](108.5,-1.52,-1.98)
  L1_SingleJet35                                 | SingleJET_i218=1 (2 combinations): Jet[0](136.0,-3.74,1.00) or Jet[1](108.5,-1.52,-1.98)
  L1_SingleJet60er1p3to2p5                       | SingleJET_i131=1 (1 combination): Jet[1](108.5,-1.52,-1.98) | SingleJET_i470=0
  L1_SingleJet60                                 | SingleJET_i219=1 (2 combinations): Jet[0](136.0,-3.74,1.00) or Jet[1](108.5,-1.52,-1.98)
  L1_SingleJet90                                 | SingleJET_i220=1 (2 combinations): Jet[0](136.0,-3.74,1.00) or Jet[1](108.5,-1.52,-1.98)
  L1_SingleJet120                                | SingleJET_i221=1 (1 combination): Jet[0](136.0,-3.74,1.00)
  L1_SingleJet35er2p5                            | SingleJET_i224=1 (1 combination): Jet[1](108.5,-1.52,-1.98)
  L1_SingleJet35_FWD2p5                          | SingleJET_i229=1 (1 combination): Jet[0](136.0,-3.74,1.00) | SingleJET_i230=0
  L1_SingleJet60_FWD2p5                          | SingleJET_i231=1 (1 combination): Jet[0](136.0,-3.74,1.00) | SingleJET_i232=0
  L1_SingleJet90_FWD2p5                          | SingleJET_i233=1 (1 combination): Jet[0](136.0,-3.74,1.00) | SingleJET_i234=0
  L1_SingleJet120_FWD2p5                         | SingleJET_i235=1 (1 combination): Jet[0](136.0,-3.74,1.00) | SingleJET_i236=0
  L1_SingleJet35_FWD3p0                          | SingleJET_i237=1 (1 combination): Jet[0](136.0,-3.74,1.00) | SingleJET_i238=0
  L1_SingleJet120_FWD3p0                         | SingleJET_i239=1 (1 combination): Jet[0](136.0,-3.74,1.00) | SingleJET_i240=0
  L1_SingleJet8erHE                              | SingleJET_i241=1 (4 combinations): Jet[1](108.5,-1.52,-1.98) or Jet[4](17.5,1.52,0.91)
  L1_SingleJet10erHE                             | SingleJET_i242=1 (3 combinations): Jet[1](108.5,-1.52,-1.98) or Jet[4](17.5,1.52,0.91)
  L1_SingleJet12erHE                             | SingleJET_i243=1 (3 combinations): Jet[1](108.5,-1.52,-1.98) or Jet[4](17.5,1.52,0.91)
  L1_ETMHF70                                     | SingleETMHF_i287=1 (1 combination): ETMHF
  L1_ETMHF70_HTT60er                             | SingleETMHF_i287=1 (1 combination): ETMHF | SingleHTT_i130=1 (1 combination): HTT
  L1_CICADA_VLoose                               | CicadaTrigger_i410=1 (0 combinations)
  L1_CICADA_Loose                                | CicadaTrigger_i411=1 (0 combinations)
  L1_CICADA_Medium                               | CicadaTrigger_i412=1 (0 combinations)
  L1_CICADA_Tight                                | CicadaTrigger_i413=1 (0 combinations)
  L1_SingleJet8_BptxAND                          | SingleJET_i449=1 (9 combinations): Jet[0](136.0,-3.74,1.00) or Jet[1](108.5,-1.52,-1.98) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet12_BptxAND                         | SingleJET_i450=1 (8 combinations): Jet[0](136.0,-3.74,1.00) or Jet[1](108.5,-1.52,-1.98) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet16_BptxAND                         | SingleJET_i451=1 (5 combinations): Jet[0](136.0,-3.74,1.00) or Jet[1](108.5,-1.52,-1.98) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet20_BptxAND                         | SingleJET_i452=1 (4 combinations): Jet[0](136.0,-3.74,1.00) or Jet[1](108.5,-1.52,-1.98) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet24_BptxAND                         | SingleJET_i453=1 (2 combinations): Jet[0](136.0,-3.74,1.00) or Jet[1](108.5,-1.52,-1.98) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet8er1p3_BptxAND                     | SingleJET_i454=1 (4 combinations): Jet[2](23.0,0.91,1.70) or Jet[3](20.0,-0.30,0.30) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet12er1p3_BptxAND                    | SingleJET_i455=1 (4 combinations): Jet[2](23.0,0.91,1.70) or Jet[3](20.0,-0.30,0.30) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet16er1p3_BptxAND                    | SingleJET_i456=1 (2 combinations): Jet[2](23.0,0.91,1.70) or Jet[3](20.0,-0.30,0.30) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet20er1p3_BptxAND                    | SingleJET_i457=1 (2 combinations): Jet[2](23.0,0.91,1.70) or Jet[3](20.0,-0.30,0.30) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet8                                  | SingleJET_i449=1 (9 combinations): Jet[0](136.0,-3.74,1.00) or Jet[1](108.5,-1.52,-1.98)
  L1_SingleJet8er1p3                             | SingleJET_i454=1 (4 combinations): Jet[2](23.0,0.91,1.70) or Jet[3](20.0,-0.30,0.30)
  L1_SingleJet8er2p13                            | SingleJET_i459=1 (6 combinations): Jet[1](108.5,-1.52,-1.98) or Jet[2](23.0,0.91,1.70)
  L1_SingleEG5_BptxAND                           | SingleEG_i460=1 (4 combinations): EG[0](7.5,-0.30,0.30) or EG[1](7.0,-0.70,-2.72) | SingleEXT_i302=1 (1 combination): External
  L1_SingleEG7_BptxAND                           | SingleEG_i461=1 (2 combinations): EG[0](7.5,-0.30,0.30) or EG[1](7.0,-0.70,-2.72) | SingleEXT_i302=1 (1 combination): External
  L1_SingleEG5                                   | SingleEG_i460=1 (4 combinations): EG[0](7.5,-0.30,0.30) or EG[1](7.0,-0.70,-2.72)
  L1_SingleEG7                                   | SingleEG_i461=1 (2 combinations): EG[0](7.5,-0.30,0.30) or EG[1](7.0,-0.70,-2.72)
  L1_SingleEG5er1p52                             | SingleEG_i462=1 (4 combinations): EG[0](7.5,-0.30,0.30) or EG[1](7.0,-0.70,-2.72)
  L1_SingleEG7er1p52                             | SingleEG_i463=1 (2 combinations): EG[0](7.5,-0.30,0.30) or EG[1](7.0,-0.70,-2.72)
  L1_DoubleEG2_BptxAND                           | DoubleEG_i464=1 (132 combinations): EG[0](7.5,-0.30,0.30) EG[1](7.0,-0.70,-2.72) or EG[0](7.5,-0.30,0.30) EG[2](5.5,1.13,2.09) | SingleEXT_i302=1 (1 combination): External
  L1_DoubleEG2er2p13_BptxAND                     | DoubleEG_i465=1 (110 combinations): EG[0](7.5,-0.30,0.30) EG[1](7.0,-0.70,-2.72) or EG[0](7.5,-0.30,0.30) EG[2](5.5,1.13,2.09) | SingleEXT_i302=1 (1 combination): External
  L1_DoubleEG3_BptxAND                           | DoubleEG_i466=1 (90 combinations): EG[0](7.5,-0.30,0.30) EG[1](7.0,-0.70,-2.72) or EG[0](7.5,-0.30,0.30) EG[2](5.5,1.13,2.09) | SingleEXT_i302=1 (1 combination): External
  L1_DoubleEG3er2p13_BptxAND                     | DoubleEG_i468=1 (72 combinations): EG[0](7.5,-0.30,0.30) EG[1](7.0,-0.70,-2.72) or EG[0](7.5,-0.30,0.30) EG[2](5.5,1.13,2.09) | SingleEXT_i302=1 (1 combination): External
  L1_DoubleEG4_BptxAND                           | DoubleEG_i467=1 (20 combinations): EG[0](7.5,-0.30,0.30) EG[1](7.0,-0.70,-2.72) or EG[0](7.5,-0.30,0.30) EG[2](5.5,1.13,2.09) | SingleEXT_i302=1 (1 combination): External
  L1_DoubleEG4er2p13_BptxAND                     | DoubleEG_i469=1 (20 combinations): EG[0](7.5,-0.30,0.30) EG[1](7.0,-0.70,-2.72) or EG[0](7.5,-0.30,0.30) EG[2](5.5,1.13,2.09) | SingleEXT_i302=1 (1 combination): External
  L1_DoubleEG2                                   | DoubleEG_i464=1 (132 combinations): EG[0](7.5,-0.30,0.30) EG[1](7.0,-0.70,-2.72) or EG[0](7.5,-0.30,0.30) EG[2](5.5,1.13,2.09)
  L1_DoubleEG2er2p13                             | DoubleEG_i465=1 (110 combinations): EG[0](7.5,-0.30,0.30) EG[1](7.0,-0.70,-2.72) or EG[0](7.5,-0.30,0.30) EG[2](5.5,1.13,2.09)
  L1_AlwaysTrue                                  | SingleEXT_i302=1 (1 combination): External | SingleEXT_i302=1 (1 combination): External
  L1_ZeroBias                                    | SingleEXT_i302=1 (1 combination): External
  L1_ZeroBias_copy                               | SingleEXT_i302=1 (1 combination): External
  L1_MinimumBiasHF0_AND_BptxAND                  | SingleMBT0HFP_i303=1 (1 combination): MinBiasHFP0 | SingleMBT0HFM_i304=1 (1 combination): MinBiasHFM0 | SingleEXT_i302=1 (1 combination): External
  L1_MinimumBiasHF0                              | SingleMBT0HFP_i303=1 (1 combination): MinBiasHFP0 | SingleMBT0HFM_i304=1 (1 combination): MinBiasHFM0
  L1_BptxOR                                      | SingleEXT_i294=1 (1 combination): External
  L1_BptxPlus                                    | SingleEXT_i305=1 (1 combination): External
  L1_BptxMinus                                   | SingleEXT_i306=1 (1 combination): External
the ladders: the event against the thresholds of the single-object families (GeV); margin = value - threshold
   SingleJet            value   136.0   highest passed: 120 (margin +16.0)   lowest failed: 180 (margin -44.0)
   SingleJet..er2p5     value   108.5   highest passed: 35 (margin +73.5)   lowest failed: 120 (margin -11.5)
   SingleJet.._FWD3p0   value   136.0   highest passed: 120 (margin +16.0)   lowest failed: none
   DoubleJet..er2p5     value    23.0   highest passed: none   lowest failed: 40 (margin -17.0)
   HTT..er              value   108.5   highest passed: none   lowest failed: 120 (margin -11.5)
   ETT                  value  1086.0   highest passed: none   lowest failed: 2000 (margin -914.0)
   ETMHF                value    79.0   highest passed: 70 (margin +9.0)   lowest failed: 80 (margin -1.0)
   SingleEG..er2p5      value     7.5   highest passed: none   lowest failed: 8 (margin -0.5)
   SingleTau..er2p1     value    58.5   highest passed: none   lowest failed: 120 (margin -61.5)
   SingleMu             value     3.0   highest passed: 3 (margin +0.0)   lowest failed: 5 (margin -2.0)
```

How the 63 are counted in the chapter. 8 do not look at any object (`L1_AlwaysTrue`, `L1_ZeroBias`, `L1_ZeroBias_copy`,
`L1_BptxOR`, `L1_BptxPlus`, `L1_BptxMinus`, and the two `L1_MinimumBiasHF0*`, which ask for one HF tower on each side);
55 do. Of these, 22 are satisfied by the two signal jets alone (`SingleJet35/60/90/120`, `SingleJet60er1p3to2p5`,
`SingleJet35er2p5`, the six `_FWD2p5` / `_FWD3p0`, the three `erHE`, `SingleJet8`, `SingleJet8er2p13` and the five
`SingleJet8/12/16/20/24_BptxAND`); 33 need an object without a generator jet or signal particle behind it: 7
single-muon and 1 muon + jet (the kaon decay); 5 barrel jets (`SingleJet8er1p3` and `SingleJet8/12/16/20er1p3_BptxAND`:
the pileup jets of 23.0 GeV at eta = 0.91 and 20.0 GeV at eta = -0.30); 14 e/gamma (`SingleEG5/7`, `SingleEG5/7er1p52`,
`SingleEG5/7_BptxAND`, `DoubleEG2`, `DoubleEG2er2p13`, `DoubleEG2/3/4_BptxAND`, `DoubleEG2/3/4er2p13_BptxAND`: e/gamma
candidates of 7.5 and 7.0 GeV in the barrel and the ones below them); 2 missing-energy (`ETMHF70`, `ETMHF70_HTT60er`);
4 `CICADA` (`VLoose` to `Tight`, thresholds 50, 60, 70, 80; `VTight`, 90, fails by 1.7). 27 of the 63 are algorithms
the 2024 menu did not have (26 low-threshold jet and e/gamma algorithms, `SingleJet8...`, `SingleEG5/7...`,
`DoubleEG2/3/4...`, and `SingleJet60er1p3to2p5`); the other 36 fired in 2024 too. The 21 that fired in 2024 and do
not now: `SingleEG8er2p5` (the largest e/gamma candidate is 7.5 GeV), the 8 `DoubleJet...Mass` algorithms (no jet in
ring 28), `ETMHF80/90/100` and the same with `_HTT60er` (ETMHF 79.0), the 5 `HTMHF` (34.5) and `CICADA_VTight`.

The thresholds of the ladders and three of the object maps as the analyzer prints them (`l1t_12_L1T.log`):

```
  ladder SingleJet: value=136 thresholds(fired): 8(1) 35(1) 60(1) 90(1) 120(1) 180(0) 200(0)
  ladder SingleJet..er2p5: value=108.5 thresholds(fired): 35(1) 120(0) 140(0) 160(0) 180(0)
  ladder SingleJet.._FWD3p0: value=136 thresholds(fired): 35(1) 120(1)
  ladder DoubleJet..er2p5: value=23 thresholds(fired): 40(0) 100(0) 120(0) 150(0)
  ladder HTT..er: value=108.5 thresholds(fired): 120(0) 160(0) 200(0) 255(0) 280(0) 320(0) 360(0) 400(0) 450(0)
  ladder ETT: value=1086 thresholds(fired): 2000(0)
  ladder ETMHF: value=79 thresholds(fired): 70(1) 80(0) 90(0) 100(0) 110(0) 120(0) 130(0) 140(0) 150(0)
  ladder SingleEG..er2p5: value=7.5 thresholds(fired): 8(0) 10(0) 15(0) 26(0) 28(0) 34(0) 36(0) 38(0) 40(0) 42(0) 45(0)
  ladder SingleTau..er2p1: value=58.5 thresholds(fired): 120(0) 130(0)
  ladder SingleMu: value=3 thresholds(fired): 3(1) 5(0) 7(0) 18(0) 20(0) 22(0) 25(0)
  fired: bit=302 name=L1_SingleJet90 final=1 expression="JET90" | SingleJET_i220=1 (2 combinations): Jet[0](136.0,-3.74,1.00) or Jet[1](108.5,-1.52,-1.98)
  fired: bit=307 name=L1_SingleJet35er2p5 final=1 expression="JET35[JET-ETA_2p52]" | SingleJET_i224=1 (1 combination): Jet[1](108.5,-1.52,-1.98)
  fired: bit=19 name=L1_SingleMu3 final=1 expression="MU3[MU-QLTY_SNGL]" | SingleMU_i12=1 (1 combination): Mu[0](3.0,-2.09,-2.88)
```

Some conditions as the menu defines them (`l1t_12_L1T.log`, `condition:` lines, printed for the conditions that are
true; cut type 0 = threshold in GeV, 1 = eta window, 4 = quality bit mask, 11 = count, 25 = CICADA score; no mass
condition is true in 2025): the muon quality masks are 61440 = bits 12-15 (`MU-QLTY_SNGL`), 65280 = bits 8-15
(`DBLE`), 65520 = bits 4-15 (`OPEN`). `L1_SingleJet60er1p3to2p5` is `JET60[JET-ETA_2p52] AND NOT JET60[JET-ETA_1p3]`:
the first condition is true for the b jet, the second false (no jet above 60 GeV within |eta| < 1.305). The external
condition `EXT_ZeroBias_BPTX_AND_VME` of the `_BptxAND` algorithms is the beam pick-up, true in every simulated event.

```
  condition: name=SingleMU_i12 type=0 | object MU3 bx 0: MU-ET_3[type 0: 3..1.79769e+308] MU-QLTY_SNGL[type 4: 0..0 data 61440]
  condition: name=SingleJET_i131 type=12 | object JET60 bx 0: JET-ET_60[type 0: 60..1.79769e+308] JET-ETA_2p52[type 1: -2.523..2.523]
  condition: name=SingleJET_i470 type=12 | object JET60 bx 0: JET-ET_60[type 0: 60..1.79769e+308] JET-ETA_1p3[type 1: -1.305..1.305]
  condition: name=SingleJET_i220 type=12 | object JET90 bx 0: JET-ET_90[type 0: 90..1.79769e+308]
  condition: name=SingleJET_i224 type=12 | object JET35 bx 0: JET-ET_35[type 0: 35..1.79769e+308] JET-ETA_2p52[type 1: -2.523..2.523]
  condition: name=SingleJET_i233 type=12 | object JET90 bx 0: JET-ET_90[type 0: 90..1.79769e+308] JET-ETA_FWD_2p48_NEG[type 1: -5..-2.4795]
  condition: name=SingleJET_i239 type=12 | object JET120 bx 0: JET-ET_120[type 0: 120..1.79769e+308] JET-ETA_FWD_3p00_NEG[type 1: -5..-3.0015]
  condition: name=SingleETMHF_i287 type=32 | object ETMHF70 bx 0: ETMHF-ET_70[type 0: 70..1.79769e+308]
  condition: name=CicadaTrigger_i413 type=75 | object CICADA bx 0: CICADA-CSCORE_80[type 25: 80..0]
  condition: name=SingleJET_i449 type=12 | object JET8 bx 0: JET-ET_8[type 0: 8..1.79769e+308]
  condition: name=SingleEXT_i302 type=20 | object EXT_ZeroBias_BPTX_AND_VME bx 0:
  condition: name=DoubleEG_i464 type=5 | object EG2 bx 0: EG-ET_2[type 0: 2..1.79769e+308] | object EG2 bx 0: EG-ET_2[type 0: 2..1.79769e+308]
  condition: name=SingleMBT0HFP_i303 type=27 | object MBT0HFP1 bx 0: MBT0HFP-COUNT_1[type 11: 1..1.79769e+308]
```

The 465 algorithms by name (the `algo:` lines; a name is counted in every group it matches): 180 contain `Mu`, 112
`EG`, 30 `Tau`, 119 are made of jets and sums only (a `Jet`, `HTT`, `HTM`, `ETM` or `ETT` in the name and none of the
three above); 8 `L1_CICADA_*` and 7 `L1_AXO_*`; 39 look at no object (zero bias, beam pick-ups, bunch-train
positions, HCAL laser, TOTEM). The same rule gives 180, 89, 27, 93, 5 + 5 and 39 for the 408 algorithms of the 2024
menu. Against 2024, 73 algorithms are new and 16 are gone. New: 27 low-threshold jet and e/gamma algorithms
(`SingleJet8/12/16/20/24_BptxAND` and their `er1p3` versions, `SingleJet8`, `SingleJet8er1p3`, `SingleJet8er2p13`,
`SingleEG5/7`, `SingleEG5/7er1p52`, `SingleEG5/7_BptxAND`, `DoubleEG2`, `DoubleEG2er2p13`, `DoubleEG2/3/4_BptxAND`,
`DoubleEG2/3/4er2p13_BptxAND`); 13 single jets and e/gamma at 60 and 28 GeV in eta slices (`SingleJet60er1p3`,
`er1p3to2p5`, `er2p5to3p0`, `er3p0to5p0`; `SingleEG28...`, `SingleLooseIsoEG28...`, `SingleIsoEG28...`, three slices
each); 16 VBF and VBF + object algorithms at higher mass cuts (`DoubleJet_110_35_..._Mass_Min1000/1100/1200`,
`..._65_35_..._DoubleJetCentral50`, `..._70_35_..._ETMHF65`, `..._85_35_..._Mu3OQ`, `DoubleJet45_Mass_Min700/800_...`);
9 double-tau and double-e/gamma topologies (`DoubleTau_Iso34/38/40_Iso26_er2p1_Jet55...`,
`DoubleEG15/16/17_11_er1p2_dR_Max0p6`, `DoubleEG15/16/17_er1p5_dEta_Max1p5`); three more CICADA thresholds (`VVTight`
to `VVVVTight`) and three AXO ones (`Medium`, `VVTight`, `VVVTight`); two BMTF single muons. Gone: three
`DoubleMu..._Upt..._SQ_er2p0`, two `SingleMu0_Upt..._SQ14_BMTF`, `AXO_Nominal`, two `TripleEG`, two
`DoubleIsoTau26er2p1_Jet55/70...` and six `LooseIsoEG2x..._HTT100er` / `_Jet34er2p5_dR_Min0p3`. The menu says nothing
about what the low-threshold algorithms are for (the payload has no comment per algorithm). The CICADA thresholds
of the 2025 menu (`conddb dump L1Menu_Collisions2025_v1_0_0_xml`): `VLoose` 50, `Loose` 60, `Medium` 70, `Tight` 80,
`VTight` 90, `VVTight` 110, `VVVTight` 125, `VVVVTight` 135 (2024: 113, 116, 121, 127, 131 for `VLoose` to `VTight`).

The HLT paths seeded by the single-jet algorithms (`HLTrigger/Configuration/python/HLT_2025v10_cff.py`, the menu of
`HLT:@relval2025` that chapter 13 runs, the same seeds as `HLT_GRun_cff.py`; `L1SeedsLogicalExpression` of the
`hltL1s*` modules): `L1_ZeroBias` -> `HLT_PFJet40/60/80`, `HLT_PFJetFwd40/60`; `L1_SingleJet90` -> `HLT_PFJet110`,
`HLT_PFJet140`; `L1_SingleJet120` -> `HLT_PFJet200`; `L1_SingleJet60_FWD2p5`, `90_FWD2p5`, `120_FWD2p5` ->
`HLT_PFJetFwd80`, `140`, `200`; `L1_SingleJet160er2p5 OR L1_SingleJet180 OR L1_SingleJet200` -> `HLT_PFJet260` to
`550` and `HLT_PFJetFwd260` to `500`. Margins of the seed of the book, `L1_SingleJet90`: 136.0 - 90 = 46.0 GeV on the
bbar jet, 108.5 - 90 = 18.5 GeV on the b jet.

### What is quoted from chapter 11 (the signal alone, the digis)

This chapter has one sample; these numbers of the text come from the output of chapter 11 (`test/11_DIGI/out/`):

| number in the chapter | value | source |
|---|---|---|
| digis of the mixed event, of which tracker, muon | 544 407, 514 881, 2016 | `ledger_11_DIGI.txt` |
| collisions in bx -1, 0, +1 | 71, 68, 54 | `ledger_11_DIGI.txt` (the playback of the chapter 10 mixing) |
| primitives the signal alone would have sent | 620, 536.5 GeV (of 2479, 2793.0 GeV with the pileup) | `trigprim_11_DIGI.txt`, section 1 |
| primitives in the 9 x 9 towers of the bbar jet without pileup | 123.75 GeV (199.75 GeV with it) | `trigprim_11_DIGI.txt`, section 5 |
| noise: one tower in nine sends a count | 445 ECAL primitives of 4032 in library event 72 | `trigprim_11_DIGI.txt`, sections 1 and 4 |

### The conditions behind the emulation (section B5 of the chapter)

`conddb list 150X_mcRun3_2025_realistic_v1`, the L1 records and their tags: `L1TUtmTriggerMenuRcd`
`L1Menu_Collisions2025_v1_0_0_xml`; `L1TCaloParamsRcd` `L1TCaloParams_static_2024_v0_1_mc`;
`L1TGlobalPrescalesVetosRcd` `L1TGlobalPrescalesVetos_passThrough_mc`; `L1TMuonBarrelParamsRcd`
`L1TMuonBarrelParams_Stage2v1_2018_mc`; `L1TMuonOverlapParamsRcd` `L1TMuonOverlapParams_static_2023_mc`;
`L1TMuonEndCapParamsRcd` `L1TMuonEndCapParams_Run3_2023_mc`; `L1TMuonEndCapForestRcd`
`L1TMuonEndCapForest_v9_Run3_2023_mc`; `L1TMuonGlobalParamsRcd` `L1TMuonGlobalParams_Stage2v0_2024_mc_v1`; 19
`EcalTPG*Rcd` payloads (9 tagged `2025_mc`, 7 `2018_mc`, 3 without a year); `HcalTPParametersRcd`
`HcalTPParameters_2019_v1.0_mc`, `HcalLutMetadataRcd` `HcalLutMetadata_2019_v1.0_mc`, `HcalTPChannelParametersRcd`
`HcalTPChannelParameters_2024_v2.0_mc`. In the 2024 tag (`150X_mcRun3_2024_realistic_v2`) the menu was
`L1Menu_Collisions2024_v1_3_0_xml`, the `EcalTPG*` payloads 9 `2018_mc`, 5 `2024_mc`, 2 `2022_mc`, 3 without a year,
and `HcalTPChannelParameters_2022_v1.1_mc`; every other record above is the same in both tags. So the 2025 trigger of
the chapter is still assembled from payloads of several years: the calorimeter-trigger parameters `2024_v0_1` (the
release has `caloParams_2025_v0_1_cfi.py` and `caloParams_2025_v0_2_cfi.py` in python, not in the global tag), the
barrel muon parameters `Stage2v1_2018_mc`, the EMTF forest `Run3_2023`, seven ECAL trigger-primitive payloads of 2018.

The Phase-2 files quoted in B5, relative to `$CMSSW_RELEASE_BASE/src`:

| statement | file, line |
|---|---|
| the Phase-2 emulation is a copy of the Run-3 task | `L1Trigger/Configuration/python/SimL1Emulator_cff.py`: 62 (`_phase2_siml1emulator = SimL1EmulatorTask.copy()`), 278-279 (`phase2_trigger.toReplaceWith`); HF comment at 29-30 |
| the Run-3 calo, muon, global emulators are kept | `L1Trigger/L1TCalorimeter/python/simDigis_cff.py`: 46; `L1Trigger/L1TMuon/python/simDigis_cff.py`: 110-111; `L1Trigger/L1TGlobal/python/simDigis_cff.py`: 36 |
| no Phase-2 modifier in the HCAL primitives | `SimCalorimetry/HcalTrigPrimProducers/python/hcaltpdigi_cfi.py` (last modifiers `run3_HB`, `run3_common`) |
| Phase-2 ECAL primitive only with `phase2_ecalTP_devel` | `SimCalorimetry/Configuration/python/ecalDigiSequence_cff.py`: 34-37; the modifier is in no `Configuration/Eras/python/Era_*.py` |
| tower pileup subtraction of 2019 | `L1Trigger/L1CaloTrigger/python/l1tTowerCalibrationProducer_cfi.py`: 4, 34; `l1tPhase2CaloJetEmulator_cfi.py`: 7-20 (same numbers), 88 (jet calibration of June 2024) |
| the calorimeter HT that is always 0 | `L1Trigger/Configuration/python/SimL1Emulator_cff.py`: 107-111 (`l1tCaloJet`), 113-116 (`BXVCaloJetsInputTag = ("L1CaloJet", "CaloJets")`); `L1Trigger/L1CaloTrigger/plugins/L1CaloJetProducer.cc`: 341; `L1CaloJetHTTProducer.cc`: 76-84; the cfi default is `("l1tCaloJetProducer","L1CaloJetCollectionBXV")` |
| Phase-2 GT jets are PUPPI seeded-cone jets | `L1Trigger/Phase2L1GT/python/l1tGTProducer_cff.py`: `CL2JetsSC4 = l1tSC4PFL1PuppiCorrectedEmulator` |
| jet corrections of March 2022, SC8 with SC4 corrections | `L1Trigger/Phase2L1ParticleFlow/python/l1pfJetMet_cff.py`: 11-19 |
| hadron calibrations `110X` | `L1Trigger/Phase2L1ParticleFlow/python/pfClustersFromCombinedCalo_cff.py`: 30, 61; `l1tPFClustersFromHGC3DClusters_cfi.py`: 61 |
| PUPPI "ballpark", HF "to be tuned / to be defined" | `L1Trigger/Phase2L1ParticleFlow/python/l1ctLayer1_cff.py`: 77, 80 (barrel), 382, 389, 402-405 (HF) |
| one vertex from a histogram | `L1Trigger/VertexFinder/python/l1tVertexProducer_cfi.py`: 11, 41, 62-63, 82 |
| HGCAL trigger: no layer calibration, V9 trigger geometry, e/gamma ID `v10_3151` | `L1Trigger/L1THGCal/python/l1tHGCalBackEndLayer1Producer_cfi.py`: 30-34; `customTriggerGeometry.py`: 3-4; `egammaIdentification.py`: 102; era chain `Configuration/Eras/python/Era_Phase2C9_cff.py`: 7, `Era_Phase2C17I13M9_cff.py` |
| the Phase-2 menu | `Configuration/Applications/python/ConfigBuilder.py`: 1597; `L1Trigger/Configuration/python/Phase2GTMenus/step1_2024.py`: 1; `SeedDefinitions/step1_2024/`: 40 `algorithms.append` (7 hadronic and MET, 12 lepton, 7 + 9 cross, 5 in `l1tGTMenu_BTagSeeds_cff.py`, all multi-muon); `l1tGTObject_scalings.py`: 5, 55-69 |
| EMTF pT assignment | `L1Trigger/L1TMuonEndCap/src/EMTFSetup.cc`: 26-27; `python/simEmtfDigis_cfi.py`: 167 |
| calorimeter parameters in python | `L1Trigger/L1TCalorimeter/python/caloParams_2024_v0_1_cfi.py` (the payload of the global tag), `caloParams_2024_v0_3_cfi.py`, `caloParams_2025_v0_1_cfi.py`, `caloParams_2025_v0_2_cfi.py` |

### The 2024 and the 2025 settings

The chapter was written on 11 November 2026 with the 2024 settings (`auto:phase1_2024_realistic`, `Run3_2024`) and
rerun on 12 November 2026 with those of 2025 (Rule I of the Constitution), on the DIGI file of chapter 11 remade with
the 2025 settings. The 2024 outputs were kept aside and compared file by file. What changed for this chapter:

| what | 2024 | 2025 | where it acts |
|---|---|---|---|
| L1 menu (`L1TUtmTriggerMenuRcd`) | `L1Menu_Collisions2024_v1_3_0_xml`: 408 algorithms, 448 conditions | `L1Menu_Collisions2025_v1_0_0_xml`: 465, 481 (73 new, 16 gone) | step 6 |
| ECAL trigger-primitive payloads | 9 `2018_mc`, 5 `2024_mc`, 2 `2022_mc`, 3 without a year | 9 of the 19 replaced: `CrystalStatus`, `FineGrainStripEE`, `LinearizationConst` (`TL470fb_moy_2025_mc`), `LutIdMap`, `Pedestals` (`TL470fb_moy_2025_mc`), `PhysicsConst`, `Spike`, `StripStatus`, `TowerStatus` (`2025_mc`) | step 1, made in the DIGI job of chapter 11 |
| `HcalTPChannelParametersRcd` | `HcalTPChannelParameters_2022_v1.1_mc` | `HcalTPChannelParameters_2024_v2.0_mc` | step 1, DIGI job |
| CICADA model (`stage2L1Trigger_2025`) | `CICADAModel_v2p1p2` | `CICADAModel_v2p2p0` | step 2, `simCaloStage2Layer1Summary` |
| CSC shower thresholds (`run3_CSC_2025`) | `showerParam` = `showerPSet` | `showerParam` unchanged; `showerParams` = `showerPSet_2025` added, read by no module | none (and the event has no shower) |

The same in both: `L1TCaloParams_static_2024_v0_1_mc` (the `caloParams` lines of the two logs are identical), the five
muon payloads, the pass-through prescales, `HcalTPParameters_2019_v1.0_mc`, `HcalLutMetadata_2019_v1.0_mc`, the 21
modules of `SimL1Emulator` and every other parameter of `settings_12_L1T.txt`. The input changed as well: the digis and
the primitives of chapter 11 were made with the 2025 calorimeter conditions (chapter 11 compares them).

The event, step by step (`ledger_12_L1T.txt` of the two runs; GeV):

| | 2024 | 2025 |
|---|---|---|
| step 1: primitives with ET > 0, sum ET | 2417, 3411.5 | 2479, 2793.0 |
| ECAL EB: n, sum ET, largest | 594, 408.0, 5.0 | 683, 468.0, 5.5 |
| ECAL EE: n, sum ET, largest | 594, 1273.5, 11.5 | 494, 579.5, 7.0 |
| ECAL in tower rings 26-28 (both sides); ring +28 / -28 | 991.0; 315.0 / 304.0 | 321.5; 68.0 / 95.0 |
| HCAL HB / HE / HF: n, sum ET | 153, 129.5 / 335, 271.5 / 741, 1329.0 | 233, 183.0 / 328, 241.5 / 741, 1321.0 |
| step 2: Layer-1, ECAL in -> out | 1681.5 -> 985.5 | 1047.5 -> 740.5 |
| HCAL (HB + HE) in -> out; HF in -> out | 401.0 -> 353.5; 1329.0 -> 1102.0 | 424.5 -> 345.5; 1321.0 -> 1095.0 |
| all in -> out (towers with ET > 0) | 3411.5 -> 2441.0 (2535) | 2793.0 -> 2181.0 (2561) |
| rings 26-28 in -> out; barrel rings in -> out | 1154.0 -> 452.0; 541.5 -> 494.5 | 445.5 -> 134.5; 655.0 -> 576.5 |
| step 3: L1 jets (seeded in tower ring 28) | 12 (6) | 9 (0) |
| bbar jet: seed, window, phi rings, pileup, calibration, ET | 44.5, 190.5, 487, 60.5, 1.05, 137.0 | 45.0, 189.0, 482, 60.0, 1.05, 136.0 |
| b jet: seed, window, phi rings, pileup, calibration, ET | 14.0, 71.0, 158, 19.5, 2.07, 106.5 | 14.0, 74.5, 176.5, 22.0, 2.07, 108.5 |
| all jets: windows, pileup estimates, jet ET | 792.5, 500.5, 524.5 | 440.5, 229.5, 356.5 |
| e/gamma, tau: number, largest | 12 (15.5 at eta 2.52), 12 (59.5) | 12 (7.5 at eta -0.30), 12 (58.5) |
| ETT / ETTEM / HTT | 1339.0 / 965.5 / 106.5 | 1086.0 / 730.0 / 108.5 |
| ETM / ETMHF / HTM / HTMHF | 50.0 / 106.5 / 106.0 / 170.0 | 60.5 / 79.0 / 108.0 / 34.5 |
| steps 4 and 5: stubs, EMTF candidates, muon | 8 CSC + 4 RPC, 2, one of 3.0 GeV (the pileup kaon) | identical |
| step 6: algorithms fired (on an object; by the two signal jets alone) | 57 of 408 (49; 14) | 63 of 465 (55; 22) |
| VBF `DoubleJet...Mass`, `ETMHF...`, `HTMHF...` fired | 8, 8, 5 | 0, 2, 0 |
| CICADA score; thresholds of the menu | 181.7; 113, 116, 121, 127, 131 (5 fired) | 88.3; 50, 60, 70, 80, 90, 110, 125, 135 (4 fired) |
| `L1_SingleJet90`, the seed of `HLT_PFJet110`: margin on the bbar / b jet | 47.0 / 16.5 | 46.0 / 18.5 |

Why the jets of tower ring 28 are gone. In 2024 six of the twelve L1 jets were seeded in ring 28, in the pileup of
the last endcap rings; in 2025 the ECAL primitives of rings 26 to 28 hold a third of their 2024 energy, the largest is
7.0 GeV, and none of rings 27 and 28 passes the Layer-1 zero suppression (6 and 9 GeV, the same `L1TCaloParams` in both
years), so the towers of those rings carry only their HCAL energy. The cause is in the DIGI job of chapter 11, where
the primitives are made, and it is in two payloads that changed together. The ECAL digitiser divides the light of
every crystal by its laser correction (`SimCalorimetry/EcalSimAlgos/src/EcalHitResponse.cc`, line 186, with
`UseLCcorrection = True` in `ecalDigiParameters_cff.py`): the crystal delivers the fraction (p/ref)^alpha of its light,
p from `EcalLaserAPDPNRatios` (2024: `..._2024_outliers_cleaned_3sigma_mc`, 2025:
`..._TL470fb_moy_2025_outliers_cleaned_3sigma_mc`), ref and alpha from `EcalLaserAPDPNRatiosRef_mc` and
`EcalLaserAlphas_UL_Run1_Run2_2018_lastIOV_movedTo1` (the same in both tags). The trigger-primitive generator then
multiplies the amplitude of every crystal by its linearisation factor mult_x12 / 2^shift_x12 of
`EcalTPGLinearizationConst`. Averaged over the crystals of a tower ring (`conddb dump` of the payloads; for an endcap
crystal the ring follows from its x, y index at z = 317 cm, an approximation near the ring boundaries):

| tower ring | light delivered, 2024 | 2025 | linearisation factor, 2025 / 2024 | primitive per GeV deposited, 2025 / 2024 |
|---|---|---|---|---|
| 1 (barrel) | 0.809 | 0.701 | 1.087 | 0.94 |
| 17 (barrel) | 0.652 | 0.557 | 1.132 | 0.96 |
| 19 | 0.674 | 0.581 | 1.110 | 0.96 |
| 22 | 0.480 | 0.399 | 1.173 | 0.97 |
| 24 | 0.339 | 0.256 | 1.228 | 0.92 |
| 25 | 0.269 | 0.189 | 1.253 | 0.88 |
| 26 | 0.183 | 0.116 | 1.297 | 0.80 |
| 27 | 0.130 | 0.053 | 1.388 | 0.54 |
| 28 | 0.096 | 0.025 | 1.373 | 0.36 |

The 2025 linearisation follows the additional transparency loss of the 2025 laser payload to within about 6 % up to
ring 23, and not in the last rings: the same deposit gives in ring 28 a third of the primitive it gave in 2024, in
ring 27 a half. The noise of a crystal, in ADC counts, is not attenuated by the transparency, so it is multiplied by
the larger linearisation factor. Whether the TPG constants of the 2025 tag are meant to stop short at high eta is
not established (both payloads are named `TL470fb_moy`).

The CICADA score. Both the model and its input changed. The L1 job was run once more on each input with each model
(scratch jobs, only `simCaloStage2Layer1Summary` kept): 2024 input with `v2p1p2` 181.7, with `v2p2p0` 126.4; 2025
input with `v2p1p2` 127.7, with `v2p2p0` 88.3. Each change alone lowers the score by 30 %, and the 2025 menu lowered
the thresholds with the model.

The HCAL. The HB primitives of the event go from 153 (129.5 GeV) to 233 (183.0 GeV); the payloads behind them that
changed are `HcalTPChannelParameters` and, in the DIGI job, the HCAL pedestals, widths, response corrections, SiPM
parameters and zero-suppression levels of the 2025 tag. Which of them makes the extra primitives is not established.

What did not change: the muon (steps 4 and 5, line for line, and `truth_12_L1T.txt`), the arithmetic of the two
signal jets to 2 GeV, and the door into chapter 13: `L1_SingleJet90` fires in both, and the HLT seeds of the jet
paths are the same in `HLT_GRun_cff.py` and `HLT_2025v10_cff.py`.
