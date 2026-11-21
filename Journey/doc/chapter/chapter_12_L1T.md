# Chapter 12, L1T: technical companion

The Level-1 trigger of the ONE event, in six steps. One sample: the event as CMS sees it, the signal with its pileup
(PU = 60 per bunch crossing, the 16 bunch crossings -12..3 of the Run-3 production, 995 collisions; until 21 November
2026 bx -1..1, see "The move to the 16 bunch crossings" at the end of section 4). Chapter 11 (DIGI) is the last stage at which the signal and the pileup exist
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

### The 2024 comparison run (Rule I3)

```bash
./12_L1T/run_12_L1T_2024.sh            # after run_12_L1T.sh; REUSE=1 keeps the ROOT files of out2024/, ANALYZE=0 only the cmsRun jobs
```

It runs four `cmsDriver.py` configurations, written next to the script, with the output in `test/12_L1T/out2024/`
(remade on 21 November 2026 on the 16-crossing mixing of chapter 10, in two minutes):

| configuration | what it is |
|---|---|
| `step3_DIGI_mixed_2024_12_L1T_cfg.py` | the mixed DIGI of `run_11_DIGI.sh` (`-s DIGI:pdigi_valid`, the same `--pileup`, `--pileup_input` and playback lines) with `--conditions auto:phase1_2024_realistic --era Run3_2024` -> `step3_DIGI_mixed_11_DIGI.root` |
| `step4_L1_2024_12_L1T_cfg.py` | the L1 step of this chapter (the same three lines) with the 2024 settings on that file -> `step4_L1_12_L1T.root` |
| `step4_CICADA_input2024_v2p2p0_12_L1T_cfg.py` | the L1 step on the 2024 input with `process.simCaloStage2Layer1Summary.CICADAModelVersion = 'CICADAModel_v2p2p0'` (the model of 2025), keeping only `simCaloStage2Layer1Summary` -> `cicada_input2024_v2p2p0.root` |
| `step4_CICADA_input2025_v2p1p2_12_L1T_cfg.py` | the L1 step on the 2025 input of chapter 11 with the 2025 settings and the model of 2024, `CICADAModel_v2p1p2` -> `cicada_input2025_v2p1p2.root` |

then the analyzers, `l1tSettings.py`, `l1tTruth.py` and `l1tLedger.py` as `run_12_L1T.sh` does (the same file names, in
`out2024/`), and `test/12_L1T/yearCompare_12_L1T.py` (FWLite, `conddb list` and `conddb dump` of the ECAL payloads;
the dumps go to a temporary directory) -> `out2024/compare_12_L1T.txt`. The numbers are in section 4, "The 2024 and the
2025 settings".

## 2. The cmsRun on the python config

```bash
cd $J/test/12_L1T/out
cmsRun ../step4_L1_12_L1T_cfg.py > step4_L1_12_L1T.log 2>&1      # about 30 s, most of it reading the conditions
```

Or the whole chapter: `./12_L1T/run_12_L1T.sh` (49 s with the L1 job on 21 November 2026, under a minute when it is reused;
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

`edmDumpEventContent file:$J/test/12_L1T/out/step4_L1_12_L1T.root | grep '"L1"'` (31.0 MB: the 30.9 MB of the input,
most of it the `TrackingParticle`s of the 16 crossings, plus about 0.1 MB). The content of the chapter-11 file is copied. New under the process `L1`, in the order of the
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
  sampleLabel="chapter 12: the Level-1 trigger of 2025|bbbar, pTHat > 100 GeV, 13.6 TeV|with PU = 60 per bx, bx -12..3" \
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
`puEt` stored in the jet for all four jets of the event. The `Layer1 top tower` lines print, besides the energies,
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
| `l1t_12_L1T_calo_layer2` | step 3, the four jets on the towers |
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
menu L1Menu_Collisions2025_v1_0_0, 465 algorithms, 481 conditions; finalOR=1 fired=54
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
ECAL primitives, EB                   428        292.0          4.5
ECAL primitives, EE                   167        136.5          7.0
ECAL primitives, all                  595        428.5          7.0
HCAL primitives, HB                   190        155.0         15.5
HCAL primitives, HE                   225        156.0         10.5
HCAL primitives, HF                   705       1064.5         89.0
HCAL primitives, all                 1120       1375.5         89.0
Layer-1 towers, barrel                482        383.0         15.5
Layer-1 towers, endcap                305        242.0         14.5
Layer-1 towers, forward              1012        818.0         44.5
Layer-1 towers, all                  1799       1443.0         44.5
all primitives (of 8856)             1715       1804.0
what Layer-1 changes (sum ET in -> out, difference):
  ECAL (EB + EE)              428.5 ->    379.0 (-49.5)
  HCAL (HB + HE)              311.0 ->    246.0 (-65.0)
  HF                         1064.5 ->    818.0 (-246.5)
  all                        1804.0 ->   1443.0 (-361.0)
Layer1 top tower 0: ieta=-34 iphi=11 et=44.5 ecalTP=0 hcalTP=89 calibratedEm=0 calibratedHad=0 | tower word: et=89 ratio=0 flags=5
Layer1 top tower 1: ieta=-34 iphi=12 et=44.5 ecalTP=0 hcalTP=89 calibratedEm=0 calibratedHad=0 | tower word: et=89 ratio=0 flags=5
Layer1 top tower 2: ieta=8 iphi=68 et=15.5 ecalTP=0 hcalTP=15.5 calibratedEm=0 calibratedHad=15.5 | tower word: et=31 ratio=0 flags=1
Layer1 top tower 3: ieta=-18 iphi=50 et=14.5 ecalTP=3.5 hcalTP=10.5 calibratedEm=4 calibratedHad=10.5 | tower word: et=29 ratio=1 flags=0
Layer1 top tower 4: ieta=-19 iphi=48 et=11 ecalTP=7 hcalTP=4 calibratedEm=7 calibratedHad=4 | tower word: et=22 ratio=0 flags=2
Layer1 top tower 5: ieta=-33 iphi=11 et=6.5 ecalTP=0 hcalTP=13 calibratedEm=0 calibratedHad=0 | tower word: et=13 ratio=0 flags=5
Layer1 top tower 6: ieta=-33 iphi=12 et=6.5 ecalTP=0 hcalTP=13 calibratedEm=0 calibratedHad=0 | tower word: et=13 ratio=0 flags=5
Layer1 top tower 7: ieta=-19 iphi=50 et=5.5 ecalTP=5 hcalTP=0 calibratedEm=5.5 calibratedHad=0 | tower word: et=11 ratio=0 flags=3
Layer1 top tower 8: ieta=-33 iphi=13 et=5.5 ecalTP=0 hcalTP=11 calibratedEm=0 calibratedHad=0 | tower word: et=11 ratio=0 flags=5
Layer1 top tower 9: ieta=-33 iphi=14 et=5.5 ecalTP=0 hcalTP=11 calibratedEm=0 calibratedHad=0 | tower word: et=11 ratio=0 flags=5
```

Derived for the chapter: the barrel (EB + HB) holds 292.0 + 155.0 = 447.0 GeV of primitives (447.5 by tower ring,
section 2, where HB counts up to |ieta| 17), the endcaps (EE + HE) 136.5 + 156.0 = 292.5 GeV, the HF 1064.5 of the
1804.0 (59 %). No tower of tower rings 26 to 28 is among the ten largest Layer-1 towers. The primitives of the two jet
windows tower by tower (FWLite on `step4_L1_12_L1T.root`, `simEcalTriggerPrimitiveDigis` and
`simHcalTriggerPrimitiveDigis`, 0.5 GeV per count, the 9 x 9 towers around the seed): b jet, seed (-18,50), ECAL 16
primitives, 32.5 GeV, the largest (-19,48) 7.0, (-19,49) 5.0, (-19,50) 5.0, (-18,50) 3.5; HCAL 10 primitives, 23.0 GeV,
the largest (-18,50) 10.5, (-19,48) 4.0, (-18,48) 3.5; bbar jet, seed (-34,12), HF primitives (-34,11) 89.0, (-33,11)
13.0, (-33,13) 11.0, (-34,13) 9.0 (the analyzer, which shares an HF primitive among the towers it covers, gives 178.75
GeV in the window). Around the generator axes chapter 11 (`trigprim_11_DIGI.txt`, section 5) has, for the signal alone
and with the pileup: b jet ECAL 29.0 and 33.5, HCAL 22.5 and 26.5 GeV; bbar jet 123.75 and 178.75 GeV. In a
`Layer1 top tower` line `et`, `ecalTP`, `hcalTP`, `calibratedEm`, `calibratedHad` are in GeV; the tower word is in
counts: `et=29` is 14.5 GeV; `flags` is the upper four bits of the word (bit 12 of the word = 1, 13 = 2, 14 = 4,
15 = 8), so `flags=3` is bits 12 and 13 and `flags=5` bits 12 and 14. The tower (-19,50), `ecalTP=5 hcalTP=0 calibratedEm=5.5`,
has `flags=3`.

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

Ring by ring (the bar chart of the chapter is drawn from this table: one grey bar per ring for ecalTP + hcalTP, one
coloured bar for layer1, 0.176 cm per ring and 0.026 cm per GeV). Rings 26 to 28 (both sides) receive 141.0 GeV, 59.0
of it from the ECAL, and keep 89.5 GeV: of their ECAL only ring -26 (4.0 GeV) and ring 26 (3.5 GeV) keep something;
in rings 27 and 28 the Layer-1 sum equals the HCAL sum (66.0 GeV), the largest ECAL primitive of these rings having
2.0 GeV (`out2024/compare_12_L1T.txt`, section (2)), far below the 6 GeV (27) and 9 GeV (28) under which the scale factor
is 0. The barrel rings (|ieta| <= 17) receive 447.5 GeV and keep 383.0 GeV; the endcap rings 292.0 and 242.0; the HF
1064.5 and 818.0:

```
-- 2. Ring by ring (72 towers in phi): ECAL primitives + HCAL primitives -> Layer-1 towers, sum ET in GeV
 ieta     ecalTP     hcalTP     layer1
  -41        0.0       29.5       14.0
  -40        0.0       32.0       16.0
  -39        0.0       30.0       20.0
  -38        0.0       31.5       23.0
  -37        0.0       39.0       26.0
  -36        0.0       48.0       40.0
  -35        0.0       49.0       40.0
  -34        0.0      145.5      136.0
  -33        0.0       81.0       73.0
  -32        0.0       40.5       31.0
  -31        0.0       50.5       42.0
  -30        0.0       28.5       18.0
  -28       17.0       26.0       26.0
  -27        9.0       15.0       15.0
  -26        9.5       12.0       16.0
  -25       10.0        9.0       19.0
  -24        6.0        4.0       10.0
  -23        4.0       13.0       17.0
  -22        3.5        1.0        4.5
  -21        1.5        1.0        2.5
  -20        1.5        6.0        7.5
  -19       22.0        5.5       28.5
  -18        7.5       15.5       23.5
  -17        8.5        0.5        9.0
  -16        6.0        3.0        9.0
  -15        5.5       14.0       17.0
  -14        3.0        6.5        6.0
  -13        3.0        2.5        3.0
  -12        8.5        2.0        8.5
  -11        5.0        2.0        5.0
  -10        9.5        1.5        9.5
   -9       11.0        2.0       12.0
   -8        6.5        5.0       10.0
   -7        9.0        5.5       12.5
   -6       13.0        3.5       14.0
   -5       10.5        2.0       10.5
   -4       14.5        4.5       15.5
   -3        9.0        7.0       10.5
   -2       14.5        4.0       14.5
   -1        8.5        2.0        8.5
    1       12.5        4.5       13.5
    2        9.0        3.0        9.0
    3        5.0        4.0        7.0
    4       12.0        3.0       14.0
    5       13.5        7.0       19.5
    6       14.0        2.5       15.0
    7        8.5        4.0       11.5
    8       11.0       21.5       30.5
    9        7.0        4.0        8.0
   10       16.0        4.0       18.0
   11       11.0        2.5       12.0
   12        8.5        4.5       10.5
   13        5.0        7.5       11.0
   14        5.0        2.5        7.0
   15        1.5        8.0        9.0
   16        3.0        5.5        8.5
   17        4.0        0.0        4.0
   18        1.0        0.5        1.5
   19        2.0        0.5        2.5
   20        2.5        2.5        5.0
   21        1.0        0.0        1.0
   22        5.0        0.0        5.0
   23        4.0        2.0        6.0
   24        3.0        5.0        8.0
   25        3.0        8.0       11.0
   26       11.5        4.0        7.5
   27        9.0        6.0        6.0
   28        3.0       19.0       19.0
   30        0.0       24.5       14.0
   31        0.0       41.5       31.0
   32        0.0       43.5       36.0
   33        0.0       45.0       37.0
   34        0.0       34.5       26.0
   35        0.0       41.5       33.0
   36        0.0       44.0       36.0
   37        0.0       38.0       28.0
   38        0.0       38.0       31.0
   39        0.0       39.0       29.0
   40        0.0       39.0       22.0
   41        0.0       31.0       16.0
```

### Step 3 (`ledger_12_L1T.txt`, sections 3 and 4)

```
-- 3. Step 3: the Layer-2 jets (GeV). window = sum of the 9 x 9 Layer-1 towers; pileup = the estimate subtracted;
      calib = the calibration factor; ET = (window - pileup) x calib; ecalTP / hcalTP = the primitives in the window
4 L1 jets; GenJets: pt 117.8 eta -1.59 phi -2.08, pt 115.9 eta -3.75 phi 1.01
       ET    eta    phi  ieta  iphi   seed  window  pileup  win-pu  calib  ecalTP  hcalTP   GenJet
    122.5  -3.74   1.00   -34    12   44.5   170.0    53.5   116.5   1.05     0.0   178.8    115.9
          phi rings of the 9 eta columns of the window (72 towers each): 23 26 40 40 136 73 31 42 18; sum 429; x 9/72 = 53.5 (the emulator: 53.5)
     93.5  -1.52  -1.98   -18    50   14.5    56.0    13.0    43.0   2.17    32.5    23.0    117.8
          phi rings of the 9 eta columns of the window (72 towers each): 4.5 2.5 7.5 28.5 23.5 9 9 17 6; sum 107.5; x 9/72 = 13 (the emulator: 13.0)
     39.0   0.65  -0.41     8    68   15.5    33.0    17.0    16.0   2.44    16.5    19.0        -
     28.0   0.39  -2.85     5    40    5.0    26.5    16.0    10.5   2.67    22.0     8.5        -
   all jets: sum of the windows 285.5, of the pileup estimates 99.5, of the jet ET 283.0

-- 4. Step 3: what Layer-2 sends to the global trigger
jets 4, e/gamma 9, taus 12
ETT (total ET, towers |ieta| <= 28)         625.0
ETTEM (ECAL only)                           377.0
HTT (jets > 30 GeV, |ieta| <= 26)           132.5
ETM                                          48.0
ETMHF (with HF)                              78.5
HTM                                         101.0
HTMHF                                        66.0
tower count                                 787.0
L1EG 0: et=6 eta=0.435 phi=-2.84669 iso=0 hwQual=7
L1EG 1: et=4.5 eta=-2.3925 phi=-2.84669 iso=3 hwQual=7
L1EG 2: et=3.5 eta=2.3925 phi=-0.0626853 iso=3 hwQual=7
L1EG 3: et=3 eta=0.87 phi=3.0885 iso=0 hwQual=7
L1EG 4: et=3 eta=0.9135 phi=2.3055 iso=0 hwQual=7
L1EG 5: et=3 eta=-0.3045 phi=1.2615 iso=3 hwQual=7
L1EG 6: et=2.5 eta=-1.305 phi=-0.845685 iso=3 hwQual=7
L1EG 7: et=2.5 eta=-0.7395 phi=-2.67269 iso=3 hwQual=7
L1EG 8: et=2.5 eta=-0.348 phi=2.0445 iso=3 hwQual=7
L1Tau 0: et=54 eta=-1.566 phi=-1.93319 iso=1
L1Tau 1: et=25 eta=0.6525 phi=-0.410685 iso=0
L1Tau 2: et=20 eta=0.435 phi=-2.84669 iso=1
L1Tau 3: et=11 eta=-1.218 phi=0.957 iso=1
L1Tau 4: et=10 eta=1.305 phi=2.9145 iso=1
L1Tau 5: et=7.5 eta=-0.609 phi=0.783 iso=1
L1Tau 6: et=5.5 eta=0.87 phi=3.132 iso=1
L1Tau 7: et=5.5 eta=-0.348 phi=2.001 iso=1
L1Tau 8: et=5.5 eta=0.522 phi=0.0435 iso=1
L1Tau 9: et=5 eta=-1.305 phi=-0.802185 iso=1
L1Tau 10: et=5 eta=1.044 phi=2.001 iso=1
L1Tau 11: et=5 eta=-2.088 phi=1.479 iso=1
```

The generator jets and the directions of the sums (`l1t_12_L1T.log`):

```
  GenJet: pt=117.788 eta=-1.58908 phi=-2.08403 energy=301.021
  GenJet: pt=115.944 eta=-3.74635 phi=1.00816 energy=2457.41
  L1Sums: ETT=625 ETTEM=377 HTT=132.5 ETM=48 ETMphi=0.957 ETMHF=78.5 ETMHFphi=-2.28119 HTM=101 HTMphi=1.5225 HTMHF=66 towerCount=787 minBiasHFP0=15 minBiasHFM0=15 minBiasHFP1=0 minBiasHFM1=0
```

Sums of the chapter's ledger table: windows of the four jets 285.5 GeV; pileup estimates 99.5 GeV; jet ET 283.0 GeV,
of which the two signal jets 122.5 + 93.5 = 216.0 GeV. The b jet: 56.0 / 8 = 7.0 GeV of its own window in its pileup
estimate of 13.0 GeV; window / visible jet 56.0 / 79.8 = 0.70; 93.5 / 117.8 = 0.79, 93.5 / 79.8 = 1.17; dR to the
generator jet sqrt((1.5225 - 1.5891)^2 + (1.9767 - 2.0840)^2) = 0.13. The bbar jet: 122.5 / 115.9 = 1.06. The two other
jets, 39.0 GeV at (0.65, -0.41) and 28.0 GeV at (0.39, -2.85), are in the barrel; none is in the endcap or seeded in
tower ring 28. The seed of the first, (8,68), is an HB primitive of 15.5 GeV with nothing in the samples before it
(`trigprim_11_DIGI.txt`, section 6: `hcalSamples=0,0,15.5,1`, the third the sample of interest); the seed of the
second, (5,40), is the barrel tower that chapter 11 found flagged high interest (ECAL primitive 4.5 GeV). HTT = 93.5 +
39.0 = 132.5 GeV (jets above 30 GeV within |ieta| <= 26). HTMHF: the vector sum of (122.5, phi 1.0005), (93.5, -1.9767)
and (39.0, -0.4107) is 65.0 GeV (the trigger: 66.0), of the two signal jets alone 33.9 GeV. Invariant masses, M^2/2 =
ET1 ET2 (cosh(deta) - cos(dphi)): bbar jet and 39.0 GeV jet 192 600 GeV^2, M = 620.6 GeV; the two signal jets 64 570
GeV^2, M = 359.4 GeV; b jet and 39.0 GeV jet 180.2 GeV; bbar jet and 28.0 GeV jet (below 35 GeV for the mass
conditions) 468.1 GeV. So `DoubleJet_70_35_DoubleJet35_Mass_Min500/550/600_ETMHF65` fire on the bbar jet and the 39.0
GeV pileup jet (MASS_MIN_600 = 180 000 GeV^2), `Min700` fails, and every `DoubleJet_110_35_..._Mass_Min800` and above
fails.

The generator truth quoted with the jets is that of the earlier chapters, read from the same file: the b jet has
117.8 GeV with its neutrino and 79.8 GeV without (`ak4GenJetsNoNu`, chapter 07); the neutrino is the missing
transverse energy of the event, 37.5 GeV at phi = -2.10 (`genMetTrue`, chapter 06).

### Steps 4 and 5 (`ledger_12_L1T.txt`, section 5; `truth_12_L1T.txt`)

```
-- 5. Steps 4 and 5: the muon trigger
DT phi segments                                   0
DT theta segments                                 0
CSC anode patterns (ALCT)                        10
CSC cathode patterns (CLCT)                       9
CSC stubs (LCT)                                   5
GEM pads                                         72
GEM pad clusters                                 44
stubs given to the EMTF                           5
   of which neighbour copies                      0
   of which copies with no own-sector twin        0
distinct stubs (drawn)                            5
EMTF tracks                                       0
BMTF (Kalman) candidates, all bx / bx 0         0/0
OMTF candidates                                 0/0
EMTF candidates                                 0/0
uGMT muons                                        0
muon showers                                      0
EMTF input stubs: all=5 neighbourCopies=0 copiesWithoutTwin=0 distinct=5 CSC=5 (bx0 0) RPC=0 (bx0 0) GEM=0 (bx0 0)
EMTFHit: subsystem=CSC endcap=1 station=1 ring=1 sector=1 chamber=4 bx=-1 neighbour=0 eta=2.46164 phi=0.514006 rho=100.764 z=586.38 quality=3 pattern=2
EMTFHit: subsystem=CSC endcap=1 station=1 ring=1 sector=1 chamber=3 bx=1 neighbour=0 eta=1.99501 phi=0.385331 rho=170.637 z=615.68 quality=3 pattern=2
EMTFHit: subsystem=CSC endcap=1 station=1 ring=1 sector=1 chamber=5 bx=1 neighbour=0 eta=2.01729 phi=0.728477 rho=166.741 z=615.68 quality=7 pattern=3
EMTFHit: subsystem=CSC endcap=-1 station=1 ring=1 sector=1 chamber=7 bx=-1 neighbour=0 eta=-2.09847 phi=1.03464 rho=153.325 z=-615.68 quality=5 pattern=2
EMTFHit: subsystem=CSC endcap=-1 station=1 ring=1 sector=1 chamber=6 bx=1 neighbour=0 eta=-1.96951 phi=0.834962 rho=166.879 z=-586.38 quality=7 pattern=4
truth: genMuons(pt>1)=0 genJets(pt>30)=2
```

The EMTF received 5 stubs, all CSC, all in the inner ring of the first station (ME1/1, `station=1 ring=1`), in
sector 1 of both endcaps, none of them in bunch crossing 0 (two in -1, three in +1); no neighbour copy, no RPC and no
GEM stub. No two stubs are in different stations, so the EMTF builds no track (a track needs two stations), the
BMTF and the OMTF have no input, and the uGMT sends no muon. Steps 4 and 5 are identical, line for line, to those of
the 2024 run (`out2024/ledger_12_L1T.txt`, section 5). In the chapter the stubs are listed with |z| and r = `rho` rounded
to the centimetre. `L1TMuonAnalyzer` counts and draws a neighbour copy only when no stub of the same chamber, bunch
crossing and position arrives in its own sector. EMTF mode: bit 8 ME1, 4 ME2, 2 ME3, 1 ME4.

The truth near the stubs (FWLite on `step4_L1_12_L1T.root`, `std::vector<TrackingParticle>` `mix:MergedTrackTruth`;
for each stub, the charged particles of the same bunch crossing (`eventId().bunchCrossing()`) with pT > 0.8 GeV and at
least one hit outside the tracker (`numberOfHits() - numberOfTrackerHits() > 0`) within dR < 0.3 in (eta, phi) at the
vertex; done with a few lines of python in a scratch area, not part of the chapter scripts):

```
TrackingParticles 211377; by bx: -12:14528 -11:14729 -10:15026 -9:11503 -8:11141 -7:9542 -6:18800 -5:14691 -4:12132 -3:13077 -2:13691 -1:11345 0:13384 1:15425 2:11416 3:10947
muons pT > 1 GeV: 52; by bx: -5: 9, 0: 4, -11: 4, -8: 4, -7: 4, -6: 4, 1: 4, -12: 3, -10: 3, -9: 3, -4: 3, -3: 3, 3: 3, -1: 1
stub eta 2.46 phi 0.51 bx -1: (none)
stub eta 2.00 phi 0.39 bx 1: pdg -13 pT 1.25 bx 1 coll 24 muonHits 8 dR 0.20 (produced at r 92.6 cm, z 282.9 cm)
stub eta 2.02 phi 0.73 bx 1: pdg 13 pT 1.17 bx 1 coll 31 muonHits 8 dR 0.20 (produced at r 102.0 cm, z 315.0 cm)
stub eta -2.10 phi 1.03 bx -1: (none)
stub eta -1.97 phi 0.83 bx 1: (none)
```

The kaon muon of the version with three crossings (below) is the TrackingParticle `mu pT 2.78 eta -2.08 phi -3.04 |
bx -11 coll 21 | vertex r 58.5 z -214.4 | 35 hits, 6 in the tracker`: library event 92, in time in the draw of three
crossings (library events 72-139), is in bunch crossing -11 in the draw of 16 (library events 72-139 again: the draw
fills the earliest crossing first, chapter 10). The four muons above 1 GeV of bunch crossing 0 have 1.81, 1.70 (a decay
in flight in the signal, collision 0, one tracker hit), 1.07 and 1.01 GeV; none made a stub.

The truth behind the L1 muons (`truth_12_L1T.txt`, in full: there is none):

```
== l1tTruth 12_L1T
0 L1 muon(s); 211377 TrackingParticles
```

The version with three crossings (`test/tmp/out_bx-1to1/12_L1T/out/`, 12 November 2026) had one L1 muon, the kaon
muon above; its EMTF tracks and candidates and the uGMT muon, quoted in step 5 of the chapter (table 12.8):

```
  EMTFTrack: endcap=-1 sector=3 mode=12 bx=0 pt=2 eta=-2.08277 phi=-2.87514 charge=-1 nHits=2 quality=7
  EMTFTrack: endcap=-1 sector=4 mode=15 bx=0 pt=3 eta=-2.08277 phi=-2.87514 charge=-1 nHits=4 quality=15
  TFCand: tf=EMTF bx=0 processor=2 pt=2 eta=-2.088 phi=-2.87979 quality=7 hwPt=5 hwEta=-192 hwPhi=96 sign=1
  TFCand: tf=EMTF bx=0 processor=3 pt=3 eta=-2.088 phi=-2.87979 quality=15 hwPt=7 hwEta=-192 hwPhi=0 sign=1
  uGMT muon: bx=0 pt=3 eta=-2.088 phi=-2.87989 charge=-1 quality=15 iso=3 etaAtVtx=-2.06625 phiAtVtx=3.09787 tfMuonIndex=99
```

`pt = (hwPt - 1) x 0.5 GeV`, `eta = hwEta x 0.010875`, global phi = (processor x 96 + hwPhi + 24) x 2 pi / 576 for the
OMTF and EMTF (`l1t::MicroGMTConfiguration::calcGlobalPhi`). The extrapolation to the vertex moved the muon from
phi = -2.880 to 3.098, that is by 2 pi - 2.880 - 3.098 = 0.31.

### Step 6 (`ledger_12_L1T.txt`, section 6)

```
-- 6. Step 6: the decision of the global trigger
FinalOR 1; 54 of 465 algorithms fire (54 after prescales and masks); CICADA score: 55.0781
uGT inputs (bx 0): jets=4 egammas=9 taus=12 muons=0 etSums=19 | ETT=625 ETTEM=377 HTT=132.5 ETM=48 ETMHF=78.5 HTM=101 HTMHF=66
the 54 algorithms that fire, with their expression:
   296 L1_SingleJet35                                             JET35
   298 L1_SingleJet60er1p3to2p5                                   JET60[JET-ETA_2p52] AND  NOT JET60[JET-ETA_1p3]
   301 L1_SingleJet60                                             JET60
   302 L1_SingleJet90                                             JET90
   303 L1_SingleJet120                                            JET120
   306 L1_SingleJet35er1p3                                        JET35[JET-ETA_1p3]
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
   369 L1_DoubleJet_70_35_DoubleJet35_Mass_Min500_ETMHF65         comb{JET70,JET35} AND mass_inv{JET35,JET35}[MASS_MIN_500] AND ETMHF65
   370 L1_DoubleJet_70_35_DoubleJet35_Mass_Min550_ETMHF65         comb{JET70,JET35} AND mass_inv{JET35,JET35}[MASS_MIN_550] AND ETMHF65
   371 L1_DoubleJet_70_35_DoubleJet35_Mass_Min600_ETMHF65         comb{JET70,JET35} AND mass_inv{JET35,JET35}[MASS_MIN_600] AND ETMHF65
   388 L1_HTT120er                                                HTT120
   400 L1_ETMHF70                                                 ETMHF70
   409 L1_ETMHF70_HTT60er                                         ETMHF70 AND HTT60
   428 L1_CICADA_VLoose                                           CICADA[CICADA-CSCORE_50]
   444 L1_SingleJet8_BptxAND                                      JET8 AND EXT_ZeroBias_BPTX_AND_VME
   445 L1_SingleJet12_BptxAND                                     JET12 AND EXT_ZeroBias_BPTX_AND_VME
   446 L1_SingleJet16_BptxAND                                     JET16 AND EXT_ZeroBias_BPTX_AND_VME
   447 L1_SingleJet20_BptxAND                                     JET20 AND EXT_ZeroBias_BPTX_AND_VME
   448 L1_SingleJet24_BptxAND                                     JET24 AND EXT_ZeroBias_BPTX_AND_VME
   449 L1_SingleJet8er1p3_BptxAND                                 JET8[JET-ETA_1p3] AND EXT_ZeroBias_BPTX_AND_VME
   450 L1_SingleJet12er1p3_BptxAND                                JET12[JET-ETA_1p3] AND EXT_ZeroBias_BPTX_AND_VME
   451 L1_SingleJet16er1p3_BptxAND                                JET16[JET-ETA_1p3] AND EXT_ZeroBias_BPTX_AND_VME
   452 L1_SingleJet20er1p3_BptxAND                                JET20[JET-ETA_1p3] AND EXT_ZeroBias_BPTX_AND_VME
   453 L1_SingleJet24er1p3_BptxAND                                JET24[JET-ETA_1p3] AND EXT_ZeroBias_BPTX_AND_VME
   454 L1_SingleJet8                                              JET8
   455 L1_SingleJet8er1p3                                         JET8[JET-ETA_1p3]
   456 L1_SingleJet8er2p13                                        JET8[JET-ETA_2p13]
   457 L1_SingleEG5_BptxAND                                       EG5 AND EXT_ZeroBias_BPTX_AND_VME
   459 L1_SingleEG5                                               EG5
   461 L1_SingleEG5er1p52                                         EG5[EG-ETA_1p52_HASHFIX]
   463 L1_DoubleEG2_BptxAND                                       comb{EG2,EG2} AND EXT_ZeroBias_BPTX_AND_VME
   464 L1_DoubleEG2er2p13_BptxAND                                 comb{EG2[EG-ETA_2p13],EG2[EG-ETA_2p13]} AND EXT_ZeroBias_BPTX_AND_VME
   465 L1_DoubleEG3_BptxAND                                       comb{EG3,EG3} AND EXT_ZeroBias_BPTX_AND_VME
   466 L1_DoubleEG3er2p13_BptxAND                                 comb{EG3[EG-ETA_2p13],EG3[EG-ETA_2p13]} AND EXT_ZeroBias_BPTX_AND_VME
   467 L1_DoubleEG4_BptxAND                                       comb{EG4,EG4} AND EXT_ZeroBias_BPTX_AND_VME
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
  L1_SingleJet35                                 | SingleJET_i218=1 (3 combinations): Jet[0](122.5,-3.74,1.00) or Jet[1](93.5,-1.52,-1.98)
  L1_SingleJet60er1p3to2p5                       | SingleJET_i131=1 (1 combination): Jet[1](93.5,-1.52,-1.98) | SingleJET_i470=0
  L1_SingleJet60                                 | SingleJET_i219=1 (2 combinations): Jet[0](122.5,-3.74,1.00) or Jet[1](93.5,-1.52,-1.98)
  L1_SingleJet90                                 | SingleJET_i220=1 (2 combinations): Jet[0](122.5,-3.74,1.00) or Jet[1](93.5,-1.52,-1.98)
  L1_SingleJet120                                | SingleJET_i221=1 (1 combination): Jet[0](122.5,-3.74,1.00)
  L1_SingleJet35er1p3                            | SingleJET_i343=1 (1 combination): Jet[2](39.0,0.65,-0.41)
  L1_SingleJet35er2p5                            | SingleJET_i224=1 (2 combinations): Jet[1](93.5,-1.52,-1.98) or Jet[2](39.0,0.65,-0.41)
  L1_SingleJet35_FWD2p5                          | SingleJET_i229=1 (1 combination): Jet[0](122.5,-3.74,1.00) | SingleJET_i230=0
  L1_SingleJet60_FWD2p5                          | SingleJET_i231=1 (1 combination): Jet[0](122.5,-3.74,1.00) | SingleJET_i232=0
  L1_SingleJet90_FWD2p5                          | SingleJET_i233=1 (1 combination): Jet[0](122.5,-3.74,1.00) | SingleJET_i234=0
  L1_SingleJet120_FWD2p5                         | SingleJET_i235=1 (1 combination): Jet[0](122.5,-3.74,1.00) | SingleJET_i236=0
  L1_SingleJet35_FWD3p0                          | SingleJET_i237=1 (1 combination): Jet[0](122.5,-3.74,1.00) | SingleJET_i238=0
  L1_SingleJet120_FWD3p0                         | SingleJET_i239=1 (1 combination): Jet[0](122.5,-3.74,1.00) | SingleJET_i240=0
  L1_SingleJet8erHE                              | SingleJET_i241=1 (1 combination): Jet[1](93.5,-1.52,-1.98)
  L1_SingleJet10erHE                             | SingleJET_i242=1 (1 combination): Jet[1](93.5,-1.52,-1.98)
  L1_SingleJet12erHE                             | SingleJET_i243=1 (1 combination): Jet[1](93.5,-1.52,-1.98)
  L1_DoubleJet_70_35_DoubleJet35_Mass_Min500_ETMHF65 | DoubleJET_i373=1 (6 combinations): Jet[0](122.5,-3.74,1.00) Jet[1](93.5,-1.52,-1.98) or Jet[0](122.5,-3.74,1.00) Jet[2](39.0,0.65,-0.41) | InvariantMass_i374=1 (2 combinations): Jet[0](122.5,-3.74,1.00) Jet[2](39.0,0.65,-0.41) or Jet[2](39.0,0.65,-0.41) Jet[0](122.5,-3.74,1.00) | SingleETMHF_i375=1 (1 combination): ETMHF
  L1_DoubleJet_70_35_DoubleJet35_Mass_Min550_ETMHF65 | DoubleJET_i373=1 (6 combinations): Jet[0](122.5,-3.74,1.00) Jet[1](93.5,-1.52,-1.98) or Jet[0](122.5,-3.74,1.00) Jet[2](39.0,0.65,-0.41) | InvariantMass_i382=1 (2 combinations): Jet[0](122.5,-3.74,1.00) Jet[2](39.0,0.65,-0.41) or Jet[2](39.0,0.65,-0.41) Jet[0](122.5,-3.74,1.00) | SingleETMHF_i375=1 (1 combination): ETMHF
  L1_DoubleJet_70_35_DoubleJet35_Mass_Min600_ETMHF65 | DoubleJET_i373=1 (6 combinations): Jet[0](122.5,-3.74,1.00) Jet[1](93.5,-1.52,-1.98) or Jet[0](122.5,-3.74,1.00) Jet[2](39.0,0.65,-0.41) | InvariantMass_i349=1 (2 combinations): Jet[0](122.5,-3.74,1.00) Jet[2](39.0,0.65,-0.41) or Jet[2](39.0,0.65,-0.41) Jet[0](122.5,-3.74,1.00) | SingleETMHF_i375=1 (1 combination): ETMHF
  L1_HTT120er                                    | SingleHTT_i270=1 (1 combination): HTT
  L1_ETMHF70                                     | SingleETMHF_i287=1 (1 combination): ETMHF
  L1_ETMHF70_HTT60er                             | SingleETMHF_i287=1 (1 combination): ETMHF | SingleHTT_i130=1 (1 combination): HTT
  L1_CICADA_VLoose                               | CicadaTrigger_i410=1 (0 combinations)
  L1_SingleJet8_BptxAND                          | SingleJET_i449=1 (4 combinations): Jet[0](122.5,-3.74,1.00) or Jet[1](93.5,-1.52,-1.98) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet12_BptxAND                         | SingleJET_i450=1 (4 combinations): Jet[0](122.5,-3.74,1.00) or Jet[1](93.5,-1.52,-1.98) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet16_BptxAND                         | SingleJET_i451=1 (4 combinations): Jet[0](122.5,-3.74,1.00) or Jet[1](93.5,-1.52,-1.98) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet20_BptxAND                         | SingleJET_i452=1 (4 combinations): Jet[0](122.5,-3.74,1.00) or Jet[1](93.5,-1.52,-1.98) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet24_BptxAND                         | SingleJET_i453=1 (4 combinations): Jet[0](122.5,-3.74,1.00) or Jet[1](93.5,-1.52,-1.98) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet8er1p3_BptxAND                     | SingleJET_i454=1 (2 combinations): Jet[2](39.0,0.65,-0.41) or Jet[3](28.0,0.39,-2.85) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet12er1p3_BptxAND                    | SingleJET_i455=1 (2 combinations): Jet[2](39.0,0.65,-0.41) or Jet[3](28.0,0.39,-2.85) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet16er1p3_BptxAND                    | SingleJET_i456=1 (2 combinations): Jet[2](39.0,0.65,-0.41) or Jet[3](28.0,0.39,-2.85) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet20er1p3_BptxAND                    | SingleJET_i457=1 (2 combinations): Jet[2](39.0,0.65,-0.41) or Jet[3](28.0,0.39,-2.85) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet24er1p3_BptxAND                    | SingleJET_i458=1 (2 combinations): Jet[2](39.0,0.65,-0.41) or Jet[3](28.0,0.39,-2.85) | SingleEXT_i302=1 (1 combination): External
  L1_SingleJet8                                  | SingleJET_i449=1 (4 combinations): Jet[0](122.5,-3.74,1.00) or Jet[1](93.5,-1.52,-1.98)
  L1_SingleJet8er1p3                             | SingleJET_i454=1 (2 combinations): Jet[2](39.0,0.65,-0.41) or Jet[3](28.0,0.39,-2.85)
  L1_SingleJet8er2p13                            | SingleJET_i459=1 (3 combinations): Jet[1](93.5,-1.52,-1.98) or Jet[2](39.0,0.65,-0.41)
  L1_SingleEG5_BptxAND                           | SingleEG_i460=1 (1 combination): EG[0](6.0,0.44,-2.85) | SingleEXT_i302=1 (1 combination): External
  L1_SingleEG5                                   | SingleEG_i460=1 (1 combination): EG[0](6.0,0.44,-2.85)
  L1_SingleEG5er1p52                             | SingleEG_i462=1 (1 combination): EG[0](6.0,0.44,-2.85)
  L1_DoubleEG2_BptxAND                           | DoubleEG_i464=1 (72 combinations): EG[0](6.0,0.44,-2.85) EG[1](4.5,-2.39,-2.85) or EG[0](6.0,0.44,-2.85) EG[2](3.5,2.39,-0.06) | SingleEXT_i302=1 (1 combination): External
  L1_DoubleEG2er2p13_BptxAND                     | DoubleEG_i465=1 (42 combinations): EG[0](6.0,0.44,-2.85) EG[3](3.0,0.87,3.09) or EG[0](6.0,0.44,-2.85) EG[4](3.0,0.91,2.31) | SingleEXT_i302=1 (1 combination): External
  L1_DoubleEG3_BptxAND                           | DoubleEG_i466=1 (30 combinations): EG[0](6.0,0.44,-2.85) EG[1](4.5,-2.39,-2.85) or EG[0](6.0,0.44,-2.85) EG[2](3.5,2.39,-0.06) | SingleEXT_i302=1 (1 combination): External
  L1_DoubleEG3er2p13_BptxAND                     | DoubleEG_i468=1 (12 combinations): EG[0](6.0,0.44,-2.85) EG[3](3.0,0.87,3.09) or EG[0](6.0,0.44,-2.85) EG[4](3.0,0.91,2.31) | SingleEXT_i302=1 (1 combination): External
  L1_DoubleEG4_BptxAND                           | DoubleEG_i467=1 (2 combinations): EG[0](6.0,0.44,-2.85) EG[1](4.5,-2.39,-2.85) or EG[1](4.5,-2.39,-2.85) EG[0](6.0,0.44,-2.85) | SingleEXT_i302=1 (1 combination): External
  L1_DoubleEG2                                   | DoubleEG_i464=1 (72 combinations): EG[0](6.0,0.44,-2.85) EG[1](4.5,-2.39,-2.85) or EG[0](6.0,0.44,-2.85) EG[2](3.5,2.39,-0.06)
  L1_DoubleEG2er2p13                             | DoubleEG_i465=1 (42 combinations): EG[0](6.0,0.44,-2.85) EG[3](3.0,0.87,3.09) or EG[0](6.0,0.44,-2.85) EG[4](3.0,0.91,2.31)
  L1_AlwaysTrue                                  | SingleEXT_i302=1 (1 combination): External | SingleEXT_i302=1 (1 combination): External
  L1_ZeroBias                                    | SingleEXT_i302=1 (1 combination): External
  L1_ZeroBias_copy                               | SingleEXT_i302=1 (1 combination): External
  L1_MinimumBiasHF0_AND_BptxAND                  | SingleMBT0HFP_i303=1 (1 combination): MinBiasHFP0 | SingleMBT0HFM_i304=1 (1 combination): MinBiasHFM0 | SingleEXT_i302=1 (1 combination): External
  L1_MinimumBiasHF0                              | SingleMBT0HFP_i303=1 (1 combination): MinBiasHFP0 | SingleMBT0HFM_i304=1 (1 combination): MinBiasHFM0
  L1_BptxOR                                      | SingleEXT_i294=1 (1 combination): External
  L1_BptxPlus                                    | SingleEXT_i305=1 (1 combination): External
  L1_BptxMinus                                   | SingleEXT_i306=1 (1 combination): External
the ladders: the event against the thresholds of the single-object families (GeV); margin = value - threshold
   SingleJet            value   122.5   highest passed: 120 (margin +2.5)   lowest failed: 180 (margin -57.5)
   SingleJet..er2p5     value    93.5   highest passed: 35 (margin +58.5)   lowest failed: 120 (margin -26.5)
   SingleJet.._FWD3p0   value   122.5   highest passed: 120 (margin +2.5)   lowest failed: none
   DoubleJet..er2p5     value    39.0   highest passed: none   lowest failed: 40 (margin -1.0)
   HTT..er              value   132.5   highest passed: 120 (margin +12.5)   lowest failed: 160 (margin -27.5)
   ETT                  value   625.0   highest passed: none   lowest failed: 2000 (margin -1375.0)
   ETMHF                value    78.5   highest passed: 70 (margin +8.5)   lowest failed: 80 (margin -1.5)
   SingleEG..er2p5      value     6.0   highest passed: none   lowest failed: 8 (margin -2.0)
   SingleTau..er2p1     value    54.0   highest passed: none   lowest failed: 120 (margin -66.0)
   SingleMu             value     0.0   highest passed: none   lowest failed: 3 (margin -3.0)
```

How the 54 are counted in the chapter. 8 do not look at any object (`L1_AlwaysTrue`, `L1_ZeroBias`, `L1_ZeroBias_copy`,
`L1_BptxOR`, `L1_BptxPlus`, `L1_BptxMinus`, and the two `L1_MinimumBiasHF0*`, which ask for one HF tower on each side);
46 do. Of these, 22 are satisfied by the two signal jets alone (`SingleJet35/60/90/120`, `SingleJet60er1p3to2p5`,
`SingleJet35er2p5`, the six `_FWD2p5` / `_FWD3p0`, the three `erHE`, `SingleJet8`, `SingleJet8er2p13` and the five
`SingleJet8/12/16/20/24_BptxAND`; `out2024/compare_12_L1T.txt`, section (6)); 24 need an object without a generator jet
or signal particle behind it: 7 barrel jets (`SingleJet35er1p3`, the 39.0 GeV jet alone; `SingleJet8er1p3` and
`SingleJet8/12/16/20/24er1p3_BptxAND`: the pileup jets of 39.0 GeV at eta = 0.65 and 28.0 GeV at eta = 0.39);
`HTT120er` (HTT = 93.5 + 39.0); the 3 `DoubleJet_70_35_DoubleJet35_Mass_Min500/550/600_ETMHF65` (the bbar jet and the
39.0 GeV jet, M = 621 GeV, and ETMHF 78.5 > 65); 2 missing-energy (`ETMHF70`, `ETMHF70_HTT60er`); 10 e/gamma
(`SingleEG5`, `SingleEG5er1p52`, `SingleEG5_BptxAND`, `DoubleEG2`, `DoubleEG2er2p13`, `DoubleEG2/3/4_BptxAND`,
`DoubleEG2/3er2p13_BptxAND`: the e/gamma candidates of 6.0 GeV at eta = 0.44, 4.5 GeV at eta = -2.39 and the ones
below them); `CICADA_VLoose` (score 55.08, threshold 50; `Loose`, 60, fails by 4.9). No muon algorithm fires. 25 of the
54 are algorithms the 2024 menu did not have (23 low-threshold jet and e/gamma algorithms, `SingleJet60er1p3to2p5` and
`DoubleJet_70_35_DoubleJet35_Mass_Min600_ETMHF65`); 26 of the other 29 fire in the 2024 run too, and `ETMHF70`,
`ETMHF70_HTT60er`, `CICADA_VLoose` only in 2025 (`out2024/compare_12_L1T.txt`, section (3)).

The thresholds of the ladders and three of the object maps as the analyzer prints them (`l1t_12_L1T.log`):

```
  ladder SingleJet: value=122.5 thresholds(fired): 8(1) 35(1) 60(1) 90(1) 120(1) 180(0) 200(0)
  ladder SingleJet..er2p5: value=93.5 thresholds(fired): 35(1) 120(0) 140(0) 160(0) 180(0)
  ladder SingleJet.._FWD3p0: value=122.5 thresholds(fired): 35(1) 120(1)
  ladder DoubleJet..er2p5: value=39 thresholds(fired): 40(0) 100(0) 120(0) 150(0)
  ladder HTT..er: value=132.5 thresholds(fired): 120(1) 160(0) 200(0) 255(0) 280(0) 320(0) 360(0) 400(0) 450(0)
  ladder ETT: value=625 thresholds(fired): 2000(0)
  ladder ETMHF: value=78.5 thresholds(fired): 70(1) 80(0) 90(0) 100(0) 110(0) 120(0) 130(0) 140(0) 150(0)
  ladder SingleEG..er2p5: value=6 thresholds(fired): 8(0) 10(0) 15(0) 26(0) 28(0) 34(0) 36(0) 38(0) 40(0) 42(0) 45(0)
  ladder SingleTau..er2p1: value=54 thresholds(fired): 120(0) 130(0)
  ladder SingleMu: value=0 thresholds(fired): 3(0) 5(0) 7(0) 18(0) 20(0) 22(0) 25(0)
  fired: bit=302 name=L1_SingleJet90 final=1 expression="JET90" | SingleJET_i220=1 (2 combinations): Jet[0](122.5,-3.74,1.00) or Jet[1](93.5,-1.52,-1.98)
  fired: bit=307 name=L1_SingleJet35er2p5 final=1 expression="JET35[JET-ETA_2p52]" | SingleJET_i224=1 (2 combinations): Jet[1](93.5,-1.52,-1.98) or Jet[2](39.0,0.65,-0.41)
  fired: bit=388 name=L1_HTT120er final=1 expression="HTT120" | SingleHTT_i270=1 (1 combination): HTT
```

Some conditions as the menu defines them (`l1t_12_L1T.log`, `condition:` lines, printed for the conditions that are
true; cut type 0 = threshold in GeV, 1 = eta window, 9 = the mass cut as M^2/2 in GeV^2, 11 = count, 25 = CICADA
score). `L1_SingleJet60er1p3to2p5` is `JET60[JET-ETA_2p52] AND NOT JET60[JET-ETA_1p3]`: the first condition is true for
the b jet, the second false (no jet above 60 GeV within |eta| < 1.305). `MASS_MIN_600` is stored as 180 000 = 600^2 / 2.
The external condition `EXT_ZeroBias_BPTX_AND_VME` of the `_BptxAND` algorithms is the beam pick-up, true in every
simulated event. (The muon conditions of the version with three crossings: quality masks 61440 = bits 12-15
(`MU-QLTY_SNGL`), 65280 = bits 8-15 (`DBLE`), 65520 = bits 4-15 (`OPEN`).)

```
  condition: name=SingleJET_i131 type=12 | object JET60 bx 0: JET-ET_60[type 0: 60..1.79769e+308] JET-ETA_2p52[type 1: -2.523..2.523]
  condition: name=SingleJET_i470 type=12 | object JET60 bx 0: JET-ET_60[type 0: 60..1.79769e+308] JET-ETA_1p3[type 1: -1.305..1.305]
  condition: name=SingleJET_i220 type=12 | object JET90 bx 0: JET-ET_90[type 0: 90..1.79769e+308]
  condition: name=SingleJET_i343 type=12 | object JET35 bx 0: JET-ET_35[type 0: 35..1.79769e+308] JET-ETA_1p3[type 1: -1.305..1.305]
  condition: name=SingleJET_i224 type=12 | object JET35 bx 0: JET-ET_35[type 0: 35..1.79769e+308] JET-ETA_2p52[type 1: -2.523..2.523]
  condition: name=DoubleJET_i373 type=13 | object JET70 bx 0: JET-ET_70[type 0: 70..1.79769e+308] | object JET35 bx 0: JET-ET_35[type 0: 35..1.79769e+308]
  condition: name=SingleETMHF_i375 type=32 | object ETMHF65 bx 0: ETMHF-ET_65[type 0: 65..1.79769e+308]
  condition: name=InvariantMass_i349 type=26 | object JET35 bx 0: JET-ET_35[type 0: 35..1.79769e+308] | object JET35 bx 0: JET-ET_35[type 0: 35..1.79769e+308] | condition cuts: MASS_MIN_600[type 9: 180000..1.15493e+10]
  condition: name=SingleHTT_i270 type=17 | object HTT120 bx 0: HTT-ET_120[type 0: 120..1.79769e+308]
  condition: name=SingleETMHF_i287 type=32 | object ETMHF70 bx 0: ETMHF-ET_70[type 0: 70..1.79769e+308]
  condition: name=CicadaTrigger_i410 type=75 | object CICADA bx 0: CICADA-CSCORE_50[type 25: 50..0]
  condition: name=SingleJET_i449 type=12 | object JET8 bx 0: JET-ET_8[type 0: 8..1.79769e+308]
  condition: name=SingleEXT_i302 type=20 | object EXT_ZeroBias_BPTX_AND_VME bx 0:
  condition: name=SingleEG_i460 type=4 | object EG5 bx 0: EG-ET_5[type 0: 5..1.79769e+308]
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
`550` and `HLT_PFJetFwd260` to `500`. Margins of the seed of the book, `L1_SingleJet90`: 122.5 - 90 = 32.5 GeV on the
bbar jet, 93.5 - 90 = 3.5 GeV on the b jet; of `L1_SingleJet120`: 122.5 - 120 = 2.5 GeV.

### What is quoted from chapter 11 (the signal alone, the digis)

This chapter has one sample; these numbers of the text come from the output of chapter 11 (`test/11_DIGI/out/`):

| number in the chapter | value | source |
|---|---|---|
| digis of the mixed event, of which tracker, muon | 475 110, 440 664 (PXB 44 725 + PXF 22 048 + TIB 79 535 + TID 30 739 + TOB 93 705 + TEC 169 912), 7611 (DT 24 + CSC strips 7216 + CSC wires 263 + RPC 7 + GEM 101) | `ledger_11_DIGI.txt` |
| collisions in the 16 bunch crossings -12..3 | 746 early, 64 in bx 0, 185 late (71 68 54 59 52 61 76 59 65 59 68 54, 64, 63 62 60) | `ledger_11_DIGI.txt` (the playback of the chapter 10 mixing) |
| the ECAL primitives of the signal alone (noise included) | 499, 328.5 GeV (595, 428.5 GeV with the pileup) | `trigprim_11_DIGI.txt`, section 1 |
| primitives the signal alone would have sent | 620, 536.5 GeV (of 1715, 1804.0 GeV with the pileup) | `trigprim_11_DIGI.txt`, section 1 |
| primitives in the 9 x 9 towers of the bbar jet without pileup | 123.75 GeV (178.75 GeV with it) | `trigprim_11_DIGI.txt`, section 5 |
| the 9 x 9 towers around the b-jet axis, ECAL / HCAL, without and with pileup | 29.0 / 22.5 GeV, 33.5 / 26.5 GeV | `trigprim_11_DIGI.txt`, section 5 |
| the seed of the 39.0 GeV pileup jet, HB (8,68): samples -2..+1 | 0, 0, [15.5], 1 GeV | `trigprim_11_DIGI.txt`, section 6 |
| the barrel tower flagged high interest | (5,40), 4.5 GeV | `trigprim_11_DIGI.txt`, section 3 |
| noise: one tower in nine sends a count | 443 ECAL primitives of 4032 in library event 747, which deposits 0.4 GeV in the ECAL | `trigprim_11_DIGI.txt`, sections 1 and 4 |

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

The Phase-2 files quoted in B5, all of the release CMSSW_15_0_5 (relative to `$CMSSW_RELEASE_BASE/src`, data files in
the `cms-data` externals of the release, global tags by `conddb list`), not of the development branch:

| statement | file, line |
|---|---|
| the Phase-2 setup of the release | `Configuration/Geometry/python/defaultPhase2ConditionsEra_cff.py`: 5 (`DEFAULT_VERSION = "Run4D110"`); `Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py`: 3171-3176 (`Run4D110`: era `Phase2C17I13M9`, GT `auto:phase2_realistic_T33`); tracker stubs: `L1Trigger/TrackTrigger/python/ProducerSetup_cfi.py`: 45 (`MinPtStub = 2.0`), 47 (`MaxEta = 2.5`) |
| the Phase-2 emulation is a copy of the Run-3 task | `L1Trigger/Configuration/python/SimL1Emulator_cff.py`: 62 (`_phase2_siml1emulator = SimL1EmulatorTask.copy()`), 278-279 (`phase2_trigger.toReplaceWith`); HF comment at 29-30 |
| the Run-3 calo, muon, global emulators are kept | `L1Trigger/L1TCalorimeter/python/simDigis_cff.py`: 46; `L1Trigger/L1TMuon/python/simDigis_cff.py`: 110-111; `L1Trigger/L1TGlobal/python/simDigis_cff.py`: 36; the Phase-2 era is `Run3_noMkFit` plus Phase-2 modifiers (`Configuration/Eras/python/Era_Phase2_cff.py`: 23-24) |
| no Phase-2 modifier in the HCAL primitives; who reads them | `SimCalorimetry/HcalTrigPrimProducers/python/hcaltpdigi_cfi.py`: 139-143 (last modifiers `run3_HB`, `run3_common`); readers `L1Trigger/L1CaloTrigger/python/l1tEGammaCrystalsEmulatorProducer_cfi.py`: 5, `l1tTowerCalibrationProducer_cfi.py`: 33, `l1tPhase2CaloJetEmulator_cfi.py`: 6, `L1Trigger/Phase2L1ParticleFlow/python/l1tPFClustersFromCombinedCalo_cfi.py`: 6, 8 (`hcalDigisHF = True`) |
| Phase-2 ECAL primitive only with `phase2_ecalTP_devel` | `SimCalorimetry/Configuration/python/ecalDigiSequence_cff.py`: 34-37 (task), 46-48 (its own ES producer); the modifier is in no `Configuration/Eras/python/Era_*.py`; only the workflows `ECALTPPh2`, `ECALTPPh2Component`, `ECALTPPh2ComponentFSW` append it to `--era` (`upgradeWorkflowComponents.py`: 1935, 1965-1984) |
| tower pileup subtraction of 2019 | `L1Trigger/L1CaloTrigger/python/l1tTowerCalibrationProducer_cfi.py`: 4, 34; `l1tPhase2CaloJetEmulator_cfi.py`: 7-20 (same numbers), 88 (jet calibration of June 2024) |
| the calorimeter HT that is always 0 | `L1Trigger/Configuration/python/SimL1Emulator_cff.py`: 107-111 (`l1tCaloJet`), 113-116 (`BXVCaloJetsInputTag = ("L1CaloJet", "CaloJets")`); `L1Trigger/L1CaloTrigger/plugins/L1CaloJetProducer.cc`: 341; `L1CaloJetHTTProducer.cc`: 78 (set to 0), 82-84 (`isValid()`); the cfi default is `("l1tCaloJetProducer","L1CaloJetCollectionBXV")`; kept by `L1Trigger/Configuration/python/L1Trigger_EventContent_cff.py`: 210 (`keep *_l1tCaloJetHTT_*_*`) |
| Phase-2 GT jets are PUPPI seeded-cone jets | `L1Trigger/Phase2L1GT/python/l1tGTProducer_cff.py`: 16 (`CL2JetsSC4 = l1tSC4PFL1PuppiCorrectedEmulator`), no calorimeter jet among the inputs 7-22 |
| jet corrections of March 2022, SC8 with SC4 corrections | `L1Trigger/Phase2L1ParticleFlow/python/l1pfJetMet_cff.py`: 11-19; `data/jecs/jecs_20220308.root` (external `data-L1Trigger-Phase2L1ParticleFlow` V00-07-00) has `L1PuppiSC4EmuJets` (INDEX, `eta_bin1-11`, `emul_eta_bin1-11`, `emul_corr_eta_bin1-11`) and `L1PuppiSC8EmuJets` (INDEX and `eta_bin1-11` only); the emulator (`HW = True`) reads `emul_eta_bin*`: `src/corrector.cc`: 78-80, 151-162 |
| hadron calibrations `110X`, chosen by `phase2_hgcalV11` | `L1Trigger/Phase2L1ParticleFlow/python/pfClustersFromCombinedCalo_cff.py`: 5 (the comment), 30, 61; `l1tPFClustersFromHGC3DClusters_cfi.py`: 61 |
| PUPPI "ballpark", HF "to be tuned / to be defined", HF cluster cut | `L1Trigger/Phase2L1ParticleFlow/python/l1ctLayer1_cff.py`: 77, 80 (barrel), 368 (`hadPtCut = 15.0`; 1.0 at 20, 141, 277), 382, 389, 402-405 (HF); the cut is applied in `plugins/L1TCorrelatorLayer1Producer.cc`: 500 |
| one vertex from a histogram | `L1Trigger/VertexFinder/python/l1tVertexProducer_cfi.py`: 11, 41, 57 (`VxMaxTrackPt = 127`), 62-63, 82 (`l1tVertexFinderEmulator`: `fastHistoEmulation`); `l1ctLayer1_cff.py`: `vtxCollection = l1tVertexFinderEmulator:L1VerticesEmulation` |
| HGCAL trigger: V16 settings, no layer calibration, V9 trigger geometry, e/gamma ID `v10_3151` | V16: `L1Trigger/L1THGCal/python/l1tHGCalTriggerGeometryESProducer_cfi.py`: 25-27, `hgcalLayersCalibrationCoefficients_cfi.py`: 137-139; `l1tHGCalBackEndLayer1Producer_cfi.py`: 30-34; `hgcalTriggerPrimitives_cff.py`: 26 and `customTriggerGeometry.py`: 3-4; `egammaIdentification.py`: 102, 202; era chain `Configuration/Eras/python/Era_Phase2C9_cff.py`: 7, `Era_Phase2C17I13M9_cff.py`: 7 |
| the Phase-2 menu | `Configuration/Applications/python/ConfigBuilder.py`: 1597; `L1Trigger/Configuration/python/Phase2GTMenus/step1_2024.py`: 1; `SeedDefinitions/step1_2024/`: 40 `algorithms.append` (7 hadronic and MET in `l1tGTMenu_hadr_metSeeds_cff.py`: 28, 40, 52 (`pDoublePuppiJet160_35_mass620`, `l1tGTsc4Jet_er5`, `minInvMass = 620`), 60, 67, 74, 100; 12 lepton, 7 + 9 cross, 5 in `l1tGTMenu_BTagSeeds_cff.py`, all multi-muon); `l1tGTObject_scalings.py`: 5, 55-69 |
| jet seeds and HT at abs(eta) < 2.4 | `SeedDefinitions/step1_2024/l1tGTMenuObjects_cff.py`: 30-36 (`l1tGTsc4Jet`, `minEta = -2.4`, `maxEta = 2.4`), 38-41 (`l1tGTsc4Jet_er5`); `L1Trigger/Phase2L1ParticleFlow/python/l1tMHTPFProducer_cfi.py`: 5-6 (`minJetPt = 30`, `maxJetEta = 2.4`); the scalings have a `forward` region (`l1tGTObject_scalings.py`: 65-68, `l1tGTObject_constants.py`: 14) |
| b tagging not passed to the GT | `L1Trigger/Phase2L1ParticleFlow/python/L1BJetProducer_cff.py`: 14-20; `SimL1Emulator_cff.py`: 269-270; `L1Trigger/Phase2L1GT/plugins/L1GTProducer.cc`: 370-390 (pT, eta, phi, z0; at most 12 jets, 373) |
| our event and the dijet-mass seed | generator jets `test/07_GenJet/out/ledger_07_GenJet.txt`: 29-30 (117.8 GeV at eta -1.59, phi -2.08, mass 16.7; 115.9 GeV at eta -3.75, phi 1.01, mass 7.3): invariant mass 387 GeV |
| the Phase-2 GT seen by the Run-3 emulators | `Configuration/AlCa/python/autoCond.py`: 111 (`phase2_realistic` = `150X_mcRun4_realistic_v1`); `autoCondPhase2.py`: 150-162 (`_T33` appends tracker records only); `conddb list 150X_mcRun4_realistic_v1`: `L1TUtmTriggerMenuRcd` `L1Menu_Collisions2023_v1_2_0_xml`, `L1TCaloParamsRcd` `L1TCaloParams_static_2022_v0_2_mc`, `L1TMuonBarrelParamsRcd` `L1TMuonBarrelParams_Stage2v1_2018_mc`, `L1TMuonGlobalParamsRcd` `L1TMuonGlobalParams_Stage2v0_2018_mc`, `L1TMuonEndCapParamsRcd` `L1TMuonEndCapParams_Run3_2022_mc`, `L1TMuonEndCapForestRcd` `L1TMuonEndCapForest_v8_Run3_2022_mc`, `L1TGlobalPrescalesVetosRcd` `L1TGlobalPrescalesVetos_passThrough_mc` |
| the ECAL payloads of the Phase-2 barrel primitives | `SimCalorimetry/EcalEBTrigPrimProducers/plugins/EcalEBTrigPrimProducer.cc`: 55-67 (ten records); in `150X_mcRun4_realistic_v1`: `beamv5_startup_mc` for `WeightIdMap`, `SlidingWindow`, `LutGroup`; `EcalTPGSpike_80X_2016_plus15fb_mc`; `EcalTPGLinearizationConst_UL_2018_mc_EE_BTCP_116_SIC_1`; `TL1000_upgradeTIA_8deg_mc` for `Pedestals`, `WeightGroup`, `LutIdMap`; `EcalTPGCrystalStatus_v2_mc`, `EcalTPGTowerStatus_startup_mc` |
| EMTF pT assignment (2025 and Phase-2) | `L1Trigger/L1TMuonEndCap/src/EMTFSetup.cc`: 26-27; `python/simEmtfDigis_cfi.py`: 167; `stage2L1Trigger_2021` is in `Configuration/Eras/python/Era_Run3_cff.py`: 9, 23, hence in `Run3_2025` and in `Phase2` |
| calorimeter parameters in python | `L1Trigger/L1TCalorimeter/python/caloParams_2024_v0_1_cfi.py`: 44-46 (the payload of the 2025 global tag; `lut_pt/eta_compress_2017v1`, `lut_calib_2023v0_ECALZS_PhiRing`), `caloParams_2024_v0_3_cfi.py`, `caloParams_2025_v0_1_cfi.py`, `caloParams_2025_v0_2_cfi.py` |

### The 2024 and the 2025 settings

The chapter was written on 11 November 2026 with the 2024 settings (`auto:phase1_2024_realistic`, `Run3_2024`) and
rerun on 12 November 2026 with those of 2025 (Rule I of the Constitution), on the DIGI file of chapter 11 remade with
the 2025 settings. The 2024 run is kept in the repository (since 13 November 2026): `test/12_L1T/run_12_L1T_2024.sh`
remakes the mixed DIGI of chapter 11 and the L1 step with `auto:phase1_2024_realistic` and `Run3_2024` on the same
SimHits and collisions (MixingModule playback) into `test/12_L1T/out2024/` (`step3_DIGI_mixed_11_DIGI.root`,
`step4_L1_12_L1T.root`, `l1t_12_L1T.log`, `ledger_12_L1T.txt`, `truth_12_L1T.txt`, `settings_12_L1T.txt`), runs the L1
step once more on each input with the CICADA model of the other year (`cicada_input2024_v2p2p0.root`,
`cicada_input2025_v2p1p2.root`), and compares the two years with `test/12_L1T/yearCompare_12_L1T.py`
(`out2024/compare_12_L1T.txt`: (1) the ledger lines that differ, (2) the ECAL primitives by tower ring, (3) the
algorithms that fire, (4) the CICADA scores, (5) the ECAL conditions by tower ring, (6) the menus and the fired
algorithms classified). Since 21 November 2026 both years are made with the 16 bunch crossings -12..3 (the run of 21
November, two minutes; the comparison with three crossings is in `test/tmp/out_bx-1to1/12_L1T/out2024/`). Every 2024
number below and in the chapter is in these files. What changed for this chapter:

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

The event, step by step (`out2024/ledger_12_L1T.txt` and `out/ledger_12_L1T.txt`; `out2024/compare_12_L1T.txt`; GeV;
both years with the 16 crossings):

| | 2024 | 2025 |
|---|---|---|
| step 1: primitives with ET > 0, sum ET | 1529, 1701.5 | 1715, 1804.0 |
| ECAL EB: n, sum ET, largest | 304, 202.5, 4.0 | 428, 292.0, 4.5 |
| ECAL EE: n, sum ET, largest | 143, 158.0, 7.0 | 167, 136.5, 7.0 |
| ECAL in tower rings 26-28 (both sides): n, sum, largest; ring +28 / -28 | 53, 78.0, 4.0; 13.0 / 41.0 | 74, 59.0, 3.5; 3.0 / 17.0 |
| HCAL HB / HE / HF: n, sum ET | 120, 107.0 / 261, 192.0 / 701, 1042.0 | 190, 155.0 / 225, 156.0 / 705, 1064.5 |
| step 2: Layer-1, ECAL in -> out | 360.5 -> 284.0 | 428.5 -> 379.0 |
| HCAL (HB + HE) in -> out; HF in -> out | 299.0 -> 262.5; 1042.0 -> 804.0 | 311.0 -> 246.0; 1064.5 -> 818.0 |
| all in -> out (towers with ET > 0) | 1701.5 -> 1350.5 (1693) | 1804.0 -> 1443.0 (1799) |
| rings 26-28 in -> out; barrel rings in -> out | 196.0 -> 118.0; 310.5 -> 274.0 | 141.0 -> 89.5; 447.5 -> 383.0 |
| step 3: L1 jets (seeded in tower ring 28) | 4 (0) | 4 (0) |
| bbar jet: seed, window, phi rings, pileup, calibration, ET | 44.5, 166.0, 418, 52.0, 1.05, 120.0 | 44.5, 170.0, 429, 53.5, 1.05, 122.5 |
| b jet: seed, window, phi rings, pileup, calibration, ET | 14.0, 56.0, 91, 11.0, 2.17, 97.5 | 14.5, 56.0, 107.5, 13.0, 2.17, 93.5 |
| the two other jets: ET (eta, phi) | 40.0 (0.65, -0.41), 32.0 (0.48, -2.76) | 39.0 (0.65, -0.41), 28.0 (0.39, -2.85) |
| all jets: windows, pileup estimates, jet ET | 277.0, 89.0, 289.5 | 285.5, 99.5, 283.0 |
| e/gamma, tau: number, largest | 3 (3.0), 11 (54.5) | 9 (6.0), 12 (54.0) |
| ETT / ETTEM / HTT | 546.5 / 281.5 / 169.5 | 625.0 / 377.0 / 132.5 |
| ETM / ETMHF / HTM / HTMHF | 59.5 / 69.0 / 121.0 / 38.0 | 48.0 / 78.5 / 101.0 / 66.0 |
| steps 4 and 5: stubs, EMTF candidates, muons | 5 CSC stubs (none in bx 0), 0, 0 | identical |
| step 6: algorithms fired (on an object; by the two signal jets alone) | 30 of 408 (22; 14) | 54 of 465 (46; 22) |
| fired in one year only | 4: `DoubleIsoTau26er2p1_Jet55/70_RmOvlp_dR0p5` (not in the 2025 menu), `DoubleJet40er2p5` (the 40.0 GeV jet), `HTT160er` (HTT 169.5 with the 32.0 GeV jet) | 28: 25 new in the 2025 menu, `ETMHF70`, `ETMHF70_HTT60er` (ETMHF 78.5; 2024: 69.0), `CICADA_VLoose` |
| CICADA score; thresholds of the menu | 82.3; 113, 116, 121, 127, 131 (none fired) | 55.1; 50, 60, 70, 80, 90, 110, 125, 135 (1 fired) |
| `L1_SingleJet120`, `L1_SingleJet90`: margin on the bbar jet | 0.0, 30.0 | 2.5, 32.5 |

The last rings of the endcap. In neither year does an ECAL primitive of rings 27 and 28 pass the Layer-1 zero
suppression (6 and 9 GeV, the same `L1TCaloParams` in both years): with the 16 crossings the ECAL primitives of rings
26 to 28 sum to 78.0 GeV in 2024 and 59.0 GeV in 2025, the largest 4.0 and 3.5 GeV (with the three crossings of the
earlier version: 991.0 and 321.5 GeV, up to 11.5 GeV, and six of the twelve 2024 L1 jets were seeded in ring 28). What
the year changes there is in two payloads that changed together, read by the DIGI job of chapter 11 where the
primitives are made. The ECAL digitiser divides the light of
every crystal by its laser correction (`SimCalorimetry/EcalSimAlgos/src/EcalHitResponse.cc`, line 186, with
`UseLCcorrection = True` in `ecalDigiParameters_cff.py`): the crystal delivers the fraction (p/ref)^alpha of its light,
p from `EcalLaserAPDPNRatios` (2024: `..._2024_outliers_cleaned_3sigma_mc`, 2025:
`..._TL470fb_moy_2025_outliers_cleaned_3sigma_mc`), ref and alpha from `EcalLaserAPDPNRatiosRef_mc` and
`EcalLaserAlphas_UL_Run1_Run2_2018_lastIOV_movedTo1` (the same in both tags). The trigger-primitive generator then
multiplies the amplitude of every crystal by its linearisation factor mult_x12 / 2^shift_x12 of
`EcalTPGLinearizationConst`. Averaged over the crystals of a tower ring (`out2024/compare_12_L1T.txt`, section (5):
`conddb dump` of the payloads of the two global tags; the tower ring of a barrel crystal from `EBDetId`, of an endcap
crystal from the crystal-to-tower map of the release, `Geometry/EcalMapping/data/EndCap_TTMap.txt`; the linear
corrections `EcalLinearCorrections_mc` are 1 in both tags, and the light is (p/p_ref)^alpha as in
`EcalLaserDbService::getLaserCorrection`; conditions, not the event: unchanged by the move to 16 crossings):

| tower ring | light delivered, 2024 | 2025 | linearisation factor, 2025 / 2024 | primitive per GeV deposited, 2025 / 2024 |
|---|---|---|---|---|
| 1 (barrel) | 0.809 | 0.701 | 1.087 | 0.94 |
| 17 (barrel) | 0.652 | 0.557 | 1.132 | 0.96 |
| 19 | 0.663 | 0.572 | 1.113 | 0.96 |
| 22 | 0.467 | 0.388 | 1.177 | 0.97 |
| 23 | 0.386 | 0.319 | 1.198 | 0.99 |
| 24 | 0.336 | 0.253 | 1.230 | 0.92 |
| 25 | 0.267 | 0.188 | 1.257 | 0.88 |
| 26 | 0.179 | 0.110 | 1.300 | 0.77 |
| 27 | 0.123 | 0.051 | 1.357 | 0.54 |
| 28 | 0.094 | 0.022 | 1.394 | 0.34 |

(Rings 1 to 23, all in the file: 0.94 to 0.99. The first version of this table placed the endcap crystals in rings by
their x, y position at z = 317 cm, an approximation near the ring boundaries; with the map of the release ring 28 is
0.094 -> 0.022, x 1.394, 0.34, instead of 0.096 -> 0.025, x 1.373, 0.36.)

The 2025 linearisation follows the additional transparency loss of the 2025 laser payload to within about 6 % up to
ring 23, and not in the last rings: the same deposit gives in ring 28 a third of the primitive it gave in 2024, in
ring 27 a half. The noise of a crystal, in ADC counts, is not attenuated by the transparency, so it is multiplied by
the larger linearisation factor. Whether the TPG constants of the 2025 tag are meant to stop short at high eta is
not established (both payloads are named `TL470fb_moy`).

The CICADA score. Both the model and its input changed. The L1 job was run once more on each input with each model
(`out2024/cicada_input2024_v2p2p0.root`, `out2024/cicada_input2025_v2p1p2.root`, only `simCaloStage2Layer1Summary`
kept; `out2024/compare_12_L1T.txt`, section (4)): 2024 input with `v2p1p2` 82.258, with `v2p2p0` 51.508; 2025 input
with `v2p1p2` 86.188, with `v2p2p0` 55.078. The model lowers the score by a third; the input of 2025 raises it a little
(with the three crossings: 181.684, 126.375, 127.695, 88.262). The 2025 menu lowered the thresholds with the model: in
2025 `VLoose` (50) fires, in 2024 nothing comes near 113.

The barrel ECAL and the HCAL. The EB primitives of the event go from 304 (202.5 GeV) to 428 (292.0 GeV) and the HB ones
from 120 (107.0 GeV) to 190 (155.0 GeV), with the same deposits: the barrel crystals of 2025 are noisier (pedestal width
3.31 instead of 2.85 ADC counts at gain 12, chapter 11, section 4.9) and their linearisation factor is 1.09 to 1.13
times larger (the table above), and the HCAL payloads that changed are `HcalTPChannelParameters` and, in the DIGI job,
the HCAL pedestals, widths, response corrections, SiPM parameters and zero-suppression levels of the 2025 tag. Which of
them makes the extra HB primitives is not established. The e/gamma candidates (3 in 2024, 9 in 2025, up to 3.0 and 6.0
GeV) follow the barrel ECAL primitives.

What did not change: the muon trigger (steps 4 and 5, line for line, and `truth_12_L1T.txt`), the four jets to 4 GeV,
and the door into chapter 13: `L1_SingleJet90` fires in both, and the HLT seeds of the jet paths are the same in
`HLT_GRun_cff.py` and `HLT_2025v10_cff.py`.

### The move to the 16 bunch crossings (21 November 2026)

Until 21 November 2026 the chapter ran on the mixed DIGI of chapter 11 made with the three bunch crossings -1..1 (71,
68, 54 collisions, library events 1-193, in time 72-139); its outputs, and those of the 2024 comparison, are kept in
`test/tmp/out_bx-1to1/12_L1T/out/` and `out2024/` (not in git). Since then chapters 10-13 mix the 16 crossings -12..3 of
the Run-3 production (`jitjet_common.sh`; the draw of chapter 08). Two things change for the trigger: the in-time
collisions are other ones (library events 747-810; the 68 of before now sit in bunch crossing -11), and the pulses of
746 earlier collisions reach the samples of our crossing (chapter 11, the section on the trigger primitives). The 2025 run,
three crossings -> 16 crossings:

| | bx -1..1 | bx -12..3 |
|---|---|---|
| step 1: primitives with ET > 0, sum ET; ECAL / HCAL+HF | 2479, 2793.0; 1047.5 / 1745.5 | 1715, 1804.0; 428.5 / 1375.5 |
| step 2: Layer-1 towers, sum ET | 2561, 2181.0 | 1799, 1443.0 |
| step 3: L1 jets; bbar / b jet | 9; 136.0 / 108.5 | 4; 122.5 / 93.5 |
| e/gamma, tau; ETT, HTT, ETMHF, HTMHF | 12, 12; 1086.0, 108.5, 79.0, 34.5 | 9, 12; 625.0, 132.5, 78.5, 66.0 |
| steps 4, 5: stubs to the EMTF, candidates, muons | 16 (13 distinct, 9 in bx 0), 2, 1 (the pileup kaon decay, 3.0 GeV) | 5 (none in bx 0), 0, 0 |
| step 6: algorithms fired (on an object; by the two signal jets alone) | 63 (55; 22) | 54 (46; 22) |
| CICADA score | 88.3 | 55.1 |
| `L1_SingleJet90` margin, bbar / b jet | 46.0 / 18.5 | 32.5 / 3.5 |
| 2024 run: algorithms fired; L1 jets (in ring 28) | 57 of 408; 12 (6) | 30 of 408; 4 (0) |
