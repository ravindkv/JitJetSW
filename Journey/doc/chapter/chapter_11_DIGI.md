# Chapter 11, DIGI: technical companion

The digitisation of the SimHits of chapter 10, for the signal alone (11a), for one pileup collision alone (11b) and
for the signal with its pileup mixed in (11a+11b). Script: `test/11_DIGI/run_11_DIGI.sh`; everything it produces is
in `test/11_DIGI/out/`. The chapter text is `doc/chapter/chapter_11_DIGI.tex`. This is the last chapter with the three
samples (Rule H of the Constitution): from chapter 12 on only the mixed event is followed, so everything a later
chapter needs to know about the signal alone is computed here. The chapter has a hand-over section (Rule B6), "Made
here for the next stage: the trigger primitives": the DIGI step also makes the calorimeter trigger primitives that the
Level-1 trigger of chapter 12 uses; sections 3 and 4.7 of this companion are about them. Section 3 (numbers of the event)
of the chapter is split into one subsection per digitiser, each with a channel or a data frame followed by hand; those
worked examples come from `test/11_DIGI/digiExamples_11_DIGI.py` (section 3 below, output in section 4.10) and from the
payloads and release files of section 4.11. All commands below are run from
`test/` of an initialized CMSSW_15_0_5 (`cmsenv`), with `J=$CMSSW_BASE/src/JitJetSW/Journey`.

Since 21 November 2026 the chapter mixes the sixteen bunch crossings -12..3 of the Run-3 production (995 library
collisions: 746 early, 64 in time, 185 late; the pileup sample (11b) is library event 747), as chapters 08-10 do. Until
then it mixed bx -1..1 (193 collisions, 71 / 68 / 54, sample (11b) = library event 72); the outputs of that version are
kept in `test/tmp/out_bx-1to1/11_DIGI/out/`.

## 1. The cmsDriver.py commands

The script runs `cmsDriver.py` three times through `run_driver` of `test/common/jitjet_common.sh`. The commands,
copied from the header of the generated configurations (`test/11_DIGI/step3_DIGI_<case>_11_DIGI_cfg.py`, "with
command line options"; the cmsDriver output is in `out/step3_DIGI_<case>_11_DIGI_cfg.driver.log`):

```bash
cd $J/test
# (11a) the signal alone
cmsDriver.py step3 --filein file:$J/test/10_SIM/out/step2_SIM_signal_10_SIM.root \
  --conditions auto:phase1_2025_realistic --era Run3_2025 --geometry DB:Extended \
  -n -1 -s DIGI:pdigi_valid --datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG --fileout file:step3_DIGI_signal_11_DIGI.root \
  --customise_commands "process.source.eventsToProcess = cms.untracked.VEventRange('1:1:1')" \
  --python_filename $J/test/11_DIGI/step3_DIGI_signal_11_DIGI_cfg.py --no_exec

# (11b) library event 747 alone
cmsDriver.py step3 --filein file:$J/test/10_SIM/out/step2_SIM_pileup_10_SIM.root \
  --conditions auto:phase1_2025_realistic --era Run3_2025 --geometry DB:Extended \
  -n -1 -s DIGI:pdigi_valid --datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG --fileout file:step3_DIGI_pileup_11_DIGI.root \
  --customise_commands "process.source.eventsToProcess = cms.untracked.VEventRange('1:1:747')" \
  --python_filename $J/test/11_DIGI/step3_DIGI_pileup_11_DIGI_cfg.py --no_exec

# (11a+11b) the signal with PU = 60 per bunch crossing in bx -12..3, the collisions of the chapter 10 mixing (playback)
cmsDriver.py step3 --filein file:$J/test/10_SIM/out/step3_MIX_10_SIM.root \
  --pileup "AVE_60_BX_25ns,{'N': 60, 'B': (-12, 3)}" --pileup_input file:$J/test/10_SIM/out/step2_SIM_pileup_10_SIM.root \
  --conditions auto:phase1_2025_realistic --era Run3_2025 --geometry DB:Extended \
  -n -1 -s DIGI:pdigi_valid --datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG --fileout file:step3_DIGI_mixed_11_DIGI.root \
  --customise_commands "process.source.eventsToProcess = cms.untracked.VEventRange('1:1:1')\nprocess.mix.input.sequential = cms.untracked.bool(True)\nprocess.mix.playback = cms.untracked.bool(True)" \
  --python_filename $J/test/11_DIGI/step3_DIGI_mixed_11_DIGI_cfg.py --no_exec
```

| argument | meaning |
|---|---|
| `step3` | the name of the configuration (free; `step1` was GEN, `step2` SIM) |
| `--filein file:...` | the input (Rule E): the SIM file of the signal (11a), the 1000-event library (11b), or the chapter-10 mixing output `step3_MIX_10_SIM.root`, which is the signal plus the `CrossingFramePlaybackInfoNew` of its mixing (11a+11b) |
| `--pileup "AVE_60_BX_25ns,{'N': 60, 'B': (-12, 3)}"` | the mixing scenario: `mix_POISSON_average_cfi` (`Configuration/StandardSequences/python/Mixing.py`), Poisson with mean `N = 60` collisions per bunch crossing, bunch crossings `B = (-12, 3)` (the window of every Run-3 scenario of the release, `SimGeneral/MixingModule/python/mix_probFunction_25ns_PoissonOOTPU_cfi.py`, lines 11-12; `B = (-12,3)` is also the default of `AVE_60_BX_25ns`, `Mixing.py` line 162), 25 ns spacing. Only for (11a+11b); without `--pileup` cmsDriver loads `mixNoPU_cfi`, the same module with no secondary source |
| `--pileup_input file:...` | the library the secondary source reads: `step2_SIM_pileup_10_SIM.root` of chapter 10 |
| `--conditions auto:phase1_2025_realistic` | the global tag of the CMS chapters of the book (Rule I of the Constitution: CMSSW_15_0_5 is a release of 2025), `150X_mcRun3_2025_realistic_v1` (`Configuration/AlCa/python/autoCond.py`): the pedestals, noise, gains, thresholds, laser transparencies and dead channels that the digitisers read from the database. Against the 2024 global tag of the first version of this chapter (`150X_mcRun3_2024_realistic_v2`) it changes, for this step: `EcalPedestals_2025_at80fb_corrected_mc` (noisier barrel crystals), `EcalLaserAPDPNRatios_TL470fb_moy_2025_outliers_cleaned_3sigma_mc` (darker crystals), `EcalSRSettings_2024_EBZS9.5_EEZS6.0_DCC2_mc` (higher barrel zero suppression), nine `EcalTPG*` payloads (the trigger primitives), `HcalPedestals` / `HcalPedestalWidths` / `HcalRespCorrs` / `HcalSiPMParameters` / `HcalZSThresholds` `2025_MOY`, `HcalTPChannelParameters_2024_v2.0_mc`, `SiPixelLorentzAngleSim_phase1_EoR3_HV800_Tr2000` and the pixel templates of the charge reweighting (`SiPixel2DTemplateDBObject` `numerator` / `denominator`, `phase1_EoR3_HV800_Tr2000`), `SiStripBadComponents_realisticMC_for2025_v1_mc`; section 4.9 |
| `--era Run3_2025` | the era modifiers of the 2025 detector: they set the Run-3 pixel thresholds (`run3_common`), the HB SiPM parameters (`run3_HB`), the 10-time-slice HB/HE readout and the one-slice zero suppression (`run3_common`), the GEM luminosity, the Run-2 CSC timing offsets (`run2_common`). `Run3_2025` is `Run3_2024` plus `run3_GEM_2025`, `run3_CSC_2025`, `run3_SiPixel_2025`, `stage2L1Trigger_2025` and `ecal_cctiming` (`Configuration/Eras/python/Era_Run3_2025_cff.py`); none of them touches the DIGI step (they change the GEM rec-hits, the CSC trigger primitives of the L1 step, the pixel clusteriser, the CICADA model of the L1 step and the ECAL rec-hit timing): the `dumpPython()` of the mixed configuration is the same with both eras except for the name of the global tag |
| `--geometry DB:Extended` | the reconstruction geometry from the database (`GeometryRecoDB_cff`): the digitisers need the module and cell positions |
| `-n -1` with `eventsToProcess` | all events of the input, restricted by the customise line to run:lumi:event 1:1:1 (the signal) or 1:1:747 (library event 747) |
| `-s DIGI:pdigi_valid` | the step: the `pdigi_valid` sequence of `Configuration/StandardSequences/python/Digi_cff.py` (section 2) |
| `--datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG` | the output tier and the full debug content: every digi and link collection, the SIM products of the signal, the truth |
| `--fileout file:...` | the output ROOT file (written in `out/`, where `cmsRun` is started) |
| `--customise_commands "..."` | python lines appended to the configuration: the event selection; for (11a+11b) `process.mix.input.sequential = True` (the library is read from its first event on, not from a random position) and `process.mix.playback = True` (the module replays the chapter-10 draw from the `CrossingFramePlaybackInfoNew` of the input file instead of drawing again) |
| `--python_filename ... --no_exec` | write the configuration and do not run it; the script runs `cmsRun` itself |

Why playback: `BMixingModule::produce` (`Mixing/Base/src/BMixingModule.cc`, lines 292-299) calls `initializeEvent`,
`addSignals` and then `doPileUp`; `addSignals` runs the digitisers on the signal, which draw random numbers (noise,
photostatistics, threshold smearing) from the engine of the module `mix` before `PileUp::CalculatePileup`
(`Mixing/Base/src/PileUp.cc`, called at line 445 of `SimGeneral/MixingModule/plugins/MixingModule.cc`) draws the number
of collisions. In chapter 10 (`mixOnly_cff`, no digitiser) the draw with seed 12345 gave, for bx -12..3, 71 68 54 59
52 61 76 59 65 59 68 54 | 64 | 63 62 60 (the draw of chapter 08, `test/08_PU/out/draw_08_PU.txt`); a fresh draw in the
DIGI job would differ (with bx -1..1 it gave 65 / 67 / 56 instead of 71 / 68 / 54). With `playback = True`
(`MixingModule.cc`, lines 416-483) the module reads `CrossingFramePlaybackInfoNew` from the input and mixes exactly
library events 1-746 (bx -12..-1), 747-810 (bx 0) and 811-995 (bx +1..+3), as `addPileupInfo` confirms (section 4.5).

## 2. The cmsRun on the python config

```bash
cd $J/test/11_DIGI/out
cmsRun ../step3_DIGI_signal_11_DIGI_cfg.py > step3_DIGI_signal_11_DIGI.log 2>&1     # 21 s (9 s for the event)
cmsRun ../step3_DIGI_pileup_11_DIGI_cfg.py > step3_DIGI_pileup_11_DIGI.log 2>&1     # 21 s (9 s for the event)
cmsRun ../step3_DIGI_mixed_11_DIGI_cfg.py  > step3_DIGI_mixed_11_DIGI.log  2>&1     # 36 s (27 s for the event; 995 library events read)
```

Or the whole chapter: `./11_DIGI/run_11_DIGI.sh` (variables: `CASES="signal pileup mixed"`, `REUSE=1` keeps existing
DIGI files; `PU=60`, `BUNCHES=-12,3`, `SKIP=746`, `NPU=64` are set in `test/common/jitjet_common.sh` for chapters 08-13;
`RUN=0` only writes the configurations, `ANALYZE=0` skips the analyzers, `PNG=0` skips the PNG pages). The three
DIGI jobs run in parallel; the times are in `run_11_DIGI.log`, `out/run_signal.log` and `out/run_pileup.log` (21 November
2026, three jobs started together at 13:30:34 on a shared machine; the whole chapter took 6 minutes, most of it the
three DigiAnalyzer pages, 1.5 minutes, 40 s and 3 minutes).

### Walk-through of `step3_DIGI_mixed_11_DIGI_cfg.py`

- `process = cms.Process('DIGI', Run3_2025)`: the process name under which the new products are stored.
- `process.load(...)`: `Services_cff` (the `RandomNumberGeneratorService`: the module `mix` has one seeded engine,
  seed 12345, from which every digitiser inside it draws; the muon digitisers, separate modules, have their own), `pythiapdt_cfi`, the `MessageLogger`, `EventContent_cff`,
  `SimGeneral.MixingModule.mix_POISSON_average_cfi` (the `MixingModule` with `theDigitizers`, `theMixObjects` and the
  secondary source `EmbeddedRootSource`; for (11a) and (11b) it is `mixNoPU_cfi`), `GeometryRecoDB_cff`,
  `MagneticField_cff`, `Configuration.StandardSequences.Digi_cff` (the `pdigi_valid` sequence), `EndOfProcess_cff`,
  `FrontierConditions_GlobalTag_cff`.
- `process.source`: `PoolSource` on the input file with `inputCommands` that drop the generator jets, `genParticles`
  and the generator MET of the input (they are remade by `fixGenInfoTask` inside `pdigi`, so that the file carries
  them under the process name `DIGI`); the customise line restricts it to one event.
- `process.FEVTDEBUGoutput`: the `PoolOutputModule` with the `FEVTDEBUGEventContent` commands.
- "Other statements" (only with `--pileup`): `process.mix.input.nbPileupEvents.averageNumber = 60`,
  `process.mix.bunchspace = 25`, `process.mix.minBunch = -12`, `process.mix.maxBunch = 3`, `process.mix.input.fileNames`
  (the library), and `process.mix.digitizers = cms.PSet(process.theDigitizersValid)`.
- `process.digitisation_step = cms.Path(process.pdigi_valid)`, then `endjob_step` and the output `EndPath`; the
  schedule runs them in this order. `customiseEarlyDelete` is appended by cmsDriver.
- The customise lines: `eventsToProcess`, `process.mix.input.sequential = True`, `process.mix.playback = True`.

What `pdigi_valid` runs (`Configuration/StandardSequences/python/Digi_cff.py`): `generatorSmeared` (passed through),
`randomEngineStateProducer`, the module `mix`, the calorimeter task (`SimCalorimetry/Configuration/python/SimCalorimetry_cff.py`:
`simEcalTriggerPrimitiveDigis`, `simEcalDigis` = the selective readout, `simEcalPreshowerDigis` = the ES zero
suppression, `simHcalTriggerPrimitiveDigis`, `simHcalDigis` = the HCAL zero suppression), the muon task
(`SimMuon/Configuration/python/SimMuon_cff.py`: `simMuonCSCDigis`, `simMuonDTDigis`, `simMuonRPCDigis`,
`simMuonGEMDigis` under `run3_GEM`), the PPS task, `addPileupInfo`, `genPUProtons`, the generator-jet remake
(`fixGenInfoTask`) and the pruning of the links and the truth (`tpPruningTask`: `prunedDigiSimLinks`,
`prunedTrackingParticles`). The pixel, strip, ECAL and HCAL digitisers are not modules of the path: they are the
sub-parameter-sets of `process.mix.digitizers` (`SimGeneral/MixingModule/python/digitizers_cfi.py`: `pixel`, `strip`,
`ecal`, `hcal`, `puVtx`, `mergedtruth`; `castor` removed by `run3_common`) and run inside the `MixingModule`, on the
signal hits and on the pileup hits of every bunch crossing, before the `CrossingFrame`s are discarded.
`theDigitizersValid` differs from `theDigitizers` only in the truth: it keeps the `TrackingParticle`s of the pileup too
(`mergedtruth.select.signalOnlyTP = False`).

Where the numbers of the digitisers are: `python3 $J/test/common/digiThresholds.py $J/test/11_DIGI/step3_DIGI_mixed_11_DIGI_cfg.py`
loads the configuration and prints every parameter of `process.mix.digitizers.*` and of the digi producers after the
mixing whose name looks like a threshold, a noise, a zero suppression, a window or a cut (section 1 of the output),
then the full parameter sets (section 2); the script writes it to `out/thresholds_11_DIGI.txt`. The values that are
not in the python are DB payloads of the global tag: `SiStripThreshold`, `SiStripNoises`, `SiStripPedestals`,
`SiPixelQuality` (label `forDigitizer`), `SiPixelGainCalibration`, `EcalSRSettings` (`configFromCondDB = True`),
`EcalPedestals`, `ESThresholds` and `ESPedestals` (read in `SimCalorimetry/EcalZeroSuppressionProducers/src/ESZeroSuppressionProducer.cc`),
`HcalZSThresholds` (`useConfigZSvalues = 0`), `HcalPedestals`, `HcalGains`.

### The content of the output ROOT file

`edmDumpEventContent file:$J/test/11_DIGI/out/step3_DIGI_mixed_11_DIGI.root` (12 MB for one event; the (11a) file is
2.1 MB, the (11b) file 1.0 MB). The SIM products of the signal (`SimTrack`, `SimVertex`, every `PSimHit` and `PCaloHit`
collection, process `SIMFULL`), the generator record (`GEN`, `GENJET`) and the playback record of chapter 10
(`CrossingFramePlaybackInfoNew mix`, process `MIX`) are copied. New under the process `DIGI`:

| product | what it is |
|---|---|
| `edm::DetSetVector<PixelDigi> simSiPixelDigis` | the pixel digis (channel, ADC); `edmNew::DetSetVector<PixelFEDChannel> simSiPixelDigis`: the FED channels killed |
| `edm::DetSetVector<PixelDigiSimLink> simSiPixelDigis` and `prunedDigiSimLinks:siPixel` | per channel, the (EncodedEventId, track id) pairs and their charge fraction |
| `edm::DetSetVector<SiStripDigi> simSiStripDigis:ZeroSuppressed` | the strip digis after the FED zero suppression; `SiStripRawDigi` `VirginRaw`, `ProcessedRaw`, `ScopeMode`, `StripAmplitudes`, `StripAmplitudesPostAPV`, `StripAPVBaselines`: the raw modes (empty collections unless configured) |
| `edm::DetSetVector<StripDigiSimLink> simSiStripDigis` and `prunedDigiSimLinks:siStrip` | the strip links |
| `EBDigiCollection simEcalDigis:ebDigis`, `EEDigiCollection simEcalDigis:eeDigis` | the ECAL data frames after the selective readout (10 samples with gain id); `EBSrFlag` / `EESrFlag` `simEcalDigis:ebSrFlags/eeSrFlags`: the selective-readout flag per trigger tower / supercrystal |
| `ESDigiCollection simEcalPreshowerDigis` | the preshower frames (3 samples) after the zero suppression |
| `EcalTriggerPrimitiveDigi simEcalTriggerPrimitiveDigis` | the ECAL trigger primitives: 4032 per event (one per trigger tower, EB 2448 + EE 1584), each with 5 time samples (the sample of interest is index 2) of 8-bit compressed ET (0.5 GeV per count), a fine-grain bit and the trigger-tower flag. They drive the selective readout inside this job and are step 1 of the Level-1 trigger of chapter 12 |
| `HcalDataFrameContainer<QIE11DataFrame> simHcalDigis:HBHEQIE11DigiCollection`, `<QIE10DataFrame> simHcalDigis:HFQIE10DigiCollection`, `HODataFrame simHcalDigis` | the HCAL frames after the zero suppression; the same three under `simHcalUnsuppressedDigis` are the frames of every channel (plus the legacy `HBHEDataFrame`, `HFDataFrame` collections, empty, and the `ZDCDataFrame`) |
| `HcalTriggerPrimitiveDigi simHcalTriggerPrimitiveDigis` | the HCAL trigger primitives: 4824 per event (HB 2304, HE 1728, HF 792), 4 time samples for HB / HE (2 presamples) and 2 for the HF, 8-bit compressed ET and feature bits; decoded to GeV with the `CaloTPGTranscoder` of the event setup (`hcaltpdigi_cff`). Used by the Level-1 trigger of chapter 12 only |
| `MuonDigiCollection<DTLayerId,DTDigi> simMuonDTDigis` and `<DTLayerId,DTDigiSimLink>` | DT wire digis (TDC time) and links |
| `MuonDigiCollection<CSCDetId,CSCStripDigi/CSCWireDigi/CSCComparatorDigi> simMuonCSCDigis` and `edm::DetSetVector<StripDigiSimLink> simMuonCSCDigis:MuonCSCStripDigiSimLinks/MuonCSCWireDigiSimLinks` | CSC strip (8 SCA samples), wire-group and comparator digis and links |
| `MuonDigiCollection<RPCDetId,RPCDigi> simMuonRPCDigis` and `edm::DetSetVector<RPCDigiSimLink> simMuonRPCDigis:RPCDigiSimLink` | RPC strip digis (strip, bx) and links |
| `MuonDigiCollection<GEMDetId,GEMDigi> simMuonGEMDigis` and `edm::DetSetVector<GEMDigiSimLink> simMuonGEMDigis:GEM` | GEM strip digis and links |
| `vector<PileupSummaryInfo> addPileupInfo`, `int addPileupInfo:bunchSpacing` | per bunch crossing of the window: the number of collisions mixed and the "true" number |
| `vector<TrackingParticle> mix:MergedTrackTruth`, `vector<TrackingVertex> mix:MergedTrackTruth`, `prunedTrackingParticles` | the truth of the signal and of every pileup collision, each with its `EncodedEventId` |
| `CrossingFramePlaybackInfoNew mix` (process `DIGI`) | the playback record of this job (identical to the chapter-10 one) |
| `vector<reco::GenJet> ak4GenJets` etc., `genParticles`, `genMetTrue`, `genPUProtons` | the generator jets, particles and MET remade under `DIGI` (the input ones were dropped by `inputCommands`) |
| `edm::HepMCProduct generatorSmeared`, `RandomEngineStates randomEngineStateProducer`, `TriggerResults` | the pass-through and bookkeeping products |

Not in the file: the `CrossingFrame`s (transient, built and consumed inside the `MixingModule`), the pileup
`SimTrack`s and hits, and the unsuppressed ECAL digis (`simEcalUnsuppressedDigis` is not part of `FEVTDEBUG`).

## 3. The analyzer configs

The DigiAnalyzer (`$J/plugins/DigiAnalyzer.cc`, configuration `test/common/digiAnalyzer_cfg.py`) runs once per
sample; the commands, from `run_11_DIGI.log`:

```bash
cd $J/test/11_DIGI/out
cmsRun ../../common/digiAnalyzer_cfg.py inputFiles=file:$J/test/11_DIGI/out/step3_DIGI_signal_11_DIGI.root \
  outputFile=digiAnalyzer_signal_11_DIGI.root maxEvents=1 eventsToProcess=1:1:1 \
  globalTag=auto:phase1_2025_realistic era=Run3_2025 geometry=DB:Extended \
  pdfFile=digiAnalyzer_signal_11_DIGI.pdf pngPrefix=digiAnalyzer_signal_11_DIGI savePNG=1 \
  sampleLabel="chapter 11_DIGI (11a): the signal digitised|bbbar, pTHat > 100 GeV|2025 detector, 13.6 TeV|no pileup" \
  > digiAnalyzer_signal_11_DIGI.log 2>&1
cmsRun ../../common/digiAnalyzer_cfg.py inputFiles=file:$J/test/11_DIGI/out/step3_DIGI_pileup_11_DIGI.root \
  outputFile=digiAnalyzer_pileup_11_DIGI.root maxEvents=1 eventsToProcess=1:1:747 \
  globalTag=auto:phase1_2025_realistic era=Run3_2025 geometry=DB:Extended \
  pdfFile=digiAnalyzer_pileup_11_DIGI.pdf pngPrefix=digiAnalyzer_pileup_11_DIGI savePNG=1 \
  sampleLabel="chapter 11_DIGI (11b): one pileup collision digitised|library event 747|SoftQCD inelastic, 13.6 TeV|2025 detector, no signal" \
  > digiAnalyzer_pileup_11_DIGI.log 2>&1
cmsRun ../../common/digiAnalyzer_cfg.py inputFiles=file:$J/test/11_DIGI/out/step3_DIGI_mixed_11_DIGI.root \
  outputFile=digiAnalyzer_mixed_11_DIGI.root maxEvents=1 eventsToProcess=1:1:1 \
  globalTag=auto:phase1_2025_realistic era=Run3_2025 geometry=DB:Extended \
  pdfFile=digiAnalyzer_mixed_11_DIGI.pdf pngPrefix=digiAnalyzer_mixed_11_DIGI savePNG=1 \
  sampleLabel="chapter 11_DIGI (11a+11b): signal + pileup digitised|PU = 60 per bx (Poisson), bx -12..3|the collisions of the chapter 10 mixing (playback)|2025 detector, 13.6 TeV" \
  > digiAnalyzer_mixed_11_DIGI.log 2>&1
python3 ../../common/digiThresholds.py ../step3_DIGI_mixed_11_DIGI_cfg.py > thresholds_11_DIGI.txt
python3 ../../common/digiLedger.py . 11_DIGI > ledger_11_DIGI.txt
python3 ../digiExamples_11_DIGI.py . > examples_11_DIGI.txt 2> examples_11_DIGI.log     # about 1 minute
```

`digiExamples_11_DIGI.py` is not an analyzer but an FWLite script (run by `run_11_DIGI.sh` after the ledger): it reads
the signal and the mixed DIGI files and the APV pulse file of the release and prints the worked examples of the chapter
(section 4.10): (1) the thickness of the silicon sensors (the largest |z_exit - z_entry| of the PSimHits in the local
frame of their module), the charge of the crossings of the full thickness (energy loss / 3.61 eV, per micrometre of
path), the ADC of the pixel and strip digis (strip digis split by whether a `StripDigiSimLink` exists for the channel), the
response of `SiPixelDigitizerAlgorithm::missCalibrate` with the parameters of `thresholds_11_DIGI.txt`; (2) the APV25
deconvolution pulse `SimTracker/SiStripDigitizer/data/APVShapeDeco_320.txt` read at the times of bunch crossings -1, 0,
+1, and the points of figure 11.3; (3) the five largest EB and EE frames of the signal, sample by sample, with the
in-time energy (PCaloHits of 0 to 100 ns) of the crystal, the frames in towers / supercrystals read in full (from the
`ebSrFlags` / `eeSrFlags`), the crystals with a deposit above 0.1, 0.3325, 0.5 and 1 GeV and how many of them have a frame,
and for the EE their approximate eta; (4) the four largest HB/HE (QIE11, with the TDC) and HF (QIE10) frames and the
median frame; (5) the muon digis of the mixed event (DT times, CSC strips per chamber and their 8 samples, CSC wire time
bins, RPC and GEM bunch crossings; a small C++ helper declared through `ROOT.gInterpreter` iterates the
`MuonDigiCollection`s, which PyROOT cannot). Products: `examples_11_DIGI.txt`, `examples_11_DIGI.log`.

What the analyzer does: it reads the digi collections of the table above, places every digi at the position of its
pixel / strip (module topology), crystal / tower (`CaloGeometry`), wire / strip (muon geometries) and draws two pages,
eta-phi (seen from the signal vertex `genParticles:xyz0`) and r-z. Tracker and muon digis are attributed through
their links: for every channel the link with the largest charge fraction decides; `EncodedEventId` bx 0 event 0 is
the signal, bx 0 event > 0 in-time pileup, bx < 0 early, bx > 0 late, no link at all is "noise". Calorimeter digis
are sized by their amplitude above pedestal in ADC counts: ECAL, the largest of the 10 samples (at gain 12
equivalent) minus the mean of the first 3; ES, the largest of the 3 minus the first; HCAL, the sum of the samples
`hcalNSumTS = 2` from the sample of interest of the frame minus the mean of the earlier ones (a frame whose amplitude
comes out negative is not drawn or counted: this is why the HF, which has no zero suppression and 3456 frames in
every file, is counted with 2270 / 2145 / 2185 digis; with the out-of-time pileup in the earlier samples 2106 of the
7075 HB/HE frames kept in the mixed event come out negative). The signal `ak4GenJets` above `jetPtMin = 30` GeV are drawn
as circles of radius 0.4. Tracker and muon digis are thinned for the drawing (`maxDrawn = 8000` per subdetector);
the counts are exact. Products:

| file | content |
|---|---|
| `digiAnalyzer_<case>_11_DIGI.pdf` | 2 pages: eta-phi, r-z (A4 MediaBox with a CropBox, do not include directly) |
| `digiAnalyzer_<case>_11_DIGI_etaphi_run1_event<E>.png`, `..._rz_run1_event<E>.png` | the same pages as PNG (E = 1 for the signal and the mixed event, 747 for the pileup collision) |
| `digiAnalyzer_<case>_11_DIGI.log` | the counts: per subdetector `nDigis`, `signal / inTime / early / late / noLink` (tracker and muon) or `sumAmplitude[ADC]` (calorimeters), the pileup summary, the total |
| `digiAnalyzer_<case>_11_DIGI.root` | the `TFileService` file (empty histograms directory; the drawing goes to the PDF) |
| `thresholds_11_DIGI.txt` | every threshold-like parameter of the digitisers of the mixed configuration, then the full parameter sets (now including the two trigger-primitive producers, `simEcalTriggerPrimitiveDigis` and `simHcalTriggerPrimitiveDigis`) |
| `ledger_11_DIGI.txt` | the three logs side by side: digis per subdetector and origin, calorimeter amplitudes |
| `counts_11_DIGI.txt` | `11_DIGI/digiFileCounts_11_DIGI.py` (FWLite on the three DIGI files): the pileup per bunch crossing, the selective-readout flags, the HCAL frames before and after the zero suppression, the ECAL frames written, the TrackingParticles by origin (section 4.5) |

### The trigger primitives (`TrigPrimAnalyzer`)

The second analyzer of the chapter (`$J/plugins/TrigPrimAnalyzer.cc`, configuration
`test/common/trigPrimAnalyzer_cfg.py`) looks at what the DIGI step makes for the Level-1 trigger. It also runs once
per sample (the function `trigprim` of `run_11_DIGI.sh`); the commands, from `run_11_DIGI.log`:

```bash
cd $J/test/11_DIGI/out
cmsRun ../../common/trigPrimAnalyzer_cfg.py inputFiles=file:$J/test/11_DIGI/out/step3_DIGI_signal_11_DIGI.root \
  maxEvents=1 eventsToProcess=1:1:1 globalTag=auto:phase1_2025_realistic era=Run3_2025 geometry=DB:Extended \
  pdfFile=trigPrim_signal_11_DIGI.pdf pngPrefix=trigPrim_signal_11_DIGI savePNG=1 mixed=0 \
  sampleLabel="chapter 11_DIGI (11a): the signal alone|trigger primitives made by the DIGI step|bbbar, pTHat > 100 GeV, no pileup" \
  > trigPrim_signal_11_DIGI.log 2>&1                           # 4-6 s each
cmsRun ../../common/trigPrimAnalyzer_cfg.py inputFiles=file:$J/test/11_DIGI/out/step3_DIGI_pileup_11_DIGI.root \
  maxEvents=1 eventsToProcess=1:1:747 globalTag=auto:phase1_2025_realistic era=Run3_2025 geometry=DB:Extended \
  pdfFile=trigPrim_pileup_11_DIGI.pdf pngPrefix=trigPrim_pileup_11_DIGI savePNG=1 mixed=0 \
  sampleLabel="chapter 11_DIGI (11b): one pileup collision|trigger primitives made by the DIGI step|library event 747, no signal" \
  > trigPrim_pileup_11_DIGI.log 2>&1
cmsRun ../../common/trigPrimAnalyzer_cfg.py inputFiles=file:$J/test/11_DIGI/out/step3_DIGI_mixed_11_DIGI.root \
  maxEvents=1 eventsToProcess=1:1:1 globalTag=auto:phase1_2025_realistic era=Run3_2025 geometry=DB:Extended \
  pdfFile=trigPrim_mixed_11_DIGI.pdf pngPrefix=trigPrim_mixed_11_DIGI savePNG=1 mixed=1 \
  sampleLabel="chapter 11_DIGI (11a+11b): signal + pileup|trigger primitives made by the DIGI step|PU = 60 per bx, bx -12..3 (playback)" \
  > trigPrim_mixed_11_DIGI.log 2>&1
python3 ../../common/trigPrimLedger.py . 11_DIGI > trigprim_11_DIGI.txt
```

What the analyzer does. It reads `simEcalTriggerPrimitiveDigis` and `simHcalTriggerPrimitiveDigis` and decodes the
sample of interest of every primitive to GeV: for the ECAL `compressedEt() x 0.5 GeV`, for the HCAL with the
`CaloTPGTranscoder` of the event setup (the configuration loads `SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff`
for it, as the L1 job of chapter 12 does). It draws them on the 82 x 72 trigger-tower grid (ieta -41..41, iphi 1..72;
an HF primitive is drawn over the 2 or 4 towers in phi it covers) and, for every generator jet above `jetPtMin = 30`
GeV, the 17 x 17 towers around the tower of the jet axis with the ET written in each and the 9 x 9 window of a
Level-1 jet. It then compares the primitives with what the file knows otherwise:

- the transverse energy the particles left in the crystals: the `PCaloHit`s `g4SimHits:EcalHitsEB/EE` summed per
  trigger tower (`EcalTrigTowerConstituentsMap`, record `IdealGeometryRecord`; the reason for the `Geometry/CaloTopology`
  dependency in `plugins/BuildFile.xml`) as E / cosh(eta) of the crystal. With `mixed=1` the file holds only the hits of
  the signal (the pileup hits live in the `CrossingFrame`, which is not stored), so a primitive without a deposit
  behind it is pileup or noise;
- the time samples stored with every primitive (5 for the ECAL, 4 for HB / HE, 2 for the HF), summed per sample;
- the ECAL trigger-tower flag of every primitive (`ttFlag()`: 0 low, 1 mid, 3 high interest) against the
  selective-readout flags `simEcalDigis:ebSrFlags / eeSrFlags` (3 = full readout) that follow from it.

Options: `mixed` (above), `jetPtMin`, `zMax = 256` (the top of the logarithmic colour scale, GeV), `pdfFile`,
`pngPrefix`, `savePNG`, `sampleLabel`, `eventsToProcess`, `globalTag`, `era`, `geometry`. Products:

| file | content |
|---|---|
| `trigPrim_<case>_11_DIGI.pdf` | the pages: ECAL primitives, HCAL primitives, and two pages per generator jet (not for the pileup collision, which has no jet above 30 GeV) |
| `trigPrim_<case>_11_DIGI_ecaltp_run1_event<E>.png`, `..._hcaltp_...` | the tower maps (figure 11.7 of the chapter, left and middle columns: signal and mixed) |
| `trigPrim_<case>_11_DIGI_jet<N>_ecaltp_run1_event1.png`, `..._jet<N>_hcaltp_...` | the towers around generator jet N (1 = the b jet, pt 117.8 GeV; 2 = the bbar jet, in the HF); figure 11.7, right column: `trigPrim_signal_11_DIGI_jet1_*`; figure 11.8: `trigPrim_signal_11_DIGI_jet2_hcaltp` and `trigPrim_mixed_11_DIGI_jet2_hcaltp` |
| `trigPrim_<case>_11_DIGI.log` | every number: `format:`, `TP <region>:`, `ECAL TP time samples`, `HCAL TP time samples`, `ECAL TT flags`, `EB tower read in full:`, `ECAL SR flags`, `ECAL deposits <region>:`, `GenJet N:`, `top tower N:` |
| `trigprim_11_DIGI.txt` | the three logs side by side (`common/trigPrimLedger.py`): section 4.7 |

Note the numbering: these pages are numbered by *generator* jet (jet 1 = the b jet), the pages of chapter 12 by
*Level-1* jet (jet 1 = the bbar jet, the larger at Level-1).

## 4. The tables

### 4.1 `ledger_11_DIGI.txt` (the output of `common/digiLedger.py`, in full)

```
== digiLedger 11_DIGI
signal: run:lumi:event 1:1:1 vertex=(0.0276598,-0.0169037,3.11193) cm | bx 0: 0 pileup events
pileup: run:lumi:event 1:1:747 vertex=(0.0276303,-0.0177769,2.1553) cm | bx 0: 0 pileup events
mixed: run:lumi:event 1:1:1 vertex=(0.0276598,-0.0169037,3.11193) cm | bx -12: 71 pileup events | bx -11: 68 pileup events | bx -10: 54 pileup events | bx -9: 59 pileup events | bx -8: 52 pileup events | bx -7: 61 pileup events | bx -6: 76 pileup events | bx -5: 59 pileup events | bx -4: 65 pileup events | bx -3: 59 pileup events | bx -2: 68 pileup events | bx -1: 54 pileup events | bx 0: 64 pileup events | bx 1: 63 pileup events | bx 2: 62 pileup events | bx 3: 60 pileup events
subdet         (11a)      (11b)  (11a+11b) | mixed by origin: signal / in-time / early / late / no link ; calorimeters: sum amplitude [ADC] (11a) (11b) (11a+11b)
PXB             3011         38      44725 | 2826 / 40191 / 1689 / 0 / 19
PXF             1789         44      22048 | 1800 / 19731 / 517 / 0 / 0
TIB             5360       2266      79535 | 3038 / 52206 / 21242 / 1393 / 1656
TID             2558        818      30739 | 1727 / 20673 / 7070 / 753 / 516
TOB             6778       3984      93705 | 2733 / 56495 / 29601 / 1416 / 3460
TEC            16906       5327     169912 | 11526 / 110643 / 40882 / 3195 / 3666
EB               156         38        766 | amplitude 1260 421 7094
EE               384        170        623 | amplitude 1876 595 4245
ES               981        198      18287 | amplitude 17885 2072 197329
HB              1722       1672       2145 | amplitude 16684 15743 25272
HE              2249       1975       2824 | amplitude 20375 11497 49440
HO                 -          -          5 | amplitude 0 0 65
HF              2270       2145       2185 | amplitude 8234 3578 37928
DT                 -          -         24 | 0 / 0 / 23 / 1 / 0
CSCstrip           -          -       7216 | 0 / 41 / 1274 / 340 / 5561
CSCwire            -          -        263 | 0 / 0 / 0 / 0 / 263
RPC                8          8          7 | 0 / 0 / 0 / 0 / 7
GEM                -          -        101 | 0 / 0 / 80 / 21 / 0
total          44172      18683     475110 | 
```

### 4.2 The analyzer logs (the split of the digis in each sample, `digiAnalyzer_<case>_11_DIGI.log`)

(11a) `digiAnalyzer_signal_11_DIGI.log`:

```
== DigiAnalyzer run:lumi:event 1:1:1 vertex=(0.0276598,-0.0169037,3.11193) cm | bx 0: 0 pileup events
  PXB: nDigis=3011 signal=3011 inTime=0 early=0 late=0 noLink=0
  PXF: nDigis=1789 signal=1789 inTime=0 early=0 late=0 noLink=0
  TIB: nDigis=5360 signal=3120 inTime=0 early=0 late=0 noLink=2240
  TID: nDigis=2558 signal=1774 inTime=0 early=0 late=0 noLink=784
  TOB: nDigis=6778 signal=2798 inTime=0 early=0 late=0 noLink=3980
  TEC: nDigis=16906 signal=11726 inTime=0 early=0 late=0 noLink=5180
  EB: nDigis=156 sumAmplitude[ADC]=1260.33
  EE: nDigis=384 sumAmplitude[ADC]=1876.33
  ES: nDigis=981 sumAmplitude[ADC]=17885
  HB: nDigis=1722 sumAmplitude[ADC]=16684.4
  HE: nDigis=2249 sumAmplitude[ADC]=20374.8
  HF: nDigis=2270 sumAmplitude[ADC]=8234
  RPC: nDigis=8 signal=0 inTime=0 early=0 late=0 noLink=8
  total digis=44172 genJets(pt>30)=2
```

(11b) `digiAnalyzer_pileup_11_DIGI.log`:

```
== DigiAnalyzer run:lumi:event 1:1:747 vertex=(0.0276303,-0.0177769,2.1553) cm | bx 0: 0 pileup events
  PXB: nDigis=38 signal=36 inTime=0 early=0 late=0 noLink=2
  PXF: nDigis=44 signal=44 inTime=0 early=0 late=0 noLink=0
  TIB: nDigis=2266 signal=0 inTime=0 early=0 late=0 noLink=2266
  TID: nDigis=818 signal=53 inTime=0 early=0 late=0 noLink=765
  TOB: nDigis=3984 signal=34 inTime=0 early=0 late=0 noLink=3950
  TEC: nDigis=5327 signal=152 inTime=0 early=0 late=0 noLink=5175
  EB: nDigis=38 sumAmplitude[ADC]=420.667
  EE: nDigis=170 sumAmplitude[ADC]=595.333
  ES: nDigis=198 sumAmplitude[ADC]=2072
  HB: nDigis=1672 sumAmplitude[ADC]=15743.4
  HE: nDigis=1975 sumAmplitude[ADC]=11497
  HF: nDigis=2145 sumAmplitude[ADC]=3578
  RPC: nDigis=8 signal=0 inTime=0 early=0 late=0 noLink=8
  total digis=18683 genJets(pt>30)=0
```

("signal" in this log means: linked to a particle of library event 747, the only event of the sample.)

(11a+11b) `digiAnalyzer_mixed_11_DIGI.log`:

```
== DigiAnalyzer run:lumi:event 1:1:1 vertex=(0.0276598,-0.0169037,3.11193) cm | bx -12: 71 pileup events | bx -11: 68 pileup events | bx -10: 54 pileup events | bx -9: 59 pileup events | bx -8: 52 pileup events | bx -7: 61 pileup events | bx -6: 76 pileup events | bx -5: 59 pileup events | bx -4: 65 pileup events | bx -3: 59 pileup events | bx -2: 68 pileup events | bx -1: 54 pileup events | bx 0: 64 pileup events | bx 1: 63 pileup events | bx 2: 62 pileup events | bx 3: 60 pileup events
  PXB: nDigis=44725 signal=2826 inTime=40191 early=1689 late=0 noLink=19
  PXF: nDigis=22048 signal=1800 inTime=19731 early=517 late=0 noLink=0
  TIB: nDigis=79535 signal=3038 inTime=52206 early=21242 late=1393 noLink=1656
  TID: nDigis=30739 signal=1727 inTime=20673 early=7070 late=753 noLink=516
  TOB: nDigis=93705 signal=2733 inTime=56495 early=29601 late=1416 noLink=3460
  TEC: nDigis=169912 signal=11526 inTime=110643 early=40882 late=3195 noLink=3666
  EB: nDigis=766 sumAmplitude[ADC]=7094.33
  EE: nDigis=623 sumAmplitude[ADC]=4244.67
  ES: nDigis=18287 sumAmplitude[ADC]=197329
  HB: nDigis=2145 sumAmplitude[ADC]=25272.4
  HE: nDigis=2824 sumAmplitude[ADC]=49439.8
  HO: nDigis=5 sumAmplitude[ADC]=65
  HF: nDigis=2185 sumAmplitude[ADC]=37928
  DT: nDigis=24 signal=0 inTime=0 early=23 late=1 noLink=0
  CSCstrip: nDigis=7216 signal=0 inTime=41 early=1274 late=340 noLink=5561
  CSCwire: nDigis=263 signal=0 inTime=0 early=0 late=0 noLink=263
  RPC: nDigis=7 signal=0 inTime=0 early=0 late=0 noLink=7
  GEM: nDigis=101 signal=0 inTime=0 early=80 late=21 noLink=0
  total digis=475110 genJets(pt>30)=2
```

Sums quoted in the chapter (from these logs): (11a) tracker 36 402 digis = 24 218 linked + 12 184 noise (pixels
4800 linked + 0 noise for 1902 PSimHits, strips 19 418 linked + 12 184 noise for 8370 PSimHits); (11b) tracker 12 477
= 319 linked + 12 158 noise (pixels 80 linked + 2 noise), all noise 12 166 (12 158 + 8 RPC), 12 158 / 12 477 = 97 %;
(11a+11b) tracker 440 664 = 23 650 signal + 299 939 in-time + 101 001 early + 6757 late + 9317 noise (5.4 / 68.1 /
22.9 / 1.5 / 2.1 %); pixels 66 773 = 4626 + 59 922 + 2206 + 0 + 19; strips 373 891 = 19 024 + 240 017 + 98 795
(26.4 %) + 6757 + 9298; the signal lost 24 218 - 23 650 = 568 digis; 299 939 / 64 = 4687 in-time digis per collision,
23 650 / 4687 = 5.0; muon 7611 digis (24 + 7216 + 263 + 7 + 101), linked 41 in time, 1377 early, 362 late. Amplitude
ratios (11a+11b)/(11a): EB 7094/1260 = 5.6, EE 4245/1876 = 2.3, ES 197329/17885 = 11.0, HB 25272/16684 = 1.5,
HE 49440/20375 = 2.4, HF 37928/8234 = 4.6. Frames added by the pileup: EB 766 - 156 = 610, EE 623 - 384 = 239.

### 4.3 `thresholds_11_DIGI.txt`: the lines quoted in table 11.2 of the chapter

```
[mix.digitizers.pixel]        ThresholdInElectrons_BPix 1600.0  ThresholdInElectrons_BPix_L1 2000.0  ThresholdInElectrons_BPix_L2 1600.0
                              ThresholdInElectrons_FPix 1600.0  ThresholdSmearing_BPix 245.0 (L1 245, L2 245)  ThresholdSmearing_FPix 210.0
                              AddThresholdSmearing True  NoiseInElectrons 175.0  ReadoutNoiseInElec 350.0  AddNoise True  AddNoisyPixels True
                              ElectronPerAdc 135.0  AdcFullScale 255  TofLowerCut -12.5  TofUpperCut 12.5  AddPixelInefficiency True
                              DeadModules_DB True  killModules True  KillBadFEDChannels True  UseReweighting True  DoPixelAging False
[mix.digitizers.strip]        APVpeakmode False  TOFCutForDeconvolution 50.0  TOFCutForPeak 100.0  RealPedestals True  SingleStripNoise True
                              CommonModeNoise True  cmnRMStib 5.92  cmnRMStid 3.08  cmnRMStob 1.08  cmnRMStec 2.44  electronPerAdcDec 247.0
                              electronPerAdcPeak 262.0  ZeroSuppression True  FedAlgorithm 4  NoiseSigmaThreshold 2.0  Noise True
                              APVShapeDecoFile SimTracker/SiStripDigitizer/data/APVShapeDeco_320.txt  includeAPVSimulation False
[mix.digitizers.ecal]         simHitToPhotoelectronsBarrel 2250.0  simHitToPhotoelectronsEndcap 1800.0  binOfMaximum 6  doPhotostatistics True
                              doENoise True  doESNoise True  ConstantTerm 0.003  applyConstantTerm True  EBCorrNoiseMatrixG12 [1.0, 0.71073, ...]
[mix.digitizers.hcal]         hb.simHitToPhotoelectrons 2000.0  hb.samplingFactors [125.44, ...]  hb.readoutFrameSize 10  hb.binOfMaximum 6
                              hb.doSiPMSmearing True  hb.threshold_currentTDC 18.7  hb.timeSmearing True  he.samplingFactors [210.55, 197.93, 186.12, ...]
                              he.readoutFrameSize 10  he.binOfMaximum 6  he.threshold_currentTDC 18.7  hf1.simHitToPhotoelectrons 6.0
                              hf1.samplingFactor 0.37  hf1.readoutFrameSize 3  hf1.binOfMaximum 2  hf1.photoelectronsToAnalog 2.79
                              hf2.photoelectronsToAnalog 1.843  doNoise True  doEmpty True  doThermalNoise True  doIonFeedback True  doTimeSlew True
                              HBDarkening False  HEDarkening False  HFDarkening False  DelivLuminosity 0  killHE False
[mix (bunch crossings)]       bunchspace 25  minBunch -12  maxBunch 3  input.nbPileupEvents.averageNumber 60.0  input.type poisson
                              input.sequential True  playback True
[simEcalDigis]                EcalSelectiveReadoutProducer: configFromCondDB True  UseFullReadout False  trigPrimBypass False  defaultTtf 4
[simEcalPreshowerDigis]       ESZeroSuppressionProducer (threshold from the DB ESThresholds)
[simHcalDigis]                HcalRealisticZS: HBlevel 8  HElevel 9  HOlevel 24  HFlevel -9999  HBregion [5, 6]  HEregion [5, 6]  HOregion [1, 8]
                              HFregion [1, 2]  use1ts True  useConfigZSvalues 0
[simMuonDTDigis]              Smearing 2.4  deadTime 150.0  LinksTimeWindow 10.0  pset.TOFCorrection 3
[simMuonCSCDigis]             strips.pedestal 600.0  strips.pedestalSigma 1.5  strips.gain 2.0  strips.me11gain 4.0  strips.nScaBins 8
                              strips.doNoise True  strips.doCorrelatedNoise True  strips.doSuppression False  strips.peakTimeSigma 3.0
                              strips.bunchTimingOffsets [0.0, 37.53, 37.66, 55.4, ...]  wires.doNoise True
[simMuonRPCDigis]             Noise True  doBkgNoise True  digiModelConfig.averageEfficiency 0.95  digiModelConfig.timingRPCOffset 50.0
[simMuonGEMDigis]             averageEfficiency 0.98  averageNoiseRate 0.001  simulateIntrinsicNoise False  simulateBkgNoise False
                              simulateNoiseCLS True  minBunch -5  maxBunch 3
[simHcalTriggerPrimitiveDigis] FG_threshold 12  FG_HF_thresholds [17, 255]
```

The CSC strip integration window quoted in the chapter (`signalStartTime = -250`, `signalStopTime = 500` ns) and the
RPC `Gate = 25` ns are in `SimMuon/CSCDigitizer/python/muonCSCDigis_cfi.py` and `SimMuon/RPCDigitizer/python/muonRPCDigis_cfi.py`
of the release (`digiThresholds.py` does not match their names).

Two python values of the list above are not what the digitiser uses, and the chapter (table 11.2) says so:

- `strips.pedestal 600.0`, `pedestalSigma 1.5`, `gain 2.0` of `simMuonCSCDigis` are read only with
  `stripConditions = 'Configurable'`; the configuration has `stripConditions = cms.string('Database')`
  (`muonCSCDigis_cfi.py`, line 78; `SimMuon/CSCDigitizer/src/CSCDigiProducer.cc`, lines 33-36), so the pedestals, gains
  and noise come from the global tag: the strip samples of section 4.10 sit at 1016 to 1141 counts, not 600.
- `hb.simHitToPhotoelectrons 2000.0` (and `he.`) is overwritten: `HcalSimParameters::simHitToPhotoelectrons`
  (`SimCalorimetry/HcalSimAlgos/src/HcalSimParameters.cc`, lines 64-73) returns `samplingFactor / fCtoGeV /
  photoelectronsToAnalog`, the gain and the fC per photoelectron of the SiPM being DB values (the condition of line 68 is
  true for every HCAL subdetector). The HF parameters are `HFSimParameters` (`simHitToPhotoelectrons = 6`).
- `HBlevel 8`, `HElevel 9`, `HOlevel 24` of `simHcalDigis` are not used either: with `useConfigZSvalues = 0` the
  thresholds are set to -1 and taken per channel from `HcalZSThresholdsRcd` (`HcalZSAlgoRealistic.cc`, lines 27-44 and
  82-84). With `use1ts = True` and `HBregion = HEregion = [5, 6]` the loop of `keepMe` (lines 87-93) tests one time slice,
  index 5 (the sample of interest): a frame is kept if its QIE11 code there reaches the level of the channel.

### 4.4 The SimHits that went in (chapter 10 files)

Signal, `test/10_SIM/out/simHits_signal_10_SIM.log` and `simAnalyzer_signal_10_SIM.log`: 1926 SimTracks, 571
primaries; TRK 10 272 PSimHits (PXB 929, PXF 973, TIB 1182, TID 975, TOB 1022, TEC 5191; `sumELoss = 2.33358` GeV);
ECAL 24 262 PCaloHits in 12 804 cells (EB 5521, EE 6214, ES 1069; `E by subdetector: EB=67.989 EE=293.66 ES=0.245266`
GeV); HCAL 2531 PCaloHits in 1353 cells (HB 173, HE 648, HO 1, HF 531; `HB=0.217328 HE=1.66239 HO=3.73269e-06
HF=1849`, the HF in photoelectrons of the shower library); MUON 0.

Library event 747, `test/10_SIM/out/simHits_pileup_10_SIM.log`: 55 SimTracks, 22 primaries (FWLite: 55 SimTracks, of
which 22 primaries, one the intact beam proton of 6799.8 GeV handed over at z = -203 m); TRK 136 PSimHits (PXB 8,
PXF 19, TIB 0, TID 30, TOB 15, TEC 64); ECAL 173 PCaloHits in 131 cells (EB 60, EE 61, ES 10);
HCAL 90 PCaloHits in 72 cells (HB 1, HE 17, HF 54, as the SimHitAnalyzer
counts them; an FWLite decoding of the HCAL test numbering gives 1, 15, 54, the difference not followed up); MUON 0. The energies of event
747 are the FWLite ones of section 4.5.

The bunch crossing, `test/10_SIM/out/step3_MIX_10_SIM.log` (station TRK of the SimHitAnalyzer on the CrossingFrames):

```
  pileup mixed in (CrossingFrame, bunch crossings -12 .. 3): 995 events (early 746, in-time 64, late 185), 149894 primaries (114093, 8804, 26997), 463662 SimTracks; events per bx: -12:71 -11:68 -10:54 -9:59 -8:52 -7:61 -6:76 -5:59 -4:65 -3:59 -2:68 -1:54 0:64 1:63 2:62 3:60
  station TRK: 2106681 PSimHits, 0 PCaloHits in 0 cells (signal: 10272 PSimHits, 0 PCaloHits)
    hits by subdetector and layer/station: PXB/1=89824 PXB/2=62169 PXB/3=49310 PXB/4=41947 PXF/1=53296 PXF/2=58008 PXF/3=61808 TIB/1=135948 TIB/2=112600 TIB/3=47183 TIB/4=40499 TID/1=57198 TID/2=60106 TID/3=63394 TOB/1=111338 TOB/2=95803 TOB/3=39951 TOB/4=33408 TOB/5=29258 TOB/6=23397 TEC/1=107974 TEC/2=113398 TEC/3=119294 TEC/4=90905 TEC/5=94783 TEC/6=99128 TEC/7=73139 TEC/8=77488 TEC/9=64127
    hits by kind of the primary: charged hadron=4999 neutral hadron=890 photon=2013 e^{#pm}=138 no stored ancestor=2232 pileup, early (bx < 0)=1588361 pileup, in-time (bx 0)=125625 pileup, late (bx > 0)=382423
```

Per subdetector (the layers summed): PXB 243 250, PXF 173 112, TIB 336 230, TID 180 698, TOB 333 155, TEC 840 236,
in all 2 106 681 = 10 272 of the signal + 125 625 in time + 1 588 361 early + 382 423 late, exactly the sums of the
library events of each class (FWLite over `step2_SIM_pileup_10_SIM.root`, chapter 10 companion section 4.5).

### 4.5 FWLite extras

Read from the ROOT files with `python3` and `DataFormats.FWLite` (`Events`, `Handle`), after `cmsenv`. The scripts
are short enough to be retyped; they are the source of the energies of library event 747, of the zero-suppression
and truth counts, of the split of the early digis by bunch crossing and of the response of the ECAL filters to an
earlier pulse (the cell counts of the chapter are those of the SimHitAnalyzer of chapter 10, section 4.4).

SimHits of one event (`simHitCounts.py <file> <run:lumi:event>`: loops over the `g4SimHits` `TrackerHits*LowTof/HighTof`,
`Muon*Hits`, `EcalHits*` and `HcalHits` collections; HCAL cells are the distinct (subdetector, side, depth, ieta,
iphi) of the test numbering, `DataFormats/HcalDetId/src/HcalTestNumbering.cc`, which merges the layers differently
from the SimHitAnalyzer of chapter 10: 164 / 517 HB / HE cells for the signal against its 173 / 648):

```
== event 1:1:1 of step2_SIM_signal_10_SIM.root
  PXB  PSimHits=929 (low 915, high 14) modules=358 sumELoss=0.1894 GeV
  PXF  PSimHits=973 (low 958, high 15) modules=383 sumELoss=0.1209 GeV
  TIB  PSimHits=1182 (low 1152, high 30) modules=713 sumELoss=0.2986 GeV
  TID  PSimHits=975 (low 959, high 16) modules=454 sumELoss=0.1628 GeV
  TOB  PSimHits=1022 (low 984, high 38) modules=611 sumELoss=0.3844 GeV
  TEC  PSimHits=5191 (low 5095, high 96) modules=2802 sumELoss=1.1774 GeV
  tracker total PSimHits=10272
  MuonDTHits   PSimHits=0
  MuonCSCHits  PSimHits=0
  MuonRPCHits  PSimHits=0
  MuonGEMHits  PSimHits=0
  EcalHitsEB PCaloHits=7273 cells=5521 E=67.9890 GeV cells>0.5MeV=3386
  EcalHitsEE PCaloHits=15867 cells=6214 E=293.6595 GeV cells>0.5MeV=5270
  EcalHitsES PCaloHits=1122 cells=1069 E=0.2453 GeV cells>0.5MeV=102
  HB   PCaloHits=358 cells(det,z,depth,ieta,iphi)=164 E=0.2173 GeV
  HE   PCaloHits=1289 cells(det,z,depth,ieta,iphi)=517 E=1.6624 GeV
  HO   PCaloHits=2 cells(det,z,depth,ieta,iphi)=1 E=0.0000 GeV
  HF   PCaloHits=882 cells(det,z,depth,ieta,iphi)=531 E=1849.0000 units (photoelectrons of the shower library)
== event 1:1:747 of step2_SIM_pileup_10_SIM.root
  PXB  PSimHits=8 (low 8, high 0) modules=5 sumELoss=0.0016 GeV
  PXF  PSimHits=19 (low 19, high 0) modules=13 sumELoss=0.0022 GeV
  TIB  PSimHits=0 (low 0, high 0) modules=0 sumELoss=0.0000 GeV
  TID  PSimHits=30 (low 30, high 0) modules=28 sumELoss=0.0029 GeV
  TOB  PSimHits=15 (low 15, high 0) modules=5 sumELoss=0.0043 GeV
  TEC  PSimHits=64 (low 64, high 0) modules=53 sumELoss=0.0098 GeV
  tracker total PSimHits=136
  MuonDTHits   PSimHits=0
  MuonCSCHits  PSimHits=0
  MuonRPCHits  PSimHits=0
  MuonGEMHits  PSimHits=0
  EcalHitsEB PCaloHits=79 cells=60 E=0.2323 GeV cells>0.5MeV=34
  EcalHitsEE PCaloHits=84 cells=61 E=0.8605 GeV cells>0.5MeV=46
  EcalHitsES PCaloHits=10 cells=10 E=0.0014 GeV cells>0.5MeV=0
  HB   PCaloHits=5 cells(det,z,depth,ieta,iphi)=1 E=0.0006 GeV
  HE   PCaloHits=22 cells(det,z,depth,ieta,iphi)=15 E=0.0413 GeV
  HO   PCaloHits=0 cells(det,z,depth,ieta,iphi)=0 E=0.0000 GeV
  HF   PCaloHits=63 cells(det,z,depth,ieta,iphi)=54 E=98.0000 units (photoelectrons of the shower library)
```

The 22 primaries of library event 747 (`SimTrack`s with `genpartIndex() > 0`): 21 particles of the generator record,
351 GeV in all, and the forward proton of 6799.8 GeV that `LHCTransport` hands over at z = -203 m: a single-diffractive
collision, one beam proton intact.

The DIGI files (`test/11_DIGI/digiFileCounts_11_DIGI.py <out dir>`, run by the script into `out/counts_11_DIGI.txt`: `addPileupInfo`, the selective-readout flags
`simEcalDigis:ebSrFlags/eeSrFlags` by value, the `simHcalUnsuppressedDigis` against `simHcalDigis` sizes, the ECAL
frames written, the `mix:MergedTrackTruth` split by `EncodedEventId`; on the files of section 4.9 it gives the 2024
numbers there):

```
== signal run 1 event 1
  pileup per bx (bx:n): 0:0; sum 0
  EB SR flags (value: n of the 2448 trigger towers): [(1, 2444), (3, 4)]; EE SR flags (n of the 624 supercrystals): [(1, 608), (3, 16)]
  HCAL unsuppressed -> zero suppressed: HBHE(QIE11) 15840 -> 4459, HF(QIE10) 3456 -> 3456, HO 2160 -> 0
  ECAL frames written: EB 156, EE 384
  TrackingParticles (mix:MergedTrackTruth): 999 (signal 999, in-time pileup 0, out-of-time 0: early 0, late 0)
== pileup run 1 event 747
  pileup per bx (bx:n): 0:0; sum 0
  EB SR flags (value: n of the 2448 trigger towers): [(1, 2448)]; EE SR flags (n of the 624 supercrystals): [(1, 616), (3, 8)]
  HCAL unsuppressed -> zero suppressed: HBHE(QIE11) 15840 -> 4186, HF(QIE10) 3456 -> 3456, HO 2160 -> 0
  ECAL frames written: EB 38, EE 170
  TrackingParticles (mix:MergedTrackTruth): 21 (signal 21, in-time pileup 0, out-of-time 0: early 0, late 0)
== mixed run 1 event 1
  pileup per bx (bx:n): -12:71 -11:68 -10:54 -9:59 -8:52 -7:61 -6:76 -5:59 -4:65 -3:59 -2:68 -1:54 0:64 1:63 2:62 3:60; sum 995
  EB SR flags (value: n of the 2448 trigger towers): [(1, 2430), (3, 18)]; EE SR flags (n of the 624 supercrystals): [(1, 606), (3, 18)]
  HCAL unsuppressed -> zero suppressed: HBHE(QIE11) 15840 -> 7075, HF(QIE10) 3456 -> 3456, HO 2160 -> 11
  ECAL frames written: EB 766, EE 623
  TrackingParticles (mix:MergedTrackTruth): 211377 (signal 999, in-time pileup 12385, out-of-time 197993: early 160205, late 37788)
```

Flag value 1 is "low interest" (zero suppression applied), 3 "high interest" (full readout); the channel totals
(EB 61 200 crystals, EE 14 648, ES 137 216, HB 9072, HE 6768, HO 2160, HF 3456) are the detector elements listed by
the SimHitAnalyzer of chapter 10 (`simHits_signal_10_SIM.log`, first line) and the sizes of the unsuppressed HCAL
collections.

The mixed digis by bunch crossing (`linksByBx.py step3_DIGI_mixed_11_DIGI.root`: for every pixel and strip channel the
`PixelDigiSimLink` / `StripDigiSimLink` with the largest charge fraction, as the DigiAnalyzer attributes them, counted
by the bunch crossing of its `EncodedEventId`; for the CSC strips the distinct (strip, collision) pairs of
`simMuonCSCDigis:MuonCSCStripDigiSimLinks`, and the links of the four chambers of the old Level-1 muon):

```
pixel digis with a link, by bx of the largest fraction: -11:103 -9:21 -8:4 -4:200 -3:5 -2:125 -1:1748 0:59922 signal:4626
strip digis with a link, by bx of the largest fraction: -12:219 -11:224 -10:228 -9:126 -8:103 -7:79 -6:263 -5:114 -4:195 -3:797 -2:6290 -1:90157 0:240017 1:6671 2:86 signal:19024
CSC strip links (channel, collision) by bx: -12:67 -11:310 -10:36 -9:83 -8:88 -7:240 -6:119 -5:125 -4:128 -1:125 0:41 1:207 2:82 3:57
  chamber endcap 2 ME1/1 chamber 20: links by (bx, collision in bx): {(-11, 21): 31, (2, 41): 32}
  chamber endcap 2 ME2/1 chamber 10: links by (bx, collision in bx): {(-11, 21): 24, (-10, 52): 36, (1, 56): 6}
  chamber endcap 2 ME3/1 chamber 11: links by (bx, collision in bx): {(-11, 21): 32}
  chamber endcap 2 ME4/1 chamber 11: links by (bx, collision in bx): {(-11, 21): 32}
```

The totals agree with the DigiAnalyzer (pixels: 2206 early = 1748 + 125 + 200 + 103 + 21 + 4 + 5; strips: 98 795
early, 90 157 of them bx -1; 6757 late = 6671 + 86). Collision 21 of bunch crossing -11 is library event 71 + 21 = 92,
the collision of the kaon-decay muon that the Level-1 trigger found when it was in time (bx -1..1 version of the book,
`test/tmp/out_bx-1to1/12_L1T/out/truth_12_L1T.txt`: `MUON pdgId 13 ... bx 0, collision 21`).

The response of the ECAL filters to the pulse of an earlier or later bunch crossing (`ootResponse.py`, on the
payloads `EcalSimPulseShapePhaseI` (1 ns steps, `conddb dump`), `EcalTPGWeightIdMap_2018_mc` (two weight groups, 7-bit
two's-complement integers over 64: group 0 = 92 93 16 31 24, group 1 = 86 95 16 33 26) and `EcalSRSettings_2024_EBZS9.5_EEZS6.0_DCC2_mc`
(`dccNormalizedWeights`, `ecalDccZs1stSample = 2`); the filter is placed on the samples where it gives the largest
in-time response, a unit pulse of bunch crossing b is the shape read at `peak + (s - 6) x 25 - 25 b` ns in sample s):

```
== barrel-shape (peak at 135 ns): samples of an in-time pulse: 0.000 0.000 -0.000 0.015 0.774 1.000 0.882 0.668 0.470 0.316
  TP group 0 weights -0.562 -0.547 0.250 0.484 0.375 on samples 3-7: in-time 1.001; relative response by bx: -12:-0.027 -11:-0.040 -10:-0.058 -9:-0.085 -8:-0.124 -7:-0.183 -6:-0.275 -5:-0.395 -4:-0.511 -3:-0.532 -2:-0.262 -1:0.496 +0:1.000 +1:0.753 +2:0.297 +3:0.006; sum over bx -12..-1: -1.997
  TP group 1 weights -0.656 -0.516 0.250 0.516 0.406 on samples 3-7: in-time 1.060; relative response by bx: -12:-0.028 -11:-0.041 -10:-0.060 -9:-0.088 -8:-0.128 -7:-0.190 -6:-0.285 -5:-0.408 -4:-0.525 -3:-0.541 -2:-0.253 -1:0.535 +0:1.000 +1:0.763 +2:0.304 +3:0.006; sum over bx -12..-1: -2.012
  DCC ZS     weights -0.374 -0.374 -0.363 0.272 0.468 0.371 on samples 2-7: in-time 1.000; relative response by bx: -12:-0.038 -11:-0.055 -10:-0.081 -9:-0.118 -8:-0.174 -7:-0.259 -6:-0.376 -5:-0.510 -4:-0.591 -3:-0.465 -2:0.069 -1:0.646 +0:1.000 +1:0.737 +2:0.294 +3:0.006; sum over bx -12..-1: -1.953
== endcap-shape (peak at 148 ns): samples of an in-time pulse: 0.000 0.000 -0.000 0.116 0.747 0.987 0.885 0.677 0.484 0.339
  TP group 0 weights -0.562 -0.547 0.250 0.484 0.375 on samples 3-7: in-time 0.933; relative response by bx: -12:-0.036 -11:-0.051 -10:-0.073 -9:-0.103 -8:-0.145 -7:-0.202 -6:-0.288 -5:-0.406 -4:-0.527 -3:-0.544 -2:-0.246 -1:0.488 +0:1.000 +1:0.815 +2:0.360 +3:0.046; sum over bx -12..-1: -2.133
  TP group 1 weights -0.656 -0.516 0.250 0.516 0.406 on samples 3-7: in-time 0.995; relative response by bx: -12:-0.037 -11:-0.053 -10:-0.075 -9:-0.105 -8:-0.149 -7:-0.208 -6:-0.295 -5:-0.417 -4:-0.538 -3:-0.550 -2:-0.233 -1:0.519 +0:1.000 +1:0.819 +2:0.365 +3:0.047; sum over bx -12..-1: -2.140
  DCC ZS     weights -0.374 -0.374 -0.363 0.272 0.468 0.371 on samples 2-7: in-time 0.951; relative response by bx: -12:-0.049 -11:-0.069 -10:-0.097 -9:-0.138 -8:-0.193 -7:-0.273 -6:-0.385 -5:-0.516 -4:-0.594 -3:-0.455 -2:0.059 -1:0.651 +0:1.000 +1:0.785 +2:0.348 +3:0.045; sum over bx -12..-1: -2.060
```

For a pileup equal in every crossing (the average), the in-time amplitude of the primitive gains `1 - 1.997 + 1.056 =
0.059` of one crossing's worth (group 0; 0.084 for the zero suppression); with only bx -1..1 it gained
`0.496 + 1 + 0.753 = 2.25`; with the Phase-2 window bx -3..3 `-0.298 + 1 + 1.056 = 1.76` (bx -12..-4 sum to -1.698).
The weights sum to zero (`-36 - 35 + 16 + 31 + 24 = 0`; `-0.374 - 0.374 - 0.3629 + 0.2721 + 0.4681 + 0.3707 = 0`).
The HCAL primitives (HB, HE) subtract a fraction of the previous sample instead (`HcalTriggerPrimitiveAlgo.cc`, line
536: `(sampleTS << 8) - sampleTSminus1 * weight) / 256`), with the weights of `HcalTPChannelParameters_2024_v2.0_mc`
(`auxi1`): 120 for |ieta| 1-20, 110 for 21-28, the coded veto threshold `auxi2` = 88 everywhere.

### 4.6 Files of the release quoted in "Open problems for Phase-2"

The section describes the release of the book, CMSSW_15_0_5, and nothing else (not the development branch, not a later
release). All files are under `/cvmfs/cms.cern.ch/el9_amd64_gcc12/cms/cmssw/CMSSW_15_0_5/src` (`$CMSSW_RELEASE_BASE/src`
after `cmsenv`); the line numbers are those of this release. The effective values, after all the modifiers of the era,
were read from a process built in python (no cmsRun job):

```bash
cd $CMSSW_BASE/src && cmsenv
python3 -c "
import FWCore.ParameterSet.Config as cms
from Configuration.Eras.Era_Phase2C17I13M9_cff import Phase2C17I13M9
p = cms.Process('DIGI', Phase2C17I13M9)
for f in ['Configuration.StandardSequences.Services_cff', 'SimGeneral.MixingModule.mixNoPU_cfi', 'Configuration.StandardSequences.Digi_cff']: p.load(f)
print(p.mix.digitizers.dumpPython()); print(p.simMuonCSCDigis.dumpPython()); print(p.es_hardcode.dumpPython())
print(p.muonDigiTask.dumpPython(), p.ecalDigiTask.dumpPython(), list(p.es_prefers_()))"
```

(the same with `Configuration.Eras.Era_Run3_2025_cff.Run3_2025` gives the settings of this chapter for comparison; the
list of modifiers in the era comes from `Phase2C17I13M9._isOrContains(m)` for every `Configuration/Eras/python/Modifier_*_cff.py`).

| topic | file (line) : what is quoted |
|---|---|
| Phase-2 era | `Configuration/Geometry/python/defaultPhase2ConditionsEra_cff.py` (5): `DEFAULT_VERSION = "Run4D110"`; `Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py` (3171): `Run4D110` -> era `Phase2C17I13M9`, GT `auto:phase2_realistic_T33`; `Configuration/Eras/python/Era_Phase2C17I13M9_cff.py` (7, excludes `phase2_hgcalV12`, adds `phase2_hgcalV16`) -> `Era_Phase2C11I13M9_cff.py` (`phase2_GE0`) -> `Era_Phase2C11I13_cff.py` (`phase2_etlV4`) -> `Era_Phase2C11_cff.py` (`phase2_hgcalV12`) -> `Era_Phase2C9_cff.py` (`phase2_hgcalV10`, `V11`) -> `Era_Phase2_cff.py` (23-24: `Run3_noMkFit` without `phase1Pixel`, `trackingPhase1`, ..., plus the ten `phase2_*`, `trackingPhase2PU140`, `hcalHardcodeConditions`); not in the era: `phase2_ecal_devel`, `phase2_hfnose` |
| digitiser set | `SimGeneral/MixingModule/python/digitizers_cfi.py` (58 HGCal, 87 MTD, 114 `strip = None`), `pixelDigitizer_cfi.py` (30: `phase2_tracker.toReplaceWith(pixelDigitizer, ...phase2TrackerDigitizer)`) |
| Phase-2 tracker | `SimTracker/SiPhase2Digitizer/python/phase2TrackerDigitizer_cfi.py` (4-20 inner pixel; 43 `ApplyTimewalk = False`; 87 "will be True for realistic simulations"; 101 6300 e "(0.4 MIP = 0.4 * 16000 e)"; 108 200 e "30% of the readout noise (should be changed in future)"; 119/159/198 "to be confirmed with simulations in CMSSW_6.X"; 133 `BiasRailInefficiencyFlag = 1`; 141 4800 e "(0.4 MIP = 0.4 * 16000 e)"; 148/187 "4.75 sigma=>occupancy =1e-6"; 212 `HitDetectionMode = 0`; 214 `CBCDeadTime = 0.0`), `plugins/PSPDigitizerAlgorithm.cc` (47, 65: rail of 37.5 micron), `plugins/Phase2TrackerDigitizerAlgorithm.cc` (138-140: the charge reweighting), `python/customizeForOTInefficiency.py` (dead strips: `KillModules`, `DeadModules_DB`), `customizeForPhase2TrackerSignalShape.py` (`ApplyTimewalk = True`) |
| Run-3 pixel | `SimGeneral/MixingModule/python/SiPixelSimParameters_cfi.py` (38 `UseReweighting = True` in `_modifyPixelDigitizerForPhase1Pixel`, 122 `phase1Pixel.toModify`, 135-142 `premix_stage1`) |
| ECAL | `SimGeneral/MixingModule/python/ecalDigitizer_cfi.py` (37 `doEE`, 40 `doES`, 45 `phase2_ecal_devel`), `ecalDigitizer_Ph2_cfi.py`, `SimCalorimetry/EcalSimProducers/python/ecalDigiParameters_cff.py`, `ecalDigiParameters_Ph2_cff.py` (7 "fake numbers"), `ecalSimParameterMap_cff.py` (5 `binOfMaximum = 6`, 9 `2250.0`), `SimCalorimetry/Configuration/python/ecalDigiSequence_cff.py` (12, 21-27: TPs and selective readout kept under `phase2_common`), `DataFormats/EcalDigi/interface/EcalConstants.h` (19 two CATIA gains, 31 6.25 ns, 36 16 samples), `SimCalorimetry/EcalSimProducers/interface/EcalDigiProducer.h` (129-130 pedestals, MC intercalibrations) and `src/EcalDigiProducer.cc` (524 laser corrections), `SimCalorimetry/EcalSelectiveReadoutProducers/python/ecalDigis_cfi.py` (78 `UseFullReadout = True` for `premix_stage1`) |
| HCAL | `SimCalorimetry/HcalSimProducers/python/hcalUnsuppressedDigis_cfi.py` (24 `DelivLuminosity`, 27-29 darkening, 68-71 `killHE`, `doZDCDigi`), `hcalSimParameters_cfi.py` (177 `run3_HB`, 186 `run3_common`, 224 `phase2_hcal`), `Configuration/StandardSequences/python/Digi_cff.py` (79-87: `es_hardcode` + `es_prefer_hcalHardcode` under `hcalHardcodeConditions`), `CalibCalorimetry/HcalPlugins/python/Hcal_Conditions_forGlobalTag_cff.py` (18 `iLumi = -1`, 112 "average from 2017 HEP17", 114 `zsThreshold = 16`, 125 `doRadiationDamage = True`, 197 `ESPrefer`, 199-226 `toGet` with `ZSThresholds`, 234 `hcalHardcodeConditions.toModify`), `CalibCalorimetry/HcalAlgos/src/HcalHardcodeParameters.cc` (57 `intlumi > 0`), `CalibCalorimetry/HcalAlgos/src/HcalDbHardcode.cc` (218-221: one ZS value per subdetector), `SimCalorimetry/HcalZeroSuppressionProducers/python/hcalDigisRealistic_cfi.py` (24 `useConfigZSvalues = 0`: thresholds from the conditions; 54 `markAndPass` for `premix_stage1`), `Geometry/HcalSimData/python/HFParameters_cff.py` (44 `run3_HFSL`: Run-3 HF shower library) |
| HGCal | `SimCalorimetry/HGCalSimProducers/python/hgcalDigitizer_cfi.py` (9 `nonAgedCCEs`, 11 `nonAgedNoises_v9`, 25 `scaleByDose = False`, 42 "this is to be deprecated", 138 `nPEperMIP`, 147 `adcThreshold_fC = 0.5`, 194 `HGCal_setEndOfLifeNoise`, 208-217 `HGCal_setEndOfLifeNoise_4000` with `byDoseFactor=1.333`, 304 `doseParams_3000fb_fluka-3.7.20.txt`, 383 `phase2_hgcalV10`), `hgcROCParameters_cfi.py` (30 `adcThreshold_fC = 0.672`), `plugins/HGCDigitizerBase.cc` (176-178 "kept for legacy compatibility with the TDR simulation") |
| MTD | `SimFastTiming/FastTimingCommon/python/mtdDigitizer_cfi.py` (25 `TestBeamMIPTimeRes`, 31 `EnergyThreshold = 4`, 35 `DarkCountRate = 10`, 70 `IntegratedLuminosity = 1000`, 72 gain, 74 `applyDegradation = False`, 76 "from HGCAL", 106 `phase2_etlV4`), `src/ETLDeviceSim.cc` (105 the fluence at the fixed luminosity; 113 `applyDegradation` only for hits outside the pads) |
| muon | `SimMuon/DTDigitizer/python/muonDTDigis_cfi.py` (20 "number from M. Pegoraro", 37-39 `phase2Digis`), `SimMuon/CSCDigitizer/python/muonCSCDigis_cfi.py` (18 "Latest tuning by Vadim Khotilovich 16-Nov-2012", 48 `doNeutrons = False`, 86-90 `run2_common` offsets, the effective ones in Phase-2), `SimMuon/RPCDigitizer/python/muonRPCDigis_cfi.py` (28 Run-3 `doBkgNoise = True`, 63 Phase-2 `False`, 94 `phase2_muon`), `SimMuon/GEMDigitizer/python/muonGEMDigis_cfi.py` (13 `instLumi = 2.0`, 16 `instLumi = 5`), `SimMuon/GEMDigitizer/plugins/GEMDigiProducer.cc` (101 `simulateBkgNoise` false; 109 "5E+34 (PU 140)"), `SimMuon/Configuration/python/SimMuon_cff.py` (27 "while GE0 is in development", 37 `phase2_GE0`), `SimMuon/{DT,CSC,RPC,GEM}Digitizer/python/*ChamberMasker_cff.py` |
| ageing | `SLHCUpgradeSimulations/Configuration/python/aging.py` (84 "todo: determine ZS threshold adjustments", 99 seeds at 3000 fb-1, 193-199 the five ECAL tags, 202-207 the ECAL thresholds 0.103-0.707 GeV, 240-273 `customise_aging_300/1000/3000/3000_ultimate/4500_ultimate`: `agedHGCal` at 255, 263, 271; no tracker, MTD or muon entry), `Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py` (2089-2120 `Aging1000`, `Aging3000`) |
| mixing, premixing | `Configuration/PyReleaseValidation/python/relval_steps.py` (4616-4617 `'Run4'`: `AVE_200_BX_25ns`), `Configuration/StandardSequences/python/Mixing.py` (184 `AVE_200_BX_25ns` `B = (-3,3)`; 185 `_m12p3`, 186 `_m6p6`, used by no workflow of the release: `grep -r m12p3 Configuration SimGeneral`), `SimGeneral/MixingModule/python/mix_probFunction_25ns_PoissonOOTPU_cfi.py` (11-12 `maxBunch = 3`, `minBunch = -12`), imported by the 2024 and 2025 profiles `mix_2024_25ns_RunIII2024Summer24_PoissonOOTPU_cfi.py` and `Run3_2025_LHC_Simulation_fill12p5h_levelling9h_nb2340_xSec80mb_il2p23_cfi.py`, `upgradeWorkflowComponents.py` (2428 `PMXS1`, 2441 `PMXS2`) |
| bunch-crossing windows (section 3.1 of the chapter, the ECAL box and the PU-200 problem) | `SimCalorimetry/EcalSimAlgos/src/EcalHitResponse.cc` (28-29 `m_minBunch(-32)`, `m_maxBunch(10)`), `interface/EcalTDigitizer.icc` (21 `withinBunchRange(bunchCrossing)`), `SimCalorimetry/CaloSimAlgos/src/CaloHitResponse.cc` (33-34 `theMinBunch(-10)`, `theMaxBunch(10)`; no `setBunchRange` call in `SimCalorimetry/` or `SimGeneral/`), `interface/CaloTDigitizer.h` (76, the HCAL), `SimTracker/SiStripDigitizer/plugins/SiStripDigitizerAlgorithm.cc` (192-193: `std::fabs(tof - cosmicShift - r/30) < tofCut`, 58 `tofCut` = `TOFCutForDeconvolution`), `SimTracker/SiPixelDigitizer/plugins/SiPixelDigitizerAlgorithm.cc` (841-842 `TofLowerCut` / `TofUpperCut`), `SimMuon/GEMDigitizer/src/GEMSignalModel.cc` (48-80 `getSimHitBx`: every hit gets its own bx; `minBunch` / `maxBunch` are read only by `GEMNoiseModel.cc` and `GEMBkgModel.cc`, both off), `SimCalorimetry/HcalTrigPrimAlgos/src/HcalTriggerPrimitiveAlgo.cc` (480 PFA2 when the weight is 255, 536 PFA1'), `SimCalorimetry/HcalTrigPrimProducers/plugins/HcalTrigPrimDigiProducer.cc` (198-203 the weight and veto from `HcalTPChannelParameters`) |

### 4.7 The trigger primitives: `trigprim_11_DIGI.txt` (the output of `common/trigPrimLedger.py`, in full)

The numbers of the section "Made here for the next stage: the trigger primitives" of the chapter (table 11.3 is
section 1 of the file).

```
== trigPrimLedger 11_DIGI
(11a)      signal: run:lumi:event 1:1:1
(11b)      pileup: run:lumi:event 1:1:747
(11a+11b)  mixed: run:lumi:event 1:1:1
format: ECAL samples per primitive=5 sampleOfInterest=2 | HCAL (HB, HE) samples per primitive=4 presamples=2

-- 1. The trigger primitives: primitives with ET > 0 / sum ET [GeV] / largest ET [GeV]
                                            (11a) signal                  (11b) pileup               (11a+11b) mixed
ECAL, EB (of 2448)                     368 / 215.0 / 3.0             327 / 186.0 / 1.5             428 / 292.0 / 4.5
ECAL, EE (of 1584)                     131 / 113.5 / 6.0              116 / 82.5 / 3.0             167 / 136.5 / 7.0
ECAL, all (of 4032)                    499 / 328.5 / 6.0             443 / 268.5 / 3.0             595 / 428.5 / 7.0
HCAL, HB (of 2304)                       69 / 35.5 / 1.5               58 / 30.0 / 1.0            190 / 155.0 / 15.5
HCAL, HE (of 1728)                      12 / 24.0 / 10.0                 0 / 0.0 / 0.0            225 / 156.0 / 10.5
HCAL, HF (of 792)                      40 / 148.5 / 89.0                 2 / 1.0 / 0.5           705 / 1064.5 / 89.0
HCAL, all (of 4824)                   121 / 208.0 / 89.0               60 / 31.0 / 1.0          1120 / 1375.5 / 89.0
all primitives (of 8856)                     620 / 536.5                   503 / 299.5                 1715 / 1804.0

-- 2. One primitive per bunch crossing: sum ET [GeV] of the time samples stored with every primitive
      (25 ns per sample; [ ] is the sample of interest, the bunch crossing of the event)
(11a)      signal
   ECAL     bx -2..+2:    426.5    268.5 [   328.5]      274      280
   HB + HE  bx -2..+1:       35       26 [    59.5]     32.5
   HF       bx -1.. 0:                 0 [   148.5]
(11b)      pileup
   ECAL     bx -2..+2:    426.5    242.5 [   268.5]      275    247.5
   HB + HE  bx -2..+1:       31     33.5 [      30]       33
   HF       bx -1.. 0:                 0 [       1]
(11a+11b)  mixed
   ECAL     bx -2..+2:      616      356 [   428.5]    420.5      339
   HB + HE  bx -2..+1:    263.5    268.5 [     311]    328.5
   HF       bx -1.. 0:             804.5 [  1064.5]

-- 3. What the ECAL primitives do inside the DIGI step: the trigger-tower flag of every primitive (low / mid / high
      interest) and the selective-readout decision of the ECAL digitisation that follows from it
                                                       (11a)           (11b)       (11a+11b)
EB towers flagged mid interest                             4               0              10
EB towers flagged high interest                            0               0               1
EB towers read in full (of 2448)                           4               0              18
   with a high flag themselves                             0               0               1
   others (mid flag, or next to a high one)                4               0              17
EE towers flagged mid interest                             3               7               6
EE towers flagged high interest                            3               0               3
EE supercrystals read in full (of 624)                    16               8              18
(11a)      EB tower read in full: ieta=-17 iphi=46 ecalTP=3 ttFlag=1
(11a)      EB tower read in full: ieta=3 iphi=17 ecalTP=2 ttFlag=1
(11a)      EB tower read in full: ieta=11 iphi=27 ecalTP=3 ttFlag=1
(11a)      EB tower read in full: ieta=12 iphi=31 ecalTP=2 ttFlag=1
(11a+11b)  EB tower read in full: ieta=-4 iphi=15 ecalTP=2.5 ttFlag=1
(11a+11b)  EB tower read in full: ieta=-5 iphi=24 ecalTP=2 ttFlag=1
(11a+11b)  EB tower read in full: ieta=-9 iphi=42 ecalTP=2 ttFlag=1
(11a+11b)  EB tower read in full: ieta=-16 iphi=63 ecalTP=2 ttFlag=1
(11a+11b)  EB tower read in full: ieta=-17 iphi=46 ecalTP=2.5 ttFlag=1
(11a+11b)  EB tower read in full: ieta=4 iphi=39 ecalTP=0 ttFlag=0
(11a+11b)  EB tower read in full: ieta=4 iphi=40 ecalTP=0 ttFlag=0
(11a+11b)  EB tower read in full: ieta=4 iphi=41 ecalTP=1 ttFlag=0
(11a+11b)  EB tower read in full: ieta=5 iphi=39 ecalTP=0 ttFlag=0
(11a+11b)  EB tower read in full: ieta=5 iphi=40 ecalTP=4.5 ttFlag=3
(11a+11b)  EB tower read in full: ieta=5 iphi=41 ecalTP=0.5 ttFlag=0
(11a+11b)  EB tower read in full: ieta=5 iphi=42 ecalTP=3 ttFlag=1
(11a+11b)  EB tower read in full: ieta=6 iphi=39 ecalTP=0 ttFlag=0
(11a+11b)  EB tower read in full: ieta=6 iphi=40 ecalTP=0 ttFlag=0
(11a+11b)  EB tower read in full: ieta=6 iphi=41 ecalTP=2 ttFlag=1
(11a+11b)  EB tower read in full: ieta=8 iphi=64 ecalTP=2 ttFlag=1
(11a+11b)  EB tower read in full: ieta=11 iphi=27 ecalTP=2.5 ttFlag=1
(11a+11b)  EB tower read in full: ieta=11 iphi=36 ecalTP=2.5 ttFlag=1

-- 4. What an ECAL primitive measures: the primitives against the transverse energy that the particles of the sample
      left in the crystals of each tower (PCaloHits; in the mixed sample only the hits of the signal are in the file)
(11a)      signal
   ECAL deposits: 2799 towers, 120.7 GeV | towers with a deposit >= 0.25 GeV and a primitive: 32 (deposit 49.8 GeV, primitives 48.0 GeV)
        primitives without a deposit >= 0.25 GeV: 467 (280.5 GeV) | deposits >= 0.25 GeV without a primitive: 41 (18.3 GeV) | deposits below 0.25 GeV per tower: 52.6 GeV
   EB   deposits: 1573 towers, 43.7 GeV | towers with a deposit >= 0.25 GeV and a primitive: 14 (deposit 14.8 GeV, primitives 16.0 GeV)
        primitives without a deposit >= 0.25 GeV: 354 (199.0 GeV) | deposits >= 0.25 GeV without a primitive: 21 (9.0 GeV) | deposits below 0.25 GeV per tower: 19.9 GeV
   EE   deposits: 1226 towers, 77.0 GeV | towers with a deposit >= 0.25 GeV and a primitive: 18 (deposit 34.9 GeV, primitives 32.0 GeV)
        primitives without a deposit >= 0.25 GeV: 113 (81.5 GeV) | deposits >= 0.25 GeV without a primitive: 20 (9.3 GeV) | deposits below 0.25 GeV per tower: 32.7 GeV
(11b)      pileup
   ECAL deposits: 67 towers, 0.4 GeV | towers with a deposit >= 0.25 GeV and a primitive: 0 (deposit 0.0 GeV, primitives 0.0 GeV)
        primitives without a deposit >= 0.25 GeV: 443 (268.5 GeV) | deposits >= 0.25 GeV without a primitive: 0 (0.0 GeV) | deposits below 0.25 GeV per tower: 0.4 GeV
   EB   deposits: 31 towers, 0.2 GeV | towers with a deposit >= 0.25 GeV and a primitive: 0 (deposit 0.0 GeV, primitives 0.0 GeV)
        primitives without a deposit >= 0.25 GeV: 327 (186.0 GeV) | deposits >= 0.25 GeV without a primitive: 0 (0.0 GeV) | deposits below 0.25 GeV per tower: 0.2 GeV
   EE   deposits: 36 towers, 0.2 GeV | towers with a deposit >= 0.25 GeV and a primitive: 0 (deposit 0.0 GeV, primitives 0.0 GeV)
        primitives without a deposit >= 0.25 GeV: 116 (82.5 GeV) | deposits >= 0.25 GeV without a primitive: 0 (0.0 GeV) | deposits below 0.25 GeV per tower: 0.2 GeV
(11a+11b)  mixed
   ECAL deposits: 2799 towers, 120.7 GeV | towers with a deposit >= 0.25 GeV and a primitive: 34 (deposit 51.8 GeV, primitives 49.0 GeV)
        primitives without a deposit >= 0.25 GeV: 561 (379.5 GeV) | deposits >= 0.25 GeV without a primitive: 39 (16.3 GeV) | deposits below 0.25 GeV per tower: 52.6 GeV
   EB   deposits: 1573 towers, 43.7 GeV | towers with a deposit >= 0.25 GeV and a primitive: 15 (deposit 16.0 GeV, primitives 15.5 GeV)
        primitives without a deposit >= 0.25 GeV: 413 (276.5 GeV) | deposits >= 0.25 GeV without a primitive: 20 (7.9 GeV) | deposits below 0.25 GeV per tower: 19.9 GeV
   EE   deposits: 1226 towers, 77.0 GeV | towers with a deposit >= 0.25 GeV and a primitive: 19 (deposit 35.8 GeV, primitives 33.5 GeV)
        primitives without a deposit >= 0.25 GeV: 148 (103.0 GeV) | deposits >= 0.25 GeV without a primitive: 19 (8.5 GeV) | deposits below 0.25 GeV per tower: 32.7 GeV

-- 5. The two generator jets in the primitives: the 9 x 9 towers around the jet axis, sum ET [GeV]
(11a)      GenJet 1: pt=117.788 eta=-1.58908 phi=-2.08403 towerIeta=-19 towerIphi=49 | 9x9 towers around it: ecalTP=29 (12 towers) hcalTP=22.5 (9 primitives) ecalDepositEt=41.6009 | hcal samples of the window (HB/HE: index 2 = sample of interest; HF: index 1): 0 0 22.5 0
(11a)      GenJet 2: pt=115.944 eta=-3.74635 phi=1.00816 towerIeta=-34 towerIphi=12 | 9x9 towers around it: ecalTP=0 (0 towers) hcalTP=123.75 (11 primitives) ecalDepositEt=0 | hcal samples of the window (HB/HE: index 2 = sample of interest; HF: index 1): 0 123.75 0 0
(11a+11b)  GenJet 1: pt=117.788 eta=-1.58908 phi=-2.08403 towerIeta=-19 towerIphi=49 | 9x9 towers around it: ecalTP=33.5 (18 towers) hcalTP=26.5 (13 primitives) ecalDepositEt=41.6009 | hcal samples of the window (HB/HE: index 2 = sample of interest; HF: index 1): 1 2 26.5 0
(11a+11b)  GenJet 2: pt=115.944 eta=-3.74635 phi=1.00816 towerIeta=-34 towerIphi=12 | 9x9 towers around it: ecalTP=0 (0 towers) hcalTP=178.75 (41 primitives) ecalDepositEt=0 | hcal samples of the window (HB/HE: index 2 = sample of interest; HF: index 1): 45.25 178.75 0 0

-- 6. The largest primitives (ECAL + HCAL of the tower)
(11a)      top tower 0: ieta=-34 iphi=11 ecalTP=0 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=89 hcalSamples=0,89,0,0
(11a)      top tower 1: ieta=-18 iphi=50 ecalTP=3 ttFlag=1 fineGrain=0 ecalDepositEt=3.44173 hcalTP=10 hcalSamples=0,0,10,0
(11a)      top tower 2: ieta=-33 iphi=11 ecalTP=0 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=12.5 hcalSamples=0,12.5,0,0
(11a)      top tower 3: ieta=-19 iphi=48 ecalTP=6 ttFlag=3 fineGrain=0 ecalDepositEt=6.57903 hcalTP=3.5 hcalSamples=0,0,3.5,0
(11a)      top tower 4: ieta=-33 iphi=13 ecalTP=0 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=9.5 hcalSamples=0,9.5,0,0
(11a)      top tower 5: ieta=-34 iphi=13 ecalTP=0 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=7 hcalSamples=0,7,0,0
(11b)      top tower 0: ieta=26 iphi=3 ecalTP=3 ttFlag=1 fineGrain=0 ecalDepositEt=0.00200589 hcalTP=0 hcalSamples=0,0,0,0
(11b)      top tower 1: ieta=-25 iphi=34 ecalTP=2 ttFlag=1 fineGrain=0 ecalDepositEt=0 hcalTP=0 hcalSamples=0,0,0,0
(11b)      top tower 2: ieta=26 iphi=52 ecalTP=2 ttFlag=1 fineGrain=0 ecalDepositEt=0 hcalTP=0 hcalSamples=0,0,0,0
(11b)      top tower 3: ieta=-28 iphi=70 ecalTP=2 ttFlag=1 fineGrain=0 ecalDepositEt=0 hcalTP=0 hcalSamples=0,0,0,0
(11b)      top tower 4: ieta=-28 iphi=69 ecalTP=2 ttFlag=1 fineGrain=0 ecalDepositEt=0 hcalTP=0 hcalSamples=0,0,0,0
(11b)      top tower 5: ieta=28 iphi=9 ecalTP=2 ttFlag=1 fineGrain=0 ecalDepositEt=0 hcalTP=0 hcalSamples=0,0,0,0
(11a+11b)  top tower 0: ieta=-34 iphi=11 ecalTP=0 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=89 hcalSamples=1,89,0,0
(11a+11b)  top tower 1: ieta=8 iphi=68 ecalTP=0 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=15.5 hcalSamples=0,0,15.5,1
(11a+11b)  top tower 2: ieta=-18 iphi=50 ecalTP=3.5 ttFlag=1 fineGrain=0 ecalDepositEt=3.44173 hcalTP=10.5 hcalSamples=0,0,10.5,0
(11a+11b)  top tower 3: ieta=-33 iphi=11 ecalTP=0 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=13 hcalSamples=0,13,0,0
(11a+11b)  top tower 4: ieta=-33 iphi=13 ecalTP=0 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=11 hcalSamples=0.5,11,0,0
(11a+11b)  top tower 5: ieta=-19 iphi=48 ecalTP=7 ttFlag=3 fineGrain=0 ecalDepositEt=6.57903 hcalTP=4 hcalSamples=0,0,4,0
```

How the chapter reads it.

- Section 1 is table 11.3. The mixed column is also the first table of chapter 12 (its step 1).
- Section 2, "one primitive per bunch crossing": the ECAL sample sums of the signal alone are 426.5, 268.5, [328.5],
  274.0, 280.0 GeV: about 270 GeV of noise per crossing (the first stored sample is higher, as in 2024) and some 55 GeV
  more in ours. With the pileup 616.0, 356.0, [428.5], 420.5, 339.0: ours is 100 GeV above the signal alone (with bx
  -1..1 it was 1047.5, 719 GeV above), the cancellation of the sum-to-zero weights of section 4.5.
- Section 3: the thresholds of the flags are 2 GeV (mid) and 4 GeV (high), from the payload of `EcalTPGPhysicsConstRcd`,
  `EcalTPGPhysicsConst_2025_mc_EB_4.0_2.0_EE_4.0_2.0` (`conddb list 150X_mcRun3_2025_realistic_v1 | grep EcalTPG`
  lists the 19 `EcalTPG*Rcd` payloads; `conddb dump` of this one gives `ttf-threshold-Low 2`, `ttf-threshold-High 4`,
  the same content as the `2018_mc` payload of the 2024 global tag). Mixed barrel: 10 mid + 1 high towers; a mid tower
  is read in full alone, a high one with its 8 neighbours, of which one, (6, 41), is itself a mid tower: 10 + 9 - 1 = 18
  towers, the 18 `EB tower read in full` lines (the seven with `ttFlag=0` are neighbours of (5, 40)). Endcap: 6 mid +
  3 high towers, 18 supercrystals. The signal alone: 4 mid barrel towers, read in full, and in the endcap 3 mid + 3
  high towers, 16 supercrystals read in full. Library event 747: no barrel flag, 7 mid endcap towers, 8 supercrystals.
- Section 4, signal alone: 120.7 GeV of ET deposited in 2799 ECAL towers; 32 towers with at least 0.25 GeV and a
  primitive (deposit 49.8 GeV, primitives 48.0 GeV); 41 towers with at least 0.25 GeV (18.3 GeV) and no primitive;
  52.6 GeV in towers below 0.25 GeV; 467 of the 499 primitives (280.5 GeV) without such a deposit. 49.8 + 18.3 + 52.6 =
  120.7. Library event 747: 0.4 GeV deposited in 67 towers, none with 0.25 GeV, 443 primitives (268.5 GeV): 4032 / 443
  = one tower in 9.1 (2024, the same collision, section 4.9: 274 primitives, one tower in 4032 / 274 = 14.7, "fifteen").
- Section 5, the 9 x 9 towers around the jet axes: the b jet 29.0 (ECAL) + 22.5 (HCAL) = 51.5 GeV alone, 33.5 + 26.5 =
  60.0 GeV with the pileup (+8.5); the bbar jet 123.75 and 178.75 GeV (+55.0; the quarter GeV: an HF primitive is two
  towers wide in phi, and one that lies half inside the window is counted half, `TrigPrimAnalyzer.cc`). Chapter 12 centres its window on
  the seed tower of the Level-1 jet, (-18, 50) instead of the jet-axis tower (-19, 49), and finds 32.5 + 23.0 = 55.5
  GeV for the b jet: the two windows differ by one tower in each direction.
- Section 6: the largest tower of the mixed event after the HF one is the HB tower (8, 68) with 15.5 GeV, pileup, far
  from both signal jets (it is not in the top six of the signal alone; chapter 12 makes a Level-1 jet of 39 GeV of it).

The parameters of the two producers (`thresholds_11_DIGI.txt`, section 2; the HCAL one abbreviated to the lines the
chapter uses):

```
[simEcalTriggerPrimitiveDigis] EcalTrigPrimProducer
   BarrelOnly                                                   False
   Debug                                                        False
   Famos                                                        False
   InstanceEB
   InstanceEE
   Label                                                        simEcalUnsuppressedDigis
   TcpOutput                                                    False
   binOfMaximum                                                 6
[simHcalTriggerPrimitiveDigis] HcalTrigPrimDigiProducer
   FG_HF_thresholds                                             [17, 255]
   FG_threshold                                                 12
   PeakFinderAlgorithm                                          2
   RunZS                                                        False
   ZS_threshold                                                 1
   applySaturationFix                                           True
   inputUpgradeLabel                                            [cms.InputTag("simHcalUnsuppressedDigis","HBHEQIE11DigiCollection"), cms.InputTag("simHcalUnsuppressedDigis","HFQIE10DigiCollection")]
   latency                                                      1
   numberOfPresamples                                           2
   numberOfPresamplesHF                                         1
   numberOfSamples                                              4
   numberOfSamplesHF                                            2
   tpScales.HBHE.LSBQIE11                                       0.0625
   upgradeHB                                                    True
   upgradeHE                                                    True
   upgradeHF                                                    True
   useTDCInMinBiasBits                                          False
```

`Label = simEcalUnsuppressedDigis`: the ECAL primitives are made from the unsuppressed frames, which are not stored.
`binOfMaximum = 6` is the sample where the digitiser puts the maximum of an in-time pulse. `tpScales.HBHE.LSBQIE11 =
0.0625`: 1/16 GeV per count of the linearised HCAL sum, before the compression to 0.5 GeV per count.

### 4.8 Files of the release quoted in the hand-over section

All under `/cvmfs/cms.cern.ch/el9_amd64_gcc12/cms/cmssw/CMSSW_15_0_5/src`:

| statement | file, line |
|---|---|
| the ECAL primitive is made before, and for, the selective readout | `SimCalorimetry/Configuration/python/ecalDigiSequence_cff.py`: 5 (`# ECAL Trigger Primitives (needed by SRP)`), 12 (`ecalDigiTask = cms.Task(simEcalTriggerPrimitiveDigis, simEcalDigis, simEcalPreshowerDigis)`) |
| the selective readout reads the primitives | `SimCalorimetry/EcalSelectiveReadoutProducers/python/ecalDigis_cfi.py`: 35 (`trigPrimProducer = cms.string('simEcalTriggerPrimitiveDigis')`) |
| the ECAL primitive producer and its input | `SimCalorimetry/EcalTrigPrimProducers/python/ecalTriggerPrimitiveDigis_cfi.py`: 10 (`binOfMaximum = 6`), 14 (`Label = 'simEcalUnsuppressedDigis'`) |
| the algorithm, named after the FENIX chips | `SimCalorimetry/EcalTrigPrimAlgos/interface/EcalFenixLinearizer.h`, `EcalFenixEtStrip.h`, `EcalFenixAmplitudeFilter.h`, `EcalFenixPeakFinder.h`, `EcalFenixStrip.h` (the strip), `EcalFenixEtTot.h`, `EcalFenixTcp.h`, `EcalFenixTcpFormatEB.h` / `EE.h`, `EcalFenixFgvbEB.h` (the tower), `EcalTrigPrimFunctionalAlgo.h` |
| what is stored: the primitives and the unsuppressed HCAL frames, not the unsuppressed EB / EE frames | `SimCalorimetry/Configuration/python/SimCalorimetry_EventContent_cff.py`: 8, 14 (`keep *_sim{Ecal,Hcal}TriggerPrimitiveDigis_*_*`), 45 (`run2_common`: `keep *_simHcalUnsuppressedDigis_*_*`), 36 (only the `ESDigiCollection` of `simEcalUnsuppressedDigis`), 51 (`EBDigiCollection_simEcalUnsuppressedDigis` only under `run3_ecal_devel`) |
| the primitives are packed into the raw data | `EventFilter/EcalDigiToRaw/python/ecalDigiToRaw_cfi.py`: 12 (`labelTT = cms.InputTag("simEcalTriggerPrimitiveDigis")`) |
| the HCAL primitive producer | `SimCalorimetry/HcalTrigPrimProducers/python/hcaltpdigi_cfi.py`: 124 (`PeakFinderAlgorithm = 2`), 133-137 (`numberOfSamplesHF = 2`, `numberOfPresamplesHF = 1` under `run2_HF_2017`); the algorithm is `SimCalorimetry/HcalTrigPrimAlgos/src/HcalTriggerPrimitiveAlgo.cc` |
| the muon trigger primitives are made by the L1 step instead | `L1Trigger/Configuration/python/SimL1Emulator_cff.py` (`simDtTriggerPrimitiveDigis`, `simCscTriggerPrimitiveDigis`): chapter 12 |

### 4.9 The 2024 and the 2025 settings

The chapter runs with `YEAR=2025` (`common/jitjet_common.sh`, Rule I of the Constitution): `auto:phase1_2025_realistic`
= `150X_mcRun3_2025_realistic_v1` and the era `Run3_2025`, for the three DIGI jobs and the two analyzers
(`digiAnalyzer_cfg.py` and `trigPrimAnalyzer_cfg.py` default to them; the script passes them). The first version of
the chapter (11 November 2026) ran with `auto:phase1_2024_realistic` = `150X_mcRun3_2024_realistic_v2` and `Run3_2024`.
Since the move to the sixteen bunch crossings (21 November 2026) the comparison is made with -12..3 in both years: the
2024 mixed DIGI file is the one `test/12_L1T/run_12_L1T_2024.sh` makes (`test/12_L1T/out2024/step3_DIGI_mixed_11_DIGI.root`,
the mixed configuration of this chapter with the 2024 settings and the same playback); `test/11_DIGI/run_11_DIGI_2024.sh`
digitises the signal and library event 747 with the same `cmsDriver.py` commands as in section 1 and
`--conditions auto:phase1_2024_realistic --era Run3_2024` (configurations `step3_DIGI_{signal,pileup}_2024_11_DIGI_cfg.py`),
links the mixed file, and analyses the three into `test/11_DIGI/out2024/` with the commands of section 3
(`globalTag=auto:phase1_2024_realistic era=Run3_2024`), `digiLedger.py`, `trigPrimLedger.py` and
`11_DIGI/digiFileCounts_11_DIGI.py` (section 4.5): `out2024/ledger_11_DIGI.txt`, `trigprim_11_DIGI.txt`,
`counts_11_DIGI.txt`. The signal-alone
numbers are those of 11 November 2026, digi for digi (4982 + 7 pixel, 19 513 + 12 256 strip digis, 43 762 in all).

What did not change.

| compared | result |
|---|---|
| the configuration | `process.dumpPython()` of `step3_DIGI_mixed_11_DIGI_cfg.py` with each era: 19 114 lines, identical except the name of the global tag. None of the five modifiers that `Run3_2025` adds to `Run3_2024` (`run3_GEM_2025`, `run3_CSC_2025`, `run3_SiPixel_2025`, `stage2L1Trigger_2025`, `ecal_cctiming`) is used by a module of the DIGI step |
| `thresholds_11_DIGI.txt` | identical (it lists the python; the database thresholds are not in it) |
| the input | the SimHits of chapter 10 are identical with both settings (chapter 10 companion, section 4.7): section 4.4 is unchanged |
| the mixing | `addPileupInfo` 71 68 54 59 52 61 76 59 65 59 68 54 / 64 / 63 62 60, the same library events (playback of the same `CrossingFramePlaybackInfoNew`) |
| the truth | `mix:MergedTrackTruth` 999 / 21 / 211 377 (in-time 12 385, out-of-time 197 993) |
| the muon digis | identical counts and split in all three samples (mixed: DT 24, CSC 7216 strips + 263 wires, RPC 7, GEM 101); the ES frames of (11a) and (11b) |
| the ECAL deposits behind the primitives | `trigprim_11_DIGI.txt` section 4: 120.7 GeV in 2799 towers (signal), 0.4 GeV in 67 towers (event 747) |

What changed: the conditions. `conddb diff 150X_mcRun3_2024_realistic_v2 150X_mcRun3_2025_realistic_v1` lists 35
records with another tag. Those the DIGI step reads, with what `conddb dump` of the two payloads gives (averages over
the channels; the eta of an EE crystal from its ix, iy at z = 317 cm, approximate):

| record | 2024 tag | 2025 tag | read by | the two payloads |
|---|---|---|---|---|
| `EcalPedestalsRcd` | `EcalPedestals_2024_corrected_mc` | `EcalPedestals_2025_at80fb_corrected_mc` | the ECAL noise (`EcalCoder.cc`, lines 99-102: the width of a sample is the pedestal RMS) | RMS at gain 12: EB 2.85 -> 3.31 ADC, EE 1.96 -> 1.98 ADC; mean 200 ADC in both |
| `EcalLaserAPDPNRatiosRcd` | `..._2024_outliers_cleaned_3sigma_mc` | `..._TL470fb_moy_2025_outliers_cleaned_3sigma_mc` | the ECAL light yield: `UseLCcorrection = True`, the photoelectrons of a hit are divided by the laser correction of the crystal (`EcalHitResponse.cc`, lines 181-191) | APD/PN ratio `p2`: EB 0.827 -> 0.755; EE 1.5-2.0: 0.626 -> 0.544, 2.0-2.5: 0.290 -> 0.218, 2.5-3.0: 0.041 -> 0.013 (the references and alphas are the same tags in both) |
| `EcalSRSettingsRcd` | `EcalSRSettings_2023_EBZS6.5_EEZS6.0_DCC2_mc` | `EcalSRSettings_2024_EBZS9.5_EEZS6.0_DCC2_mc` | the selective readout (`configFromCondDB = True`) | `srpLowInterestChannelZS` EB 0.2275 -> 0.3325 GeV, EE 0.36 GeV in both; nothing else differs |
| nine `EcalTPG*Rcd` | `..._2024_mc`, `..._2022_mc`, `..._2018_mc` | `..._2025_mc`, `..._TL470fb_moy_2025_mc` | the ECAL trigger primitives | linearisation `mult-x12`: EB 187 -> 189.5, EE 181 -> 182 on average, 179-185 -> 181-183 per EE eta range: the primitives are not corrected for the darker crystals; `EcalTPGPhysicsConst`: the same content (flags at 2 and 4 GeV) under a new name |
| `HcalSiPMParametersRcd` | `HcalSiPMParameters_2024_50fb_v1.0_mc` | `HcalSiPMParameters_2025_MOY_mc` | the SiPM dark current added as photoelectrons (`HcalSiPMHitResponse.cc`, line 155) | dark current HB 7.7 -> 14.9 uA, HE 0.94 -> 2.69 uA |
| `HcalZSThresholdsRcd` | `HcalZSThresholds_2024_50fb_v1.0_mc` | `HcalZSThresholds_2025_MOY_mc` | the zero suppression of `simHcalDigis` (`useConfigZSvalues = 0`) | levels of the used channels HB 19.0 -> 25.4, HE 6.9 -> 9.0 ADC; HO 24 in both, HF none |
| `HcalRespCorrsRcd`, `HcalTPChannelParametersRcd` | `2024_v1.0`, `2022_v1.1` | `2025_MOY_v3`, `2024_v2.0` | the HCAL trigger primitives: the response correction enters the linearisation table (`HcaluLUTTPGCoder.cc`, line 450, gain x response correction), the channel parameters the `HcalTriggerPrimitiveAlgo` | not quantified |
| `HcalPedestalsRcd`, `HcalPedestalWidthsRcd` (label `effective`) | `2024_50fb` | `2025_MOY` | not the digitiser noise: `HcalAmplifier.cc` uses the plain pedestals and widths, `HcalPedestals(Widths)_2021_v1.0_mc` in both tags | effective widths HB 94 -> 143 fC, HE 33 -> 54 fC |
| `SiPixelLorentzAngleSimRcd` | `SiPixelLorentzAngleSim_phase1_38T_2022_v7_fix_mc` | `SiPixelLorentzAngleSim_phase1_EoR3_HV800_Tr2000` | the drift of the pixel charge in the field (`LorentzAngle_DB = True`) | tan(Lorentz angle) per tesla: BPix L1 0.0707 -> 0.0358, L2 0.0709 -> 0.0542, L3 / L4 0.0839 -> 0.0709, FPix 0.0770 -> 0.0639 |
| `SiPixel2DTemplateDBObjectRcd` (`numerator`, `denominator`) | `..._phase1_38T_2022_v7_fix(_denominator)_mc` | `..._phase1_EoR3_HV800_Tr2000_bugfix4_num / _bugfix_den` | the radiation-damage reweighting of the pixel charge (`UseReweighting = True`, `SiPixelChargeReweightingAlgorithm.h`, lines 155-156) | not quantified |
| `SiStripBadChannelRcd` | `SiStripBadComponents_realisticMC_for2024_v4_mc` | `SiStripBadComponents_realisticMC_for2025_v1_mc` | the strips killed by the digitiser | 1570 -> 1605 modules with bad components |

The other changed records (beam spots, alignment, pixel reconstruction templates and Lorentz angle, `HcalPFCuts`,
`EcalChannelStatus`, the L1 menu, the jet corrections, `DeDxCalibration`) have no consumer in the DIGI step (for
`EcalChannelStatusRcd`: no file of `SimCalorimetry/` reads it).

The numbers, 2024 -> 2025 ((11a) / (11b) / (11a+11b); bx -12..3 in both years):

| quantity | 2024 | 2025 | why |
|---|---|---|---|
| pixel digis linked (11a), for 1902 PSimHits | 4982 | 4800 | smaller Lorentz angle: less charge sharing between pixels (the reweighting templates changed too; the share of each not established) |
| pixel noise digis (11a) | 7 | 0 | the same noise configuration; a different random draw (below) |
| strip digis linked / noise (11a) | 19 513 / 12 256 | 19 418 / 12 184 | bad components; random noise |
| tracker digis (11a) / (11b) / (11a+11b) | 36 758 / 12 768 / 446 563 | 36 402 / 12 477 / 440 664 | |
| mixed tracker: signal / in-time / early / late / noise | 23 929 / 304 495 / 101 897 / 6756 / 9486 | 23 650 / 299 939 / 101 001 / 6757 / 9317 | |
| EB frames | 399 / 286 / 1489 | 156 / 38 / 766 | barrel zero suppression 0.2275 -> 0.3325 GeV (+46 %), against crystal noise +16 % and an APD/PN ratio -9 % (less light) |
| EE frames | 372 / 53 / 1005 | 384 / 170 / 623 | EE zero suppression unchanged; the mixed event has 18 instead of 30 supercrystals read in full |
| EB towers / EE supercrystals read in full | 2 / 0 / 13 and 15 / 1 / 30 | 4 / 0 / 18 and 16 / 8 / 18 | follow the primitives (flags 2 and 4 GeV) |
| ES frames | 981 / 198 / 18 188 | 981 / 198 / 18 287 | ES payloads unchanged; noise draw |
| HB / HE frames kept by the zero suppression (HBHE) | 3285 / 2959 / 6501 | 4459 / 4186 / 7075 | SiPM dark current x1.9 (HB) and x2.9 (HE), zero-suppression levels x1.3 |
| HF frames with a non-negative amplitude | 2251 / 2171 / 2230 | 2270 / 2145 / 2185 | HF payloads unchanged (pedestal 14.9 fC, width 2.97 fC in both); noise draw |
| all digis | 43 762 / 18 175 / 481 572 | 44 172 / 18 683 / 475 110 | |
| ECAL primitives EB (> 0, sum ET) | 200, 114.5 / 183, 96.5 / 304, 202.5 GeV | 368, 215.0 / 327, 186.0 / 428, 292.0 GeV | noisier barrel crystals; the TPG payloads changed too (the share not established) |
| ECAL primitives EE | 98, 85.5 / 91, 53.5 / 143, 158.0 GeV | 131, 113.5 / 116, 82.5 / 167, 136.5 GeV | (11a+11b): the pileup of 2.5 < abs(eta) < 3 in darker crystals (APD/PN 0.041 -> 0.013), not compensated by the linearisation |
| HCAL primitives HB / HE / HF (11a+11b) | 120, 107.0 / 261, 192.0 / 701, 1042.0 GeV | 190, 155.0 / 225, 156.0 / 705, 1064.5 GeV | HB: dark current; HE: the response corrections and TP channel parameters changed too (not established) |
| all primitives | 363, 380.5 / 282, 154.0 / 1529, 1701.5 GeV | 620, 536.5 / 503, 299.5 / 1715, 1804.0 GeV | |
| signal towers with a deposit >= 0.25 GeV and a primitive | 33 (deposit 51.0, primitives 50.0 GeV) | 32 (deposit 49.8, primitives 48.0 GeV) | less light per GeV |
| b-jet window (axis tower (-19, 49)), ECAL + HCAL | 32.0 + 23.5 (11a), 35.0 + 25.5 (11a+11b) | 29.0 + 22.5 (11a), 33.5 + 26.5 (11a+11b) | |
| bbar-jet window | 124.0 (11a), 175.0 (11a+11b) | 123.75 (11a), 178.75 (11a+11b) | |
| largest primitive | HF (-34, 11): 89.0 GeV (11a and 11a+11b) | 89.0 GeV (11a and 11a+11b) | |

(With bx -1..1, the comparison of 11 November 2026, the mixed column read: EB frames 2050 / 964, EE frames 4144 / 2520,
EE supercrystals read in full 175 / 100, all primitives 3411.5 / 2793.0 GeV for 2024 / 2025; the outputs of that version
are in `test/tmp/out_bx-1to1/11_DIGI/out/`. The year changes the same channels in both windows.)

"A different random draw": every digitiser inside the `MixingModule` draws from the one engine of `mix` (seed 12345;
`RandomNumberGeneratorService` has no engine of its own for them). A payload that changes how many random numbers one
digitiser draws (a different Lorentz angle, more dark-current photoelectrons) shifts the sequence for all the others,
so the noise of channels whose payloads did not change (the pixel noise, the ES, the HF) is a different draw. Such
differences are fluctuations, not effects of the 2025 conditions.

### 4.10 The worked examples: `examples_11_DIGI.txt` (the output of `test/11_DIGI/digiExamples_11_DIGI.py`, in full)

```
== digiExamples 11_DIGI: worked examples from out

-- 1. The silicon: the PSimHits of the signal, the digis made of them
  PXB PSimHits=  929  thickness(max |dz local|)=285 um  crossings of the full thickness=592: charge median=48008 e (25%: 34430, 75%: 73814) | per um of path: median 84.2 e (25%: 77.6) -> at normal incidence 23993 e
  PXF PSimHits=  973  thickness(max |dz local|)=290 um  crossings of the full thickness=726: charge median=27594 e (25%: 24603, 75%: 32718) | per um of path: median 82.8 e (25%: 74.0) -> at normal incidence 24015 e
  TIB PSimHits= 1182  thickness(max |dz local|)=290 um  crossings of the full thickness=983: charge median=46627 e (25%: 33841, 75%: 66749) | per um of path: median 93.5 e (25%: 80.3) -> at normal incidence 27122 e
  TID PSimHits=  975  thickness(max |dz local|)=290 um  crossings of the full thickness=854: charge median=29232 e (25%: 24414, 75%: 40292) | per um of path: median 88.2 e (25%: 76.3) -> at normal incidence 25571 e
  TOB PSimHits= 1022  thickness(max |dz local|)=470 um  crossings of the full thickness=710: charge median=71194 e (25%: 54397, 75%: 104233) | per um of path: median 90.5 e (25%: 80.5) -> at normal incidence 42516 e
  TEC PSimHits= 5191  thickness(max |dz local|)=470 um  crossings of the full thickness=1958: charge median=51076 e (25%: 42988, 75%: 69989) | per um of path: median 88.5 e (25%: 79.5) -> at normal incidence 41610 e
  pixel digis=4800  ADC median=69  25%=46 75%=105  at full scale (255)=170  below 50=1348
  pixel ADC response (BPix L2-4, FPix):   1600 e -> ADC  31 ; layer 1: ADC  34
  pixel ADC response (BPix L2-4, FPix):   2000 e -> ADC  33 ; layer 1: ADC  36
  pixel ADC response (BPix L2-4, FPix):   5000 e -> ADC  49 ; layer 1: ADC  51
  pixel ADC response (BPix L2-4, FPix):  10000 e -> ADC  80 ; layer 1: ADC  80
  pixel ADC response (BPix L2-4, FPix):  15000 e -> ADC 115 ; layer 1: ADC 112
  pixel ADC response (BPix L2-4, FPix):  20000 e -> ADC 151 ; layer 1: ADC 147
  pixel ADC response (BPix L2-4, FPix):  25000 e -> ADC 188 ; layer 1: ADC 181
  pixel ADC response (BPix L2-4, FPix):  30000 e -> ADC 222 ; layer 1: ADC 214
  pixel ADC response (BPix L2-4, FPix):  40000 e -> ADC 255 ; layer 1: ADC 255
  pixel ADC reaches 255 at 35592 e (L2-4, FPix)
  strip digis linked=19418 ADC median=64 25%=33 75%=106 (>=254: 990) | not linked=12184 ADC median=18 25%=15 75%=21 max=85
  linked strip clusters (runs of adjacent linked strips)=6473, strips per cluster=3.00

-- 2. The APV25 deconvolution pulse of the release (SimTracker/SiStripDigitizer/data/APVShapeDeco_320.txt)
  2001 values, resolution 0.1 ns, maximum at index 500
  SiLinearChargeDivider::TimeResponse: the charge is multiplied by shape[t0 + (r/c - tof)/resolution]; a hit of bunch crossing -1 has tof shifted by -25 ns, i.e. it is read 25 ns AFTER its peak
  read -30.0 ns after the peak: 0.004
  read -25.0 ns after the peak: 0.065
  read -20.0 ns after the peak: 0.239
  read -15.0 ns after the peak: 0.465
  read -12.5 ns after the peak: 0.582
  read -10.0 ns after the peak: 0.695
  read  -5.0 ns after the peak: 0.902
  read  +0.0 ns after the peak: 1.000
  read  +5.0 ns after the peak: 0.921
  read +10.0 ns after the peak: 0.742
  read +12.5 ns after the peak: 0.635
  read +15.0 ns after the peak: 0.523
  read +20.0 ns after the peak: 0.302
  read +25.0 ns after the peak: 0.121
  read +30.0 ns after the peak: 0.007
  read +40.0 ns after the peak: -0.091
  read +50.0 ns after the peak: -0.108
  curve (t after the peak [ns], shape): (-40.0,0.004) (-37.5,0.004) (-35.0,0.004) (-32.5,0.004) (-30.0,0.004) (-27.5,0.018) (-25.0,0.065) (-22.5,0.141) (-20.0,0.239) (-17.5,0.349) (-15.0,0.465) (-12.5,0.582) (-10.0,0.695) (-7.5,0.802) (-5.0,0.902) (-2.5,0.975) (0.0,1.000) (2.5,0.979) (5.0,0.921) (7.5,0.840) (10.0,0.742) (12.5,0.635) (15.0,0.523) (17.5,0.412) (20.0,0.302) (22.5,0.203) (25.0,0.121) (27.5,0.056) (30.0,0.007) (32.5,-0.030) (35.0,-0.057) (37.5,-0.077) (40.0,-0.091) (42.5,-0.100) (45.0,-0.105) (47.5,-0.108) (50.0,-0.108) (52.5,-0.107) (55.0,-0.104) (57.5,-0.101) (60.0,-0.097) (62.5,-0.092) (65.0,-0.087) (67.5,-0.083) (70.0,-0.078) (72.5,-0.073) (75.0,-0.068) (77.5,-0.064) (80.0,-0.059)
  rises above 0.01 at 28.4 ns before the peak; crosses zero 30.5 ns after the peak; minimum -0.108 at 49.2 ns

-- 3. The ECAL data frames of the signal (simEcalDigis), and the energy the particles left in the crystal
  EB: 156 frames, 10 samples each; gain ids of all samples: {1: 1560}; first three samples of all frames: mean 198.5, rms 3.79 ADC
    ieta=-82 iphi=239 amplitude=63.3 ADC (max - mean of first 3) deposit (0-100 ns)=3.719 GeV -> 17.0 ADC/GeV | samples [204, 201, 200, 200, 250, 265, 253, 241, 231, 219] gains [1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
    ieta=54 iphi=142 amplitude=56.3 ADC (max - mean of first 3) deposit (0-100 ns)=3.184 GeV -> 17.7 ADC/GeV | samples [205, 202, 204, 202, 245, 260, 251, 242, 230, 222] gains [1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
    ieta=60 iphi=162 amplitude=31.3 ADC (max - mean of first 3) deposit (0-100 ns)=1.388 GeV -> 22.6 ADC/GeV | samples [195, 197, 198, 197, 220, 228, 219, 213, 211, 204] gains [1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
    ieta=-44 iphi=219 amplitude=29.3 ADC (max - mean of first 3) deposit (0-100 ns)=1.599 GeV -> 18.4 ADC/GeV | samples [195, 193, 193, 194, 215, 223, 218, 212, 204, 204] gains [1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
    ieta=-82 iphi=258 amplitude=18.7 ADC (max - mean of first 3) deposit (0-100 ns)=0.455 GeV -> 41.0 ADC/GeV | samples [196, 196, 194, 202, 206, 207, 212, 214, 210, 209] gains [1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
    crystals with a deposit > 0.5 GeV and a frame: 13, ADC per GeV median 18.4 (deposits 0.58-3.72 GeV)
    frames in a tower/supercrystal read in full: 100; frames that passed the zero suppression elsewhere: 56
    crystals with an in-time deposit: 5218 (67.08 GeV); of them with a frame: 86; frames with no deposit: 70
    deposit > 0.1000 GeV: 113 crystals, 24 of them written
    deposit > 0.3325 GeV: 30 crystals, 19 of them written
    deposit > 0.5000 GeV: 17 crystals, 13 of them written
    deposit > 1.0000 GeV: 6 crystals, 6 of them written
  EE: 384 frames, 10 samples each; gain ids of all samples: {1: 3840}; first three samples of all frames: mean 199.7, rms 2.07 ADC
    ix=31 iy=7 z=-1 amplitude=107.3 ADC (max - mean of first 3) deposit (0-100 ns)=7.882 GeV -> 13.6 ADC/GeV | samples [199, 201, 202, 215, 283, 308, 298, 276, 253, 235] gains [1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
    ix=28 iy=13 z=-1 amplitude=107.3 ADC (max - mean of first 3) deposit (0-100 ns)=12.780 GeV -> 8.4 ADC/GeV | samples [195, 197, 198, 213, 281, 304, 294, 271, 253, 238] gains [1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
    ix=30 iy=8 z=-1 amplitude=67.7 ADC (max - mean of first 3) deposit (0-100 ns)=4.129 GeV -> 16.4 ADC/GeV | samples [198, 201, 202, 210, 253, 268, 262, 246, 233, 222] gains [1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
    ix=30 iy=7 z=-1 amplitude=47.0 ADC (max - mean of first 3) deposit (0-100 ns)=3.485 GeV -> 13.5 ADC/GeV | samples [203, 202, 201, 207, 236, 249, 243, 232, 224, 214] gains [1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
    ix=34 iy=10 z=-1 amplitude=45.0 ADC (max - mean of first 3) deposit (0-100 ns)=4.517 GeV -> 10.0 ADC/GeV | samples [202, 202, 199, 204, 236, 246, 243, 233, 224, 215] gains [1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
    crystals with a deposit > 0.5 GeV and a frame: 32, ADC per GeV median 12.1 (deposits 0.56-12.78 GeV)
    frames in a tower/supercrystal read in full: 355; frames that passed the zero suppression elsewhere: 29
    crystals with an in-time deposit: 6117 (288.62 GeV); of them with a frame: 333; frames with no deposit: 51
    EE crystals > 1 GeV written: 19, |eta| 1.55-1.66 (approx.), deposits 1.03-12.78 GeV
    EE crystals > 1 GeV NOT written: 15, |eta| 2.20-2.88 (approx.), deposits 1.02-5.81 GeV
    deposit > 0.1000 GeV: 553 crystals, 111 of them written
    deposit > 0.3325 GeV: 144 crystals, 49 of them written
    deposit > 0.5000 GeV: 78 crystals, 32 of them written
    deposit > 1.0000 GeV: 34 crystals, 19 of them written

-- 4. The HCAL frames of the signal (simHcalDigis)
  HB/HE QIE11: 4459 frames of 10 time slices, sample of interest at index [5]; median of the smallest sample of each frame: 6
    subdet=2 ieta=-18 iphi=50 depth=3  ADC [7, 11, 8, 7, 5, 118, 93, 51, 35, 34]  TDC [63, 23, 63, 63, 63, 6, 62, 62, 62, 62]
    subdet=2 ieta=-18 iphi=48 depth=1  ADC [8, 12, 8, 6, 8, 109, 84, 44, 29, 21]  TDC [63, 63, 63, 63, 63, 6, 62, 62, 62, 62]
    subdet=2 ieta=-19 iphi=48 depth=1  ADC [3, 6, 7, 6, 8, 100, 77, 38, 19, 13]  TDC [63, 63, 63, 63, 63, 7, 62, 62, 62, 49]
    subdet=2 ieta=-18 iphi=48 depth=2  ADC [6, 8, 9, 14, 11, 88, 66, 33, 19, 14]  TDC [63, 63, 63, 34, 63, 6, 62, 62, 62, 62]
    median frame (by max - min): subdet=2 ieta=-16 iphi=31 depth=4  ADC [4, 4, 5, 7, 6, 15, 12, 12, 15, 12]  TDC [63, 63, 63, 63, 63, 32, 42, 63, 31, 63]
  HF QIE10: 3456 frames of 3 time slices, sample of interest at index [1]; median of the smallest sample of each frame: 3
    subdet=4 ieta=-34 iphi=11 depth=1  ADC [2, 122, 20]  TDC [-1, -1, -1]
    subdet=4 ieta=-34 iphi=11 depth=3  ADC [5, 121, 18]  TDC [-1, -1, -1]
    subdet=4 ieta=-34 iphi=11 depth=2  ADC [3, 103, 15]  TDC [-1, -1, -1]
    subdet=4 ieta=-34 iphi=11 depth=4  ADC [3, 100, 12]  TDC [-1, -1, -1]
    median frame (by max - min): subdet=4 ieta=-31 iphi=43 depth=4  ADC [3, 2, 4]  TDC [-1, -1, -1]

-- 5. The muon digis of the mixed event (simMuon*Digis); the iteration is done by a small C++ helper
  DT digis=24 (wheel station sector superlayer layer wire time[ns]):
    -2 1 4 2 1 5 289.062
    2 1 5 1 3 2 289.062
    -2 1 4 2 4 5 338.281
    -2 1 6 1 4 16 344.531
    2 2 4 1 2 29 352.344
    2 1 10 1 4 5 365.625
    2 2 4 1 1 29 395.312
    -2 1 4 3 1 30 404.688
    -2 1 6 1 1 23 408.594
    -2 1 4 2 3 5 435.938
    2 1 5 1 1 2 446.875
    -2 1 4 3 2 31 473.438
    -2 1 4 2 2 5 476.562
    -2 1 6 1 1 15 501.562
    2 1 10 1 3 5 518.75
    -2 1 6 1 3 15 557.812
    2 1 5 1 2 3 558.594
    0 1 6 3 4 38 587.5
    2 1 10 1 1 5 600.781
    0 1 10 3 3 18 643.75
    -2 1 6 1 2 16 645.312
    2 1 10 1 2 6 678.906
    -2 1 6 1 2 24 681.25
    2 2 1 1 3 56 711.719
  CSC strip digis=7216 in 47 chambers (endcap station ring chamber: strips)
    1 1 1 26: 112 strip digis in layers 1,2,3,4,5
    1 1 1 27: 208 strip digis in layers 1,2,3,4,5,6
    1 1 1 3: 176 strip digis in layers 1,2,3,4,5,6
    1 1 1 31: 160 strip digis in layers 1,2,3,4,5,6
    1 1 1 36: 192 strip digis in layers 1,2,3,4,5,6
    1 1 1 4: 160 strip digis in layers 1,2,3,4,5,6
    1 1 1 5: 128 strip digis in layers 1,2,3,4,5,6
    1 1 4 11: 192 strip digis in layers 1,2,3,4,5,6
    1 1 4 15: 192 strip digis in layers 1,2,3,4,5,6
    1 1 4 21: 160 strip digis in layers 1,2,3,4,5,6
    1 1 4 23: 112 strip digis in layers 1,2,3,4,5,6
    1 1 4 4: 96 strip digis in layers 1,2,3,4,5,6
    1 1 4 5: 160 strip digis in layers 1,2,3,4,5,6
    1 1 4 7: 64 strip digis in layers 1,2,3,4
    1 1 4 8: 144 strip digis in layers 1,2,3,4,5,6
    1 1 4 9: 112 strip digis in layers 2,3,4,5,6
    1 2 1 1: 80 strip digis in layers 1,2,3,4,5
    1 2 1 5: 144 strip digis in layers 1,2,3,4,5,6
    1 2 1 8: 192 strip digis in layers 1,2,3,4,5,6
    2 1 1 19: 96 strip digis in layers 1,2,3,4,5,6
    2 1 1 20: 288 strip digis in layers 1,2,3,4,5,6
    2 1 1 26: 176 strip digis in layers 1,2,3,4,5,6
    2 1 1 30: 176 strip digis in layers 1,2,3,4,5,6
    2 1 1 31: 256 strip digis in layers 1,2,3,4,5,6
    2 1 1 4: 160 strip digis in layers 1,2,3,4,5,6
    2 1 1 6: 192 strip digis in layers 1,2,3,4,5,6
    2 1 1 7: 176 strip digis in layers 1,2,3,4,5,6
    2 1 2 25: 144 strip digis in layers 1,2,3,4,5,6
    2 1 4 12: 192 strip digis in layers 1,2,3,4,5,6
    2 1 4 14: 160 strip digis in layers 1,2,3,4,5,6
    2 1 4 16: 96 strip digis in layers 1,2,3,4,5,6
    2 1 4 18: 144 strip digis in layers 1,2,3,4,5,6
    2 1 4 19: 288 strip digis in layers 1,2,3,4,5,6
    2 1 4 2: 192 strip digis in layers 1,2,3,4,5,6
    2 1 4 25: 96 strip digis in layers 1,2,3,4,5,6
    2 1 4 27: 128 strip digis in layers 1,2,3,4,5,6
    2 1 4 3: 192 strip digis in layers 1,2,3,4,5,6
    2 1 4 33: 128 strip digis in layers 1,2,3,4,5,6
    2 1 4 9: 192 strip digis in layers 1,2,3,4,5,6
    2 2 1 10: 256 strip digis in layers 1,2,3,4,5,6
    2 2 1 11: 48 strip digis in layers 1,2,6
    2 2 1 16: 96 strip digis in layers 1,2,3,4,5,6
    2 2 1 3: 160 strip digis in layers 1,2,3,4,5,6
    2 2 1 4: 96 strip digis in layers 1,2,3,4,5,6
    2 2 1 7: 112 strip digis in layers 2,3,4,5,6
    2 3 1 11: 96 strip digis in layers 1,2,3,4,5,6
    2 4 1 11: 96 strip digis in layers 1,2,3,4,5,6
  CSC strip with the largest pulse: endcap 1 ME1/4 chamber 8 layer 3 strip 2, 8 samples (50 ns each): 1021 952 2887 4095 4095 4095 4095 4095
  CSC strip pulse heights (max - min of the 8 samples): median 5, 90% 18, max 3143
  a median strip: ME1/4 chamber 14 layer 3 strip 27: 1012 1009 1007 1009 1010 1007 1009 1012
  CSC wire digis=263 in 41 chambers; first time bin: {'0': 19, '1': 57, '2': 24, '3': 24, '4': 22, '5': 2, '6': 5, '7': 17, '8': 16, '9': 38, '10': 22, '11': 16, '13': 1}
  RPC digis=7 (region station ring strip bx): -1 1 2 15 0; 0 1 2 53 1; 0 2 -1 6 -3; 1 3 2 24 4; -1 3 3 28 -3; -1 3 3 22 -2; -1 4 2 27 0
  GEM digis=101 (region station chamber layer strip bx): -1 1 32 1 362 -6; -1 1 32 1 363 -6; -1 1 32 1 364 -6; -1 1 11 1 120 -1; -1 1 11 1 121 -1; 1 1 26 1 292 -4; 1 1 26 1 293 -4; 1 1 26 1 294 -4; -1 1 30 1 204 -7; -1 1 30 1 205 -7; -1 1 30 1 206 -7; -1 1 30 1 220 -8; 1 1 20 1 206 -2; 1 1 20 1 207 -2; 1 1 36 1 159 -6; 1 1 36 1 160 -6; 1 1 5 1 290 1; 1 1 5 1 291 1; -1 1 6 1 311 1; -1 1 6 1 312 1; 1 1 27 1 370 -6; 1 1 27 1 371 -6; 1 1 4 1 27 1; 1 1 4 1 28 1; 1 1 26 1 383 -6; -1 1 7 1 56 -1; -1 1 7 1 57 -1; 1 1 9 1 149 -4; 1 1 9 1 150 -4; 1 1 9 1 151 -4; 1 1 9 1 152 -4; -1 1 20 1 213 -11; -1 1 20 1 214 -11; -1 1 20 1 215 -11; -1 1 2 1 163 -7; -1 1 2 1 164 -7; -1 1 2 1 164 -6; -1 1 2 1 165 -6; -1 1 2 1 172 -10; -1 1 2 1 173 -10; -1 1 2 1 174 -10; -1 1 2 1 175 -10; 1 1 7 1 127 -11; 1 1 7 1 128 -11; -1 1 12 1 220 -9; -1 1 12 1 221 -9; 1 1 15 1 89 2; 1 1 15 1 90 2; 1 1 15 1 91 2; -1 1 20 2 315 3; -1 1 20 2 316 3; -1 1 20 2 317 3; 1 1 21 2 353 -3; 1 1 21 2 354 -3; 1 1 21 2 355 -3; 1 1 21 2 356 -4; 1 1 21 2 356 -3; 1 1 21 2 357 -3; 1 1 21 2 358 -3; -1 1 31 2 370 -6; -1 1 31 2 371 -6; -1 1 31 2 372 -6; -1 1 32 2 380 -6; -1 1 32 2 381 -6; -1 1 32 2 382 -6; 1 1 26 2 307 -3; 1 1 26 2 308 -3; -1 1 30 2 184 -7; -1 1 30 2 185 -7; -1 1 30 2 186 -7; 1 1 36 2 168 -4; 1 1 36 2 169 -4; 1 1 2 2 304 -11; 1 1 5 2 276 1; 1 1 5 2 277 1; -1 1 6 2 305 1; -1 1 6 2 306 1; 1 1 27 2 361 -6; 1 1 27 2 362 -6; 1 1 27 2 363 -6; 1 1 4 2 17 1; 1 1 4 2 18 1; -1 1 7 2 63 -2; -1 1 7 2 64 -2; -1 1 7 2 65 -2; -1 1 7 2 65 0; 1 1 9 2 129 -4; 1 1 9 2 130 -4; 1 1 9 2 131 -4; -1 1 20 2 210 -12; -1 1 2 2 165 -11; -1 1 2 2 166 -11; 1 1 7 2 116 -10; 1 1 7 2 117 -10; 1 1 7 2 118 -10; -1 1 12 2 226 -10; -1 1 12 2 227 -10; 1 1 15 2 73 1; 1 1 15 2 74 1; 1 1 15 2 75 1; -1 2 16 1 116 -7
```

How the chapter reads it.

- Section 1, the pixels (chapter 11.3, "The pixels"): the sensors are 285 um thick (also `SensorThick = 0.285*mm` of
  `Geometry/TrackerCommonData/data/PhaseI/pixbarladder.xml`); the 592 PXB crossings of the full thickness have a median
  of 84.2 electrons per um of path, so a crossing at right angles frees 84 x 285 = 24 000 electrons (the script: 23 993):
  24 000 / 1600 = 15 times the threshold, 24 000 / 175 = 137 ("140") times the noise; 24 000 / 2.5 = 9600 electrons per
  pixel give ADC 78 on the response; the median pixel digi has ADC 69, 170 of 4800 are at 255, the response reaches 255
  at 35 592 ("35 600") electrons. The Lorentz drift: tan(theta_L) per tesla 0.0358 (layer 1) and 0.0709 (layers 3, 4)
  (section 4.9) x 3.8 T x 285 um = 39 and 77 um.
- Section 1, the strips ("The strips"): sensitive thickness 290 um (TIB, TID) and 470 um (TOB, TEC); TIB median 93.5
  electrons per um: 93.5 x 290 = 27 115 ("27 000") electrons = 110 ADC counts at 247 electrons per count; linked strip
  digis 19 418, median ADC 64, 990 at 254 or 255; unlinked 12 184, median 18, maximum 85; 6473 clusters of 3.0 strips;
  19 418 / 8370 = 2.3 linked strip digis per hit. The noise estimate of the box: a Gaussian fluctuation passes 2 sigma
  with probability 0.0228; a strip and one of its two neighbours both above 2 sigma: about 2 x 0.0228^2 = 0.001 per strip.
- Section 2 (figure 11.3, right): the pulse is 0.121 at +25 ns (a hit of bunch crossing -1, `Adjuster.cc` shifts its
  time by -25 ns and `SiLinearChargeDivider::TimeResponse` reads the shape at t0 + (r/c - tof)), 0.065 at -25 ns (bunch
  crossing +1), 0.302 at +20 ns and 0.004 at -30 ns; above 0.01 from 28.4 ns before to 30.5 ns after the peak (the strip
  window of figure 11.2: hit times -30.5 to +28.4 ns).
- Section 3, the ECAL ("The ECAL and the preshower"): the frame of the worked example, EB (-82, 239), 3.719 GeV in time,
  samples 204 201 200 200 250 265 253 241 231 219 at gain 12 (gain id 1 in every sample of every frame); pedestal
  (204 + 201 + 200) / 3 = 201.7, amplitude 63.3. The digitiser's full scale (`EcalCoder.cc`, line 23: `m_maxEneEB(1668.3)`,
  "4095(MAXADC)*12(gain 2)*0.035(GeVtoADC)*0.97"; line 24: EE 2859.9) gives 1668.3 / (4095 x 12) = 0.03395 GeV per count at
  gain 12 (EE: 0.0582), 1 / 0.03395 = 29.5 counts per GeV, 3.719 / 0.03395 = 110 counts; gain 12 up to (4095 - 200) x
  0.03395 = 132 ("some 130") GeV, before the intercalibration and with a gain ratio of 12 (the actual ratios are the
  payload `EcalGainRatios_mc`). Observed 17.0 counts per GeV for this crystal, median 18.4 over 13 crystals, 18.4 / 29.5 =
  0.62 ("about 0.6"). EB: 156 frames = 100 in the 4 towers read in full + 56 zero-suppressed; crystals with an in-time
  deposit 5218 (67.08 GeV); above 1 GeV 6 of 6 written, above 0.3325 GeV 19 of 30, above 0.1 GeV 24 of 113; 70 frames
  without an in-time deposit. EE: 384 frames, 355 in the 16 supercrystals read in full; above 1 GeV 34 crystals, 19
  written (|eta| 1.55-1.66, the b jet) and 15 not (|eta| 2.20-2.88, up to 5.81 GeV). The laser-monitoring ratios quoted
  for 2 < |eta| < 2.5 (0.22) and 2.5 < |eta| < 3 (0.013) are the 2025 `p2` of section 4.9.
- Section 4, the HCAL ("The HCAL"): the frame of figure 11.4 (middle) is HE (-18, 50), depth 3: codes 7 11 8 7 5 118 93
  51 35 34, TDC 63 23 63 63 63 6 62 62 62 62 (TDC code x 0.5 ns: 23 -> 11.5 ns, 6 -> 3 ns; 62 and 63 are the special codes
  of `HcalTDCParameters.h`); the HF frame (figure 11.4, right) is (-34, 11), depth 1: 2 122 20; the median kept HB/HE
  frame is HE (-16, 31), depth 4: 4 4 5 7 6 15 12 12 15 12. 4459 HB/HE frames kept for 173 + 648 = 821 HB and HE cells
  with a hit (`simHits_signal_10_SIM.log`): at least 4459 - 821 = 3638 ("at least 3600") frames have no hit; library
  event 747: 4186 frames for 0.04 GeV in the HB and HE (FWLite, section 4.5: 1 HB and 15 HE cells in the test numbering).
- Section 5, the muon chambers ("The muon chambers"): 24 DT digis between 289.062 and 711.719 ns, minus the offset of
  500 ns (`muonDTDigis_cfi.py`, `pset.offset = 500`): -211 to +212 ns. Twelve of them are the twelve DT digis of the bx
  -1..1 version (`test/tmp/out_bx-1to1/11_DIGI/out/examples_11_DIGI.txt`) on the same wires 275 ns earlier: nine to the
  TDC count (e.g. wheel -2 station 1 sector 4 SL 2 layer 1 wire 5: 564.062 -> 289.062), three within 1.6 to 3.1 ns
  (2 2 4 1 2 29: 625.781 -> 352.344; 2 2 4 1 1 29: 672.656 -> 395.312; 0 1 10 3 3 18: 921.875 -> 643.750), the 2.4 ns
  smearing of the digitiser drawn again. 7216 CSC strip digis in 47 chambers, 48 to 288 strips each (multiples of 16);
  the chambers (endcap 2 = negative z) ME1/1 chamber 20, ME2/1 chamber 10, ME3/1 and ME4/1 chamber 11 are those of the
  trigger muon of the bx -1..1 version, library event 92 (section 4.5), now in bx -11; ring 4 of station 1 is ME1/1a,
  so the largest pulse (endcap 1, ME1/4 chamber 8 layer 3 strip 2, 1021 952 2887 4095 4095 4095 4095 4095) is in ME1/1
  and saturates the 12-bit ADC. Strip pulse heights (largest minus smallest of the 8 samples): median 5, 90 % below
  18. Wire time bins 0 to 13: 0-5 148 digis, 6-11 114, 13 one; RPC bunch crossings -3 to +4 (all 7 without a link);
  GEM -12 to +3 (80 early, 21 late by the DigiAnalyzer).

Numbers of the chapter derived from the tables above (sums and ratios):

| quantity | value | from |
|---|---|---|
| strip digis of the signal | 36 402 - 4800 = 31 602 | 4.1 |
| pixel / strip PSimHits of the bunch crossing | 416 362 / 1 690 319 | 4.4 |
| pixel / strip digis per hit, bunch crossing | 66 773 / 416 362 = 0.16 and 373 891 / 1 690 319 = 0.22 | 4.1, 4.4 |
| mixed pixels: signal / in-time / early / noise | 2826 + 1800 = 4626 / 40 191 + 19 731 = 59 922 / 1689 + 517 = 2206 / 19 + 0 = 19 | 4.1 |
| mixed strips: signal / in-time / early / late / noise | 19 024 / 240 017 / 98 795 / 6757 / 9298 | 4.1 |
| early digis by bunch crossing (largest link fraction) | pixels: bx -1 1748, -2 125, -4 200, other 133; strips: bx -1 90 157, -2 6290, other 2348; late strips +1 6671, +2 86 | 4.5 |
| ECAL crystals | 61 200 + 14 648 = 75 848 | 4.5 |
| pixel window for a hit of bunch crossing -1 | tof - r/c = delay - 25 ns within -12.5..+12.5 ns: delay 12.5 to 37.5 ns | `TofLowerCut`, `TofUpperCut` |

### 4.11 The payloads and release files of the worked examples

The payloads, from `conddb list 150X_mcRun3_2025_realistic_v1` and `conddb dump <tag>` (after `cmsenv`):

| record | tag | what the chapter uses |
|---|---|---|
| `SiStripThresholdRcd` | `SiStripThreshold_Ideal_31X_v2` | 15 148 modules, each one range from strip 0 with `FirstStrip-and-Hth = 25` and `lowTh = 10`, i.e. high threshold 25 / 5 = 5.0 and low threshold 10 / 5 = 2.0 times the strip noise (`CondFormats/SiStripObjects/interface/SiStripThreshold.h`, lines 60-61: `getHth() = (... & HighThStripMask_) / 5.0`, `getLth() = lowTh / 5.0`); used by FED algorithm 4 (`RecoLocalTracker/SiStripZeroSuppression/src/SiStripFedZeroSuppression.cc`, lines 428-445) |
| `ESThresholdsRcd` | `ESThresholds_LG_V03_mc` | `ts2 = 6`, `zs = 0`: the preshower threshold of 6 ADC counts on the second sample |
| `EcalSimPulseShapeRcd` | `EcalSimPulseShapePhaseI` | `barrel-shape`: 500 values, 1 ns apart (`time-interval = 1`), maximum 1.0 at index 135; above 0.01 from 50 ns before the peak; at the sample times (peak + (k - 6) x 25 ns, k = 1..10): 0, 0, 0, 0.015, 0.774, 1, 0.882, 0.668, 0.470, 0.316 (the line of figure 11.4, left, scaled to 63.3 counts on 201.7) |

The release files (all under `$CMSSW_RELEASE_BASE/src`):

| statement of the chapter | file, line |
|---|---|
| the pileup hits are shifted by 25 ns per bunch crossing | `SimGeneral/MixingModule/plugins/Adjuster.cc`, lines 30 and 44 (`timeOffset = bcr * bunchSpace`, `setTof(timeOfFlight() + timeOffset)`) |
| the strip time window and the APV pulse | `SimTracker/SiStripDigitizer/plugins/SiStripDigitizerAlgorithm.cc`, lines 192-194 (`fabs(tof - cosmicShift - r/30) < tofCut`); `SiLinearChargeDivider.cc`, lines 178-185 (`TimeResponse`); the pulse file `SimTracker/SiStripDigitizer/data/APVShapeDeco_320.txt` |
| 3.61 eV per pair, diffusion 3.7 um per 300 um | `SimTracker/SiPixelDigitizer/plugins/SiPixelDigitizerAlgorithm.cc`, lines 202-204 (`GeVperElectron(3.61E-09)`, `Sigma0(0.00037)`, `Dist300(0.0300)`) |
| the pixel ADC response | the same file, from line 2193 (`missCalibrate`: `(signal - ElectronsPerVcal_Offset) / ElectronsPerVcal`, layer 1 with the `_L1` values, `p3 + p2 * tanh(p0 * signal - p1)`) and lines 1712-1719 (used because `MissCalibrate = True`), capped at `AdcFullScale = 255` |
| pixel sensor and readout chip | `Geometry/TrackerCommonData/data/PhaseI/pixbarladder.xml`, line 13 (`SensorThick = 0.285*mm`); `trackerStructureTopology.xml`, `PixelROCRows = 80`, `PixelROCCols = 52`; the pitch of 100 x 150 um is the Phase-1 design value, not read from a file |
| strip cross-talk | `thresholds_11_DIGI.txt`, `CouplingConstantRunIIDecIB1 [0.8361, 0.0703, 0.0117]` (`CouplingConstantsRunIIDecB = True`) |
| ECAL digitiser full scale | `SimCalorimetry/EcalSimAlgos/src/EcalCoder.cc`, lines 23-24 (`m_maxEneEB(1668.3)`, `m_maxEneEE(2859.9)`), line 109 (`LSB[igain] = Emax / (MAXADC * gains[igain])`), line 165 (divided by the intercalibration `icalconst`) |
| ECAL selective-readout unit | `SimCalorimetry/EcalSelectiveReadoutAlgos/src/EcalSelectiveReadoutSuppressor.cc`, lines 83-86 (`ebDccAdcToGeV`, `eeDccAdcToGeV` of the `EcalSRSettings` payload): 0.3325 GeV = 9.5 counts, i.e. 35 MeV per count (the name of the payload, section 4.9) |
| HCAL photoelectrons per GeV | `SimCalorimetry/HcalSimAlgos/src/HcalSimParameters.cc`, lines 64-73 (section 4.3) |
| QIE11 TDC | `SimCalorimetry/HcalSimAlgos/interface/HcalTDCParameters.h` (6 bits, 50 bins per 25 ns, `alreadyTransitionCode = 62`, `noTransitionCode = 63`); `src/HcalTDC.cc`, lines 45-54 |
| HCAL zero suppression | `SimCalorimetry/HcalZeroSuppressionProducers/src/HcalZSAlgoRealistic.cc` (section 4.3) |
| DT digitiser | `SimMuon/DTDigitizer/python/muonDTDigis_cfi.py`: `IdealModelConstantDriftVelocity = 0.0055` cm/ns (55 um/ns, the ideal model; `IdealModel = False`, so only an order of magnitude), `pset.offset = 500`, `Smearing = 2.4`, `deadTime = 150`, "phase 1 units (25/32 ns)" |
| CSC strips | `SimMuon/CSCDigitizer/src/CSCStripElectronicsSim.cc`: line 42 (`sca_time_bin_size(50.)`), lines 240-280 (`channelsToRead`: the whole CFEB of 16 strips, and the neighbouring one within 3 strips of an edge); `muonCSCDigis_cfi.py`: `nScaBins = 8`, `scaPeakBin = 5`, `stripConditions = 'Database'` |
| the time windows of figure 11.2 | pixels `TofLowerCut / TofUpperCut = -12.5 / 12.5`; strips `TOFCutForDeconvolution = 50` and the APV pulse; ECAL `binOfMaximum = 6` of 10 samples; HB/HE `readoutFrameSize = 10`, `binOfMaximum = 6`; HF 3 and 2; GEM `minBunch = -5`, `maxBunch = 3`; CSC `signalStartTime = -250`, `signalStopTime = 500`; DT: the 12 digis of the event (section 4.10) |
