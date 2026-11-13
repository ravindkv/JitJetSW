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

# (10b) the library: all 250 collisions, full physics, its own Geant4 seed
cmsDriver.py step2 --filein file:$LIB $COMMON -n -1 --fileout file:step2_SIM_pileup_10_SIM.root \
  --customise_commands "process.g4SimHits.UseMagneticField = cms.bool(True)\nprocess.g4SimHits.Physics.EMPhysics = cms.untracked.bool(True)\nprocess.g4SimHits.Physics.HadPhysics = cms.untracked.bool(True)\nprocess.RandomNumberGeneratorService.g4SimHits.initialSeed = 20243" \
  --python_filename $J/test/10_SIM/step2_SIM_pileup_10_SIM_cfg.py --no_exec

# (10a+10b) the mixing at SimHit level: the MixingModule of the DIGI step without digitisers, SimHitAnalyzer on the CrossingFrames
cmsDriver.py step3 --filein file:$J/test/10_SIM/out/step2_SIM_signal_10_SIM.root \
  --pileup "AVE_60_BX_25ns,{'N': 60, 'B': (-1, 1)}" --pileup_input file:$J/test/10_SIM/out/step2_SIM_pileup_10_SIM.root \
  --conditions auto:phase1_2025_realistic --era Run3_2025 --geometry DB:Extended -n -1 -s DIGI:pdigi_valid --processName MIX \
  --datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG --fileout file:step3_MIX_10_SIM.root \
  --outputCommands "drop *_mix_*_*,keep CrossingFramePlaybackInfoNew_mix_*_*" \
  --customise_commands "process.mix.input.sequential = cms.untracked.bool(True)\nprocess.source.eventsToProcess = cms.untracked.VEventRange('1:1:1')\nfrom JitJetSW.Journey.mixOnly_cff import mixOnly; process = mixOnly(process, outputDir='$J/test/10_SIM/out/mixed', sampleLabel='chapter 10_SIM (10a+10b): signal + pileup at SimHit level|PU = 60 per bx (Poisson), bx -1..1|library in sequence, no digitisers|2025 detector, 13.6 TeV', prefix='simHits_mixed_10_SIM', highlight=1, highlightPtMin=2, savePNG=True, printHits=False)" \
  --python_filename $J/test/10_SIM/step3_MIX_10_SIM_cfg.py --no_exec
```

The arguments of the SIM steps:

| argument | meaning |
|---|---|
| `step2` | the name of the configuration (free; `step1` was the GEN step of chapters 01 to 08) |
| `--filein file:...` | the input (Rule E): the transport-only SIM file of chapter 09 (signal) or its 250-event library (pileup). The generator record is read from it; the SIM products of process `SIM` are read too but dropped from the output |
| `--conditions auto:phase1_2025_realistic` | the global tag of 2025 (`150X_mcRun3_2025_realistic_v1` through `Configuration/AlCa/python/autoCond.py`, line 107; `YEAR=2025` of `jitjet_common.sh`): geometry, field map, beam spot; the geometry `141DD4hepV2_Extended2024` and the field map are the same tags as in the 2024 global tag (chapter 09 companion, section 4) |
| `--era Run3_2025` | the era modifiers of the 2025 detector (`Era_Run3_2025_cff.py`: `Run3_2024` plus five modifiers of the reconstruction and the Level-1 trigger, none used by `g4SimHits` or the `MixingModule`): through `Run3`, `run3_common` selects the Run-3 sensitive detectors and hit collections in `g4SimHits_cfi.py`, `run3_HFSL` the HF shower library `HFShowerLibrary_run3_v7.root` |
| `--geometry DB:Extended` | the simulation geometry (DDD XML) from the database, label `Extended` |
| `--beamspot DBrealistic` | only sets the `VtxSmeared` label in a SIM-only step; the smeared vertices are already in the input |
| `-n 1` / `-n -1` | one event (the signal) / all events (the 250 library collisions) |
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
| `--pileup "AVE_60_BX_25ns,{'N': 60, 'B': (-1, 1)}"` | the mixing scenario: `mix_POISSON_average_cfi` (`Configuration/StandardSequences/python/Mixing.py`, `AVE_60_BX_25ns` = `BX 25, B (-12,3), N 60`), overridden by the dictionary: Poisson with mean `N = 60` collisions per bunch crossing, bunch crossings `B = (-1, 1)`, 25 ns spacing |
| `--pileup_input file:.../step2_SIM_pileup_10_SIM.root` | the library (10b) read by the secondary source `EmbeddedRootSource` of the module `mix` |
| `-s DIGI:pdigi_valid` | the DIGI step; its `pdigi_valid` sequence is replaced by `mixOnly` with the `mix` module alone (section 2) |
| `--processName MIX` | the process name of the mixing job (the input holds `GEN`, `GENJET`, `SIMFULL`) |
| `--datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG` | the full content of the input is copied to the output |
| `--outputCommands "drop *_mix_*_*,keep CrossingFramePlaybackInfoNew_mix_*_*"` | nothing of the module `mix` is written (the `CrossingFrame`s are transient anyway) except the `CrossingFramePlaybackInfoNew`: the record of which library events went into which bunch crossing, the input of the playback of chapter 11 |
| `--fileout file:step3_MIX_10_SIM.root` | the output: the signal event with the playback record |
| `--customise_commands` | `process.mix.input.sequential = True`: the library is read from its first event on, in order (the default picks a random start); `process.source.eventsToProcess = '1:1:1'`: the event of the book; `mixOnly(process, outputDir=..., sampleLabel=..., prefix=..., highlight=1, highlightPtMin=2, savePNG=True, printHits=False)`: `$J/python/mixOnly_cff.py`, section 2. With `ANALYZE=0` the script writes the same configuration with `mixOnly(process)` (no drawing): the mixing and its playback record only |

Variables of the script: `PU=60 BUNCHES=-1,1 SKIP=71 NPU=68` (the pileup convention of the book, `common/jitjet_common.sh`),
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
cmsRun ../step2_SIM_pileup_10_SIM_cfg.py     > step2_SIM_pileup_10_SIM.log 2>&1     # 250 events, 7-12 minutes
cmsRun ../step3_MIX_10_SIM_cfg.py            > step3_MIX_10_SIM.log 2>&1            # the mixing, writes mixed/<station>/ and step3_MIX_10_SIM.root
```

The script runs the library in the background while the four signal stages run (`run_sim pileup ... &`, `run_pileup.log`),
with `REUSE=1` an existing `step2_SIM_<stage>_10_SIM.root` is kept. Timing from the logs (wall clock on a shared lxplus-like
node, so it depends on its load): the signal event through the full physics takes 8 s in the 2024 run of 7 November
2026 (`Begin processing ... 09:51:53`, `Closed file ... 09:52:01`, after 36 s of Geant4 initialisation) and 12 s in
the 2025 rerun of 12 November 2026 (`10:15:10` to `10:15:22`, after 50 s of initialisation, load average 5-7); the 250
library collisions 442 s (`09:49:57` to `09:57:19`), 1.8 s per collision, and 695 s (`10:12:15` to `10:23:51`), 2.8 s
per collision. All logs carry the
banner `Geant4 version Name: geant4-11-02-patch-02 [MT] (21-June-2024)`.

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
0.91 MB, `signal` 1.19 MB, `pileup` 59.6 MB (250 events).

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
  prefix=simHits_pileup_10_SIM eventsToProcess=1:1:72 $ARGS \
  sampleLabel="chapter 10_SIM (10b): one pileup collision|library event 72, full Geant4 physics|SoftQCD inelastic, 13.6 TeV|2025 detector, no signal" > simHits_pileup_10_SIM.log 2>&1
# the mixed sample is drawn inside the mixing job (section 2); its analyzer lines are extracted from the job log:
grep -E "^SimHitAnalyzer|^  pileup mixed|^    pileup event|^  station|^    hits by|^    primaries with hits" step3_MIX_10_SIM.log > simHits_mixed_10_SIM.log
# the ledger
python3 ../../common/simLedger.py $J/test/10_SIM/out 10_SIM $J/test/09_G4/out/simAnalyzer_signal_09_G4.log > ledger_10_SIM.txt
# the FWLite extras (no cmsRun; about 15 s): decays, conversions, nuclear interactions, the b-system budget, the
# followed photon in two runs, the vertices of the mixing; the last step of run_10_SIM.sh
cd $J/test && python3 10_SIM/simExtras_10_SIM.py > 10_SIM/out/extras_10_SIM.txt 2> 10_SIM/out/extras_10_SIM.log
```

`simAnalyzer_cfg.py` (chapter 09, section 3, for the options): `simStage=N` highlights the stage in the right panel of every
page (1 transport, 2 + B, 3 + decays, 4 + EM, 5 + hadronic); `genStages=5` draws the five generator pages first (signal
stage only). `simHitAnalyzer_cfg.py`: `highlight=1` follows the leading primary of every kind (above `highlightPtMin=2` GeV)
through the layers and labels it; `savePNG=1` writes every page as PNG next to the PDF; `printHits=1` would print every
primary with its hit counts per station; `stations=TRK,ALL` would restrict the stations. The global tag, era and geometry
give the reco geometry (`GeometryRecoDB`) for the hit positions and the detector elements drawn in grey.

What each run produces in `out/`:

| file | content |
|---|---|
| `simAnalyzer_<stage>_10_SIM.pdf`, `_page-N.png` | for `B`, `B_Decay`, `B_Decay_EM` five pages: 1 the primaries handed to Geant4 (by kind, the 63 stable particles without `SimTrack` as crosses), 2 the primaries at the tracker/calorimeter boundary (marker at the crossing point, line from the production direction, open marker if not reaching it), 3 the secondaries by creator process (decay, ionisation, bremsstrahlung, conversion, Compton, annihilation, hadron elastic / inelastic), 4 the tracker and muon `PSimHit`s by subdetector, 5 the `PCaloHit`s summed per cell with the marker size growing with the energy, the b and bbar systems in red and blue on every page. For `signal` ten pages: the five generator stages of chapters 01 to 06 first (`_page-01..05`), then the five SIM pages (`_page-06..10`) |
| `simAnalyzer_<stage>_10_SIM.log` | the per-page counts quoted in the chapter (section 4) |
| `simAnalyzer_<stage>_10_SIM.root` | `TFileService` histograms per page |
| `signal/<station>/simHits_signal_10_SIM_<station>_run1_event1.pdf`, `_<station>_{rz,xy,etaphi}_run1_event1.png` | (10a) one PDF of three pages per station (`TRK`, `ECAL`, `HCAL`, `MUON`, `ALL`): the r-z view, the transverse view of the barrel, the eta-phi view from the primary vertex; the detector elements of the station in grey, the hits coloured by the kind of the primary they descend from, the leading primary per kind connected through the layers; the header lists hits per subdetector and layer and the followed primaries |
| `pileup/<station>/simHits_pileup_10_SIM_<station>_..._run1_event72.pdf/.png` | (10b) the same for library event 72 |
| `mixed/<station>/simHits_mixed_10_SIM_<station>_..._run1_event1.pdf/.png` | (10a+10b) the same for the mixed event: the signal hits by kind on top, the pileup hits underneath coloured by bunch crossing (olive early, blue in time, red late); a calorimeter cell takes the colour of the origin that deposited most of its energy |
| `simHits_signal_10_SIM.log`, `simHits_pileup_10_SIM.log`, `simHits_mixed_10_SIM.log` | per station: hits and cells, hits by subdetector and layer, hits by kind of the primary (cells and their energy for the calorimeters), primaries with hits; the mixed log also lists every mixed collision with its bunch crossing, primaries, `SimTrack`s, `SimVertex`s, vertex z and time, and the "events per bx" line the script checks against `SKIP` / `NPU` |
| `step3_MIX_10_SIM.log` | the full mixing job log (the analyzer lines above, the detector element extents); the job was run with the drawing, and the playback file came from the same configuration run again without it, so this log does not mention the output module |
| `step3_MIX_10_SIM.root` | the signal event with the `CrossingFramePlaybackInfoNew` (section 2), the input of (11a+11b) |
| `ledger_10_SIM.txt` | the ledger of `common/simLedger.py`: table 1, the stages from the SimAnalyzer logs (with the transport-only run of chapter 09 as first line); table 2, the three samples station by station from the SimHitAnalyzer logs |
| `extras_10_SIM.txt` | the output of `test/10_SIM/simExtras_10_SIM.py` (FWLite on `test/09_G4/out/step2_G4_signal_09_G4.root`, the four `step2_SIM_<stage>_10_SIM.root`, `step2_SIM_pileup_10_SIM.root` and `simHits_mixed_10_SIM.log`): (1) the field, (2) the decays in flight, (3) conversions and bremsstrahlung, (4) nuclear interactions, (5) the b and bbar systems in the calorimeters, (6) the followed photon in two runs, (7) the vertices of the 193 mixed collisions, (8) the library collisions at z = +-203 m; section 4.8 |
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
sample pileup: SimHitAnalyzer run 1 event 72: 534 SimTracks, 164 primaries handed to Geant4, PV = (0.0286612, -0.0165738, -0.482677) cm
  TRK   2097 PSimHits, 0 PCaloHits in 0 cells
        by kind: charged hadron=1061 neutral hadron=186 photon=474 no stored ancestor=376
        primaries with hits: 79 of 164
  ECAL  0 PSimHits, 4742 PCaloHits in 3388 cells
        by kind: charged hadron=1448(29.4 GeV) neutral hadron=468(7.45 GeV) photon=1459(15.7 GeV) no stored ancestor=13(0.356 GeV)
        primaries with hits: 108 of 164
  HCAL  0 PSimHits, 392 PCaloHits in 262 cells
        by kind: charged hadron=204(145 GeV) neutral hadron=33(10 GeV) photon=25(18 GeV)
        primaries with hits: 52 of 164
  MUON  11 PSimHits, 0 PCaloHits in 0 cells
        by kind: no stored ancestor=11
        primaries with hits: 0 of 164
  ALL   2108 PSimHits, 5134 PCaloHits in 3650 cells
        by kind: charged hadron=2713(175 GeV) neutral hadron=687(17.5 GeV) photon=1958(33.7 GeV) no stored ancestor=400(0.356 GeV)
        primaries with hits: 142 of 164
sample mixed: SimHitAnalyzer run 1 event 1: 1926 SimTracks, 571 primaries handed to Geant4, PV = (0.0276598, -0.0169037, 3.11193) cm
  pileup mixed in (CrossingFrame, bunch crossings -1 .. 1): 193 events (early 71, in-time 68, late 54), 31060 primaries (10012, 10414, 10634), 97276 SimTracks; events per bx: -1:71 0:68 1:54
  TRK   448315 PSimHits, 0 PCaloHits in 0 cells (signal: 10272 PSimHits, 0 PCaloHits)
        by kind: charged hadron=4999 neutral hadron=890 photon=2013 e^{#pm}=138 no stored ancestor=2232 pileup, early (bx < 0)=139551 pileup, in-time (bx 0)=146028 pileup, late (bx > 0)=152464
        primaries with hits: 293 of 571
  ECAL  0 PSimHits, 939254 PCaloHits in 106170 cells (signal: 0 PSimHits, 24262 PCaloHits; 2362 cells dominated by the signal, 103808 by pileup)
        by kind: charged hadron=950(69.3 GeV) neutral hadron=345(32.4 GeV) photon=1011(129 GeV) e^{#pm}=56(5.64 GeV) pileup, early (bx < 0)=32941(3.95e+03 GeV) pileup, in-time (bx 0)=34418(3.86e+03 GeV) pileup, late (bx > 0)=36449(4.32e+03 GeV)
        primaries with hits: 272 of 571
  HCAL  0 PSimHits, 99833 PCaloHits in 11021 cells (signal: 0 PSimHits, 2531 PCaloHits; 258 cells dominated by the signal, 10763 by pileup)
        by kind: charged hadron=224(897 GeV) neutral hadron=26(175 GeV) photon=6(0.0149 GeV) no stored ancestor=2(0.000918 GeV) pileup, early (bx < 0)=3322(1.55e+04 GeV) pileup, in-time (bx 0)=3529(1.65e+04 GeV) pileup, late (bx > 0)=3912(2.09e+04 GeV)
        primaries with hits: 92 of 571
  MUON  204 PSimHits, 0 PCaloHits in 0 cells (signal: 0 PSimHits, 0 PCaloHits)
        by kind: pileup, early (bx < 0)=51 pileup, in-time (bx 0)=129 pileup, late (bx > 0)=24
        primaries with hits: 0 of 571
  ALL   448519 PSimHits, 1039087 PCaloHits in 117191 cells (signal: 10272 PSimHits, 26793 PCaloHits; 2620 cells dominated by the signal, 114571 by pileup)
        by kind: charged hadron=6173(967 GeV) neutral hadron=1261(208 GeV) photon=3030(129 GeV) e^{#pm}=194(5.64 GeV) no stored ancestor=2234(0.000918 GeV) pileup, early (bx < 0)=175865(1.94e+04 GeV) pileup, in-time (bx 0)=184104(2.03e+04 GeV) pileup, late (bx > 0)=192849(2.52e+04 GeV)
        primaries with hits: 378 of 571
```

Reading the "by kind" lines: for the tracker and muon stations the numbers are hits; for the calorimeter stations they are
cells (attributed to the kind of the primary that deposited most of the cell's energy) with the energy of those cells in
parentheses; `ALL` adds the two. The HCAL "GeV" add the visible energy of HB, HE, HO (GeV) and the HF shower-library units
(photoelectrons), as the analyzer sums them. In the chapter: the mixed-event ECAL energies 236 GeV (signal, 69.3 + 32.4 +
129 + 5.64) and 12 130 GeV (pileup, 3950 + 3860 + 4320), total 12 366; the HCAL 1072 (signal) and 52 900 (pileup), total
54 000 (rounded); the tracker pileup hits 139 551 + 146 028 + 152 464 = 438 043; per collision 139 551 / 71 = 1966 (1965.5),
146 028 / 68 = 2147, 152 464 / 54 = 2823; the signal's share 10 272 / 448 315 = 2.3 % (of its own bunch crossing
10 272 / (10 272 + 146 028) = 6.6 %); the signal cells lost to the pileup
12 804 - 2362 = 10 442; 3860 / 68 = 57 GeV of ECAL energy per in-time collision.

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
`simHits_pileup_10_SIM.log` (library event 72), station `ALL`:
```
PXB/1=80 PXB/2=72 PXB/3=81 PXB/4=44 PXF/1=43 PXF/2=67 PXF/3=66 TIB/1=97 TIB/2=46 TIB/3=14 TIB/4=30 TID/1=52 TID/2=56 TID/3=64 TOB/1=91 TOB/2=38 TOB/3=31 TOB/4=27 TOB/5=19 TOB/6=3 TEC/1=135 TEC/2=146 TEC/3=160 TEC/4=116 TEC/5=121 TEC/6=134 TEC/7=83 TEC/8=92 TEC/9=89 CSC/1=11 EB=1217 EE=1948 ES=223 HB=21 HE=122 HF=119
```
The page header of `pileup/ALL/simHits_pileup_10_SIM_ALL_etaphi_run1_event72.png` gives the energies: `EB 1217 cells (9.45 GeV),
EE 1948 cells (43.5 GeV), ES 223 cells (0.0463 GeV), HB 21 cells (0.0175 GeV), HE 122 cells (0.223 GeV), HF 119 cells
(173, shower-library units)`; 9.45 + 43.5 + 0.05 = 53.0 GeV of ECAL energy, 0.0175 + 0.223 + 173 = 173.2 HCAL units;
`5758 hits` = 2108 PSimHits + 3650 cells.

`simHits_mixed_10_SIM.log`, station `ALL`:
```
PXB/1=19053 PXB/2=13201 PXB/3=10078 PXB/4=8621 PXF/1=11399 PXF/2=12317 PXF/3=12984 TIB/1=29220 TIB/2=23887 TIB/3=10109 TIB/4=8725 TID/1=11910 TID/2=12594 TID/3=13393 TOB/1=23175 TOB/2=20098 TOB/3=8412 TOB/4=6786 TOB/5=5771 TOB/6=4898 TEC/1=22851 TEC/2=24335 TEC/3=25839 TEC/4=19769 TEC/5=20618 TEC/6=21515 TEC/7=15890 TEC/8=16975 TEC/9=13892 DT/1=37 DT/2=6 CSC/1=86 CSC/2=19 CSC/3=33 CSC/4=13 RPC/1=2 GEM/1=8 EB=59897 EE=14619 ES=31654 HB=3624 HE=4170 HO=15 HF=3212
```
The page header of `mixed/ALL/simHits_mixed_10_SIM_ALL_etaphi_run1_event1.png`: `565710 hits: ... EB 59897 cells (2.63e+03 GeV),
EE 14619 cells (9.72e+03 GeV), ES 31654 cells (9.49 GeV), HB 3624 cells (10.3 GeV), HE 4170 cells (60.1 GeV), HO 15 cells
(0.0108 GeV), HF 3212 cells (5.39e+04, shower-library units)`; `signal + pileup (MixingModule): 12892 hits of the signal
event + 552818 hits of the 193 mixed pileup events (31060 pileup primaries)`; the followed primaries `pi+ 31 GeV` and
`K0S 16 GeV`. 565 710 = 448 519 PSimHits + 117 191 cells; 12 892 = 10 272 + 2620.

The per-collision list of the mixed log (193 lines, `pileup event <k> in bx <b>: <primaries> primaries, <SimTracks>
SimTracks, <SimVertices> SimVertices, vertex z = <z> cm, t = <t> ns`), first and last of each bunch crossing:
```
    pileup event 1 in bx -1: 178 primaries, 662 SimTracks, 271 SimVertices, vertex z = -3.30 cm, t = -25.0 ns
    pileup event 71 in bx -1: 139 primaries, 445 SimTracks, 182 SimVertices, vertex z = -8.02 cm, t = -25.0 ns
    pileup event 1 in bx 0: 164 primaries, 534 SimTracks, 208 SimVertices, vertex z = -0.48 cm, t = 0.0 ns
    pileup event 68 in bx 0: 69 primaries, 201 SimTracks, 78 SimVertices, vertex z = 3.51 cm, t = 0.0 ns
    pileup event 1 in bx 1: 175 primaries, 575 SimTracks, 219 SimVertices, vertex z = -2.53 cm, t = 25.0 ns
    pileup event 54 in bx 1: 428 primaries, 1417 SimTracks, 543 SimVertices, vertex z = -3.04 cm, t = 25.0 ns
```
Event 1 of bx 0 is library event 72
(164 primaries, 534 SimTracks, z = -0.48 cm, as in the pileup sample); the pileup times are the bunch-crossing shift.

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

Over the 250 events of `step2_SIM_pileup_10_SIM.root`: `primaries (SimTracks with genpartIndex > 0) 38862, SimTracks 120574,
tracker PSimHits 548028, EcalHitsEB 394627 hits (3149.2 GeV), EcalHitsEE 691000 hits (11616.1 GeV), HcalHits 119964`;
the same loop on `test/09_G4/out/step2_G4_pileup_09_G4.root` gives `38862 primaries, 38862 SimTracks, 382523 tracker
crossings`. For library events 72 to 139: `68 events, 10414 primaries, 32406 SimTracks, 146028 tracker hits` (full
physics; 10 414 primaries, 10 414 SimTracks and 102 018 crossings transport-only); the smallest and largest events of the
library have 1 (event 12) and 1897 (event 247) SimTracks.

Reconciliation of the 10 414 with the 10 387 primaries and 27 PPS protons of chapter 09 (events 72 to 139): FWLite
gives `SimTracks with link 10414, duplicate links 1, distinct status-1 particles with a SimTrack 10388, other 25`. The 25 `SimTrack`s whose
link is the barcode of a non-status-1 particle of `generatorSmeared` are all protons (`type() = 2212`) of 3040 to 6800 GeV
with their vertex at z = +-20300 cm, r = 0.03 cm: the forward protons that `LHCTransport` delivers to the PPS region
(section 4.8, (8)), not particles of the minimum-bias record. The `generatorSmeared` particle that happens to carry the
same barcode is, by PDG id, `{21: 14, -1: 2, -2: 2, 2: 2, 1: 1, 213: 1, 2101: 1, 113: 1, 313: 1}` (e.g. event 78: a proton of
4132 GeV, the barcode of a gluon of 495 GeV; event 124: protons of 6800 and 5539 GeV, the barcodes of a u quark of 171 GeV and
a ud diquark of 6629 GeV); FWLite on events 72-139 of `step2_SIM_pileup_10_SIM.root`, `SimTrack::type()`, the vertex z and
`HepMC::GenEvent::barcode_to_particle` of `generatorSmeared`. The `g4Ledger.py` of
chapter 09 counts, since 13 November 2026, the PPS protons apart: it finds the `SimTrack`s with their vertex at
|z| > 100 m and matches them to their generator protons by energy (27 protons, 150058.2 GeV), and counts as primaries
the status-1 particles whose barcode is the `genpartIndex()` of a `SimTrack` from the collision (10 387; before the
fix 10 388, the proton of event 132, whose link happens to equal its barcode, being counted as a primary). The
`SimHitAnalyzer` counts all `SimTrack`s with `genpartIndex() > 0`: 10 387 + 27 = 10 414 (the 25 links above, the one
of event 132, and the one of event 75 that coincides with the barcode of a stable proton of 6.0 GeV that is a primary
as well, hence the "duplicate link"). For the signal both give 571.

### 4.6 The release parameters quoted in "Open problems for Phase-2"

All under `$CMSSW_RELEASE_BASE/src` (`/cvmfs/cms.cern.ch/el9_amd64_gcc12/cms/cmssw/CMSSW_15_0_5/src`):

| parameter | value | file (line) |
|---|---|---|
| `CaloSD.EminTrack`, `TmaxHit` | `1.0` GeV, `1000.0` ns | `SimG4Core/Application/python/g4SimHits_cfi.py` (382-383) |
| `CaloSD.HCNames`, `EminHits`, `TmaxHits` | `('EcalHitsEB','EcalHitsEE','EcalHitsES','HcalHits','ZDCHITS')`, `(0.015,0.010,0.0,0.0,0.0)` MeV (`SimG4CMS/Calo/src/CaloSD.cc` line 90: `eminHits[k] * CLHEP::MeV`, i.e. 15 keV and 10 keV), `(500,500,500,500,2000)` | same (384-387) |
| `HGCSD.EminHit`, `TimeSliceUnit` | `0.0`, `0.001` ns | same (547-549) |
| `HGCScintSD.EminHit`, `UseBirkLaw`, `BirkC3`, `BirkC2`, `BirkC1` | `0.0`, `True`, `1.75`, `0.142`, `0.0060` | same (567-574) |
| `TotemT2ScintSD.BirkC3`, `BirkC2`, `BirkC1`; `AHCalSD.BirkC3`, `BirkC2`, `BirkC1` | `1.75`, `0.142`, `0.006`; `1.75`, `0.142`, `0.0052` | same (601-605, 664-668) |
| `HCalSD.UseBirkLaw`, `BirkC3`, `BirkC2`, `BirkC1`, comment | `True`, `1.75`, `0.142`, `0.0060`, "Values of Birks constants from NIM 80 (1970) 239-244: as implemented in Geant3 required correction due to biased computation of enery deposition" | same (426-432) |
| `HCalSD.UseShowerLibrary`, `HBDarkening`, `HEDarkening`, `HFDarkening` | `True`, `False`, `False`, `False` | same (433, 446-448) |
| `ECalSD.UseBirkLaw`, `BirkL3Parametrization`, `BirkSlope`, `BirkCut`, `BirkC1`, `BirkC3`, `BirkC2` | `True`, `True`, `0.253694`, `0.1`, `0.03333`, `1.0`, `0.0` (applied to the EB and the EE crystals, `useWeight`; not to the ES: `SimG4CMS/Calo/src/ECalSD.cc` lines 121, 205-210) | same (403-409) |
| `common_UseLuminosity.InstLuminosity`, `DelivLuminosity` | `0.`, `5000.` | same (50-53), used by `ECalSD` (402) and `HCalSD` (425) |
| `HFDarkeningParameterBlock` | the dose table | `SimG4CMS/Calo/python/HFDarkeningParams_cff.py`, loaded at line 6 of `g4SimHits_cfi.py`, passed at line 456 |
| `HFShower.UseShowerLibrary`, `ApplyFiducialCut` | `True`, `True` | `g4SimHits_cfi.py` (469, 473) |
| `HFLibraryFileBlock.FileName`, `FileVersion`; `HFShowerBlock.EqualizeTimeShift` | `HFShowerLibrary_run3_v7.root`, `3`; `True` under `run3_HFSL` (default `HFShowerLibrary_oldpmt_noatt_eta4_16en_v3.root`, `0`, `False`) | `Geometry/HcalSimData/python/HFParameters_cff.py` |
| `run3_HFSL` in the eras | in `Run3` (`Configuration/Eras/python/Era_Run3_cff.py`, line 23); `Phase2 = ModifierChain(Run3_noMkFit.copyAndExclude([...]), phase2_common, ...)` | `Configuration/Eras/python/Era_Phase2_cff.py` |
| `Physics.type`, `EminFTFP`, `EmaxBERT`, `EmaxBERTpi`, `DefaultCutValue`, `CutsPerRegion`, `G4BremsstrahlungThreshold`, `G4TrackingCut`, `LimitsPerRegion` | `SimG4Core/Physics/FTFP_BERT_EMM`, `3.`, `6.`, `12.`, `1.0` cm, `True`, `0.5` GeV, `0.025` MeV, `('EcalRegion','HcalRegion')` | `g4SimHits_cfi.py` (184-282) |
| era modifiers of `g4SimHits` | `run2_common` (ZdcSD), `run2_HCAL_2017` (TestNumberingScheme), `run3_common` (OnlySDs, TrackHits, CaloHits, LHCTransport), `phase2_timing` (ECalSD time slices), `h2tb`, `dd4hep`, `phase2_common` (OnlySDs with HGCal, MTD, HFNose; LHCTransport False; MuonSD.HaveDemoChambers False), `hgcaltb`, `phase2_hgcalOnly`, `phase2_hgcalV18` (HGCSD.HitCollection 2), `fixLongLivedSleptonSim`; `phase2_hgcal` on `common_MCtruth` (SaveCaloBoundaryInformation True) | `g4SimHits_cfi.py` (77-80, 675-800) |
| `mix` of `mix_POISSON_average_cfi` | `minBunch -5`, `maxBunch 3`, `bunchspace 450`, `input.type 'poisson'`, `sequential False`, `playback False` (overridden by `defineMixing`) | `SimGeneral/MixingModule/python/mix_POISSON_average_cfi.py` |
| `AVE_60_BX_25ns`, `AVE_200_BX_25ns`, `_m12p3`, `_m6p6` | `{'BX':25, 'B': (-12,3), 'N': 60}`, `{'BX':25, 'B': (-3,3), 'N': 200}`, `B (-12,3)`, `B (-6,6)` | `Configuration/StandardSequences/python/Mixing.py` (162, 184-186) |
| `2025_LHC_Simulation_fill12p5h_levelling9h_nb2340_xSec80mb_il2p23` (the 2025 profile) | `probFunctionVariable 0..99` with its `probValue`s (mean 64.34, r.m.s. 11.70, median 65, mode 66; 34.1 % of the crossings with at most 60), on `mix_probFunction_25ns_PoissonOOTPU_cfi` (`minBunch -12`, `maxBunch 3`, `bunchspace 25`, `type 'probFunction'`); header comment "A distribution for Run3 studies (2023) derived from LHC simulations", 2340 colliding bunches, hadronic cross section 80 mb, fill 750 min, levelling 540 min | `SimGeneral/MixingModule/python/Run3_2025_LHC_Simulation_fill12p5h_levelling9h_nb2340_xSec80mb_il2p23_cfi.py`; `Configuration/StandardSequences/python/Mixing.py` (139) |
| `2024_25ns_RunIII2024Summer24_PoissonOOTPU` (the 2024 profile) | the same structure, mean 44.92, r.m.s. 9.53 (98.7 % of the crossings with at most 60) | `SimGeneral/MixingModule/python/mix_2024_25ns_RunIII2024Summer24_PoissonOOTPU_cfi.py`; `Mixing.py` (132) |
| `PUDataSets` of the relvals | `'Run4'`: `'--pileup':'AVE_200_BX_25ns', '--pileup_input':'das:/RelValMinBias_14TeV/.../GEN-SIM'`; 2022-2025: `'Run3_Flat55To75_PoissonOOTPU'` | `Configuration/PyReleaseValidation/python/relval_steps.py` (4607-4619) |
| premixing workflows | stage 1: `-s GEN,SIM,DIGI:pdigi_valid --datatier PREMIX --eventcontent PREMIX --procModifiers premix_stage1`; stage 2: `DIGI,DATAMIX --datamix PreMix --procModifiers premix_stage2` | `Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py` (`UpgradeWorkflowPremix`, 2311-2400) |
| `mixSimHits.pcrossingFrames` | `[]`, `premix_stage1`: `['MuonCSCHits','MuonDTHits','MuonRPCHits']`, `+ MuonGEMHits` (`run3_GEM`, `run2_GEM_2017`), `+ MuonME0Hits` (`phase2_muon`), removed again by `phase2_GE0` | `SimGeneral/MixingModule/python/mixObjects_cfi.py` (45-58, 234-278) |
| `theMixObjects` Phase-2 | `phase2_hgcal`: `mixCH` + `HGCHitsEE`, `HGCHitsHEfront`, `HGCHitsHEback`; `phase2_hfnose`: + `HFNoseHits`; `phase2_timing_layer`: `mixSH` + `FastTimerHitsBarrel`, `FastTimerHitsEndcap` | same (280-305) |
| `theDigitizers` premix | `(premix_stage2 & phase2_hgcal)`: `hgceeDigitizer`, `hgchebackDigitizer`, `hgchefrontDigitizer` `premixStage1 = True`, `calotruth premixStage1 = True` ("HGCAL still needs calotruth for production mode"); `(premix_stage2 & phase2_timing_layer)`: `mtdDigitizer` barrel/endcap `premixStage1 = True` | `SimGeneral/MixingModule/python/digitizers_cfi.py` (92-110) |
| `mixData` (`PreMixingModule`) | `input.type 'fixed'`, `averageNumber 1.0`, `sequential False`, `bunchspace 25`, `minBunch 0`, `maxBunch 0`, `workers.pileup.CFPlaybackInputTag = "mix"` | `SimGeneral/PreMixingModule/python/mixOne_premix_on_sim_cfi.py` |
| `premix_stage1`, `premix_stage2` | `cms.Modifier()` with the comments "the production of pre-mixed pileup library" / "mixing the premixed pileup library with the signal event" | `Configuration/ProcessModifiers/python/premix_stage{1,2}_cff.py` |
| the mixing order | `BMixingModule::produce`: `initializeEvent`, `addSignals`, `doPileUp`; `PileUp::CalculatePileup` | `Mixing/Base/src/BMixingModule.cc`, `Mixing/Base/src/PileUp.cc`, `SimGeneral/MixingModule/plugins/MixingModule.cc` (chapter 11, section 1, for the line numbers) |
| darkening switches searched | no `HBDarkening = True`, `HEDarkening = True` or `HFDarkening = True` under `Configuration/`, `SimG4Core/`, `SimG4CMS/` python except `SimG4Core/Application/test/GeometryProducer_cfg.py` and `SimG4CMS/Calo/test/python/runHFPMT_cfg.py` (both `False`) | `grep -rn` over the three packages |

The Geant4 version of every log: `geant4-11-02-patch-02 [MT] (21-June-2024)`.

### 4.7 The 2024 and the 2025 settings

The chapter runs with `YEAR=2025` (`common/jitjet_common.sh`): `auto:phase1_2025_realistic` =
`150X_mcRun3_2025_realistic_v1`, era `Run3_2025`, for the five SIM jobs, the mixing job and the analyzers
(`simAnalyzer_cfg.py` defaults to them; `simHitAnalyzer_cfg.py` receives them from the script). The geometry, the
field map and the simulated beam spot are the same tags as in 2024 (chapter 09 companion, section 4.7; chapter 08
companion, section 4.9); the five modifiers of `Run3_2025` occur neither in `g4SimHits_cfi.py` nor in
`SimGeneral/MixingModule/python/`.

The rerun with the 2025 settings (12 November 2026, `REUSE=0`) against the 2024 run (7 November 2026):

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

-- (7) the 193 mixed collisions (simHits_mixed_10_SIM.log, lines "pileup event")
  190 collisions with |z| < 1 m: mean z = 0.13 cm, rms 3.76 cm, min -9.52, max 10.08; within |z - 3.11| < 0.1 cm of the signal vertex: 2
  outside: bx -1 pileup event 12 at z = -20300 cm (1 primary)
  outside: bx +0 pileup event 53 at z = -20300 cm (2 primary)
  outside: bx +1 pileup event 40 at z = 20300 cm (1 primary)
  bx -1: 71 collisions, primaries per collision: min 1, median 69, max 537, sum 10012
  bx +0: 68 collisions, primaries per collision: min 2, median 114.5, max 470, sum 10414
  bx +1: 54 collisions, primaries per collision: min 1, median 148.5, max 574, sum 10634

-- (8) the library collisions with a vertex at |z| = 203 m (step2_SIM_pileup_10_SIM.root, library events 12, 124, 179)
  library event 12: SimTrack pdg 2212, E = 6800 GeV, primary True, vertex z = -20300 cm
  library event 124: SimTrack pdg 2212, E = 6800 GeV, primary True, vertex z = -20300 cm
  library event 124: SimTrack pdg 2212, E = 5539 GeV, primary True, vertex z = 20300 cm
  library event 179: SimTrack pdg 2212, E = 6800 GeV, primary True, vertex z = 20300 cm
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
| the followed photon and pi+ in the mixed event: 80 crystals with 42.6 GeV, 8 with 3.9 GeV | the header of `mixed/ECAL/simHits_mixed_10_SIM_ECAL_run1_event1.pdf` (`pdftotext`): `π+ 13.3 GeV (η -1.54), 3.91 GeV in 8 cells; KS 15.7 GeV (η -3.77), 4.56 GeV in 63 cells; γ 17.7 GeV (η -1.57), 42.57 GeV in 80 cells` |
| the followed primaries of the signal: photon 270 cells with 42.0 GeV, pi+ 35 cells with 3.4 GeV, 171 tracker hits of the photon, 809 HF units in 19 cells of the HF pi+, K0S 203 hits | the headers of `signal/{TRK,ECAL,HCAL}/simHits_signal_10_SIM_<station>_run1_event1.pdf`: `π+ 13.3 GeV (η -1.54), 29 hits; KS 15.7 GeV (η -3.77), 203 hits; γ 17.7 GeV (η -1.57), 171 hits; e 5.0 GeV (η -1.43), 138 hits`; `π+ 13.3 GeV (η -1.54), 3.43 GeV in 35 cells; ...; γ 17.7 GeV (η -1.57), 42.03 GeV in 270 cells`; `π+ 30.6 GeV (η -3.74), 809 (HF units) in 19 HF cells`; the "GeV" of these labels is the pT of the primary (`plugins/SimHitAnalyzer.cc`, `highlightLine`) |
| 98 % of the barrel crystals hit; EE 14 619 of 14 648; ES 31 654 of 137 216; HB 3624 of 9072; HF 3212 of 3456 | `simHits_mixed_10_SIM.log` (cells per subdetector) against the detector elements of `simHits_signal_10_SIM.log` (`EB=61200 EE=14648 ES=137216 HB=9072 HE=6768 HO=2160 HF=3456`); 59 897 / 61 200 = 97.9 % |
| EE 9720 GeV, 33 times the signal | the header of `mixed/ECAL/...`: `EE 14619 cells (9.72e+03 GeV)`; 9720 / 293.66 = 33.1 |
| PU vertices: r.m.s. 3.76 cm, -9.5 to +10.1 cm, 2 within 1 mm of z = 3.11 cm | section (7); the three collisions at z = +-203 m left out |
| primaries per collision: median 69, 114.5, 148.5 in bx -1, 0, +1 | section (7) of `extras_10_SIM.txt`: for the even counts 68 and 54 the mean of the two middle values, (103 + 126) / 2 and (143 + 154) / 2 (the script printed the upper middle value, 126 and 154, until it was fixed; the same from the `pileup event` lines of `simHits_mixed_10_SIM.log`) |
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
