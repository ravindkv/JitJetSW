# Chapter 11, DIGI: technical companion

The digitisation of the SimHits of chapter 10, for the signal alone (11a), for one pileup collision alone (11b) and
for the signal with its pileup mixed in (11a+11b). Script: `test/11_DIGI/run_11_DIGI.sh`; everything it produces is
in `test/11_DIGI/out/`. The chapter text is `doc/chapter/chapter_11_DIGI.tex`. This is the last chapter with the three
samples (Rule H of the Constitution): from chapter 12 on only the mixed event is followed, so everything a later
chapter needs to know about the signal alone is computed here. The chapter has a hand-over section (Rule B6), "Made
here for the next stage: the trigger primitives": the DIGI step also makes the calorimeter trigger primitives that the
Level-1 trigger of chapter 12 uses; sections 3 and 4.7 of this companion are about them. All commands below are run from
`test/` of an initialized CMSSW_15_0_5 (`cmsenv`), with `J=$CMSSW_BASE/src/JitJetSW/Journey`.

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

# (11b) library event 72 alone
cmsDriver.py step3 --filein file:$J/test/10_SIM/out/step2_SIM_pileup_10_SIM.root \
  --conditions auto:phase1_2025_realistic --era Run3_2025 --geometry DB:Extended \
  -n -1 -s DIGI:pdigi_valid --datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG --fileout file:step3_DIGI_pileup_11_DIGI.root \
  --customise_commands "process.source.eventsToProcess = cms.untracked.VEventRange('1:1:72')" \
  --python_filename $J/test/11_DIGI/step3_DIGI_pileup_11_DIGI_cfg.py --no_exec

# (11a+11b) the signal with PU = 60 per bunch crossing in bx -1..1, the collisions of the chapter 10 mixing (playback)
cmsDriver.py step3 --filein file:$J/test/10_SIM/out/step3_MIX_10_SIM.root \
  --pileup "AVE_60_BX_25ns,{'N': 60, 'B': (-1, 1)}" --pileup_input file:$J/test/10_SIM/out/step2_SIM_pileup_10_SIM.root \
  --conditions auto:phase1_2025_realistic --era Run3_2025 --geometry DB:Extended \
  -n -1 -s DIGI:pdigi_valid --datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG --fileout file:step3_DIGI_mixed_11_DIGI.root \
  --customise_commands "process.source.eventsToProcess = cms.untracked.VEventRange('1:1:1')\nprocess.mix.input.sequential = cms.untracked.bool(True)\nprocess.mix.playback = cms.untracked.bool(True)" \
  --python_filename $J/test/11_DIGI/step3_DIGI_mixed_11_DIGI_cfg.py --no_exec
```

| argument | meaning |
|---|---|
| `step3` | the name of the configuration (free; `step1` was GEN, `step2` SIM) |
| `--filein file:...` | the input (Rule E): the SIM file of the signal (11a), the 250-event library (11b), or the chapter-10 mixing output `step3_MIX_10_SIM.root`, which is the signal plus the `CrossingFramePlaybackInfoNew` of its mixing (11a+11b) |
| `--pileup "AVE_60_BX_25ns,{'N': 60, 'B': (-1, 1)}"` | the mixing scenario: `mix_POISSON_average_cfi` (`Configuration/StandardSequences/python/Mixing.py`), Poisson with mean `N = 60` collisions per bunch crossing, bunch crossings `B = (-1, 1)`, 25 ns spacing. Only for (11a+11b); without `--pileup` cmsDriver loads `mixNoPU_cfi`, the same module with no secondary source |
| `--pileup_input file:...` | the library the secondary source reads: `step2_SIM_pileup_10_SIM.root` of chapter 10 |
| `--conditions auto:phase1_2025_realistic` | the global tag of the CMS chapters of the book (Rule I of the Constitution: CMSSW_15_0_5 is a release of 2025), `150X_mcRun3_2025_realistic_v1` (`Configuration/AlCa/python/autoCond.py`): the pedestals, noise, gains, thresholds, laser transparencies and dead channels that the digitisers read from the database. Against the 2024 global tag of the first version of this chapter (`150X_mcRun3_2024_realistic_v2`) it changes, for this step: `EcalPedestals_2025_at80fb_corrected_mc` (noisier barrel crystals), `EcalLaserAPDPNRatios_TL470fb_moy_2025_outliers_cleaned_3sigma_mc` (darker crystals), `EcalSRSettings_2024_EBZS9.5_EEZS6.0_DCC2_mc` (higher barrel zero suppression), `EcalChannelStatus_2025_mc`, nine `EcalTPG*` payloads (the trigger primitives), `HcalPedestals` / `HcalPedestalWidths` / `HcalRespCorrs` / `HcalSiPMParameters` / `HcalZSThresholds` `2025_MOY`, `HcalTPChannelParameters_2024_v2.0_mc`, `SiPixelLorentzAngleSim_phase1_EoR3_HV800_Tr2000` and the pixel templates of the charge reweighting (`SiPixel2DTemplateDBObject` `numerator` / `denominator`, `phase1_EoR3_HV800_Tr2000`), `SiStripBadComponents_realisticMC_for2025_v1_mc`; section 4.9 |
| `--era Run3_2025` | the era modifiers of the 2025 detector: they set the Run-3 pixel thresholds (`run3_common`), the HB SiPM parameters (`run3_HB`), the 10-time-slice HB/HE readout and the one-slice zero suppression (`run3_common`), the GEM luminosity, the Run-2 CSC timing offsets (`run2_common`). `Run3_2025` is `Run3_2024` plus `run3_GEM_2025`, `run3_CSC_2025`, `run3_SiPixel_2025`, `stage2L1Trigger_2025` and `ecal_cctiming` (`Configuration/Eras/python/Era_Run3_2025_cff.py`); none of them touches the DIGI step (they change the GEM rec-hits, the CSC trigger primitives of the L1 step, the pixel clusteriser, the CICADA model of the L1 step and the ECAL rec-hit timing): the `dumpPython()` of the mixed configuration is the same with both eras except for the name of the global tag |
| `--geometry DB:Extended` | the reconstruction geometry from the database (`GeometryRecoDB_cff`): the digitisers need the module and cell positions |
| `-n -1` with `eventsToProcess` | all events of the input, restricted by the customise line to run:lumi:event 1:1:1 (the signal) or 1:1:72 (library event 72) |
| `-s DIGI:pdigi_valid` | the step: the `pdigi_valid` sequence of `Configuration/StandardSequences/python/Digi_cff.py` (section 2) |
| `--datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG` | the output tier and the full debug content: every digi and link collection, the SIM products of the signal, the truth |
| `--fileout file:...` | the output ROOT file (written in `out/`, where `cmsRun` is started) |
| `--customise_commands "..."` | python lines appended to the configuration: the event selection; for (11a+11b) `process.mix.input.sequential = True` (the library is read from its first event on, not from a random position) and `process.mix.playback = True` (the module replays the chapter-10 draw from the `CrossingFramePlaybackInfoNew` of the input file instead of drawing again) |
| `--python_filename ... --no_exec` | write the configuration and do not run it; the script runs `cmsRun` itself |

Why playback: `BMixingModule::produce` (`Mixing/Base/src/BMixingModule.cc`, lines 292-299) calls `initializeEvent`,
`addSignals` and then `doPileUp`; `addSignals` runs the digitisers on the signal, which draw random numbers (noise,
photostatistics, threshold smearing) from the engine of the module `mix` before `PileUp::CalculatePileup`
(`Mixing/Base/src/PileUp.cc`, called at line 445 of `SimGeneral/MixingModule/plugins/MixingModule.cc`) draws the number
of collisions. In chapter 10 (`mixOnly_cff`, no digitiser) the draw with seed 12345 gave 71 / 68 / 54; a fresh draw in
the DIGI job gave 65 / 67 / 56. With `playback = True` (`MixingModule.cc`, lines 416-483) the module reads
`CrossingFramePlaybackInfoNew` from the input and mixes exactly library events 1-71 (bx -1), 72-139 (bx 0) and 140-193
(bx +1), as `addPileupInfo` confirms (section 4).

## 2. The cmsRun on the python config

```bash
cd $J/test/11_DIGI/out
cmsRun ../step3_DIGI_signal_11_DIGI_cfg.py > step3_DIGI_signal_11_DIGI.log 2>&1     # 31 s (15 s for the event)
cmsRun ../step3_DIGI_pileup_11_DIGI_cfg.py > step3_DIGI_pileup_11_DIGI.log 2>&1     # 31 s (15 s for the event)
cmsRun ../step3_DIGI_mixed_11_DIGI_cfg.py  > step3_DIGI_mixed_11_DIGI.log  2>&1     # 41 s (25 s for the event; 193 library events read)
```

Or the whole chapter: `./11_DIGI/run_11_DIGI.sh` (variables: `CASES="signal pileup mixed"`, `REUSE=1` keeps existing
DIGI files; `PU=60`, `BUNCHES=-1,1`, `SKIP=71`, `NPU=68` are set in `test/common/jitjet_common.sh` for the whole book;
`RUN=0` only writes the configurations, `ANALYZE=0` skips the analyzers, `PNG=0` skips the PNG pages). The three
DIGI jobs run in parallel; the times are in `run_11_DIGI.log`, `out/run_signal.log` and `out/run_pileup.log` (12 November
2026, three jobs started together at 13:14:42 on a shared machine; the whole chapter took 7 minutes, most of it the
three DigiAnalyzer pages, 1.5, 1 and 3 minutes).

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
  `process.mix.bunchspace = 25`, `process.mix.minBunch = -1`, `process.mix.maxBunch = 1`, `process.mix.input.fileNames`
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
  outputFile=digiAnalyzer_pileup_11_DIGI.root maxEvents=1 eventsToProcess=1:1:72 \
  globalTag=auto:phase1_2025_realistic era=Run3_2025 geometry=DB:Extended \
  pdfFile=digiAnalyzer_pileup_11_DIGI.pdf pngPrefix=digiAnalyzer_pileup_11_DIGI savePNG=1 \
  sampleLabel="chapter 11_DIGI (11b): one pileup collision digitised|library event 72|SoftQCD inelastic, 13.6 TeV|2025 detector, no signal" \
  > digiAnalyzer_pileup_11_DIGI.log 2>&1
cmsRun ../../common/digiAnalyzer_cfg.py inputFiles=file:$J/test/11_DIGI/out/step3_DIGI_mixed_11_DIGI.root \
  outputFile=digiAnalyzer_mixed_11_DIGI.root maxEvents=1 eventsToProcess=1:1:1 \
  globalTag=auto:phase1_2025_realistic era=Run3_2025 geometry=DB:Extended \
  pdfFile=digiAnalyzer_mixed_11_DIGI.pdf pngPrefix=digiAnalyzer_mixed_11_DIGI savePNG=1 \
  sampleLabel="chapter 11_DIGI (11a+11b): signal + pileup digitised|PU = 60 per bx (Poisson), bx -1..1|the collisions of the chapter 10 mixing (playback)|2025 detector, 13.6 TeV" \
  > digiAnalyzer_mixed_11_DIGI.log 2>&1
python3 ../../common/digiThresholds.py ../step3_DIGI_mixed_11_DIGI_cfg.py > thresholds_11_DIGI.txt
python3 ../../common/digiLedger.py . 11_DIGI > ledger_11_DIGI.txt
```

What the analyzer does: it reads the digi collections of the table above, places every digi at the position of its
pixel / strip (module topology), crystal / tower (`CaloGeometry`), wire / strip (muon geometries) and draws two pages,
eta-phi (seen from the signal vertex `genParticles:xyz0`) and r-z. Tracker and muon digis are attributed through
their links: for every channel the link with the largest charge fraction decides; `EncodedEventId` bx 0 event 0 is
the signal, bx 0 event > 0 in-time pileup, bx < 0 early, bx > 0 late, no link at all is "noise". Calorimeter digis
are sized by their amplitude above pedestal in ADC counts: ECAL, the largest of the 10 samples (at gain 12
equivalent) minus the mean of the first 3; ES, the largest of the 3 minus the first; HCAL, the sum of the samples
`hcalNSumTS = 2` from the sample of interest of the frame minus the mean of the earlier ones (a frame whose amplitude
comes out negative is not drawn or counted: this is why the HF, which has no zero suppression and 3456 frames in
every file, is counted with 2270 / 2169 / 1889 digis). The signal `ak4GenJets` above `jetPtMin = 30` GeV are drawn
as circles of radius 0.4. Tracker and muon digis are thinned for the drawing (`maxDrawn = 8000` per subdetector);
the counts are exact. Products:

| file | content |
|---|---|
| `digiAnalyzer_<case>_11_DIGI.pdf` | 2 pages: eta-phi, r-z (A4 MediaBox with a CropBox, do not include directly) |
| `digiAnalyzer_<case>_11_DIGI_etaphi_run1_event<E>.png`, `..._rz_run1_event<E>.png` | the same pages as PNG (E = 1 for the signal and the mixed event, 72 for the pileup collision) |
| `digiAnalyzer_<case>_11_DIGI.log` | the counts: per subdetector `nDigis`, `signal / inTime / early / late / noLink` (tracker and muon) or `sumAmplitude[ADC]` (calorimeters), the pileup summary, the total |
| `digiAnalyzer_<case>_11_DIGI.root` | the `TFileService` file (empty histograms directory; the drawing goes to the PDF) |
| `thresholds_11_DIGI.txt` | every threshold-like parameter of the digitisers of the mixed configuration, then the full parameter sets (now including the two trigger-primitive producers, `simEcalTriggerPrimitiveDigis` and `simHcalTriggerPrimitiveDigis`) |
| `ledger_11_DIGI.txt` | the three logs side by side: digis per subdetector and origin, calorimeter amplitudes |

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
  > trigPrim_signal_11_DIGI.log 2>&1                           # 8 s each
cmsRun ../../common/trigPrimAnalyzer_cfg.py inputFiles=file:$J/test/11_DIGI/out/step3_DIGI_pileup_11_DIGI.root \
  maxEvents=1 eventsToProcess=1:1:72 globalTag=auto:phase1_2025_realistic era=Run3_2025 geometry=DB:Extended \
  pdfFile=trigPrim_pileup_11_DIGI.pdf pngPrefix=trigPrim_pileup_11_DIGI savePNG=1 mixed=0 \
  sampleLabel="chapter 11_DIGI (11b): one pileup collision|trigger primitives made by the DIGI step|library event 72, no signal" \
  > trigPrim_pileup_11_DIGI.log 2>&1
cmsRun ../../common/trigPrimAnalyzer_cfg.py inputFiles=file:$J/test/11_DIGI/out/step3_DIGI_mixed_11_DIGI.root \
  maxEvents=1 eventsToProcess=1:1:1 globalTag=auto:phase1_2025_realistic era=Run3_2025 geometry=DB:Extended \
  pdfFile=trigPrim_mixed_11_DIGI.pdf pngPrefix=trigPrim_mixed_11_DIGI savePNG=1 mixed=1 \
  sampleLabel="chapter 11_DIGI (11a+11b): signal + pileup|trigger primitives made by the DIGI step|PU = 60 per bx, bx -1..1 (playback)" \
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
| `trigPrim_<case>_11_DIGI_ecaltp_run1_event<E>.png`, `..._hcaltp_...` | the tower maps (figure 11.2 of the chapter, left and middle columns: signal and mixed) |
| `trigPrim_<case>_11_DIGI_jet<N>_ecaltp_run1_event1.png`, `..._jet<N>_hcaltp_...` | the towers around generator jet N (1 = the b jet, pt 117.8 GeV; 2 = the bbar jet, in the HF); figure 11.2, right column: `trigPrim_signal_11_DIGI_jet1_*` |
| `trigPrim_<case>_11_DIGI.log` | every number: `format:`, `TP <region>:`, `ECAL TP time samples`, `HCAL TP time samples`, `ECAL TT flags`, `EB tower read in full:`, `ECAL SR flags`, `ECAL deposits <region>:`, `GenJet N:`, `top tower N:` |
| `trigprim_11_DIGI.txt` | the three logs side by side (`common/trigPrimLedger.py`): section 4.7 |

Note the numbering: these pages are numbered by *generator* jet (jet 1 = the b jet), the pages of chapter 12 by
*Level-1* jet (jet 1 = the bbar jet, the larger at Level-1).

## 4. The tables

### 4.1 `ledger_11_DIGI.txt` (the output of `common/digiLedger.py`, in full)

```
== digiLedger 11_DIGI
signal: run:lumi:event 1:1:1 vertex=(0.0276598,-0.0169037,3.11193) cm | bx 0: 0 pileup events
pileup: run:lumi:event 1:1:72 vertex=(0.0286612,-0.0165738,-0.482677) cm | bx 0: 0 pileup events
mixed: run:lumi:event 1:1:1 vertex=(0.0276598,-0.0169037,3.11193) cm | bx -1: 71 pileup events | bx 0: 68 pileup events | bx 1: 54 pileup events
subdet         (11a)      (11b)  (11a+11b) | mixed by origin: signal / in-time / early / late / no link ; calorimeters: sum amplitude [ADC] (11a) (11b) (11a+11b)
PXB             3011        819      52101 | 2824 / 47941 / 1336 / 0 / 0
PXF             1789        318      25879 | 1779 / 23634 / 466 / 0 / 0
TIB             5360       2841      91364 | 3077 / 60025 / 25030 / 1530 / 1702
TID             2558       1141      34922 | 1730 / 24228 / 7759 / 692 / 513
TOB             6778       4448     106837 | 2761 / 62356 / 37048 / 1220 / 3452
TEC            16906       7937     203778 | 11594 / 132534 / 52105 / 3587 / 3958
EB               156         43        964 | amplitude 1260 502 8899
EE               384        105       2520 | amplitude 1876 384 12194
ES               981        360      15972 | amplitude 17885 5053 230844
HB              1722       1700       2197 | amplitude 16684 15856 29111
HE              2249       2036       3966 | amplitude 20375 12739 141702
HO                 -          -          2 | amplitude 0 0 63
HF              2270       2169       1889 | amplitude 8234 4167 30729
DT                 -          -         12 | 0 / 1 / 9 / 2 / 0
CSCstrip           -        176       1888 | 0 / 305 / 62 / 36 / 1485
CSCwire            -          3         91 | 0 / 0 / 0 / 0 / 91
RPC                8          8         10 | 0 / 3 / 0 / 0 / 7
GEM                -          -         15 | 0 / 14 / 1 / 0 / 0
total          44172      24104     544407 | 
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
== DigiAnalyzer run:lumi:event 1:1:72 vertex=(0.0286612,-0.0165738,-0.482677) cm | bx 0: 0 pileup events
  PXB: nDigis=819 signal=819 inTime=0 early=0 late=0 noLink=0
  PXF: nDigis=318 signal=318 inTime=0 early=0 late=0 noLink=0
  TIB: nDigis=2841 signal=555 inTime=0 early=0 late=0 noLink=2286
  TID: nDigis=1141 signal=339 inTime=0 early=0 late=0 noLink=802
  TOB: nDigis=4448 signal=541 inTime=0 early=0 late=0 noLink=3907
  TEC: nDigis=7937 signal=2698 inTime=0 early=0 late=0 noLink=5239
  EB: nDigis=43 sumAmplitude[ADC]=501.667
  EE: nDigis=105 sumAmplitude[ADC]=384
  ES: nDigis=360 sumAmplitude[ADC]=5053
  HB: nDigis=1700 sumAmplitude[ADC]=15856
  HE: nDigis=2036 sumAmplitude[ADC]=12738.6
  HF: nDigis=2169 sumAmplitude[ADC]=4167
  CSCstrip: nDigis=176 signal=18 inTime=0 early=0 late=0 noLink=158
  CSCwire: nDigis=3 signal=0 inTime=0 early=0 late=0 noLink=3
  RPC: nDigis=8 signal=0 inTime=0 early=0 late=0 noLink=8
  total digis=24104 genJets(pt>30)=0
```

("signal" in this log means: linked to a particle of library event 72, the only event of the sample.)

(11a+11b) `digiAnalyzer_mixed_11_DIGI.log`:

```
== DigiAnalyzer run:lumi:event 1:1:1 vertex=(0.0276598,-0.0169037,3.11193) cm | bx -1: 71 pileup events | bx 0: 68 pileup events | bx 1: 54 pileup events
  PXB: nDigis=52101 signal=2824 inTime=47941 early=1336 late=0 noLink=0
  PXF: nDigis=25879 signal=1779 inTime=23634 early=466 late=0 noLink=0
  TIB: nDigis=91364 signal=3077 inTime=60025 early=25030 late=1530 noLink=1702
  TID: nDigis=34922 signal=1730 inTime=24228 early=7759 late=692 noLink=513
  TOB: nDigis=106837 signal=2761 inTime=62356 early=37048 late=1220 noLink=3452
  TEC: nDigis=203778 signal=11594 inTime=132534 early=52105 late=3587 noLink=3958
  EB: nDigis=964 sumAmplitude[ADC]=8899
  EE: nDigis=2520 sumAmplitude[ADC]=12194
  ES: nDigis=15972 sumAmplitude[ADC]=230844
  HB: nDigis=2197 sumAmplitude[ADC]=29111.2
  HE: nDigis=3966 sumAmplitude[ADC]=141702
  HO: nDigis=2 sumAmplitude[ADC]=63
  HF: nDigis=1889 sumAmplitude[ADC]=30729
  DT: nDigis=12 signal=0 inTime=1 early=9 late=2 noLink=0
  CSCstrip: nDigis=1888 signal=0 inTime=305 early=62 late=36 noLink=1485
  CSCwire: nDigis=91 signal=0 inTime=0 early=0 late=0 noLink=91
  RPC: nDigis=10 signal=0 inTime=3 early=0 late=0 noLink=7
  GEM: nDigis=15 signal=0 inTime=14 early=1 late=0 noLink=0
  total digis=544407 genJets(pt>30)=2
```

Sums quoted in the chapter (from these logs): (11a) tracker 36 402 digis = 24 218 linked + 12 184 noise (pixels
4800 linked + 0 noise for 1902 PSimHits, strips 19 418 linked + 12 184 noise for 8370 PSimHits); (11b) tracker 17 504
= 5270 linked + 12 234 noise, all linked digis 5288 (5270 + 18 CSC strips), all noise 12 403 (12 234 + 158 + 3 + 8);
(11a+11b) tracker 514 881 = 23 765 signal + 350 718 in-time + 123 744 early + 7029 late + 9625 noise; pixels 77 980
with 1802 early and 0 late; strips 436 901 with 121 942 early (27.9 %); the signal lost 24 218 - 23 765 = 453 digis;
350 718 / 68 = 5158 in-time digis per collision; muon 2016 digis. Amplitude ratios (11a+11b)/(11a): EB 8899/1260 =
7.1, EE 12194/1876 = 6.5, ES 230844/17885 = 12.9, HB 29111/16684 = 1.7, HE 141702/20375 = 7.0, HF 30729/8234 = 3.7.

### 4.3 `thresholds_11_DIGI.txt`: the lines quoted in table 11.1 of the chapter

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
[mix (bunch crossings)]       bunchspace 25  minBunch -1  maxBunch 1  input.nbPileupEvents.averageNumber 60.0  input.type poisson
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

### 4.4 The SimHits that went in (chapter 10 files)

Signal, `test/10_SIM/out/simHits_signal_10_SIM.log` and `simAnalyzer_signal_10_SIM.log`: 1926 SimTracks, 571
primaries; TRK 10 272 PSimHits (PXB 929, PXF 973, TIB 1182, TID 975, TOB 1022, TEC 5191; `sumELoss = 2.33358` GeV);
ECAL 24 262 PCaloHits in 12 804 cells (EB 5521, EE 6214, ES 1069; `E by subdetector: EB=67.989 EE=293.66 ES=0.245266`
GeV); HCAL 2531 PCaloHits in 1353 cells (HB 173, HE 648, HO 1, HF 531; `HB=0.217328 HE=1.66239 HO=3.73269e-06
HF=1849`, the HF in photoelectrons of the shower library); MUON 0.

The bunch crossing, `test/10_SIM/out/step3_MIX_10_SIM.log`:

```
  pileup mixed in (CrossingFrame, bunch crossings -1 .. 1): 193 events (early 71, in-time 68, late 54), 31060 primaries (10012, 10414, 10634), 97276 SimTracks; events per bx: -1:71 0:68 1:54
  station TRK: 448315 PSimHits, 0 PCaloHits in 0 cells (signal: 10272 PSimHits, 0 PCaloHits)
    hits by subdetector and layer/station: PXB/1=19053 PXB/2=13201 PXB/3=10078 PXB/4=8621 PXF/1=11399 PXF/2=12317 PXF/3=12984 TIB/1=29220 TIB/2=23887 TIB/3=10109 TIB/4=8725 TID/1=11910 TID/2=12594 TID/3=13393 TOB/1=23175 TOB/2=20098 TOB/3=8412 TOB/4=6786 TOB/5=5771 TOB/6=4898 TEC/1=22851 TEC/2=24335 TEC/3=25839 TEC/4=19769 TEC/5=20618 TEC/6=21515 TEC/7=15890 TEC/8=16975 TEC/9=13892
    hits by kind of the primary: charged hadron=4999 neutral hadron=890 photon=2013 e^{#pm}=138 no stored ancestor=2232 pileup, early (bx < 0)=139551 pileup, in-time (bx 0)=146028 pileup, late (bx > 0)=152464
```

Per subdetector (the layers summed): PXB 50 953, PXF 36 700, TIB 71 941, TID 37 897, TOB 69 140, TEC 181 684 = 448 315.

### 4.5 FWLite extras

Read from the ROOT files with `python3` and `DataFormats.FWLite` (`Events`, `Handle`), after `cmsenv`. The scripts
are short enough to be retyped; they are the source of the library-event-72 numbers, of the HCAL cell counts and of
the zero-suppression and truth counts of the chapter.

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
== event 1:1:72 of step2_SIM_pileup_10_SIM.root
  PXB  PSimHits=277 (low 276, high 1) modules=155 sumELoss=0.0686 GeV
  PXF  PSimHits=176 (low 173, high 3) modules=123 sumELoss=0.0221 GeV
  TIB  PSimHits=187 (low 185, high 2) modules=135 sumELoss=0.0497 GeV
  TID  PSimHits=172 (low 171, high 1) modules=130 sumELoss=0.0252 GeV
  TOB  PSimHits=209 (low 205, high 4) modules=142 sumELoss=0.0802 GeV
  TEC  PSimHits=1076 (low 1065, high 11) modules=803 sumELoss=0.2895 GeV
  tracker total PSimHits=2097
  MuonDTHits   PSimHits=0
  MuonCSCHits  PSimHits=11
  MuonRPCHits  PSimHits=0
  MuonGEMHits  PSimHits=0
  EcalHitsEB PCaloHits=1443 cells=1217 E=9.4494 GeV cells>0.5MeV=729
  EcalHitsEE PCaloHits=3069 cells=1948 E=43.4787 GeV cells>0.5MeV=1545
  EcalHitsES PCaloHits=230 cells=223 E=0.0463 GeV cells>0.5MeV=17
  HB   PCaloHits=43 cells(det,z,depth,ieta,iphi)=21 E=0.0175 GeV
  HE   PCaloHits=209 cells(det,z,depth,ieta,iphi)=101 E=0.2230 GeV
  HO   PCaloHits=0 cells(det,z,depth,ieta,iphi)=0 E=0.0000 GeV
  HF   PCaloHits=140 cells(det,z,depth,ieta,iphi)=119 E=173.0000 units (photoelectrons of the shower library)
```

The 164 primaries of library event 72 are from `test/09_G4/out/ledger_09_G4.txt` (chapter 9).

The DIGI files (`addPileupInfo`, the selective-readout flags `simEcalDigis:ebSrFlags/eeSrFlags` by value, the
`simHcalUnsuppressedDigis` against `simHcalDigis` sizes, the `mix:MergedTrackTruth` split by `EncodedEventId`; the
script was retyped for the 2025 run, and on the 2024 files it reproduces the 2024 numbers quoted in section 4.9):

```
== signal run 1 event 1
  bx 0: PU_NumInteractions=0 TrueNumInteractions=0.0
  EB SR flags (value: n of the 2448 trigger towers): [(1, 2444), (3, 4)]; EE SR flags (n of the 624 supercrystals): [(1, 608), (3, 16)]
  HCAL unsuppressed -> zero suppressed: HBHE(QIE11) 15840 -> 4459, HF(QIE10) 3456 -> 3456, HO 2160 -> 0
  TrackingParticles (mix:MergedTrackTruth): 999 (signal 999, in-time pileup 0, out-of-time 0)
== pileup run 1 event 72
  bx 0: PU_NumInteractions=0 TrueNumInteractions=0.0
  EB SR flags (value: n of the 2448 trigger towers): [(1, 2448)]; EE SR flags (n of the 624 supercrystals): [(1, 621), (3, 3)]
  HCAL unsuppressed -> zero suppressed: HBHE(QIE11) 15840 -> 4310, HF(QIE10) 3456 -> 3456, HO 2160 -> 0
  TrackingParticles (mix:MergedTrackTruth): 237 (signal 237, in-time pileup 0, out-of-time 0)
== mixed run 1 event 1
  bx -1: PU_NumInteractions=71 TrueNumInteractions=71.0
  bx 0: PU_NumInteractions=68 TrueNumInteractions=68.0
  bx 1: PU_NumInteractions=54 TrueNumInteractions=54.0
  EB SR flags (value: n of the 2448 trigger towers): [(1, 2425), (3, 23)]; EE SR flags (n of the 624 supercrystals): [(1, 524), (3, 100)]
  HCAL unsuppressed -> zero suppressed: HBHE(QIE11) 15840 -> 6545, HF(QIE10) 3456 -> 3456, HO 2160 -> 2
  TrackingParticles (mix:MergedTrackTruth): 45282 (signal 999, in-time pileup 14729, out-of-time 29554)
```

Flag value 1 is "low interest" (zero suppression applied), 3 "high interest" (full readout); the channel totals
(EB 61 200 crystals, EE 14 648, ES 137 216, HB 9072, HE 6768, HO 2160, HF 3456) are the detector elements listed by
the SimHitAnalyzer of chapter 10 (`simHits_signal_10_SIM.log`, first line) and the sizes of the unsuppressed HCAL
collections.

### 4.6 Files of the release quoted in "Open problems for Phase-2"

All under `/cvmfs/cms.cern.ch/el9_amd64_gcc12/cms/cmssw/CMSSW_15_0_5/src`:

| topic | file |
|---|---|
| Phase-2 era | `Configuration/Eras/python/Era_Phase2C17I13M9_cff.py` (-> `Phase2C11I13M9`, `Phase2C11I13`, `Phase2C11`, `Phase2C9`, `Phase2`), `Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py` (`Run4D110`: era `Phase2C17I13M9`) |
| digitiser set | `SimGeneral/MixingModule/python/digitizers_cfi.py` (`phase2_tracker`: `strip = None`; `phase2_hgcal`: HGCal digitisers; `phase2_timing_layer`: `fastTimingLayer = mtdDigitizer`), `SimGeneral/MixingModule/python/pixelDigitizer_cfi.py` (`phase2_tracker.toReplaceWith(pixelDigitizer, phase2TrackerDigitizer)`) |
| Phase-2 tracker | `SimTracker/SiPhase2Digitizer/python/phase2TrackerDigitizer_cfi.py`, `customizeForOTInefficiency.py`, `customizeForPhase2TrackerSignalShape.py` |
| Run-3 pixel | `SimGeneral/MixingModule/python/SiPixelSimParameters_cfi.py` (`_modifyPixelDigitizerForRun3`, `phase1Pixel`, `premix_stage1`) |
| Run-3 strip | `SimGeneral/MixingModule/python/SiStripSimParameters_cfi.py`, `RecoLocalTracker/SiStripZeroSuppression/src/SiStripFedZeroSuppression.cc` (algorithm 4) |
| ECAL | `SimGeneral/MixingModule/python/ecalDigitizer_cfi.py`, `ecalDigitizer_Ph2_cfi.py`, `SimCalorimetry/EcalSimProducers/python/ecalDigiParameters_cff.py`, `ecalDigiParameters_Ph2_cff.py`, `ecalSimParameterMap_cff.py`, `SimCalorimetry/EcalSelectiveReadoutProducers/python/ecalDigis_cfi.py`, `SimCalorimetry/EcalZeroSuppressionProducers/python/ecalPreshowerDigis_cfi.py` |
| HCAL | `SimCalorimetry/HcalSimProducers/python/hcalUnsuppressedDigis_cfi.py`, `hcalSimParameters_cfi.py`, `SimCalorimetry/HcalZeroSuppressionProducers/python/hcalDigisRealistic_cfi.py`, `CalibCalorimetry/HcalPlugins/python/HBHEDarkening_cff.py` |
| HGCal | `SimCalorimetry/HGCalSimProducers/python/hgcalDigitizer_cfi.py`, `hgcROCParameters_cfi.py` |
| MTD | `SimFastTiming/FastTimingCommon/python/mtdDigitizer_cfi.py` |
| muon | `SimMuon/DTDigitizer/python/muonDTDigis_cfi.py`, `SimMuon/CSCDigitizer/python/muonCSCDigis_cfi.py`, `SimMuon/RPCDigitizer/python/muonRPCDigis_cfi.py`, `SimMuon/GEMDigitizer/python/muonGEMDigis_cfi.py`, `muonME0Digis_cfi.py`, `SimMuon/GEMDigitizer/plugins/GEMDigiProducer.cc` (`fillDescriptions`), `SimMuon/Configuration/python/SimMuon_cff.py` |
| ageing | `SLHCUpgradeSimulations/Configuration/python/aging.py` (`customise_aging_300/1000/3000`, `ageHcal`, `ageEcal`, `agedHGCal`, `ageSiPM`) |
| mixing, premixing | `Configuration/StandardSequences/python/Mixing.py`, `SimGeneral/MixingModule/python/mix_POISSON_average_cfi.py`, `mix_2024_25ns_RunIII2024Summer24_PoissonOOTPU_cfi.py`, `mix_probFunction_25ns_PoissonOOTPU_cfi.py`, `Configuration/PyReleaseValidation/python/relval_steps.py` (`'Run4'`: `AVE_200_BX_25ns`), `Configuration/StandardSequences/python/DataMixerPreMix_cff.py`, `Mixing/Base/src/BMixingModule.cc`, `Mixing/Base/src/PileUp.cc`, `SimGeneral/MixingModule/plugins/MixingModule.cc` |

### 4.7 The trigger primitives: `trigprim_11_DIGI.txt` (the output of `common/trigPrimLedger.py`, in full)

The numbers of the section "Made here for the next stage: the trigger primitives" of the chapter (table 11.2 is
section 1 of the file).

```
== trigPrimLedger 11_DIGI
(11a)      signal: run:lumi:event 1:1:1
(11b)      pileup: run:lumi:event 1:1:72
(11a+11b)  mixed: run:lumi:event 1:1:1
format: ECAL samples per primitive=5 sampleOfInterest=2 | HCAL (HB, HE) samples per primitive=4 presamples=2

-- 1. The trigger primitives: primitives with ET > 0 / sum ET [GeV] / largest ET [GeV]
                                            (11a) signal                  (11b) pileup               (11a+11b) mixed
ECAL, EB (of 2448)                     368 / 215.0 / 3.0             340 / 185.0 / 1.5             683 / 468.0 / 5.5
ECAL, EE (of 1584)                     131 / 113.5 / 6.0              105 / 69.0 / 3.0             494 / 579.5 / 7.0
ECAL, all (of 4032)                    499 / 328.5 / 6.0             445 / 254.0 / 3.0           1177 / 1047.5 / 7.0
HCAL, HB (of 2304)                       69 / 35.5 / 1.5               51 / 26.0 / 1.0             233 / 183.0 / 7.5
HCAL, HE (of 1728)                      12 / 24.0 / 10.0                 0 / 0.0 / 0.0            328 / 241.5 / 10.0
HCAL, HF (of 792)                      40 / 148.5 / 89.0                 4 / 2.0 / 0.5           741 / 1321.0 / 90.5
HCAL, all (of 4824)                   121 / 208.0 / 89.0               55 / 28.0 / 1.0          1302 / 1745.5 / 90.5
all primitives (of 8856)                     620 / 536.5                   500 / 282.0                 2479 / 2793.0

-- 2. One primitive per bunch crossing: sum ET [GeV] of the time samples stored with every primitive
      (25 ns per sample; [ ] is the sample of interest, the bunch crossing of the event)
(11a)      signal
   ECAL     bx -2..+2:    426.5    268.5 [   328.5]      274      280
   HB + HE  bx -2..+1:       35       26 [    59.5]     32.5
   HF       bx -1.. 0:                 0 [   148.5]
(11b)      pileup
   ECAL     bx -2..+2:    443.5    243.5 [     254]    247.5      245
   HB + HE  bx -2..+1:     28.5       27 [      26]       32
   HF       bx -1.. 0:                 0 [       2]
(11a+11b)  mixed
   ECAL     bx -2..+2:    517.5      826 [  1047.5]    575.5    190.5
   HB + HE  bx -2..+1:       34    255.5 [   424.5]      350
   HF       bx -1.. 0:              1124 [    1321]

-- 3. What the ECAL primitives do inside the DIGI step: the trigger-tower flag of every primitive (low / mid / high
      interest) and the selective-readout decision of the ECAL digitisation that follows from it
                                                       (11a)           (11b)       (11a+11b)
EB towers flagged mid interest                             4               0              14
EB towers flagged high interest                            0               0               1
EB towers read in full (of 2448)                           4               0              23
   with a high flag themselves                             0               0               1
   others (mid flag, or next to a high one)                4               0              22
EE towers flagged mid interest                             3               3              76
EE towers flagged high interest                            3               0              13
EE supercrystals read in full (of 624)                    16               3             100
(11a)      EB tower read in full: ieta=-17 iphi=46 ecalTP=3 ttFlag=1
(11a)      EB tower read in full: ieta=3 iphi=17 ecalTP=2 ttFlag=1
(11a)      EB tower read in full: ieta=11 iphi=27 ecalTP=3 ttFlag=1
(11a)      EB tower read in full: ieta=12 iphi=31 ecalTP=2 ttFlag=1
(11a+11b)  EB tower read in full: ieta=-3 iphi=3 ecalTP=1 ttFlag=0
(11a+11b)  EB tower read in full: ieta=-3 iphi=4 ecalTP=0.5 ttFlag=0
(11a+11b)  EB tower read in full: ieta=-3 iphi=5 ecalTP=0 ttFlag=0
(11a+11b)  EB tower read in full: ieta=-3 iphi=55 ecalTP=2 ttFlag=1
(11a+11b)  EB tower read in full: ieta=-4 iphi=3 ecalTP=0 ttFlag=0
(11a+11b)  EB tower read in full: ieta=-4 iphi=4 ecalTP=5.5 ttFlag=3
(11a+11b)  EB tower read in full: ieta=-4 iphi=5 ecalTP=0 ttFlag=0
(11a+11b)  EB tower read in full: ieta=-5 iphi=3 ecalTP=1 ttFlag=0
(11a+11b)  EB tower read in full: ieta=-5 iphi=4 ecalTP=0 ttFlag=0
(11a+11b)  EB tower read in full: ieta=-5 iphi=5 ecalTP=0.5 ttFlag=0
(11a+11b)  EB tower read in full: ieta=-9 iphi=41 ecalTP=2.5 ttFlag=1
(11a+11b)  EB tower read in full: ieta=-9 iphi=42 ecalTP=2.5 ttFlag=1
(11a+11b)  EB tower read in full: ieta=-10 iphi=30 ecalTP=2 ttFlag=1
(11a+11b)  EB tower read in full: ieta=-10 iphi=32 ecalTP=2 ttFlag=1
(11a+11b)  EB tower read in full: ieta=-13 iphi=67 ecalTP=2 ttFlag=1
(11a+11b)  EB tower read in full: ieta=-17 iphi=46 ecalTP=2 ttFlag=1
(11a+11b)  EB tower read in full: ieta=4 iphi=8 ecalTP=2 ttFlag=1
(11a+11b)  EB tower read in full: ieta=6 iphi=11 ecalTP=2 ttFlag=1
(11a+11b)  EB tower read in full: ieta=7 iphi=38 ecalTP=3 ttFlag=1
(11a+11b)  EB tower read in full: ieta=11 iphi=27 ecalTP=3.5 ttFlag=1
(11a+11b)  EB tower read in full: ieta=13 iphi=47 ecalTP=2 ttFlag=1
(11a+11b)  EB tower read in full: ieta=14 iphi=25 ecalTP=2.5 ttFlag=1
(11a+11b)  EB tower read in full: ieta=17 iphi=49 ecalTP=2 ttFlag=1

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
   ECAL deposits: 1195 towers, 14.6 GeV | towers with a deposit >= 0.25 GeV and a primitive: 2 (deposit 0.6 GeV, primitives 1.0 GeV)
        primitives without a deposit >= 0.25 GeV: 443 (253.0 GeV) | deposits >= 0.25 GeV without a primitive: 2 (0.6 GeV) | deposits below 0.25 GeV per tower: 13.4 GeV
   EB   deposits: 557 towers, 6.0 GeV | towers with a deposit >= 0.25 GeV and a primitive: 2 (deposit 0.6 GeV, primitives 1.0 GeV)
        primitives without a deposit >= 0.25 GeV: 338 (184.0 GeV) | deposits >= 0.25 GeV without a primitive: 0 (0.0 GeV) | deposits below 0.25 GeV per tower: 5.4 GeV
   EE   deposits: 638 towers, 8.6 GeV | towers with a deposit >= 0.25 GeV and a primitive: 0 (deposit 0.0 GeV, primitives 0.0 GeV)
        primitives without a deposit >= 0.25 GeV: 105 (69.0 GeV) | deposits >= 0.25 GeV without a primitive: 2 (0.6 GeV) | deposits below 0.25 GeV per tower: 8.0 GeV
(11a+11b)  mixed
   ECAL deposits: 2799 towers, 120.7 GeV | towers with a deposit >= 0.25 GeV and a primitive: 41 (deposit 54.0 GeV, primitives 72.5 GeV)
        primitives without a deposit >= 0.25 GeV: 1136 (975.0 GeV) | deposits >= 0.25 GeV without a primitive: 32 (14.1 GeV) | deposits below 0.25 GeV per tower: 52.6 GeV
   EB   deposits: 1573 towers, 43.7 GeV | towers with a deposit >= 0.25 GeV and a primitive: 17 (deposit 16.2 GeV, primitives 20.0 GeV)
        primitives without a deposit >= 0.25 GeV: 666 (448.0 GeV) | deposits >= 0.25 GeV without a primitive: 18 (7.6 GeV) | deposits below 0.25 GeV per tower: 19.9 GeV
   EE   deposits: 1226 towers, 77.0 GeV | towers with a deposit >= 0.25 GeV and a primitive: 24 (deposit 37.8 GeV, primitives 52.5 GeV)
        primitives without a deposit >= 0.25 GeV: 470 (527.0 GeV) | deposits >= 0.25 GeV without a primitive: 14 (6.5 GeV) | deposits below 0.25 GeV per tower: 32.7 GeV

-- 5. The two generator jets in the primitives: the 9 x 9 towers around the jet axis, sum ET [GeV]
(11a)      GenJet 1: pt=117.788 eta=-1.58908 phi=-2.08403 towerIeta=-19 towerIphi=49 | 9x9 towers around it: ecalTP=29 (12 towers) hcalTP=22.5 (9 primitives) ecalDepositEt=41.6009 | hcal samples of the window (HB/HE: index 2 = sample of interest; HF: index 1): 0 0 22.5 0
(11a)      GenJet 2: pt=115.944 eta=-3.74635 phi=1.00816 towerIeta=-34 towerIphi=12 | 9x9 towers around it: ecalTP=0 (0 towers) hcalTP=123.75 (11 primitives) ecalDepositEt=0 | hcal samples of the window (HB/HE: index 2 = sample of interest; HF: index 1): 0 123.75 0 0
(11a+11b)  GenJet 1: pt=117.788 eta=-1.58908 phi=-2.08403 towerIeta=-19 towerIphi=49 | 9x9 towers around it: ecalTP=48 (31 towers) hcalTP=30.5 (17 primitives) ecalDepositEt=41.6009 | hcal samples of the window (HB/HE: index 2 = sample of interest; HF: index 1): 0 0.5 30.5 1.5
(11a+11b)  GenJet 2: pt=115.944 eta=-3.74635 phi=1.00816 towerIeta=-34 towerIphi=12 | 9x9 towers around it: ecalTP=0 (0 towers) hcalTP=199.75 (45 primitives) ecalDepositEt=0 | hcal samples of the window (HB/HE: index 2 = sample of interest; HF: index 1): 50.75 199.75 0 0

-- 6. The largest primitives (ECAL + HCAL of the tower)
(11a)      top tower 0: ieta=-34 iphi=11 ecalTP=0 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=89 hcalSamples=0,89,0,0
(11a)      top tower 1: ieta=-18 iphi=50 ecalTP=3 ttFlag=1 fineGrain=0 ecalDepositEt=3.44173 hcalTP=10 hcalSamples=0,0,10,0
(11a)      top tower 2: ieta=-33 iphi=11 ecalTP=0 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=12.5 hcalSamples=0,12.5,0,0
(11a)      top tower 3: ieta=-19 iphi=48 ecalTP=6 ttFlag=3 fineGrain=0 ecalDepositEt=6.57903 hcalTP=3.5 hcalSamples=0,0,3.5,0
(11a)      top tower 4: ieta=-33 iphi=13 ecalTP=0 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=9.5 hcalSamples=0,9.5,0,0
(11a)      top tower 5: ieta=-34 iphi=13 ecalTP=0 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=7 hcalSamples=0,7,0,0
(11b)      top tower 0: ieta=26 iphi=3 ecalTP=3 ttFlag=1 fineGrain=0 ecalDepositEt=0 hcalTP=0 hcalSamples=0,0,0,0
(11b)      top tower 1: ieta=2 iphi=49 ecalTP=1 ttFlag=0 fineGrain=0 ecalDepositEt=0.000319419 hcalTP=1 hcalSamples=0,0.5,1,0
(11b)      top tower 2: ieta=27 iphi=5 ecalTP=2 ttFlag=1 fineGrain=0 ecalDepositEt=0 hcalTP=0 hcalSamples=0,0,0,0
(11b)      top tower 3: ieta=27 iphi=6 ecalTP=2 ttFlag=1 fineGrain=0 ecalDepositEt=0 hcalTP=0 hcalSamples=0,0,0,0
(11b)      top tower 4: ieta=26 iphi=15 ecalTP=1.5 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=0 hcalSamples=0,0,0,0
(11b)      top tower 5: ieta=2 iphi=23 ecalTP=1 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=0.5 hcalSamples=0,0,0.5,0
(11a+11b)  top tower 0: ieta=-34 iphi=11 ecalTP=0 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=90.5 hcalSamples=1,90.5,0,0
(11a+11b)  top tower 1: ieta=-33 iphi=11 ecalTP=0 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=15.5 hcalSamples=0,15.5,0,0
(11a+11b)  top tower 2: ieta=-18 iphi=50 ecalTP=3.5 ttFlag=1 fineGrain=0 ecalDepositEt=3.44173 hcalTP=10 hcalSamples=0,0,10,0
(11a+11b)  top tower 3: ieta=-33 iphi=13 ecalTP=0 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=12 hcalSamples=0.5,12,0,0
(11a+11b)  top tower 4: ieta=40 iphi=19 ecalTP=0 ttFlag=0 fineGrain=0 ecalDepositEt=0 hcalTP=11.5 hcalSamples=2.5,11.5,0,0
(11a+11b)  top tower 5: ieta=-19 iphi=48 ecalTP=7 ttFlag=3 fineGrain=0 ecalDepositEt=6.57903 hcalTP=3.5 hcalSamples=0,0,3.5,0
```

How the chapter reads it.

- Section 1 is table 11.2. The mixed column is also the first table of chapter 12 (its step 1).
- Section 2, "one primitive per bunch crossing": the ECAL sample sums of the signal alone are 426.5, 268.5, [328.5],
  274.0, 280.0 GeV: about 270 GeV of noise per crossing (the first stored sample is higher, as in 2024) and some 55 GeV
  more in ours.
- Section 3: the thresholds of the flags are 2 GeV (mid) and 4 GeV (high), from the payload of `EcalTPGPhysicsConstRcd`,
  `EcalTPGPhysicsConst_2025_mc_EB_4.0_2.0_EE_4.0_2.0` (`conddb list 150X_mcRun3_2025_realistic_v1 | grep EcalTPG`
  lists the 19 `EcalTPG*Rcd` payloads; `conddb dump` of this one gives `ttf-threshold-Low 2`, `ttf-threshold-High 4`,
  the same content as the `2018_mc` payload of the 2024 global tag). Mixed barrel: 14 mid + 1 high towers; a mid tower
  is read in full alone, a high one with its 8 neighbours: 14 + 9 = 23 towers, the 23 `EB tower read in full` lines
  (the eight with `ttFlag=0` are the neighbours of (-4, 4)). The signal alone: 4 mid barrel towers, read in full, and
  in the endcap 3 mid + 3 high towers, 16 supercrystals read in full.
- Section 4, signal alone: 120.7 GeV of ET deposited in 2799 ECAL towers; 32 towers with at least 0.25 GeV and a
  primitive (deposit 49.8 GeV, primitives 48.0 GeV); 41 towers with at least 0.25 GeV (18.3 GeV) and no primitive;
  52.6 GeV in towers below 0.25 GeV; 467 of the 499 primitives (280.5 GeV) without such a deposit. 49.8 + 18.3 + 52.6 =
  120.7. Library event 72: 14.6 GeV deposited, 4 towers with 0.25 GeV, 445 primitives (254.0 GeV): 445 / 4032 = one
  tower in 9.1.
- Section 5, the 9 x 9 towers around the jet axes: the b jet 29.0 (ECAL) + 22.5 (HCAL) = 51.5 GeV alone, 48.0 + 30.5 =
  78.5 GeV with the pileup (+27.0); the bbar jet 123.75 and 199.75 GeV (+76.0; the quarter GeV: an HF primitive is two
  towers wide in phi, and one that lies half inside the window is counted half, `TrigPrimAnalyzer.cc`). Chapter 12 centres its window on
  the seed tower of the Level-1 jet, (-18, 50) instead of the jet-axis tower (-19, 49), and finds 46.0 + 28.0 = 74.0
  GeV for the b jet: the two windows differ by one tower in each direction.

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
the chapter (11 November 2026) ran with `auto:phase1_2024_realistic` = `150X_mcRun3_2024_realistic_v2` and `Run3_2024`;
its outputs were kept for the comparison below.

What did not change.

| compared | result |
|---|---|
| the configuration | `process.dumpPython()` of `step3_DIGI_mixed_11_DIGI_cfg.py` with each era: 19 114 lines, identical except the name of the global tag. None of the five modifiers that `Run3_2025` adds to `Run3_2024` (`run3_GEM_2025`, `run3_CSC_2025`, `run3_SiPixel_2025`, `stage2L1Trigger_2025`, `ecal_cctiming`) is used by a module of the DIGI step |
| `thresholds_11_DIGI.txt` | identical (it lists the python; the database thresholds are not in it) |
| the input | the SimHits of chapter 10 are identical with both settings (chapter 10 companion, section 4.7): section 4.4 is unchanged |
| the mixing | `addPileupInfo` 71 / 68 / 54, the same library events (playback of the same `CrossingFramePlaybackInfoNew`) |
| the truth | `mix:MergedTrackTruth` 999 / 237 / 45 282 (in-time 14 729, out-of-time 29 554) |
| the muon digis | identical counts and split in all three samples (mixed: DT 12, CSC 1888 strips + 91 wires, RPC 10, GEM 15); the ES frames of (11a) and (11b) |
| the ECAL deposits behind the primitives | `trigprim_11_DIGI.txt` section 4: 120.7 GeV in 2799 towers (signal), 14.6 GeV in 1195 towers (event 72) |

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

The numbers, 2024 -> 2025 (the 2024 values from the outputs of 11 November 2026; (11a) / (11b) / (11a+11b)):

| quantity | 2024 | 2025 | why |
|---|---|---|---|
| pixel digis linked (11a), for 1902 PSimHits | 4982 | 4800 | smaller Lorentz angle: less charge sharing between pixels (the reweighting templates changed too; the share of each not established) |
| pixel noise digis (11a) | 7 | 0 | the same noise configuration; a different random draw (below) |
| strip digis linked / noise (11a) | 19 513 / 12 256 | 19 418 / 12 184 | bad components; random noise |
| tracker digis (11a) / (11b) / (11a+11b) | 36 758 / 17 684 / 520 289 | 36 402 / 17 504 / 514 881 | |
| mixed tracker: signal / in-time / early / late / noise | 23 971 / 355 086 / 124 603 / 6966 / 9663 | 23 765 / 350 718 / 123 744 / 7029 / 9625 | |
| EB frames | 399 / 285 / 2050 | 156 / 43 / 964 | barrel zero suppression 0.2275 -> 0.3325 GeV (+46 %), against crystal noise +16 % and an APD/PN ratio -9 % (less light) |
| EE frames | 372 / 55 / 4144 | 384 / 105 / 2520 | EE zero suppression unchanged; the mixed event has 100 instead of 175 supercrystals read in full |
| EB towers / EE supercrystals read in full | 2 / 0 / 23 and 15 / 1 / 175 | 4 / 0 / 23 and 16 / 3 / 100 | follow the primitives (flags 2 and 4 GeV) |
| ES frames | 981 / 360 / 15 907 | 981 / 360 / 15 972 | ES payloads unchanged; noise draw |
| HB / HE frames kept by the zero suppression (HBHE) | 3285 / 2972 / 5839 | 4459 / 4310 / 6545 | SiPM dark current x1.9 (HB) and x2.9 (HE), zero-suppression levels x1.3 |
| HF frames with a positive amplitude | 2251 / 2115 / 1889 | 2270 / 2169 / 1889 | HF payloads unchanged (pedestal 14.9 fC, width 2.97 fC in both); noise draw |
| all digis | 43 762 / 23 363 / 551 889 | 44 172 / 24 104 / 544 407 | |
| ECAL primitives EB (> 0, sum ET) | 200, 114.5 / 157, 83.5 / 594, 408.0 GeV | 368, 215.0 / 340, 185.0 / 683, 468.0 GeV | noisier barrel crystals; the TPG payloads changed too (the share not established) |
| ECAL primitives EE | 98, 85.5 / 72, 44.0 / 594, 1273.5 GeV | 131, 113.5 / 105, 69.0 / 494, 579.5 GeV | (11a+11b): the pileup of 2.5 < abs(eta) < 3 in darker crystals (APD/PN 0.041 -> 0.013), not compensated by the linearisation |
| HCAL primitives HB / HE / HF (11a+11b) | 153, 129.5 / 335, 271.5 / 741, 1329.0 GeV | 233, 183.0 / 328, 241.5 / 741, 1321.0 GeV | HB: dark current |
| all primitives | 363, 380.5 / 244, 135.0 / 2417, 3411.5 GeV | 620, 536.5 / 500, 282.0 / 2479, 2793.0 GeV | |
| signal towers with a deposit >= 0.25 GeV and a primitive | 33 (deposit 51.0, primitives 50.0 GeV) | 32 (deposit 49.8, primitives 48.0 GeV) | less light per GeV |
| b-jet window (axis tower (-19, 49)), ECAL + HCAL | 32.0 + 23.5 (11a), 48.0 + 27.5 (11a+11b) | 29.0 + 22.5 (11a), 48.0 + 30.5 (11a+11b) | |
| bbar-jet window | 124.0 (11a), 200.5 (11a+11b) | 123.75 (11a), 199.75 (11a+11b) | |
| largest primitive | HF (-34, 11): 89.0 GeV (11a and 11a+11b) | 89.0 (11a), 90.5 GeV (11a+11b) | |

"A different random draw": every digitiser inside the `MixingModule` draws from the one engine of `mix` (seed 12345;
`RandomNumberGeneratorService` has no engine of its own for them). A payload that changes how many random numbers one
digitiser draws (a different Lorentz angle, more dark-current photoelectrons) shifts the sequence for all the others,
so the noise of channels whose payloads did not change (the pixel noise, the ES, the HF) is a different draw. Such
differences are fluctuations, not effects of the 2025 conditions.
