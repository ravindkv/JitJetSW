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
   = 2.9 cm * 122.34 = 354.8 cm`, and the particle is dropped if it is still inside `r < RDecLenCut = 2.9 cm` there),
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
   (`EminHits = 0.015, 0.010, 0, 0, 0` MeV, i.e. 15 and 10 keV, for EB, EE, ES, HCAL, ZDC: `SimG4CMS/Calo/src/CaloSD.cc` multiplies them by `CLHEP::MeV`; `TmaxHits = 500 ns`, ZDC 2000), `HCalSD` and
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
python3 ../g4Extras_09_G4.py step2_G4_signal_09_G4.root step2_G4_pileup_09_G4.root 1:1:72 > extras_09_G4.txt 2> extras_09_G4.log
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
| `ledger_09_G4.txt` | the ledger of `common/g4Ledger.py`: one line per event (status 1 particles with their energy and pT, the primaries, the dropped particles by reason with their energy, and the forward protons the PPS transport hands to Geant4 at the Roman pots), the totals, the stable particles, primaries and PPS protons by region and by kind, and the fractions handed to Geant4, for the signal and for library events 72 to 139 |
| `extras_09_G4.txt` | the extra numbers of `test/09_G4/g4Extras_09_G4.py` (FWLite, a few seconds): where the primaries cross into a calorimeter volume, by surface (signal, its b and bbar systems, library event 72); the eta cut as a line for the signal particles near abs(eta) = 5.5; the HF shower-library photoelectrons per GeV; the tracker crossings per primary by abs(eta) (section 4.8) |
| `step2_G4_signal_09_G4.log`, `step2_G4_pileup_09_G4.log` | the `cmsRun` logs: the Geant4 banner (`geant4-11-02-patch-02 [MT] (21-June-2024)`) and the event records |

`g4Ledger.py` reads `generatorSmeared`, the `SimTrack`s and the `SimVertex`s with FWLite: a status 1 particle whose
barcode is the `genpartIndex()` of a `SimTrack` that starts at the collision is a primary; a forward proton that the
PPS transport hands to Geant4 at the Roman pots (a `SimTrack` with its vertex at |z| > 100 m) is matched by energy
and counted in the column PPS; the others are classified as `|eta| > 5.5`, `p < 40 MeV`, neutrino,
or "other", in this order, with the eta of the momentum (the Generator applies the cut to the line from the vertex,
hence the eight "other" of the pileup and the one border pi+ of the signal).

## 4. The tables

### 4.1 `ledger_09_G4.txt`

```
== g4Ledger (9a) signal step2_G4_signal_09_G4.root (events 1:1:1)
event     status1    E[GeV]  pT[GeV] | primary    E[GeV]  pT[GeV] | dropped: |eta|>5.5 (E) / p<40MeV (E) / neutrino (E) / other (E) | PPS (E)
1:1:1         634   13600.0    528.9 |     571    6923.0    471.7 | 56 (6581.3) / 6 (0.2) / 1 (95.5) / 0 (0.0) | 0 (0.0)
total(1)      634   13600.0    528.9 |     571    6923.0    471.7 | 56 (6581.3) / 6 (0.2) / 1 (95.5) / 0 (0.0) | 0 (0.0)
by region and by kind, n / E[GeV] / pT[GeV]: stable                           | primaries (from the vertex)      | PPS (at the Roman pots)
  abs(eta) < 2.5   316 / 733.8 / 278.7                | 309 / 638.1 / 241.1              | 0 / 0.0 / 0.0
  2.5 to 3.0       55 / 193.1 / 24.6                  | 55 / 193.1 / 24.6                | 0 / 0.0 / 0.0
  3.0 to 5.2       189 / 5019.6 / 197.4               | 189 / 5019.6 / 197.4             | 0 / 0.0 / 0.0
  5.2 to 5.5       16 / 836.4 / 7.8                   | 16 / 836.4 / 7.8                 | 0 / 0.0 / 0.0
  above 5.5        58 / 6817.1 / 20.3                 | 2 / 235.9 / 0.7                  | 0 / 0.0 / 0.0
  charged hadrons  303 / 7167.7 / 259.1               | 276 / 3298.6 / 247.8             | 0 / 0.0 / 0.0
  neutral hadrons  56 / 3449.9 / 84.7                 | 48 / 1869.8 / 81.1               | 0 / 0.0 / 0.0
  photons          273 / 2875.9 / 142.7               | 246 / 1743.7 / 137.9             | 0 / 0.0 / 0.0
  electrons        1 / 10.9 / 5.0                     | 1 / 10.9 / 5.0                   | 0 / 0.0 / 0.0
  neutrinos        1 / 95.5 / 37.5                    | 0 / 0.0 / 0.0                    | 0 / 0.0 / 0.0
handed to Geant4: 90.1 % of the stable particles, 50.9 % of their energy, 89.2 % of their pT

== g4Ledger (9b) the 68 in-time pileup collisions step2_G4_pileup_09_G4.root (events 1:1:72-1:1:139)
1:1:72        192   13600.0     56.6 |     164    4558.9     46.7 | 27 (9041.1) / 1 (0.0) / 0 (0.0) / 0 (0.0) | 0 (0.0)     <- the drawn one
1:1:94        547   13600.0    262.7 |     470    5451.6    240.7 | 75 (8148.3) / 2 (0.1) / 0 (0.0) / 0 (0.0) | 0 (0.0)     <- the largest
1:1:95        461   13600.0    203.7 |     386    3518.7    178.9 | 69 (10067.2) / 5 (0.1) / 0 (0.0) / 1 (13.9) | 0 (0.0)
1:1:101       444   13600.0    196.7 |     389    3718.4    173.3 | 49 (9876.9) / 5 (0.1) / 0 (0.0) / 1 (4.6) | 0 (0.0)
1:1:106       382   13600.0    163.1 |     302    2938.3    135.8 | 73 (10634.3) / 6 (0.1) / 0 (0.0) / 1 (27.3) | 0 (0.0)
1:1:124         6   13600.0      1.8 |       0       0.0      0.0 | 4 (1260.7) / 0 (0.0) / 0 (0.0) / 0 (0.0) | 2 (12339.3)  <- the smallest
1:1:126       273   13600.0     82.5 |     218    2916.2     66.9 | 52 (10664.1) / 1 (0.0) / 1 (1.3) / 1 (18.4) | 0 (0.0)
1:1:129       249   13600.0     92.4 |     175    2124.1     71.0 | 73 (11451.2) / 0 (0.0) / 0 (0.0) / 1 (24.7) | 0 (0.0)
1:1:132         3   13600.0      0.5 |       1    6326.6      0.1 | 1 (473.4) / 0 (0.0) / 0 (0.0) / 0 (0.0) | 1 (6800.0)
1:1:135       265   13600.0    102.2 |     226    4141.0     87.6 | 37 (9423.3) / 1 (0.0) / 0 (0.0) / 1 (35.7) | 0 (0.0)
1:1:136        89   13600.0     27.7 |      64     505.7     17.9 | 24 (13066.8) / 0 (0.0) / 0 (0.0) / 1 (27.5) | 0 (0.0)
1:1:137        98   13600.0     29.5 |      65    5742.5     20.5 | 32 (7843.2) / 0 (0.0) / 0 (0.0) / 1 (14.3) | 0 (0.0)
total(68)   12993  924800.0   5314.6 |   10387  211752.6   4485.5 | 2466 (562762.7) / 98 (2.7) / 7 (57.5) / 8 (166.4) | 27 (150058.2)
PPS: 27 forward protons with 150058.2 GeV handed to Geant4 at the Roman pots (largest energy difference of a match 2.0e-13)
by region and by kind, n / E[GeV] / pT[GeV]: stable                           | primaries (from the vertex)      | PPS (at the Roman pots)
  abs(eta) < 2.5   4864 / 5519.3 / 2179.4             | 4765 / 5505.6 / 2173.6           | 0 / 0.0 / 0.0
  2.5 to 3.0       1037 / 3597.0 / 449.2              | 1035 / 3597.0 / 449.2            | 0 / 0.0 / 0.0
  3.0 to 5.2       3996 / 58413.2 / 1674.3            | 3992 / 58366.7 / 1672.7          | 0 / 0.0 / 0.0
  5.2 to 5.5       436 / 17054.0 / 160.3              | 428 / 16887.6 / 158.9            | 0 / 0.0 / 0.0
  above 5.5        2660 / 840216.5 / 851.4            | 167 / 127395.7 / 31.0            | 27 / 150058.2 / 11.2
  charged hadrons  5911 / 572734.9 / 3087.8           | 4676 / 48863.6 / 2596.2          | 27 / 150058.2 / 11.2
  neutral hadrons  1088 / 179327.8 / 749.7            | 837 / 80592.4 / 627.3            | 0 / 0.0 / 0.0
  photons          5900 / 171675.6 / 1458.8           | 4808 / 82175.1 / 1251.9          | 0 / 0.0 / 0.0
  electrons        85 / 998.9 / 10.9                  | 64 / 116.1 / 8.4                 | 0 / 0.0 / 0.0
  muons            2 / 5.3 / 1.7                      | 2 / 5.3 / 1.7                    | 0 / 0.0 / 0.0
  neutrinos        7 / 57.5 / 5.6                     | 0 / 0.0 / 0.0                    | 0 / 0.0 / 0.0
handed to Geant4: 79.9 % of the stable particles, 22.9 % of their energy, 84.4 % of their pT
```

(the 56 other per-collision lines are in the file). Per collision: 12993 / 68 = 191.1 stable particles, 10387 / 68 =
152.8 primaries, 211752.6 / 68 = 3114.0 GeV, 4485.5 / 68 = 66.0 GeV of pT. Signal + 68 collisions: 634 + 12993 = 13627
stable particles, 13600 + 924800 = 938400 GeV, 528.9 + 5314.6 = 5843.5 GeV pT; 571 + 10387 = 10958 primaries,
6923.0 + 211752.6 = 218675.6 GeV, 471.7 + 4485.5 = 4957.2 GeV pT (80.4 %, 23.3 %, 84.8 %), and the 27 PPS protons
(150058.2 GeV); dropped 56 + 2466 = 2522 beyond |eta| 5.5 (6581.3 + 562762.7 = 569344.0 GeV), 6 + 98 = 104 below
40 MeV (0.2 + 2.7 = 2.9 GeV), 1 + 7 = 8 neutrinos (95.5 + 57.5 = 153.0 GeV), 8 other (166.4 GeV). Signal share of the
bunch crossing: 571 / 10958 = 5.2 % of the primaries, 471.7 / 4957.2 = 9.5 % of the pT.

The PPS column (since 13 November 2026): the forward protons that `LHCTransport` carries along the beam line and hands
to Geant4 at the Roman pots (|z| = 203 m). Their `SimTrack`s carry the index of the transported HepMC record, not the
barcode of the generator proton (in event 124 the two point to a status-24 quark and a status-63 diquark of
`generatorSmeared`), so `g4Ledger.py` recognises them by their vertex (|z| > 100 m) and matches each to the stable
generator proton of the same side with the nearest energy (the 27 matches agree to 2e-13). Before, the ledger
counted 26 of them (143258.2 GeV) among the dropped beyond abs(eta) 5.5 and the one of event 132, whose index
happens to equal its barcode (3), as a primary: 10388 primaries with 218552.6 GeV, 2492 dropped beyond 5.5 with
706020.8 GeV.

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

The page-6 header (`simAnalyzer_signal_09_G4_page-06.png`) adds "38 beyond |eta| = 6 (x)" for the markers drawn on
the border of the display (`SimAnalyzer::Page::clamp` counts every marker beyond `etaMax`): 37 grey crosses (all 37
are among the 56 dropped by the eta cut, FWLite) and the forward neutron primary at eta 9.72, and the GenPartAnalyzer part of the log repeats the generator numbers of chapters 01 to 07
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
| 5.2 to 5.5 | 16 / 836.4 / 7.8 | 16 / 836.4 / 7.8 | (12 of the 16 never enter a calorimeter volume; the 4 that do: pi- eta +5.25, neutron +5.30, pi+ -5.32, photon -5.32) |
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
  daughters anti-nu_e (pT 37.5), e- (5.0), D_0*(2300)0 (pdg 10421, 51.9, status 2; it decays to a D0 of 84.7 GeV,
  pT 34.0, which decays weakly 0.10 mm further, at 0.79 cm from the primary vertex, (0.32, 2.39), to K*- (19.0) and
  rho+ (15.1): the fourth weak decay of the two b systems, not in the table of the chapter); B+ (521, E 2388.2, pT 112.6, eta -3.75) at 11.73 cm,
  (0.55, -8.60), daughters D*0bar (48.1), pi+ (30.6), pi0 (5.5), K0 (15.7), K*0bar (12.8); D0bar (-421, E 911.9,
  pT 42.4, eta -3.76) at 25.80 cm, (1.21, -22.66), daughters K0bar (12.9), pi- (4.3), pi+ (7.5), pi0 (9.1), pi0 (8.7).
- Boundary: 557 primaries have `crossedBoundary()`; their `trackerSurfaceMomentum()` equals their production
  momentum. The 14 without: pi- (pT 0.432, eta -5.50), pi- (0.299, -5.38), neutron (0.018, 9.72), anti-Sigma-
  (0.523, 5.46), pi+ (0.692, -5.50), K0S (0.355, 5.48), K0L (0.884, 5.35), K0L (0.315, 5.43), pi- (0.155, -5.40),
  photons (0.281, 5.45), (0.247, 5.44), (0.749, 5.47), (0.178, 5.34), (0.050, -5.37).
- Tracker hits: 6734, all in the `LowTof` collections (0 `HighTof`), time of flight 0.21 to 13.95 ns, `energyLoss()`
  sum 0; from 375 distinct primaries (193 charged, 182 neutral; abs(eta) from 0.01 to 3.11, pT down to 7 MeV), at most
  32 hits for one track (two tracks have 32: a photon at eta -1.96, the one `extras_09_G4.txt` prints, and a pi- at
  eta +1.83, the maximum of the charged primaries with 1 < abs(eta) < 2); by kind of the primary: charged hadrons 3433, photons 2907, neutral hadrons 376,
  electrons 18. Charged primaries with abs(eta) < 2.5: 159.
- `LHCTransport` HepMC product of the signal event: 0 particles.

### 4.5 FWLite extras (library event 72 and the 68 in-time collisions, from `step2_G4_pileup_09_G4.root`)

Library event 72: 192 stable, 164 primaries, 7 SimVertices; dropped: 27 beyond 5.5 (9041.1 GeV: 14 charged hadrons
with 4873.6 GeV, 4 neutral hadrons with 3922.8, 9 photons with 244.7; abs(eta) 5.52 to 9.85; 3 beyond 8.2 with 5788.4 GeV,
the most energetic a Lambda of 3413.7 GeV at eta 9.85; no proton), 1 photon below 40 MeV; 4 primaries beyond abs(eta) 5.5
with 3749.2 GeV (the very-forward neutral rule). By region (stable -> primaries): abs(eta) < 2.5: 92 -> 92 (66.4 GeV,
25.7 pT); 2.5 to 3.0: 18 -> 17 (34.8 GeV); 3.0 to 5.2: 43 -> 43 (493.5 GeV); 5.2 to 5.5: 8 -> 8 (215.0 GeV); above 5.5:
31 -> 4. By kind: charged hadrons 84 -> 70, neutral hadrons 15 -> 11, photons 93 -> 83.

The 68 collisions (library events 72 to 139), n / E [GeV] / pT [GeV] (the region and kind rows of `ledger_09_G4.txt`,
4.1; the same as an independent FWLite read):

| | stable | primaries (from the vertex) | PPS (at the Roman pots) |
|---|---|---|---|
| abs(eta) < 2.5 | 4864 / 5519.3 / 2179.4 | 4765 / 5505.6 / 2173.6 | |
| 2.5 to 3.0 | 1037 / 3597.0 / 449.2 | 1035 / 3597.0 / 449.2 | |
| 3.0 to 5.2 | 3996 / 58413.2 / 1674.3 | 3992 / 58366.7 / 1672.7 | |
| 5.2 to 5.5 | 436 / 17054.0 / 160.3 | 428 / 16887.6 / 158.9 | |
| above 5.5 | 2660 / 840216.5 / 851.4 | 167 / 127395.7 / 31.0 | 27 / 150058.2 / 11.2 |
| charged hadrons | 5911 / 572734.9 / 3087.8 | 4676 / 48863.6 / 2596.2 | 27 / 150058.2 / 11.2 (protons) |
| neutral hadrons | 1088 / 179327.8 / 749.7 | 837 / 80592.4 / 627.3 | |
| photons | 5900 / 171675.6 / 1458.8 | 4808 / 82175.1 / 1251.9 | |
| electrons | 85 / 998.9 / 10.9 | 64 / 116.1 / 8.4 | |
| muons | 2 / 5.3 / 1.7 | 2 / 5.3 / 1.7 | |
| neutrinos | 7 / 57.5 / 5.6 | 0 | |
| all | 12993 / 924800.0 / 5314.6 | 10387 / 211752.6 / 4485.5 | 27 / 150058.2 / 11.2 |

Dropped by cut and kind (FWLite, the logic of `g4Ledger.py`): eta cut 2466 (562762.7 GeV, pT 809.2 GeV = 851.4 - 31.0
- 11.2 of the table) = 1202 charged hadrons (373708.1 GeV, of which 106 protons with 82899.1 GeV), 251 neutral hadrons
(98735.3), 994 photons (89436.6), 19 electrons (882.7); abs(eta) from 5.50 to 11.35, the most energetic a pi+ of
4212.1 GeV at abs(eta) 9.98 (event 115); 185 of them beyond abs(eta) = 8.2 (221607.2 GeV). Momentum
cut 98 = 94 photons (2.4 GeV), 2 charged hadrons (0.3), 2 electrons (0.1). Neutrinos 7 (57.5 GeV). Other 8 (166.4 GeV):
photons in events 95 (E 13.9, eta 5.50, vertex z = 3.49 cm), 101 (4.6, -5.49, -2.87 cm), 126 (18.4, -5.49, -1.06 cm),
129 (24.7, -5.49, -6.03 cm) and charged hadrons in events 106 (pi+ 27.3, 5.48, 2.88 cm), 135 (K+ 35.7, -5.48,
-4.16 cm), 136 (pi+ 27.5, 5.49, 4.76 cm), 137 (pi- 14.3, 5.49, 5.01 cm): particles at the border of the eta cut whose
line from a displaced vertex is still inside r = 2.9 cm at |z| = 354.8 cm. The 167 primaries beyond abs(eta) 5.5
(127395.7 GeV): 160 neutrons, antineutrons and photons whose line is inside r = 2.9 cm at |z| = 52.8 m (the
very-forward rule, 127268.7 GeV: 125 photons, 29 neutrons, 6 antineutrons; abs(eta) from 8.19) and 7 border particles
at abs(eta) 5.50 to 5.52 whose line from a displaced vertex is outside 2.9 cm at 354.8 cm (126.9 GeV: events 73, 92,
107, 126, 129 (two), 134). PPS: the transport of `LHCTransport` (switched on by `run3_common`) hands 27 protons to
Geant4 at |z| = 203 m in the 68 collisions (150058.2 GeV, pT 11.2 GeV; generator abs(eta) 8.48, event 84, to 11.55,
event 132). Primaries per collision: from 0 (event 124, 6 stable particles, all beyond 5.5: 4 dropped, 1260.7 GeV,
and its two protons, 12339.3 GeV, handed over at the Roman pots at z = -203 m and +203 m) to 470 (event 94).
Per collision in the tracker region:
4765 / 68 = 70.1 primaries, 2173.6 / 68 = 32.0 GeV pT; pileup pT per unit of eta-phi area in abs(eta) < 2.5:
2173.6 / (5 x 2 pi) = 69.2 GeV (about 200 GeV for 200 collisions).

### 4.6 Numbers from the release quoted in "Open problems for Phase-2"

The state of the release CMSSW_15_0_5 itself (not of the CMSSW development branch), all under
`$CMSSW_RELEASE_BASE/src` = `/cvmfs/cms.cern.ch/el9_amd64_gcc12/cms/cmssw/CMSSW_15_0_5/src`. "What Phase-2 changes" is
the difference of the effective `g4SimHits` (after all modifiers) between `Run3_2025` and the default Phase-2 era,
`Phase2C17I13M9` (`DEFAULT_VERSION = "Run4D110"` in `Configuration/Geometry/python/defaultPhase2ConditionsEra_cff.py`;
`'Run4D110': Geom ExtendedRun4D110, Era Phase2C17I13M9` in `Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py`,
line 3171), dumped from an initialized area with

```bash
for e in Run3_2025 Phase2C17I13M9; do python3 -c "
import FWCore.ParameterSet.Config as cms, importlib
m = importlib.import_module('Configuration.Eras.Era_$e'+'_cff'); p = cms.Process('X', getattr(m, '$e'))
p.load('SimG4Core.Application.g4SimHits_cfi'); print(p.g4SimHits.dumpPython())" > g4_$e.txt; done
diff g4_Run3_2025.txt g4_Phase2C17I13M9.txt
```

The difference: `CaloHits`, `OnlySDs`, `TrackHits` (`phase2_common`), `SaveCaloBoundaryInformation` in `CaloSD`,
`CaloTrkProcessing`, `TrackingAction` (`phase2_hgcal`), `ECalSD.StoreLayerTimeSim`, `ECalSD.TimeSliceUnit`
(`phase2_timing`), `LHCTransport`, `MuonSD.HaveDemoChambers` (`phase2_common`), `g4GeometryDD4hepSource` (True in
`Run3_2025` through `dd4hep`, which `Configuration/Eras/python/Era_Phase2_cff.py` excludes); nothing else.

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
  `LHCTransport = False`; `MuonSD.HaveDemoChambers = False`; comment "Selection of SD's for Phase2, exclude PPS",
  line 751, although the PPS sensitive detectors stay in the list), `hgcaltb`, `phase2_hgcalOnly` (used only by test
  configurations), `phase2_hgcalV18` (`HGCSD.HitCollection = 2`; only in `Era_Phase2C22I13M9_cff.py`),
  `fixLongLivedSleptonSim`; `phase2_hgcal` on `common_MCtruth` (`SaveCaloBoundaryInformation = True`); `fineCalo` on
  `common_MCtruth`. `phase2_common` `CaloHits` (line 758) has no `EcalHitsEE`, `EcalHitsES`.
- The eta cut outside `g4SimHits_cfi.py` (`grep -rn "MinEtaCut\|MaxEtaCut"`, test configurations left out):
  `Configuration/StandardSequences/python/SimWithCastor_cff.py`: `MinEtaCut = -7.0`, `MaxEtaCut = 5.5`;
  `SimG4Core/Application/python/NeutronBGforMuons_cff.py`: `MinEtaCut = -7.0`, `MaxEtaCut = 7.0`, `TrackNeutrino` left,
  `Physics.type = FTFP_BERT_XS_EML`; `NeutronBGforMuonsHP_cff.py`: `FTFP_BERT_HP_EML`; `cpuBenchmark_cff.py`:
  `FTFP_BERT`, `TrackNeutrino = True`, roulette limits 0, eta cut set to the same 5.5;
  `SimPPS/PPSSimTrackProducer/python/SimTrackProducerForFullSim_cff.py` (the "PPS one"): `MinEtaCut = -13.0`,
  `MaxEtaCut = 13.0`; `Configuration/StandardSequences/python/SimWithoutCastor_cff.py`: 5.5. No `toModify(g4SimHits`
  outside `g4SimHits_cfi.py` except `Validation/Geometry/python/materialBudgetVolume_cfi.py`.
- `Physics.type` set elsewhere (all customisation functions or tools): `cpuBenchmark_cff.py`, `NeutronBGforMuons*_cff.py`
  (above), `SimG4Core/CustomPhysics/python/Exotica_*_SIM_cfi.py` (`CustomPhysics`), `SimG4Core/GFlash/python/GflashSim_cfi.py`
  (`GFlash`), `SimG4Core/PrintGeomInfo/python/*` and `SimTracker/TrackerMaterialAnalysis` (`DummyPhysics`),
  `SimMuon/Neutron/python/SimIdealNeutrons*_cff.py` (`QGSP_BERT_HP`), `reproc2011_2012_cff.py`.
- `SimG4Core/Generators/src/Generator.cc`: lines 51 to 56 (`theRDecLenCut`, `theDecLenCut` local), 82 to 87
  (`Z_lmax = R * (1 - exp(-2 eta)) / (2 exp(-eta)) = R sinh(eta)`, `Z_hector`), 90 (the only use of `theDecLenCut`:
  a printout), 305 to 313 (the very-forward n / nbar / gamma rule), 316 to 365 (the p and geometric eta cuts), 369
  (status 2 with `r >= RDecLenCut` and `abs(z) < Z_hector`), 318 (`ptot < theMinPCut`: the 40 MeV cut is on the total
  momentum), 394 (`setGenId`).
- `SimG4Core/Application/src/StackingAction.cc`: line 189 (primary neutrinos killed), 204 (secondary neutrinos),
  341 (`if (prob < 1.0 && aTrack->GetKineticEnergy() < elim)`), 399 (`regionHGcal` set only if `gRusRoHGcal < 1.0 ||
  nRusRoHGcal < 1.0`).
- `SimG4Core/Application/src/RunManagerMT.cc`, `runForPhase2()` (lines 348 to 356): `m_isPhase2 = true` if a region
  `HGCalRegion`, `FastTimerRegionETL` or `FastTimerRegionBTL` exists; `RunManagerMTWorker.cc` line 454:
  `Phase2SteppingAction` instead of `SteppingAction`. `SteppingAction.cc`: line 24 (`CMStoZDCtransport` read), 138
  (region checks also for `sVeryForward`), 143 to 144 (dead regions, except `isForZDC`, `SteppingAction.h` lines 95 to
  97: only if `CMStoZDCtransport`). `Phase2SteppingAction.cc`: constructor lines 21 to 40 (`CMStoZDCtransport` never
  read; `CMS2ZDCName` read, unused), 142 to 148 (dead regions, no ZDC exception), 171 to 213 (tracker / BTL
  transitions).
- `SimG4Core/PhysicsLists/src/CMSEmStandardPhysics.cc`: lines 56 to 57 (`SetMscRangeFactor(0.2)`,
  `SetMscStepLimitType(fMinimal)`: the defaults), 112 to 113 (`HcalRegion`, `HGCalRegion` looked up), 142 ("e-/e+ msc
  for HCAL and HGCAL using the Urban model"), 144 to 153 (`msc3` with `G4MscRangeFactor = 0.04`, `G4MscGeomFactor =
  2.5`, `G4MscSafetyFactor = 0.6`, `G4MscLambdaLimit = 1` mm, `G4MscStepLimit = UseSafety` of `g4SimHits_cfi.py`),
  173 to 190 (`msc3` added for both regions); the same for e+ at lines 214 to 259.
- `SimG4CMS/Forward/src/MtdSD.cc`: lines 47 to 48 (`//default must be 0.5`, `//default must be 0.05`);
  `SimG4CMS/Forward/src/TimingSD.cc`: lines 163 to 172 (`setStoreTrack` only if `incidentEnergy > energyCut`).
- `SimG4CMS/Calo/src/CaloSD.cc`: line 43 (`eminHit(0.)`), 59 (`tmaxHit = TmaxHit`, 1000 ns), 87 to 99 (`EminHits`,
  `TmaxHits` taken only for a collection named in `HCNames`).
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
  `(+1.5, +3.0)` (lines 97, 121, 126), `(-5.5, -3.0)`, `(+3.0, +5.5)` (lines 412, 416).
- The geometries: `Geometry/CMSCommonData/python/cmsExtendedGeometryRun4D110XML_cfi.py` (D110) against
  `cmsExtendedGeometry2025XML_cfi.py`. Both build the HF from `Geometry/HcalCommonData/data/hcalforwardalgo.xml`
  (`VCAL`: `rMin = cms:ForwdBeamR1`, `dzVCal` from `ForwdVcalZ1`, `ForwdVcalZ2`) with `hf.xml`, `hfpmt.xml`,
  `hffibrebundle.xml`; `Geometry/CMSCommonData/data/cms/2030/v5/cms.xml` (D110) and `cms/2021/v3/cms.xml` (2025):
  `ForwdVcalZ1 = 11.1495*m`, `ForwdVcalZ2 = 12.8005*m`, `ForwdBeamR1 = 12.495*cm`; asinh(1114.95 / 12.495) = 5.18,
  asinh(1280.05 / 12.495) = 5.32. Beam pipe: `Geometry/CMSCommonData/data/beampipe/2030/v3/beampipe.xml` (D110) and
  `beampipe/2021/v1/beampipe.xml` (2025) are identical (`diff` empty); `BeampipeRout1 = 2.2500*cm` (`BeamTube1`,
  length `BeampipeZ2 = 1.902*m`), `BeampipeRin1 = Rout1 - 0.8*mm`. First inner-tracker layer of D110:
  `Geometry/TrackerCommonData/data/PhaseII/Tracker_DD4hep_compatible_IT711_2023_05/pixel.xml`, `ITLayer1` (`rMin =
  26.905*mm`, line 6596), rods at `Radius = 27.5*mm` (flipped, line 12381) and `32.5*mm` (unflipped, line 12392).
  Beam line: `Geometry/CMSCommonData/data/extend/v2/cmsextent.xml` (D110; `v3` in 2025, same `CMStoZDC`): `CMStoZDC`,
  `dz = 56*m - 25*cm` at `z = +-(83*m + 25*cm)`, i.e. abs(z) 27.5 to 139.0 m, `CMSCutsRegion = InterimRegion`;
  `Geometry/ForwardCommonData/data/cmszdc.xml`: `zposition = 140.0*m`; `Geometry/ForwardSimData/data/zdcsens.xml`
  (D110). Sensitive-volume files of D110: `trackersens.xml`, `pixelsens.xml`, `ecalsens.xml`, `hcalsenspmf.xml`,
  `hgcsensv15.xml`, `muonSens.xml`, `zdcsens.xml`, `mtdsens.xml` (no `pltsens`, `bcm1fsens`, `bhmsens`, Roman pots).
  Production cuts of D110: `Geometry/HGCalSimData/data/hgcProdCutsv15.xml` (`HGCal`, `HGCalEE`, `HGCalHEsil`,
  `HGCalHEmix` -> `HGCalRegion`, 0.03 mm for e-, e+, gamma, protons), `Geometry/EcalSimData/data/EcalProdCuts.xml`
  (`EcalRegion` 1 mm), `Geometry/HcalSimData/data/HcalProdCuts/2030/v1/HcalProdCuts.xml` (`HcalRegion` 1 mm),
  `Geometry/MTDSimData/data/v5/mtdProdCuts.xml` (0.1 mm; 0.01 mm in the ETL sensors).
- `config/toolbox/el9_amd64_gcc12/tools/selected/geant4.xml`: `geant4` version 11.2.2; the log banner:
  `Geant4 version Name: geant4-11-02-patch-02 [MT] (21-June-2024)`; `g4hepem.xml`, `g4hepemcore.xml` are in the same
  toolbox (`G4HepEmActive = False`).
- Not quoted, since the release carries no history: since when the `RusRoHGcal*` and the other values are what
  they are.

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

### 4.8 The numbers added for the teaching version of the chapter (12 November 2026)

The chapter was rewritten in parts A to F (one per ingredient of the SIM step); the numbers it added are listed here.

**`extras_09_G4.txt`** (`python3 test/09_G4/g4Extras_09_G4.py ...`, section 3; quoted in parts B, D, E, F, table
"Where the primaries cross into a calorimeter volume"):

```
== g4Extras chapter 09_G4: step2_G4_signal_09_G4.root, step2_G4_pileup_09_G4.root
Generator cuts: Z_lmax = 2.9 cm * sinh(5.5) = 354.8 cm; Z_hector = 2.9 cm * sinh(8.2) = 5279.4 cm = 52.8 m
signal primary vertex (0.028, -0.017, 3.11) cm
corner of the tracker volume (r, z) = (123.3, -293.2) cm seen from the vertex: eta = -1.611

-- (1) the tracker/calorimeter boundary
signal 1:1:1: 571 primaries, 557 reach the boundary
  barrel cylinder r = 123.3 cm      172  |eta| 0.01-1.61  r 123.2-123.3 cm  |z| 0.4-293.2 cm
  endcap disk |z| = 293.2 cm        228  |eta| 1.60-3.23  r 22.9-121.6 cm  |z| 293.2-293.2 cm
  HF front face |z| = 1114.9 cm      36  |eta| 3.26-5.16  r 12.8-85.2 cm  |z| 1114.9-1114.9 cm
  nose ring |z| = 355.1 cm           98  |eta| 3.50-4.51  r 7.8-21.3 cm  |z| 355.1-355.1 cm
  inner wall along the beam pipe     23  |eta| 4.51-5.32  r 7.8-12.5 cm  |z| 356.1-1277.4 cm
  not reaching                       14
  b system (B-): 9 stable; barrel cylinder r = 123.3 cm 7, endcap disk |z| = 293.2 cm 1, no SimTrack (pdg -12) 1; r 114.2-123.3 cm, z -293.2 to -240.2 cm
  bbar system (B*+): 16 stable; nose ring |z| = 355.1 cm 16; r 15.4-20.3 cm, z -355.1 to -355.1 cm

-- (2) the eta cut as a line: stable particles of the signal with 5.3 < |eta| < 5.7
      eta    pdg  E[GeV]     pT   vtx z   r(nominal)    r(vertex) primary
  -5.6085     22    39.4  0.289    3.11        2.602        2.612 False
  -5.5415   -211    98.5  0.773    3.11        2.782        2.799 False
  -5.5240     22    12.9  0.103    3.11        2.831        2.887 False
  -5.5002    211    84.6  0.692    3.11        2.899        2.940 True
  -5.4995   -211    52.8  0.432    3.11        2.901        2.956 True
  -5.3986   -211    17.1  0.155    3.11        3.209        3.260 True
  -5.3804   -211    32.4  0.299    3.11        3.268        3.329 True
  -5.3688     22     5.3  0.050    3.11        3.307        3.365 True
  -5.3245     22    51.9  0.505    3.11        3.456        3.507 True
  -5.3190    211    22.0  0.216    3.11        3.475        3.501 True
  +5.3001   2112    64.5  0.644    3.11        3.542        3.521 True
  +5.3406     22    18.6  0.178    3.12        3.401        3.374 True
  +5.3481    130    92.9  0.884    3.11        3.376        3.342 True
  +5.4325    130    36.0  0.315    3.11        3.103        3.090 True
  +5.4445     22    28.6  0.247    3.11        3.065        3.026 True
  +5.4544     22    32.9  0.281    3.11        3.035        3.016 True
  +5.4617  -3112    61.6  0.523    3.11        3.013        3.018 True
  +5.4745     22    89.3  0.749    3.11        2.975        2.973 True
  +5.4824    310    42.7  0.355    3.11        2.951        2.903 True
  +5.5200    211    20.4  0.164    3.11        2.843        2.792 False
  +5.5206     22     3.2  0.026    3.11        2.841        2.800 False
  +5.5311     22     9.7  0.077    3.11        2.811        2.756 False
  +5.6026     22    23.6  0.174    3.11        2.617        2.624 False
  +5.6414    211    44.5  0.316    3.11        2.518        2.498 False

-- (3) the HF shower library (all HcalHits of the transport-only run are HF library hits)
  746 hits in 459 cells, 1511 photoelectrons, from 144 tracks with E = 4266.4 GeV: 0.354 photoelectrons per GeV
  track  501 pdg   211 E  642.9 GeV eta -3.74:  321 photoelectrons, 0.50 per GeV
  track  562 pdg   130 E  241.0 GeV eta -3.71:   84 photoelectrons, 0.35 per GeV
  track  571 pdg    22 E  132.9 GeV eta -3.73:   67 photoelectrons, 0.50 per GeV
  track  569 pdg    22 E  179.0 GeV eta -3.77:   65 photoelectrons, 0.36 per GeV
  track  560 pdg  -211 E   95.9 GeV eta -3.80:   54 photoelectrons, 0.56 per GeV
  track  561 pdg   211 E  164.6 GeV eta -3.78:   46 photoelectrons, 0.28 per GeV

-- (4) tracker crossings (PSimHits, zero energy loss) per primary
  6734 hits from 375 tracks; time of flight 0.21-13.95 ns
  charged primaries 0.0 < |eta| < 1.0:  38, hits per track 18.7 (min 17, max 21)
  charged primaries 1.0 < |eta| < 2.0:  72, hits per track 21.3 (min 14, max 32)
  charged primaries 2.0 < |eta| < 2.5:  49, hits per track 21.0 (min 11, max 28)
  photons |eta| < 1: 47, hits per photon 18.6
  most hits: 32, track 358 pdg 22 pT 0.047 GeV eta -1.96

-- (1b) one pileup collision
pileup 1:1:72: 164 primaries, 155 reach the boundary
  barrel cylinder r = 123.3 cm       54  |eta| 0.07-1.60  r 123.1-123.3 cm  |z| 8.1-293.2 cm
  endcap disk |z| = 293.2 cm         66  |eta| 1.62-3.25  r 22.8-120.8 cm  |z| 293.2-293.2 cm
  HF front face |z| = 1114.9 cm       9  |eta| 3.31-4.92  r 16.3-81.9 cm  |z| 1114.9-1114.9 cm
  nose ring |z| = 355.1 cm           18  |eta| 3.63-4.48  r 8.1-18.8 cm  |z| 355.1-355.1 cm
  inner wall along the beam pipe      8  |eta| 4.57-5.30  r 7.9-12.5 cm  |z| 380.3-1245.4 cm
  not reaching                        9
  HF: 153 hits in 135 cells, 192 photoelectrons
```

Notes on it: the surfaces are classified by the crossing point (`r = 123.3 cm` within 0.5 cm, `|z| = 293.2 cm`,
`|z| = 355.15 cm`, `|z| = 1114.9 cm`, anything else is the inner wall along the beam pipe); the b system is the 9
stable descendants of the B- (pdg -521, pT 94.3), the bbar system the 16 of the B*+ (pdg 523, pT 114.2), matched to
`SimTrack`s by `genParticles` index + 1 = HepMC barcode. The b-system crossings one by one (the same script, printed
during the development): pi- pT 11.0 eta -1.53 at z = -269.0 cm; pi+ 13.3, -1.54, -272.7; K0S 8.0, -1.61, -291.6;
photon 0.3, -1.60, -289.1; photon 0.2, -1.52, -265.5; photon 17.7, -1.57, -279.1; e- 5.0, -1.43, -240.2 (all through the
barrel cylinder); photon 1.5, -1.68 through the endcap disk at r = 114.2 cm; the anti-nu_e has no `SimTrack`. The
"charged primaries" of section (4) are all charged `SimTrack`s, which here are all primaries (no secondaries).

**Worked examples of part B** (the eta cut as a line): `sinh(5.5) = 122.34`, `2.9 cm x 122.34 = 354.8 cm`;
`sinh(8.2) = 1820.5`, `2.9 cm x 1820.5 = 5279 cm = 52.8 m`; the pi+ at eta -5.5002: `354.8 / sinh(5.5002) = 2.899 cm`
from the nominal point, 2.940 cm from the vertex at z = +3.11 cm (flight 354.8 + 3.1 = 357.9 cm), kept; the photon at
-5.5240: 2.887 cm, dropped; the K0S at +5.4824: 2.951 cm nominal, 2.903 cm from the vertex, kept (all from
`extras_09_G4.txt`, section (2)). With more digits (FWLite, the same construction): the pi+ at eta -5.500201 reaches
r = 2.89942 cm from the nominal point, 5.8 um inside the 2.9 cm ("6 um" in the chapter), and 2.94031 cm from the
vertex; the K0S at +5.482419 reaches 2.90257 cm from the vertex, 25.7 um outside ("30 um").

**Worked examples of part D**: the corner of the tracker volume seen from the vertex, `asinh((293.2 + 3.11) / 123.3) =
asinh(2.403) = 1.611`; the hole of the HF, `asinh(1280.05 / 12.495) = asinh(102.4) = 5.32` (the HF, `VCAL`, from
`ForwdVcalZ1 = 11.1495 m` to `ForwdVcalZ2 = 12.8005 m`, `rMin = ForwdBeamR1 = 12.495 cm`, `rMax = ForwdVcalR1 = 1.595 m`;
`Geometry/HcalCommonData/data/hcalforwardalgo.xml` line 18 and `Geometry/CMSCommonData/data/cms/2021/v3/cms.xml`
lines 87, 100 to 102). The bbar system flies from the nose ring at z = -355.1 cm to the HF face at -1114.9 cm: 7.6 m.
The b system at eta -1.55 is 1.61 - 1.55 = 0.06 from the corner.

**Worked example of part C** (decay lengths; masses and lifetimes from the Particle Data Group: m(B+-) = 5.279 GeV,
c tau(B+-) = 491.1 um; m(D0) = 1.865 GeV, c tau(D0) = 123.0 um; c tau(K0S) = 26.8 mm, c tau(Lambda) = 78.9 mm, quoted
rounded as 27 and 79 mm; the 10 mm of `tau0Max`):

| hadron | E [GeV] | beta gamma = p / m | mean beta gamma c tau | flight | flight / mean | P(flight > observed) |
|---|---|---|---|---|---|---|
| B- (b jet) | 235.9 | 44.7 | 2.19 cm | 0.78 cm (section 4.4) | 0.36 | 0.70 |
| B+ (bbar jet, from the B*+ at the primary vertex) | 2388.2 | 452.4 | 22.22 cm | 11.73 cm | 0.53 | 0.59 |
| D0bar (from the D*0bar of the B+, at the B+ vertex) | 911.9 | 489.0 | 6.01 cm | 25.80 - 11.73 = 14.07 cm | 2.34 | 0.10 |

The D0bar at eta -3.76: pz / pT = sinh(3.76) = 21.4 ("21 times faster along the beam"). Its daughters, from
`genParticles`: K0bar (pdg -311, pT 12.9), pi- (4.3), pi+ (7.5), pi0 (9.1), pi0 (8.7); the B+ daughters D*0bar (-423, 48.1),
pi+ (30.6), pi0 (5.5), K0 (311, 15.7), K*0bar (-313, 12.8).

**Part A, the detector** (all under `$CMSSW_RELEASE_BASE/src`):

- `Geometry/CMSCommonData/data/dd4hep/cmsExtendedGeometry2024.xml`: 382 lines, 374 `<Include ref=...>` files (`grep -c`);
  the payload of `GeometryFileRcd` / `Extended` is `141DD4hepV2_Extended2024` (section 4.7).
- `Geometry/CMSCommonData/data/cms/2021/v3/cms.xml` lines 21 to 25, 32, 45 to 52: `TrackCalorR = 1.233 m`,
  `CalorMuonR = 2.950 m`, `TrackBeamZ2 = 2.932 m`, `TrackBeamR3 = 7.7633 cm`, `PIXNoseRin = 215 mm`, `PIXNoseLength =
  619.5 mm` (293.2 + 61.95 = 355.15 cm), `CalorBeamZ2 = 5.541 m`, `CalorBeamR2 = 8.93 cm`; logical parts `OCMS` (line 151)
  and `CMSE` (line 159). `Geometry/CMSCommonData/data/caloBase/2017/v1/caloBase.xml`: the `CALO` polycone built from them.
- `Geometry/HcalSimData/data/CaloUtil/2021/v1/CaloUtil.xml`: `SensitiveDetector = CaloTrkProcessing`, `Calorimeter` =
  `CALO`, `VCAL`, `MBAT`, `MBBT`, ..., `CAST`, `ZDC`; `Inside` = `BEAM`, `Tracker`, `MUON`, `BEAM2`, `MB`, ...
- Regions and production cuts (`CMSCutsRegion`, `ProdCutsForGamma/Electrons/Positrons`): `Geometry/TrackerSimData/data/
  trackerProdCutsBEAM.xml`: `BeamPipe` (the `BeamTube1` volume) 0.01 mm, `BeamPipeOutside` 1 mm;
  `Geometry/TrackerSimData/data/PhaseI/trackerProdCuts.xml`: `TrackerDeadRegion` (`//Tracker`) 1 mm, `TrackerSensRegion`
  (the active strip volumes `TIBActiveRphi0`, ...) 0.1 mm, `TrackerPixelDeadRegion` 1 mm; `Geometry/EcalSimData/data/
  EcalProdCuts.xml`: `EcalRegion` (`//ECAL`) 1 mm, `APDRegion` (`EAPD.*`, `EATJ.*`) 0.005 mm; `ESProdCuts.xml`:
  `PreshowerRegion` 1 mm, `PreshowerSensRegion` (`SFSX`, `SFSY`) 0.1 mm; `Geometry/HcalSimData/data/HcalProdCuts/2021/v2/
  HcalProdCuts.xml`: `HcalRegion` (`CALO`, `MBAT`, `MBBT.*`, `VCAL`) 1 mm; `Geometry/MuonSimData/data/muonProdCuts/2021/v3/
  muonProdCuts.xml`: `Muon` (`//MUON`) 100 mm for photons, 10 mm for e+- and protons (and the sub-regions `MuonIron`,
  `MuonChamber`, `MuonSensitive_DT-CSC`, `MuonSensitive_RPC`). `g4SimHits_cfi.py` lines 190 to 192: `CutsPerRegion = True`,
  `CutsOnProton = True`, `DefaultCutValue = 1.0` ("cuts in cm").
- `g4SimHits_cfi.py` lines 695 to 698 (`run3_common`): `OnlySDs` with 15 entries, `TrackHits` with 25 collections
  (`BCM1FHits`, `BHMHits`, `BSCHits`, `CTPPSPixelHits`, `CTPPSTimingHits`, `MuonCSCHits`, `MuonDTHits`, `MuonGEMHits`,
  `MuonME0Hits`, `MuonRPCHits`, `PLTHits`, `TotemHitsRP`, `TotemHitsT1` and the 12 tracker `LowTof`/`HighTof`),
  `CaloHits` with 7 (`CaloHitsTk`, `EcalHitsEB`, `EcalHitsEE`, `EcalHitsES`, `HcalHits`, `TotemHitsT2Scint`, `ZDCHITS`).
- In `step2_G4_signal_09_G4.root` (`edmDumpEventContent`, then FWLite): 36 products of process `SIM`, of which 32 hit
  collections; filled for the signal: the six tracker `LowTof` collections (908, 829, 960, 679, 860, 2498) and `HcalHits`
  (746); `BSCHits`, `MuonME0Hits` and `TotemHitsT1` are declared but not produced; `SimTrack` 571, `SimVertex` 29.
- `SimG4CMS/Calo/src/HFShowerLibrary.cc`, `interpolate()` (lines 575 to 640): one stored shower from each of the two
  library energies around the particle's momentum, the photons of the lower one kept with probability 1 - w and of the
  upper one with w; above the highest energy `extrapolate()`; separate electromagnetic and hadronic showers (`isEM`).

**Part E, the hits per track**: a straight track at abs(eta) < 1 crosses 4 PXB layers, 4 TIB layers and 6 TOB layers;
TIB layers 1 and 2 and TOB layers 1 and 2 are double-sided (an r-phi and a stereo sensor, each its own `DetId`), hence
4 + (4 + 2) + (6 + 2) = 18 sensors, against 18.7 (17 to 21) measured for the 38 charged primaries with abs(eta) < 1
(`extras_09_G4.txt`, section (4)). HF photoelectrons per GeV: 1511 / 4266.4 = 0.354; pi+ 321 / 642.9 = 0.50,
K0L 84 / 241.0 = 0.35, pi+ 46 / 164.6 = 0.28.

