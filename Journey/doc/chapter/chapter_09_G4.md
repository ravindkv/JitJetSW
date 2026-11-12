# Chapter 09, G4: technical companion

The hand-over of the generator final state to Geant4, seen with the Geant4 physics reduced to the transportation.
Script: `test/09_G4/run_09_G4.sh`; everything it produces is in `test/09_G4/out/`. The chapter text is
`doc/chapter/chapter_09_G4.tex`. All commands below are run from `test/` of an initialized CMSSW_15_0_5 (`cmsenv`),
with `J=$CMSSW_BASE/src/JitJetSW/Journey`.

## 1. The cmsDriver.py commands

The script runs `cmsDriver.py` twice through `run_driver` of `test/common/jitjet_common.sh`, once for the signal (9a)
and once for the pileup library (9b). The commands, copied from the header of the generated configurations
(`test/09_G4/step2_G4_signal_09_G4_cfg.py` and `step2_G4_pileup_09_G4_cfg.py`, "with command line options"; the
cmsDriver output is in `out/step2_G4_signal_09_G4_cfg.driver.log` and `out/step2_G4_pileup_09_G4_cfg.driver.log`):

```bash
cd $J/test
cmsDriver.py step2 --filein file:$J/test/07_GenJet/out/step2_GENJET_07_GenJet.root \
  --conditions auto:phase1_2025_realistic --era Run3_2025 --geometry DB:Extended --beamspot DBrealistic \
  -n 1 -s SIM --datatier GEN-SIM --eventcontent FEVTDEBUG --fileout file:step2_G4_signal_09_G4.root \
  --customise_commands "process.g4SimHits.UseMagneticField = cms.bool(False)\nprocess.g4SimHits.Physics.type = cms.string('SimG4Core/Physics/JourneyTransportPhysics')\nprocess.g4SimHits.Physics.G4TransportWithMSC = cms.int32(0)\nprocess.g4SimHits.Physics.G4GammaGeneralProcess = cms.bool(False)\nprocess.g4SimHits.TrackerSD.ZeroEnergyLoss = cms.bool(True)" \
  --python_filename $J/test/09_G4/step2_G4_signal_09_G4_cfg.py --no_exec

cmsDriver.py step2 --filein file:$J/test/08_PU/out/step1_GEN_MinBias_08_PU.root \
  --conditions auto:phase1_2025_realistic --era Run3_2025 --geometry DB:Extended --beamspot DBrealistic \
  -n -1 -s SIM --datatier GEN-SIM --eventcontent FEVTDEBUG --fileout file:step2_G4_pileup_09_G4.root \
  --customise_commands "process.g4SimHits.UseMagneticField = cms.bool(False)\nprocess.g4SimHits.Physics.type = cms.string('SimG4Core/Physics/JourneyTransportPhysics')\nprocess.g4SimHits.Physics.G4TransportWithMSC = cms.int32(0)\nprocess.g4SimHits.Physics.G4GammaGeneralProcess = cms.bool(False)\nprocess.g4SimHits.TrackerSD.ZeroEnergyLoss = cms.bool(True)\nprocess.RandomNumberGeneratorService.g4SimHits.initialSeed = 20243" \
  --python_filename $J/test/09_G4/step2_G4_pileup_09_G4_cfg.py --no_exec
```

| argument | meaning |
|---|---|
| `step2` | the name of the configuration (free; the `step1` was the GEN step of chapters 01 to 08) |
| `--filein file:...` | the input: the GEN-GENJET file of chapter 07 (signal) or the 250-event library of chapter 08 (pileup). Rule E |
| `--conditions auto:phase1_2025_realistic` | the global tag of 2025 (resolved to `150X_mcRun3_2025_realistic_v1` by `Configuration/AlCa/python/autoCond.py`, line 107; `YEAR=2025` of `jitjet_common.sh`): geometry, magnetic field map, beam spot from the database. The geometry (`GeometryFileRcd`, label `Extended`: `141DD4hepV2_Extended2024`) and the field (`MFGeometry_160812`, `MFConfig_160812_Run2_3_8T`) are the same tags as in the 2024 global tag `150X_mcRun3_2024_realistic_v2` (section 4) |
| `--era Run3_2025` | the era modifiers of the 2025 detector (`Configuration/Eras/python/Era_Run3_2025_cff.py` = `Run3_2024` + `run3_GEM_2025`, `run3_CSC_2025`, `run3_SiPixel_2025`, `stage2L1Trigger_2025`, `ecal_cctiming`; none of the five appears in `g4SimHits_cfi.py`); through `Run3` they select the sensitive detectors and hit collections of Run 3 in `g4SimHits_cfi.py` (`run3_common`) |
| `--geometry DB:Extended` | the simulation geometry (DDD XML) is read from the database (`GeometrySimDB_cff`), label `Extended` |
| `--beamspot DBrealistic` | the vertex smearing of the GEN step; in a SIM-only step it only sets the matching `VtxSmeared` label, the vertices are already in the input |
| `-n 1` / `-n -1` | one event (the signal file holds one) / all events (the 250 library collisions) |
| `-s SIM` | the step: the `psim` sequence (`Configuration/StandardSequences/python/SimIdeal_cff.py` = `g4SimHits` + `randomEngineStateProducer`) |
| `--datatier GEN-SIM --eventcontent FEVTDEBUG` | the output tier and the full debug event content: every `SimTrack`, `SimVertex`, `PSimHit` and `PCaloHit` collection is kept, plus everything of the input |
| `--fileout file:...` | the output ROOT file (written in `out/`, where `cmsRun` is started) |
| `--customise_commands "..."` | python lines appended to the configuration (`\n` separated): the transport-only setup, see section 2 |
| `--python_filename ... --no_exec` | write the configuration and do not run it; the script runs `cmsRun` itself |

The five customise lines, and why:

| line | effect |
|---|---|
| `process.g4SimHits.UseMagneticField = cms.bool(False)` | no field: the primaries fly in straight lines |
| `process.g4SimHits.Physics.type = cms.string('SimG4Core/Physics/JourneyTransportPhysics')` | the physics list of `$J/plugins/JourneyTransportPhysics.cc`: every Geant4 particle is constructed, no process is registered, only the transportation that `G4VModularPhysicsList` adds itself. The standard lists (`FTFP_BERT_EMM` with `EMPhysics`/`HadPhysics` off, `DummyPhysics`) all register `G4DecayPhysics`, and `/process/inactivate Decay` in `G4Commands` has no effect because the commands are applied before the processes exist |
| `process.g4SimHits.Physics.G4TransportWithMSC = cms.int32(0)` | no coupling of the transportation to multiple scattering (there is none) |
| `process.g4SimHits.Physics.G4GammaGeneralProcess = cms.bool(False)` | no "general" photon process wrapper (there are no photon processes) |
| `process.g4SimHits.TrackerSD.ZeroEnergyLoss = cms.bool(True)` | the tracker sensitive detector (`SimG4CMS/Tracker/src/TkAccumulatingSensitiveDetector.cc`, line 108: `if (aStep->GetTotalEnergyDeposit() > 0. || allowZeroEnergyLoss)`) records every crossing of a sensor as a `PSimHit`, with `energyLoss() = 0`; without it nothing would be recorded, since nothing ionises |
| `process.RandomNumberGeneratorService.g4SimHits.initialSeed = 20243` (pileup only) | the Geant4 seed of the library run, `PUSEED + 2` with `PUSEED = 20241` of chapter 08 (only the HF shower library draws random numbers in this run) |

## 2. The cmsRun on the python config

A physics list built in this package links the shared Geant4 libraries, while the release's simulation "biglib"
(`biglib/pluginSimulation.so`) carries its own statically linked copy of Geant4; with both loaded the job crashes
in the `PhysicsList` constructor. The script therefore runs `cmsRun` with the biglib directories removed from
`LD_LIBRARY_PATH` (`sim_ld_library_path` of `jitjet_common.sh`, `CMSRUN_PREFIX`):

```bash
cd $J/test/09_G4/out
env LD_LIBRARY_PATH=$(echo "$LD_LIBRARY_PATH" | tr ':' '\n' | grep -v '/biglib/' | paste -sd:) \
    cmsRun ../step2_G4_signal_09_G4_cfg.py > step2_G4_signal_09_G4.log 2>&1     # 53 s, mostly initialisation
env LD_LIBRARY_PATH=$(echo "$LD_LIBRARY_PATH" | tr ':' '\n' | grep -v '/biglib/' | paste -sd:) \
    cmsRun ../step2_G4_pileup_09_G4_cfg.py > step2_G4_pileup_09_G4.log 2>&1     # 88 s for the 250 collisions
```

Or the whole chapter: `./09_G4/run_09_G4.sh` (variables: `PUSEED=20241`; `NPU=68` and `SKIP=71` are set in
`test/common/jitjet_common.sh` for the whole book: the MixingModule of chapter 10, seed 12345, Poisson with mean 60 per
bunch crossing, draws library events 1 to 71 into bunch crossing -1 and events 72 to 139 into bunch crossing 0 for this
signal event; `RUN=0` to only write the configurations, `ANALYZE=0` to skip the analyzers, `PNG=0` to skip the PNG pages).

### Walk-through of `step2_G4_signal_09_G4_cfg.py`

- `process = cms.Process('SIM', Run3_2025)`: the process name under which the new products are stored.
- `process.load(...)`: `Services_cff` (the random number service), `pythiapdt_cfi` (the particle table), the
  `MessageLogger`, `EventContent_cff`, `mixNoPU_cfi` (the mixing module without pileup, not run here),
  `GeometryRecoDB_cff` and `GeometrySimDB_cff` (reco and simulation geometry from the database),
  `MagneticField_cff` (loaded, then switched off for `g4SimHits` by the customise line), `SimIdeal_cff` (the
  `g4SimHits` module with all its defaults from `SimG4Core/Application/python/g4SimHits_cfi.py`), `EndOfProcess_cff`,
  `FrontierConditions_GlobalTag_cff`.
- `process.maxEvents`: 1 (signal) or -1 (all library events).
- `process.source`: `PoolSource` on the input file.
- `process.FEVTDEBUGoutput`: the `PoolOutputModule` with `FEVTDEBUGEventContent`, data tier `GEN-SIM`.
- `process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase1_2025_realistic', '')`.
- `process.simulation_step = cms.Path(process.psim)`, the end paths, and the schedule.
- the five (six) customise lines of section 1, then `customiseEarlyDelete` (memory).

What `g4SimHits` does with its defaults (`g4SimHits_cfi.py`), in the order the particles meet it:

1. `Generator` PSet (`HepMCProductLabel = generatorSmeared`): the HepMC record after the vertex smearing is read
   (`SimG4Core/Generators/src/Generator.cc`, `HepMC2G4`). For every generator vertex with at least one particle to
   be tracked, a `G4PrimaryVertex` is made and the particles are converted with the cuts `ApplyPCuts = True`,
   `MinPCut = 0.04` (GeV, total momentum), `MaxPCut = 99999`, `ApplyEtaCuts = True`, `MinEtaCut = -5.5`,
   `MaxEtaCut = 5.5` (geometric: the line from the production vertex is followed to `|z| = RDecLenCut * sinh(5.5)
   = 2.9 cm * 122.3 = 354.8 cm`, and the particle is dropped if it is still inside `r < RDecLenCut = 2.9 cm` there),
   `ApplyPhiCuts = False`, `ApplyPtransCut = False`, `PDGselection.PDGfilterSel = False` with `PDGfilter =
   (21, 1, 2, 3, 4, 5, 6)` (quarks and gluons with status 1 would be filtered out; there are none), `RDecLenCut = 2.9`
   (status-2 particles whose decay vertex has `r >= 2.9 cm` are given to Geant4 undecayed, with their generator
   decay attached as a predefined decay channel; inside the cylinder the generator decay is kept and the daughters
   become primaries), `LDecLenCut = 30.0` (read and printed only), `EtaCutForHector = 8.2` (the very-forward rule:
   status-1 neutrons, antineutrons and photons whose line is inside `r < 2.9 cm` at `|z| = 2.9 cm * sinh(8.2) =
   52.8 m` are added whatever their eta; other particles there, e.g. protons, are left to the PPS transport).
   `setGenId(g4prim, barcode)` keeps the HepMC barcode, which becomes `SimTrack::genpartIndex()`.
2. `StackingAction`: `TrackNeutrino = False` kills the neutrinos (pdg 12, 14, 16, 18) when they are stacked
   (`StackingAction.cc`, line 189: `classification = fKill`); `KillGamma = True` with `GammaThreshold = 0.0001` MeV,
   `KillDeltaRay = False`, `KillHeavy = False`; the Russian roulette parameters (section 4) act on secondaries only.
3. `SteppingAction` with `common_maximum_time`: `MaxTrackTime = 500` ns, `MaxTrackTimeForward = 2000` ns beyond
   `MaxZCentralCMS = 50` m, `DeadRegions = QuadRegion, InterimRegion`, `CriticalEnergyForVacuum = 2` MeV.
4. the sensitive detectors of `OnlySDs` (the `run3_common` list: `TkAccumulatingSensitiveDetector`,
   `EcalSensitiveDetector`, `HcalSensitiveDetector`, `MuonSensitiveDetector`, `CaloTrkProcessing`, the BRIL and
   forward detectors), with `TrackerSD` (`ZeroEnergyLoss`, `ElectronicSigmaInNanoSeconds = 12.06`:
   hits with time of flight below 3 sigma = 36.18 ns go to the `LowTof` collections, the others to `HighTof`;
   `EnergyThresholdForPersistencyInGeV = 0.2`, `EnergyThresholdForHistoryInGeV = 0.05`), `CaloSD`
   (`EminHits = 0.015, 0.010, 0, 0, 0` GeV for EB, EE, ES, HCAL, ZDC; `TmaxHits = 500 ns`, ZDC 2000), `HCalSD` and
   `HFShower` (`UseShowerLibrary = True`: in the HF the first step of any particle but a muon calls the shower
   library, `HCalSD::getFromHFLibrary`, which returns the photoelectrons of a stored shower and kills the track;
   each photoelectron is booked as a hit of 1 GeV, so the stored HF "energy" is a photoelectron count), and
   `CaloTrkProcessing`, which fills `SimTrack::trackerSurfacePosition()` / `trackerSurfaceMomentum()` and the
   `crossedBoundary()` flag when a track steps from the `Tracker` into the `CALO` volume.

### The output ROOT file

`edmDumpEventContent file:$J/test/09_G4/out/step2_G4_signal_09_G4.root` lists everything of the input (the
`GEN` products: `genParticles`, `generatorSmeared`, `LHCTransport`, `genMetTrue`, ...; the `GENJET` jets of chapter 07)
plus the `SIM` products:

```
vector<SimTrack>     "g4SimHits"   ""                               "SIM"   571 primaries (signal event)
vector<SimVertex>    "g4SimHits"   ""                               "SIM"   29: the primary vertex + 28 generator decay vertices
vector<PSimHit>      "g4SimHits"   "TrackerHitsPixelBarrelLowTof"   "SIM"   and PixelEndcap, TIB, TID, TOB, TEC, each Low/HighTof
vector<PSimHit>      "g4SimHits"   "MuonDTHits" / "MuonCSCHits" / "MuonRPCHits" / "MuonGEMHits" / "MuonME0Hits"   (empty here)
vector<PSimHit>      "g4SimHits"   "BCM1FHits", "BHMHits", "BSCHits", "PLTHits", "CTPPSPixelHits", "CTPPSTimingHits", "TotemHitsRP", "TotemHitsT1"
vector<PCaloHit>     "g4SimHits"   "EcalHitsEB" / "EcalHitsEE" / "EcalHitsES" / "HcalHits" / "CaloHitsTk" / "ZDCHITS" / "TotemHitsT2Scint"
edm::TriggerResults, edm::RandomEngineStates "randomEngineStateProducer"    "SIM"
```

The four simulation objects, as the student will meet them in the next chapters:

| object | package | content |
|---|---|---|
| `SimTrack` | `SimDataFormats/Track` | one Geant4 track kept for the record: `type()` (pdg), `momentum()` at production, `trackId()` (the Geant4 id), `vertIndex()` (its `SimVertex`), `genpartIndex()` (the HepMC barcode if it is a primary, -1 otherwise), `charge()`, `trackerSurfacePosition()` / `trackerSurfaceMomentum()` and `crossedBoundary()` (the tracker/calorimeter boundary) |
| `SimVertex` | `SimDataFormats/Vertex` | a production point: `position()` (x, y, z, t), `parentIndex()` (the `trackId` of the track that produced it, `noParent()` for generator vertices), `processType()` (the Geant4 process sub-type, 0 for generator vertices) |
| `PSimHit` | `SimDataFormats/TrackingHit` | a crossing of a tracker or muon sensor: `entryPoint()` / `exitPoint()` in the local frame of the sensor, `energyLoss()`, `timeOfFlight()`, `pabs()`, `particleType()`, `detUnitId()`, `trackId()`, `processType()` |
| `PCaloHit` | `SimDataFormats/CaloHit` | an energy deposit summed per cell and track: `energy()` (with `energyEM()` / `energyHad()`), `time()`, `id()` (the cell `DetId`, in the HCAL "test numbering" of the simulation), `geantTrackId()` (the track that entered the calorimeter), `depth()` |

## 3. The analyzer configs

`test/common/simAnalyzer_cfg.py` runs `GenPartAnalyzer` (the five generator-stage pages of chapters 01 to 06) and
`SimAnalyzer` (five SIM pages) on one event into a single PDF, and writes a ROOT file with the histograms of every
page and a log with the per-page counts. The commands of the script (`run_09_G4.log`):

```bash
cd $J/test/09_G4/out
cmsRun ../../common/simAnalyzer_cfg.py inputFiles=file:$J/test/09_G4/out/step2_G4_signal_09_G4.root \
  outputFile=simAnalyzer_signal_09_G4.root maxEvents=1 eventsToProcess=1:1:1 simStage=1 genStages=5 \
  globalTag=auto:phase1_2025_realistic era=Run3_2025 geometry=DB:Extended pdfFile=simAnalyzer_signal_09_G4.pdf \
  sampleLabel="chapter 09_G4 (9a): the signal handed to Geant4|transport only: no B field, no decays|no EM, no hadronic physics|bbbar pTHat > 100 GeV, 13.6 TeV" \
  > simAnalyzer_signal_09_G4.log 2>&1
cmsRun ../../common/simAnalyzer_cfg.py inputFiles=file:$J/test/09_G4/out/step2_G4_pileup_09_G4.root \
  outputFile=simAnalyzer_pileup_09_G4.root maxEvents=1 eventsToProcess=1:1:72 simStage=1 genStages=5 \
  globalTag=auto:phase1_2025_realistic era=Run3_2025 geometry=DB:Extended pdfFile=simAnalyzer_pileup_09_G4.pdf \
  sampleLabel="chapter 09_G4 (9b): one pileup collision handed to Geant4|library event 72, transport only|no B field, no decays, no EM, no hadronic physics|SoftQCD inelastic, 13.6 TeV" \
  > simAnalyzer_pileup_09_G4.log 2>&1
pdftoppm -cropbox -r 110 -png simAnalyzer_signal_09_G4.pdf simAnalyzer_signal_09_G4_page     # the PNG pages
pdftoppm -cropbox -r 110 -png simAnalyzer_pileup_09_G4.pdf simAnalyzer_pileup_09_G4_page
python3 ../../common/g4Ledger.py step2_G4_signal_09_G4.root --events 1:1:1 --label "(9a) signal"
python3 ../../common/g4Ledger.py step2_G4_pileup_09_G4.root --events 1:1:72-1:1:139 --label "(9b) the 68 in-time pileup collisions"
```

Options: `simStage=1` highlights "1 transport" in the right panel of every page; `genStages=5` draws the five
generator pages first (0 to skip them); `maxPage` (default 5) stops after a given SIM page; `etaMax` (6.0) the eta
range; `labelPtMin` (5 GeV) the pT above which the value is written next to a particle; `printTracks=True` prints
every `SimTrack` with its generator link, ancestor primary and creator process; `hcalTestNumbering` (True) relabels
the HCAL ids; `eventsToProcess=run:lumi:event`. The global tag, era and geometry are needed for the hit positions of
pages 4 and 5 (`GeometryRecoDB`).

What each run produces in `out/`:

| file | content |
|---|---|
| `simAnalyzer_signal_09_G4.pdf`, `_page-01..10.png` | pages 1 to 5: the generator stages (hard process, parton shower, MPI and beam remnants, hadronisation, decays) as in chapters 01 to 06; page 6: the primaries handed to Geant4 (status 1 `genParticles` with a `SimTrack`, by kind, the status 1 particles without `SimTrack` as grey crosses, the smeared primary vertex); page 7: the primaries at the tracker/calorimeter boundary (marker at the crossing point, line from the production direction: no line here, no field); page 8: the secondaries by creator process (none); page 9: the tracker and muon `PSimHit`s at their global position, by subdetector; page 10: the `PCaloHit`s summed per cell, marker size growing with the energy |
| `simAnalyzer_signal_09_G4.log` | the per-page counts quoted in the chapter (section 4) |
| `simAnalyzer_signal_09_G4.root` | `TFileService` histograms per page (eta-phi, multiplicities, kind / process / subdetector, b and bbar sums) |
| `simAnalyzer_pileup_09_G4.pdf/.png/.log/.root` | the same ten pages for library event 72 (`eventsToProcess=1:1:72`, the first collision of bunch crossing 0) |
| `ledger_09_G4.txt` | the ledger of `common/g4Ledger.py`: one line per event (status 1 particles with their energy and pT, the primaries, and the dropped particles by reason with their energy), the totals and the fractions handed to Geant4, for the signal and for library events 72 to 139 |
| `step2_G4_signal_09_G4.log`, `step2_G4_pileup_09_G4.log` | the `cmsRun` logs: the Geant4 banner (`geant4-11-02-patch-02 [MT] (21-June-2024)`) and the event records |

`g4Ledger.py` reads `generatorSmeared` and the `SimTrack`s with FWLite: a status 1 particle whose barcode is the
`genpartIndex()` of a `SimTrack` is a primary; the others are classified as `|eta| > 5.5`, `p < 40 MeV`, neutrino,
or "other", in this order, with the eta of the momentum (the Generator applies the cut to the line from the vertex,
hence the eight "other" of the pileup and the one border pi+ of the signal).

## 4. The tables

### 4.1 `ledger_09_G4.txt`

```
== g4Ledger (9a) signal step2_G4_signal_09_G4.root (events 1:1:1)
event     status1    E[GeV]  pT[GeV] | primary    E[GeV]  pT[GeV] | dropped: |eta|>5.5 (E) / p<40MeV (E) / neutrino (E) / other (E)
1:1:1         634   13600.0    528.9 |     571    6923.0    471.7 | 56 (6581.3) / 6 (0.2) / 1 (95.5) / 0 (0.0)
total(1)      634   13600.0    528.9 |     571    6923.0    471.7 | 56 (6581.3) / 6 (0.2) / 1 (95.5) / 0 (0.0)
handed to Geant4: 90.1 % of the stable particles, 50.9 % of their energy, 89.2 % of their pT

== g4Ledger (9b) the 68 in-time pileup collisions step2_G4_pileup_09_G4.root (events 1:1:72-1:1:139)
1:1:72        192   13600.0     56.6 |     164    4558.9     46.7 | 27 (9041.1) / 1 (0.0) / 0 (0.0) / 0 (0.0)     <- the drawn one
1:1:94        547   13600.0    262.7 |     470    5451.6    240.7 | 75 (8148.3) / 2 (0.1) / 0 (0.0) / 0 (0.0)     <- the largest
1:1:95        461   13600.0    203.7 |     386    3518.7    178.9 | 69 (10067.2) / 5 (0.1) / 0 (0.0) / 1 (13.9)
1:1:101       444   13600.0    196.7 |     389    3718.4    173.3 | 49 (9876.9) / 5 (0.1) / 0 (0.0) / 1 (4.6)
1:1:106       382   13600.0    163.1 |     302    2938.3    135.8 | 73 (10634.3) / 6 (0.1) / 0 (0.0) / 1 (27.3)
1:1:124         6   13600.0      1.8 |       0       0.0      0.0 | 6 (13600.0) / 0 (0.0) / 0 (0.0) / 0 (0.0)     <- the smallest
1:1:126       273   13600.0     82.5 |     218    2916.2     66.9 | 52 (10664.1) / 1 (0.0) / 1 (1.3) / 1 (18.4)
1:1:129       249   13600.0     92.4 |     175    2124.1     71.0 | 73 (11451.2) / 0 (0.0) / 0 (0.0) / 1 (24.7)
1:1:135       265   13600.0    102.2 |     226    4141.0     87.6 | 37 (9423.3) / 1 (0.0) / 0 (0.0) / 1 (35.7)
1:1:136        89   13600.0     27.7 |      64     505.7     17.9 | 24 (13066.8) / 0 (0.0) / 0 (0.0) / 1 (27.5)
1:1:137        98   13600.0     29.5 |      65    5742.5     20.5 | 32 (7843.2) / 0 (0.0) / 0 (0.0) / 1 (14.3)
total(68)   12993  924800.0   5314.6 |   10388  218552.6   4485.6 | 2492 (706020.8) / 98 (2.7) / 7 (57.5) / 8 (166.4)
handed to Geant4: 80.0 % of the stable particles, 23.6 % of their energy, 84.4 % of their pT
```

(the 57 other per-collision lines are in the file). Per collision: 12993 / 68 = 191.1 stable particles, 10388 / 68 =
152.8 primaries, 218552.6 / 68 = 3214.0 GeV, 4485.6 / 68 = 66.0 GeV of pT. Signal + 68 collisions: 634 + 12993 = 13627
stable particles, 13600 + 924800 = 938400 GeV, 528.9 + 5314.6 = 5843.5 GeV pT; 571 + 10388 = 10959 primaries,
6923.0 + 218552.6 = 225475.6 GeV, 471.7 + 4485.6 = 4957.3 GeV pT (80.4 %, 24.0 %, 84.8 %); dropped 56 + 2492 = 2548
beyond |eta| 5.5 (6581.3 + 706020.8 = 712602.1 GeV), 6 + 98 = 104 below 40 MeV (0.2 + 2.7 = 2.9 GeV), 1 + 7 = 8 neutrinos
(95.5 + 57.5 = 153.0 GeV), 8 other (166.4 GeV). Signal share of the bunch crossing: 571 / 10959 = 5.2 % of the
primaries, 471.7 / 4957.3 = 9.5 % of the pT.

### 4.2 `simAnalyzer_signal_09_G4.log` (event 1:1:1)

```
== SimAnalyzer run:lumi:event 1:1:1 nGenParticles=1846 nSimTracks=571 nSimVertices=29
  page 1 (primaries): nPrimaries=571 nStatus1=634 noSimTrack=63 (eta 56, p 6, nu 1, other 0) PV=(0.0276598,-0.0169037,3.11193)
     | b system: n=8/9 pt=56.8234 eta=-1.55313 phi=-2.08847 | bbar system: n=16/16 pt=114.174 eta=-3.74697 phi=1.01039 | net |sum pT|=57.4538
  page 2 (calo surface): nReaching=557 nNotReaching=14 | b system at the boundary: n=8/8 pt=56.8234 ... | bbar system at the boundary: n=16/16 pt=114.174 ... | net |sum pT|=57.4538
  page 3 (secondaries): nSecondaries=0 decay=0 EM=0 hadronic=0 other=0
  page 4 (tracker/muon hits): nHits=6734 tracker=6734 muon=0 sumELoss[GeV]=0 noGeometry=0 noSimTrack=0 bHits=179 bbarHits=0
     by subdetector: PXB=908 PXF=829 TIB=960 TID=679 TOB=860 TEC=2498
  page 5 (calo hits): nHits=746 nCells=459 noGeometry=0 E by subdetector: HF=1511(459) | b system: E_ECAL=0 E_HCAL=0 E_HF=0 nCells=0 | bbar system: E_ECAL=0 E_HCAL=0 E_HF=760 nCells=39
```

The page-6 header (`simAnalyzer_signal_09_G4_page-06.png`) adds "38 beyond |eta| = 6 (x)" for the crosses drawn on
the border of the display, and the GenPartAnalyzer part of the log repeats the generator numbers of chapters 01 to 07
(hard b: pt 113.221, eta -1.43851; status 62 b 91.7437 / bbar 116.356; B- 94.2789 at eta -1.56724, phi -2.09222;
B*+ 114.174 at eta -3.74697, phi 1.01039; net b+bbar at hadron level 20.302 GeV).

### 4.3 `simAnalyzer_pileup_09_G4.log` (library event 1:1:72)

```
== SimAnalyzer run:lumi:event 1:1:72 nGenParticles=406 nSimTracks=164 nSimVertices=7
  page 1 (primaries): nPrimaries=164 nStatus1=192 noSimTrack=28 (eta 27, p 1, nu 0, other 0) PV=(0.0286612,-0.0165738,-0.482677)
  page 2 (calo surface): nReaching=155 nNotReaching=9
  page 3 (secondaries): nSecondaries=0
  page 4 (tracker/muon hits): nHits=1965 tracker=1965 muon=0 sumELoss[GeV]=0  by subdetector: PXB=280 PXF=221 TIB=304 TID=211 TOB=279 TEC=670
  page 5 (calo hits): nHits=153 nCells=135 E by subdetector: HF=192(135)
```

### 4.4 FWLite extras (signal event 1:1:1, from `step2_G4_signal_09_G4.root`)

Read with `python3` and `from DataFormats.FWLite import Events, Handle` as in `common/g4Ledger.py` (the HepMC
record `generatorSmeared`, the `SimTrack`s, `SimVertex`s and the tracker `PSimHit` collections of `g4SimHits`).

Stable particles (status 1) and primaries by region and by kind, n / E [GeV] / pT [GeV]:

| | stable | primaries | lost |
|---|---|---|---|
| abs(eta) < 2.5 | 316 / 733.8 / 278.7 | 309 / 638.1 / 241.1 | 6 photons below 40 MeV (0.2 GeV), the neutrino (95.5) |
| 2.5 to 3.0 | 55 / 193.1 / 24.6 | 55 / 193.1 / 24.6 | |
| 3.0 to 5.2 | 189 / 5019.6 / 197.4 | 189 / 5019.6 / 197.4 | |
| 5.2 to 5.5 | 16 / 836.4 / 7.8 | 16 / 836.4 / 7.8 | (13 of the 16 never enter the CALO volume) |
| above 5.5 | 58 / 6817.1 / 20.3 | 2 / 235.9 / 0.7 | 56 dropped by the eta cut (6581.3 GeV) |
| charged hadrons | 303 / 7167.7 / 259.1 | 276 / 3298.6 / 247.8 | 27 beyond 5.5 (3869.2 GeV, 4 protons with 1276.6) |
| neutral hadrons | 56 / 3449.9 / 84.7 | 48 / 1869.8 / 81.1 | 8 beyond 5.5 (1580.1 GeV; the most energetic an antineutron of 998.3 GeV at eta -7.47) |
| photons | 273 / 2875.9 / 142.7 | 246 / 1743.7 / 137.9 | 21 beyond 5.5 (1132.0 GeV), 6 below 40 MeV |
| electrons | 1 / 10.9 / 5.0 | 1 / 10.9 / 5.0 | |
| neutrinos | 1 / 95.5 / 37.5 | 0 | the anti-nu_e of the B- decay, pT 37.46, eta -1.59 |
| all | 634 / 13600.0 / 528.9 | 571 / 6923.0 / 471.7 | 63 |

- The 56 dropped by the eta cut have 5.52 < abs(eta) < 8.04, none beyond 8.2; pT sum 19.6 GeV.
- The 6 photons below 40 MeV: 38.3, 30.5, 36.8, 27.1, 29.5, 33.4 MeV, all from pi0 decays at the primary vertex.
- The 2 primaries beyond abs(eta) = 5.5: a neutron (pdg 2112, E 151.2 GeV, pT 0.018, eta 9.72; the very-forward
  neutral rule) and a pi+ (E 84.6 GeV, pT 0.692, eta -5.50; the geometric eta cut from a vertex at z = +3.1 cm).
- Generator decay vertices among the 29 SimVertices (all with `noParent()`, `processType() = 0`; distance from the
  primary vertex, position): B- (pdg -521, E 235.9, pT 94.3, eta -1.57) at 0.78 cm, (r, z) = (0.32, 2.39) cm,
  daughters anti-nu_e (pT 37.5), e- (5.0), D*0bar (51.9, status 2); B+ (521, E 2388.2, pT 112.6, eta -3.75) at 11.73 cm,
  (0.55, -8.60), daughters D*0bar (48.1), pi+ (30.6), pi0 (5.5), K0 (15.7), K*0bar (12.8); D0bar (-421, E 911.9,
  pT 42.4, eta -3.76) at 25.80 cm, (1.21, -22.66), daughters K0bar (12.9), pi- (4.3), pi+ (7.5), pi0 (9.1), pi0 (8.7).
- Boundary: 557 primaries have `crossedBoundary()`; their `trackerSurfaceMomentum()` equals their production
  momentum. The 14 without: pi- (pT 0.432, eta -5.50), pi- (0.299, -5.38), neutron (0.018, 9.72), anti-Sigma-
  (0.523, 5.46), pi+ (0.692, -5.50), K0S (0.355, 5.48), K0L (0.884, 5.35), K0L (0.315, 5.43), pi- (0.155, -5.40),
  photons (0.281, 5.45), (0.247, 5.44), (0.749, 5.47), (0.178, 5.34), (0.050, -5.37).
- Tracker hits: 6734, all in the `LowTof` collections (0 `HighTof`), time of flight 0.21 to 13.95 ns, `energyLoss()`
  sum 0; from 375 distinct primaries (193 charged, 182 neutral; abs(eta) from 0.01 to 3.11, pT down to 7 MeV), at most
  32 hits for one track; by kind of the primary: charged hadrons 3433, photons 2907, neutral hadrons 376,
  electrons 18. Charged primaries with abs(eta) < 2.5: 159.
- `LHCTransport` HepMC product of the signal event: 0 particles.

### 4.5 FWLite extras (library event 72 and the 68 in-time collisions, from `step2_G4_pileup_09_G4.root`)

Library event 72: 192 stable, 164 primaries, 7 SimVertices; dropped: 27 beyond 5.5 (9041.1 GeV: 14 charged hadrons
with 4873.6 GeV, 4 neutral hadrons with 3922.8, 9 photons with 244.7; abs(eta) 5.52 to 9.85; 3 beyond 8.2 with 5788.4 GeV,
the most energetic a Lambda of 3413.7 GeV at eta 9.85; no proton), 1 photon below 40 MeV; 4 primaries beyond abs(eta) 5.5
with 3749.2 GeV (the very-forward neutral rule). By region (stable -> primaries): abs(eta) < 2.5: 92 -> 92 (66.4 GeV,
25.7 pT); 2.5 to 3.0: 18 -> 17 (34.8 GeV); 3.0 to 5.2: 43 -> 43 (493.5 GeV); 5.2 to 5.5: 8 -> 8 (215.0 GeV); above 5.5:
31 -> 4. By kind: charged hadrons 84 -> 70, neutral hadrons 15 -> 11, photons 93 -> 83.

The 68 collisions (library events 72 to 139), n / E [GeV] / pT [GeV]:

| | stable | primaries |
|---|---|---|
| abs(eta) < 2.5 | 4864 / 5519.3 / 2179.4 | 4765 / 5505.6 / 2173.6 |
| 2.5 to 3.0 | 1037 / 3597.0 / 449.2 | 1035 / 3597.0 / 449.2 |
| 3.0 to 5.2 | 3996 / 58413.2 / 1674.3 | 3992 / 58366.7 / 1672.7 |
| 5.2 to 5.5 | 436 / 17054.0 / 160.3 | 428 / 16887.6 / 158.9 |
| above 5.5 | 2660 / 840216.5 / 851.4 | 168 / 134195.7 / 31.1 |
| charged hadrons | 5911 / 572734.9 / 3087.8 | 4677 / 55663.6 / 2596.3 |
| neutral hadrons | 1088 / 179327.8 / 749.7 | 837 / 80592.4 / 627.3 |
| photons | 5900 / 171675.6 / 1458.8 | 4808 / 82175.1 / 1251.9 |
| electrons | 85 / 998.9 / 10.9 | 64 / 116.1 / 8.4 |
| muons | 2 / 5.3 / 1.7 | 2 / 5.3 / 1.7 |
| neutrinos | 7 / 57.5 / 5.6 | 0 |
| all | 12993 / 924800.0 / 5314.6 | 10388 / 218552.6 / 4485.6 |

Dropped by cut and kind: eta cut 2492 = 1228 charged hadrons (516966.3 GeV, of which 132 protons with 226157.3 GeV),
251 neutral hadrons (98735.3), 994 photons (89436.6), 19 electrons (882.7); abs(eta) from 5.50 to 11.35, the most
energetic a beam proton of 6800.0 GeV at abs(eta) 10.57; 211 of them beyond abs(eta) = 8.2 (364865.4 GeV). Momentum
cut 98 = 94 photons (2.4 GeV), 2 charged hadrons (0.3), 2 electrons (0.1). Neutrinos 7 (57.5 GeV). Other 8 (166.4 GeV):
photons in events 95 (E 13.9, eta 5.50, vertex z = 3.49 cm), 101 (4.6, -5.49, -2.87 cm), 126 (18.4, -5.49, -1.06 cm),
129 (24.7, -5.49, -6.03 cm) and charged hadrons in events 106 (pi+ 27.3, 5.48, 2.88 cm), 135 (K+ 35.7, -5.48,
-4.16 cm), 136 (pi+ 27.5, 5.49, 4.76 cm), 137 (pi- 14.3, 5.49, 5.01 cm): particles at the border of the eta cut whose
line from a displaced vertex is still inside r = 2.9 cm at |z| = 354.8 cm. Primaries per collision: from 0 (event 124,
6 stable particles, all beyond 5.5; its 2 SimTracks with a generator link are status-2 particles handed over with a
predefined decay, which the ledger does not count) to 470 (event 94). Per collision in the tracker region:
4765 / 68 = 70.1 primaries, 2173.6 / 68 = 32.0 GeV pT; pileup pT per unit of eta-phi area in abs(eta) < 2.5:
2173.6 / (5 x 2 pi) = 69.2 GeV (about 200 GeV for 200 collisions).

### 4.6 Numbers from the release quoted in "Open problems for Phase-2"

All under `/cvmfs/cms.cern.ch/el9_amd64_gcc12/cms/cmssw/CMSSW_15_0_5/src`:

- `SimG4Core/Application/python/g4SimHits_cfi.py`: `Generator`: `MinPCut = 0.04`, `MaxPCut = 99999.0`,
  `MinEtaCut = -5.5`, `MaxEtaCut = 5.5`, `RDecLenCut = 2.9`, `LDecLenCut = 30.0`, `PDGfilter = (21,1,2,3,4,5,6)` with
  `PDGfilterSel = False`; `HectorEtaCut` (`hectorParameter_cfi.py`): `EtaCutForHector = 8.2`; `StackingAction`:
  `TrackNeutrino = False`, `KillGamma = True`, `GammaThreshold = 0.0001`, `RusRoGammaEnergyLimit = 5.0` MeV,
  `RusRoEcalGamma = RusRoHcalGamma = RusRoMuonIronGamma = RusRoPreShowerGamma = RusRoCastorGamma = RusRoZDCGamma =
  RusRoWorldGamma = 0.3`, `RusRoHGcalGamma = 1.3`, `RusRoNeutronEnergyLimit = 10.0` MeV, the neutron probabilities 0.1
  and `RusRoHGcalNeutron = 1.1`, `RusRoProtonEnergyLimit = 0.0`; `common_heavy_suppression`: `NeutronThreshold =
  ProtonThreshold = IonThreshold = 30.0` (MeV), used with `KillHeavy = False`; `common_maximum_time`:
  `MaxTrackTime = 500.0`, `MaxTrackTimeForward = 2000.0`, `MaxZCentralCMS = 50.0`, `DeadRegions = QuadRegion,
  InterimRegion`, `CriticalEnergyForVacuum = 2.0`, modified only by `h2tb` (`MaxTrackTime = 1000.0`); `Physics.type =
  SimG4Core/Physics/FTFP_BERT_EMM`, `G4TransportWithMSC = 2`, `G4GammaGeneralProcess = True`, `G4HepEmActive =
  False`, `EminFTFP = 3`, `EmaxBERT = 6`, `EmaxBERTpi = 12`, `EminQGSP = 12`, `EmaxFTFP = 25`; `SteppingAction`:
  `CMStoZDCtransport = False`, `BTLName = BarrelTimingLayer`; `TrackerSD`: `ZeroEnergyLoss = False`,
  `ElectronicSigmaInNanoSeconds = 12.06`, `EnergyThresholdForPersistencyInGeV = 0.2`, `EnergyThresholdForHistoryInGeV
  = 0.05`; `CaloSD`: `EminHits = (0.015, 0.010, 0.0, 0.0, 0.0)`, `TmaxHits = (500, 500, 500, 500, 2000)`; `HFShower`:
  `UseShowerLibrary = True`, `PEPerGeV = 0.31`; `MtdSD`: `TimeSliceUnit = 0.01`, `EminHit = 0.0`,
  `EnergyThresholdForPersistencyInGeV = 1e9`, `EnergyThresholdForHistoryInGeV = 1e9` ("keep temporarily old
  behaviour"); `HGCSD`: `TimeSliceUnit = 0.001`, `EminHit = 0.0`; era modifiers in the file: `run2_common`,
  `run2_HCAL_2017`, `run3_common` (`LHCTransport = True`, the Run-3 `OnlySDs` / `TrackHits` / `CaloHits`),
  `pp_on_PbPb_run3 | run3_upc`, `phase2_timing` (`ECalSD.StoreLayerTimeSim = True`, `TimeSliceUnit = 0.001`),
  `ecal_component`, `h2tb`, `dd4hep`, `phase2_common` (`OnlySDs` with `HGCalSensitiveDetector`,
  `HGCScintillatorSensitiveDetector`, `HFNoseSensitiveDetector`, `MtdSensitiveDetector`, `ZdcSensitiveDetector`,
  `BCM1FSensitiveDetector`, `PLTSensitiveDetector`, ...; `TrackHits` with `FastTimerHitsBarrel`, `FastTimerHitsEndcap`;
  `CaloHits` with `HGCHitsEE`, `HGCHitsHEfront`, `HGCHitsHEback`, `CalibrationHGCHits*`, `HFNoseHits`;
  `LHCTransport = False`; `MuonSD.HaveDemoChambers = False`), `hgcaltb`, `phase2_hgcalOnly`, `phase2_hgcalV18`
  (`HGCSD.HitCollection = 2`), `fixLongLivedSleptonSim`; `phase2_hgcal` on `common_MCtruth`
  (`SaveCaloBoundaryInformation = True`); `fineCalo` on `common_MCtruth`.
- `Configuration/StandardSequences/python/SimWithCastor_cff.py`: `MinEtaCut = -7.0`, `MaxEtaCut = 5.5`;
  `SimG4Core/Application/python/NeutronBGforMuons_cff.py`: `MinEtaCut = -7.0`, `MaxEtaCut = 7.0`, `TrackNeutrino` left,
  `Physics.type = FTFP_BERT_XS_EML`; `NeutronBGforMuonsHP_cff.py`: `FTFP_BERT_HP_EML`; `cpuBenchmark_cff.py`:
  `FTFP_BERT`, `TrackNeutrino = True`, roulette limits 0.
- `SimG4Core/Generators/src/Generator.cc`: lines 51 to 56 (`theRDecLenCut`, `theDecLenCut` local), 82 to 87
  (`Z_lmax = R * (1 - exp(-2 eta)) / (2 exp(-eta)) = R sinh(eta)`, `Z_hector`), 90 (the only use of `theDecLenCut`:
  a printout), 305 to 313 (the very-forward n / nbar / gamma rule), 316 to 365 (the p and geometric eta cuts), 369
  (status 2 with `r >= RDecLenCut`), 394 (`setGenId`).
- `SimG4Core/Application/src/StackingAction.cc`: line 189 (primary neutrinos killed), 204 (secondary neutrinos),
  341 (`if (prob < 1.0 && aTrack->GetKineticEnergy() < elim)`), 399 (`regionHGcal` set only if `gRusRoHGcal < 1.0 ||
  nRusRoHGcal < 1.0`).
- `SimG4Core/Application/src/RunManagerMT.cc`, `runForPhase2()` (lines 348 to 356): `m_isPhase2 = true` if a region
  `HGCalRegion`, `FastTimerRegionETL` or `FastTimerRegionBTL` exists; `RunManagerMTWorker.cc` line 454:
  `Phase2SteppingAction` instead of `SteppingAction`.
- `SimG4Core/PhysicsLists/plugins/FTFPCMS_BERT_EMM.cc`: `CMSEmStandardPhysics`, `G4EmExtraPhysics`, `G4DecayPhysics`,
  `G4HadronElasticPhysics`, `CMSHadronPhysicsFTFP_BERT(minFTFP, maxBERT, maxBERTpi, ...)`, `G4StoppingPhysics`;
  the variants `FTFPCMS_BERT_EMMT`, `FTFPCMS_BERT_EMM_TRK`, `FTFPCMS_BERT_HP_EMM`, `FTFPCMS_INCLXX_EMM`,
  `QGSPCMS_FTFP_BERT_EMM` live next to it.
- `SimG4CMS/Tracker/src/TkAccumulatingSensitiveDetector.cc`: line 64 (`theTofLimit = ElectronicSigmaInNanoSeconds * 3`),
  108 (`ZeroEnergyLoss`), 192 (`LowTof` / `HighTof`). `SimG4CMS/Calo/src/HCalSD.cc`: `getFromHFLibrary` (lines 737 to
  780, each library hit booked with `1. * CLHEP::GeV`). `SimG4CMS/Calo/src/CaloTrkProcessing.cc`: lines 287 to 303
  (`setIDonCaloSurface`, `setCrossedBoundary`).
- `Geometry/HcalSimData/python/HFParameters_cff.py`: `HFLibraryFileBlock.FileName`, with `run3_HFSL` ->
  `SimG4CMS/Calo/data/HFShowerLibrary_run3_v7.root`, `FileVersion = 3`, `EqualizeTimeShift = True`.
- `Configuration/Generator/python/Pythia8CommonSettings_cfi.py`: `ParticleDecays:limitTau0 = on`,
  `ParticleDecays:tau0Max = 10` (mm).
- `L1Trigger/Phase2L1ParticleFlow/python/l1ctLayer1_cff.py`: `etaBoundaries = (-1.5, 1.5)`, `(-3.0, -1.5)`,
  `(+1.5, +3.0)`, `(-5.5, -3.0)`, `(+3.0, +5.5)`.
- `config/toolbox/el9_amd64_gcc12/tools/selected/geant4.xml`: `geant4` version 11.2.2; the log banner:
  `Geant4 version Name: geant4-11-02-patch-02 [MT] (21-June-2024)`.
- Not established from the release: the radius of the Phase-2 beam pipe and of the first inner-tracker layer (only in
  the XML of `Geometry/TrackerCommonData/data/PhaseII/`), whether the Phase-2 forward geometry closes the gap between
  the calorimeter envelope (about abs(eta) 5.3 in this event) and the 5.5 cut, and since when the `RusRoHGcal*`
  values are what they are (no history on cvmfs).

### 4.7 The 2024 and the 2025 settings

The chapter runs with `YEAR=2025` (`common/jitjet_common.sh`): `auto:phase1_2025_realistic` =
`150X_mcRun3_2025_realistic_v1` (`autoCond.py`, line 107), era `Run3_2025`. The records a SIM job reads are the same
tags in the 2024 and the 2025 global tags (`conddb list 150X_mcRun3_2024_realistic_v2`, `conddb list
150X_mcRun3_2025_realistic_v1`):

```
GeometryFileRcd     Extended   141DD4hepV2_Extended2024      (and the FlatMinus/Plus, ZeroMaterial variants, also ...Extended2024...)
MFGeometryFileRcd   160812     MFGeometry_160812
MagFieldConfigRcd   3.8T       MFConfig_160812_Run2_3_8T
SimBeamSpotObjectsRcd          SimBeamSpot_Realistic25ns13p6TeVEarly2024CollisionVtxSmearingParameters_v1_mc
```

The 35 records that differ (the list is in the chapter 08 companion, section 4.9) are calibrations, alignment and
trigger tables of the digitisation, the reconstruction and the Level-1 trigger. `Era_Run3_2025_cff.py` =
`Run3_2024` + `run3_GEM_2025`, `run3_CSC_2025`, `run3_SiPixel_2025`, `stage2L1Trigger_2025`, `ecal_cctiming`; none of
them occurs in `SimG4Core/Application/python/g4SimHits_cfi.py` (`grep`), the `g4SimHits` configuration is the same.

The rerun with the 2025 settings (12 November 2026) against the 2024 run (7 November 2026): `ledger_09_G4.txt`,
`simAnalyzer_signal_09_G4.log`, `simAnalyzer_pileup_09_G4.log` and the two `cmsRun` logs `step2_G4_signal_09_G4.log`,
`step2_G4_pileup_09_G4.log` are identical apart from time stamps, timings and the era / global-tag lines. The two ROOT
files were also compared product by product with FWLite (per event and per collection: the number of `SimTrack`s,
`SimVertex`s, `PSimHit`s and `PCaloHit`s, and the sums of their energies, times, positions and track ids): signal 31
collections, 571 `SimTrack`s, 7480 hits; library 250 events, 7750 collections, 38 862 `SimTrack`s, 425 436 hits; 0
differences. Every number of the chapter holds for both years. Run times of the rerun: signal 53 s, library 88 s, the
two SimAnalyzer jobs 11 s and 7 s.

One thing does move with the year, the drawing: `SimAnalyzer` places a tracker hit (stored in the local frame of its
module) with the reconstruction geometry of `GeometryRecoDB_cff`, whose `trackerGeometryDB` has `applyAlignment =
True`, i.e. the `TrackerAlignmentRcd` of the global tag: `TrackerAlignment_Run3MC_2023_rereco_v1` (2024 tag) and
`TrackerAlignment_Run3MC_2025_EOY_v1` (2025 tag). From `conddb dump` of the two payloads (20 292 modules each), the
module positions differ by 10 um in the median (90 % below 0.49 mm; medians PXB 2 um, PXF 6 um, TIB 4 um, TID 51 um,
TOB 7 um, TEC 0.33 mm, 7484 of 7552 TEC modules moving by more than 0.1 mm), four modules by more than 1 mm (largest
6.8 mm, one TIB module). The muon and calorimeter alignment records are the same in both tags. The pages of the
chapter therefore show the same hits shifted by these amounts, which no plot resolves; the log prints no tracker
position and is identical.
