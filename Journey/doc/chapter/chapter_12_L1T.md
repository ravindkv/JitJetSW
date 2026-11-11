# Chapter 12, L1T: technical companion

The Level-1 trigger of the ONE event, in six steps, for the signal alone (12a), for one pileup collision alone (12b)
and for the signal with its pileup (12a+12b). Script: `test/12_L1T/run_12_L1T.sh`; everything it produces is in
`test/12_L1T/out/`. The chapter text is `doc/chapter/chapter_12_L1T.tex`, a checkpoint chapter (Rule G of the
Constitution: B1, B2, one section per step, B4, B5). All commands below are run from `test/` of an initialized
CMSSW_15_0_5 (`cmsenv`), with `J=$CMSSW_BASE/src/JitJetSW/Journey`.

## 1. The cmsDriver.py commands

The script runs `cmsDriver.py` three times through `run_driver` of `test/common/jitjet_common.sh`. The commands,
copied from the header of the generated configurations (`test/12_L1T/step4_L1_<case>_12_L1T_cfg.py`, "with command
line options"; the cmsDriver output is in `out/step4_L1_<case>_12_L1T_cfg.driver.log`):

```bash
cd $J/test
for case in signal pileup mixed; do
  ev=1:1:1; [[ $case == pileup ]] && ev=1:1:72            # library event 72 for (12b)
  cmsDriver.py step4 --filein file:$J/test/11_DIGI/out/step3_DIGI_${case}_11_DIGI.root \
    --conditions auto:phase1_2024_realistic --era Run3_2024 --geometry DB:Extended \
    -n -1 -s L1 --datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG --fileout file:step4_L1_${case}_12_L1T.root \
    --customise_commands "process.load('SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff')\nprocess.FEVTDEBUGoutput.outputCommands.append('keep *_simCaloStage2Layer1Summary_*_*')\nprocess.source.eventsToProcess = cms.untracked.VEventRange('$ev')" \
    --python_filename $J/test/12_L1T/step4_L1_${case}_12_L1T_cfg.py --no_exec
done
```

| argument | meaning |
|---|---|
| `step4` | the name of the configuration (free; `step1` was GEN, `step2` SIM, `step3` DIGI) |
| `--filein file:...` | the input (Rule E): the DIGI file of chapter 11 of the same sample. It holds the digis and, made in the same DIGI job, the calorimeter trigger primitives `simEcalTriggerPrimitiveDigis` and `simHcalTriggerPrimitiveDigis` (step 1 of the chapter) |
| `--conditions auto:phase1_2024_realistic` | the global tag of the book, `150X_mcRun3_2024_realistic_v2`. For the L1 it provides the menu (`L1TUtmTriggerMenuRcd`: `L1Menu_Collisions2024_v1_3_0_xml`), the calorimeter-trigger parameters (`L1TCaloParamsRcd`: `L1TCaloParams_static_2024_v0_1_mc`), the muon track-finder parameters (`L1TMuonBarrelParamsRcd`, `L1TMuonOverlapParamsRcd`, `L1TMuonEndCapParamsRcd`, `L1TMuonEndCapForestRcd`, `L1TMuonGlobalParamsRcd`) and pass-through prescales (`L1TGlobalPrescalesVetos_passThrough_mc`); list them with `conddb list 150X_mcRun3_2024_realistic_v2 \| grep -E "L1T\|EcalTPG\|HcalTP"` |
| `--era Run3_2024` | selects the stage-2 emulator (`stage2L1Trigger`) with its yearly modifiers up to `stage2L1Trigger_2024` (OMTF patterns, Kalman BMTF quality), `run3_GEM` (GEM pads and the Run-3 CSC primitives), `run3_common` (muon showers, HCAL saturation fix) |
| `--geometry DB:Extended` | the reconstruction geometry from the database: the muon trigger-primitive emulators and the track finders need the chamber geometry |
| `-n -1` with `eventsToProcess` | all events of the input, restricted to the one event of the sample |
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
3. `process.source.eventsToProcess = cms.untracked.VEventRange('1:1:1')` (`'1:1:72'` for the pileup sample): the event
   of the sample; the DIGI files of chapter 11 hold only that event, the line is a safeguard.

## 2. The cmsRun on the python config

```bash
cd $J/test/12_L1T/out
cmsRun ../step4_L1_signal_12_L1T_cfg.py > step4_L1_signal_12_L1T.log 2>&1      # 15 s
cmsRun ../step4_L1_pileup_12_L1T_cfg.py > step4_L1_pileup_12_L1T.log 2>&1      # 15 s
cmsRun ../step4_L1_mixed_12_L1T_cfg.py  > step4_L1_mixed_12_L1T.log  2>&1      # 15 s; the event itself takes 2 s
```

Or the whole chapter: `./12_L1T/run_12_L1T.sh` (one minute; variables: `CASES="signal pileup mixed"`, `REUSE=1` keeps
existing L1 files, `RUN=0` only writes the configurations, `ANALYZE=0` skips the analyzers, `PNG=0` skips the PNG
pages). The three L1 jobs run in parallel; their logs are `out/run_<case>.log`.

### Walk-through of `step4_L1_mixed_12_L1T_cfg.py`

- `process = cms.Process('L1', Run3_2024)`: the process name under which the new products are stored.
- `process.load(...)`: `Services_cff`, `pythiapdt_cfi`, the `MessageLogger`, `EventContent_cff`, `mixNoPU_cfi` (loaded
  by cmsDriver for every step, not used), `GeometryRecoDB_cff`, `MagneticField_cff`,
  `Configuration.StandardSequences.SimL1Emulator_cff` (the emulator), `EndOfProcess_cff`,
  `FrontierConditions_GlobalTag_cff`.
- `process.source`: `PoolSource` on the DIGI file of chapter 11.
- `process.FEVTDEBUGoutput`: the `PoolOutputModule` with the `FEVTDEBUGEventContent` commands.
- `process.L1simulation_step = cms.Path(process.SimL1Emulator)`, then `endjob_step` and the output `EndPath`.
- The three customise lines of section 1.

What `SimL1Emulator` runs (`out/settings_12_L1T.txt`, section 1; the full parameter sets are in its section 2, the
two trigger-primitive emulators of the DIGI job in its section 3):

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
`L1_Bptx*` algorithms fire on every simulated event. `l1tZDCEtSums` are the ZDC sums for heavy-ion menus.

Where the numbers of the trigger are. The python holds almost none of them (`out/settings_12_L1T.txt`); the trigger is
configured from the database, and the analyzers print what they can reach of it into `out/l1t_<case>_12_L1T.log`:
`caloParams:` lines (the calorimeter-trigger parameters of `L1TCaloParamsRcd`, with the Layer-1 scale-factor tables),
`menu:` and `condition:` lines (the menu of `L1TUtmTriggerMenuRcd` and the cuts of the conditions that were true).

### The content of the output ROOT file

`edmDumpEventContent file:$J/test/12_L1T/out/step4_L1_mixed_12_L1T.root | grep '"L1"'` (12.3 MB for the mixed event,
2.1 MB signal, 1.0 MB pileup: the input plus about 0.1 MB). The content of the chapter-11 file is copied. New under
the process `L1`, in the order of the steps:

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

One job per sample runs three analyzers, one after the other (`test/common/l1tAnalyzer_cfg.py`; the plugins are
`plugins/L1TCaloAnalyzer.cc`, `L1TMuonAnalyzer.cc`, `L1TGlobalAnalyzer.cc`, with the tower grid and the page layout
shared in `plugins/L1TDisplay.h`):

```bash
cd $J/test/12_L1T/out
cmsRun $J/test/common/l1tAnalyzer_cfg.py inputFiles=file:step4_L1_mixed_12_L1T.root maxEvents=1 eventsToProcess=1:1:1 \
  globalTag=auto:phase1_2024_realistic era=Run3_2024 geometry=DB:Extended pdfPrefix=l1t_mixed_12_L1T savePNG=1 \
  sampleLabel="chapter 12 (12a+12b): signal + pileup|PU = 60 per bx, bx -1..1 (playback)|2024 Level-1 trigger" \
  > l1t_mixed_12_L1T.log 2>&1                                  # 6 s
```

Options: `jetPtMin=30` (the ak4GenJets drawn as circles), `genMuonPtMin=1`, `zoomJets=2` (the leading L1 jets drawn
tower by tower), `zMax=256` (the top of the colour scale, GeV). The job needs the conditions (menu, calorimeter-trigger
parameters, HCAL primitive scale: it loads `hcaltpdigi_cff` like the L1 job) and the geometry (DT chambers).

| analyzer | steps | PDF and PNG pages (`l1t_<case>_12_L1T_...`) | log lines |
|---|---|---|---|
| `L1TCaloAnalyzer` | 1, 2, 3 | `_calo.pdf`; `_calo_ecaltp`, `_calo_hcaltp` (step 1: the primitives on the 82 x 72 tower grid), `_calo_layer1` (step 2), `_calo_layer2` (step 3: the jet windows on the towers, the jet table), and for each of the two leading jets `_calo_jet<N>_ecaltp`, `_hcaltp`, `_layer1`: the 17 x 17 towers around the seed with the ET written in each, the 9 x 9 window, the towers of the pileup estimate, the arithmetic | `caloParams:`, `TP <region>:`, `Layer1 <region>:`, `ring ieta=`, `Layer1 top tower`, `GenJet:`, `L1Jet N:` (with the window sums and the recomputed pileup estimate), `L1EG`, `L1Tau`, `L1Sums:`, `counts:` |
| `L1TMuonAnalyzer` | 4, 5 | `_muon.pdf`; `_muon_etaphi`, `_muon_rz`: stubs, track-finder candidates, uGMT muons | `DT TP:`, `EMTFHit:`, `TP counts:`, `EMTF input stubs:`, `EMTFTrack:`, `TFCand:`, `TF counts`, `uGMT muon:`, `GenMuon:` |
| `L1TGlobalAnalyzer` | 6 | `_global.pdf`; `_global_objects` (the event as the global trigger receives it), `_global_ladders` (the thresholds of the single-object families and the event) | `uGT inputs`, `CICADA score:`, `menu:`, `decision:`, `fired:` (expression, conditions, the objects behind them), `condition:` (the cuts of the menu), `ladder`, `algo:` (all 408) |

Each analyzer opens and closes its PDF inside its own `analyze()` (ROOT has one global PDF), so the three can share a
job. The `L1Jet` lines check the emulator: the pileup estimate recomputed from the towers (`recomputedPu`) equals the
`puEt` stored in the jet for all 14 jets of the three samples.

Three scripts then read the logs and the files:

```bash
python3 $J/test/common/l1tSettings.py $J/test/12_L1T/step4_L1_signal_12_L1T_cfg.py $J/test/11_DIGI/step3_DIGI_signal_11_DIGI_cfg.py > settings_12_L1T.txt
python3 $J/test/common/l1tTruth.py . 12_L1T > truth_12_L1T.txt      # FWLite: the TrackingParticles behind every L1 muon
python3 $J/test/common/l1tLedger.py . 12_L1T > ledger_12_L1T.txt    # the three samples side by side
```

## 4. The tables

Every number of the chapter, by step. Source files in `test/12_L1T/out/`.

### The samples, the menu, the calorimeter-trigger parameters (`ledger_12_L1T.txt`, head)

```
== l1tLedger 12_L1T
(12a)     signal: menu L1Menu_Collisions2024_v1_3_0, 408 algorithms; finalOR=1 fired=19
(12b)     pileup: menu L1Menu_Collisions2024_v1_3_0, 408 algorithms; finalOR=1 fired=8
(12a+12b) mixed: menu L1Menu_Collisions2024_v1_3_0, 408 algorithms; finalOR=1 fired=57
caloParams: towerLsbSum=0.5 towerLsbE=0.5 towerLsbH=0.5 towerNBitsSum=9 jetLsb=0.5 jetSeedThreshold=4 jetNeighbourThreshold=0 jetBypassPUS=0 jetPUSUsePhiRing=1 jetPUST...
caloParams etSum (index: etaMin etaMax etThreshold): 0: 1 28 0; 1: 1 26 30; 2: 1 28 0; 3: 1 26 30; 4: 1 28 0;
jet pileup estimate: PhiRing1
```

`jetPUSUsePhiRing=1` selects the phi-ring pileup subtraction although `jetPUSType` still reads `ChunkyDonut`
(`Stage2Layer2JetAlgorithmFirmwareImp1::create`). `etSum` index 0 = ETT, 1 = HTT, 2 = ETM, 3 = HTM, 4 = ETMHF: towers
up to ieta 28, jets up to ieta 26 and above 30 GeV.

### Steps 1 and 2 (`ledger_12_L1T.txt`, sections 1 and 2)

```
-- 1. Steps 1 and 2: the transverse energy the trigger sees (towers with ET > 0 / sum ET in GeV / largest ET)
                                          (12a) signal                (12b) pileup             (12a+12b) mixed
ECAL primitives, EB                  200 / 114.5 / 3.0            157 / 83.5 / 1.0           594 / 408.0 / 5.0
ECAL primitives, EE                    98 / 85.5 / 6.5             72 / 44.0 / 2.0         594 / 1273.5 / 11.5
ECAL primitives, all                 298 / 200.0 / 6.5           229 / 127.5 / 2.0        1188 / 1681.5 / 11.5
HCAL primitives, HB                     13 / 7.0 / 1.0              11 / 5.5 / 0.5           153 / 129.5 / 8.0
HCAL primitives, HE                   13 / 25.0 / 10.0               0 / 0.0 / 0.0          335 / 271.5 / 10.0
HCAL primitives, HF                  39 / 148.5 / 89.0               4 / 2.0 / 0.5         741 / 1329.0 / 89.0
HCAL primitives, all                 65 / 180.5 / 89.0              15 / 7.5 / 0.5        1229 / 1730.0 / 89.0
Layer-1 towers, barrel               203 / 117.0 / 3.0            160 / 85.0 / 1.0           644 / 494.5 / 8.0
Layer-1 towers, endcap                49 / 71.5 / 14.0             28 / 14.0 / 0.5          609 / 844.5 / 14.0
Layer-1 towers, forward              28 / 131.0 / 44.5               0 / 0.0 / 0.0        1282 / 1102.0 / 44.5
Layer-1 towers, all                 280 / 319.5 / 44.5            188 / 99.0 / 1.0        2535 / 2441.0 / 44.5
Layer-1 inputs, EM | HAD                  161.5 | 27.0                  97.5 | 1.5               985.5 | 353.5   (the calibrated ECAL and HB+HE inputs of the towers, sum ET)
what Layer-1 changes (sum ET in, sum ET out, difference):
  ECAL (EB + EE)                200.0 -> 161.5 (-38.5)       127.5 -> 97.5 (-30.0)    1681.5 -> 985.5 (-696.0)
  HCAL (HB + HE)                   32.0 -> 27.0 (-5.0)           5.5 -> 1.5 (-4.0)      401.0 -> 353.5 (-47.5)
  HF                            148.5 -> 131.0 (-17.5)           2.0 -> 0.0 (-2.0)   1329.0 -> 1102.0 (-227.0)
```

The Layer-1 scale factors of the global tag (`l1t_mixed_12_L1T.log`; one row per ET bin of the primitive, one column
per tower ring):

```
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

Ring by ring:

```
-- 2. Ring by ring (72 towers in phi): ECAL primitives + HCAL primitives -> Layer-1 towers, sum ET in GeV
 ieta       (12a) ecal + hcal -> layer1       (12b) ecal + hcal -> layer1   (12a+12b) ecal + hcal -> layer1
  -41                  0.0 + 0.0 -> 0.0                  0.0 + 0.0 -> 0.0                0.0 + 38.0 -> 24.0
  -40                  0.0 + 2.0 -> 0.0                  0.0 + 0.5 -> 0.0                0.0 + 36.5 -> 24.0
  -39                  0.0 + 0.5 -> 0.0                  0.0 + 0.0 -> 0.0                0.0 + 46.0 -> 39.0
  -38                  0.0 + 0.0 -> 0.0                  0.0 + 0.0 -> 0.0                0.0 + 38.5 -> 31.0
  -37                  0.0 + 0.0 -> 0.0                  0.0 + 0.0 -> 0.0                0.0 + 48.5 -> 41.0
  -36                  0.0 + 0.5 -> 0.0                  0.0 + 0.0 -> 0.0                0.0 + 64.5 -> 55.0
  -35                  0.0 + 2.5 -> 1.0                  0.0 + 0.0 -> 0.0                0.0 + 53.5 -> 43.0
  -34               0.0 + 100.5 -> 98.0                  0.0 + 0.5 -> 0.0              0.0 + 148.5 -> 140.0
  -33                0.0 + 24.5 -> 22.0                  0.0 + 0.0 -> 0.0                0.0 + 75.0 -> 68.0
  -32                  0.0 + 0.0 -> 0.0                  0.0 + 0.0 -> 0.0                0.0 + 50.5 -> 42.0
  -31                  0.0 + 2.5 -> 2.0                  0.0 + 0.0 -> 0.0                0.0 + 51.5 -> 41.0
  -30                  0.0 + 1.5 -> 0.0                  0.0 + 0.5 -> 0.0                0.0 + 36.0 -> 26.0
  -28                 16.0 + 1.0 -> 1.0                  3.0 + 0.0 -> 0.0             304.0 + 65.0 -> 125.0
  -27                  4.0 + 0.0 -> 0.0                  2.0 + 0.0 -> 0.0               111.0 + 8.0 -> 21.0
  -26                  8.0 + 0.0 -> 0.0                  6.0 + 0.0 -> 0.0               62.5 + 13.0 -> 33.0
  -25                  5.5 + 0.0 -> 5.5                  4.5 + 0.0 -> 4.5                35.0 + 8.0 -> 43.5
  -24                  2.0 + 0.0 -> 2.0                  1.5 + 0.0 -> 1.5                29.0 + 8.0 -> 37.5
  -23                  1.0 + 2.0 -> 3.0                  0.5 + 0.0 -> 0.5                21.5 + 3.0 -> 25.0
  -22                  1.5 + 0.0 -> 1.5                  0.0 + 0.0 -> 0.0                18.5 + 1.0 -> 20.0
  -21                  0.5 + 0.0 -> 0.5                  0.5 + 0.0 -> 0.5                 6.0 + 4.0 -> 10.0
  -20                  2.0 + 1.0 -> 3.0                  0.0 + 0.0 -> 0.0                13.0 + 3.0 -> 16.0
  -19                22.0 + 4.5 -> 27.0                  0.5 + 0.0 -> 0.5               31.0 + 10.0 -> 41.5
  -18                6.5 + 15.0 -> 22.5                  0.5 + 0.0 -> 0.5               10.0 + 17.0 -> 28.0
  -17                  5.5 + 0.5 -> 6.0                  0.0 + 0.0 -> 0.0                 7.5 + 3.0 -> 10.5
  -16                  2.0 + 0.5 -> 2.5                  2.0 + 1.0 -> 3.0                  6.5 + 3.0 -> 9.5
  -15                  1.5 + 0.0 -> 1.5                  1.0 + 0.0 -> 1.0                 8.0 + 8.0 -> 15.0
  -14                  1.5 + 0.5 -> 1.5                  1.5 + 0.0 -> 1.5                  6.5 + 3.0 -> 7.5
  -13                  1.5 + 0.0 -> 1.5                  1.0 + 0.0 -> 1.0                12.5 + 2.5 -> 13.5
  -12                  3.0 + 0.0 -> 3.0                  0.0 + 0.0 -> 0.0                14.5 + 6.0 -> 19.0
  -11                  1.5 + 0.0 -> 1.5                  2.0 + 0.0 -> 2.0                 9.0 + 3.5 -> 11.0
  -10                  6.0 + 0.0 -> 6.0                  1.5 + 0.0 -> 1.5                13.0 + 3.0 -> 14.0
   -9                  6.5 + 0.0 -> 6.5                  2.5 + 0.0 -> 2.5                15.5 + 3.0 -> 15.5
   -8                  4.0 + 0.0 -> 4.0                  1.5 + 0.0 -> 1.5                14.5 + 2.5 -> 16.0
   -7                  3.0 + 0.0 -> 3.0                  3.0 + 0.5 -> 3.0                17.0 + 1.0 -> 17.0
   -6                  5.0 + 0.5 -> 5.0                  6.0 + 0.5 -> 6.0                15.0 + 5.0 -> 18.0
   -5                  7.0 + 0.0 -> 7.0                  2.5 + 0.0 -> 2.5                14.0 + 5.0 -> 18.0
   -4                  3.5 + 0.0 -> 3.5                  4.5 + 0.0 -> 4.5                20.0 + 8.5 -> 25.0
   -3                  6.0 + 0.0 -> 6.0                  2.5 + 0.0 -> 2.5                12.5 + 4.0 -> 15.0
   -2                  3.5 + 0.0 -> 3.5                  4.0 + 1.0 -> 4.0                13.0 + 7.0 -> 18.5
   -1                  3.5 + 0.0 -> 3.5                  6.0 + 0.5 -> 6.0                13.0 + 3.0 -> 15.0
    1                  5.5 + 0.5 -> 5.5                  2.0 + 0.0 -> 2.0                19.5 + 1.5 -> 20.5
    2                  4.0 + 2.0 -> 4.0                  3.0 + 0.0 -> 3.0                12.5 + 0.5 -> 12.5
    3                  4.0 + 1.0 -> 4.0                  4.0 + 0.0 -> 4.0                14.5 + 2.0 -> 15.5
    4                  5.0 + 0.0 -> 5.0                  4.0 + 1.0 -> 4.0                17.0 + 2.0 -> 18.0
    5                  3.0 + 0.0 -> 3.0                  2.0 + 0.0 -> 2.0                12.0 + 2.5 -> 13.5
    6                  4.0 + 0.0 -> 4.0                  6.0 + 0.0 -> 6.0                12.0 + 4.5 -> 14.5
    7                  3.0 + 0.0 -> 3.0                  2.0 + 0.5 -> 2.0                13.0 + 2.5 -> 14.0
    8                  4.5 + 0.5 -> 4.5                  2.5 + 0.0 -> 2.5                  6.0 + 0.5 -> 6.0
    9                  3.5 + 0.0 -> 3.5                  2.5 + 0.0 -> 2.5                  9.0 + 1.0 -> 9.0
   10                  1.5 + 0.0 -> 1.5                  1.0 + 0.0 -> 1.0                13.0 + 3.0 -> 14.0
   11                  4.0 + 0.0 -> 4.0                  2.5 + 0.0 -> 2.5               10.0 + 16.5 -> 25.5
   12                  3.5 + 0.0 -> 3.5                  3.0 + 0.0 -> 3.0                  7.0 + 6.0 -> 9.0
   13                  1.5 + 0.0 -> 1.5                  2.5 + 0.0 -> 2.5                11.0 + 5.5 -> 15.5
   14                  1.5 + 0.0 -> 1.5                  0.0 + 0.0 -> 0.0                13.0 + 4.5 -> 15.0
   15                  0.0 + 0.0 -> 0.0                  2.0 + 0.0 -> 2.0                13.0 + 5.0 -> 15.5
   16                  1.0 + 1.5 -> 2.5                  1.0 + 0.5 -> 1.5                 7.5 + 4.0 -> 11.5
   17                  0.5 + 0.0 -> 0.5                  2.0 + 0.0 -> 2.0                  6.0 + 1.0 -> 7.0
   18                  0.5 + 0.0 -> 0.5                  0.0 + 0.0 -> 0.0                  2.0 + 5.0 -> 7.0
   19                  0.5 + 0.0 -> 0.5                  0.0 + 0.0 -> 0.0                  4.0 + 5.5 -> 9.5
   20                  0.5 + 0.0 -> 0.5                  1.0 + 0.0 -> 1.0                 7.0 + 4.0 -> 11.0
   21                  0.0 + 0.0 -> 0.0                  0.0 + 0.0 -> 0.0                  6.5 + 2.0 -> 8.5
   22                  0.5 + 0.0 -> 0.5                  0.5 + 0.0 -> 0.5                 9.0 + 3.0 -> 12.0
   23                  0.0 + 0.0 -> 0.0                  1.5 + 0.0 -> 1.5                17.0 + 9.0 -> 26.0
   24                  2.0 + 0.0 -> 2.0                  0.0 + 0.0 -> 0.0                34.0 + 6.0 -> 41.5
   25                  0.5 + 1.0 -> 1.5                  3.0 + 0.0 -> 3.0               39.0 + 16.0 -> 55.5
   26                  2.0 + 0.0 -> 0.0                  7.0 + 0.0 -> 0.0               79.5 + 12.0 -> 60.0
   27                  4.0 + 0.0 -> 0.0                  7.0 + 0.0 -> 0.0               119.0 + 7.0 -> 32.0
   28                  6.0 + 0.0 -> 0.0                  5.0 + 0.0 -> 0.0             315.0 + 58.0 -> 181.0
   30                  0.0 + 0.0 -> 0.0                  0.0 + 0.0 -> 0.0                0.0 + 41.5 -> 33.0
   31                  0.0 + 4.5 -> 3.0                  0.0 + 0.0 -> 0.0                0.0 + 45.0 -> 36.0
   32                  0.0 + 3.0 -> 3.0                  0.0 + 0.0 -> 0.0                0.0 + 44.0 -> 33.0
   33                  0.0 + 2.5 -> 2.0                  0.0 + 0.0 -> 0.0                0.0 + 56.5 -> 50.0
   34                  0.0 + 0.5 -> 0.0                  0.0 + 0.0 -> 0.0                0.0 + 53.5 -> 45.0
   35                  0.0 + 0.5 -> 0.0                  0.0 + 0.5 -> 0.0                0.0 + 56.0 -> 48.0
   36                  0.0 + 0.0 -> 0.0                  0.0 + 0.0 -> 0.0                0.0 + 62.5 -> 53.0
   37                  0.0 + 1.5 -> 0.0                  0.0 + 0.0 -> 0.0                0.0 + 60.5 -> 51.0
   38                  0.0 + 0.0 -> 0.0                  0.0 + 0.0 -> 0.0                0.0 + 65.5 -> 57.0
   39                  0.0 + 0.5 -> 0.0                  0.0 + 0.0 -> 0.0                0.0 + 52.5 -> 44.0
   40                  0.0 + 1.0 -> 0.0                  0.0 + 0.0 -> 0.0                0.0 + 54.5 -> 42.0
   41                  0.0 + 0.0 -> 0.0                  0.0 + 0.0 -> 0.0                0.0 + 50.0 -> 36.0
```

### Step 3 (`ledger_12_L1T.txt`, sections 3 and 4)

```
-- 3. Step 3: the Layer-2 jets (GeV). window = sum of the 9 x 9 Layer-1 towers; pileup = the estimate subtracted;
      calib = the calibration factor; ET = (window - pileup) x calib; ecalTP / hcalTP = the primitives in the window
(12a) signal: 2 L1 jets; GenJets: pt 117.8 eta -1.59 phi -2.08, pt 115.9 eta -3.75 phi 1.01
       ET    eta    phi  ieta  iphi   seed  window  pileup  win-pu  calib  ecalTP  hcalTP   GenJet
    113.0  -3.74   1.00   -34    12   44.5   119.0    15.0   104.0   1.09     0.0   124.0    115.9
          phi rings of the 9 eta columns of the window (72 towers each): 0 0 0 1 98 22 0 2 0; sum 123; x 9/72 = 15
    102.5  -1.52  -1.98   -18    50   14.0    55.5     8.0    47.5   2.16    32.5    21.5    117.8
          phi rings of the 9 eta columns of the window (72 towers each): 1.5 0.5 3 27 22.5 6 2.5 1.5 1.5; sum 66; x 9/72 = 8
(12b) pileup: 0 L1 jets; GenJets: none
       ET    eta    phi  ieta  iphi   seed  window  pileup  win-pu  calib  ecalTP  hcalTP   GenJet
(12a+12b) mixed: 12 L1 jets; GenJets: pt 117.8 eta -1.59 phi -2.08, pt 115.9 eta -3.75 phi 1.01
       ET    eta    phi  ieta  iphi   seed  window  pileup  win-pu  calib  ecalTP  hcalTP   GenJet
    137.0  -3.74   1.00   -34    12   44.5   190.5    60.5   130.0   1.05     0.0   200.5    115.9
          phi rings of the 9 eta columns of the window (72 towers each): 31 41 55 43 140 68 42 41 26; sum 487; x 9/72 = 60.5
    106.5  -1.52  -1.98   -18    50   14.0    71.0    19.5    51.5   2.07    45.0    25.0    117.8
          phi rings of the 9 eta columns of the window (72 towers each): 20 10 16 41.5 28 10.5 9.5 15 7.5; sum 158; x 9/72 = 19.5
     75.5  -2.74   1.00   -28    12   12.0    94.0    54.5    39.5   1.91    71.0    64.0        -
     56.0   2.74  -0.06    28    72   12.0    90.5    65.0    25.5   2.20    92.5    37.0        -
     36.5  -2.74  -0.58   -28    66   12.0    69.5    54.5    15.0   2.43    90.0    45.2        -
     22.5   2.74  -3.02    28    38   10.5    73.0    65.0     8.0   2.81    91.5    31.0        -
     18.0   0.91   1.70    11    20    8.0    20.0    15.0     5.0   3.60    11.0     9.0        -
     17.0   0.91  -2.50    11    44    4.5    19.5    15.0     4.5   3.78    14.0     8.5        -
     15.5   1.52   0.91    18    11    4.0    15.0    12.0     3.0   5.17     8.5     7.0        -
     14.0  -0.30   0.30    -4     4    5.5    23.5    20.0     3.5   4.00    20.0     6.5        -
     13.5  -2.74   2.22   -28    26   10.5    58.0    54.5     3.5   3.86    75.5    26.0        -
     12.5   2.74   1.00    28    12   11.0    68.0    65.0     3.0   4.17    90.5    37.5        -

-- 4. Step 3: what Layer-2 sends to the global trigger
                                 (12a)           (12b)       (12a+12b)
jets                                 2               0              12
e/gamma                              1               0              12
taus                                 2               0              12
ETT (total ET)                   188.5            99.0          1339.0
ETTEM (ECAL only)                162.0            97.5           965.5
HTT (jets > 30 GeV)              102.5             0.0           106.5
ETM                               52.5             3.5            50.0
ETMHF (with HF)                   65.5             3.5           106.5
HTM                              102.0             0.0           106.0
HTMHF                             21.5             0.0           170.0
tower count                      252.0           188.0          1253.0
(12a) leading L1EG: ET 3.5 eta 0.91 phi 2.31 iso 3
(12a) leading L1Tau: ET 54.5 eta -1.57 phi -1.93 iso 1
(12a+12b) leading L1EG: ET 15.5 eta 2.52 phi -0.93 iso 0
(12a+12b) leading L1Tau: ET 59.5 eta -1.57 phi -1.93 iso 1
```

Sums of the chapter's ledger table: windows of the jets 119.0 + 55.5 = 174.5 GeV (12a) and 792.5 GeV (12a+12b, twelve
jets); pileup estimates 23.0 and 500.5 GeV; jet ET 215.5 and 524.5 GeV. Invariant masses of the worked example,
M^2/2 = ET1 ET2 (cosh(deta) - cos(dphi)): the two signal jets (137.0, -3.741, 1.0005) and (106.5, -1.5225, -1.9767):
82 400 GeV^2, M = 406 GeV; the HF jet and the pileup jet (56.0, 2.7405, -0.0627): 2 501 000 GeV^2, M = 2237 GeV; the
cut MASS_MIN_800 is 320 000 GeV^2.

### Steps 4 and 5 (`ledger_12_L1T.txt`, section 5; `l1t_mixed_12_L1T.log`; `truth_12_L1T.txt`)

```
-- 5. Steps 4 and 5: the muon trigger
                                             (12a)           (12b)       (12a+12b)
DT phi segments                                  0               0               0
DT theta segments                                0               0               0
CSC anode patterns (ALCT)                        0               0              11
CSC cathode patterns (CLCT)                      0               0               8
CSC stubs (LCT)                                  0               0               8
GEM pads                                         0               0              11
GEM pad clusters                                 0               0               7
stubs given to the EMTF                          0               0              16
   of which neighbour copies                     0               0               4
EMTF tracks                                      0               0               2
BMTF (Kalman) candidates                       0/0             0/0             0/0
OMTF candidates                                0/0             0/0             0/0
EMTF candidates                                0/0             0/0             2/2
uGMT muons                                       0               0               1
muon showers                                     0               0               0
```

The stubs the EMTF received and what it made of them (mixed sample):

```
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
```

`pt = (hwPt - 1) x 0.5 GeV`, `eta = hwEta x 0.010875`, global phi = (processor x 96 + hwPhi + 24) x 2 pi / 576 for the
OMTF and EMTF (`l1t::MicroGMTConfiguration::calcGlobalPhi`). EMTF mode: bit 8 ME1, 4 ME2, 2 ME3, 1 ME4.

The truth behind the L1 muon:

```
== l1tTruth 12_L1T
signal: 0 L1 muon(s); 999 TrackingParticles
pileup: 0 L1 muon(s); 237 TrackingParticles
mixed: 1 L1 muon(s); 45282 TrackingParticles
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
reaches r = (214.4 + 5.5) / sinh(2.035) = 58.5 cm at that z: the muon is the daughter of the kaon.

### Step 6 (`ledger_12_L1T.txt`, section 6)

```
-- 6. Step 6: the decision of the global trigger
(12a)     FinalOR 1; 19 of 408 algorithms fire (19 after prescales and masks); CICADA score: 23.043
(12b)     FinalOR 1; 8 of 408 algorithms fire (8 after prescales and masks); CICADA score: 0
(12a+12b) FinalOR 1; 57 of 408 algorithms fire (57 after prescales and masks); CICADA score: 181.684
the algorithms that fire (x) in each sample:
   bit name                                                           (12a)     (12b) (12a+12b)  expression
     0 L1_SingleMuCosmics                                                 .         .         x  MU0
     3 L1_SingleMuCosmics_EMTF                                            .         .         x  MU0[MU-INDEX_EMTF_NEG,MU-INDEX_EMTF_POS]
     4 L1_SingleMuOpen                                                    .         .         x  MU0[MU-QLTY_OPEN]
     7 L1_SingleMuOpen_EMTF                                               .         .         x  MU0[MU-INDEX_EMTF_NEG,MU-INDEX_EMTF_POS,MU-QLTY_OPEN]
     8 L1_SingleMu0_DQ                                                    .         .         x  MU0[MU-QLTY_DBLE]
    11 L1_SingleMu0_EMTF                                                  .         .         x  MU0[MU-INDEX_EMTF_NEG,MU-INDEX_EMTF_POS,MU-QLTY_SNGL]
    19 L1_SingleMu3                                                       .         .         x  MU3[MU-QLTY_SNGL]
   129 L1_Mu3_Jet30er2p5                                                  .         .         x  MU3[MU-QLTY_SNGL] AND JET30[JET-ETA_2p52]
   159 L1_SingleEG8er2p5                                                  .         .         x  EG8[EG-ETA_2p52]
   305 L1_SingleJet35                                                     x         .         x  JET35
   306 L1_SingleJet60                                                     x         .         x  JET60
   307 L1_SingleJet90                                                     x         .         x  JET90
   308 L1_SingleJet120                                                    .         .         x  JET120
   312 L1_SingleJet35er2p5                                                x         .         x  JET35[JET-ETA_2p52]
   320 L1_SingleJet35_FWD2p5                                              x         .         x  JET35[JET-ETA_FWD_2p48_NEG] OR JET35[JET-ETA_FWD_2p48_POS]
   321 L1_SingleJet60_FWD2p5                                              x         .         x  JET60[JET-ETA_FWD_2p48_NEG] OR JET60[JET-ETA_FWD_2p48_POS]
   322 L1_SingleJet90_FWD2p5                                              x         .         x  JET90[JET-ETA_FWD_2p48_NEG] OR JET90[JET-ETA_FWD_2p48_POS]
   323 L1_SingleJet120_FWD2p5                                             .         .         x  JET120[JET-ETA_FWD_2p48_NEG] OR JET120[JET-ETA_FWD_2p48_POS]
   324 L1_SingleJet35_FWD3p0                                              x         .         x  JET35[JET-ETA_FWD_3p00_NEG] OR JET35[JET-ETA_FWD_3p00_POS]
   327 L1_SingleJet120_FWD3p0                                             .         .         x  JET120[JET-ETA_FWD_3p00_NEG] OR JET120[JET-ETA_FWD_3p00_POS]
   328 L1_SingleJet8erHE                                                  x         .         x  JET8[JET-ETA_1p392_2p958_NEG,JET-ETA_1p392_2p958_POS]
   329 L1_SingleJet10erHE                                                 x         .         x  JET10[JET-ETA_1p392_2p958_NEG,JET-ETA_1p392_2p958_POS]
   330 L1_SingleJet12erHE                                                 x         .         x  JET12[JET-ETA_1p392_2p958_NEG,JET-ETA_1p392_2p958_POS]
   357 L1_DoubleJet_110_35_DoubleJet35_Mass_Min800                        .         .         x  comb{JET110,JET35} AND mass_inv{JET35,JET35}[MASS_MIN_800]
   358 L1_DoubleJet_110_35_DoubleJet35_Mass_Min850                        .         .         x  comb{JET110,JET35} AND mass_inv{JET35,JET35}[MASS_MIN_850]
   362 L1_DoubleJet45_Mass_Min550_IsoTau45er2p1_RmOvlp_dR0p5              .         .         x  mass_inv_orm{JET45,JET45,TAU45[TAU-ETA_2p13,TAU-ISO_0xE]}[MASS_MIN_550,ORMDR_0p5]
   363 L1_DoubleJet45_Mass_Min600_IsoTau45er2p1_RmOvlp_dR0p5              .         .         x  mass_inv_orm{JET45,JET45,TAU45[TAU-ETA_2p13,TAU-ISO_0xE]}[MASS_MIN_600,ORMDR_0p5]
   368 L1_DoubleJet_85_35_DoubleJet35_Mass_Min600_Mu3OQ                   .         .         x  comb{JET85,JET35} AND mass_inv{JET35,JET35}[MASS_MIN_600] AND MU3[MU-QLTY_OPEN]
   369 L1_DoubleJet_85_35_DoubleJet35_Mass_Min650_Mu3OQ                   .         .         x  comb{JET85,JET35} AND mass_inv{JET35,JET35}[MASS_MIN_650] AND MU3[MU-QLTY_OPEN]
   370 L1_DoubleJet_70_35_DoubleJet35_Mass_Min500_ETMHF65                 .         .         x  comb{JET70,JET35} AND mass_inv{JET35,JET35}[MASS_MIN_500] AND ETMHF65
   371 L1_DoubleJet_70_35_DoubleJet35_Mass_Min550_ETMHF65                 .         .         x  comb{JET70,JET35} AND mass_inv{JET35,JET35}[MASS_MIN_550] AND ETMHF65
   418 L1_ETMHF70                                                         .         .         x  ETMHF70
   419 L1_ETMHF80                                                         .         .         x  ETMHF80
   420 L1_ETMHF90                                                         .         .         x  ETMHF90
   421 L1_ETMHF100                                                        .         .         x  ETMHF100
   427 L1_ETMHF70_HTT60er                                                 .         .         x  ETMHF70 AND HTT60
   428 L1_ETMHF80_HTT60er                                                 .         .         x  ETMHF80 AND HTT60
   429 L1_ETMHF90_HTT60er                                                 .         .         x  ETMHF90 AND HTT60
   430 L1_ETMHF100_HTT60er                                                .         .         x  ETMHF100 AND HTT60
   434 L1_HTMHF100                                                        .         .         x  HTMHF100
   435 L1_HTMHF120                                                        .         .         x  HTMHF120
   436 L1_HTMHF125                                                        .         .         x  HTMHF125
   437 L1_HTMHF130                                                        .         .         x  HTMHF130
   438 L1_HTMHF150                                                        .         .         x  HTMHF150
   445 L1_CICADA_VLoose                                                   .         .         x  CICADA[CICADA-CSCORE_113]
   446 L1_CICADA_Loose                                                    .         .         x  CICADA[CICADA-CSCORE_116]
   447 L1_CICADA_Medium                                                   .         .         x  CICADA[CICADA-CSCORE_121]
   448 L1_CICADA_Tight                                                    .         .         x  CICADA[CICADA-CSCORE_127]
   449 L1_CICADA_VTight                                                   .         .         x  CICADA[CICADA-CSCORE_131]
   458 L1_AlwaysTrue                                                      x         x         x  EXT_ZeroBias_BPTX_AND_VME OR ( NOT EXT_ZeroBias_BPTX_AND_VME)
   459 L1_ZeroBias                                                        x         x         x  EXT_ZeroBias_BPTX_AND_VME
   460 L1_ZeroBias_copy                                                   x         x         x  EXT_ZeroBias_BPTX_AND_VME
   461 L1_MinimumBiasHF0_AND_BptxAND                                      x         x         x  (MBT0HFP1 AND MBT0HFM1) AND EXT_ZeroBias_BPTX_AND_VME
   462 L1_MinimumBiasHF0                                                  x         x         x  MBT0HFP1 AND MBT0HFM1
   464 L1_BptxOR                                                          x         x         x  EXT_BPTX_OR_VME
   466 L1_BptxPlus                                                        x         x         x  EXT_BPTX_B1_VME
   467 L1_BptxMinus                                                       x         x         x  EXT_BPTX_B2_VME
the objects behind the conditions that are true in (12a+12b) mixed (at most two combinations per condition):
  L1_SingleMu3                                   | SingleMU_i12=1 (1 combination): Mu[0](3.0,-2.09,-2.88)
  L1_Mu3_Jet30er2p5                              | SingleMU_i12=1 (1 combination): Mu[0](3.0,-2.09,-2.88) | SingleJET_i103=1 (1 combination): Jet[1](106.5,-1.52,-1.98)
  L1_SingleEG8er2p5                              | SingleEG_i135=1 (1 combination): EG[1](8.0,2.13,-1.76)
  L1_SingleJet90                                 | SingleJET_i231=1 (2 combinations): Jet[0](137.0,-3.74,1.00) or Jet[1](106.5,-1.52,-1.98)
  L1_SingleJet120                                | SingleJET_i232=1 (1 combination): Jet[0](137.0,-3.74,1.00)
  L1_SingleJet35er2p5                            | SingleJET_i235=1 (1 combination): Jet[1](106.5,-1.52,-1.98)
  L1_DoubleJet_110_35_DoubleJet35_Mass_Min800    | DoubleJET_i352=1 (8 combinations): Jet[0](137.0,-3.74,1.00) Jet[1](106.5,-1.52,-1.98) or Jet[0](137.0,-3.74,1.00) Jet[2](75.5,-2.74,1.00) | InvariantMass_i353=1 (4 combinations): Jet[0](137.0,-3.74,1.00) Jet[3](56.0,2.74,-0.06) or Jet[2](75.5,-2.74,
  L1_DoubleJet45_Mass_Min550_IsoTau45er2p1_RmOvlp_dR0p5 | InvariantMassOvRm_i358=1 (4 combinations): Jet[0](137.0,-3.74,1.00) Jet[3](56.0,2.74,-0.06) Tau[0](59.5,-1.57,-1.93) or Jet[2](75.5,-2.74,1.00) Jet[3](56.0,2.74,-0.06) Tau[0](59.5,-1.57,-1.93)
  (the other algorithms: out/ledger_12_L1T.txt)
the ladders: the event against the thresholds of the single-object families (GeV); margin = value - threshold
(12a) signal
   SingleJet            value   113.0   highest passed: 90 (margin +23.0)   lowest failed: 120 (margin -7.0)
   SingleJet..er2p5     value   102.5   highest passed: 35 (margin +67.5)   lowest failed: 120 (margin -17.5)
   SingleJet.._FWD3p0   value   113.0   highest passed: 35 (margin +78.0)   lowest failed: 120 (margin -7.0)
   DoubleJet..er2p5     value     0.0   highest passed: none   lowest failed: 40 (margin -40.0)
   HTT..er              value   102.5   highest passed: none   lowest failed: 120 (margin -17.5)
   ETT                  value   188.5   highest passed: none   lowest failed: 2000 (margin -1811.5)
   ETMHF                value    65.5   highest passed: none   lowest failed: 70 (margin -4.5)
   SingleEG..er2p5      value     3.5   highest passed: none   lowest failed: 8 (margin -4.5)
   SingleTau..er2p1     value    54.5   highest passed: none   lowest failed: 120 (margin -65.5)
   SingleMu             value     0.0   highest passed: none   lowest failed: 3 (margin -3.0)
(12b) pileup
   SingleJet            value     0.0   highest passed: none   lowest failed: 35 (margin -35.0)
   SingleJet..er2p5     value     0.0   highest passed: none   lowest failed: 35 (margin -35.0)
   SingleJet.._FWD3p0   value     0.0   highest passed: none   lowest failed: 35 (margin -35.0)
   DoubleJet..er2p5     value     0.0   highest passed: none   lowest failed: 40 (margin -40.0)
   HTT..er              value     0.0   highest passed: none   lowest failed: 120 (margin -120.0)
   ETT                  value    99.0   highest passed: none   lowest failed: 2000 (margin -1901.0)
   ETMHF                value     3.5   highest passed: none   lowest failed: 70 (margin -66.5)
   SingleEG..er2p5      value     0.0   highest passed: none   lowest failed: 8 (margin -8.0)
   SingleTau..er2p1     value     0.0   highest passed: none   lowest failed: 120 (margin -120.0)
   SingleMu             value     0.0   highest passed: none   lowest failed: 3 (margin -3.0)
(12a+12b) mixed
   SingleJet            value   137.0   highest passed: 120 (margin +17.0)   lowest failed: 180 (margin -43.0)
   SingleJet..er2p5     value   106.5   highest passed: 35 (margin +71.5)   lowest failed: 120 (margin -13.5)
   SingleJet.._FWD3p0   value   137.0   highest passed: 120 (margin +17.0)   lowest failed: none
   DoubleJet..er2p5     value    18.0   highest passed: none   lowest failed: 40 (margin -22.0)
   HTT..er              value   106.5   highest passed: none   lowest failed: 120 (margin -13.5)
   ETT                  value  1339.0   highest passed: none   lowest failed: 2000 (margin -661.0)
   ETMHF                value   106.5   highest passed: 100 (margin +6.5)   lowest failed: 110 (margin -3.5)
   SingleEG..er2p5      value     8.0   highest passed: 8 (margin +0.0)   lowest failed: 10 (margin -2.0)
   SingleTau..er2p1     value    59.5   highest passed: none   lowest failed: 120 (margin -60.5)
   SingleMu             value     3.0   highest passed: 3 (margin +0.0)   lowest failed: 5 (margin -2.0)
```

Some conditions as the menu defines them (`l1t_mixed_12_L1T.log`, `condition:` lines; cut type 0 = threshold in GeV,
1 = eta window, 4 = quality bit mask, 9 = mass as M^2/2, 11 = count, 25 = CICADA score): the muon quality masks are
61440 = bits 12-15 (`MU-QLTY_SNGL`), 65280 = bits 8-15 (`DBLE`), 65520 = bits 4-15 (`OPEN`).

```
  condition: name=SingleMU_i12 type=0 | object MU3 bx 0: MU-ET_3[type 0: 3..1.79769e+308] MU-QLTY_SNGL[type 4: 0..0 data 61440]
  condition: name=SingleJET_i231 type=12 | object JET90 bx 0: JET-ET_90[type 0: 90..1.79769e+308]
  condition: name=SingleJET_i235 type=12 | object JET35 bx 0: JET-ET_35[type 0: 35..1.79769e+308] JET-ETA_2p52[type 1: -2.523..2.523]
  condition: name=SingleJET_i244 type=12 | object JET90 bx 0: JET-ET_90[type 0: 90..1.79769e+308] JET-ETA_FWD_2p48_NEG[type 1: -5..-2.4795]
  condition: name=SingleJET_i250 type=12 | object JET120 bx 0: JET-ET_120[type 0: 120..1.79769e+308] JET-ETA_FWD_3p00_NEG[type 1: -5..-3.0015]
  condition: name=DoubleJET_i352 type=13 | object JET110 bx 0: JET-ET_110[type 0: 110..1.79769e+308] | object JET35 bx 0: JET-ET_35[type 0: 35..1.79769e+308]
  condition: name=InvariantMass_i353 type=26 | object JET35 bx 0: JET-ET_35[type 0: 35..1.79769e+308] | object JET35 bx 0: JET-ET_35[type 0: 35..1.79769e+308] | condition cuts: MASS_MIN_800[type 9: 320000..1.15493e+10]
  condition: name=SingleETMHF_i299 type=32 | object ETMHF100 bx 0: ETMHF-ET_100[type 0: 100..1.79769e+308]
  condition: name=CicadaTrigger_i436 type=75 | object CICADA bx 0: CICADA-CSCORE_131[type 25: 131..0]
  condition: name=SingleMBT0HFP_i314 type=27 | object MBT0HFP1 bx 0: MBT0HFP-COUNT_1[type 11: 1..1.79769e+308]
```

The 408 algorithms by name: 180 contain `Mu`, 89 `EG`, 27 `Tau`, 93 are made of jets and sums only; 5 `L1_CICADA_*`
and 5 `L1_AXO_*`. The HLT paths seeded by the single-jet algorithms (`HLTrigger/Configuration/python/HLT_GRun_cff.py`,
`L1SeedsLogicalExpression` of the `hltL1s*` modules): `L1_ZeroBias` -> `HLT_PFJet40/60/80`, `HLT_PFJetFwd40/60`;
`L1_SingleJet90` -> `HLT_PFJet110`, `HLT_PFJet140`; `L1_SingleJet120` -> `HLT_PFJet200`; `L1_SingleJet60_FWD2p5`,
`90_FWD2p5`, `120_FWD2p5` -> `HLT_PFJetFwd80`, `140`, `200`; `L1_SingleJet180 OR L1_SingleJet200` (and 170) ->
`HLT_PFJet260` and above.

### The conditions behind the emulation (section B5 of the chapter)

`conddb list 150X_mcRun3_2024_realistic_v2`, the L1 records and their tags: `L1TUtmTriggerMenuRcd`
`L1Menu_Collisions2024_v1_3_0_xml`; `L1TCaloParamsRcd` `L1TCaloParams_static_2024_v0_1_mc`;
`L1TGlobalPrescalesVetosRcd` `L1TGlobalPrescalesVetos_passThrough_mc`; `L1TMuonBarrelParamsRcd`
`L1TMuonBarrelParams_Stage2v1_2018_mc`; `L1TMuonOverlapParamsRcd` `L1TMuonOverlapParams_static_2023_mc`;
`L1TMuonEndCapParamsRcd` `L1TMuonEndCapParams_Run3_2023_mc`; `L1TMuonEndCapForestRcd`
`L1TMuonEndCapForest_v9_Run3_2023_mc`; `L1TMuonGlobalParamsRcd` `L1TMuonGlobalParams_Stage2v0_2024_mc_v1`; 19
`EcalTPG*Rcd` payloads (9 tagged `2018_mc`, 5 `2024_mc`, 2 `2022_mc`, 3 without a year); `HcalTPParametersRcd`
`HcalTPParameters_2019_v1.0_mc`, `HcalLutMetadataRcd` `HcalLutMetadata_2019_v1.0_mc`, `HcalTPChannelParametersRcd`
`HcalTPChannelParameters_2022_v1.1_mc`.

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
