# Chapter 10, SIM: technical companion

The full Geant4 physics switched on stage by stage (field, decays, electromagnetic, hadronic), the signal, the pileup
library and the mixing at SimHit level. Script: `test/10_SIM/run_10_SIM.sh`; everything it produces is in
`test/10_SIM/out/`. The chapter text is `doc/chapter/chapter_10_SIM.tex`. All commands below are run from `test/` of an
initialized CMSSW_15_0_5 (`cmsenv`), with `J=$CMSSW_BASE/src/JitJetSW/Journey`.

## 1. The cmsDriver.py commands

The script runs `cmsDriver.py` six times through `run_driver` of `test/common/jitjet_common.sh`: four SIM stages on the
signal, the SIM of the library, and the mixing job. The commands, copied from the header of the generated configurations
(`test/10_SIM/step2_SIM_<stage>_10_SIM_cfg.py` and `step3_MIX_10_SIM_cfg.py`, "with command line options"; the cmsDriver
output is in `out/<config>.driver.log`):

```bash
cd $J/test
IN=$J/test/09_G4/out/step2_G4_signal_09_G4.root       # Rule E: the transport-only signal of chapter 09
LIB=$J/test/09_G4/out/step2_G4_pileup_09_G4.root      # the transport-only library of chapter 09
COMMON="--conditions auto:phase1_2025_realistic --era Run3_2025 --geometry DB:Extended --beamspot DBrealistic \
  -s SIM --processName SIMFULL --datatier GEN-SIM --eventcontent FEVTDEBUG --outputCommands drop *_*_*_SIM"

# stage B: field on, still the transport-only physics list (no decays, no EM, no hadronic physics)
cmsDriver.py step2 --filein file:$IN $COMMON -n 1 --fileout file:step2_SIM_B_10_SIM.root \
  --customise_commands "process.g4SimHits.UseMagneticField = cms.bool(True)\nprocess.g4SimHits.Physics.type = cms.string('SimG4Core/Physics/JourneyTransportPhysics')\nprocess.g4SimHits.Physics.G4TransportWithMSC = cms.int32(0)\nprocess.g4SimHits.Physics.G4GammaGeneralProcess = cms.bool(False)\nprocess.g4SimHits.TrackerSD.ZeroEnergyLoss = cms.bool(True)" \
  --python_filename $J/test/10_SIM/step2_SIM_B_10_SIM_cfg.py --no_exec

# stage B_Decay: + decays in flight (FTFP_BERT_EMM with its EM and hadronic constructors off)
cmsDriver.py step2 --filein file:$IN $COMMON -n 1 --fileout file:step2_SIM_B_Decay_10_SIM.root \
  --customise_commands "process.g4SimHits.UseMagneticField = cms.bool(True)\nprocess.g4SimHits.Physics.EMPhysics = cms.untracked.bool(False)\nprocess.g4SimHits.Physics.HadPhysics = cms.untracked.bool(False)\nprocess.g4SimHits.Physics.G4TransportWithMSC = cms.int32(0)\nprocess.g4SimHits.Physics.G4GammaGeneralProcess = cms.bool(False)\nprocess.g4SimHits.TrackerSD.ZeroEnergyLoss = cms.bool(True)" \
  --python_filename $J/test/10_SIM/step2_SIM_B_Decay_10_SIM_cfg.py --no_exec

# stage B_Decay_EM: + electromagnetic physics
cmsDriver.py step2 --filein file:$IN $COMMON -n 1 --fileout file:step2_SIM_B_Decay_EM_10_SIM.root \
  --customise_commands "process.g4SimHits.UseMagneticField = cms.bool(True)\nprocess.g4SimHits.Physics.EMPhysics = cms.untracked.bool(True)\nprocess.g4SimHits.Physics.HadPhysics = cms.untracked.bool(False)" \
  --python_filename $J/test/10_SIM/step2_SIM_B_Decay_EM_10_SIM_cfg.py --no_exec

# stage signal: + hadronic physics = the full SIM step (10a), the file chapters 10 and 11 use
cmsDriver.py step2 --filein file:$IN $COMMON -n 1 --fileout file:step2_SIM_signal_10_SIM.root \
  --customise_commands "process.g4SimHits.UseMagneticField = cms.bool(True)\nprocess.g4SimHits.Physics.EMPhysics = cms.untracked.bool(True)\nprocess.g4SimHits.Physics.HadPhysics = cms.untracked.bool(True)" \
  --python_filename $J/test/10_SIM/step2_SIM_signal_10_SIM_cfg.py --no_exec

# (10b) the library: all 1000 collisions, full physics, its own Geant4 seed
cmsDriver.py step2 --filein file:$LIB $COMMON -n -1 --fileout file:step2_SIM_pileup_10_SIM.root \
  --customise_commands "process.g4SimHits.UseMagneticField = cms.bool(True)\nprocess.g4SimHits.Physics.EMPhysics = cms.untracked.bool(True)\nprocess.g4SimHits.Physics.HadPhysics = cms.untracked.bool(True)\nprocess.RandomNumberGeneratorService.g4SimHits.initialSeed = 20243" \
  --python_filename $J/test/10_SIM/step2_SIM_pileup_10_SIM_cfg.py --no_exec

# (10a+10b) the mixing at SimHit level: the MixingModule of the DIGI step without digitisers, SimHitAnalyzer on the CrossingFrames
cmsDriver.py step3 --filein file:$J/test/10_SIM/out/step2_SIM_signal_10_SIM.root \
  --pileup "AVE_60_BX_25ns,{'N': 60, 'B': (-12, 3)}" --pileup_input file:$J/test/10_SIM/out/step2_SIM_pileup_10_SIM.root \
  --conditions auto:phase1_2025_realistic --era Run3_2025 --geometry DB:Extended -n -1 -s DIGI:pdigi_valid --processName MIX \
  --datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG --fileout file:step3_MIX_10_SIM.root \
  --outputCommands "drop *_mix_*_*,keep CrossingFramePlaybackInfoNew_mix_*_*" \
  --customise_commands "process.mix.input.sequential = cms.untracked.bool(True)\nprocess.source.eventsToProcess = cms.untracked.VEventRange('1:1:1')\nfrom JitJetSW.Journey.mixOnly_cff import mixOnly; process = mixOnly(process, outputDir='$J/test/10_SIM/out/mixed', sampleLabel='chapter 10_SIM (10a+10b): signal + pileup at SimHit level|PU = 60 per bx (Poisson), bx -12..3|library in sequence, no digitisers|2025 detector, 13.6 TeV', prefix='simHits_mixed_10_SIM', highlight=1, highlightPtMin=2, savePNG=True, printHits=False)" \
  --python_filename $J/test/10_SIM/step3_MIX_10_SIM_cfg.py --no_exec
```

The arguments of the SIM steps:

| argument | meaning |
|---|---|
| `step2` | the name of the configuration (free; `step1` was the GEN step of chapters 01 to 08) |
| `--filein file:...` | the input (Rule E): the transport-only SIM file of chapter 09 (signal) or its 1000-event library (pileup). The generator record is read from it; the SIM products of process `SIM` are read too but dropped from the output |
| `--conditions auto:phase1_2025_realistic` | the global tag of 2025 (`150X_mcRun3_2025_realistic_v1` through `Configuration/AlCa/python/autoCond.py`, line 107; `YEAR=2025` of `jitjet_common.sh`): geometry, field map, beam spot; the geometry `141DD4hepV2_Extended2024` and the field map are the same tags as in the 2024 global tag (chapter 09 companion, section 4) |
| `--era Run3_2025` | the era modifiers of the 2025 detector (`Era_Run3_2025_cff.py`: `Run3_2024` plus five modifiers of the reconstruction and the Level-1 trigger, none used by `g4SimHits` or the `MixingModule`): through `Run3`, `run3_common` selects the Run-3 sensitive detectors and hit collections in `g4SimHits_cfi.py`, `run3_HFSL` the HF shower library `HFShowerLibrary_run3_v7.root` |
| `--geometry DB:Extended` | the simulation geometry (DDD XML) from the database, label `Extended` |
| `--beamspot DBrealistic` | only sets the `VtxSmeared` label in a SIM-only step; the smeared vertices are already in the input |
| `-n 1` / `-n -1` | one event (the signal) / all events (the 1000 library collisions) |
| `-s SIM` | the step: the `psim` sequence (`Configuration/StandardSequences/python/SimIdeal_cff.py` = `randomEngineStateProducer` + `g4SimHits`) |
| `--processName SIMFULL` | the process name of this run; the input already holds a process `SIM` (chapter 09) and cmsRun refuses two processes of the same name in one file |
| `--datatier GEN-SIM --eventcontent FEVTDEBUG` | the output tier and the full debug event content |
| `--outputCommands "drop *_*_*_SIM"` | the transport-only `SimTrack`s, `SimVertex`s, `PSimHit`s and `PCaloHit`s of chapter 09 are not copied: the output holds one set of simulation products, those of process `SIMFULL` |
| `--fileout file:...` | the output ROOT file, written in `out/` where `cmsRun` is started |
| `--customise_commands "..."` | python lines appended to the configuration (`\n` separated): the physics switches of the stage, below |
| `--python_filename ... --no_exec` | write the configuration and do not run it; the script runs `cmsRun` itself |

The customise lines of the stages (`stage_switches` and `run_sim` in `run_10_SIM.sh`):

| line | stages | effect |
|---|---|---|
| `process.g4SimHits.UseMagneticField = cms.bool(True)` | all | the 3.8 T field map of the global tag is used (chapter 09 had `False`) |
| `process.g4SimHits.Physics.type = cms.string('SimG4Core/Physics/JourneyTransportPhysics')` | `B` | the physics list of `$J/plugins/JourneyTransportPhysics.cc`: all particles constructed, no process registered, only the transportation (chapter 09). Every standard list registers `G4DecayPhysics`, so "decays off" needs this list |
| `process.g4SimHits.Physics.EMPhysics = cms.untracked.bool(False/True)` | `B_Decay`, `B_Decay_EM`, `signal`, `pileup` | the `FTFP_BERT_EMM` list of `g4SimHits_cfi.py` (`Physics.type` untouched) with its electromagnetic constructor `CMSEmStandardPhysics` off or on (`SimG4Core/PhysicsLists/plugins/FTFPCMS_BERT_EMM.cc` reads `EMPhysics` and `HadPhysics`) |
| `process.g4SimHits.Physics.HadPhysics = cms.untracked.bool(False/True)` | same | the hadronic constructors (elastic, FTFP/Bertini inelastic, stopping) off or on; `G4DecayPhysics` is always registered by the list |
| `process.g4SimHits.Physics.G4TransportWithMSC = cms.int32(0)`, `G4GammaGeneralProcess = cms.bool(False)` | `B`, `B_Decay` | no coupling of the transportation to multiple scattering and no general photon process when there is no EM physics (the default `2` / `True` would reference processes that do not exist) |
| `process.g4SimHits.TrackerSD.ZeroEnergyLoss = cms.bool(True)` | `B`, `B_Decay` | without ionisation no hit would be recorded (`TkAccumulatingSensitiveDetector` keeps a hit only if the energy loss is above zero or this flag is set); the crossings are recorded with `energyLoss() = 0` |
| `process.RandomNumberGeneratorService.g4SimHits.initialSeed = 20243` | `pileup` | the Geant4 seed of the library, `PUSEED + 2` with `PUSEED = 20241` (chapter 08), so that the library's showers do not reuse the signal's random sequence |

The arguments of the mixing job (step3) that differ:

| argument | meaning |
|---|---|
| `--filein file:.../step2_SIM_signal_10_SIM.root` | the full-physics signal (10a) |
| `--pileup "AVE_60_BX_25ns,{'N': 60, 'B': (-12, 3)}"` | the mixing scenario: `mix_POISSON_average_cfi` (`Configuration/StandardSequences/python/Mixing.py`, `AVE_60_BX_25ns` = `BX 25, B (-12,3), N 60`), with the dictionary: Poisson with mean `N = 60` collisions per bunch crossing, bunch crossings `B = (-12, 3)` (the default of the scenario and the window of the Run-3 production profiles, written out so that `BUNCHES` of the script decides it; `-1, 1` until 21 November 2026), 25 ns spacing |
| `--pileup_input file:.../step2_SIM_pileup_10_SIM.root` | the library (10b) read by the secondary source `EmbeddedRootSource` of the module `mix` |
| `-s DIGI:pdigi_valid` | the DIGI step; its `pdigi_valid` sequence is replaced by `mixOnly` with the `mix` module alone (section 2) |
| `--processName MIX` | the process name of the mixing job (the input holds `GEN`, `GENJET`, `SIMFULL`) |
| `--datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG` | the full content of the input is copied to the output |
| `--outputCommands "drop *_mix_*_*,keep CrossingFramePlaybackInfoNew_mix_*_*"` | nothing of the module `mix` is written (the `CrossingFrame`s are transient anyway) except the `CrossingFramePlaybackInfoNew`: the record of which library events went into which bunch crossing, the input of the playback of chapter 11 |
| `--fileout file:step3_MIX_10_SIM.root` | the output: the signal event with the playback record |
| `--customise_commands` | `process.mix.input.sequential = True`: the library is read from its first event on, in order (the default picks a random start); `process.source.eventsToProcess = '1:1:1'`: the event of the book; `mixOnly(process, outputDir=..., sampleLabel=..., prefix=..., highlight=1, highlightPtMin=2, savePNG=True, printHits=False)`: `$J/python/mixOnly_cff.py`, section 2. With `ANALYZE=0` the script writes the same configuration with `mixOnly(process)` (no drawing): the mixing and its playback record only |

Variables of the script: `PU=60 BUNCHES=-12,3 SKIP=746 NPU=64` (the pileup convention of the book, `common/jitjet_common.sh`;
until 21 November 2026 `BUNCHES=-1,1 SKIP=71 NPU=68` with a 250-event library),
`PUSEED=20241`, `STAGES="B B_Decay B_Decay_EM signal"`, `CASES="signal pileup mixed"`, `REUSE=1` (an existing SIM file is
not re-run), `STATIONS=` (a subset of `TRK,ECAL,HCAL,MUON,ALL`), `HIGHLIGHT=1 HIGHLIGHT_PTMIN=2 PRINTHITS=0` (the
SimHitAnalyzer options), `RUN=0` (write every configuration, the mixing one included, run nothing), `ANALYZE=0` (the SIM
stages, the library and the mixing with its playback record, no analyzer and no drawing).

## 2. The cmsRun on the python config

```bash
cd $J/test/10_SIM/out
# stage B uses the physics list built in this package: run it without the simulation biglib (chapter 09, section 2)
env LD_LIBRARY_PATH=$(echo "$LD_LIBRARY_PATH" | tr ':' '\n' | grep -v '/biglib/' | paste -sd:) \
  cmsRun ../step2_SIM_B_10_SIM_cfg.py > step2_SIM_B_10_SIM.log 2>&1
cmsRun ../step2_SIM_B_Decay_10_SIM_cfg.py    > step2_SIM_B_Decay_10_SIM.log 2>&1
cmsRun ../step2_SIM_B_Decay_EM_10_SIM_cfg.py > step2_SIM_B_Decay_EM_10_SIM.log 2>&1
cmsRun ../step2_SIM_signal_10_SIM_cfg.py     > step2_SIM_signal_10_SIM.log 2>&1
cmsRun ../step2_SIM_pileup_10_SIM_cfg.py     > step2_SIM_pileup_10_SIM.log 2>&1     # 1000 events, about 30 minutes
cmsRun ../step3_MIX_10_SIM_cfg.py            > step3_MIX_10_SIM.log 2>&1            # the mixing, writes mixed/<station>/ and step3_MIX_10_SIM.root
```

The script runs the library in the background while the four signal stages run (`run_sim pileup ... &`, `run_pileup.log`),
with `REUSE=1` an existing `step2_SIM_<stage>_10_SIM.root` is kept. Timing from the logs (wall clock on a shared lxplus-like
node, so it depends on its load): the signal event through the full physics takes 8 s in the 2024 run of 7 November
2026 (`Begin processing ... 09:51:53`, `Closed file ... 09:52:01`, after 36 s of Geant4 initialisation) and 12 s in
the 2025 rerun of 12 November 2026 (`10:15:10` to `10:15:22`, after 50 s of initialisation, load average 5-7); the 250
library collisions 442 s (`09:49:57` to `09:57:19`), 1.8 s per collision, and 695 s (`10:12:15` to `10:23:51`), 2.8 s
per collision; the 1000 collisions of 21 November 2026 1903 s (`12:58:12` to `13:29:55`, load average 5-6), 1.9 s per
collision. All logs carry the
banner `Geant4 version Name: geant4-11-02-patch-02 [MT] (21-June-2024)`.

The pages, run of 21 November 2026, 19:36 (`run_10_SIM.log`, SIM files reused; the numbers of table "The jobs of
run_10_SIM.sh" of the chapter). Every PNG is rendered from its PDF page by `pdftoppm` (`plugins/PdfPng.h`, section 3):

| job | from | to | time |
|---|---|---|---|
| the four `SimAnalyzer` runs | 19:36:39 | 19:37:10 | 6 to 9 s each |
| `SimHitAnalyzer` (10a), 5 stations x 3 views | 19:37:10 | 19:37:27 | 17 s |
| `SimHitAnalyzer` (10b), library event 747 | 19:37:27 | 19:37:40 | 13 s |
| the mixing with its pages (`cmsRun` start to the last PNG; `Begin processing` 19:37:49) | 19:37:43 | 19:41:14 | 3 min 31 s |
| of which `TRK` (the mixing, the PDF closed at 19:38:55, the three PNGs) | 19:37:49 | 19:39:27 | 1 min 38 s |
| `ECAL`, `HCAL`, `MUON` | 19:39:27 | 19:39:39 | 12 s |
| `ALL` (the PDF closed at 19:40:40) | 19:39:39 | 19:41:14 | 1 min 35 s |
| the ledger, the extras | 19:41:15 | 19:41:23 | 1 s, 8 s |
| the whole run (`== 10_SIM`, `== 11_DIGI` of the wrapper) | 19:36:39 | 19:41:23 | 4 min 44 s |

A run from scratch adds the 33 min of the library (the four stages run inside it): about 38 min; with `ANALYZE=0`
(the SIM jobs and the mixing, 10 s, with its playback record) about 35 min. The outputs of this run are those of the run
before it, number for number (`ledger_10_SIM.txt`, `extras_10_SIM.txt`, `simHits_mixed_10_SIM.log` identical; the
other two logs differ in their time stamps only), and the PNGs have the size of ROOT's, 1396 x 872 pixels.

The run before it, of 21 November 2026, 13:41, made the PNGs with ROOT (`TCanvas::SaveAs`); its logs are kept in
`test/tmp/out_rootpng_10_SIM/` (`run_10_SIM.log`, `step3_MIX_10_SIM.log`, the three `simHits_*` logs):

| job | from | to | time |
|---|---|---|---|
| `SimHitAnalyzer` (10a) | 13:42:13 | 13:51:57 | 9.7 min |
| `SimHitAnalyzer` (10b) | 13:51:57 | 13:59:30 | 7.5 min |
| the mixing with its pages (`Begin processing` 14:00:06) | 13:59:46 | 18:39:04 | 4 h 39 min |
| of which `TRK` (PNG time stamps: r-z 14:53:26, x-y 15:16:19, eta-phi 16:07:18) | 14:00:06 | 16:07:18 | 2 h 07 min |
| `ECAL` / `HCAL` / `MUON` | 16:07:18 | 16:22:00 | 11 min / 3.4 min / 4 s |
| `ALL` (r-z 17:20:29, x-y 17:45:04, eta-phi 18:39:04) | 16:22:00 | 18:39:04 | 2 h 17 min |
| the whole run | 13:41:43 | 18:39:15 | 4 h 58 min |

Per marker: the tracker r-z view drew 2 106 681 hits in 53 min, the x-y view the 909 502 pileup hits of the barrel
(685 160 + 54 572 + 169 770 in its legend) and those of the signal in 23 min, the eta-phi view 2 106 681 in 51 min:
about 1.5 ms per marker and view ("1.5 ms" of the chapter). The time went into ROOT's PNG: per view the analyzer
painted the canvas, printed the PDF page and saved the PNG, and closed the PDF of the station after the eta-phi page;
`mixed/TRK/simHits_mixed_10_SIM_TRK_run1_event1.pdf` was last written at 15:16:47, 28 s after the x-y PNG, and the
eta-phi PNG came 50 min later (`ALL`: 17:45:04, 17:45:34, 18:39:04; `ECAL`: 16:14:30, 16:14:32, 16:18:31).
`pdftoppm -cropbox -png` made the three pages of that PDF in 32 s, which led to `plugins/PdfPng.h`.

### The SIM configuration, `step2_SIM_signal_10_SIM_cfg.py`

| part | content |
|---|---|
| `process = cms.Process('SIMFULL', Run3_2025)` | the process name and the era |
| `process.load(...)` | `Services_cff`, `pythiapdt_cfi` (the particle table), `MessageLogger_cfi`, `EventContent_cff`, `mixNoPU_cfi` (the `mix` module without pileup, present in every SIM step but not in the path), `GeometryRecoDB_cff` and `GeometrySimDB_cff` (both geometries from the database), `MagneticField_cff`, `SimIdeal_cff` (`g4SimHits`, i.e. `SimG4Core/Application/python/g4SimHits_cfi.py`), `EndOfProcess_cff`, `FrontierConditions_GlobalTag_cff` |
| `process.source` | `PoolSource` on the chapter-09 file; `process.maxEvents.input = 1` (`-1` for the library) |
| `process.FEVTDEBUGoutput` | `PoolOutputModule`, `dataTier GEN-SIM`, `outputCommands = FEVTDEBUGEventContent.outputCommands` plus the appended `'drop *_*_*_SIM'` |
| `process.GlobalTag` | `auto:phase1_2025_realistic` resolved by `Configuration.AlCa.GlobalTag`; `XMLFromDBSource.label = "Extended"` selects the geometry payload |
| `process.simulation_step = cms.Path(process.psim)` | the one path: `randomEngineStateProducer` + `g4SimHits` (`OscarMTProducer`) |
| `process.schedule` | `simulation_step, endjob_step, FEVTDEBUGoutput_step` |
| customisation | the physics switches of section 1, then `customiseEarlyDelete` |

The stage configurations differ only in `--fileout` and in the customise lines (`diff step2_SIM_signal_10_SIM_cfg.py
step2_SIM_B_10_SIM_cfg.py`); the library configuration in the input file, `maxEvents.input = -1` and the seed line.

### The mixing configuration, `step3_MIX_10_SIM_cfg.py`

| part | content |
|---|---|
| `process = cms.Process('MIX', Run3_2025)` | |
| `process.load('SimGeneral.MixingModule.mix_POISSON_average_cfi')` | the `mix` module (`MixingModule`), with `digitizers = theDigitizers`, `input = EmbeddedRootSource(type = 'poisson', sequential = False, ...)`, `mixObjects = theMixObjects` (`mixObjects_cfi.py`: which `PSimHit`, `PCaloHit`, `SimTrack`, `SimVertex` collections are mixed) |
| `process.load('Configuration.StandardSequences.Digi_cff')` | the digitisers and `pdigi_valid` |
| `process.source.inputCommands` | the `drop *_genParticles_*_*`, `drop *_ak4GenJets_*_*`, ... that `cmsDriver.py` writes for a DIGI step (the GEN collections are remade from `generatorSmeared` by `pdigi_valid`; with `mixOnly` they are simply not in the output) |
| `process.FEVTDEBUGoutput` | `dataTier GEN-SIM-DIGI`, `fileName step3_MIX_10_SIM.root`, `outputCommands` + `'drop *_mix_*_*'` + `'keep CrossingFramePlaybackInfoNew_mix_*_*'` |
| `process.mix.input.nbPileupEvents.averageNumber = 60`, `bunchspace = 25`, `minBunch = -1`, `maxBunch = 1`, `input.fileNames = [the library]` | written by `defineMixing` of `Mixing.py` from the `--pileup` dictionary |
| `process.mix.digitizers = cms.PSet(process.theDigitizersValid)` | the digitisers with truth (`pdigi_valid`), removed again by `mixOnly` |
| `process.digitisation_step = cms.Path(process.pdigi_valid)` | replaced by `mixOnly` |
| customisation | `process.mix.input.sequential = True`; `process.source.eventsToProcess = VEventRange('1:1:1')`; `mixOnly(...)` |

`mixOnly` (`$J/python/mixOnly_cff.py`): `process.mix.digitizers = cms.PSet()` (no digitiser, no `TrackingTruth` accumulator);
the `EDAlias`es of `SimGeneral/MixingModule/python/aliases_cfi.py` (`simSiPixelDigis`, `simSiStripDigis`,
`simEcalUnsuppressedDigis`, `simHcalUnsuppressedDigis`, `simAPVsaturation`, ...) that point at products of the digitisers
inside `mix` are deleted (they would fail the alias check at start-up); `process.digitisation_step = cms.Path(process.mix)`;
and with `outputDir` given, `addSimHitAnalyzer` of `$J/python/simHitAnalyzer_cff.py` adds the `SimHitAnalyzer` reading the
`CrossingFrame` products `mix:g4SimHits<Collection>` (instead of `g4SimHits:<Collection>`) on an `EndPath`. The output module
stays: it writes the signal event again with the playback record.

What the MixingModule does per event (`SimGeneral/MixingModule/plugins/MixingModule.cc`, `Mixing/Base/src/BMixingModule.cc`,
`Mixing/Base/src/PileUp.cc`): for every bunch crossing from `minBunch` to `maxBunch` it draws the number of collisions from
the Poisson distribution (`PileUp::CalculatePileup`), reads that many events from the secondary source (in sequence here),
and for every mixed collection copies the objects into a `CrossingFrame<T>` with the bunch crossing and the event number
encoded (`EncodedEventId`), the times shifted by `bunchspace * bx`; the signal objects go first. The `CrossingFrame`s are
transient products of `mix`. The `CrossingFramePlaybackInfoNew` records, per bunch crossing, the library events used.

### The content of the output files

`edmDumpEventContent file:step2_SIM_signal_10_SIM.root` (the stage files have the same content):

```
Type                                  Module                      Label             Process
GenEventInfoProduct                   "generator"                 ""                "GEN"
ROOT::Math::PositionVector3D<...>     "genParticles"              "xyz0"            "GEN"
edm::HepMCProduct                     "LHCTransport"              ""                "GEN"
edm::HepMCProduct                     "generatorSmeared"          ""                "GEN"
edm::RandomEngineStates               "randomEngineStateProducer" ""                "GEN"
edm::TriggerResults                   "TriggerResults"            ""                "GEN"
float                                 "genParticles"              "t0"              "GEN"
vector<int>                           "genParticles"              ""                "GEN"
vector<reco::GenMET>                  "genMetCalo"                ""                "GEN"
vector<reco::GenMET>                  "genMetTrue"                ""                "GEN"
vector<reco::GenParticle>             "genParticles"              ""                "GEN"
double / vector<double>               "ak4GenJets", "ak4GenJetsNoNu", "ak8GenJets", "ak8GenJetsNoNu"   "rho", "sigma", "rhos", "sigmas"   "GENJET"
edm::TriggerResults                   "TriggerResults"            ""                "GENJET"
vector<reco::GenJet>                  "ak4GenJets", "ak4GenJetsNoNu", "ak8GenJets", "ak8GenJetsNoNu"   ""   "GENJET"
edm::RandomEngineStates               "randomEngineStateProducer" ""                "SIMFULL"
edm::TriggerResults                   "TriggerResults"            ""                "SIMFULL"
vector<PCaloHit>                      "g4SimHits"                 "CaloHitsTk"      "SIMFULL"
vector<PCaloHit>                      "g4SimHits"                 "EcalHitsEB"      "SIMFULL"
vector<PCaloHit>                      "g4SimHits"                 "EcalHitsEE"      "SIMFULL"
vector<PCaloHit>                      "g4SimHits"                 "EcalHitsES"      "SIMFULL"
vector<PCaloHit>                      "g4SimHits"                 "HcalHits"        "SIMFULL"
vector<PCaloHit>                      "g4SimHits"                 "TotemHitsT2Scint" "SIMFULL"
vector<PCaloHit>                      "g4SimHits"                 "ZDCHITS"         "SIMFULL"
vector<PSimHit>                       "g4SimHits"                 "BCM1FHits", "BHMHits", "BSCHits", "CTPPSPixelHits", "CTPPSTimingHits", "PLTHits", "TotemHitsRP", "TotemHitsT1"   "SIMFULL"
vector<PSimHit>                       "g4SimHits"                 "MuonCSCHits", "MuonDTHits", "MuonGEMHits", "MuonME0Hits", "MuonRPCHits"   "SIMFULL"
vector<PSimHit>                       "g4SimHits"                 "TrackerHitsPixelBarrelLowTof", "TrackerHitsPixelBarrelHighTof", "TrackerHitsPixelEndcapLowTof", "TrackerHitsPixelEndcapHighTof"   "SIMFULL"
vector<PSimHit>                       "g4SimHits"                 "TrackerHitsTIBLowTof", "TrackerHitsTIBHighTof", "TrackerHitsTIDLowTof", "TrackerHitsTIDHighTof", "TrackerHitsTOBLowTof", "TrackerHitsTOBHighTof", "TrackerHitsTECLowTof", "TrackerHitsTECHighTof"   "SIMFULL"
vector<SimTrack>                      "g4SimHits"                 ""                "SIMFULL"
vector<SimVertex>                     "g4SimHits"                 ""                "SIMFULL"
```

No product of process `SIM` is left. The largest branches of the signal file (`edmEventSize -v`, bytes on disk for the one
event): `TrackerHitsTECLowTof` 200 551, `SimTracks` 160 460, `EcalHitsEE` 153 366, `generatorSmeared` 104 813, `EcalHitsEB`
80 680, `TrackerHitsTIBLowTof` 48 720, `TrackerHitsTOBLowTof` 42 317, `TrackerHitsPixelEndcapLowTof` 41 971, `genParticles`
41 860, `TrackerHitsPixelBarrelLowTof` 41 370, `TrackerHitsTIDLowTof` 40 296, `SimVertexs` 27 921, `HcalHits` 26 874,
`EcalHitsES` 15 618, `TrackerHitsTECHighTof` 6861. File sizes: `step2_SIM_B` 0.58 MB, `B_Decay` 0.60 MB, `B_Decay_EM`
0.91 MB, `signal` 1.19 MB, `pileup` 226 MB (1000 events; 59.6 MB for the 250 events until 21 November 2026).

`edmDumpEventContent file:step3_MIX_10_SIM.root`: the `GEN` products `generator`, `LHCTransport`, `generatorSmeared`,
`randomEngineStateProducer`, `TriggerResults` (the `genParticles`, `genMet*` and the `GENJET` jets are dropped by the
`inputCommands` of the DIGI source; `TriggerResults` of `GENJET` stays), all the `SIMFULL` products above, and two
new ones:

```
CrossingFramePlaybackInfoNew     "mix"                       ""                "MIX"
edm::TriggerResults              "TriggerResults"            ""                "MIX"
```

## 3. The analyzer configs

```bash
cd $J/test/10_SIM/out
# the five SimAnalyzer pages per stage (+ the five generator pages for the signal stage), one PDF each
for stage in B B_Decay B_Decay_EM signal; do
  case $stage in B) sw="B field on, Decay off|EM off, Had off"; n=2;; B_Decay) sw="B field on, Decay on|EM off, Had off"; n=3;;
    B_Decay_EM) sw="B field on, Decay on|EM on, Had off"; n=4;; signal) sw="B field on, Decay on|EM on, Had on"; n=5;; esac
  gen=0; [[ $stage == signal ]] && gen=5
  cmsRun ../../common/simAnalyzer_cfg.py inputFiles=file:$J/test/10_SIM/out/step2_SIM_${stage}_10_SIM.root \
    outputFile=simAnalyzer_${stage}_10_SIM.root maxEvents=1 eventsToProcess=1:1:1 simStage=$n genStages=$gen \
    globalTag=auto:phase1_2025_realistic era=Run3_2025 geometry=DB:Extended pdfFile=simAnalyzer_${stage}_10_SIM.pdf \
    sampleLabel="chapter 10_SIM: stage $stage|$sw|bbbar pTHat > 100 GeV, 13.6 TeV" > simAnalyzer_${stage}_10_SIM.log 2>&1
  pdftoppm -cropbox -r 110 -png simAnalyzer_${stage}_10_SIM.pdf simAnalyzer_${stage}_10_SIM_page
done

# the three samples station by station (SimHitAnalyzer): TRK, ECAL, HCAL, MUON, ALL; r-z, x-y and eta-phi views
ARGS="era=Run3_2025 globalTag=auto:phase1_2025_realistic geometry=DB:Extended highlight=1 highlightPtMin=2 savePNG=1 printHits=0 maxEvents=-1"
cmsRun ../../common/simHitAnalyzer_cfg.py inputFiles=file:$J/test/10_SIM/out/step2_SIM_signal_10_SIM.root outputDir=$J/test/10_SIM/out/signal \
  prefix=simHits_signal_10_SIM eventsToProcess=1:1:1 $ARGS \
  sampleLabel="chapter 10_SIM (10a): the signal|bbbar, pTHat > 100 GeV|full Geant4 physics, 2025 detector|no pileup" > simHits_signal_10_SIM.log 2>&1
cmsRun ../../common/simHitAnalyzer_cfg.py inputFiles=file:$J/test/10_SIM/out/step2_SIM_pileup_10_SIM.root outputDir=$J/test/10_SIM/out/pileup \
  prefix=simHits_pileup_10_SIM eventsToProcess=1:1:747 $ARGS \
  sampleLabel="chapter 10_SIM (10b): one pileup collision|library event 747, full Geant4 physics|SoftQCD inelastic, 13.6 TeV|2025 detector, no signal" > simHits_pileup_10_SIM.log 2>&1
# the mixed sample is drawn inside the mixing job (section 2); its analyzer lines are extracted from the job log:
grep -E "^SimHitAnalyzer|^  pileup mixed|^    pileup event|^  station|^    hits by|^    primaries with hits" step3_MIX_10_SIM.log > simHits_mixed_10_SIM.log
# the ledger
python3 ../../common/simLedger.py $J/test/10_SIM/out 10_SIM $J/test/09_G4/out/simAnalyzer_signal_09_G4.log > ledger_10_SIM.txt
# the FWLite extras (no cmsRun; about 10 s): decays, conversions, nuclear interactions, the b-system budget, the
# followed photon in two runs, the vertices of the mixing; the last step of run_10_SIM.sh
cd $J/test && python3 10_SIM/simExtras_10_SIM.py > 10_SIM/out/extras_10_SIM.txt 2> 10_SIM/out/extras_10_SIM.log
```

`simAnalyzer_cfg.py` (chapter 09, section 3, for the options): `simStage=N` highlights the stage in the right panel of every
page (1 transport, 2 + B, 3 + decays, 4 + EM, 5 + hadronic); `genStages=5` draws the five generator pages first (signal
stage only). `simHitAnalyzer_cfg.py`: `highlight=1` labels the leading primary of every kind (above `highlightPtMin=2` GeV)
next to its hits and circles its calorimeter cells (the hits are not joined into lines: those of one primary come from
its whole family of descendants, and tracks belong to the reconstruction chapters); `savePNG=1` writes every page as PNG next to the PDF, rendered from the closed PDF
by `pdftoppm -cropbox` (`plugins/PdfPng.h`: the PNG keeps ROOT's size, 1396 x 872 pixels, and no resolution chunk, so the
trims of the chapter stay valid; ROOT's own PNG painted the 2.1 million hits of the mixed event for hours); `printHits=1` would print every
primary with its hit counts per station; `stations=TRK,ALL` would restrict the stations. The global tag, era and geometry
give the reco geometry (`GeometryRecoDB`) for the hit positions and the detector elements drawn in grey. Set in
`python/simHitAnalyzer_cff.py`, not on the command line: `rUpDownMax=300` for the station `ALL` (a fourth view,
`rzUpDown`: r-z with r signed by the half of the detector, + for y > 0, - for y < 0, |r| < 300 cm, both halves of
CMS apart; the muon chambers beyond are left out, they hold 838 hits of the mixed event and none of the signal);
`binPileupAbove=100000` and `pileupBins=120` for the mixing job (Constitution, Rule K1): a station with more pileup
hits and cells than that, TRK (2.10 million), ECAL (158 thousand) and ALL, draws its pileup binned, 120 bins across
the frame and square bins on the page, one box per bin whose area is log10(1 + N) / log10(1 + Nmax) of the bin, filled
by the three bunch-crossing classes as nested squares with visible areas in proportion to their counts (early outside,
late in the middle, in-time in the centre); HCAL (15.7 thousand) and MUON (838) keep the markers. The page header is
one line, the hits and cells grouped by detector (Tracker, ECAL, HCAL, Muon) with the subdetectors in brackets, rounded
to K and M; the exact numbers are in the logs.

What each run produces in `out/`:

| file | content |
|---|---|
| `simAnalyzer_<stage>_10_SIM.pdf`, `_page-N.png` | for `B`, `B_Decay`, `B_Decay_EM` five pages: 1 the primaries handed to Geant4 (by kind, the 63 stable particles without `SimTrack` as crosses), 2 the primaries at the tracker/calorimeter boundary (marker at the crossing point, line from the production direction, open marker if not reaching it), 3 the secondaries by creator process (decay, ionisation, bremsstrahlung, conversion, Compton, annihilation, hadron elastic / inelastic), 4 the tracker and muon `PSimHit`s by subdetector, 5 the `PCaloHit`s summed per cell with the marker size growing with the energy, the b and bbar systems in red and blue on every page. For `signal` ten pages: the five generator stages of chapters 01 to 06 first (`_page-01..05`), then the five SIM pages (`_page-06..10`) |
| `simAnalyzer_<stage>_10_SIM.log` | the per-page counts quoted in the chapter (section 4) |
| `simAnalyzer_<stage>_10_SIM.root` | `TFileService` histograms per page |
| `signal/<station>/simHits_signal_10_SIM_<station>_run1_event1.pdf`, `_<station>_{rz,xy,etaphi}_run1_event1.png` | (10a) one PDF of three pages per station (`TRK`, `ECAL`, `HCAL`, `MUON`, `ALL`): the r-z view, the transverse view of the barrel, the eta-phi view from the primary vertex; `ALL` has four, the r-z view of both halves (`_ALL_rzUpDown_`, \|r\| < 300 cm) second; the detector elements of the station in grey, the hits coloured by the kind of the primary they descend from, the leading primary per kind labelled; the header is the hits and cells by detector, the subdetectors in brackets, K / M rounded |
| `pileup/<station>/simHits_pileup_10_SIM_<station>_..._run1_event747.pdf/.png` | (10b) the same for library event 747 |
| `mixed/<station>/simHits_mixed_10_SIM_<station>_..._run1_event1.pdf/.png` | (10a+10b) the same for the mixed event: the signal hits by kind on top, the pileup underneath coloured by bunch crossing (olive early, blue in time, red late); a calorimeter cell takes the colour of the origin that deposited most of its energy. `TRK`, `ECAL` and `ALL` draw the pileup binned (Rule K1, nested boxes, see above), `HCAL` and `MUON` as markers |
| `simHits_signal_10_SIM.log`, `simHits_pileup_10_SIM.log`, `simHits_mixed_10_SIM.log` | per station: hits and cells, hits by subdetector and layer, hits by kind of the primary (cells and their energy for the calorimeters), primaries with hits, the energy by subdetector, the followed primaries (hits, cells and energy of each), and one line per view (`view rzUpDown`: hits within \|r\| < 300 cm; `view xy`: the barrel hits by kind, as in the legend of the x-y page; `view etaphi`: the primary vertex and the hits beyond the eta range); the mixed log adds the `signal and pileup` line (hits of the signal and of the pileup, binned or not), and also lists every mixed collision with its bunch crossing, primaries, `SimTrack`s, `SimVertex`s, vertex z and time, and the "events per bx" line the script checks against `SKIP` / `NPU` |
| `step3_MIX_10_SIM.log` | the full mixing job log (the analyzer lines above, the detector element extents); the same job draws the pages and writes `step3_MIX_10_SIM.root`, but the log names only the two input files it opens and closes, not the output file |
| `step3_MIX_10_SIM.root` | the signal event with the `CrossingFramePlaybackInfoNew` (section 2), the input of (11a+11b) |
| `ledger_10_SIM.txt` | the ledger of `common/simLedger.py`: table 1, the stages from the SimAnalyzer logs (with the transport-only run of chapter 09 as first line); table 2, the three samples station by station from the SimHitAnalyzer logs |
| `extras_10_SIM.txt` | the output of `test/10_SIM/simExtras_10_SIM.py` (FWLite on `test/09_G4/out/step2_G4_signal_09_G4.root`, the four `step2_SIM_<stage>_10_SIM.root`, `step2_SIM_pileup_10_SIM.root` and `simHits_mixed_10_SIM.log`): (1) the field, (2) the decays in flight, (3) conversions and bremsstrahlung, (4) nuclear interactions, (5) the b and bbar systems in the calorimeters, (6) the followed photon in two runs, (7) the vertices and primaries of the 995 mixed collisions, (8) the library collisions at z = +-203 m; section 4.8 |
| `step2_SIM_<stage>_10_SIM.log`, `step2_SIM_pileup_10_SIM.log` | the `cmsRun` logs of the SIM steps: the Geant4 banner and the event records |

`simLedger.py` parses `nReaching`, `nSecondaries`, `decay/EM/hadronic`, `nHits`, `sumELoss`, `nCells`, the `E by subdetector`
and the b-system entries of the SimAnalyzer logs with regular expressions, and the `station` blocks of the SimHitAnalyzer
logs; no ROOT is needed. The `primaries handed to Geant4` of the SimHitAnalyzer are the `SimTrack`s with
`genpartIndex() > 0` (see the reconciliation in section 4.5).

## 4. The tables

### 4.1 `ledger_10_SIM.txt`

```
== simLedger 10_SIM (/afs/cern.ch/work/r/rverma/public/JME/JitJet/CMSSW_15_0_5/src/JitJetSW/Journey/test/10_SIM/out)
-- the Geant4 physics switched on stage by stage (SimAnalyzer, event 1)
stage            primaries reachCal secondaries (decay/EM/had)          trk+mu hits eLoss[GeV]  caloCell E EB/EE/HB/HE/HF               b system: pt0 ptCal E_ECAL E_HCAL
Transport (09)    571/634       557           0 (0/0/0)                        6734          0       459 0/0/0.00/0.00/1511             56.8234 56.8234 0 0
B                 571/634       552           0 (0/0/0)                        7019          0       454 0/0/0.00/0.00/1533             56.8234 56.6345 0 0
B_Decay           571/634       507          66 (66/0/0)                       7304          0       451 0/0/0.00/0.00/1607             56.8234 48.6762 0 0
B_Decay_EM        571/634       378         733 (67/666/0)                     7861    1.16607      8467 47/191/0.15/1.34/1599          56.8234 25.9738 77.9357 0.0421658
signal            571/634       310        1355 (196/773/386)                 10272    2.33358     14157 68/294/0.22/1.66/1849          56.8234 24.3503 63.5766 0.385802
-- the three samples, station by station (SimHitAnalyzer)
sample signal: SimHitAnalyzer run 1 event 1: 1926 SimTracks, 571 primaries handed to Geant4, PV = (0.0276598, -0.0169037, 3.11193) cm
  TRK   10272 PSimHits, 0 PCaloHits in 0 cells
        by kind: charged hadron=4999 neutral hadron=890 photon=2013 e^{#pm}=138 no stored ancestor=2232
        primaries with hits: 293 of 571
  ECAL  0 PSimHits, 24262 PCaloHits in 12804 cells
        by kind: charged hadron=5343(144 GeV) neutral hadron=1810(47.4 GeV) photon=5287(162 GeV) e^{#pm}=342(7.11 GeV) no stored ancestor=22(0.871 GeV)
        primaries with hits: 366 of 571
  HCAL  0 PSimHits, 2531 PCaloHits in 1353 cells
        by kind: charged hadron=930(1.28e+03 GeV) neutral hadron=215(314 GeV) photon=158(206 GeV) e^{#pm}=2(0.000731 GeV) no stored ancestor=48(53 GeV)
        primaries with hits: 266 of 571
  MUON  0 PSimHits, 0 PCaloHits in 0 cells
        by kind:
        primaries with hits: 0 of 571
  ALL   10272 PSimHits, 26793 PCaloHits in 14157 cells
        by kind: charged hadron=11272(1.42e+03 GeV) neutral hadron=2915(362 GeV) photon=7458(368 GeV) e^{#pm}=482(7.11 GeV) no stored ancestor=2302(53.9 GeV)
        primaries with hits: 512 of 571
sample pileup: SimHitAnalyzer run 1 event 747: 55 SimTracks, 22 primaries handed to Geant4, PV = (0.0276303, -0.0177769, 2.1553) cm
  TRK   136 PSimHits, 0 PCaloHits in 0 cells
        by kind: charged hadron=69 photon=40 no stored ancestor=27
        primaries with hits: 6 of 22
  ECAL  0 PSimHits, 173 PCaloHits in 131 cells
        by kind: charged hadron=70(0.798 GeV) photon=60(0.293 GeV) no stored ancestor=1(0.00266 GeV)
        primaries with hits: 8 of 22
  HCAL  0 PSimHits, 90 PCaloHits in 72 cells
        by kind: charged hadron=66(87 GeV) photon=6(11 GeV)
        primaries with hits: 13 of 22
  MUON  0 PSimHits, 0 PCaloHits in 0 cells
        by kind:
        primaries with hits: 0 of 22
  ALL   136 PSimHits, 263 PCaloHits in 203 cells
        by kind: charged hadron=205(87.8 GeV) photon=106(11.3 GeV) no stored ancestor=28(0.00266 GeV)
        primaries with hits: 18 of 22
sample mixed: SimHitAnalyzer run 1 event 1: 1926 SimTracks, 571 primaries handed to Geant4, PV = (0.0276598, -0.0169037, 3.11193) cm
  pileup mixed in (CrossingFrame, bunch crossings -12 .. 3): 995 events (early 746, in-time 64, late 185), 149894 primaries (114093, 8804, 26997), 463662 SimTracks; events per bx: -12:71 -11:68 -10:54 -9:59 -8:52 -7:61 -6:76 -5:59 -4:65 -3:59 -2:68 -1:54 0:64 1:63 2:62 3:60
  TRK   2106681 PSimHits, 0 PCaloHits in 0 cells (signal: 10272 PSimHits, 0 PCaloHits)
        by kind: charged hadron=4999 neutral hadron=890 photon=2013 e^{#pm}=138 no stored ancestor=2232 pileup, early (bx < 0)=1588361 pileup, in-time (bx 0)=125625 pileup, late (bx > 0)=382423
        primaries with hits: 293 of 571
  ECAL  0 PSimHits, 4377663 PCaloHits in 158847 cells (signal: 0 PSimHits, 24262 PCaloHits; 699 cells dominated by the signal, 158148 by pileup)
        by kind: charged hadron=237(21.8 GeV) neutral hadron=101(11.6 GeV) photon=341(111 GeV) e^{#pm}=20(6.07 GeV) pileup, early (bx < 0)=129238(5.29e+04 GeV) pileup, in-time (bx 0)=6980(797 GeV) pileup, late (bx > 0)=21930(2.73e+03 GeV)
        primaries with hits: 182 of 571
  HCAL  0 PSimHits, 459290 PCaloHits in 15791 cells (signal: 0 PSimHits, 2531 PCaloHits; 69 cells dominated by the signal, 15722 by pileup)
        by kind: charged hadron=60(313 GeV) neutral hadron=7(58.1 GeV) photon=1(0.0024 GeV) no stored ancestor=1(0.000496 GeV) pileup, early (bx < 0)=13395(2.46e+05 GeV) pileup, in-time (bx 0)=553(28.2 GeV) pileup, late (bx > 0)=1774(406 GeV)
        primaries with hits: 38 of 571
  MUON  838 PSimHits, 0 PCaloHits in 0 cells (signal: 0 PSimHits, 0 PCaloHits)
        by kind: pileup, early (bx < 0)=639 pileup, in-time (bx 0)=75 pileup, late (bx > 0)=124
        primaries with hits: 0 of 571
  ALL   2107519 PSimHits, 4836953 PCaloHits in 174638 cells (signal: 10272 PSimHits, 26793 PCaloHits; 768 cells dominated by the signal, 173870 by pileup)
        by kind: charged hadron=5296(334 GeV) neutral hadron=998(69.7 GeV) photon=2355(111 GeV) e^{#pm}=158(6.07 GeV) no stored ancestor=2233(0.000496 GeV) pileup, early (bx < 0)=1731633(2.98e+05 GeV) pileup, in-time (bx 0)=133233(825 GeV) pileup, late (bx > 0)=406251(3.14e+03 GeV)
        primaries with hits: 353 of 571
```

Reading the "by kind" lines: for the tracker and muon stations the numbers are hits; for the calorimeter stations they are
cells (attributed to the kind of the primary that deposited most of the cell's energy) with the energy of those cells in
parentheses; `ALL` adds the two. The HCAL "GeV" add the visible energy of HB, HE, HO (GeV) and the HF shower-library units
(photoelectrons), as the analyzer sums them. In the chapter: the mixed-event ECAL energies 150 GeV (signal, 21.8 +
11.6 + 111 + 6.07) and 56 427 GeV (pileup, 52 900 + 797 + 2730), total 56 577, 56 600 in the ledger table of the chapter (the line
`energy by subdetector` of station `ECAL` in `simHits_mixed_10_SIM.log`, EB 12 136 + EE 44 419 + ES 43.4 = 56 598; the library sums of section 4.5, 12 068 + 44 126 + 43 = 56 237,
plus the signal's 362 = 56 599); the HCAL 371 (signal, 313 + 58.1 + 0.0024 + 0.0005) and 246 434 (pileup, 246 000 + 28.2
+ 406), total 246 805, 246 800 in the table; the tracker pileup hits 1 588 361 + 125 625 + 382 423 = 2 096 409; per
collision 1 588 361 / 746 = 2129, 125 625 / 64 = 1963, 382 423 / 185 = 2067; the signal's share 10 272 / 2 106 681 =
0.49 % (of its own bunch crossing 10 272 / (10 272 + 125 625) = 7.6 %); the signal cells lost to the pileup 12 804 - 699
= 12 105; the early share of the pileup cells 129 238 / 158 148 = 82 %; EE 44 400 / 293.66 = 151 times the signal; the
in-time ECAL deposits of the library (section 4.5) 776.5 + 2650.6 + 2.6 = 3429.7 GeV in 91 425 + 159 272 + 11 522 =
262 219 hits, 3430 / 64 = 54 GeV per collision.

Tracker and muon `PSimHit`s of the library per bunch crossing of the draw (FWLite, `TrackerHits*` and `MuonDTHits`,
`MuonCSCHits`, `MuonRPCHits`, `MuonGEMHits` of `g4SimHits` summed over the library events of each crossing; the class
sums equal the mixed log, 1 588 361 / 125 625 / 382 423 and 639 / 75 / 124):

| bx | -12 | -11 | -10 | -9 | -8 | -7 | -6 | -5 | -4 | -3 | -2 | -1 | 0 | 1 | 2 | 3 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| tracker | 139 551 | 146 028 | 152 464 | 121 527 | 108 480 | 95 664 | 190 508 | 140 200 | 122 216 | 128 879 | 130 258 | 112 586 | 125 625 | 153 911 | 116 155 | 112 357 |
| muon | 51 | 129 | 24 | 74 | 80 | 77 | 43 | 33 | 64 | 7 | 12 | 45 | 75 | 72 | 31 | 21 |

The first three columns are the tracker and muon hits of the bunch crossings -1, 0, +1 of the earlier version (139 551,
146 028, 152 464; 51, 129, 24). Older than 75 ns (bx -12 to -4, library events 1-565): 1 216 638 tracker hits and 575 muon
hits (DT 67, CSC 465, RPC 8, GEM 35), the numbers of the open problem "The mixing window of Phase-2".

### 4.2 The SimAnalyzer logs, page lines (`simAnalyzer_<stage>_10_SIM.log`)

Stage `B` (`nSimTracks=571 nSimVertices=29`):
```
  page 1 (primaries): nPrimaries=571 nStatus1=634 noSimTrack=63 (eta 56, p 6, nu 1, other 0) PV=(0.0276598,-0.0169037,3.11193) | b system: n=8/9 pt=56.8234 eta=-1.55313 phi=-2.08847 | bbar system: n=16/16 pt=114.174 eta=-3.74697 phi=1.01039 | net |sum pT|=57.4538
  page 2 (calo surface): nReaching=552 nNotReaching=19 | b system at the boundary: n=8/8 pt=56.6345 eta=-1.55618 phi=-2.06419 | bbar system at the boundary: n=16/16 pt=114.176 eta=-3.74695 phi=1.00878 | net |sum pT|=57.8053
  page 3 (secondaries): nSecondaries=0 decay=0 EM=0 hadronic=0 other=0 by process: | b system: n=0 (decay 0, EM 0, had 0) | bbar system: n=0 (decay 0, EM 0, had 0)
  page 4 (tracker/muon hits): nHits=7019 tracker=7019 muon=0 sumELoss[GeV]=0 noGeometry=0 noSimTrack=0 bHits=176 bbarHits=0 by subdetector: PXB=981 PXF=853 TIB=1202 TID=744 TOB=718 TEC=2521
  page 5 (calo hits): nHits=737 nCells=454 noGeometry=0 E by subdetector: HF=1533(454) | b system: E_ECAL=0 E_HCAL=0 E_HF=0 nCells=0 | bbar system: E_ECAL=0 E_HCAL=0 E_HF=797 nCells=38
```
Stage `B_Decay` (`nSimTracks=637 nSimVertices=70`):
```
  page 2 (calo surface): nReaching=507 nNotReaching=64 | b system at the boundary: n=7/8 pt=48.6762 eta=-1.54787 phi=-2.06483 | bbar system at the boundary: n=16/16 pt=114.176 eta=-3.74695 phi=1.00878 | net |sum pT|=65.6955
  page 3 (secondaries): nSecondaries=66 decay=66 EM=0 hadronic=0 other=0 by process: decay=66 | b system: n=2 (decay 2, EM 0, had 0) | bbar system: n=0 (decay 0, EM 0, had 0)
  page 4 (tracker/muon hits): nHits=7304 tracker=7304 muon=0 sumELoss[GeV]=0 noGeometry=0 noSimTrack=0 bHits=180 bbarHits=0 by subdetector: PXB=983 PXF=900 TIB=1248 TID=778 TOB=687 TEC=2708
  page 5 (calo hits): nHits=759 nCells=451 noGeometry=0 E by subdetector: HF=1607(451) | b system: E_ECAL=0 E_HCAL=0 E_HF=0 nCells=0 | bbar system: E_ECAL=0 E_HCAL=0 E_HF=760 nCells=47
```
Stage `B_Decay_EM` (`nSimTracks=1304 nSimVertices=474`):
```
  page 2 (calo surface): nReaching=378 nNotReaching=193 | b system at the boundary: n=5/8 pt=25.9738 eta=-1.55408 phi=-2.07714 | bbar system at the boundary: n=14/16 pt=110.429 eta=-3.74816 phi=1.00878 | net |sum pT|=84.5079
  page 3 (secondaries): nSecondaries=733 decay=67 EM=666 hadronic=0 other=0 by process: decay=67 ionisation (#delta ray)=20 bremsstrahlung=213 pair production / conversion=430 Compton scattering=3 | b system: n=49 (decay 6, EM 43, had 0) | bbar system: n=6 (decay 2, EM 4, had 0)
  page 4 (tracker/muon hits): nHits=7861 tracker=7339 muon=522 sumELoss[GeV]=1.16607 noGeometry=0 noSimTrack=970 bHits=149 bbarHits=0 by subdetector: PXB=897 PXF=778 TIB=1050 TID=833 TOB=473 TEC=3308 DT=7 CSC=448 RPC=30 GEM=37
  page 5 (calo hits): nHits=14246 nCells=8467 noGeometry=0 E by subdetector: EB=46.7952(3009) EE=190.504(3567) ES=0.124087(683) HB=0.150422(52) HE=1.33514(674) HO=0.00293081(1) HF=1599(481) | b system: E_ECAL=77.9357 E_HCAL=0.0421658 E_HF=0 nCells=428 | bbar system: E_ECAL=0 E_HCAL=0 E_HF=794 nCells=51
```
Stage `signal` (`nSimTracks=1926 nSimVertices=768`; pages 6 to 10 of the PDF):
```
  page 1 (primaries): nPrimaries=571 nStatus1=634 noSimTrack=63 (eta 56, p 6, nu 1, other 0) PV=(0.0276598,-0.0169037,3.11193) | b system: n=8/9 pt=56.8234 eta=-1.55313 phi=-2.08847 | bbar system: n=16/16 pt=114.174 eta=-3.74697 phi=1.01039 | net |sum pT|=57.4538
  page 2 (calo surface): nReaching=310 nNotReaching=261 | b system at the boundary: n=3/8 pt=24.3503 eta=-1.54703 phi=-2.07783 | bbar system at the boundary: n=13/16 pt=97.4011 eta=-3.74386 phi=1.01371 | net |sum pT|=73.0914
  page 3 (secondaries): nSecondaries=1355 decay=196 EM=773 hadronic=386 other=0 by process: decay=196 ionisation (#delta ray)=3 bremsstrahlung=275 pair production / conversion=488 Compton scattering=4 annihilation=3 hadron elastic=1 hadron inelastic=385 | b system: n=133 (decay 11, EM 105, had 17) | bbar system: n=89 (decay 29, EM 21, had 39)
  page 4 (tracker/muon hits): nHits=10272 tracker=10272 muon=0 sumELoss[GeV]=2.33358 noGeometry=0 noSimTrack=2232 bHits=615 bbarHits=203 by subdetector: PXB=929 PXF=973 TIB=1182 TID=975 TOB=1022 TEC=5191
  page 5 (calo hits): nHits=26793 nCells=14157 noGeometry=0 E by subdetector: EB=67.989(5521) EE=293.66(6214) ES=0.245266(1069) HB=0.217328(173) HE=1.66239(648) HO=3.73269e-06(1) HF=1849(531) | b system: E_ECAL=63.5766 E_HCAL=0.385802 E_HF=0 nCells=1452 | bbar system: E_ECAL=9.90505 E_HCAL=0.0251385 E_HF=987 nCells=671
```
Page 1 is identical in all four stages (the hand-over does not change). In the chapter: the secondaries of the B_Decay
stage sit in 70 - 29 = 41 new vertices, those of the signal stage in 768 - 29 = 739 (FWLite: 739 `SimVertex`s with a
stored secondary, by `processType()`: 201 decay 130, 14 conversion 268, 3 bremsstrahlung 189, 2 ionisation 3, 13 Compton 4,
5 annihilation 3, 121 hadron inelastic 141, 111 hadron elastic 1); the 385 / 1 of `hadron inelastic` / `hadron elastic` are
secondaries, from 141 + 1 = 142 vertices (section 4.8, (4)); 10 272 - 7339 = 2933 tracker hits more than the EM stage; the ECAL energy of the
signal 67.989 + 293.66 + 0.245 = 361.9 GeV; the HCAL 0.217 + 1.662 + 0.000 + 1849 = 1851 (GeV of HB, HE, HO plus the HF
units; the sum of the three-digit by-kind energies of the SimHitAnalyzer, 1280 + 314 + 206 + 0.0007 + 53 = 1853, is the
same number up to rounding); the looper threshold 0.3 x 3.8 T x 1.29 m / 2 = 0.735 GeV.

### 4.3 The SimHitAnalyzer logs, hits by subdetector and layer

`simHits_signal_10_SIM.log`, station `ALL`:
```
PXB/1=397 PXB/2=221 PXB/3=179 PXB/4=132 PXF/1=323 PXF/2=319 PXF/3=331 TIB/1=587 TIB/2=354 TIB/3=119 TIB/4=122 TID/1=290 TID/2=305 TID/3=380 TOB/1=427 TOB/2=294 TOB/3=99 TOB/4=68 TOB/5=57 TOB/6=77 TEC/1=596 TEC/2=646 TEC/3=705 TEC/4=562 TEC/5=641 TEC/6=649 TEC/7=469 TEC/8=523 TEC/9=400 EB=5521 EE=6214 ES=1069 HB=173 HE=648 HO=1 HF=531
```
`simHits_pileup_10_SIM.log` (library event 747), station `ALL`:
```
PXB/1=3 PXB/2=5 PXF/1=6 PXF/2=7 PXF/3=6 TID/1=5 TID/2=9 TID/3=16 TOB/1=1 TOB/2=13 TOB/4=1 TEC/1=6 TEC/2=9 TEC/3=16 TEC/4=3 TEC/5=5 TEC/6=6 TEC/7=5 TEC/8=7 TEC/9=7 EB=60 EE=61 ES=10 HB=1 HE=17 HF=54
```
The energies, line `energy by subdetector` of station `ALL` in the same log: `EB=0.232 GeV EE=0.86 GeV ES=0.00141 GeV
HB=0.000649 GeV HE=0.0413 GeV HF=98 HF shower-library units`; 0.232 + 0.86 + 0.001 = 1.09 GeV of ECAL energy, 0.0006 +
0.041 + 98 = 98.0 HCAL units; 339 hits and cells = 136 PSimHits + 203 cells (the page header: `339 = Tracker 136 hits
(PXB 8, PXF 19, TID 30, TOB 15, TEC 64) + ECAL 131 cells (EB 60, EE 61, ES 10) + HCAL 72 cells (HB 1, HE 17, HF 54)`). No hit in TIB or in the muon system: the collision is single diffractive, its
charged particles leave at small angles (TID, TEC and PXF hold 113 of the 136 tracker hits, HF 54 of the 72 HCAL cells).

`simHits_mixed_10_SIM.log`, station `ALL`:
```
PXB/1=89824 PXB/2=62169 PXB/3=49310 PXB/4=41947 PXF/1=53296 PXF/2=58008 PXF/3=61808 TIB/1=135948 TIB/2=112600 TIB/3=47183 TIB/4=40499 TID/1=57198 TID/2=60106 TID/3=63394 TOB/1=111338 TOB/2=95803 TOB/3=39951 TOB/4=33408 TOB/5=29258 TOB/6=23397 TEC/1=107974 TEC/2=113398 TEC/3=119294 TEC/4=90905 TEC/5=94783 TEC/6=99128 TEC/7=73139 TEC/8=77488 TEC/9=64127 DT/1=62 DT/2=8 CSC/1=432 CSC/2=221 CSC/3=39 CSC/4=18 RPC/1=8 RPC/4=1 GEM/1=48 GEM/2=1 EB=61200 EE=14648 ES=82999 HB=6856 HE=5464 HO=66 HF=3405
```
The other lines of station `ALL` in `simHits_mixed_10_SIM.log`: `energy by subdetector: EB=12136 GeV EE=44419 GeV ES=43.4
GeV HB=43.7 GeV HE=280 GeV HO=0.0614 GeV HF=246009 HF shower-library units`; `signal and pileup: 11040 hits and cells of the
signal event, 2271117 of the 995 mixed pileup events (149894 pileup primaries); pileup drawn binned (more than 100000, Rule
K1)`; by kind `pileup, early (bx < 0)=1731633`, `pileup, in-time (bx 0)=133233`, `pileup, late (bx > 0)=406251` (the
legend of the r-z page, rounded: `1.73M`, `133K`, `406K`); the followed primaries `#pi^{+} 30.6 GeV`, `K_{S}^{0} 15.7 GeV`,
`#gamma 17.7 GeV`, `e^{-} 5.0 GeV`, labelled on the pages with the pT rounded, `π+ 31 GeV`, `K0S 16 GeV`, `γ 18 GeV`,
`e- 5 GeV`. The page header gives the hits rounded, `2.28M = Tracker 2.11M hits (PXB 243K, PXF 173K, TIB 336K, TID 181K,
TOB 333K, TEC 840K) + ECAL 159K cells (EB 61.2K, EE 14.6K, ES 83.0K) + HCAL 15.8K cells (HB 6.86K, HE 5.46K, HO 66, HF
3.40K) + Muon 838 hits (DT 70, CSC 710, RPC 9, GEM 49)` (in the
eta-phi view the labels of the photon and the electron overlap in the b jet). 2 282 157 = 2 107 519 PSimHits + 174 638
cells; 11 040 = 10 272 + 768, one in 207 (2 282 157 / 11 040 = 206.7); the tracker 243 250 + 173 112 + 336 230 + 180 698 +
333 155 + 840 236 = 2 106 681. In the eta-phi view the early crossings dominate 129 238 of the 158 847 ECAL cells (81 %,
"four in five") and 13 395 of the 15 791 HCAL cells (85 %, "five in six").

The per-collision list of the mixed log (995 lines, `pileup event <k> in bx <b>: <primaries> primaries, <SimTracks>
SimTracks, <SimVertices> SimVertices, vertex z = <z> cm, t = <t> ns`), first and last of the earliest, of the in-time
and of the latest bunch crossing:
```
    pileup event 1 in bx -12: 178 primaries, 662 SimTracks, 271 SimVertices, vertex z = -3.30 cm, t = -300.0 ns
    pileup event 71 in bx -12: 139 primaries, 445 SimTracks, 182 SimVertices, vertex z = -8.02 cm, t = -300.0 ns
    pileup event 1 in bx 0: 22 primaries, 55 SimTracks, 19 SimVertices, vertex z = 2.16 cm, t = 0.0 ns
    pileup event 64 in bx 0: 285 primaries, 890 SimTracks, 348 SimVertices, vertex z = -3.24 cm, t = 0.0 ns
    pileup event 1 in bx 3: 55 primaries, 138 SimTracks, 52 SimVertices, vertex z = -1.80 cm, t = 75.0 ns
    pileup event 60 in bx 3: 167 primaries, 569 SimTracks, 222 SimVertices, vertex z = -3.07 cm, t = 75.0 ns
```
Event 1 of bx 0 is library event 747 (22 primaries, 55 SimTracks, z = 2.16 cm, as in the pileup sample); the first two
lines are the first and the last line of bx -1 of the earlier three-crossing version (library events 1 and 71, then at
t = -25 ns); the pileup times are the bunch-crossing shift.

### 4.4 FWLite extras: time-of-flight windows and calorimeter times of the signal

`python3` with `from DataFormats.FWLite import Events, Handle` on `step2_SIM_signal_10_SIM.root` (the script is in the
companion's scratch, reproduced by the loop of chapter 09 section 4 with `timeOfFlight()`, `energyLoss()`, `time()`):

| collection | LowTof hits, tof range [ns], energy loss [GeV] | HighTof hits, tof range [ns], energy loss [GeV] |
|---|---|---|
| `TrackerHitsPixelBarrel` | 915, 0.2-21.8, 0.1867 | 14, 36.9-469.5, 0.0027 |
| `TrackerHitsPixelEndcap` | 958, 1.1-26.8, 0.1137 | 15, 37.2-257.0, 0.0072 |
| `TrackerHitsTIB` | 1152, 1.0-35.8, 0.2750 | 30, 36.3-252.2, 0.0235 |
| `TrackerHitsTID` | 959, 2.8-35.7, 0.1600 | 16, 39.8-83.8, 0.0028 |
| `TrackerHitsTOB` | 984, 2.3-36.1, 0.3766 | 38, 37.1-499.0, 0.0078 |
| `TrackerHitsTEC` | 5095, 4.3-36.2, 1.1429 | 96, 36.2-441.0, 0.0346 |
| total | 10 063 | 209 |

`SimTracks 1926, genpartIndex > 0: 571, noVertex: 0, SimVertices 768`; muon collections all empty; `EcalHitsEB: 7273 hits,
E sum 67.989, tmax 481.5 ns; EcalHitsEE: 15867 hits, E 293.660, tmax 497.0 ns; EcalHitsES: 1122 hits, E 0.245, tmax 25.5 ns;
HcalHits: 2531 hits, E sum 1850.880 (GeV + HF units), tmax 498.9 ns; ZDCHITS, CaloHitsTk: 0 hits`. 1862 of the 7273 EB
hits (3.34 GeV of the 68.0) are later than 25 ns. The smallest hit energies, EB 1.52e-05 GeV and EE 1.00e-05 GeV (6562 of
the 7273 EB hits are below 15 MeV), show the `CaloSD.EminHits` thresholds of 15 keV and 10 keV (section 4.8, last table). The ECAL hit counts (7273 + 15 867 + 1122 = 24 262) and the cell counts
(5521 + 6214 + 1069 = 12 804) agree with the SimHitAnalyzer.

### 4.5 FWLite extras: the library and the reconciliation with chapter 09

Over the 1000 events of `step2_SIM_pileup_10_SIM.root` (FWLite, 21 November 2026; `genpartIndex() > 0` for the
primaries, the twelve `TrackerHits*` collections, the calorimeter collections of `g4SimHits`): `primaries 150266,
SimTracks 464653, tracker PSimHits 2101876, EcalHitsEB 1514223 hits (12088.1 GeV), EcalHitsEE 2657171 hits (44222.2
GeV), EcalHitsES 191874 hits (43.2 GeV), HcalHits 458101 (245080.1 GeV + HF units)`; the same loop on
`test/09_G4/out/step2_G4_pileup_09_G4.root` gives `150266 primaries, 150266 SimTracks, 1469994 tracker crossings`. By the
classes of the draw (`test/08_PU/out/draw_08_PU.txt`), full physics:

| library events | collisions | primaries | of which PPS protons (vertex at \|z\| > 100 m) | SimTracks | tracker hits | EB hits (GeV) | EE hits (GeV) | HCAL hits |
|---|---|---|---|---|---|---|---|---|
| 1-746, bx -12..-1 | 746 | 114 093 | 271 | 354 098 | 1 588 361 | 1 146 846 (9134.5) | 2 008 061 (33 410.2) | 349 246 |
| 747-810, bx 0 | 64 | 8804 | 18 | 26 785 | 125 625 | 91 425 (776.5) | 159 272 (2650.6) | 26 556 |
| 811-995, bx 1..3 | 185 | 26 997 | 70 | 82 779 | 382 423 | 273 080 (2157.0) | 483 363 (8064.7) | 80 957 |
| 996-1000, not drawn | 5 | 372 | 1 | 991 | 5467 | 2872 (20.1) | 6475 (96.7) | 1342 |

Transport only (chapter 09 file), tracker crossings 1 114 158 / 88 633 / 263 541 / 3662 for the same four classes. The
smallest and the largest events of the library have 1 (event 12) and 2669 (event 760, in time) SimTracks. Library event
747: `22 primaries (one a proton of 6800 GeV at z = -20300 cm), 55 SimTracks, 136 tracker hits, EcalHitsEB 79 hits
(0.232 GeV), EcalHitsEE 84 (0.860 GeV), EcalHitsES 10, HcalHits 90 (98.0 GeV + HF units)`. The first 250 events are those
of the 250-event library of the earlier version of the chapter: SimTracks, `TrackerHitsTECLowTof` and the `EcalHitsEE`
energy compared event by event, 0 differences.

The 64 in-time collisions: 8804 primaries = the 8786 primaries of chapter 09 (`ledger_09_G4.txt`, `g4Ledger.py`, which
counts the PPS protons apart) + 18 forward protons of 3307 to 6800 GeV (104 857 GeV in all) that `LHCTransport` hands
to Geant4 at z = +-203 m (section 4.8, (8)); the `SimHitAnalyzer` counts every `SimTrack` with `genpartIndex() > 0`. (In
the earlier version, library events 72 to 139 of bx -1..1: 10 414 = 10 387 + 27; the `SimTrack` link of a PPS proton
is the barcode of some non-status-1 particle of `generatorSmeared`, which is why `g4Ledger.py` matches them by energy
since 13 November 2026.)

Primaries per collision by bunch crossing (FWLite on the library, the ranges of the draw; the same numbers are in section
(7) of `extras_10_SIM.txt`, from the mixed log):

| bx | -12 | -11 | -10 | -9 | -8 | -7 | -6 | -5 | -4 | -3 | -2 | -1 | 0 | 1 | 2 | 3 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| collisions | 71 | 68 | 54 | 59 | 52 | 61 | 76 | 59 | 65 | 59 | 68 | 54 | 64 | 63 | 62 | 60 |
| median primaries | 69 | 114.5 | 148.5 | 100 | 78 | 65 | 100 | 161 | 90 | 99 | 104 | 80 | 84 | 104 | 77.5 | 76.5 |
| primaries | 10 012 | 10 414 | 10 634 | 8452 | 7926 | 7045 | 13 204 | 10 446 | 9017 | 9161 | 9813 | 7969 | 8804 | 10 972 | 8118 | 7907 |

All 995: median 90, from 1 to 754, 149 894 primaries; the in-time crossing averages 137.6 per collision against 150.6.

### 4.6 The release parameters quoted in "Open problems for Phase-2"

The state of the release CMSSW_15_0_5 itself (not of the CMSSW development branch), all under `$CMSSW_RELEASE_BASE/src`
(`/cvmfs/cms.cern.ch/el9_amd64_gcc12/cms/cmssw/CMSSW_15_0_5/src`). "Phase-2" is the default Phase-2 era of the release,
`Phase2C17I13M9` with geometry D110 (chapter 09 companion, section 4.6, for the reason and for the command that dumps the
effective `g4SimHits` of `Run3_2025` and `Phase2C17I13M9`); the same with `p.load('SimGeneral.MixingModule.mixObjects_cfi')`
and `cms.Process('X', era, premix_stage1)` (from `Configuration.ProcessModifiers.premix_stage1_cff`) gives the
premixing lists below.

| parameter | value | file (line) |
|---|---|---|
| `CaloSD.EminTrack`, `TmaxHit` | `1.0` GeV, `1000.0` ns | `SimG4Core/Application/python/g4SimHits_cfi.py` (382-383) |
| `CaloSD.HCNames`, `EminHits`, `TmaxHits` | `('EcalHitsEB','EcalHitsEE','EcalHitsES','HcalHits','ZDCHITS')`, `(0.015,0.010,0.0,0.0,0.0)` MeV (`SimG4CMS/Calo/src/CaloSD.cc` line 90: `eminHits[k] * CLHEP::MeV`, i.e. 15 keV and 10 keV), `(500,500,500,500,2000)`; unchanged in `Phase2C17I13M9` | same (384-387) |
| a collection not in `HCNames` (HGCal, HFNose) | `eminHit` stays `0.` (line 43), `tmaxHit = TmaxHit` = 1000 ns (line 59); the list is only searched at lines 87-99 | `SimG4CMS/Calo/src/CaloSD.cc` |
| `CaloHits` of `phase2_common` | `CalibrationHGCHitsEE/HEback/HEfront`, `CaloHitsTk`, `EcalHitsEB`, `HFNoseHits`, `HGCHitsEE/HEback/HEfront`, `HcalHits`, `ZDCHITS` (no `EcalHitsEE`, `EcalHitsES`) | `g4SimHits_cfi.py` (758) |
| `HGCSD.EminHit`, `TimeSliceUnit` | `0.0`, `0.001` ns; the scintillator SD takes the same `HGCSD.TimeSliceUnit` (`SimG4CMS/Calo/src/HGCScintSD.cc` line 41) | same (547-549) |
| scintillator materials | HGCal tiles `HGCalHEScintillatorSensitive`: `materials:H_Scintillator`; HB, HE, HO tiles of 2025: `materials:Scintillator`; both `density="1.032*g/cm3"`, Carbon 0.91512109, Hydrogen 0.084878906 in `Geometry/CMSCommonData/data/materials/2021/v1/materials.xml` (D110) and `2021/v4/materials.xml` (2025): the same plastic under two names | `Geometry/HGCalCommonData/data/hgcalHEmix/v17/hgcalHEmix.xml` (126-133, in D110); `Geometry/HcalCommonData/data/hcalbarrelalgo.xml` |
| `HGCScintSD.EminHit`, `UseBirkLaw`, `BirkC3`, `BirkC2`, `BirkC1` | `0.0`, `True`, `1.75`, `0.142`, `0.0060` | same (567-574) |
| `TotemT2ScintSD.BirkC3`, `BirkC2`, `BirkC1`; `AHCalSD.BirkC3`, `BirkC2`, `BirkC1` | `1.75`, `0.142`, `0.006`; `1.75`, `0.142`, `0.0052` | same (601-605, 664-668) |
| `HCalSD.UseBirkLaw`, `BirkC3`, `BirkC2`, `BirkC1`, comment | `True`, `1.75`, `0.142`, `0.0060`, "Values of Birks constants from NIM 80 (1970) 239-244: as implemented in Geant3 required correction due to biased computation of enery deposition" | same (426-432) |
| `HCalSD.UseShowerLibrary`, `HBDarkening`, `HEDarkening`, `HFDarkening` | `True`, `False`, `False`, `False` | same (433, 446-448) |
| `ECalSD.UseBirkLaw`, `BirkL3Parametrization`, `BirkSlope`, `BirkCut`, `BirkC1`, `BirkC3`, `BirkC2` | `True`, `True`, `0.253694`, `0.1`, `0.03333`, `1.0`, `0.0` (applied to the EB and the EE crystals, `useWeight`; not to the ES: `SimG4CMS/Calo/src/ECalSD.cc` lines 121, 205-210) | same (403-409) |
| `common_UseLuminosity.InstLuminosity`, `DelivLuminosity` | `0.`, `5000.` | same (50-53), used by `ECalSD` (402) and `HCalSD` (425) |
| `HFDarkeningParameterBlock` | the dose table, `numberOfZLayers = 33`, `numberOfRLayers = 13` (`SimG4CMS/Calo/interface/HFDarkening.h` lines 24-25); used only if `HFDarkening` (`SimG4CMS/Calo/src/HCalSD.cc` lines 96, 251-252) | `SimG4CMS/Calo/python/HFDarkeningParams_cff.py`, loaded at line 6 of `g4SimHits_cfi.py`, passed at line 456 |
| `HFShower.UseShowerLibrary`, `ApplyFiducialCut` | `True`, `True` | `g4SimHits_cfi.py` (469, 473) |
| `HFLibraryFileBlock.FileName`, `FileVersion`; `HFShowerBlock.EqualizeTimeShift` | `HFShowerLibrary_run3_v7.root`, `3`; `True` under `run3_HFSL` (default `HFShowerLibrary_oldpmt_noatt_eta4_16en_v3.root`, `0`, `False`) | `Geometry/HcalSimData/python/HFParameters_cff.py` |
| `run3_HFSL` in the eras | in `Run3` (`Configuration/Eras/python/Era_Run3_cff.py`, line 23); `Phase2 = ModifierChain(Run3_noMkFit.copyAndExclude([phase1Pixel,trackingPhase1,seedingDeepCore,displacedRegionalTracking,ctpps_2022,dd4hep]), phase2_common, ...)`; effective under `Phase2C17I13M9`: `HFShowerLibrary_run3_v7.root`, `FileVersion 3`, `EqualizeTimeShift True` | `Configuration/Eras/python/Era_Phase2_cff.py` |
| the library energies | `BranchEvt = ''` -> new format, tree `HFSimHits` with the branches `emParticles`, `hadParticles` only (159 999 entries); `loadEventInfo` without a branch: `nMomBin_ = 16`, `evtPerBin_ = 10000`, `pmom_ = {2, 3, 5, 7, 10, 15, 20, 30, 50, 75, 100, 150, 250, 350, 500, 1000}` GeV (line 568); above the last bin `extrapolate` adds `int(E/1000)+1` records of the top bin (lines 656-669) | `SimG4CMS/Calo/src/HFShowerLibrary.cc`; file `external/el9_amd64_gcc12/data/SimG4CMS/Calo/data/HFShowerLibrary_run3_v7.root` |
| `Physics.type`, `EminFTFP`, `EmaxBERT`, `EmaxBERTpi`, `DefaultCutValue`, `CutsPerRegion`, `G4BremsstrahlungThreshold`, `G4TrackingCut`, `LimitsPerRegion` | `SimG4Core/Physics/FTFP_BERT_EMM`, `3.`, `6.`, `12.`, `1.0` cm, `True`, `0.5` GeV, `0.025` MeV, `('EcalRegion','HcalRegion')` | `g4SimHits_cfi.py` (184-282) |
| era modifiers of `g4SimHits` | `run2_common` (ZdcSD), `run2_HCAL_2017` (TestNumberingScheme), `run3_common` (OnlySDs, TrackHits, CaloHits, LHCTransport), `phase2_timing` (ECalSD time slices), `h2tb`, `dd4hep`, `phase2_common` (OnlySDs with HGCal, MTD, HFNose; LHCTransport False; MuonSD.HaveDemoChambers False), `hgcaltb`, `phase2_hgcalOnly`, `phase2_hgcalV18` (HGCSD.HitCollection 2), `fixLongLivedSleptonSim`; `phase2_hgcal` on `common_MCtruth` (SaveCaloBoundaryInformation True) | `g4SimHits_cfi.py` (77-80, 675-800) |
| `mix` of `mix_POISSON_average_cfi` | `minBunch -5`, `maxBunch 3`, `bunchspace 450`, `input.type 'poisson'`, `sequential False`, `playback False` (overridden by `defineMixing`) | `SimGeneral/MixingModule/python/mix_POISSON_average_cfi.py` |
| `AVE_60_BX_25ns`, `AVE_200_BX_25ns`, `_m12p3`, `_m6p6` | `{'BX':25, 'B': (-12,3), 'N': 60}`, `{'BX':25, 'B': (-3,3), 'N': 200}`, `B (-12,3)`, `B (-6,6)`; `_m12p3` and `_m6p6` appear in no other python file of the release (`grep -rln`) | `Configuration/StandardSequences/python/Mixing.py` (162, 184-186) |
| `2025_LHC_Simulation_fill12p5h_levelling9h_nb2340_xSec80mb_il2p23` (the 2025 profile) | `probFunctionVariable 0..99` with its `probValue`s (mean 64.34, r.m.s. 11.70, median 65, mode 66; 34.1 % of the crossings with at most 60), on `mix_probFunction_25ns_PoissonOOTPU_cfi` (`minBunch -12`, `maxBunch 3`, `bunchspace 25`, `type 'probFunction'`); header comment "A distribution for Run3 studies (2023) derived from LHC simulations", 2340 colliding bunches, hadronic cross section 80 mb, fill 750 min, levelling 540 min | `SimGeneral/MixingModule/python/Run3_2025_LHC_Simulation_fill12p5h_levelling9h_nb2340_xSec80mb_il2p23_cfi.py`; `Configuration/StandardSequences/python/Mixing.py` (139) |
| `2024_25ns_RunIII2024Summer24_PoissonOOTPU` (the 2024 profile) | the same structure, mean 44.92, r.m.s. 9.53 (98.7 % of the crossings with at most 60) | `SimGeneral/MixingModule/python/mix_2024_25ns_RunIII2024Summer24_PoissonOOTPU_cfi.py`; `Mixing.py` (132) |
| `PUDataSets` of the relvals | `'Run4'`: `'--pileup':'AVE_200_BX_25ns', '--pileup_input':'das:/RelValMinBias_14TeV/.../GEN-SIM'`; 2022-2025: `'Run3_Flat55To75_PoissonOOTPU'` | `Configuration/PyReleaseValidation/python/relval_steps.py` (4607-4619) |
| premixing workflows | stage 1: `-s GEN,SIM,DIGI:pdigi_valid --datatier PREMIX --eventcontent PREMIX --procModifiers premix_stage1`; stage 2: `DIGI,DATAMIX --datamix PreMix --procModifiers premix_stage2` | `Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py` (`UpgradeWorkflowPremix`, 2311-2400) |
| `mixSimHits.pcrossingFrames` | `[]`, `premix_stage1`: `['MuonCSCHits','MuonDTHits','MuonRPCHits']`, `+ MuonGEMHits` (`run3_GEM`, `run2_GEM_2017`), `+ MuonME0Hits` (`phase2_muon`), removed again by `phase2_GE0`; effective under `Phase2C17I13M9` (which contains `phase2_GE0`) with `premix_stage1`: `['MuonCSCHits', 'MuonDTHits', 'MuonRPCHits', 'MuonGEMHits']` | `SimGeneral/MixingModule/python/mixObjects_cfi.py` (45-58, 234-278) |
| the muon digis of a premixed sample | stage 1 keeps the muon SimHits (`SimMuonPREMIX`: `keep *_mix_g4SimHitsMuonDTHits_*`, `..CSC..`, `..RPC..`, lines 48-53 of `SimMuon/Configuration/python/SimMuon_EventContent_cff.py`); stage 2: `premix_stage2` removes `muonDigiTask` from `doAllDigiTask` (`Configuration/StandardSequences/python/Digi_cff.py`), the `PreMixingModule` merges the crossing frames (`PreMixingCrossingFramePSimHitWorker` for `dt`, `rpc`, `csc`, `gem`), then `postDMDigi = cms.Task(ecalDigiTaskDM, hcalDigiTaskDM, muonDigiTask)` runs (line 59) | `Configuration/StandardSequences/python/DataMixerPreMix_cff.py` (59, 71); `SimGeneral/PreMixingModule/python/mixOne_premix_on_sim_cfi.py` (117-134, 214-219) |
| `theMixObjects` Phase-2 | `phase2_hgcal`: `mixCH` + `HGCHitsEE`, `HGCHitsHEfront`, `HGCHitsHEback`; `phase2_hfnose`: + `HFNoseHits`; `phase2_timing_layer`: `mixSH` + `FastTimerHitsBarrel`, `FastTimerHitsEndcap` | same (280-305) |
| `theDigitizers` premix | `(premix_stage2 & phase2_hgcal)`: `hgceeDigitizer`, `hgchebackDigitizer`, `hgchefrontDigitizer` `premixStage1 = True`, `calotruth premixStage1 = True` ("HGCAL still needs calotruth for production mode"); `(premix_stage2 & phase2_timing_layer)`: `mtdDigitizer` barrel/endcap `premixStage1 = True` | `SimGeneral/MixingModule/python/digitizers_cfi.py` (92-110) |
| `mixData` (`PreMixingModule`) | `input.type 'fixed'`, `averageNumber 1.0`, `sequential False`, `bunchspace 25`, `minBunch 0`, `maxBunch 0`, `workers.pileup.CFPlaybackInputTag = "mix"` | `SimGeneral/PreMixingModule/python/mixOne_premix_on_sim_cfi.py` |
| `premix_stage1`, `premix_stage2` | `cms.Modifier()` with the comments "the production of pre-mixed pileup library" / "mixing the premixed pileup library with the signal event" | `Configuration/ProcessModifiers/python/premix_stage{1,2}_cff.py` |
| the mixing order | `BMixingModule::produce`: `initializeEvent`, `addSignals`, `doPileUp`; `PileUp::CalculatePileup` | `Mixing/Base/src/BMixingModule.cc`, `Mixing/Base/src/PileUp.cc`, `SimGeneral/MixingModule/plugins/MixingModule.cc` (chapter 11, section 1, for the line numbers) |
| darkening switches searched | `grep -rn "HBDarkening\|HEDarkening\|HFDarkening\|DelivLuminosity"` over the release python: `g4SimHits_cfi.py`, test configurations (`False`), `SimCalorimetry/HcalSimProducers/python/hcalUnsuppressedDigis_cfi.py` (digitiser, `False`, `DelivLuminosity 0`) and `SLHCUpgradeSimulations/Configuration/python/aging.py` | release python |
| the HL-LHC ageing | `customise_aging_1000` / `_3000` (lines 245-258): `ageHcal` (`hcaldigi.DelivLuminosity`, "integrated lumi in fb-1", line 131; `g4SimHits.HCalSD.InstLuminosity`, `DelivLuminosity` only `if hasattr(process,'g4SimHits')`, comment "these lines need to be further activated by turning on 'complete' aging for HF", line 133; `ageHB`, `ageHE`, `ageHF` set `hcaldigi.HBDarkening`, `HEDarkening`, `HFDarkening`; then `turn_off_HE_aging`, "avoid conflict between HGCal and Hcal in phase2 geom configuration"), `ageEcal` (ECAL `TL{lumi}` tags); `hf_complete_aging` (lines 178-183): `g4SimHits.HCalSD.HFDarkening = cms.untracked.bool(True)`, called by no function of the file and by no other python file; `UpgradeWorkflowAging` applies `--customise SLHCUpgradeSimulations/Configuration/aging.customise_aging_<lumi>` to steps with `Digi` or `Reco` in their name, `Run4` only (`Aging1000`, `Aging3000`) | `SLHCUpgradeSimulations/Configuration/python/aging.py`; `Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py` (2087-2120) |
| tracked against untracked | `HCalSD.cc` line 96: `m_HC.getParameter<bool>("HFDarkening")` (tracked); `ParameterSet::retrieve`: "Parameter '...' is designated as tracked in the code, but is designated as untracked in the configuration file" (`StatusMismatch`); in python, after `hf_complete_aging`, `p.g4SimHits.HCalSD.HFDarkening.isTracked()` is `False`; the digitiser reads its `HFDarkening` tracked as well (`SimCalorimetry/HcalSimProducers/src/HcalDigitizer.cc` line 127; HF correction `HFRecalibration`, lines 262-263, 869-871) | `FWCore/ParameterSet/src/ParameterSet.cc` (228-242) |

The Geant4 version of every log: `geant4-11-02-patch-02 [MT] (21-June-2024)`.

**Moved from chapter 09 on 16 November 2026** (when chapter 09 became the hand-over only): the box "The HF shower
library" (section 3.5), table 10.3 of the production cuts (section 3.3) and the open problems "Physics list and Geant4
version", "The StackingAction thresholds and the HGCal production cut", "The time windows and the timing detectors"
(the parts of the last one already in "The calorimeter hit thresholds" were not repeated). Their release sources are
listed in the chapter 09 companion, section 4.6 (`g4SimHits_cfi.py` values, `StackingAction.cc` lines 341 and 399,
`RunManagerMT.cc` `runForPhase2()` lines 348 to 356, `CMSEmStandardPhysics.cc` lines 56-57, 112-113, 142-190,
`MtdSD.cc` lines 47-48, `FTFPCMS_BERT_EMM.cc`, `hgcProdCutsv15.xml`), plus: `common_heavy_suppression`:
`NeutronThreshold = ProtonThreshold = IonThreshold = 30.0` MeV, used only with `KillHeavy = True`, which is `False`
(left out of the text). The HF box: `SimG4CMS/Calo/src/HCalSD.cc` (`getFromHFLibrary`), `HFShowerLibrary.cc`; the
transport-only numbers it quotes (1511 photoelectrons in 459 cells) are the `Transport (09)` row of `ledger_10_SIM.txt`.

Table 10.3 (production cuts), read from the nine `*ProdCuts.xml` files that
`Geometry/CMSCommonData/data/dd4hep/cmsExtendedGeometry2024.xml` includes (`CMSCutsRegion`, `ProdCutsForGamma`,
`ProdCutsForElectrons`, `ProdCutsForPositrons`, with their `PartSelector`s):

| file | region: volumes -> gamma / e- / e+ |
| --- | --- |
| `Geometry/TrackerSimData/data/PhaseI/trackerProdCuts.xml` | `TrackerDeadRegion`: `//Tracker` -> 1 mm; `TrackerSensRegion`: 44 strip `*Active*` volumes -> 0.1 mm; `TrackerPixelDeadRegion`: `PixelBarrel`, `PixelForwardZplus/Zminus` -> 1 mm; `TrackerPixelSensRegion`: `PixelBarrelActiveFull0-3`, `PixelForwardSensor*` -> 0.01 mm |
| `Geometry/TrackerSimData/data/trackerProdCutsBEAM.xml` | `BeamPipe`: `BeamTube1` -> 0.01 mm; `BeamPipeOutside`: `BEAM.*` -> 1 mm; `BeamPipeVacuum`: `BeamVacuum.*` -> 1 mm |
| `Geometry/EcalSimData/data/EcalProdCuts.xml` | `EcalRegion`: `ECAL` -> 1 mm; `APDRegion`: `EAPD.*`, `EATJ.*` -> 0.005 mm |
| `Geometry/EcalSimData/data/ESProdCuts.xml` | `PreshowerRegion`: `SF` -> 1 mm; `PreshowerSensRegion`: `SFSX`, `SFSY` -> 0.1 mm |
| `Geometry/HcalSimData/data/HcalProdCuts/2021/v2/HcalProdCuts.xml` | `HcalRegion`: `CALO`, `MBAT`, `MBBT.*` (the HO, `hcalouteralgo.xml`), `VCAL` -> 1 mm |
| `Geometry/MuonSimData/data/muonProdCuts/2021/v3/muonProdCuts.xml` | `Muon`: `MUON` -> 100 / 10 / 10 mm; `MuonIron`: `MGNT`, `YB*`, `COI*`, ... (17 selectors) -> 1 mm; `MuonChamber`: `MB.*N`, `MB.*P`, `ME.*Space`, `RR.*`, `GE.*` -> 1 / 0.002 / 0.002 mm; `MuonSensitive_DT-CSC`: `MB.*PhiGas`, `MB.*ZGas`, `ME.*ActiveGasVol` -> 25 mm / 1 mm / 2.5 m; `MuonSensitive_RPC` -> 8.5 m / 8 mm / 2.5 m (comment "10 keV for gamma/e-, 1 MeV for e+"); the `SpecPar` `MuonSensitive_GEM` (`GHA.*`, 1 / 0.002 / 0.002 mm) has no `CMSCutsRegion` and is not in the table |
| `Geometry/ForwardSimData/data/ForwardShieldProdCuts.xml`, `bhmProdCuts/2021/v1/bhmProdCuts.xml`, `zdcProdCuts/2021/v3/zdcProdCuts.xml` | `QuadRegion` (`QuadInner/Coil/Outer`, `FixedShield10-12`) -> 10 / 1 / 1 mm; `BHMRegion` -> 10 / 1 / 1 mm; `ZDCRegion` (`ZDC`, `RPD`) -> 1 mm |

Outside every region: `DefaultCutValue = 1.0` (cm), `g4SimHits_cfi.py` line 192.

### 4.7 The 2024 and the 2025 settings

The chapter runs with `YEAR=2025` (`common/jitjet_common.sh`): `auto:phase1_2025_realistic` =
`150X_mcRun3_2025_realistic_v1`, era `Run3_2025`, for the five SIM jobs, the mixing job and the analyzers
(`simAnalyzer_cfg.py` defaults to them; `simHitAnalyzer_cfg.py` receives them from the script). The geometry, the
field map and the simulated beam spot are the same tags as in 2024 (chapter 09 companion, section 4.7; chapter 08
companion, section 4.9); the five modifiers of `Run3_2025` occur neither in `g4SimHits_cfi.py` nor in
`SimGeneral/MixingModule/python/`.

The rerun with the 2025 settings (12 November 2026, `REUSE=0`) against the 2024 run (7 November 2026); both were made
with the 250-event library and the window bx -1..1 of that time, so the playback and in-time rows quote the draw of then
(the rerun of 21 November 2026 on bx -12..3, section 4.5, used the 2025 settings only: nothing of the SIM step depends on
the year apart from the drawing alignment below, and the first 250 events of the new library are the old ones):

| compared | result |
|---|---|
| `ledger_10_SIM.txt` | identical |
| `simAnalyzer_{B,B_Decay,B_Decay_EM,signal}_10_SIM.log` | identical apart from time stamps and the era / global-tag lines |
| `simHits_mixed_10_SIM.log` (counts per station, per bunch crossing, per origin) | identical |
| `simHits_signal_10_SIM.log`, `simHits_pileup_10_SIM.log` | identical except the six lines of the tracker layer ranges (`PXB: r = 2.73564 - 16.4165, \|z\| = 3.34977 - 23.4511` in 2024, `r = 2.7255 - 16.4164, \|z\| = 3.34899 - 23.4511` in 2025, ...): the hit positions are drawn with the aligned tracker geometry, see below |
| `step2_SIM_{B,B_Decay,B_Decay_EM,signal}_10_SIM.root` (FWLite, per collection: number of `SimTrack`s, `SimVertex`s, `PSimHit`s, `PCaloHit`s and the sums of their energies, times, local positions, track ids) | 31 collections each, 0 differences (signal: 1926 `SimTrack`s, 37 182 hits) |
| `step2_SIM_pileup_10_SIM.root` | 250 events, 7750 collections, 120 574 `SimTrack`s, 1 850 410 hits, 0 differences |
| `step3_MIX_10_SIM.root`, `CrossingFramePlaybackInfoNew` of `mix` | identical: `sizes_` = 71 / 68 / 54 for bx -1 / 0 / +1, the same 193 library events (1-193) in the same order |
| in-time check of the script | "in-time collisions of the mixing: library events 72-139, as assumed in chapters 08 and 09" (no `WARNING_intime_10_SIM.txt`) |

The one difference is in the drawing: `SimAnalyzer` and `SimHitAnalyzer` convert the local tracker hit positions with
`trackerGeometryDB` of `GeometryRecoDB_cff` (`applyAlignment = True`), i.e. with `TrackerAlignmentRcd` of the global
tag, `TrackerAlignment_Run3MC_2023_rereco_v1` in 2024 and `TrackerAlignment_Run3MC_2025_EOY_v1` in 2025: module
positions differ by 10 um in the median, 0.33 mm in the TEC, at most 6.8 mm for one TIB module (chapter 09 companion,
section 4.7). No number of the chapter depends on it.

### 4.8 FWLite extras: `extras_10_SIM.txt`

Made by `test/10_SIM/simExtras_10_SIM.py` (section 3), from the files of chapters 09 and 10; nothing is rerun. A hit or
a cell is attributed to a primary through the `SimTrack` / `SimVertex` parents, as the analyzers do; the b and bbar
systems are the status-1 descendants of the B- [829] and the B*+ [968] of `genParticles`, matched to the primaries by the
barcodes of `genParticles` (`std::vector<int>`). The creator process of a vertex is `SimVertex::processType()` mapped as
in `plugins/SimAnalyzer.cc` (`procOf`). The regions of a vertex: beam pipe r < 2.2 cm; pixel volume r < 20 cm, |z| < 55 cm;
strip volume r < 110 cm, |z| < 280 cm (the tracker volume of the conversion and interaction fractions); calorimeters
r > 129 cm or |z| > 317.5 cm (the EB and EE faces of `simHits_signal_10_SIM.log`); preshower 303.5 < |z| < 317.5 cm;
the rest "between the tracker and the calorimeters". The HCAL hits are in the test numbering of the simulation
(`DataFormats/HcalDetId/src/HcalTestNumbering.cc`: subdetector = bits 28-31; 1 HB, 2 HE, 3 HO, 4 HF); the check line
reproduces the HB, HE and HF sums of the SimAnalyzer.

```
== simExtras 10_SIM: FWLite on the SIM files of chapters 09 and 10 (event 1)
  transport (09)  09_G4/out/step2_G4_signal_09_G4.root: 571 SimTracks, 29 SimVertices, 571 primaries
  B               10_SIM/out/step2_SIM_B_10_SIM.root: 571 SimTracks, 29 SimVertices, 571 primaries
  B_Decay         10_SIM/out/step2_SIM_B_Decay_10_SIM.root: 637 SimTracks, 70 SimVertices, 571 primaries
  B_Decay_EM      10_SIM/out/step2_SIM_B_Decay_EM_10_SIM.root: 1304 SimTracks, 474 SimVertices, 571 primaries
  signal          10_SIM/out/step2_SIM_signal_10_SIM.root: 1926 SimTracks, 768 SimVertices, 571 primaries

-- (1) the field (stage B against the transport-only run of chapter 09)
  primaries reaching the boundary: transport 557, B 552; reaching without field but not with it: 5
    pi+-   charge -1  pT = 0.031 GeV  eta = +2.55
    pi+-   charge +1  pT = 0.034 GeV  eta = +3.75
    pi+-   charge +1  pT = 0.039 GeV  eta = +4.20
    pi+-   charge +1  pT = 0.216 GeV  eta = -5.32
    pi+-   charge -1  pT = 0.453 GeV  eta = -1.01
  charged primaries: 277; with pT < 0.735 GeV and |eta| < 1.479 (would curl inside r = 1.29 m): 54, of which reaching: 53
  charged primaries with |eta| < 2.5: 159; pT < 0.735 GeV: 114; median pT 0.48 GeV
  tracker hits of the charged, pT < 0.735  primaries: transport  2480, B  2756
  tracker hits of the charged, pT >= 0.735 primaries: transport   971, B   980
  tracker hits of the neutral              primaries: transport  3283, B  3283
  pi+ followed: pT = 13.31 GeV, eta = -1.545, phi at the vertex -2.0811, boundary point r = 123.3 cm z = -272.8 cm phi -2.1328: dphi = -0.0518 rad (-6.38 cm at that r)

-- (2) the decays in flight (stage B_Decay)
  41 decay vertices with a stored daughter, 66 daughters
    pi+-     -> mu                        12
    K0S      -> pi+- pi+-                  7
    111      -> gamma gamma                6
    K0S      -> 111 111                    3
    Lambdabar -> pbar pi+-                  3
    Lambda   -> p pi+-                     3
    K+-      -> mu                         3
    Sigma-bar -> nbar pi+-                  2
    Lambdabar -> 111 nbar                   1
    111      -> gamma                      1
    decay vertex strip volume (20 < r < 110 cm)    18
    decay vertex pixel volume (r < 20 cm)          12
    decay vertex beam pipe (r < 2.2 cm)            10
    decay vertex in the calorimeters (r > 129 cm or |z| > 317 cm)   1
  b system: K0S of pT 7.96 GeV (E 20.63) decayed at r = 80.2 cm, z = -188.5 cm into pi+- pi+-

-- (3) the electromagnetic physics (stage B_Decay_EM)
  conversion: 230 vertices, 430 stored secondaries; strip volume (20 < r < 110 cm) 121, in the calorimeters (r > 129 cm or |z| > 317 cm) 47, between the tracker and the calorimeters 41, pixel volume (r < 20 cm) 16, in the preshower (303.5 < |z| < 317.5 cm) 4, beam pipe (r < 2.2 cm) 1
    of which conversions of a primary photon: 187
  bremsstrahlung: 153 vertices, 213 stored secondaries; strip volume (20 < r < 110 cm) 109, between the tracker and the calorimeters 38, pixel volume (r < 20 cm) 3, in the calorimeters (r > 129 cm or |z| > 317 cm) 2, in the preshower (303.5 < |z| < 317.5 cm) 1
  primary photons with E > 1 GeV, 0.0 <= |eta| < 1.0: 3, converted inside the tracker volume: 2 (67 %)
  primary photons with E > 1 GeV, 1.0 <= |eta| < 2.5: 26, converted inside the tracker volume: 16 (62 %)
  primary photons with E > 1 GeV, 0.0 <= |eta| < 2.5: 29, converted inside the tracker volume: 18 (62 %)
  followed gamma: pT = 17.73 GeV, eta = -1.566, E = 44.27 GeV; converts at r = 112.3 cm, z = -253.9 cm (between the tracker and the calorimeters)
  followed e: pT = 4.95 GeV, eta = -1.431, E = 10.95 GeV

-- (3) the electromagnetic physics (stage signal)
  conversion: 268 vertices, 488 stored secondaries; strip volume (20 < r < 110 cm) 134, in the calorimeters (r > 129 cm or |z| > 317 cm) 51, between the tracker and the calorimeters 47, pixel volume (r < 20 cm) 27, in the preshower (303.5 < |z| < 317.5 cm) 7, beam pipe (r < 2.2 cm) 2
    of which conversions of a primary photon: 186
  bremsstrahlung: 189 vertices, 275 stored secondaries; strip volume (20 < r < 110 cm) 136, between the tracker and the calorimeters 44, pixel volume (r < 20 cm) 7, in the calorimeters (r > 129 cm or |z| > 317 cm) 2
  primary photons with E > 1 GeV, 0.0 <= |eta| < 1.0: 3, converted inside the tracker volume: 0 (0 %)
  primary photons with E > 1 GeV, 1.0 <= |eta| < 2.5: 26, converted inside the tracker volume: 15 (58 %)
  primary photons with E > 1 GeV, 0.0 <= |eta| < 2.5: 29, converted inside the tracker volume: 15 (52 %)
  followed gamma: pT = 17.73 GeV, eta = -1.566, E = 44.27 GeV; converts at r = 3.0 cm, z = -3.7 cm (pixel volume (r < 20 cm))
  followed e: pT = 4.95 GeV, eta = -1.431, E = 10.95 GeV

-- (4) the hadronic physics (stage signal)
  hadronic vertices with a stored secondary: 142, 386 secondaries; strip volume (20 < r < 110 cm) 85, between the tracker and the calorimeters 26, pixel volume (r < 20 cm) 13, in the calorimeters (r > 129 cm or |z| > 317 cm) 9, in the preshower (303.5 < |z| < 317.5 cm) 5, beam pipe (r < 2.2 cm) 4
  their secondaries by kind: pi+- 178, 111 62, n 60, p 58, K+- 6, K0S 5, K0L 4, 221 4
  parents: pi+- 102, p 10, n 10, K+- 8, K0L 4, nbar 4
  primary charged pi/K/p with pT > 0.5 GeV, 0.0 <= |eta| < 1.0: 14, with a nuclear interaction inside the tracker volume: 3 (21 %)
  primary charged pi/K/p with pT > 0.5 GeV, 1.0 <= |eta| < 2.5: 59, with a nuclear interaction inside the tracker volume: 16 (27 %)
  primary charged pi/K/p with pT > 0.5 GeV, 0.0 <= |eta| < 2.5: 73, with a nuclear interaction inside the tracker volume: 19 (26 %)
  followed K0S of the bbar jet: pT = 15.65 GeV, E = 338.5 GeV, eta = -3.766; vertices it ends in or creates: hadron inelastic at r = 2.2 cm, z = -43.7 cm
  followed pi+- of the bbar jet: pT = 30.61 GeV, E = 642.9 GeV, eta = -3.737; vertices it ends in or creates: none

-- check: HcalHits of the signal by subdetector (test numbering): HB 0.2173, HE 1.6624, HF 1849.0000, HO 0.0000

-- (5) the b system (B- decay products) and the bbar system (B*+ decay products), stage signal
  b system: 8 primaries, E = 140.3 GeV: gamma 44.3, pi+- 32.6, pi+- 26.6, K0S 20.6, e 10.9, gamma 4.1, gamma 0.8, gamma 0.4
    deposits: EB 6.229 (364 cells), EE 57.323 (895 cells), ES 0.024 (96 cells), HB 0.010 (17 cells), HE 0.376 (105 cells)
  bbar system: 16 primaries, E = 2421.4 GeV: pi+- 642.9, K0S 338.5, K0S 275.5, K0L 241.0, gamma 179.0, pi+- 164.6, gamma 132.9, gamma 121.9, pi+- 95.9, gamma 58.4, gamma 51.9, gamma 45.8, gamma 33.2, gamma 18.2, gamma 16.3, gamma 5.4
    deposits: EB 0.540 (119 cells), EE 9.353 (418 cells), ES 0.012 (52 cells), HB 0.000 (2 cells), HE 0.025 (14 cells), HF 987.000 (68 cells)

-- (6) the followed photon: its own deposits in the EB/EE crystals (stage B_Decay_EM)
  photon pT 17.73 E 44.27: 156 EB/EE cells, 42.79 GeV (97 % of E); hottest 28.57 GeV (67 %), 5 hottest 38.11 GeV (89 %), 90 % in 6 cells; cells above 1 GeV: 4; ES 0.0002 GeV; tracker hits of its descendants (stored tracks) 0

-- (6) the followed photon: its own deposits in the EB/EE crystals (stage signal)
  photon pT 17.73 E 44.27: 466 EB/EE cells, 39.45 GeV (89 % of E); hottest 7.80 GeV (20 %), 5 hottest 18.56 GeV (47 %), 90 % in 38 cells; cells above 1 GeV: 10; ES 0.0039 GeV; tracker hits of its descendants (stored tracks) 171

-- (7) the mixed collisions (simHits_mixed_10_SIM.log, lines "pileup event")
  984 collisions with |z| < 1 m: mean z = 0.38 cm, rms 3.58 cm, min -12.42, max 12.86; within |z - 3.11| < 0.1 cm of the signal vertex: 13
  outside: bx -12 pileup event 12 at z = -20300 cm (1 primary)
  outside: bx -11 pileup event 53 at z = -20300 cm (2 primary)
  outside: bx -10 pileup event 40 at z = 20300 cm (1 primary)
  outside: bx -9 pileup event 3 at z = 20300 cm (1 primary)
  outside: bx -4 pileup event 2 at z = 20300 cm (2 primary)
  outside: bx -2 pileup event 8 at z = 20300 cm (1 primary)
  outside: bx -2 pileup event 60 at z = -20300 cm (2 primary)
  outside: bx +0 pileup event 28 at z = 20300 cm (2 primary)
  outside: bx +1 pileup event 23 at z = -20300 cm (2 primary)
  outside: bx +2 pileup event 6 at z = -20300 cm (1 primary)
  outside: bx +3 pileup event 35 at z = -20300 cm (2 primary)
  bx -12: 71 collisions, primaries per collision: min 1, median 69, max 537, sum 10012
  bx -11: 68 collisions, primaries per collision: min 2, median 114.5, max 470, sum 10414
  bx -10: 54 collisions, primaries per collision: min 1, median 148.5, max 574, sum 10634
  bx -9: 59 collisions, primaries per collision: min 1, median 100, max 623, sum 8452
  bx -8: 52 collisions, primaries per collision: min 2, median 78, max 736, sum 7926
  bx -7: 61 collisions, primaries per collision: min 2, median 65, max 481, sum 7045
  bx -6: 76 collisions, primaries per collision: min 3, median 100, max 752, sum 13204
  bx -5: 59 collisions, primaries per collision: min 4, median 161, max 734, sum 10446
  bx -4: 65 collisions, primaries per collision: min 2, median 90, max 610, sum 9017
  bx -3: 59 collisions, primaries per collision: min 3, median 99, max 607, sum 9161
  bx -2: 68 collisions, primaries per collision: min 1, median 104, max 542, sum 9813
  bx -1: 54 collisions, primaries per collision: min 2, median 80, max 610, sum 7969
  bx +0: 64 collisions, primaries per collision: min 2, median 84, max 754, sum 8804
  bx +1: 63 collisions, primaries per collision: min 2, median 104, max 535, sum 10972
  bx +2: 62 collisions, primaries per collision: min 1, median 77.5, max 589, sum 8118
  bx +3: 60 collisions, primaries per collision: min 2, median 76.5, max 675, sum 7907
  early (bx < 0) : 746 collisions, primaries per collision: min 1, median 93.5, max 752, sum 114093
  in time (bx 0) : 64 collisions, primaries per collision: min 2, median 84, max 754, sum 8804
  late (bx > 0)  : 185 collisions, primaries per collision: min 1, median 84, max 675, sum 26997
  all            : 995 collisions, primaries per collision: min 1, median 90, max 754, sum 149894

-- (8) the library collisions with a vertex at |z| = 203 m (step2_SIM_pileup_10_SIM.root, all library events scanned)
  library event 12: SimTrack pdg 2212, E = 6800 GeV, primary True, vertex z = -20300 cm
  library event 124: SimTrack pdg 2212, E = 6800 GeV, primary True, vertex z = -20300 cm
  library event 124: SimTrack pdg 2212, E = 5539 GeV, primary True, vertex z = 20300 cm
  library event 179: SimTrack pdg 2212, E = 6800 GeV, primary True, vertex z = 20300 cm
  library event 196: SimTrack pdg 2212, E = 6800 GeV, primary True, vertex z = 20300 cm
  library event 502: SimTrack pdg 2212, E = 6800 GeV, primary True, vertex z = 20300 cm
  library event 502: SimTrack pdg 2212, E = 3268 GeV, primary True, vertex z = -20300 cm
  library event 632: SimTrack pdg 2212, E = 6800 GeV, primary True, vertex z = 20300 cm
  library event 684: SimTrack pdg 2212, E = 6800 GeV, primary True, vertex z = -20300 cm
  library event 684: SimTrack pdg 2212, E = 3605 GeV, primary True, vertex z = 20300 cm
  library event 774: SimTrack pdg 2212, E = 6800 GeV, primary True, vertex z = 20300 cm
  library event 774: SimTrack pdg 2212, E = 4783 GeV, primary True, vertex z = -20300 cm
  library event 833: SimTrack pdg 2212, E = 6800 GeV, primary True, vertex z = -20300 cm
  library event 833: SimTrack pdg 2212, E = 5577 GeV, primary True, vertex z = 20300 cm
  library event 879: SimTrack pdg 2212, E = 6800 GeV, primary True, vertex z = -20300 cm
  library event 970: SimTrack pdg 2212, E = 6800 GeV, primary True, vertex z = -20300 cm
  library event 970: SimTrack pdg 2212, E = 5811 GeV, primary True, vertex z = 20300 cm
  library events with a primary at |z| > 100 m: 318 of 1000 (360 such primaries); with nothing else for Geant4: 11 (12 124 179 196 502 632 684 774 833 879 970)
```

The arithmetic of the chapter on these numbers:

| quoted | from |
|---|---|
| looper threshold 0.735 GeV, R = 0.88 m x pT | 0.3 x 3.8 T x 1.29 m / 2; 1 / (0.3 x 3.8) = 0.877 m/GeV |
| 114 of 159 charged primaries with \|eta\| < 2.5 below 0.735 GeV, "almost three quarters" | 114 / 159 = 72 % |
| 276 of the 285 new crossings | 2756 - 2480 = 276; 7019 - 6734 = 285; the fast charged 980 - 971 = 9 |
| the followed pi+: R = 11.7 m, expected 0.0528 rad, field 2 % weaker, sagitta 1.3 cm | 13.31 / (0.3 x 3.8) = 11.68 m; asin(1.233 / (2 x 11.68)) = 0.0528; 0.0518 / 0.0528 = 0.981; 1.1^2 / (8 x 11.68) = 0.0130 m |
| K0S of the b system: beta gamma = 41, mean flight 1.1 m, 2.08 m from the vertex, 15 % | p = 20.62 GeV, m(K0S) = 0.4976 GeV (PDG), c tau = 2.684 cm (PDG): 41.4 x 2.684 = 111 cm; sqrt(80.2^2 + (188.5 + 3.11)^2) = 207.7 cm = 1.87 mean flights, exp(-1.87) = 0.15 |
| c tau of pi+-, K+-, K0S, Lambda: 7.8 m, 3.7 m, 2.7 cm, 7.9 cm | PDG |
| seven pi0 -> gamma gamma | 6 + 1 vertices (one with a single stored photon) |
| 230 conversions: 1 + 16 + 121 + 41 + 4 + 47 | the location line of section (3), stage B_Decay_EM |
| t = 0.9 to 1.2 X0 | 1 - exp(-7t/9) = 15/29 (full) and 18/29 (B_Decay_EM): t = -(9/7) ln(14/29) = 0.94, -(9/7) ln(11/29) = 1.25 |
| 0.15 MeV per crossing (0.23 in the full simulation) | 1.16607 GeV / 7861 hits; 2.33358 GeV / 10 272 hits |
| 0.12 MeV for a MIP in 300 um of silicon | (dE/dx)min = 1.664 MeV cm2/g x 2.329 g/cm3 (PDG) x 0.03 cm = 0.116 MeV |
| X0 = 0.89 cm, R_M ~ 2 cm, E_c ~ 9 MeV of PbWO4; lambda_I ~ 20 cm (PbWO4), ~ 16 cm (brass) | PDG tables of material properties |
| shower maximum 8.5 X0, 7.6 cm for 44 GeV | ln(44 000 / 9) = 8.49; 8.49 x 0.89 cm = 7.56 cm |
| crystals 22-23 cm (about 25 X0), 2.2 cm (EB) and 2.9 cm (EE) wide | the CMS ECAL crystal dimensions (barrel 23 cm, front face 2.2 cm; endcap 22 cm, 2.86 cm) |
| photon: 97 %, two thirds, 89 %, 90 % in 6 (B_Decay_EM); 89 %, a fifth, 47 %, 90 % in 38, 11 % lost (full) | section (6) |
| two hadrons in three start their shower in an ECAL crystal | 1 - exp(-23 / 20.3) = 0.68 |
| 26 % of the charged hadrons interact in the tracker, 0.3 lambda_I | 19 / 73; -ln(1 - 19/73) = 0.30 |
| 142 nuclear-interaction vertices: 4 + 13 + 85 + 26 + 14 | the location line of section (4); 14 = 9 (calorimeters) + 5 (preshower) |
| b system: ECAL 63.58 GeV, HCAL 0.386 GeV in 122 cells, x sampling = 73 GeV | 6.229 + 57.323 + 0.024; 0.010 + 0.376; 17 + 105; 0.010 x 125 + 0.376 x 190 = 72.7 (HB and HE sampling factors of `test/11_DIGI/out/thresholds_11_DIGI.txt`: `hb.samplingFactors` 124.5-143.5, `he.samplingFactors` 186.1-210.6) |
| bbar system: 9.9 GeV in the ECAL (9.4 in the EE) | 0.540 + 9.353 + 0.012 = 9.905 (the SimAnalyzer: 9.90505) |
| the followed photon and pi+ in the mixed event: 37 crystals with 41.7 GeV, 2 with 2.8 GeV; the HF pi+ 2 of its 19 cells | `simHits_mixed_10_SIM.log`, line `followed primaries` of station `ECAL`: `#pi^{+} 13.3 GeV (#eta -1.54), 2.78 GeV in 2 cells;  K_{S}^{0} 15.7 GeV (#eta -3.77), 1.44 GeV in 28 cells;  #gamma 17.7 GeV (#eta -1.57), 41.69 GeV in 37 cells;  e^{-} 5.0 GeV (#eta -1.43), 6.07 GeV in 20 cells`; of station `HCAL`: `#pi^{+} 30.6 GeV (#eta -3.74), 311 (HF units) in 2 HF cells;  K_{L}^{0} 11.8 GeV (#eta -3.71), 54 (HF units) in 1 HF cells` |
| the followed primaries of the signal: photon 270 cells with 42.0 GeV, pi+ 35 cells with 3.4 GeV, 171 tracker hits of the photon, 809 HF units in 19 cells of the HF pi+, K0S 203 hits | `simHits_signal_10_SIM.log`, lines `followed primaries` of stations `TRK`, `ECAL`, `HCAL`: `#pi^{+} 13.3 GeV (#eta -1.54), 29 hits;  K_{S}^{0} 15.7 GeV (#eta -3.77), 203 hits;  #gamma 17.7 GeV (#eta -1.57), 171 hits;  e^{-} 5.0 GeV (#eta -1.43), 138 hits`; `#pi^{+} 13.3 GeV (#eta -1.54), 3.43 GeV in 35 cells; ...; #gamma 17.7 GeV (#eta -1.57), 42.03 GeV in 270 cells`; `#pi^{+} 30.6 GeV (#eta -3.74), 809 (HF units) in 19 HF cells`; the "GeV" of these labels is the pT of the primary (`plugins/SimHitAnalyzer.cc`, `highlightLine`) |
| every barrel and endcap crystal hit; ES 82 999 of 137 216; HB 6856 of 9072, HE 5464 of 6768, HF 3405 of 3456 | `simHits_mixed_10_SIM.log` (cells per subdetector: `EB=61200 EE=14648 ES=82999`, `HB=6856 HE=5464 HO=66 HF=3405`) against the detector elements of `simHits_signal_10_SIM.log` (`EB=61200 EE=14648 ES=137216 HB=9072 HE=6768 HO=2160 HF=3456`) |
| EE 44 400 GeV, 151 times the signal | `simHits_mixed_10_SIM.log`, station `ECAL`: `energy by subdetector: EB=12136 GeV EE=44419 GeV ES=43.4 GeV`; 44 419 / 293.66 = 151 |
| PU vertices: r.m.s. 3.58 cm, -12.4 to +12.9 cm, 13 of 984 within 1 mm of z = 3.11 cm, one of them in time | section (7); the 11 collisions at z = +-203 m left out (984 + 11 = 995) |
| primaries per collision: median 90 of the 995, per crossing from 65 (bx -7) to 161 (bx -5), 84 in time; 137.6 per in-time collision against 150.6 | section (7) of `extras_10_SIM.txt` (for an even count the mean of the two middle values; the script printed the upper middle value until 13 November 2026); the same from FWLite on the library, section 4.5: 8804 / 64 = 137.6, 149 894 / 995 = 150.6 |
| the PPS protons, `LHCTransport`, 203 m | section (8); `g4SimHits_cfi.py` line 694 `run3_common.toModify( g4SimHits, LHCTransport = True )`; `SimTransport/PPSProtonTransport/python/CommonParameters_cfi.py` lines 10-11 `PPSRegionStart_45/56 = 203` |

The release parameters quoted in the subsections (all under `$CMSSW_RELEASE_BASE/src`):

| parameter | value | file (line) |
|---|---|---|
| `UseMagneticField` | `True` (chapter 09 and stage B of the transport list: `False` / `True` by `--customise_commands`) | `SimG4Core/Application/python/g4SimHits_cfi.py` (87) |
| `MagneticField.ConfGlobalMFM...StepperParam`: `DeltaOneStep`, `DeltaOneStepTracker`, `DeltaChord`, `DeltaChordTracker`, `EnergyThSimple`, `DeltaChordSimple` | `0.001` mm, `1e-4` mm, `0.002` mm, `0.001` mm, `0.015` GeV, `0.1` mm | same (157-169) |
| `Physics.G4TrackingCut` | `0.025` MeV | same (195) |
| `Physics.EminFTFP`, `EmaxBERT`, `EmaxBERTpi` | `3.`, `6.`, `12.` GeV | same (217-221) |
| `StackingAction.TrackNeutrino`, `SaveAllPrimaryDecayProductsAndConversions` | `False`, `True` | same (308, 316) |
| `TrackerSD.EnergyThresholdForPersistencyInGeV` | `0.2` (compared with the kinetic energy, `SimG4CMS/Tracker/src/TkAccumulatingSensitiveDetector.cc` line 153) | same (367) |
| `CaloSD.EminTrack`, `EminHits` | `1.0` GeV ("Minimum energy of track for saving it", `SimG4CMS/Calo/src/CaloSD.cc` lines 58, 124, 678), `(0.015, 0.010, 0.0, 0.0, 0.0)` MeV (`CaloSD.cc` line 90, applied per hit in `CaloSD::filterHit`, line 928) | same (382, 385) |
| production cuts | `TrackerDeadRegion` 1 mm, `TrackerSensRegion` 0.1 mm, `TrackerPixelDeadRegion` 1 mm, `TrackerPixelSensRegion` 0.01 mm; `EcalRegion` 1 mm, `APDRegion` 0.005 mm; `HcalRegion` 1 mm | `Geometry/TrackerSimData/data/PhaseI/trackerProdCuts.xml` (13-16, 63-66, 72-75, 86-89), `Geometry/EcalSimData/data/EcalProdCuts.xml` (6-17), `Geometry/HcalSimData/data/HcalProdCuts/2021/v2/HcalProdCuts.xml`, included by `Geometry/CMSCommonData/data/dd4hep/cmsExtendedGeometry2024.xml` (213, 373-375) |
