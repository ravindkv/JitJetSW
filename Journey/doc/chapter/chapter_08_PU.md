# Chapter 08, PU: technical companion

The companion of `doc/chapter/chapter_08_PU.tex` (Rule C of `Constitution.txt`): the commands of
`test/08_PU/run_08_PU.sh`, a walk-through of the configurations, and every number the chapter quotes, copied
from `test/08_PU/out/`. All commands are run from `JitJetSW/Journey/test` in an initialised CMSSW_15_0_5
(`cd CMSSW_15_0_5/src && cmsenv`); the chapter needs the output of chapter 07,
`test/07_GenJet/out/step2_GENJET_07_GenJet.root` (Rule E).

```bash
./08_PU/run_08_PU.sh                 # everything below: the library, the catalogue, the drawn collision, the overlay, the ledger
RUN=0 ./08_PU/run_08_PU.sh           # only write step1_GEN_MinBias_08_PU_cfg.py
ANALYZE=0 ./08_PU/run_08_PU.sh       # the library and gencheck_08_PU.log only (no catalogue, no plots, no overlay)
PUSEED=777 ./08_PU/run_08_PU.sh      # another 1000 collisions (seeds 777 / 778; the draw then still gives 746 / 64 / 185)
NPU=20 SKIP=0 ./08_PU/run_08_PU.sh   # overlay library events 1-20 instead of 747-810 (WARNING_intime_08_PU.txt: not the draw)
BUNCHES=-1,1 SKIP=71 NPU=68 ./08_PU/run_08_PU.sh   # the three crossings of chapters 10-13: the draw gives 71 / 68 / 54
PUTYPE=poisson ./08_PU/run_08_PU.sh  # draw the number of in-time collisions from Poisson(NPU) with seed 20243
JETS=ak4GenJetsPU,ak4GenJetsNoNuPU,ak8GenJetsPU ./08_PU/run_08_PU.sh   # three overlay pages (9, 10, 11)
```

Variables (defaults): `PUEVENTS=1000 PUSEED=20241 MIXSEED=12345 PUTYPE=fixed JETS=ak4GenJetsPU JETPTMIN=3 LABELPTMIN=30`, plus
the common ones of `common/jitjet_common.sh` (`YEAR=2025 CONDITIONS=auto:phase1_2025_realistic ERA=Run3_2025
GEOMETRY=DB:Extended BEAMSPOT=DBrealistic COMENERGY=13600.0 EVENT=1:1:1 RUN=1 ANALYZE=1 PNG=0` (the .tex of chapters 01-08 include the PDFs), and the pileup
convention of the book, set per chapter in `jitjet_init` like `YEAR`: for 08 and 09 `PU=60 BUNCHES=-12,3 SKIP=746 NPU=64`,
for 10-13 still `PU=60 BUNCHES=-1,1 SKIP=71 NPU=68`). `SKIP` and `NPU` are the library events used up by the
early bunch crossings and the in-time collisions of bunch crossing 0 in the draw of the MixingModule for this
signal event (seed `MIXSEED` = 12345, Poisson(60) per bunch crossing, the library read in sequence, early
crossings first): with bx -12..3, events 1-746 in the twelve early crossings, 747-810 in bx 0, 811-995 in the
three late ones (section 4.4). The script makes the draw itself (`common/pileupDraw_cfg.py`, `draw_08_PU.txt`) and
writes `WARNING_intime_08_PU.txt` if it does not give `SKIP` and `NPU`. `BUNCHES=-12,3` is the window of every
Run-3 scenario of the release (`SimGeneral/MixingModule/python/mix_probFunction_25ns_PoissonOOTPU_cfi.py`:
`minBunch = cms.int32(-12)`, `maxBunch = cms.int32(3)`, imported by all the `Run3_*` and `mix_202*` profiles,
none of which overrides it; chapter 08 uses it since 16 November 2026, before that bx -1..1 with a 250-event
library). The first 250 events of the 1000 are those of the 250-event library, collision by collision (same
seeds; `genParticles` compared particle by particle with FWLite, and the 250 catalogue lines identical), so the
outputs of chapters 10-13, made from the 250-event library, are unchanged; chapter 09 was moved the same day (all
1000 collisions through Geant4, the ledger of library events 747-810).

## 1. The cmsDriver.py commands

One `cmsDriver.py` command, written by `run_driver` of `common/jitjet_common.sh` (its own output is
`out/step1_GEN_MinBias_08_PU_cfg.driver.log`); the command line is also the second line of the generated
configuration (`# with command line options: ...`):

```bash
cmsDriver.py MinBias_14TeV_pythia8_TuneCP5_cfi --conditions auto:phase1_2025_realistic --era Run3_2025 \
  --geometry DB:Extended --beamspot DBrealistic -n 1000 -s GEN --datatier GEN --eventcontent FEVTDEBUG \
  --fileout file:step1_GEN_MinBias_08_PU.root \
  --customise_commands "process.generator.comEnergy = cms.double(13600.0)\nprocess.RandomNumberGeneratorService.generator.initialSeed = 20241\nprocess.RandomNumberGeneratorService.VtxSmeared.initialSeed = 20242" \
  --python_filename 08_PU/step1_GEN_MinBias_08_PU_cfg.py --no_exec
```

| argument | what it does |
|---|---|
| `MinBias_14TeV_pythia8_TuneCP5_cfi` | the generator fragment, `Configuration/Generator/python/MinBias_14TeV_pythia8_TuneCP5_cfi.py` of the release: `Pythia8ConcurrentGeneratorFilter` with `SoftQCD:inelastic = on`, the common settings block and the CP5 tune block, `comEnergy = 14000` |
| `--conditions auto:phase1_2025_realistic` | the global tag of 2025 (`150X_mcRun3_2025_realistic_v1`, `Configuration/AlCa/python/autoCond.py`, line 107); in a GEN job it supplies only the beam-spot payload (`SimBeamSpotObjectsRcd`) of the vertex smearing, `SimBeamSpot_Realistic25ns13p6TeVEarly2024CollisionVtxSmearingParameters_v1_mc`, the same tag as in the 2024 global tag `150X_mcRun3_2024_realistic_v2` of chapters 01-07 (section 4.9) |
| `--era Run3_2025` | the era modifiers (`Configuration/Eras/python/Era_Run3_2025_cff.py`: `Run3_2024` plus `run3_GEM_2025`, `run3_CSC_2025`, `run3_SiPixel_2025`, `stage2L1Trigger_2025`, `ecal_cctiming`, which act only in the reconstruction and the Level-1 trigger); in a GEN job they switch on the PPS proton transport (`LHCTransport`, `run3_common`) |
| `--geometry DB:Extended` | not used by the generator; carried in the configuration so that the SIM step of chapter 09 inherits it |
| `--beamspot DBrealistic` | the vertex-smearing module: `IOMC/EventVertexGenerators/python/VtxSmearedRealistic_cfi.py`, a `BetafuncEvtVtxGenerator` with `readDB = True` |
| `-n 1000` | events to generate (`PUEVENTS`); the filter accepts every event, so 1000 are written: the draw of the 16 crossings uses 995 |
| `-s GEN` | the step: the generator sequence `pgen` of `Configuration/StandardSequences/python/Generator_cff.py` |
| `--datatier GEN --eventcontent FEVTDEBUG` | the label of the output and the list of kept products (everything the GEN step makes) |
| `--fileout file:step1_GEN_MinBias_08_PU.root` | the output file, written in `out/` (`cmsRun` runs there) |
| `--customise_commands "..."` | Python lines appended verbatim to the configuration (a literal `\n` separates them): the energy of the signal and the two seeds (`PUSEED`, `PUSEED + 1`) |
| `--python_filename ... --no_exec` | the name of the configuration, and "write it, do not run it" (`run_driver` then runs `cmsRun` on it) |

The signal of chapters 01-07 used the same geometry, beam spot and energy with its own fragment
(`BBbar_14TeV_TuneCP5_cfi`), the `cmsDriver.py` default seeds and the 2024 settings (`YEAR=2024`:
`auto:phase1_2024_realistic`, `Run3_2024`): the generator chapters keep 2024, the CMS chapters 08 to 13 run
with the 2025 settings of the release. At generator level the two are the
same: the only record a GEN job reads is `SimBeamSpotObjectsRcd`, one tag in both global tags, so the signal
and the library come from the same luminous region (section 4.9). The library must not reuse the signal's
seeds, or library event k would get the vertex of signal event k.

## 2. The cmsRun on the python config

```bash
cd 08_PU/out && cmsRun ../step1_GEN_MinBias_08_PU_cfg.py > step1_GEN_MinBias_08_PU.log 2>&1 && cd ../..   # about 3 min
```

The generated configuration `08_PU/step1_GEN_MinBias_08_PU_cfg.py`, part by part:

- **process and era**: `process = cms.Process('GEN', Run3_2025)`; the standard includes (`Services_cff`,
  the particle table `pythiapdt_cfi`, `MessageLogger`, `EventContent_cff`, `mixNoPU_cfi` (no mixing in a
  GEN job), `GeometryRecoDB_cff`, `MagneticField_cff`, `Generator_cff`, `VtxSmearedRealistic_cfi`,
  `genFilterSummary_cff`, `EndOfProcess_cff`, `FrontierConditions_GlobalTag_cff`).
- **source and events**: `process.source = cms.Source("EmptySource")` (a generator job makes its own events,
  run 1, lumi 1, events 1 to 1000); `process.maxEvents.input = 1000`.
- **output**: `process.FEVTDEBUGoutput`, a `PoolOutputModule` with `SelectEvents = ['generation_step']` (only
  events that pass the generator filter) and `fileName = 'file:step1_GEN_MinBias_08_PU.root'`.
- **global tag**: `process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase1_2025_realistic', '')`.
- **the generator**: `process.generator = cms.EDFilter("Pythia8ConcurrentGeneratorFilter", ...)`, the
  fragment spelled out in full: `processParameters = ['SoftQCD:inelastic = on']`, the CP5 block (`Tune:pp 14`,
  `MultipartonInteractions:pT0Ref=1.41`, `ecmPow=0.03344`, `bProfile=2`, `coreRadius=0.7634`,
  `coreFraction=0.63`, `ColourReconnection:range=5.176`, `SigmaTotal:zeroAXB=off`, alpha_s = 0.118 at two
  loops in the hard process, both showers and the MPI, `SigmaTotal:mode = 0`, `SigmaTotal:sigmaEl = 21.89`,
  `SigmaTotal:sigmaTot = 100.309`, `PDF:pSet=LHAPDF6:NNPDF31_nnlo_as_0118`), the common block
  (`Main:timesAllowErrors = 10000`, `Check:epTolErr = 0.01`, `ParticleDecays:limitTau0 = on`,
  `ParticleDecays:tau0Max = 10`, `ParticleDecays:allowPhotonRadiation = on`), `comEnergy = 14000.0`
  (overridden below), `crossSection = 71390000000.0` (metadata only), `maxEventsToPrint = 0`.
- **paths and schedule**: `generation_step = cms.Path(process.pgen)`, the end paths `genfiltersummary_step`,
  `endjob_step`, `FEVTDEBUGoutput_step`; `ProductionFilterSequence` (the generator filter) is inserted at the
  start of every path. `pgen` runs, per event: `randomEngineStateProducer` (records the engine states),
  `generator` (Pythia8, `HepMCProduct generator:unsmeared` + `GenEventInfoProduct`), `VtxSmeared` (moves the
  event to a vertex of the Run-3 beam spot, the early-2024 payload that the 2025 global tag keeps, in space and
  time), `generatorSmeared` (the same record under the label every later step reads), `genParticles` (the record as `reco::GenParticle`s with `xyz0`, `t0`),
  `genJetMET` (ak4 / ak8 jets with and without neutrinos, `genMetTrue`, `genMetCalo`), `LHCTransport` (the
  surviving protons carried to the PPS Roman pots).
- **the customisation lines** at the end: `process.generator.comEnergy = cms.double(13600.0)`,
  `process.RandomNumberGeneratorService.generator.initialSeed = 20241`,
  `process.RandomNumberGeneratorService.VtxSmeared.initialSeed = 20242`, then the early-deletion customisation
  `customiseEarlyDelete`.

The checks the script writes into `out/gencheck_08_PU.log`: the process, geometry, vertex-smearing and
global-tag lines of the configuration, the seeds, `edmFileUtil -e` (1000 events), the Pythia8 statistics and
the `GenXsecAnalyzer` cross section (section 4).

**The output ROOT file** (`edmDumpEventContent file:08_PU/out/step1_GEN_MinBias_08_PU.root`), 31 products
per event, all from the process `GEN`; 40 MB for 1000 events, 40 kB per event (`edmEventSize -v`, compressed: 26.9 kB
the HepMC record, 10.7 kB the `GenParticle`s, 0.5 kB the engine states):

```
Type                                  Module                      Label      Process
GenEventInfoProduct                   "generator"                 ""         "GEN"      process id (101..106), weight, qScale
edm::HepMCProduct                     "generatorSmeared"          ""         "GEN"      the full record, vertex smeared (the only HepMC the mixing reads)
edm::HepMCProduct                     "LHCTransport"              ""         "GEN"      the protons at the PPS Roman pots
edm::RandomEngineStates               "randomEngineStateProducer" ""         "GEN"      engine states at the start of the event
edm::TriggerResults                   "TriggerResults"            ""         "GEN"      the generator filter decision (always true)
vector<reco::GenParticle>             "genParticles"              ""         "GEN"      the record as a flat collection
vector<int>                           "genParticles"              ""         "GEN"      the HepMC barcode of every entry
ROOT::Math::PositionVector3D<...>     "genParticles"              "xyz0"     "GEN"      the smeared vertex
float                                 "genParticles"              "t0"       "GEN"      the smeared vertex time
vector<reco::GenJet>                  "ak4GenJets", "ak4GenJetsNoNu", "ak8GenJets", "ak8GenJetsNoNu"   "GEN"   + rho, sigma, rhos, sigmas (double, vector<double>) per collection
vector<reco::GenMET>                  "genMetTrue", "genMetCalo"  ""         "GEN"
```

The unsmeared record `generator:unsmeared` is not kept by the FEVTDEBUG event content.

**The overlay** is a second `cmsRun`, on an analyzer configuration (section 3); its EDM output
`step2_GENJETPU_08_PU.root` (`edmDumpEventContent`) holds, from the signal file (process `GEN`),
`GenEventInfoProduct generator`, `genParticles` (+ barcodes, `xyz0`, `t0`), and from the process `GENJETPU`:

```
CrossingFramePlaybackInfoNew          "mix"                       ""         "GENJETPU"   which library events went in
PileupMixingContent                   "mix"                       ""         "GENJETPU"   bunch crossings [0], collisions [64], true mean [64.0]
vector<reco::GenParticle>             "genParticlesPU"            ""         "GENJETPU"   30 295 particles, collisionId 0 = signal, 1..64 = pileup
vector<int>, xyz0, t0                 "genParticlesPU"                       "GENJETPU"
edm::PtrVector<reco::Candidate>       "genParticlesForJetsPU", "genParticlesForJetsNoNuPU"   "GENJETPU"   the status 1 inputs (without neutrinos for NoNu)
vector<reco::GenJet>                  "ak4GenJetsPU", "ak4GenJetsNoNuPU", "ak8GenJetsPU", "ak8GenJetsNoNuPU"   "GENJETPU"   + rho, sigma, rhos, sigmas
```

**The draw** is a third `cmsRun` (section 3), whose output `draw_08_PU.root` holds one product,
`CrossingFramePlaybackInfoNew "mix" "" "DRAW"`: the number of library events in each of the 16 bunch crossings and
their event ids, for signal event 1:1:1.

## 3. The analyzer configs

Five steps after the GEN job, all run by the script (`ANALYZE=1`):

**(8b) the catalogue**, `common/minBiasGenStats.py` (FWLite, no `cmsRun`):

```bash
python3 common/minBiasGenStats.py 08_PU/out/step1_GEN_MinBias_08_PU.root > 08_PU/out/genstats_08_PU.txt 2> 08_PU/out/genstats_08_PU.log
python3 common/minBiasGenStats.py 08_PU/out/step1_GEN_MinBias_08_PU.root --event 747 774   # + the stable particles of events 747 and 774
```

One line per collision (process from `signal_process_id()` of the HepMC record, interactions = 1 + the
status-31 parton pairs, stable / charged / central (|eta| < 2.5, pT > 0.5 GeV) / |eta| > 5.5 particles, the
hardest particle, the scalar sum of pT, ak4 jets above 10 GeV, protons at the PPS, vertex z and t), then the
summary (process shares, means, species, |eta| and pT distributions, multiplicity against interactions, the
SD masses). Section 4 copies the summary and the lines of the in-time collisions.

**(8b) the draw**, `common/pileupDraw_cfg.py` (the MixingModule of `python/genJetPileup_cff.py` with `type =
poisson`, `averageNumber = PU`, `minBunch`/`maxBunch` from `BUNCHES`, `skipEvents = 0`, the `mix` engine seeded with
`MIXSEED` = 12345 and `MixMaxRng` like every cmsDriver configuration, `IOMC/RandomEngine/python/IOMC_cff.py`; only
`CrossingFramePlaybackInfoNew` is written), then `common/pileupDraw.py` (FWLite: `getNumberOfEvents(bx, 0)` and
`getEventId(offset)` of the record) for the table:

```bash
cd 08_PU/out && cmsRun ../../common/pileupDraw_cfg.py inputFiles=file:../../07_GenJet/out/step2_GENJET_07_GenJet.root \
  pileupFiles=file:step1_GEN_MinBias_08_PU.root eventsToProcess=1:1:1 nPileup=60 minBunch=-12 maxBunch=3 mixSeed=12345 \
  outputFile=draw_08_PU.root > draw_08_PU.log 2>&1
python3 ../../common/pileupDraw.py draw_08_PU.root --bunches=-12,3 --library 1000 > draw_08_PU.txt
cd ../..
```

The draw does not depend on the detector or on the library content: `MixingModule::produce` calls
`PileUp::CalculatePileup` (`SimGeneral/MixingModule/plugins/MixingModule.cc` 445), which draws the number of collisions
of every crossing in order from `minBunch` to `maxBunch` (`Mixing/Base/src/PileUp.cc` 456-479; `type = poisson`
without `manage_OOT`: `poissonDistribution(streamID)->fire()`, line 479), before any event is read (`readPileUp`,
`MixingModule.cc` 541). Validated against chapter 10:
`minBunch=-1 maxBunch=1` gives `-1:71[1-71] 0:68[72-139] 1:54[140-193]`, the `events per bx: -1:71 0:68 1:54` of
`test/10_SIM/out/simHits_mixed_10_SIM.log` (the SimHit mixing of cmsDriver `--pileup AVE_60_BX_25ns,{'N': 60, 'B': (-1, 1)}`).
The script reads the last line, `skip 746 intime 64 total 995`, and compares it with `SKIP` and `NPU`.

**(8b) one collision drawn**, `common/genPartAnalyzer_cfg.py` on library event 747 (`SKIP + 1`), the five
generator pages of chapters 01-06:

```bash
cd 08_PU/out && cmsRun ../../common/genPartAnalyzer_cfg.py inputFiles=file:step1_GEN_MinBias_08_PU.root \
  outputFile=genPartAnalyzer_MinBias_08_PU.root maxEvents=1 eventsToProcess=1:1:747 maxStage=5 \
  pdfFile=genPartAnalyzer_MinBias_08_PU.pdf \
  sampleLabel="chapter 08_PU (8b): one minimum-bias collision|library event 747 of 1000|SoftQCD inelastic, CP5, 13.6 TeV|no hard process" \
  > genPartAnalyzer_MinBias_08_PU.log 2>&1
cd ../..
```

Produces `genPartAnalyzer_MinBias_08_PU.pdf` (5 pages: hard process, parton shower, MPI / remnants,
hadronisation, hadron decays; page 5 is figure 8.1 of the chapter, `\includegraphics[page=5]`), the
`.root` with the eta-phi histograms, and the `.log` with the particle count per stage.

**(8a+8b) the overlay**, `common/genJetPileup_cfg.py` (the MixingModule on the HepMC record,
`python/genJetPileup_cff.py`, then the anti-kT producers and the `GenJetAnalyzer`):

```bash
cd 08_PU/out && cmsRun ../../common/genJetPileup_cfg.py inputFiles=file:../../07_GenJet/out/step2_GENJET_07_GenJet.root \
  pileupFiles=file:step1_GEN_MinBias_08_PU.root eventsToProcess=1:1:1 nPileup=64 skipPileup=746 pileupType=fixed \
  pileupSeed=20243 jetPtMin=3 labelPtMin=30 jets=ak4GenJetsPU outputFile=genJetPileup_08_PU.root \
  edmOutput=step2_GENJETPU_08_PU.root pdfFile=genJetAnalyzer_08_PU.pdf maxEvents=-1 \
  sampleLabel="chapter 08_PU (8a+8b): signal + pileup|+ 64 minimum-bias collisions in time|library events 747-810 (bx 0)|bbbar pTHat > 100 GeV, 13.6 TeV|jet pT > 3 GeV, drawn > 30 GeV" \
  > genJetPileup_08_PU.log 2>&1
cd ../..
```

`genJetPileup_cfg.py`, part by part: `RandomNumberGeneratorService.mix` (the MixingModule needs an engine
even with `type = fixed`); the source is the signal file restricted to `eventsToProcess`; `process.load
('JitJetSW.Journey.genJetPileup_cff')` brings `mix` (a `MixingModule` with `digitizers = cms.PSet()`,
`minBunch = maxBunch = 0`, `bunchspace = 25`, `mixObjects = mixHepMC` only, `input = EmbeddedRootSource` with
`sequential = True`), whose `fileNames`, `type`, `averageNumber` and `skipEvents` are set from the
`pileupFiles`, `pileupType`, `nPileup` and `skipPileup` arguments; `genParticlesPU` (`GenParticleProducer`
with `useCrossingFrame = True`, `mix = 'mix'`); `genParticlesForJetsPU` / `NoNuPU` (the status 1 selectors
of `RecoJets/Configuration/GenJetParticles_cff.py` on `genParticlesPU`); `ak4GenJetsPU`, `ak8GenJetsPU`,
`ak4GenJetsNoNuPU`, `ak8GenJetsNoNuPU` (the producers of `RecoJets/JetProducers`, `jetPtMin` from the
argument); and `genJetAnalyzer` (`GenJetAnalyzer` on `genParticlesPU` with `signalCollisionId = 0`, one page
per collection in `jets`). Produces `genJetAnalyzer_08_PU.pdf` (one page, the overlay; until 14 November 2026
the 8 pages of chapter 07 were put in front of it with `pdfunite`), `genJetPileup_08_PU.root` (the jet histograms,
`TFileService`), `genJetPileup_08_PU.log` (every jet with pT, eta, phi, mass, constituents and pileup pT, the b / bbar
matching; section 4), the EDM file `step2_GENJETPU_08_PU.root` (section 2); `genJetAnalyzer_08_PU.pdf` is figure 8.2 of the chapter.

**the ledger**, `common/genLedger.py --pu` on the EDM file:

```bash
python3 common/genLedger.py 08_PU/out/step2_GENJETPU_08_PU.root --event 1:1:1 --pu --jets ak4GenJetsPU > 08_PU/out/ledger_08_PU.txt
echo >> 08_PU/out/ledger_08_PU.txt
python3 common/genLedger.py 08_PU/out/step1_GEN_MinBias_08_PU.root --events 1:1:1-1:1:746,1:1:811-1:1:995 \
  --label "out-of-time pileup (931 collisions, bx -12..-1 and 1..3)" >> 08_PU/out/ledger_08_PU.txt
```

Prints the record by status group and the final state by kind and region, each for the signal
(`collisionId` 0), the in-time pileup (`collisionId` > 0) and the sum, and the `ak4GenJetsPU` summary; then, with
`--events` (since 16 November 2026), the final state of the out-of-time pileup summed over the library events of
the draw outside bunch crossing 0 (1 to `SKIP`, `SKIP + NPU + 1` to the total of `draw_08_PU.txt`), read from the
library file itself: it is never overlaid (section 4).

## 4. The tables

### 4.1 The Pythia8 process table and the cross sections (`step1_GEN_MinBias_08_PU.log`, `gencheck_08_PU.log`)

```
 | We collide p+ with p+ at a CM energy of 1.360e+04 GeV            |
 *-------  PYTHIA Event and Cross Section Statistics  -------------------------------------------------------------*
 | Subprocess                                    Code |            Number of events       |      sigma +- delta    |
 |                                                    |       Tried   Selected   Accepted |     (estimated) (mb)   |
 | non-diffractive                                101 |         722        722        722 |   5.542e+01  0.000e+00 |
 | A B -> X B single diffractive                  103 |          74         74         74 |   6.432e+00  2.400e-08 |
 | A B -> A X single diffractive                  104 |          87         87         87 |   6.432e+00  2.858e-08 |
 | A B -> X X double diffractive                  105 |         105        105        105 |   8.848e+00  0.000e+00 |
 | A B -> A X B central diffractive               106 |          12         12         12 |   1.285e+00  6.083e-09 |
 | sum                                                |        1000       1000       1000 |   7.842e+01  3.781e-08 |
Before Filter: total cross section = 7.842e+10 +- 3.781e+01 pb
Filter efficiency (event-level)= (1000) / (1000) = 1.000e+00 +- 0.000e+00    [TO BE USED IN MCM]
```

Derived: sigma_inel = 100.309 - 21.89 = 78.419 mb (the CP5 `SigmaTotal` settings, `PythiaCP5Settings_cfi.py`);
shares ND 55.42 / 78.42 = 70.7 %, SD 12.864 / 78.42 = 16.4 %, DD 8.848 / 78.42 = 11.3 %, CD 1.285 / 78.42 = 1.6 %;
expected in 1000 events: 706.7, 164.0, 112.8, 16.4; pulls (n - expected) / sqrt(expected): ND +0.57, SD -0.24, DD -0.74,
CD -1.08. The first 250 events alone (the library of chapters 10-13: 183 / 45 / 19 / 3 against 176.7 / 41.0 / 28.2 / 4.1)
had a DD deficit of (28.2 - 19) / sqrt(28.2) = 1.7 sigma.

```
 |                   sigmaNonDiffractive =    55.42 mb              |
 |    pT0 =  1.44 gives sigmaInteraction =   492.18 mb: accepted    |
```

pT0 = pT0Ref x (E_cm / 7 TeV)^ecmPow = 1.41 x (13.6 / 7)^0.03344 = 1.44 GeV; 492.18 / 55.42 = 8.9 interactions
per ND collision on average. Pythia version: 8.311; PDF: NNPDF31_nnlo_as_0118 (LHAPDF ID 303600), LHAPDF 6.4.0.
Pythia warnings (the last "Error and Warning Messages Statistics" of the log): 1 `BeamRemnants::setKinematics`
kinematics failure (retried), 169 `SimpleSpaceShower::pT2nextQCD: weight above unity`, 124 `SimpleSpaceShower::pT2nextQCD:
small daughter PDF`, 3 `MultipartonInteractions::init: maximum increased`, 2 `SimpleTimeShower::init: pTmin too low`,
1 `Pythia::check: energy-momentum not quite conserved`.

### 4.2 The configuration checks (`gencheck_08_PU.log`)

```
process = cms.Process('GEN',Run3_2025)
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('IOMC.EventVertexGenerators.VtxSmearedRealistic_cfi')
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase1_2025_realistic', '')
process.generator.comEnergy = cms.double(13600.0)
process.RandomNumberGeneratorService.generator.initialSeed = 20241
process.RandomNumberGeneratorService.VtxSmeared.initialSeed = 20242
== events in the file
1000 events
```

### 4.3 The catalogue summary (`genstats_08_PU.txt`, lines 1003-1023)

```
== 1000 events, beam energy 6800 GeV
process shares: ND 722 (72.2%), SD 161 (16.1%), DD 105 (10.5%), CD 12 (1.2%)
interactions mean    6.59  median    3.00  min    1.00  max   45.00
stable       mean  187.68  median  119.00  min    3.00  max  857.00
charged      mean   86.59  median   56.00  min    2.00  max  378.00
central      mean   20.36  median    9.00  min    0.00  max  167.00
|eta|>5.5    mean   38.25  median   35.00  min    2.00  max   93.00
lead pT      mean    2.21  median    1.72  min    0.16  max   18.99
sum pT       mean   76.45  median   41.25  min    0.33  max  498.28
jets>10      mean    0.15  median    0.00  min    0.00  max    6.00
PPS protons  mean    0.36  median    0.00  min    0.00  max    2.00
per process (events, mean interactions, stable, charged, central, lead pT, sum pT):
  ND  722    8.5  236.1  108.8  26.3  2.58   97.7
  SD  161    2.1   69.6   32.7   6.6  1.40   25.4
  DD  105    1.3   54.6   24.9   2.8  1.12   16.8
  CD   12    1.2   24.3   12.7   2.2  0.71    6.9
stable particles by species: gamma 45.7%, pi+- 37.3%, K+- 4.8%, p 2.8%, n 2.5%, K0L 2.4%, K0S 2.3%, Lambda 0.9%, e 0.6%, Sigma+ 0.3%, Sigma- 0.3%, Xi- 0.1%
|eta| of the stable particles, per unit (last bin >= 10): 26935 28804 29894 28086 24909 20449 13866 7961 4364 1752 664
pT of the stable particles (<0.2, 0.2-0.5, 0.5-1, 1-2, 2-5, >5 GeV): 70499 67692 35172 11781 2463 77
ND: mean stable particles against the number of interactions: 1: 87 (180 ev), 2: 113 (83 ev), 3: 143 (45 ev), 4: 161 (45 ev), 5: 183 (39 ev), 6: 205 (25 ev), 7: 251 (27 ev), 8: 245 (14 ev), 9: 245 (23 ev), 10: 282 (19 ev), 11: 294 (23 ev), 12: 346 (13 ev), 13: 310 (10 ev), 14: 384 (20 ev), 15: 363 (21 ev), 16: 385 (8 ev), 17: 439 (7 ev), 18: 402 (14 ev), 19: 456 (9 ev), 20: 441 (7 ev), 21: 516 (7 ev), 22: 459 (8 ev), 23: 495 (5 ev), 24: 521 (10 ev), 25: 511 (11 ev), 26: 533 (6 ev), 27: 588 (10 ev), 28: 566 (3 ev), 29: 563 (3 ev), 30: 588 (3 ev), 31: 562 (5 ev), 32: 588 (7 ev), 33: 536 (2 ev), 34: 712 (2 ev), 36: 555 (2 ev), 37: 634 (1 ev), 39: 692 (3 ev), 42: 826 (1 ev), 45: 805 (1 ev)
SD: diffractive mass M_X median 242.9 GeV (min 1.2, max 7686), xi = 1 - E_p/E_beam median 3.2e-04 (min 3.4e-09, max 0.32)
```

Derived from the per-event lines (columns z[cm], t[ns], jet10) and, at full precision, from the `xyz0` and `t0`
products of `genParticles` (FWLite): vertex z mean 0.378 cm, median 0.369, r.m.s. 3.566 cm, min -12.42, max 12.86;
t mean 0.0006 ns, median -0.0059, r.m.s. 0.120 ns, min -0.392, max 0.380 (the beam-spot centre of the payload is
z0 = 0.416 cm; the first 250 vertices had mean 0.08 cm, 1.5 sigma of their mean below it);
85 of the 1000 collisions have a jet above 10 GeV; total stable particles 1000 x 187.684 = 187 684, of which
70499 + 67692 = 138 191 below 0.5 GeV and 77 above 5 GeV; beyond |eta| = 5.5: 38.25 / 187.68 = 20 %. Largest collision:
event 379 (ND, 27 interactions, 857 stable); most interactions: event 484 (ND, 45, 805 stable); hardest particle:
event 985 (18.99 GeV).

### 4.4 The draw (`draw_08_PU.txt`, in full) and the in-time collisions, library events 747-810 (`genstats_08_PU.txt`, lines 748-811)

```
== the draw for signal event 1:1:1: bunch crossings -12 .. 3
  bx  t [ns] collisions  library events
 -12    -300         71  1-71
 -11    -275         68  72-139
 -10    -250         54  140-193
  -9    -225         59  194-252
  -8    -200         52  253-304
  -7    -175         61  305-365
  -6    -150         76  366-441
  -5    -125         59  442-500
  -4    -100         65  501-565
  -3     -75         59  566-624
  -2     -50         68  625-692
  -1     -25         54  693-746
   0       0         64  747-810
   1      25         63  811-873
   2      50         62  874-935
   3      75         60  936-995
early (bx -12 .. -1): 746   in time (bx 0): 64, library events 747-810   late (bx 1 .. 3): 185   all: 995
library: 1000 events, every collision used at most once
skip 746 intime 64 total 995
```

The same job with `minBunch=-1 maxBunch=1` (validation, not kept): `-1:71[1-71] 0:68[72-139] 1:54[140-193]`, total 193,
the draw of chapter 10; the first three crossings of bx -12..3 get the same 71, 68, 54 (the same first three
Poisson numbers of the engine). With the 2024 settings (`auto:phase1_2024_realistic`, `Run3_2024`) the draw is
identical (section 4.9).

```
event proc nInt stable  charg  centr |eta|>5.5  leadpT   sumpT jet10  PPS   z[cm]   t[ns]
  747   SD    1     37     20      0     16    1.08    13.9     0    1    2.16  -0.075
  748   ND    2     91     44      5     31    1.15    32.3     0    0    0.21   0.115
  749   ND    2    146     70     10     42    2.69    50.9     0    0    7.61   0.270
  750   SD    4    161     88      9     25    1.91    60.0     0    1    3.21   0.058
  751   DD    1     22     10      0     17    0.88     7.2     0    0   -1.45  -0.147
  752   ND    1     58     24      4     16    1.16    18.3     0    0   -2.54  -0.025
  753   DD    1     69     38      8     19    1.46    26.8     0    0    4.97   0.155
  754   ND    4    158     78     27     26    2.50    69.9     0    0    5.81  -0.080
  755   ND    3    215    106     13     43    1.82    73.0     0    1    4.84  -0.112
  756   DD    1     50     26      1     30    1.88    15.1     0    0    6.28  -0.166
  757   DD    1     11      6      0     11    0.47     2.9     0    0    8.33   0.090
  758   ND    1     97     42      7     15    1.42    35.9     0    0    2.02  -0.003
  759   ND   31    544    246     72     90    3.54   240.6     0    0    7.31  -0.090
  760   ND   42    826    378    167     60    6.19   498.3     6    0    2.25   0.243
  761   ND    2     69     28      4     17    1.34    23.4     0    0   -2.09   0.015
  762   ND    1    104     58      5     36    1.08    36.6     0    0    1.96   0.046
  763   ND    1    110     46      5     37    1.13    33.0     0    0    1.04  -0.127
  764   ND    3    343    156     80     25   14.34   225.5     3    0    2.56   0.125
  765   ND    4    133     66     17     35    2.15    52.2     0    0    6.04  -0.162
  766   ND   12    414    214     56     53    2.00   148.7     0    0   -1.28   0.056
  767   SD    1     32     16      4      9    2.31    11.8     0    1    2.35   0.077
  768   DD    1     49     20      2     30    0.64    12.4     0    1   -0.38  -0.124
  769   ND    5    201    110     29     29    2.82    84.1     0    0   -2.30   0.127
  770   ND    8    272    134     36     50    3.31   110.0     0    0    2.74   0.196
  771   ND    1    120     50      6     33    1.82    39.3     0    0   -0.16   0.004
  772   ND   26    649    280    109     62    4.33   331.4     1    0   -1.83  -0.280
  773   DD    1     59     32      4     23    0.76    17.0     0    0   -2.71  -0.081
  774   SD    1      4      4      0      4    0.35     0.7     0    2   -0.33  -0.186
  775   DD    1    106     44     10     34    1.15    29.8     0    0   -0.86  -0.185
  776   ND    7    229    102     22     42    1.99    82.0     0    0    1.55  -0.042
  777   ND    3    112     58      8     40    1.19    37.5     0    0    7.23  -0.036
  778   ND    6    296    134     21     50    2.56   102.4     0    0    4.18  -0.196
  779   ND    4    168     92     17     35    1.82    62.4     0    0   -1.84   0.163
  780   ND    5    135     62     15     43    2.25    60.0     0    1    3.05  -0.019
  781   ND   21    555    242     84     66    3.57   248.9     0    0   -3.62   0.040
  782   DD    1     20      8      0     17    0.87     6.4     0    0    0.51  -0.160
  783   SD    1     27     14      3      5    1.68     8.0     0    1   -1.85   0.248
  784   DD    1     56     30      3     35    0.80    15.6     0    0   -2.68   0.260
  785   DD    1     50     20      5     23    0.93    14.8     0    0   -2.64   0.170
  786   SD    2     63     28      6     11    0.96    17.9     0    1   -5.60  -0.049
  787   ND    1     72     34      1     32    0.81    18.7     0    0    4.59   0.130
  788   ND    3    120     54      8     48    2.39    42.0     0    0   -0.18   0.212
  789   ND    4     94     50     12     29    2.09    41.9     0    0   -3.67   0.123
  790   SD    2    105     66      9     23    1.73    42.4     0    1   -1.22   0.054
  791   SD    6    181     84     16     37    3.51    61.5     0    1    0.86  -0.075
  792   SD    3     94     28     12     23    2.67    36.6     0    1    0.66   0.002
  793   ND    8    232    104     15     42    1.75    71.6     0    0    1.63  -0.179
  794   ND   10    261    138     20     49    2.05    88.5     0    0   -6.52   0.051
  795   ND    5    157     76     26     37    2.72    70.3     0    1   -1.52   0.151
  796   ND    1     82     40      3     26    0.88    21.3     0    0    2.89   0.262
  797   ND   14    329    156     35     68    3.17   124.8     0    0   -2.37  -0.079
  798   ND   14    480    248     61     70    8.51   225.6     2    0   -3.54  -0.027
  799   ND    5    159     70     18     30    3.44    66.9     0    0   -2.97   0.054
  800   SD    4    155     62     15     21    1.49    56.1     0    1   -1.19  -0.267
  801   SD    1     24     18      1     13    1.08     8.3     0    1    2.01  -0.031
  802   ND   13    239    114     19     35    2.22    87.6     0    0   -2.65  -0.069
  803   ND   14    340    166     57     41    3.84   146.4     0    0    2.87  -0.170
  804   ND    5    126     58      9     27    1.52    41.9     0    1   -4.85  -0.007
  805   ND    1     94     40      5     29    1.41    29.4     0    0    1.31   0.016
  806   SD    1     56     30      3     17    2.39    30.6     0    1    2.80   0.050
  807   ND    3    175     84     11     39    1.51    58.3     0    0   -1.44   0.113
  808   DD    1     51     24      0     36    0.74    13.0     0    0   -0.04   0.012
  809   ND    2     45     24      3     22    0.77    13.4     0    0    4.13  -0.020
  810   ND   14    361    168     39     74    3.85   152.9     0    0   -3.24  -0.173
```

Sums over the 64 lines: 41 ND, 12 SD, 11 DD, 0 CD; 350 interactions; 10 893 stable particles (mean 170.2,
median 116 = (112 + 120) / 2, min 4 = event 774 (SD), max 826 = event 760 (ND, 42 interactions)); 5160 charged;
1272 central; 12 jets above 10 GeV. For comparison, the early crossings (events 1-746) have 142 212 stable
particles (190.6 per collision), the late ones (811-995) 34 054 (184.1).

### 4.5 The drawn collision, library event 747 (`genPartAnalyzer_MinBias_08_PU.log`)

```
== GenPartAnalyzer run:lumi:event 1:1:747 nGenParticles=74 nStatus1=37 hard b idx=hard bbar idx=
  stage 1 (hardProcess): nParticles=2 | b: none | #bar{b}: none
  stage 2 (partonShower): nParticles=2
  stage 3 (mpiBeamRemnants): nParticles=7
  stage 4 (hadronisation): nParticles=24
  stage 5 (hadronDecays): nParticles=37
```

Catalogue line 747: SD, 1 interaction, 37 stable, 20 charged, 0 central, 16 beyond |eta| = 5.5, hardest particle
1.08 GeV (a K+-), sum pT 13.9 GeV, no jet above 10 GeV, 1 proton at the PPS, z = 2.16 cm, t = -0.075 ns.
`GenEventInfoProduct::signalProcessID()` = 103 (A B -> X B). The record (`genParticles`, FWLite): [2] the diffractive
system pdg 9902210, mass 76.89 GeV, eta 11.97, E 6800.2 GeV; [3] the surviving proton, status 1, pT 0.086 GeV,
eta -11.97, E 6799.8 GeV (xi = 3e-5); [5] the Pomeron (pdg 990); [6], [7] the incoming gluons (status 21) of the
proton and of the Pomeron; [8], [9] the outgoing gluons (status 23), pT 0.329 and 0.398 GeV: the two particles of the
hard-process page. `minBiasGenStats.py --event 747`: the 37 stable particles have eta between 0.09 and 9.27 except the
proton at -11.97; 13 have |eta| > 6 (drawn on the border of page 5 of `genPartAnalyzer_MinBias_08_PU.pdf`).

### 4.6 The overlay (`genJetPileup_08_PU.log`, first lines and the 12 leading jets of 261)

```
== GenJetAnalyzer run:lumi:event 1:1:1 nGenParticles=30295 nStatus1=11527 (signal 634 + pileup 10893 from 64 collisions) | hard b: [5] pt=113.221 -> last copy [824] status=71 pt=91.7437 eta=-1.56979 phi=-2.08372 | hard bbar: [4] pt=113.221 -> last copy [964] status=71 pt=116.356 eta=-3.74787 phi=1.01139
  ak4GenJetsPU (process GENJETPU, AntiKt R=0.4, jetPtMin=3 GeV, src=genParticlesForJetsPU): nJets=261 nClustered=10988/11527 status 1 particles (signal 626/634, pileup 10362/10893)
    b quark -> jet [0] pt=150.752 eta=-1.63804 phi=-2.07282 nConst=101 dR=0.0691127 pt(jet)/pt(quark)=1.64319 pileupPt=34.2175 (22.6978%)
    bbar quark -> jet [1] pt=144.688 eta=-3.77367 phi=1.00923 nConst=86 dR=0.0258912 pt(jet)/pt(quark)=1.24349 pileupPt=29.5083 (20.3945%)
    net b jet + bbar jet: |sum pT|=10.6807 (px=4.50565 py=-9.68382) scalar sum pT=295.44 dphi=3.08205
    [0] pt=150.752 eta=-1.63804 phi=-2.07282 mass=50.5254 nConst=101 pileupPt=34.2175 (22.6978%)
    [1] pt=144.688 eta=-3.77367 phi=1.00923 mass=42.8672 nConst=86 pileupPt=29.5083 (20.3945%)
    [2] pt=72.2467 eta=-2.20474 phi=1.24018 mass=39.3864 nConst=109 pileupPt=70.254 (97.2418%)
    [3] pt=68.613 eta=0.436581 phi=-2.83504 mass=26.6221 nConst=94 pileupPt=68.6471 (100.05%)
    [4] pt=60.6734 eta=1.39986 phi=2.45011 mass=37.7583 nConst=101 pileupPt=48.0892 (79.2592%)
    [5] pt=58.1092 eta=-2.57512 phi=-3.09456 mass=29.6635 nConst=91 pileupPt=56.0586 (96.4712%)
    [6] pt=55.494 eta=0.772405 phi=-0.34326 mass=33.166 nConst=88 pileupPt=54.7552 (98.6688%)
    [7] pt=46.3936 eta=1.20398 phi=-3.12776 mass=31.72 nConst=90 pileupPt=46.3674 (99.9435%)
    [8] pt=45.0961 eta=2 phi=3.03012 mass=25.1327 nConst=78 pileupPt=42.0106 (93.1579%)
    [9] pt=43.3509 eta=2.31988 phi=-0.483933 mass=27.9429 nConst=90 pileupPt=39.2179 (90.4661%)
    [10] pt=42.9907 eta=-0.393213 phi=-2.07452 mass=23.4205 nConst=90 pileupPt=42.9312 (99.8616%)
    [11] pt=42.4741 eta=-2.46808 phi=1.81364 mass=28.4894 nConst=83 pileupPt=38.9951 (91.8093%)
```

The signal alone, for comparison (`test/07_GenJet/out/genJetAnalyzer_07_GenJet.log`, `ak4GenJets`):

```
  ak4GenJets (process GENJET, AntiKt R=0.4, jetPtMin=3 GeV, src=genParticlesForJets): nJets=27 nClustered=239/634 status 1 particles
    b quark -> jet [0] pt=117.788 eta=-1.58908 phi=-2.08403 nConst=24 dR=0.0192893 pt(jet)/pt(quark)=1.28389
    bbar quark -> jet [1] pt=115.944 eta=-3.74635 phi=1.00816 nConst=18 dR=0.00357334 pt(jet)/pt(quark)=0.996452
    net b jet + bbar jet: |sum pT|=6.06053 (px=4.01277 py=-4.54178) scalar sum pT=233.732 dphi=3.09219
    [0] pt=117.788 eta=-1.58908 phi=-2.08403 mass=16.7186 nConst=24
    [1] pt=115.944 eta=-3.74635 phi=1.00816 mass=7.32812 nConst=18
    [2] pt=13.759 eta=1.14207 phi=2.42033 mass=5.67954 nConst=11
    [3] pt=13.3261 eta=1.73863 phi=1.84424 mass=6.28552 nConst=12
```

Derived from the two jet lists: jets above 10 / 20 / 30 / 50 GeV: 6 / 2 / 2 / 2 (signal) and 163 / 99 / 45 / 7
(overlay); dijet mass m(b jet + bbar jet) from (pT, eta, phi, mass): 386.8 GeV (signal, also printed on the AK4 page
of chapter 07 as "m = 386.8 GeV") and 505.2 GeV (overlay, printed on its page); the b jet (jet [0] in both) gained
150.752 - 117.788 = 33.0 GeV and the bbar jet (jet [1] in both) 144.688 - 115.944 = 28.7 GeV; the jet axes moved by
Delta R = 0.050 (b) and 0.027 (bbar).
From `step2_GENJETPU_08_PU.root` with FWLite (`Events` / `Handle('std::vector<reco::GenJet>')` on
`ak4GenJetsPU`, then `jet.daughter(k).collisionId()` over `k < jet.numberOfDaughters()`, 0 = signal, inside the event
loop): jet [0] (b) 23 signal + 78 pileup constituents from 33 collisions, jet [1] (bbar) 18 + 68 from 30, jet [2]
7 + 102 from 35, jet [3] 1 + 93 from 33, jet [4] 9 + 92 from 34, jet [5] 6 + 85 from 35, jet [6] 3 + 85 from 36;
79 of the 261 jets have no signal constituent; 44 of the 45 jets above 30 GeV have at least one; 626 signal
particles are inside a jet. The bbar jet holds the 18 signal constituents of chapter 07; the b jet 23 of its 24:
the pi+ of pT 0.478 GeV, eta -1.407 is now a constituent of jet [251] (pT 3.4 GeV, eta -1.48, phi -2.46, 4
constituents). `PileupMixingContent` of `mix`: bunch crossings [0], collisions [64], true mean [64.0].

### 4.7 The ledger (`ledger_08_PU.txt`, in full but for the `members` lines of the cones)

```
== genLedger step2_GENJETPU_08_PU.root event 1:1:1: 30295 particles in genParticlesPU
record by status group (signal / pileup / all):
  hard process (21-29)            4 /    264 /    268
  MPI (31-39)                   124 /   1144 /   1268
  ISR (41-49)                   190 /   2505 /   2695
  FSR (51-59)                   192 /   2838 /   3030
  beam remnants (61-69)         233 /   3103 /   3336
  strings (71-79)               165 /   2369 /   2534
  decayed (status 2)            302 /   5127 /   5429
  final state (status 1)        634 /  10893 /  11527
hard process: incoming pdg -2 (E=14.8 GeV, x=0.0022) + pdg 2 (E=2652.9 GeV, x=0.3901) -> sqrt(s_hat) = 396.1 GeV
  outgoing pdg -5: pT=113.2 eta=-3.75 phi=0.99 E=2415.7 GeV
  outgoing pdg 5: pT=113.2 eta=-1.44 phi=-2.15 E=252.1 GeV
final state (status 1) of the signal: n=634 sumE=13600.0 GeV sumPt=528.9 GeV
  by kind                 n    E[GeV]  pT[GeV]
  charged hadron        303    7167.7    259.1
  neutral hadron         56    3449.9     84.7
  photon                273    2875.9    142.7
  electron                1      10.9      5.0
  neutrino                1      95.5     37.5
  by region               n    E[GeV]  pT[GeV]     E_nu
  |eta|<2.5             316     733.8    278.7     95.5
  2.5-3.0                55     193.1     24.6      0.0
  3.0-5.2               189    5019.6    197.4      0.0
  5.2-5.5                16     836.4      7.8      0.0
  >5.5                   58    6817.1     20.3      0.0
  visible (|eta|<5.2, no neutrinos): n=559 E=5851.0 GeV pT=463.4 GeV; neutrinos: n=1 E=95.5 GeV pT=37.5 GeV
  cone Delta R < 0.4 around the last copy of each outgoing parton (status 1, no neutrinos):
    bbar axis [964] status 71 pT=116.4 eta=-3.75 phi=1.01: n=18 E=2457.4 GeV vector pT=115.9 GeV scalar pT=116.0 GeV
    b    axis [824] status 71 pT=91.7 eta=-1.57 phi=-2.08: n=21 E=200.6 GeV vector pT=79.1 GeV scalar pT=79.5 GeV
final state (status 1) of the pileup: n=10893 sumE=870400.0 GeV sumPt=4507.4 GeV
  by kind                 n    E[GeV]  pT[GeV]
  charged hadron       5081  516149.1   2697.2
  neutral hadron        880  207301.9    587.4
  photon               4852  145397.3   1210.6
  electron               78    1171.7     11.4
  muon                    1      89.3      0.2
  neutrino                1     290.6      0.5
  by region               n    E[GeV]  pT[GeV]     E_nu
  |eta|<2.5            4252    4793.6   1922.1      0.0
  2.5-3.0               911    3216.1    404.4      0.0
  3.0-5.2              3230   44254.3   1320.1      0.0
  5.2-5.5               347   13646.3    129.7      0.0
  >5.5                 2153  804489.7    731.0    290.6
  visible (|eta|<5.2, no neutrinos): n=8393 E=52264.0 GeV pT=3646.6 GeV; neutrinos: n=1 E=290.6 GeV pT=0.5 GeV
  cone Delta R < 0.4 around the last copy of each outgoing parton (status 1, no neutrinos):
    bbar axis [964] status 71 pT=116.4 eta=-3.75 phi=1.01: n=75 E=668.0 GeV vector pT=30.9 GeV scalar pT=31.8 GeV
    b    axis [824] status 71 pT=91.7 eta=-1.57 phi=-2.08: n=66 E=73.2 GeV vector pT=27.5 GeV scalar pT=28.2 GeV
final state (status 1) of the all: n=11527 sumE=884000.0 GeV sumPt=5036.3 GeV
  by kind                 n    E[GeV]  pT[GeV]
  charged hadron       5384  523316.9   2956.4
  neutral hadron        936  210751.8    672.1
  photon               5125  148273.2   1353.2
  electron               79    1182.7     16.4
  muon                    1      89.3      0.2
  neutrino                2     386.1     37.9
  by region               n    E[GeV]  pT[GeV]     E_nu
  |eta|<2.5            4568    5527.4   2200.9     95.5
  2.5-3.0               966    3409.2    429.1      0.0
  3.0-5.2              3419   49273.9   1517.5      0.0
  5.2-5.5               363   14482.6    137.5      0.0
  >5.5                 2211  811306.8    751.3    290.6
  visible (|eta|<5.2, no neutrinos): n=8952 E=58115.0 GeV pT=4110.0 GeV; neutrinos: n=2 E=386.1 GeV pT=37.9 GeV
  cone Delta R < 0.4 around the last copy of each outgoing parton (status 1, no neutrinos):
    bbar axis [964] status 71 pT=116.4 eta=-3.75 phi=1.01: n=93 E=3125.4 GeV vector pT=146.8 GeV scalar pT=147.8 GeV
    b    axis [824] status 71 pT=91.7 eta=-1.57 phi=-2.08: n=87 E=273.8 GeV vector pT=106.6 GeV scalar pT=107.6 GeV
jets ak4GenJetsPU: n=261 sumPt=4815.5 GeV sumE=511052.8 GeV constituents=10988
  [0] pT=150.8 eta=-1.64 phi=-2.07 mass=50.5 nConst=101
  [1] pT=144.7 eta=-3.77 phi=1.01 mass=42.9 nConst=86
  [2] pT=72.2 eta=-2.20 phi=1.24 mass=39.4 nConst=109
  [3] pT=68.6 eta=0.44 phi=-2.84 mass=26.6 nConst=94

== genLedger step1_GEN_MinBias_08_PU.root events 1:1:1-1:1:746,1:1:811-1:1:995: 931 events summed (genParticles)
final state (status 1) of the out-of-time pileup (931 collisions, bx -12..-1 and 1..3): n=176266 sumE=12661600.1 GeV sumPt=71747.8 GeV
  by kind                 n    E[GeV]  pT[GeV]
  charged hadron      80092 7800297.9  41710.2
  neutral hadron      14309 2373526.0   9908.2
  photon              80717 2467116.5  19893.3
  electron             1036   16157.7    147.3
  muon                   42    2992.7     35.1
  neutrino               70    1509.3     53.7
  by region               n    E[GeV]  pT[GeV]     E_nu
  |eta|<2.5           66309   73558.3  29013.2     54.3
  2.5-3.0             13954   48338.8   6056.2     27.6
  3.0-5.2             54120  781316.4  22581.6    475.8
  5.2-5.5              5949  242976.3   2310.8    370.2
  >5.5                35934 11515410.2  11785.9    581.4
  visible (|eta|<5.2, no neutrinos): n=134323 E=902655.8 GeV pT=57603.4 GeV; neutrinos: n=70 E=1509.3 GeV pT=53.7 GeV
```

In the chapter's ledger table the rows "electrons, muons" are electron + muon (pileup 78 + 1 = 79, E
1171.7 + 89.3 = 1261.0 GeV, pT 11.4 + 0.2 = 11.6 GeV; all 80, 1272.0, 16.6). Derived: pileup energy beyond
|eta| = 5.5: 804 490 / 870 400 = 92 %; pileup pT density in |eta| < 2.5: 1922.1 / (5 x 2 pi) = 61.2 GeV per
unit of eta-phi area; mean pT of a pileup particle there 1922.1 / 4252 = 0.45 GeV; an R = 0.4 cone (area
pi R^2 = 0.503) collects 30.8 GeV (the pileup inside the cones around the two partons of the ledger: 27.5 GeV for
the b, 30.9 GeV for the bbar; in the jets 34.2 and 29.5 GeV); the signal block equals
`test/07_GenJet/out/ledger_07_GenJet.txt` (diff). At PU 200 (x 200 / 64 = 3.1): 191 GeV per unit area, 96 GeV per cone.

The out-of-time block (since 16 November 2026; the table of the chapter gives n and pT only, without the E column):
931 collisions, 176 266 stable particles (189.3 per collision, against 10 893 / 64 = 170.2 in time), 71 747.8 GeV of
pT (77.1 per collision, against 70.4), 12 661 600 GeV = 931 x 13 600 GeV; "electrons, muons" 1036 + 42 = 1078 with
147.3 + 35.1 = 182.4 GeV of pT; 931 / 64 = 14.5 times as many collisions as in time. Not overlaid: the jets and the
sum column are signal + in-time pileup. Release files of the text: `RecoLocalCalo/EcalRecProducers/plugins/
EcalUncalibRecHitWorkerMultiFit.cc` line 736 (`activeBXs` default {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4});
`RecoLocalCalo/HcalRecProducers/python/HBHEMahiParameters_cfi.py` line 12 (`activeBXs = cms.vint32(-3, -2, -1, 0, 1,
2, 3, 4)`); `CommonTools/ParticleFlow/python/pfCHS_cff.py` (`pfCHS`, a `CandPtrSelector` with `fromPV(0)>0 || ...`);
`CommonTools/PileupAlgos/plugins/PuppiProducer.cc`.

### 4.8 The release files quoted in "Open problems for Phase-2" (`$CMSSW_RELEASE_BASE/src`)

```
Configuration/PyReleaseValidation/python/relval_steps.py (4617):
    elif 'Run4' in ds:
        PUDataSets[ds]={'-n':10,'--pileup':'AVE_200_BX_25ns','--pileup_input':'das:/RelValMinBias_14TeV/%s/GEN-SIM'%(name,)}
    (4611-4615: '2022', '2023', '2024', '2025' -> 'Run3_Flat55To75_PoissonOOTPU' with RelValMinBias_14TeV)
Configuration/StandardSequences/python/Mixing.py (184, 159, 124):
    addMixingScenario("AVE_200_BX_25ns",{'file': 'SimGeneral.MixingModule.mix_POISSON_average_cfi','BX':25, 'B': (-3,3), 'N': 200})
    addMixingScenario("AVE_50_BX_25ns",{'file': 'SimGeneral.MixingModule.mix_POISSON_average_cfi','BX':25, 'B': (-12,3), 'N': 50})
    addMixingScenario("Run3_Flat55To75_PoissonOOTPU",{'file': 'SimGeneral.MixingModule.mix_Run3_Flat55To75_PoissonOOTPU_cfi'})
SimGeneral/MixingModule/python/mix_POISSON_average_cfi.py (26) and mix_probFunction_25ns_PoissonOOTPU_cfi.py (42):
    sequential = cms.untracked.bool(False),      (this book: sequential = True; the overlay skipEvents = 746, the draw skipEvents = 0)
Configuration/StandardSequences/python/Mixing.py (139, 132): the profiles of 2025 and 2024
    "2025_LHC_Simulation_fill12p5h_levelling9h_nb2340_xSec80mb_il2p23" -> Run3_2025_LHC_Simulation_fill12p5h_levelling9h_nb2340_xSec80mb_il2p23_cfi
        probFunction over 0-99 collisions: mean 64.34, r.m.s. 11.70, median 65; header "A distribution for Run3 studies (2023)
        derived from LHC simulations", "Hadronic x-sec : 80mb", 2340 colliding bunches, minBunch -12, maxBunch 3
    "2024_25ns_RunIII2024Summer24_PoissonOOTPU" -> mix_2024_25ns_RunIII2024Summer24_PoissonOOTPU_cfi: mean 44.92, r.m.s. 9.53
    defineMixing(): 'N' -> process.mix.input.nbPileupEvents.averageNumber, 'BX' -> process.mix.bunchspace, 'B' -> process.mix.minBunch / maxBunch
SimGeneral/MixingModule/python/mix_POISSON_average_cfi.py: type = 'poisson', sequential = cms.untracked.bool(False), minBunch -5, maxBunch 3, bunchspace 450 (overridden by the scenario)
SimGeneral/MixingModule/python/mix_probFunction_25ns_PoissonOOTPU_cfi.py: minBunch = -12, maxBunch = 3, bunchspace = 25, type = 'probFunction', manage_OOT = True
SimGeneral/MixingModule/python/mix_Run3_Flat55To75_PoissonOOTPU_cfi.py: probValue 0 for 0-54, 1/21 = 0.047619 for 55-75
Configuration/Generator/python/MinBias_14TeV_pythia8_TuneCP5_cfi.py: comEnergy = 14000.0, crossSection = 71.39e+09, SoftQCD:inelastic = on
Configuration/Generator/python/MCTunes2017/PythiaCP5Settings_cfi.py: SigmaTotal:mode = 0, SigmaTotal:sigmaEl = 21.89, SigmaTotal:sigmaTot = 100.309, MultipartonInteractions:pT0Ref=1.41, ecmPow=0.03344
    (no SigmaDiffractive setting, nor in Pythia8CommonSettings_cfi.py); sigma_inel = 100.309 - 21.89 = 78.42 mb at every energy
Configuration/Generator/python/MCTunesRun3ECM13p6TeV/PythiaCP5Settings_cfi.py: identical to the 2017 file except SigmaTotal:sigmaEl = 22.08, SigmaTotal:sigmaTot = 101.037 (lines 23-24): 78.96 mb
    imported by 6 python files of the release (all 13p6TeV fragments with a filter); MCTunes2017.PythiaCP5Settings_cfi imported by 84 (82 in Configuration/Generator, 2 in GeneratorInterface); no 14 TeV directory
$PYTHIA8_BASE/share/Pythia8/xmldoc/TotalCrossSections.xml (Pythia 8.311, toolbox pythia8.xml): line 88-92 SigmaTotal:mode default 1, option 0 'Make your own choices (the "own model"), set as fixed values.'
    line 108-115: SigmaDiffractive:mode default 1 = 'The SaS parametrizations'
test/08_PU/out/step1_GEN_MinBias_08_PU.log (740, 786): sum 7.842e+01 mb (ND 55.42, SD 6.432 + 6.432, DD 8.848, CD 1.285); 'total cross section = 7.842e+10 +- 3.028e+02 pb'
Configuration/Generator/python/: MinBias_13TeV_pythia8_TuneCUETP8M1_cfi, MinBias_14TeV_pythia8_TuneCUETP8M1_cfi, MinBias_TuneZ2star_*_pythia6_cff, MinBias_13TeV_cfi / MinBias_cfi / MinBiasP0_cfi / PythiaMinBias_cfi (Pythia6), Hydjet_*_MinBias_* (no unfiltered 13.6 TeV CP5 minimum-bias fragment)
    CP5 at 13 TeV: MinBias_13TeV_pythia8_cff.py and SoftQCDinelastic_13TeV_pythia8_cff.py (PythiaCP5Settings_cfi, SoftQCD:inelastic = on, comEnergy = 13000.0)
    and SoftQCDnonDiffractive_13TeV_pythia8_cff.py (SoftQCD:nonDiffractive = on), used by the GEN validation steps of relval_steps.py
    (1697, 1704, 1706: genvalid('MinBias_13TeV_pythia8_cff'), genvalid('SoftQCDnonDiffractive_13TeV_pythia8_cff'), genvalid('SoftQCDinelastic_13TeV_pythia8_cff'))
    CP5 at 13.6 TeV, filtered: XiMinus_13p6TeV_SoftQCDInel_TuneCP5_cfi.py (PythiaFilter ParticleID 3312, MinPt 1.0, lines 26-33), OmegaMinus_13p6TeV_SoftQCDInel_TuneCP5_cfi.py
    (OmegaFilter), DStarToD0Pi_D0ToKsPiPi_inclusive_SoftQCD_TuneCP5_13p6TeV-pythia8-evtgen.py (SoftQCD:nonDiffractive with PTFilter on the c quark)
    no energy override for RelValMinBias_14TeV: neither 13600 nor comEnergy in relval_steps.py, relval_upgrade.py, upgradeWorkflowComponents.py
    (the only '13p6' entries there are the filtered fragments, upgradeWorkflowComponents.py 3397-3404); the fragment enters as 'MinBias_14TeV' (3296)
Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py (3171-3177, 'Run4D110'): 'Geom': 'ExtendedRun4D110', 'GT': 'auto:phase2_realistic_T33', 'Era': 'Phase2C17I13M9', 'ScenToRun': ['GenSimHLBeamSpot', ...]
    relval_upgrade.py (40-42): a 14TeV fragment runs 'GenSimHLBeamSpot14', whose '--beamspot' is 'DBrealisticHLLHC' (relval_steps.py 4674-4677; 'GenSimHLBeamSpot' 4665-4668 uses gt + '_13TeV');
    'Run4D110GenOnly' / 'Run4D110SimOnGen' (3206-3220) carry 'BeamSpot': 'DBrealisticHLLHC' explicitly
Configuration/AlCa/python/autoCondPhase2.py (162): 'phase2_realistic_T33' = 'phase2_realistic' + the T33 tracker tags (the SimBeamSpotHLLHCObjectsRcd tag comes from 150X_mcRun4_realistic_v1;
    conddb list 150X_mcRun4_realistic_v1: SimBeamSpotHLLHCObjectsRcd SimBeamSpotHLLHCObjects_z4p3cm_t193ns_14TeV_nominal_mc_v1);
    autoCondModifiers.py (128-143): the '_13TeV' variants add SimBeamSpotHLLHCObjects_z4p3cm_t193ns_13TeV_nominal_mc_v1
Configuration/AlCa/python/autoCond.py (111): 'phase2_realistic' : '150X_mcRun4_realistic_v1'; (95): 'phase1_2024_realistic' : '150X_mcRun3_2024_realistic_v2'; (107): 'phase1_2025_realistic' : '150X_mcRun3_2025_realistic_v1'
IOMC/EventVertexGenerators/python/VtxSmearedRealisticHLLHC_cfi.py: VtxSmeared = HLLHCEvtVtxGenerator.clone(src = "generator:unsmeared", readDB = True)   # SimBeamSpotHLLHCObjectsRcd
IOMC/EventVertexGenerators/python/VtxSmearedRealistic_cfi.py: VtxSmeared = BetafuncEvtVtxGenerator.clone(src = "generator:unsmeared", readDB = True)    # SimBeamSpotObjectsRcd
IOMC/EventVertexGenerators/python/VtxSmearedParameters_cfi.py (1100-1117), '# Parameters for HL-LHC operation at 13TeV' then HLLHCVtxSmearingParameters:
    MeanXIncm = MeanYIncm = MeanZIncm = 0, TimeOffsetInns = 0.0, EprotonInGeV = 6500.0, CrossingAngleInurad = 510.0, CrabFrequencyInMHz = 400.0, RF800 = False,
    BetaCrossingPlaneInm = 0.20, BetaSeparationPlaneInm = 0.20, HorizontalEmittance = 2.5e-06, VerticalEmittance = 2.05e-06, BunchLengthInm = 0.090,
    CrabbingAngleCrossingInurad = 380.0, CrabbingAngleSeparationInurad = 0.0
IOMC/EventVertexGenerators/python/VtxSmearedHLLHC14TeV_cfi.py: HLLHCVtxSmearingParameters.clone(EprotonInGeV = cms.double(7000))
Configuration/StandardSequences/python/VtxSmeared.py (4, 44, 45): 'DBrealisticHLLHC' -> VtxSmearedRealisticHLLHC_cfi, 'HLLHC' -> VtxSmearedHLLHC_cfi (6500 GeV), 'HLLHC14TeV' -> VtxSmearedHLLHC14TeV_cfi
IOMC/EventVertexGenerators/src/HLLHCEvtVtxGenerator.cc (131-132): z = (shoot() - 0.5) * 6.0 * fBunchLength; t = (shoot() - 0.5) * 6.0 * fBunchLength;  (accept-reject on intensity(x, y, z, t))
IOMC/EventVertexGenerators/src/BetafuncEvtVtxGenerator.cc (84, 97): tmp_sigz = RandGaussQ::shoot(engine, 0., fSigmaZ); tmp_sigt = RandGaussQ::shoot(engine, 0., fSigmaZ)
conddb --db pro list 150X_mcRun4_realistic_v1: SimBeamSpotHLLHCObjectsRcd  SimBeamSpotHLLHCObjects_z4p3cm_t193ns_14TeV_nominal_mc_v1
conddb --db pro list SimBeamSpotHLLHCObjects_z4p3cm_t193ns_14TeV_nominal_mc_v1: one IOV from run 1, inserted 2024-07-04 12:49:25, payload b8e3fc124707dcee90d5b68a50d9adfd5f8775ee
conddb --db pro dump b8e3fc124707dcee90d5b68a50d9adfd5f8775ee (SimBeamSpotHLLHCObjects):
    fMeanX = fMeanY = fMeanZ = 0, fEProton = 7000, fCrabFrequency = 400, fRF800 = 0, fCrossingAngle = 510, fCrabbingAngleCrossing = 380,
    fCrabbingAngleSeparation = 0, fBetaCrossingPlane = fBetaSeparationPlane = 0.20, fHorizontalEmittance = 2.5e-06, fVerticalEmittance = 2.05e-06,
    fBunchLength = 0.090, fTimeOffset = 0   (no time-spread parameter: 't193ns' is only in the tag name; 0.090 m / c = 300 ps)
RecoLocalFastTime/FTLRecProducers/python/mtdUncalibratedRecHits_cfi.py (25): ETL timeResolutionInNs = cms.string("0.0370"), # [ns]
    (11: BTL timeResolutionInNs = "0.308*pow(x,-0.4175)"; SimFastTiming/FastTimingCommon/python/mtdDigitizer_cfi.py 25: TestBeamMIPTimeRes, "0.048[ns]*sqrt(8000.)")
RecoVertex/Configuration/python/RecoVertex_phase2_timing_cff.py (5, 33): unsortedOfflinePrimaryVertices4D, algorithm = "DA2D_vect", assignment = dict(useTiming = True)
    (switched on by RecoVertex/Configuration/python/RecoVertex_cff.py 78: phase2_timing.toReplaceWith(vertexrecoTask, _phase2_tktiming_vertexrecoTask))
```

Library sizes: PU 200 x 7 crossings (-3..3) = 1400 minimum-bias events per signal event; Run 3 scenario
~65 x 16 crossings (-12..3) = 1040; the draw of this chapter (bunch crossings -12..3, mean 60, 960 on average)
995 for this event: 746 + 64 + 185 library events for the early crossings, bunch crossing 0 and the late ones;
1000 / 1400 = 70 % of one PU-200 event. The mixing of chapters 10 and 11 (bunch crossings -1..1): 193, 71 + 68 + 54.
In-time pedestal: 61.2 GeV per unit area (64 collisions) x 200 / 64 = 191 GeV; cone 30.8 x 200 / 64 = 96 GeV, against
the 117.8 GeV of the b jet of chapter 07 (82 %).

### 4.9 The 2024 and the 2025 settings (`conddb`, and the rerun)

The generator chapters 01-07 run with `YEAR=2024`, chapters 08-13 with `YEAR=2025` (`common/jitjet_common.sh`,
`jitjet_init`; 08-10 were moved first, 11-13 in a second step on the same day). `Configuration/AlCa/python/autoCond.py` (95, 107):

```
'phase1_2024_realistic' : '150X_mcRun3_2024_realistic_v2'
'phase1_2025_realistic' : '150X_mcRun3_2025_realistic_v1'
```

`conddb diff 150X_mcRun3_2024_realistic_v2 150X_mcRun3_2025_realistic_v1` (622 and 623 records): 35 rows differ,
34 tags plus `DeDxCalibrationRcd` (only in 2025): `BeamSpotObjectsRcd` and the two online beam spots (Early2024 ->
Winter25), 13 ECAL records (`EcalPedestalsRcd` `EcalPedestals_2025_at80fb_corrected_mc`, the laser corrections
`..._TL470fb_moy_2025_...`, the selective-readout settings `EcalSRSettings_2024_EBZS9.5_EEZS6.0_DCC2_mc` instead of
`..._2023_EBZS6.5_...`, the channel status and 9 trigger-primitive tables), 7 HCAL records (pedestals and widths,
response corrections, SiPM parameters, zero-suppression thresholds, PF cuts, TP channel parameters), 6 pixel records
(templates, generic errors, Lorentz angles incl. `SiPixelLorentzAngleSimRcd`), `SiStripBadChannelRcd`,
`TrackerAlignmentRcd`, `TrackerSurfaceDeformationRcd`, `JetCorrectionsRecord` (AK4PFPuppi: Winter24 in 2024,
Winter23 in 2025) and `L1TUtmTriggerMenuRcd` (`L1Menu_Collisions2024_v1_3_0_xml` -> `L1Menu_Collisions2025_v1_0_0_xml`).
None of them is read by a GEN job. Identical in both global tags (`conddb list`):

```
SimBeamSpotObjectsRcd   SimBeamSpot_Realistic25ns13p6TeVEarly2024CollisionVtxSmearingParameters_v1_mc
GeometryFileRcd         Extended   141DD4hepV2_Extended2024
MFGeometryFileRcd       160812     MFGeometry_160812
MagFieldConfigRcd       3.8T       MFConfig_160812_Run2_3_8T
```

The simulated beam spot (`conddb list` of the tag: one IOV from run 1, inserted 2024-08-01, payload
`2223f0e4e230c6c0e0e887bb366612aa500d5822`; `conddb dump`):

```
fX0 = 0.0284683  fY0 = -0.016576  fZ0 = 0.416038  (cm)   fSigmaZ = 3.6 cm   fbetastar = 30 cm
femittance = 5.725e-08 cm   fSigmaX = fSigmaY = -1 (from betastar and emittance)   fPhi = fAlpha = 0   fTimeOffset = 0
```

The era: `Configuration/Eras/python/Era_Run3_2025_cff.py`: `Run3_2025 = cms.ModifierChain(Run3_2024, run3_GEM_2025,
run3_SiPixel_2025, stage2L1Trigger_2025, run3_CSC_2025, ecal_cctiming)`; the five modifiers are used only in
`RecoLocalMuon/GEMRecHit`, `L1Trigger/CSCTriggerPrimitives`, `RecoLocalTracker/SiPixelClusterizer`,
`L1Trigger/L1TCaloLayer1`, `DQM/L1TMonitor` and `RecoLocalCalo/EcalRecProducers` (grep over the python of the release).

The rerun with the 2025 settings (12 November 2026) against the 2024 run (7 November 2026): `ledger_08_PU.txt`,
`genstats_08_PU.txt` (every collision: process, interactions, particles, PPS protons, vertex z and t), the Pythia8 statistics of
`step1_GEN_MinBias_08_PU.log`, `genPartAnalyzer_MinBias_08_PU.log` and `genJetPileup_08_PU.log` are identical line by
line; `gencheck_08_PU.log` differs only in the era and global-tag lines. Every number of the chapter holds for both years.

The 1000-collision library of 16 November 2026 was checked again: `step1_GEN_MinBias_08_PU_cfg.py` with the three
year lines changed (`Era_Run3_2024_cff`, `cms.Process('GEN',Run3_2024)`, `auto:phase1_2024_realistic`), run on the
same 1000 events: `genstats_08_PU.txt` (all 1000 catalogue lines and the summary), the draw `draw_08_PU.txt` and the
Pythia8 "Event and Cross Section Statistics" are identical to the 2025 ones (`diff`).
