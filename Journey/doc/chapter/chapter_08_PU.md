# Chapter 08, PU: technical companion

The companion of `doc/chapter/chapter_08_PU.tex` (Rule C of `Constitution.txt`): the commands of
`test/08_PU/run_08_PU.sh`, a walk-through of the configurations, and every number the chapter quotes, copied
from `test/08_PU/out/`. All commands are run from `JitJetSW/Journey/test` in an initialised CMSSW_15_0_5
(`cd CMSSW_15_0_5/src && cmsenv`); the chapter needs the output of chapter 07,
`test/07_GenJet/out/step2_GENJET_07_GenJet.root` and `genJetAnalyzer_07_GenJet.pdf` (Rule E).

```bash
./08_PU/run_08_PU.sh                 # everything below: the library, the catalogue, the drawn collision, the overlay, the ledger
RUN=0 ./08_PU/run_08_PU.sh           # only write step1_GEN_MinBias_08_PU_cfg.py
ANALYZE=0 ./08_PU/run_08_PU.sh       # the library and gencheck_08_PU.log only (no catalogue, no plots, no overlay)
PUSEED=777 ./08_PU/run_08_PU.sh      # another 250 collisions (seeds 777 / 778)
NPU=20 SKIP=0 ./08_PU/run_08_PU.sh   # overlay library events 1-20 instead of 72-139
PUTYPE=poisson ./08_PU/run_08_PU.sh  # draw the number of in-time collisions from Poisson(NPU) with seed 20243
JETS=ak4GenJetsPU,ak4GenJetsNoNuPU,ak8GenJetsPU ./08_PU/run_08_PU.sh   # three overlay pages (9, 10, 11)
```

Variables (defaults): `PUEVENTS=250 PUSEED=20241 PUTYPE=fixed JETS=ak4GenJetsPU JETPTMIN=3 LABELPTMIN=30`, plus
the common ones of `common/jitjet_common.sh` (`CONDITIONS=auto:phase1_2024_realistic ERA=Run3_2024
GEOMETRY=DB:Extended BEAMSPOT=DBrealistic COMENERGY=13600.0 EVENT=1:1:1 RUN=1 ANALYZE=1 PNG=1`, and the pileup
convention of the book `PU=60 BUNCHES=-1,1 SKIP=71 NPU=68`). `SKIP` and `NPU` are the library events used up
by bunch crossing -1 and the in-time collisions of bunch crossing 0 in the mixing of chapter 10 for this
signal event (MixingModule seed 12345, Poisson(60) per bunch crossing, the library read in sequence: events
1-71 in bx -1, 72-139 in bx 0, 140-193 in bx +1); chapter 10 checks its draw against them and chapter 11
replays it with the MixingModule playback.

## 1. The cmsDriver.py commands

One `cmsDriver.py` command, written by `run_driver` of `common/jitjet_common.sh` (its own output is
`out/step1_GEN_MinBias_08_PU_cfg.driver.log`); the command line is also the second line of the generated
configuration (`# with command line options: ...`):

```bash
cmsDriver.py MinBias_14TeV_pythia8_TuneCP5_cfi --conditions auto:phase1_2024_realistic --era Run3_2024 \
  --geometry DB:Extended --beamspot DBrealistic -n 250 -s GEN --datatier GEN --eventcontent FEVTDEBUG \
  --fileout file:step1_GEN_MinBias_08_PU.root \
  --customise_commands "process.generator.comEnergy = cms.double(13600.0)\nprocess.RandomNumberGeneratorService.generator.initialSeed = 20241\nprocess.RandomNumberGeneratorService.VtxSmeared.initialSeed = 20242" \
  --python_filename 08_PU/step1_GEN_MinBias_08_PU_cfg.py --no_exec
```

| argument | what it does |
|---|---|
| `MinBias_14TeV_pythia8_TuneCP5_cfi` | the generator fragment, `Configuration/Generator/python/MinBias_14TeV_pythia8_TuneCP5_cfi.py` of the release: `Pythia8ConcurrentGeneratorFilter` with `SoftQCD:inelastic = on`, the common settings block and the CP5 tune block, `comEnergy = 14000` |
| `--conditions auto:phase1_2024_realistic` | the global tag (`150X_mcRun3_2024_realistic_v2`, `Configuration/AlCa/python/autoCond.py`); in a GEN job it supplies the beam-spot payload (`SimBeamSpotObjectsRcd`) of the vertex smearing |
| `--era Run3_2024` | the era modifiers (`Configuration/Eras/python/Era_Run3_2024_cff.py`); in a GEN job they switch on the PPS proton transport (`LHCTransport`) |
| `--geometry DB:Extended` | not used by the generator; carried in the configuration so that the SIM step of chapter 09 inherits it |
| `--beamspot DBrealistic` | the vertex-smearing module: `IOMC/EventVertexGenerators/python/VtxSmearedRealistic_cfi.py`, a `BetafuncEvtVtxGenerator` with `readDB = True` |
| `-n 250` | events to generate (`PUEVENTS`); the filter accepts every event, so 250 are written |
| `-s GEN` | the step: the generator sequence `pgen` of `Configuration/StandardSequences/python/Generator_cff.py` |
| `--datatier GEN --eventcontent FEVTDEBUG` | the label of the output and the list of kept products (everything the GEN step makes) |
| `--fileout file:step1_GEN_MinBias_08_PU.root` | the output file, written in `out/` (`cmsRun` runs there) |
| `--customise_commands "..."` | Python lines appended verbatim to the configuration (a literal `\n` separates them): the energy of the signal and the two seeds (`PUSEED`, `PUSEED + 1`) |
| `--python_filename ... --no_exec` | the name of the configuration, and "write it, do not run it" (`run_driver` then runs `cmsRun` on it) |

The signal of chapters 01-07 used the same conditions, era, geometry, beam spot and energy with its own
fragment (`BBbar_14TeV_TuneCP5_cfi`) and the `cmsDriver.py` default seeds; the library must not reuse those
seeds, or library event k would get the vertex of signal event k.

## 2. The cmsRun on the python config

```bash
cd 08_PU/out && cmsRun ../step1_GEN_MinBias_08_PU_cfg.py > step1_GEN_MinBias_08_PU.log 2>&1 && cd ../..   # about 1 min
```

The generated configuration `08_PU/step1_GEN_MinBias_08_PU_cfg.py`, part by part:

- **process and era**: `process = cms.Process('GEN', Run3_2024)`; the standard includes (`Services_cff`,
  the particle table `pythiapdt_cfi`, `MessageLogger`, `EventContent_cff`, `mixNoPU_cfi` (no mixing in a
  GEN job), `GeometryRecoDB_cff`, `MagneticField_cff`, `Generator_cff`, `VtxSmearedRealistic_cfi`,
  `genFilterSummary_cff`, `EndOfProcess_cff`, `FrontierConditions_GlobalTag_cff`).
- **source and events**: `process.source = cms.Source("EmptySource")` (a generator job makes its own events,
  run 1, lumi 1, events 1 to 250); `process.maxEvents.input = 250`.
- **output**: `process.FEVTDEBUGoutput`, a `PoolOutputModule` with `SelectEvents = ['generation_step']` (only
  events that pass the generator filter) and `fileName = 'file:step1_GEN_MinBias_08_PU.root'`.
- **global tag**: `process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase1_2024_realistic', '')`.
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
  event to a vertex of the 2024 beam spot, in space and time), `generatorSmeared` (the same record under the
  label every later step reads), `genParticles` (the record as `reco::GenParticle`s with `xyz0`, `t0`),
  `genJetMET` (ak4 / ak8 jets with and without neutrinos, `genMetTrue`, `genMetCalo`), `LHCTransport` (the
  surviving protons carried to the PPS Roman pots).
- **the customisation lines** at the end: `process.generator.comEnergy = cms.double(13600.0)`,
  `process.RandomNumberGeneratorService.generator.initialSeed = 20241`,
  `process.RandomNumberGeneratorService.VtxSmeared.initialSeed = 20242`, then the early-deletion customisation
  `customiseEarlyDelete`.

The checks the script writes into `out/gencheck_08_PU.log`: the process, geometry, vertex-smearing and
global-tag lines of the configuration, the seeds, `edmFileUtil -e` (250 events), the Pythia8 statistics and
the `GenXsecAnalyzer` cross section (section 4).

**The output ROOT file** (`edmDumpEventContent file:08_PU/out/step1_GEN_MinBias_08_PU.root`), 31 products
per event, all from the process `GEN`; 11 MB for 250 events, 44 kB per event (`edmEventSize -v`: 29.0 kB the
HepMC record, 11.5 kB the `GenParticle`s, 0.8 kB the engine states):

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
PileupMixingContent                   "mix"                       ""         "GENJETPU"   bunch crossings [0], collisions [68], true mean [68.0]
vector<reco::GenParticle>             "genParticlesPU"            ""         "GENJETPU"   36 218 particles, collisionId 0 = signal, 1..68 = pileup
vector<int>, xyz0, t0                 "genParticlesPU"                       "GENJETPU"
edm::PtrVector<reco::Candidate>       "genParticlesForJetsPU", "genParticlesForJetsNoNuPU"   "GENJETPU"   the status 1 inputs (without neutrinos for NoNu)
vector<reco::GenJet>                  "ak4GenJetsPU", "ak4GenJetsNoNuPU", "ak8GenJetsPU", "ak8GenJetsNoNuPU"   "GENJETPU"   + rho, sigma, rhos, sigmas
```

## 3. The analyzer configs

Four steps after the GEN job, all run by the script (`ANALYZE=1`):

**(8b) the catalogue**, `common/minBiasGenStats.py` (FWLite, no `cmsRun`):

```bash
python3 common/minBiasGenStats.py 08_PU/out/step1_GEN_MinBias_08_PU.root > 08_PU/out/genstats_08_PU.txt 2> 08_PU/out/genstats_08_PU.log
python3 common/minBiasGenStats.py 08_PU/out/step1_GEN_MinBias_08_PU.root --event 72 132    # + the stable particles of events 72 and 132
```

One line per collision (process from `signal_process_id()` of the HepMC record, interactions = 1 + the
status-31 parton pairs, stable / charged / central (|eta| < 2.5, pT > 0.5 GeV) / |eta| > 5.5 particles, the
hardest particle, the scalar sum of pT, ak4 jets above 10 GeV, protons at the PPS, vertex z and t), then the
summary (process shares, means, species, |eta| and pT distributions, multiplicity against interactions, the
SD masses). Section 4 copies the summary and the lines of the in-time collisions.

**(8b) one collision drawn**, `common/genPartAnalyzer_cfg.py` on library event 72 (`SKIP + 1`), the five
generator pages of chapters 01-06:

```bash
cd 08_PU/out && cmsRun ../../common/genPartAnalyzer_cfg.py inputFiles=file:step1_GEN_MinBias_08_PU.root \
  outputFile=genPartAnalyzer_MinBias_08_PU.root maxEvents=1 eventsToProcess=1:1:72 maxStage=5 \
  pdfFile=genPartAnalyzer_MinBias_08_PU.pdf \
  sampleLabel="chapter 08_PU (8b): one minimum-bias collision|library event 72 of 250|SoftQCD inelastic, CP5, 13.6 TeV|no hard process" \
  > genPartAnalyzer_MinBias_08_PU.log 2>&1
pdftoppm -cropbox -r 110 -png genPartAnalyzer_MinBias_08_PU.pdf genPartAnalyzer_MinBias_08_PU_page; cd ../..
```

Produces `genPartAnalyzer_MinBias_08_PU.pdf` (5 pages: hard process, parton shower, MPI / remnants,
hadronisation, hadron decays), the PNG pages `_page-1..5.png` (page 5 is figure 8.1 of the chapter), the
`.root` with the eta-phi histograms, and the `.log` with the particle count per stage.

**(8a+8b) the overlay**, `common/genJetPileup_cfg.py` (the MixingModule on the HepMC record,
`python/genJetPileup_cff.py`, then the anti-kT producers and the `GenJetAnalyzer`):

```bash
cd 08_PU/out && cmsRun ../../common/genJetPileup_cfg.py inputFiles=file:../../07_GenJet/out/step2_GENJET_07_GenJet.root \
  pileupFiles=file:step1_GEN_MinBias_08_PU.root eventsToProcess=1:1:1 nPileup=68 skipPileup=71 pileupType=fixed \
  pileupSeed=20243 jetPtMin=3 labelPtMin=30 jets=ak4GenJetsPU outputFile=genJetPileup_08_PU.root \
  edmOutput=step2_GENJETPU_08_PU.root pdfFile=genJetPileup_08_PU.pdf maxEvents=-1 \
  sampleLabel="chapter 08_PU (8a+8b): signal + pileup|+ 68 minimum-bias collisions in time|library events 72-139 (bx 0)|bbbar pTHat > 100 GeV, 13.6 TeV|jet pT > 3 GeV, drawn > 30 GeV" \
  > genJetPileup_08_PU.log 2>&1
pdfunite ../../07_GenJet/out/genJetAnalyzer_07_GenJet.pdf genJetPileup_08_PU.pdf genJetAnalyzer_08_PU.pdf
pdftoppm -cropbox -r 110 -png genJetAnalyzer_08_PU.pdf genJetAnalyzer_08_PU_page; cd ../..
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
per collection in `jets`). Produces `genJetPileup_08_PU.pdf` (one page), `.root` (the jet histograms,
`TFileService`), `.log` (every jet with pT, eta, phi, mass, constituents and pileup pT, the b / bbar
matching; section 4), the EDM file `step2_GENJETPU_08_PU.root` (section 2); `pdfunite` makes
`genJetAnalyzer_08_PU.pdf` (the 8 pages of chapter 07 + the overlay page 9) and `pdftoppm` the PNG pages
(`_page-9.png` is figure 8.2 of the chapter).

**the ledger**, `common/genLedger.py --pu` on the EDM file:

```bash
python3 common/genLedger.py 08_PU/out/step2_GENJETPU_08_PU.root --event 1:1:1 --pu --jets ak4GenJetsPU > 08_PU/out/ledger_08_PU.txt
```

Prints the record by status group and the final state by kind and region, each for the signal
(`collisionId` 0), the pileup (`collisionId` > 0) and the sum, and the `ak4GenJetsPU` summary (section 4).

## 4. The tables

### 4.1 The Pythia8 process table and the cross sections (`step1_GEN_MinBias_08_PU.log`, `gencheck_08_PU.log`)

```
 | We collide p+ with p+ at a CM energy of 1.360e+04 GeV            |
 *-------  PYTHIA Event and Cross Section Statistics  -------------------------------------------------------------*
 | Subprocess                                    Code |            Number of events       |      sigma +- delta    |
 |                                                    |       Tried   Selected   Accepted |     (estimated) (mb)   |
 | non-diffractive                                101 |         183        183        183 |   5.542e+01  2.991e-07 |
 | A B -> X B single diffractive                  103 |          27         27         27 |   6.432e+00  0.000e+00 |
 | A B -> A X single diffractive                  104 |          18         18         18 |   6.432e+00  0.000e+00 |
 | A B -> X X double diffractive                  105 |          19         19         19 |   8.848e+00  4.737e-08 |
 | A B -> A X B central diffractive               106 |           3          3          3 |   1.285e+00  0.000e+00 |
 | sum                                                |         250        250        250 |   7.842e+01  3.028e-07 |
Before Filter: total cross section = 7.842e+10 +- 3.028e+02 pb
Filter efficiency (event-level)= (250) / (250) = 1.000e+00 +- 0.000e+00    [TO BE USED IN MCM]
```

Derived: sigma_inel = 100.309 - 21.89 = 78.419 mb (the CP5 `SigmaTotal` settings, `PythiaCP5Settings_cfi.py`);
shares ND 55.42 / 78.42 = 70.7 %, SD 12.864 / 78.42 = 16.4 %, DD 8.848 / 78.42 = 11.3 %, CD 1.285 / 78.42 = 1.6 %;
expected in 250 events: 176.7, 41.0, 28.2, 4.1 (DD deficit (28.2 - 19) / sqrt(28.2) = 1.7 sigma).

```
 |                   sigmaNonDiffractive =    55.42 mb              |
 |    pT0 =  1.44 gives sigmaInteraction =   492.18 mb: accepted    |
```

pT0 = pT0Ref x (E_cm / 7 TeV)^ecmPow = 1.41 x (13.6 / 7)^0.03344 = 1.44 GeV; 492.18 / 55.42 = 8.9 interactions
per ND collision on average. Pythia version: 8.311; PDF: NNPDF31_nnlo_as_0118 (LHAPDF ID 303600), LHAPDF 6.4.0.
Pythia warnings: 1 `BeamRemnants::setKinematics` kinematics failure (retried), 44 `pT2nextQCD: weight above
unity`, 3 `MultipartonInteractions::init: maximum increased`, 2 `TimeShower::init: pTmin too low`.

### 4.2 The configuration checks (`gencheck_08_PU.log`)

```
process = cms.Process('GEN',Run3_2024)
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('IOMC.EventVertexGenerators.VtxSmearedRealistic_cfi')
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase1_2024_realistic', '')
process.generator.comEnergy = cms.double(13600.0)
process.RandomNumberGeneratorService.generator.initialSeed = 20241
process.RandomNumberGeneratorService.VtxSmeared.initialSeed = 20242
== events in the file
250 events
```

### 4.3 The catalogue summary (`genstats_08_PU.txt`, lines 253-268)

```
== 250 events, beam energy 6800 GeV
process shares: ND 183 (73.2%), SD 45 (18.0%), DD 19 (7.6%), CD 3 (1.2%)
interactions mean    7.03  median    3.00  min    1.00  max   39.00
stable       mean  193.75  median  132.00  min    3.00  max  723.00
charged      mean   89.02  median   59.00  min    2.00  max  320.00
central      mean   21.19  median    9.50  min    0.00  max  126.00
|eta|>5.5    mean   38.92  median   36.50  min    3.00  max   93.00
lead pT      mean    2.20  median    1.81  min    0.23  max    8.08
sum pT       mean   79.25  median   44.65  min    0.46  max  376.09
jets>10      mean    0.15  median    0.00  min    0.00  max    3.00
PPS protons  mean    0.37  median    0.00  min    0.00  max    2.00
per process (events, mean interactions, stable, charged, central, lead pT, sum pT):
  ND  183    9.0  241.1  110.8  27.0  2.53   99.9
  SD   45    2.0   69.5   32.8   7.0  1.41   26.2
  DD   19    1.2   56.8   24.2   2.1  1.13   16.7
  CD    3    2.0   35.0   16.0   2.3  0.84    9.4
stable particles by species: gamma 45.9%, pi+- 37.1%, K+- 4.8%, p 2.7%, n 2.5%, K0L 2.4%, K0S 2.2%, Lambda 0.9%, e 0.6%, Sigma- 0.3%, Sigma+ 0.3%, Xi- 0.1%
|eta| of the stable particles, per unit (last bin >= 10): 6950 7498 7816 7227 6406 5365 3509 2041 1039 419 167
pT of the stable particles (<0.2, 0.2-0.5, 0.5-1, 1-2, 2-5, >5 GeV): 18209 17452 8978 3094 687 17
ND: mean stable particles against the number of interactions: 1: 85 (44 ev), 2: 109 (17 ev), 3: 120 (10 ev), 4: 150 (7 ev), 5: 177 (7 ev), 6: 195 (11 ev), 7: 235 (8 ev), 8: 298 (3 ev), 9: 248 (6 ev), 10: 319 (6 ev), 11: 303 (10 ev), 12: 334 (4 ev), 13: 355 (2 ev), 14: 398 (4 ev), 15: 330 (4 ev), 16: 409 (5 ev), 17: 428 (2 ev), 18: 390 (5 ev), 19: 403 (3 ev), 20: 444 (2 ev), 22: 428 (2 ev), 23: 495 (5 ev), 24: 438 (2 ev), 25: 467 (4 ev), 26: 548 (2 ev), 27: 576 (2 ev), 28: 658 (1 ev), 30: 498 (1 ev), 31: 492 (1 ev), 32: 561 (1 ev), 36: 540 (1 ev), 39: 723 (1 ev)
SD: diffractive mass M_X median 166.8 GeV (min 1.2, max 6754), xi = 1 - E_p/E_beam median 1.5e-04 (min 3.7e-09, max 0.25)
```

Derived from the per-event lines (columns z[cm], t[ns], jet10): vertex z mean 0.08 cm, median -0.06, r.m.s.
3.56 cm, min -9.52, max 10.08; t mean -0.007 ns, median -0.016, r.m.s. 0.122 ns, min -0.392, max 0.365;
21 of the 250 collisions have a jet above 10 GeV; total stable particles 250 x 193.75 = 48 437, of which
18209 + 17452 = 35 661 below 0.5 GeV; beyond |eta| = 5.5: 38.92 / 193.75 = 20 %.

### 4.4 The in-time collisions, library events 72-139 (`genstats_08_PU.txt`, lines 73-140)

```
event proc nInt stable  charg  centr |eta|>5.5  leadpT   sumpT jet10  PPS   z[cm]   t[ns]
   72   ND    3    192     84     13     31    2.20    56.6     0    0   -0.48  -0.092
   73   ND    4    113     56      4     40    1.65    31.2     0    0   -4.13  -0.195
   74   ND    2    118     54     14     35    1.85    47.9     0    0   -3.66  -0.041
   75   SD    1     53     22      1     20    0.93    14.7     0    1   -0.32   0.013
   76   SD    1     29     16      3      7    0.76     9.1     0    2    1.52   0.198
   77   SD    4    206     90     18     15    3.60    73.0     0    1   -6.74   0.064
   78   ND    1     76     36      6     31    1.73    27.5     0    1    0.09  -0.013
   79   ND    4    152     58      5     54    2.83    49.5     0    0   -5.42  -0.096
   80   ND    2     75     34      3     22    1.26    26.1     0    0    0.54   0.031
   81   ND   12    321    140     66     40    6.45   203.3     3    0   -0.34  -0.018
   82   ND   10    352    160     57     43    4.51   157.5     0    0    1.49  -0.003
   83   SD    2     86     44     14     19    1.32    33.6     0    1   -9.52   0.091
   84   ND    3     71     34      8     20    1.36    22.2     0    1   -4.74   0.096
   85   ND    5    105     42      3     27    1.07    33.4     0    0   -1.43  -0.106
   86   SD    6    167     84     30     30    2.91    82.4     0    1    6.14   0.109
   87   ND    2    114     52      5     36    1.30    31.6     0    0   -4.02  -0.118
   88   ND   10    260    122     31     49    2.76   114.1     0    0   -3.17  -0.052
   89   DD    1     24      6      0     14    0.98     6.2     0    1    3.24   0.066
   90   ND    1     92     38      0     32    1.24    24.0     0    1   -0.72   0.193
   91   SD    1     19      6      0     15    0.96     5.1     0    1   -1.03  -0.047
   92   ND   25    434    188     57     64    3.64   191.0     0    0    5.51  -0.006
   93   ND   11    250    126     16     63    2.20    84.9     0    0    0.76  -0.052
   94   ND   26    547    242     81     76    5.31   262.7     0    0    2.50   0.117
   95   ND   16    461    220     47     70    3.14   203.7     0    0    3.49  -0.005
   96   ND    1     64     38      7     21    1.24    20.0     0    0   -0.36   0.262
   97   ND    9    242    114     35     59    1.96    83.8     0    0    6.53   0.020
   98   SD    1     66     38      6     18    1.33    21.7     0    2   -4.68   0.142
   99   DD    1     39     20      1     22    0.97    11.8     0    1   -3.35  -0.161
  100   SD    7    161     88     38     17    4.27    89.8     0    1   -0.08   0.051
  101   ND   20    444    214     48     50    3.05   196.7     0    0   -2.87   0.135
  102   ND   23    519    250    113     65    5.38   317.3     2    0   -5.45   0.092
  103   ND   15    405    192     72     72    5.66   205.7     0    0    1.25  -0.179
  104   SD    2     68     36      7     24    2.33    21.9     0    1   -1.00  -0.074
  105   ND    6    283    122     25     51    5.98   119.0     0    0    3.34   0.091
  106   ND   12    382    182     47     75    2.96   163.1     0    0    2.88   0.201
  107   ND   36    540    252     90     71    4.91   281.4     2    0    0.36  -0.152
  108   ND   15    279    134     22     61    2.04    98.5     0    0    4.67  -0.089
  109   ND   24    479    202     49     55    3.35   213.4     0    0    0.27   0.155
  110   SD    3    149     70     24     19    5.42    91.1     2    1   -2.05  -0.316
  111   SD    1     12      4      0     11    0.64     3.6     0    1   -2.34   0.189
  112   ND    1     88     38      2     36    1.03    22.9     0    0   -4.79  -0.074
  113   ND    7    170     64      9     38    1.08    48.7     0    0   -4.66  -0.115
  114   ND   11    315    162     36     93    4.41   124.1     2    0    2.08   0.331
  115   ND    1     79     40      5     19    1.02    23.0     0    0   -2.34  -0.087
  116   ND    1     90     46      3     36    0.90    25.7     0    0    4.18   0.040
  117   DD    1     56     16      0     39    1.20    14.8     0    0   -1.38  -0.147
  118   ND    6    241    108     14     47    3.32    79.7     0    0    2.08   0.182
  119   ND    5    180     76      7     53    1.51    58.0     0    0    5.74   0.091
  120   ND    1     62     28      6     24    1.35    23.1     0    2   -1.37   0.109
  121   ND    2    109     54      4     43    1.45    36.7     0    0   -4.89  -0.000
  122   ND   11    371    164     36     61    2.97   142.8     0    0   -2.82   0.196
  123   ND    5    234     98     25     39    2.60    82.0     0    0   -2.05   0.078
  124   SD    1      6      4      0      6    0.43     1.8     0    2    1.03  -0.053
  125   DD    1     61     28      1     29    0.77    16.9     0    1    1.14  -0.119
  126   ND    7    273    128     21     56    1.71    82.5     0    0   -1.06   0.143
  127   SD    4    131     62     14     31    1.57    51.7     0    1   -1.03  -0.140
  128   ND   13    262    112     16     52    2.31    92.2     0    0   -2.12   0.038
  129   ND    7    249    108     23     78    2.33    92.4     0    0   -6.03  -0.079
  130   ND    1     72     38      2     18    1.16    22.5     0    0    3.93   0.024
  131   ND   24    397    164     41     75    3.50   157.9     0    0   -0.06  -0.244
  132   SD    1      3      2      0      3    0.23     0.5     0    1    0.54   0.009
  133   ND    2    198    106     27     35    1.53    72.5     0    0    3.82  -0.053
  134   SD    2     45     26      0     10    0.85    12.5     0    1   -4.15  -0.098
  135   ND    8    265    136     27     39    2.26   102.2     0    0   -4.16  -0.016
  136   ND    1     89     38      5     24    1.45    27.7     0    0    4.76  -0.121
  137   ND    1     98     48      5     38    1.80    29.5     0    0    5.01  -0.060
  138   ND   12    304    148     14     63    2.00   111.7     0    0    4.36  -0.137
  139   ND    1     96     46      5     31    0.96    28.7     0    1    3.51  -0.160
```

Sums over the 68 lines: 49 ND, 15 SD, 4 DD, 0 CD; 467 interactions; 12 993 stable particles (mean 191.1,
median 149, min 3 = event 132 (SD), max 547 = event 94 (ND)); 5998 charged; 1414 central; 11 jets above 10 GeV.
The neighbouring draws of the mixing of chapter 10 (seed 12345, Poisson(60) per bunch crossing, the library
read in sequence): bunch crossing -1 = library events 1-71, bunch crossing +1 = events 140-193.

### 4.5 The drawn collision, library event 72 (`genPartAnalyzer_MinBias_08_PU.log`)

```
== GenPartAnalyzer run:lumi:event 1:1:72 nGenParticles=406 nStatus1=192 hard b idx=hard bbar idx=
  stage 1 (hardProcess): nParticles=2 | b: none | #bar{b}: none
  stage 2 (partonShower): nParticles=13
  stage 3 (mpiBeamRemnants): nParticles=26
  stage 4 (hadronisation): nParticles=102
  stage 5 (hadronDecays): nParticles=192
```

Catalogue line 72: ND, 3 interactions, 192 stable, 84 charged, 13 central, 31 beyond |eta| = 5.5, hardest
particle 2.20 GeV, sum pT 56.6 GeV, no jet above 10 GeV, z = -0.48 cm, t = -0.092 ns. Page 1 of
`genPartAnalyzer_MinBias_08_PU.pdf` draws the two hard-process partons without a pT label (both below the
5 GeV label threshold); page 5 draws 192 particles, 47 primary hadrons and 145 decay products, 18 with
|eta| > 6 on the border.

### 4.6 The overlay (`genJetPileup_08_PU.log`, first lines and the 12 leading jets of 266)

```
== GenJetAnalyzer run:lumi:event 1:1:1 nGenParticles=36218 nStatus1=13627 (signal 634 + pileup 12993 from 68 collisions) | hard b: [5] pt=113.221 -> last copy [824] status=71 pt=91.7437 eta=-1.56979 phi=-2.08372 | hard bbar: [4] pt=113.221 -> last copy [964] status=71 pt=116.356 eta=-3.74787 phi=1.01139
  ak4GenJetsPU (process GENJETPU, AntiKt R=0.4, jetPtMin=3 GeV, src=genParticlesForJetsPU): nJets=266 nClustered=13106/13627 status 1 particles (signal 627/634, pileup 12479/12993)
    b quark -> jet [1] pt=150.895 eta=-1.62991 phi=-2.08378 nConst=114 dR=0.0601206 pt(jet)/pt(quark)=1.64474 pileupPt=33.9195 (22.4789%)
    bbar quark -> jet [0] pt=152.028 eta=-3.77978 phi=1.00135 nConst=88 dR=0.033447 pt(jet)/pt(quark)=1.30657 pileupPt=36.7424 (24.1683%)
    net b jet + bbar jet: |sum pT|=8.62555 (px=7.91221 py=-3.43469) scalar sum pT=302.922 dphi=3.08513
    [0] pt=152.028 eta=-3.77978 phi=1.00135 mass=44.3279 nConst=88 pileupPt=36.7424 (24.1683%)
    [1] pt=150.895 eta=-1.62991 phi=-2.08378 mass=51.3568 nConst=114 pileupPt=33.9195 (22.4789%)
    [2] pt=60.9265 eta=-0.427131 phi=0.301034 mass=35.8603 nConst=94 pileupPt=59.1301 (97.0515%)
    [3] pt=57.9195 eta=-1.41116 phi=3.03549 mass=35.9035 nConst=109 pileupPt=54.9782 (94.9218%)
    [4] pt=56.1916 eta=-0.942531 phi=-2.72205 mass=30.7832 nConst=96 pileupPt=53.1819 (94.6437%)
    [5] pt=54.6808 eta=2.54838 phi=-2.16859 mass=34.1365 nConst=114 pileupPt=52.4901 (95.9936%)
    [6] pt=54.1034 eta=1.23233 phi=2.42233 mass=36.5441 nConst=95 pileupPt=40.9415 (75.6728%)
    [7] pt=51.5411 eta=-2.43167 phi=-0.104322 mass=28.0673 nConst=106 pileupPt=51.0359 (99.0199%)
    [8] pt=51.011 eta=0.24542 phi=-1.52298 mass=29.7201 nConst=92 pileupPt=51.3976 (100.758%)
    [9] pt=48.7428 eta=-1.98297 phi=-1.42794 mass=28.2221 nConst=89 pileupPt=39.4301 (80.8941%)
    [10] pt=48.2716 eta=1.5031 phi=-0.796267 mass=31.7833 nConst=100 pileupPt=40.5155 (83.9324%)
    [11] pt=47.1941 eta=-1.079 phi=-0.140472 mass=27.0372 nConst=85 pileupPt=44.8704 (95.0763%)
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

Derived from the two jet lists: jets above 10 / 20 / 30 / 50 GeV: 6 / 2 / 2 / 2 (signal) and 175 / 116 / 76 / 9
(overlay); dijet mass m(jet [0] + jet [1]) from (pT, eta, phi, mass): 386.8 GeV (signal, also printed on page 6
as "m = 386.8 GeV") and 521.7 GeV (overlay, printed on page 9); the b jet (now jet [1]) gained
150.895 - 117.788 = 33.1 GeV and the bbar jet (jet [0]) 152.028 - 115.944 = 36.1 GeV; the jet axes moved by
Delta R = 0.041 (b) and 0.034 (bbar).
From `step2_GENJETPU_08_PU.root` with FWLite (`Events` / `Handle('std::vector<reco::GenJet>')` on
`ak4GenJetsPU`, then `jet.daughter(k).collisionId()` over `k < jet.numberOfDaughters()`, 0 = signal): jet [0]
(bbar) 18 signal + 70 pileup constituents
from 32 collisions, jet [1] (b) 24 + 90 from 41, jet [2] 5 + 89 from 34, jet [3] 8 + 101 from 44; 87 of the 266
jets have no signal constituent; 74 of the 76 jets above 30 GeV have at least one. `PileupMixingContent` of
`mix`: bunch crossing [0], 68 collisions, true mean 68.

### 4.7 The ledger (`ledger_08_PU.txt`, in full)

```
== genLedger step2_GENJETPU_08_PU.root event 1:1:1: 36218 particles in genParticlesPU
record by status group (signal / pileup / all):
  hard process (21-29)            4 /    267 /    271
  MPI (31-39)                   124 /   1596 /   1720
  ISR (41-49)                   190 /   3077 /   3267
  FSR (51-59)                   192 /   3390 /   3582
  beam remnants (61-69)         233 /   3855 /   4088
  strings (71-79)               165 /   2882 /   3047
  decayed (status 2)            302 /   6121 /   6423
  final state (status 1)        634 /  12993 /  13627
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
final state (status 1) of the pileup: n=12993 sumE=924800.0 GeV sumPt=5314.6 GeV
  by kind                 n    E[GeV]  pT[GeV]
  charged hadron       5911  572734.9   3087.8
  neutral hadron       1088  179327.8    749.7
  photon               5900  171675.6   1458.8
  electron               85     998.9     10.9
  muon                    2       5.3      1.7
  neutrino                7      57.5      5.6
  by region               n    E[GeV]  pT[GeV]     E_nu
  |eta|<2.5            4864    5519.3   2179.4     11.0
  2.5-3.0              1037    3597.0    449.2      0.0
  3.0-5.2              3996   58413.2   1674.3     46.5
  5.2-5.5               436   17054.0    160.3      0.0
  >5.5                 2660  840216.5    851.4      0.0
  visible (|eta|<5.2, no neutrinos): n=9890 E=67472.0 GeV pT=4297.3 GeV; neutrinos: n=7 E=57.5 GeV pT=5.6 GeV
final state (status 1) of the all: n=13627 sumE=938400.0 GeV sumPt=5843.5 GeV
  by kind                 n    E[GeV]  pT[GeV]
  charged hadron       6214  579902.7   3347.0
  neutral hadron       1144  182777.7    834.4
  photon               6173  174551.6   1601.4
  electron               86    1009.8     15.9
  muon                    2       5.3      1.7
  neutrino                8     153.0     43.0
  by region               n    E[GeV]  pT[GeV]     E_nu
  |eta|<2.5            5180    6253.1   2458.1    106.5
  2.5-3.0              1092    3790.1    473.9      0.0
  3.0-5.2              4185   63432.8   1871.7     46.5
  5.2-5.5               452   17890.4    168.1      0.0
  >5.5                 2718  847033.6    871.7      0.0
  visible (|eta|<5.2, no neutrinos): n=10449 E=73323.0 GeV pT=4760.7 GeV; neutrinos: n=8 E=153.0 GeV pT=43.0 GeV
jets ak4GenJetsPU: n=266 sumPt=5616.2 GeV sumE=555114.4 GeV constituents=13106
  [0] pT=152.0 eta=-3.78 phi=1.00 mass=44.3 nConst=88
  [1] pT=150.9 eta=-1.63 phi=-2.08 mass=51.4 nConst=114
  [2] pT=60.9 eta=-0.43 phi=0.30 mass=35.9 nConst=94
  [3] pT=57.9 eta=-1.41 phi=3.04 mass=35.9 nConst=109
```

In the chapter's ledger table the rows "electrons, muons" are electron + muon (pileup 85 + 2 = 87, E
998.9 + 5.3 = 1004.2 GeV, pT 10.9 + 1.7 = 12.6 GeV; all 88, 1015.1, 17.6). Derived: pileup energy beyond
|eta| = 5.5: 840 217 / 924 800 = 91 %; pileup pT density in |eta| < 2.5: 2179.4 / (5 x 2 pi) = 69.4 GeV per
unit of eta-phi area; mean pT of a pileup particle there 2179.4 / 4864 = 0.45 GeV; an R = 0.4 cone (area
pi R^2 = 0.503) collects 34.9 GeV; the signal column equals `test/07_GenJet/out/ledger_07_GenJet.txt`.
At PU 200 (x 200 / 68 = 2.9): 204 GeV per unit area, 103 GeV per cone.

### 4.8 The release files quoted in "Open problems for Phase-2" (`$CMSSW_RELEASE_BASE/src`)

```
Configuration/PyReleaseValidation/python/relval_steps.py (4617):
    elif 'Run4' in ds:
        PUDataSets[ds]={'-n':10,'--pileup':'AVE_200_BX_25ns','--pileup_input':'das:/RelValMinBias_14TeV/%s/GEN-SIM'%(name,)}
    (4612-4615: '2022', '2023', '2024', '2025' -> 'Run3_Flat55To75_PoissonOOTPU' with RelValMinBias_14TeV)
Configuration/StandardSequences/python/Mixing.py (184, 160, 124):
    addMixingScenario("AVE_200_BX_25ns",{'file': 'SimGeneral.MixingModule.mix_POISSON_average_cfi','BX':25, 'B': (-3,3), 'N': 200})
    addMixingScenario("AVE_50_BX_25ns",{'file': 'SimGeneral.MixingModule.mix_POISSON_average_cfi','BX':25, 'B': (-12,3), 'N': 50})
    addMixingScenario("Run3_Flat55To75_PoissonOOTPU",{'file': 'SimGeneral.MixingModule.mix_Run3_Flat55To75_PoissonOOTPU_cfi'})
    defineMixing(): 'N' -> process.mix.input.nbPileupEvents.averageNumber, 'BX' -> process.mix.bunchspace, 'B' -> process.mix.minBunch / maxBunch
SimGeneral/MixingModule/python/mix_POISSON_average_cfi.py: type = 'poisson', sequential = cms.untracked.bool(False), minBunch -5, maxBunch 3, bunchspace 450 (overridden by the scenario)
SimGeneral/MixingModule/python/mix_probFunction_25ns_PoissonOOTPU_cfi.py: minBunch = -12, maxBunch = 3, bunchspace = 25, type = 'probFunction', manage_OOT = True
SimGeneral/MixingModule/python/mix_Run3_Flat55To75_PoissonOOTPU_cfi.py: probValue 0 for 0-54, 1/21 = 0.047619 for 55-75
Configuration/Generator/python/MinBias_14TeV_pythia8_TuneCP5_cfi.py: comEnergy = 14000.0, crossSection = 71.39e+09, SoftQCD:inelastic = on
Configuration/Generator/python/MCTunes2017/PythiaCP5Settings_cfi.py: SigmaTotal:mode = 0, SigmaTotal:sigmaEl = 21.89, SigmaTotal:sigmaTot = 100.309, MultipartonInteractions:pT0Ref=1.41, ecmPow=0.03344
Configuration/Generator/python/: MinBias_13TeV_pythia8_TuneCUETP8M1_cfi, MinBias_14TeV_pythia8_TuneCUETP8M1_cfi, MinBias_TuneZ2star_*_pythia6_cff, Hydjet_*_MinBias_* (no 13.6 TeV CP5 fragment)
Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py (3207-3218, 'Run4D110'): 'Geom': 'ExtendedRun4D110', 'BeamSpot': 'DBrealisticHLLHC', 'GT': 'auto:phase2_realistic_T33', 'Era': 'Phase2C17I13M9'
Configuration/AlCa/python/autoCond.py (111): 'phase2_realistic' : '150X_mcRun4_realistic_v1'; (95): 'phase1_2024_realistic' : '150X_mcRun3_2024_realistic_v2'
IOMC/EventVertexGenerators/python/VtxSmearedRealisticHLLHC_cfi.py: VtxSmeared = HLLHCEvtVtxGenerator.clone(src = "generator:unsmeared", readDB = True)   # SimBeamSpotHLLHCObjectsRcd
IOMC/EventVertexGenerators/python/VtxSmearedRealistic_cfi.py: VtxSmeared = BetafuncEvtVtxGenerator.clone(src = "generator:unsmeared", readDB = True)    # SimBeamSpotObjectsRcd
IOMC/EventVertexGenerators/python/VtxSmearedParameters_cfi.py (1101-1117), HLLHCVtxSmearingParameters:
    MeanXIncm = MeanYIncm = MeanZIncm = 0, TimeOffsetInns = 0.0, EprotonInGeV = 6500.0, CrossingAngleInurad = 510.0, CrabFrequencyInMHz = 400.0, RF800 = False,
    BetaCrossingPlaneInm = 0.20, BetaSeparationPlaneInm = 0.20, HorizontalEmittance = 2.5e-06, VerticalEmittance = 2.05e-06, BunchLengthInm = 0.090,
    CrabbingAngleCrossingInurad = 380.0, CrabbingAngleSeparationInurad = 0.0
IOMC/EventVertexGenerators/python/VtxSmearedHLLHC14TeV_cfi.py: HLLHCVtxSmearingParameters.clone(EprotonInGeV = cms.double(7000))
IOMC/EventVertexGenerators/src/HLLHCEvtVtxGenerator.cc (131-132): z = (shoot() - 0.5) * 6.0 * fBunchLength; t = (shoot() - 0.5) * 6.0 * fBunchLength;  (accept-reject on intensity(x, y, z, t))
IOMC/EventVertexGenerators/src/BetafuncEvtVtxGenerator.cc (84, 97): tmp_sigz = RandGaussQ::shoot(engine, 0., fSigmaZ); tmp_sigt = RandGaussQ::shoot(engine, 0., fSigmaZ)
doc/tmp/Gen4Phase2.md (53): SimBeamSpotHLLHCObjectsRcd payload SimBeamSpotHLLHCObjects_z4p3cm_t193ns_14TeV_nominal_mc_v1 (GT 150X_mcRun4_realistic_v1); not re-checked with conddb for this chapter
```

Library sizes: PU 200 x 7 crossings (-3..3) = 1400 minimum-bias events per signal event; Run 3 scenario
~65 x 16 crossings (-12..3) = 1040; this book's mixing (chapter 10, bunch crossings -1..1, mean 60) 193 for
this event: 71 + 68 + 54 library events for bunch crossings -1, 0 and +1.
