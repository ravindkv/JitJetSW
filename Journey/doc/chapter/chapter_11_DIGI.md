# Chapter 11, DIGI: technical companion

The digitisation of the SimHits of chapter 10, for the signal alone (11a), for one pileup collision alone (11b) and
for the signal with its pileup mixed in (11a+11b). Script: `test/11_DIGI/run_11_DIGI.sh`; everything it produces is
in `test/11_DIGI/out/`. The chapter text is `doc/chapter/chapter_11_DIGI.tex`. All commands below are run from
`test/` of an initialized CMSSW_15_0_5 (`cmsenv`), with `J=$CMSSW_BASE/src/JitJetSW/Journey`.

## 1. The cmsDriver.py commands

The script runs `cmsDriver.py` three times through `run_driver` of `test/common/jitjet_common.sh`. The commands,
copied from the header of the generated configurations (`test/11_DIGI/step3_DIGI_<case>_11_DIGI_cfg.py`, "with
command line options"; the cmsDriver output is in `out/step3_DIGI_<case>_11_DIGI_cfg.driver.log`):

```bash
cd $J/test
# (11a) the signal alone
cmsDriver.py step3 --filein file:$J/test/10_SIM/out/step2_SIM_signal_10_SIM.root \
  --conditions auto:phase1_2024_realistic --era Run3_2024 --geometry DB:Extended \
  -n -1 -s DIGI:pdigi_valid --datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG --fileout file:step3_DIGI_signal_11_DIGI.root \
  --customise_commands "process.source.eventsToProcess = cms.untracked.VEventRange('1:1:1')" \
  --python_filename $J/test/11_DIGI/step3_DIGI_signal_11_DIGI_cfg.py --no_exec

# (11b) library event 72 alone
cmsDriver.py step3 --filein file:$J/test/10_SIM/out/step2_SIM_pileup_10_SIM.root \
  --conditions auto:phase1_2024_realistic --era Run3_2024 --geometry DB:Extended \
  -n -1 -s DIGI:pdigi_valid --datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG --fileout file:step3_DIGI_pileup_11_DIGI.root \
  --customise_commands "process.source.eventsToProcess = cms.untracked.VEventRange('1:1:72')" \
  --python_filename $J/test/11_DIGI/step3_DIGI_pileup_11_DIGI_cfg.py --no_exec

# (11a+11b) the signal with PU = 60 per bunch crossing in bx -1..1, the collisions of the chapter 10 mixing (playback)
cmsDriver.py step3 --filein file:$J/test/10_SIM/out/step3_MIX_10_SIM.root \
  --pileup "AVE_60_BX_25ns,{'N': 60, 'B': (-1, 1)}" --pileup_input file:$J/test/10_SIM/out/step2_SIM_pileup_10_SIM.root \
  --conditions auto:phase1_2024_realistic --era Run3_2024 --geometry DB:Extended \
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
| `--conditions auto:phase1_2024_realistic` | the global tag of the book, `150X_mcRun3_2024_realistic_v2` (`Configuration/AlCa/python/autoCond.py`): the pedestals, noise, gains, thresholds and dead channels that the digitisers read from the database |
| `--era Run3_2024` | the era modifiers of the 2024 detector: they set the Run-3 pixel thresholds (`run3_common`), the HB SiPM parameters (`run3_HB`), the 10-time-slice HB/HE readout and the one-slice zero suppression (`run3_common`), the GEM luminosity, the Run-2 CSC timing offsets (`run2_common`) |
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
cmsRun ../step3_DIGI_signal_11_DIGI_cfg.py > step3_DIGI_signal_11_DIGI.log 2>&1     # 18 s
cmsRun ../step3_DIGI_pileup_11_DIGI_cfg.py > step3_DIGI_pileup_11_DIGI.log 2>&1     # 18 s
cmsRun ../step3_DIGI_mixed_11_DIGI_cfg.py  > step3_DIGI_mixed_11_DIGI.log  2>&1     # 25 s (193 library events read)
```

Or the whole chapter: `./11_DIGI/run_11_DIGI.sh` (variables: `CASES="signal pileup mixed"`, `REUSE=1` keeps existing
DIGI files; `PU=60`, `BUNCHES=-1,1`, `SKIP=71`, `NPU=68` are set in `test/common/jitjet_common.sh` for the whole book;
`RUN=0` only writes the configurations, `ANALYZE=0` skips the analyzers, `PNG=0` skips the PNG pages). The three
DIGI jobs run in parallel; the times are in `run_11_DIGI.log`, `out/run_signal.log` and `out/run_pileup.log`.

### Walk-through of `step3_DIGI_mixed_11_DIGI_cfg.py`

- `process = cms.Process('DIGI', Run3_2024)`: the process name under which the new products are stored.
- `process.load(...)`: `Services_cff` (the `RandomNumberGeneratorService`: every digitiser and the module `mix` have
  their own seeded engine), `pythiapdt_cfi`, the `MessageLogger`, `EventContent_cff`,
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
2.0 MB, the (11b) file 0.9 MB). The SIM products of the signal (`SimTrack`, `SimVertex`, every `PSimHit` and `PCaloHit`
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
| `EcalTriggerPrimitiveDigi simEcalTriggerPrimitiveDigis` | the ECAL trigger primitives that drive the selective readout |
| `HcalDataFrameContainer<QIE11DataFrame> simHcalDigis:HBHEQIE11DigiCollection`, `<QIE10DataFrame> simHcalDigis:HFQIE10DigiCollection`, `HODataFrame simHcalDigis` | the HCAL frames after the zero suppression; the same three under `simHcalUnsuppressedDigis` are the frames of every channel (plus the legacy `HBHEDataFrame`, `HFDataFrame` collections, empty, and the `ZDCDataFrame`) |
| `HcalTriggerPrimitiveDigi simHcalTriggerPrimitiveDigis` | the HCAL trigger primitives |
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
  globalTag=auto:phase1_2024_realistic era=Run3_2024 geometry=DB:Extended \
  pdfFile=digiAnalyzer_signal_11_DIGI.pdf pngPrefix=digiAnalyzer_signal_11_DIGI savePNG=1 \
  sampleLabel="chapter 11_DIGI (11a): the signal digitised|bbbar, pTHat > 100 GeV|2024 detector, 13.6 TeV|no pileup" \
  > digiAnalyzer_signal_11_DIGI.log 2>&1
cmsRun ../../common/digiAnalyzer_cfg.py inputFiles=file:$J/test/11_DIGI/out/step3_DIGI_pileup_11_DIGI.root \
  outputFile=digiAnalyzer_pileup_11_DIGI.root maxEvents=1 eventsToProcess=1:1:72 \
  globalTag=auto:phase1_2024_realistic era=Run3_2024 geometry=DB:Extended \
  pdfFile=digiAnalyzer_pileup_11_DIGI.pdf pngPrefix=digiAnalyzer_pileup_11_DIGI savePNG=1 \
  sampleLabel="chapter 11_DIGI (11b): one pileup collision digitised|library event 72|SoftQCD inelastic, 13.6 TeV|2024 detector, no signal" \
  > digiAnalyzer_pileup_11_DIGI.log 2>&1
cmsRun ../../common/digiAnalyzer_cfg.py inputFiles=file:$J/test/11_DIGI/out/step3_DIGI_mixed_11_DIGI.root \
  outputFile=digiAnalyzer_mixed_11_DIGI.root maxEvents=1 eventsToProcess=1:1:1 \
  globalTag=auto:phase1_2024_realistic era=Run3_2024 geometry=DB:Extended \
  pdfFile=digiAnalyzer_mixed_11_DIGI.pdf pngPrefix=digiAnalyzer_mixed_11_DIGI savePNG=1 \
  sampleLabel="chapter 11_DIGI (11a+11b): signal + pileup digitised|PU = 60 per bx (Poisson), bx -1..1|the collisions of the chapter 10 mixing (playback)|2024 detector, 13.6 TeV" \
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
every file, is counted with 2251 / 2115 / 1889 digis). The signal `ak4GenJets` above `jetPtMin = 30` GeV are drawn
as circles of radius 0.4. Tracker and muon digis are thinned for the drawing (`maxDrawn = 8000` per subdetector);
the counts are exact. Products:

| file | content |
|---|---|
| `digiAnalyzer_<case>_11_DIGI.pdf` | 2 pages: eta-phi, r-z (A4 MediaBox with a CropBox, do not include directly) |
| `digiAnalyzer_<case>_11_DIGI_etaphi_run1_event<E>.png`, `..._rz_run1_event<E>.png` | the same pages as PNG (E = 1 for the signal and the mixed event, 72 for the pileup collision; the stale `event67` PNGs are from an earlier run with the old SKIP) |
| `digiAnalyzer_<case>_11_DIGI.log` | the counts: per subdetector `nDigis`, `signal / inTime / early / late / noLink` (tracker and muon) or `sumAmplitude[ADC]` (calorimeters), the pileup summary, the total |
| `digiAnalyzer_<case>_11_DIGI.root` | the `TFileService` file (empty histograms directory; the drawing goes to the PDF) |
| `thresholds_11_DIGI.txt` | every threshold-like parameter of the digitisers of the mixed configuration, then the full parameter sets |
| `ledger_11_DIGI.txt` | the three logs side by side: digis per subdetector and origin, calorimeter amplitudes |

## 4. The tables

### 4.1 `ledger_11_DIGI.txt` (the output of `common/digiLedger.py`, in full)

```
== digiLedger 11_DIGI
signal: run:lumi:event 1:1:1 vertex=(0.0276598,-0.0169037,3.11193) cm | bx 0: 0 pileup events
pileup: run:lumi:event 1:1:72 vertex=(0.0286612,-0.0165738,-0.482677) cm | bx 0: 0 pileup events
mixed: run:lumi:event 1:1:1 vertex=(0.0276598,-0.0169037,3.11193) cm | bx -1: 71 pileup events | bx 0: 68 pileup events | bx 1: 54 pileup events
subdet         (11a)      (11b)  (11a+11b) | mixed by origin: signal / in-time / early / late / no link ; calorimeters: sum amplitude [ADC] (11a) (11b) (11a+11b)
PXB             3134        796      53601 | 2883 / 49385 / 1333 / 0 / 0
PXF             1855        325      26745 | 1844 / 24454 / 447 / 0 / 0
TIB             5573       2914      93129 | 3107 / 61435 / 25539 / 1350 / 1698
TID             2495       1116      34819 | 1733 / 24082 / 7824 / 647 / 533
TOB             6695       4599     106962 | 2751 / 62340 / 37149 / 1248 / 3474
TEC            17006       7934     205033 | 11653 / 133390 / 52311 / 3721 / 3958
EB               399        285       2050 | amplitude 3426 2423 18927
EE               372         55       4144 | amplitude 2088 283 26952
ES               981        360      15907 | amplitude 17885 5053 228109
HB              1538       1530       2115 | amplitude 13074 12692 27573
HE              1455       1147       3478 | amplitude 14537 5739 141454
HO                 -          -          1 | amplitude 0 0 63
HF              2251       2115       1889 | amplitude 8120 3940 31194
DT                 -          -         12 | 0 / 1 / 9 / 2 / 0
CSCstrip           -        176       1888 | 0 / 305 / 62 / 36 / 1485
CSCwire            -          3         91 | 0 / 0 / 0 / 0 / 91
RPC                8          8         10 | 0 / 3 / 0 / 0 / 7
GEM                -          -         15 | 0 / 14 / 1 / 0 / 0
total          43762      23363     551889 |
```

### 4.2 The analyzer logs (the split of the digis in each sample, `digiAnalyzer_<case>_11_DIGI.log`)

(11a) `digiAnalyzer_signal_11_DIGI.log`:

```
== DigiAnalyzer run:lumi:event 1:1:1 vertex=(0.0276598,-0.0169037,3.11193) cm | bx 0: 0 pileup events
  PXB: nDigis=3134 signal=3128 inTime=0 early=0 late=0 noLink=6
  PXF: nDigis=1855 signal=1854 inTime=0 early=0 late=0 noLink=1
  TIB: nDigis=5573 signal=3171 inTime=0 early=0 late=0 noLink=2402
  TID: nDigis=2495 signal=1793 inTime=0 early=0 late=0 noLink=702
  TOB: nDigis=6695 signal=2813 inTime=0 early=0 late=0 noLink=3882
  TEC: nDigis=17006 signal=11736 inTime=0 early=0 late=0 noLink=5270
  EB: nDigis=399 sumAmplitude[ADC]=3426
  EE: nDigis=372 sumAmplitude[ADC]=2088
  ES: nDigis=981 sumAmplitude[ADC]=17885
  HB: nDigis=1538 sumAmplitude[ADC]=13073.8
  HE: nDigis=1455 sumAmplitude[ADC]=14537
  HF: nDigis=2251 sumAmplitude[ADC]=8120
  RPC: nDigis=8 signal=0 inTime=0 early=0 late=0 noLink=8
  total digis=43762 genJets(pt>30)=2
```

(11b) `digiAnalyzer_pileup_11_DIGI.log`:

```
== DigiAnalyzer run:lumi:event 1:1:72 vertex=(0.0286612,-0.0165738,-0.482677) cm | bx 0: 0 pileup events
  PXB: nDigis=796 signal=796 inTime=0 early=0 late=0 noLink=0
  PXF: nDigis=325 signal=325 inTime=0 early=0 late=0 noLink=0
  TIB: nDigis=2914 signal=577 inTime=0 early=0 late=0 noLink=2337
  TID: nDigis=1116 signal=339 inTime=0 early=0 late=0 noLink=777
  TOB: nDigis=4599 signal=563 inTime=0 early=0 late=0 noLink=4036
  TEC: nDigis=7934 signal=2708 inTime=0 early=0 late=0 noLink=5226
  EB: nDigis=285 sumAmplitude[ADC]=2423.33
  EE: nDigis=55 sumAmplitude[ADC]=282.667
  ES: nDigis=360 sumAmplitude[ADC]=5053
  HB: nDigis=1530 sumAmplitude[ADC]=12692.4
  HE: nDigis=1147 sumAmplitude[ADC]=5739.4
  HF: nDigis=2115 sumAmplitude[ADC]=3940
  CSCstrip: nDigis=176 signal=18 inTime=0 early=0 late=0 noLink=158
  CSCwire: nDigis=3 signal=0 inTime=0 early=0 late=0 noLink=3
  RPC: nDigis=8 signal=0 inTime=0 early=0 late=0 noLink=8
  total digis=23363 genJets(pt>30)=0
```

("signal" in this log means: linked to a particle of library event 72, the only event of the sample.)

(11a+11b) `digiAnalyzer_mixed_11_DIGI.log`:

```
== DigiAnalyzer run:lumi:event 1:1:1 vertex=(0.0276598,-0.0169037,3.11193) cm | bx -1: 71 pileup events | bx 0: 68 pileup events | bx 1: 54 pileup events
  PXB: nDigis=53601 signal=2883 inTime=49385 early=1333 late=0 noLink=0
  PXF: nDigis=26745 signal=1844 inTime=24454 early=447 late=0 noLink=0
  TIB: nDigis=93129 signal=3107 inTime=61435 early=25539 late=1350 noLink=1698
  TID: nDigis=34819 signal=1733 inTime=24082 early=7824 late=647 noLink=533
  TOB: nDigis=106962 signal=2751 inTime=62340 early=37149 late=1248 noLink=3474
  TEC: nDigis=205033 signal=11653 inTime=133390 early=52311 late=3721 noLink=3958
  EB: nDigis=2050 sumAmplitude[ADC]=18927
  EE: nDigis=4144 sumAmplitude[ADC]=26951.7
  ES: nDigis=15907 sumAmplitude[ADC]=228109
  HB: nDigis=2115 sumAmplitude[ADC]=27573
  HE: nDigis=3478 sumAmplitude[ADC]=141454
  HO: nDigis=1 sumAmplitude[ADC]=63
  HF: nDigis=1889 sumAmplitude[ADC]=31194
  DT: nDigis=12 signal=0 inTime=1 early=9 late=2 noLink=0
  CSCstrip: nDigis=1888 signal=0 inTime=305 early=62 late=36 noLink=1485
  CSCwire: nDigis=91 signal=0 inTime=0 early=0 late=0 noLink=91
  RPC: nDigis=10 signal=0 inTime=3 early=0 late=0 noLink=7
  GEM: nDigis=15 signal=0 inTime=14 early=1 late=0 noLink=0
  total digis=551889 genJets(pt>30)=2
```

Sums quoted in the chapter (from these logs): (11a) tracker 36 758 digis = 24 495 linked + 12 263 noise (pixels
4982 linked + 7 noise for 1902 PSimHits, strips 19 513 linked + 12 256 noise for 8370 PSimHits); (11b) tracker 17 684
= 5308 linked + 12 376 noise, all linked digis 5326 (5308 + 18 CSC strips), all noise 12 545 (12 376 + 158 + 3 + 8);
(11a+11b) tracker 520 289 = 23 971 signal + 355 086 in-time + 124 603 early + 6966 late + 9663 noise; pixels 80 346
with 1780 early and 0 late; strips 439 943 with 122 823 early (27.9 %); the signal lost 24 495 - 23 971 = 524 digis;
355 086 / 68 = 5222 in-time digis per collision; muon 2016 digis. Amplitude ratios (11a+11b)/(11a): EB 18927/3426 =
5.5, EE 26952/2088 = 12.9, ES 228109/17885 = 12.8, HB 27573/13074 = 2.1, HE 141454/14537 = 9.7, HF 31194/8120 = 3.8.

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
`simHcalUnsuppressedDigis` against `simHcalDigis` sizes, the `mix:MergedTrackTruth` split by `EncodedEventId`):

```
== signal run 1 event 1
  bx 0: PU_NumInteractions=0 TrueNumInteractions=0.0
  EB SR flags (value: n of the 2448 trigger towers): [(1, 2446), (3, 2)]; EE SR flags (n of the 624 supercrystals): [(1, 609), (3, 15)]
  HCAL unsuppressed -> zero suppressed: HBHE(QIE11) 15840 -> 3285, HF(QIE10) 3456 -> 3456, HO 2160 -> 0
  TrackingParticles (mix:MergedTrackTruth): 999 (signal 999, in-time pileup 0, out-of-time 0)
== pileup run 1 event 72
  bx 0: PU_NumInteractions=0 TrueNumInteractions=0.0
  EB SR flags: [(1, 2448)]; EE SR flags: [(1, 623), (3, 1)]
  HCAL unsuppressed -> zero suppressed: HBHE(QIE11) 15840 -> 2972, HF(QIE10) 3456 -> 3456, HO 2160 -> 0
  TrackingParticles (mix:MergedTrackTruth): 237 (signal 237, in-time pileup 0, out-of-time 0)
== mixed run 1 event 1
  bx -1: PU_NumInteractions=71 TrueNumInteractions=71.0
  bx 0: PU_NumInteractions=68 TrueNumInteractions=68.0
  bx 1: PU_NumInteractions=54 TrueNumInteractions=54.0
  EB SR flags: [(1, 2425), (3, 23)]; EE SR flags: [(1, 449), (3, 175)]
  HCAL unsuppressed -> zero suppressed: HBHE(QIE11) 15840 -> 5839, HF(QIE10) 3456 -> 3456, HO 2160 -> 1
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
| 2024 pixel | `SimGeneral/MixingModule/python/SiPixelSimParameters_cfi.py` (`_modifyPixelDigitizerForRun3`, `phase1Pixel`, `premix_stage1`) |
| 2024 strip | `SimGeneral/MixingModule/python/SiStripSimParameters_cfi.py`, `RecoLocalTracker/SiStripZeroSuppression/src/SiStripFedZeroSuppression.cc` (algorithm 4) |
| ECAL | `SimGeneral/MixingModule/python/ecalDigitizer_cfi.py`, `ecalDigitizer_Ph2_cfi.py`, `SimCalorimetry/EcalSimProducers/python/ecalDigiParameters_cff.py`, `ecalDigiParameters_Ph2_cff.py`, `ecalSimParameterMap_cff.py`, `SimCalorimetry/EcalSelectiveReadoutProducers/python/ecalDigis_cfi.py`, `SimCalorimetry/EcalZeroSuppressionProducers/python/ecalPreshowerDigis_cfi.py` |
| HCAL | `SimCalorimetry/HcalSimProducers/python/hcalUnsuppressedDigis_cfi.py`, `hcalSimParameters_cfi.py`, `SimCalorimetry/HcalZeroSuppressionProducers/python/hcalDigisRealistic_cfi.py`, `CalibCalorimetry/HcalPlugins/python/HBHEDarkening_cff.py` |
| HGCal | `SimCalorimetry/HGCalSimProducers/python/hgcalDigitizer_cfi.py`, `hgcROCParameters_cfi.py` |
| MTD | `SimFastTiming/FastTimingCommon/python/mtdDigitizer_cfi.py` |
| muon | `SimMuon/DTDigitizer/python/muonDTDigis_cfi.py`, `SimMuon/CSCDigitizer/python/muonCSCDigis_cfi.py`, `SimMuon/RPCDigitizer/python/muonRPCDigis_cfi.py`, `SimMuon/GEMDigitizer/python/muonGEMDigis_cfi.py`, `muonME0Digis_cfi.py`, `SimMuon/GEMDigitizer/plugins/GEMDigiProducer.cc` (`fillDescriptions`), `SimMuon/Configuration/python/SimMuon_cff.py` |
| ageing | `SLHCUpgradeSimulations/Configuration/python/aging.py` (`customise_aging_300/1000/3000`, `ageHcal`, `ageEcal`, `agedHGCal`, `ageSiPM`) |
| mixing, premixing | `Configuration/StandardSequences/python/Mixing.py`, `SimGeneral/MixingModule/python/mix_POISSON_average_cfi.py`, `mix_2024_25ns_RunIII2024Summer24_PoissonOOTPU_cfi.py`, `mix_probFunction_25ns_PoissonOOTPU_cfi.py`, `Configuration/PyReleaseValidation/python/relval_steps.py` (`'Run4'`: `AVE_200_BX_25ns`), `Configuration/StandardSequences/python/DataMixerPreMix_cff.py`, `Mixing/Base/src/BMixingModule.cc`, `Mixing/Base/src/PileUp.cc`, `SimGeneral/MixingModule/plugins/MixingModule.cc` |
