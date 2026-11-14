# Chapter 02, PS: technical companion

The technical companion of `doc/chapter/chapter_02_PS.tex` (Rule C of `Journey/Constitution.txt`): the commands of
`test/02_PS/run_02_PS.sh`, the walk-through of the generated configuration, the analyzer commands, and every number
the chapter quotes, copied from `test/02_PS/out/` (and from `test/01_ME/out/`, `test/04_MPI/out/` for the comparisons).
All commands are run from `test/` of an initialized `CMSSW_15_0_5` (`cd CMSSW_15_0_5/src && cmsenv && cd JitJetSW/Journey/test`).

## 1. The cmsDriver.py commands

The chapter script is `test/02_PS/run_02_PS.sh`. It sets `CHAPTER=02_PS`, sources `test/common/jitjet_common.sh`
and runs three steps:

```bash
#         ISR FSR MPI Had Decay pages
CONE_R=${CONE_R:-0.4} gen_stage on  on  off off off  2 "the parton shower (ISR + FSR)"
XSEC_FILE=1 gen_xsec            on  on  off off off
python3 "$COMMON/genShowerStats.py" "$OUT/xsec_GEN_02_PS.root" --event 1:1:1 > "$OUT/showerStats_02_PS.txt"
```

`CONE_R=0.4` makes the GenPartAnalyzer pages draw the cones of Delta R < 0.4 around the b and the bbar (section 3).
`gen_xsec` (chapter 01) generates 1000 events with the same settings and seeds; `XSEC_FILE=1` keeps them in
`out/xsec_GEN_02_PS.root`, and `genShowerStats.py` counts the shower branchings of every event (sections 3 and 4.7).

`gen_stage` (in `jitjet_common.sh`) symlinks the fragment `JitJetSW/Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py`
into `Configuration/GenProduction/python/` (where `cmsDriver.py` looks for it), then runs `cmsDriver.py` with
`--no_exec` to write `test/02_PS/step1_GEN_02_PS_cfg.py`, then `cmsRun` on it in `test/02_PS/out/`, then the analyzer
and the ledger. The exact `cmsDriver.py` command, as recorded in the header of
`step1_GEN_02_PS_cfg.py` ("with command line options") and in `out/step1_GEN_02_PS_cfg.driver.log`:

```bash
./02_PS/run_02_PS.sh                 # the whole chapter: config, cmsRun, analyzer, ledger
RUN=0 ./02_PS/run_02_PS.sh           # only write the configuration
ANALYZE=0 ./02_PS/run_02_PS.sh       # generate without the analyzer, the ledger and the shower statistics
CONE_R=0 ./02_PS/run_02_PS.sh        # the display without the cones
XSEC_EVENTS=10000 ./02_PS/run_02_PS.sh   # the 1000-event job with 10000 events

# what gen_stage runs (the python_filename is given as an absolute path by the script):
cmsDriver.py Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py \
  --conditions auto:phase1_2024_realistic --era Run3_2024 --geometry DB:Extended --beamspot DBrealistic \
  -n 1 -s GEN --datatier GEN --eventcontent FEVTDEBUG --fileout file:step1_GEN_02_PS.root \
  --customise_commands "process.generator.comEnergy = cms.double(13600.0)\nprocess.generator.PythiaParameters.processParameters.extend(['PhaseSpace:pTHatMin = 100', 'PartonLevel:ISR = on', 'PartonLevel:FSR = on', 'PartonLevel:MPI = off', 'HadronLevel:all = off', 'HadronLevel:Hadronize = off', 'HadronLevel:Decay = off'])" \
  --python_filename 02_PS/step1_GEN_02_PS_cfg.py --no_exec
```

The arguments:

| argument | meaning |
|---|---|
| `Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py` | the generator fragment: `Pythia8ConcurrentGeneratorFilter` with `HardQCD:qqbar2bbbar = on`, `gg2bbbar = off`, `PhaseSpace:pTHatMin = 30.`, the `pythia8CommonSettings` and `pythia8CP5Settings` blocks, `comEnergy = 14000` (both overridden below) |
| `--conditions auto:phase1_2024_realistic` | the global tag alias (resolved by `Configuration/AlCa/python/autoCond.py`); at GEN it supplies the beam spot used by the vertex smearing |
| `--era Run3_2024` | the era modifiers of the 2024 detector (`Configuration/Eras/python/Era_Run3_2024_cff.py`), `process = cms.Process('GEN', Run3_2024)` |
| `--geometry DB:Extended` | the geometry from the database (loaded, not used at GEN) |
| `--beamspot DBrealistic` | `IOMC/EventVertexGenerators/python/VtxSmearedRealistic_cfi.py`: the generated vertex is smeared with the beam spot of the global tag (the `generatorSmeared` HepMC product) |
| `-n 1` | one event (`EVENTS=1`; the book follows one event) |
| `-s GEN` | the GEN step only: `generation_step = cms.Path(process.pgen)` |
| `--datatier GEN` | the data tier written in the output provenance |
| `--eventcontent FEVTDEBUG` | the output content: everything of GEN (`FEVTDEBUGEventContent` of `Configuration/EventContent/python/EventContent_cff.py`) |
| `--fileout file:step1_GEN_02_PS.root` | the output file, written in `test/02_PS/out/` because `cmsRun` is run there |
| `--customise_commands "..."` | python appended at the end of the configuration: the beam energy `13600 GeV` (`COMENERGY`) and the Pythia8 switches appended to `processParameters`: `PhaseSpace:pTHatMin = 100` (`PTHATMIN`, replaces the 30 of the fragment because a later setting of the same parameter wins), `PartonLevel:ISR = on`, `PartonLevel:FSR = on`, `PartonLevel:MPI = off`, `HadronLevel:all = off`, `HadronLevel:Hadronize = off`, `HadronLevel:Decay = off`. The `\n` is a literal backslash-n in the bash string; `cmsDriver.py` turns it into a line break, so the generated file has two lines |
| `--python_filename` | where to write the configuration (`test/02_PS/step1_GEN_02_PS_cfg.py`) |
| `--no_exec` | write the configuration, do not run it (the script runs `cmsRun` itself, in `out/`) |

The second command, for the 1000 events (`gen_xsec`; header of `test/02_PS/xsec_GEN_02_PS_cfg.py`, output
`out/xsec_GEN_02_PS_cfg.driver.log`, log of `cmsRun` `out/xsec_GEN_02_PS.log`), differs only in `-n` and the file:

```bash
cmsDriver.py Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py \
  --conditions auto:phase1_2024_realistic --era Run3_2024 --geometry DB:Extended --beamspot DBrealistic \
  -n 1000 -s GEN --datatier GEN --eventcontent FEVTDEBUG --fileout file:xsec_GEN_02_PS.root \
  --customise_commands "<the same two statements as above>" \
  --python_filename 02_PS/xsec_GEN_02_PS_cfg.py --no_exec
```

The only differences from chapter 01 (`diff 01_ME/step1_GEN_01_ME_cfg.py 02_PS/step1_GEN_02_PS_cfg.py`) are
`PartonLevel:ISR = on`, `PartonLevel:FSR = on` (off in 01) and the file names. The random seed is the default of
`IOMC/RandomEngine/python/IOMC_cff.py` (`generator: initialSeed = 123456789`, loaded by `Configuration/StandardSequences/Services_cff.py`),
the same in every generator chapter; this is why the hard process is the same event.

## 2. The cmsRun on the python config

```bash
cd 02_PS/out && cmsRun ../step1_GEN_02_PS_cfg.py > step1_GEN_02_PS.log 2>&1; cd ../..
```

A walk-through of `test/02_PS/step1_GEN_02_PS_cfg.py` (written by `cmsDriver.py`, nothing edited by hand):

- `process = cms.Process('GEN', Run3_2024)`: the process name `GEN` is the one the products carry (`genParticles`, `""`, `GEN` in the dump below).
- `process.load(...)`: the standard sequences. `Services_cff` (among them the `RandomNumberGeneratorService` with the
  `generator` seed), `SimGeneral.HepPDTESSource.pythiapdt_cfi` (the particle table), `MessageLogger`,
  `EventContent_cff`, `mixNoPU_cfi` (no pileup), `GeometryRecoDB_cff`, `MagneticField_cff`, `Generator_cff` (the
  `pgen` sequence: the generator, `genParticles`, the generator jets and MET, the gen filter summary),
  `VtxSmearedRealistic_cfi` (the vertex smearing), `genFilterSummary_cff`, `EndOfProcess_cff`, the `GlobalTag`.
- `process.maxEvents.input = 1`, `process.source = cms.Source("EmptySource")`: no input file, the generator makes the event (run 1, lumi 1, event 1).
- `process.options`: the framework options, defaults (`numberOfThreads = 1`).
- `process.FEVTDEBUGoutput = cms.OutputModule("PoolOutputModule", ...)`: writes `file:step1_GEN_02_PS.root` with
  `FEVTDEBUGEventContent.outputCommands`, only for events that passed `generation_step` (`SelectEvents`).
- `process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase1_2024_realistic', '')`.
- `process.generator = cms.EDFilter("Pythia8ConcurrentGeneratorFilter", ...)`: the fragment, copied in. The
  `PythiaParameters` PSet holds `pythia8CommonSettings` (8 lines), `pythia8CP5Settings` (21 lines) and
  `processParameters` (3 lines); `parameterSets` lists the order in which they are passed to Pythia8.
- `process.generation_step = cms.Path(process.pgen)` and the three `EndPath`s (`genfiltersummary_step`,
  `endjob_step`, `FEVTDEBUGoutput_step`); `process.schedule` runs them in this order; the loop
  `for path in process.paths: getattr(process, path).insert(0, process.ProductionFilterSequence)` puts the generator
  filter first in every path.
- The customisation from the command line, appended at the end: `process.generator.comEnergy = cms.double(13600.0)`
  and the `extend([...])` of `processParameters` with the seven switches; then the early-deletion customisation.

The content of the output file, `edmDumpEventContent 02_PS/out/step1_GEN_02_PS.root`:

```
Type                                  Module                      Label      Process
---------------------------------------------------------------------------------------
GenEventInfoProduct                   "generator"                 ""         "GEN"
ROOT::Math::PositionVector3D<...>     "genParticles"              "xyz0"     "GEN"
double                                "ak4GenJets"                "rho"      "GEN"
double                                "ak4GenJetsNoNu"            "rho"      "GEN"
double                                "ak8GenJets"                "rho"      "GEN"
double                                "ak8GenJetsNoNu"            "rho"      "GEN"
double                                "ak4GenJets"                "sigma"    "GEN"
double                                "ak4GenJetsNoNu"            "sigma"    "GEN"
double                                "ak8GenJets"                "sigma"    "GEN"
double                                "ak8GenJetsNoNu"            "sigma"    "GEN"
edm::HepMCProduct                     "LHCTransport"              ""         "GEN"
edm::HepMCProduct                     "generatorSmeared"          ""         "GEN"
edm::RandomEngineStates               "randomEngineStateProducer"   ""         "GEN"
edm::TriggerResults                   "TriggerResults"            ""         "GEN"
float                                 "genParticles"              "t0"       "GEN"
vector<double>                        "ak4GenJets"                "rhos"     "GEN"
vector<double>                        "ak4GenJetsNoNu"            "rhos"     "GEN"
vector<double>                        "ak8GenJets"                "rhos"     "GEN"
vector<double>                        "ak8GenJetsNoNu"            "rhos"     "GEN"
vector<double>                        "ak4GenJets"                "sigmas"   "GEN"
vector<double>                        "ak4GenJetsNoNu"            "sigmas"   "GEN"
vector<double>                        "ak8GenJets"                "sigmas"   "GEN"
vector<double>                        "ak8GenJetsNoNu"            "sigmas"   "GEN"
vector<int>                           "genParticles"              ""         "GEN"
vector<reco::GenJet>                  "ak4GenJets"                ""         "GEN"
vector<reco::GenJet>                  "ak4GenJetsNoNu"            ""         "GEN"
vector<reco::GenJet>                  "ak8GenJets"                ""         "GEN"
vector<reco::GenJet>                  "ak8GenJetsNoNu"            ""         "GEN"
vector<reco::GenMET>                  "genMetCalo"                ""         "GEN"
vector<reco::GenMET>                  "genMetTrue"                ""         "GEN"
vector<reco::GenParticle>             "genParticles"              ""         "GEN"
```

- `genParticles` (`vector<reco::GenParticle>`): the Pythia8 event record, 34 entries in this chapter, with the Pythia8
  status codes, mother and daughter links; this is what the analyzer and the ledger read. `genParticles:xyz0` and
  `genParticles:t0` are the position and time of the primary vertex after the beam-spot smearing; `vector<int> genParticles`
  is the map from the collection index to the HepMC barcode.
- `generator` (`GenEventInfoProduct`): the event weight, the `qScale` of the hard process, the PDF information (x1, x2, id1, id2).
- `generatorSmeared` and `LHCTransport` (`edm::HepMCProduct`): the HepMC record of the event after the vertex smearing
  (and after the beam transport, which does nothing here); the HepMC record is what Geant4 will read in chapter 09.
- `ak4GenJets`, `ak4GenJetsNoNu`, `ak8GenJets`, `ak8GenJetsNoNu` (with their `rho`, `sigma`): the GEN step always runs
  the generator-jet sequence; in this chapter the "final state" is made of partons (status 1 quarks, gluons and
  diquarks), so these jets are parton jets. The jets of the book are those of chapter 07, built on the full event.
- `genMetTrue`, `genMetCalo`: the generator missing transverse momentum (zero neutrinos here).
- `TriggerResults` (the path results, the `generation_step` filter decision) and `randomEngineStateProducer` (the
  state of the random engines, so that the event could be reproduced from this file).

## 3. The analyzer configs

Run by `gen_stage` after `cmsRun`, from `test/02_PS/out/` (the `|` in `sampleLabel` separates the lines of the right panel):

```bash
cd 02_PS/out
cmsRun ../../common/genPartAnalyzer_cfg.py inputFiles=file:step1_GEN_02_PS.root outputFile=genPartAnalyzer_02_PS.root \
  maxEvents=1 eventsToProcess=1:1:1 maxStage=2 drawStages=2 pdfFile=genPartAnalyzer_02_PS.pdf \
  sampleLabel="chapter 02_PS: the parton shower (ISR + FSR)|ISR on, FSR on, MPI off|Hadronize off, Decay off|pTHatMin 100 GeV, 13.6 TeV" \
  coneR=0.4 > genPartAnalyzer_02_PS.log 2>&1
python3 ../../common/genLedger.py step1_GEN_02_PS.root --event 1:1:1 > ledger_02_PS.txt 2> ledger_02_PS.log
python3 ../../common/genShowerStats.py xsec_GEN_02_PS.root --event 1:1:1 > showerStats_02_PS.txt 2> showerStats_02_PS.log
cd ../..
```

- `genPartAnalyzer_cfg.py` runs the `GenPartAnalyzer` plugin (`Journey/plugins/GenPartAnalyzer.cc`) on the
  `genParticles`. It classifies every entry by its status code into the stage that produced it (1 hard process
  21-29, 2 parton shower 41-49 and 51-59, 3 MPI and beam remnants 31-39 and 61-69, 4 hadronisation, 5 decays), draws
  the particles alive at the end of stage 2 (`maxStage=2 drawStages=2`; stage 1 is analysed too, for the log) in the eta-phi plane (marker size ~ log pT, the pT
  written for pT > 5 GeV) and follows the b and bbar through their copies. It produces
  `genPartAnalyzer_02_PS.pdf` (1 page, the parton shower; the hard process is the page of chapter 01), `genPartAnalyzer_02_PS.root`
  (TH2 eta-phi weighted by pT and the pT histograms of the b, bbar and net b+bbar per stage, in the `TFileService`
  file) and `genPartAnalyzer_02_PS.log` (the b, bbar and net b+bbar per stage, quoted in section 4).
  With `coneR=0.4` (default 0, off; `gen_stage` passes `CONE_R`) every page also has the cone Delta R < 0.4 around
  the b and the bbar system: all particles alive at the stage except neutrinos within 0.4 of the system direction.
  Each cone is a dashed circle with its pT and n below it, its n and pT are appended to the b / bbar header lines,
  and a line `  cone dR<0.4 (stage N) | around b: n=... pt=... members [idx]pdg:pt:dR = ...` is printed per stage.
  The fuller cone (here the b cone, six partons) is drawn again in an inset placed in the corner of the frame that
  covers the fewest particles and no cone (`GenPartAnalyzer::drawZoom`): +-0.5 around the axis, every particle with
  its name and pT in a column on the right, joined to it by a thin line; the main frame then does not write their
  labels, which overlapped. The b+bbar label moves to the left of the star, clear of the bbar circle.
- The .tex includes `genPartAnalyzer_02_PS.pdf` itself as the figure of the chapter (pdfTeX uses the CropBox
  around the landscape canvas, not the A4 MediaBox of the ROOT PDF).
- `genLedger.py` (FWLite, no `cmsRun`) prints `ledger_02_PS.txt`: the record by status group, the hard process, the
  final state by kind and by region, and (option `--cone`, default 0.4) the cone around each outgoing parton of the
  hard process: the axis is the last copy of the parton (status 1 here, with the primordial kT), the cone holds the
  status-1 particles except neutrinos within Delta R < 0.4 (what `ak4GenJets` would cluster). These are the cone
  rows of table 2.3; they are of the status-1 record, the display's of the partons at the end of the shower.
- `genShowerStats.py` (FWLite) reads the 1000 events of `xsec_GEN_02_PS.root` and counts per event: the ISR
  branchings (entries with status 43, one per branching), the FSR branchings (status-51 entries / 2, including the
  FSR of the ISR partons) and the emissions q -> q g of the b and of the bbar of the hard process (a quark of the
  same-flavour chain with two status-51 daughters). It prints mean, distribution and number of events with none, the
  values of event 1:1:1 (the event of the book: the first of the 1000, same seeds) and the fraction of events without
  ISR in bins of the two momentum fractions x (`showerStats_02_PS.txt`, section 4.7).

The full record and the derived numbers of the chapter (table 2.2, the b system, the recoil of the incoming u) come
from this FWLite snippet, run from `test/`:

```bash
python3 - <<'EOF'
import math
from DataFormats.FWLite import Events, Handle
h = Handle('std::vector<reco::GenParticle>')
for ev in Events('02_PS/out/step1_GEN_02_PS.root'):
    ev.getByLabel('genParticles', h); ps = h.product(); break
def key(p): return (p.pdgId(), p.status(), p.pt(), p.eta())
index = {key(ps[i]): i for i in range(ps.size())}
print('%3s %6s %4s %8s %7s %7s %9s  %s' % ('i', 'pdg', 'st', 'pT', 'eta', 'phi', 'E', 'mothers -> daughters'))
for i in range(ps.size()):
    p = ps[i]
    moms = [index[key(p.mother(k))] for k in range(p.numberOfMothers())]
    daus = [index[key(p.daughter(k))] for k in range(p.numberOfDaughters())]
    print('%3d %6d %4d %8.3f %7.3f %7.3f %9.2f  %s -> %s' % (i, p.pdgId(), p.status(), p.pt(), p.eta() if p.pt() > 1e-6 else float('nan'), p.phi(), p.energy(), moms, daus))
def dr(a, b):
    d = abs(a.phi() - b.phi()); d = 2*math.pi - d if d > math.pi else d
    return math.hypot(a.eta() - b.eta(), d)
sysb = [24, 25, 26, 27, 28, 29]
px = sum(ps[i].px() for i in sysb); py = sum(ps[i].py() for i in sysb); pz = sum(ps[i].pz() for i in sysb); e = sum(ps[i].energy() for i in sysb)
print('b system %s: vector pT=%.2f scalar pT=%.2f E=%.2f mass=%.2f' % (sysb, math.hypot(px, py), sum(ps[i].pt() for i in sysb), e, math.sqrt(e*e-px*px-py*py-pz*pz)))
for i in sysb[1:]: print('  idx %d pdg %d pT=%.2f dR(b)=%.3f' % (i, ps[i].pdgId(), ps[i].pt(), dr(ps[i], ps[24])))
print('b system + bbar vector pT = %.2f' % math.hypot(px + ps[23].px(), py + ps[23].py()))
print('first branching: pT(6)=%.2f pT(7)=%.2f |sum|=%.2f m=%.2f dR=%.3f' % (ps[6].pt(), ps[7].pt(), math.hypot(ps[6].px()+ps[7].px(), ps[6].py()+ps[7].py()),
      math.sqrt((ps[6].energy()+ps[7].energy())**2-(ps[6].px()+ps[7].px())**2-(ps[6].py()+ps[7].py())**2-(ps[6].pz()+ps[7].pz())**2), dr(ps[6], ps[7])))
print('incoming u: E %.2f -> %.2f -> %.2f, x %.5f -> %.5f' % (ps[3].energy(), ps[8].energy(), ps[11].energy(), ps[3].energy()/6800., ps[11].energy()/6800.))
for r, d1, d2 in [(5, 6, 7), (7, 9, 10), (10, 12, 13), (14, 15, 16), (15, 18, 19)]:
    print('branching %d -> %d, %d: pT %.2f -> %.2f, %.2f |sum|=%.2f' % (r, d1, d2, ps[r].pt(), ps[d1].pt(), ps[d2].pt(), math.hypot(ps[d1].px()+ps[d2].px(), ps[d1].py()+ps[d2].py())))
def vpt(idx): return math.hypot(sum(ps[i].px() for i in idx), sum(ps[i].py() for i in idx))
shw = [12, 16, 17, 18, 19, 20]
print('shower copies %s: vector pT=%.2f, with the bbar (4): %.5f; status-1 remnants [30..33]: vector pT=%.2f' % (shw, vpt(shw), vpt(shw + [4]), vpt([30, 31, 32, 33])))
EOF
```

## 4. The tables

### 4.1 The ledgers (`test/01_ME/out/ledger_01_ME.txt`, `test/02_PS/out/ledger_02_PS.txt`)

```
== genLedger step1_GEN_01_ME.root event 1:1:1: 14 particles in genParticles
record by status group:
  hard process (21-29)            4
  beam remnants (61-69)           2
  final state (status 1)          6
hard process: incoming pdg -2 (E=14.8 GeV, x=0.0022) + pdg 2 (E=2652.9 GeV, x=0.3901) -> sqrt(s_hat) = 396.1 GeV
  outgoing pdg -5: pT=113.2 eta=-3.75 phi=0.99 E=2415.7 GeV
  outgoing pdg 5: pT=113.2 eta=-1.44 phi=-2.15 E=252.1 GeV
final state (status 1): n=6 sumE=13600.0 GeV sumPt=228.6 GeV
  by kind                 n    E[GeV]  pT[GeV]
  parton                  6   13600.0    228.6
  by region               n    E[GeV]  pT[GeV]     E_nu
  |eta|<2.5               1     253.1    113.6      0.0
  2.5-3.0                 0       0.0      0.0      0.0
  3.0-5.2                 2    2423.7    113.0      0.0
  5.2-5.5                 0       0.0      0.0      0.0
  >5.5                    3   10923.1      2.1      0.0
  visible (|eta|<5.2, no neutrinos): n=3 E=2676.9 GeV pT=226.5 GeV; neutrinos: n=0 E=0.0 GeV pT=0.0 GeV
  cone Delta R < 0.4 around the last copy of each outgoing parton (status 1, no neutrinos):
    bbar axis [8] status 1 pT=112.3 eta=-3.76 phi=0.99: n=1 E=2414.6 GeV vector pT=112.3 GeV scalar pT=112.3 GeV
         members (pdg pT dR): -5 112.3 0.00
    b    axis [9] status 1 pT=113.6 eta=-1.44 phi=-2.16: n=1 E=253.1 GeV vector pT=113.6 GeV scalar pT=113.6 GeV
         members (pdg pT dR): 5 113.6 0.00
```

```
== genLedger step1_GEN_02_PS.root event 1:1:1: 34 particles in genParticles
record by status group:
  hard process (21-29)            4
  FSR (51-59)                    15
  beam remnants (61-69)           2
  final state (status 1)         11
hard process: incoming pdg -2 (E=14.8 GeV, x=0.0022) + pdg 2 (E=2652.9 GeV, x=0.3901) -> sqrt(s_hat) = 396.1 GeV
  outgoing pdg -5: pT=113.2 eta=-3.75 phi=0.99 E=2415.7 GeV
  outgoing pdg 5: pT=113.2 eta=-1.44 phi=-2.15 E=252.1 GeV
final state (status 1): n=11 sumE=13600.0 GeV sumPt=230.7 GeV
  by kind                 n    E[GeV]  pT[GeV]
  parton                 11   13600.0    230.7
  by region               n    E[GeV]  pT[GeV]     E_nu
  |eta|<2.5               6     256.9    113.7      0.0
  2.5-3.0                 0       0.0      0.0      0.0
  3.0-5.2                 1    2413.9    114.0      0.0
  5.2-5.5                 0       0.0      0.0      0.0
  >5.5                    4   10929.2      2.9      0.0
  visible (|eta|<5.2, no neutrinos): n=7 E=2670.8 GeV pT=227.8 GeV; neutrinos: n=0 E=0.0 GeV pT=0.0 GeV
  cone Delta R < 0.4 around the last copy of each outgoing parton (status 1, no neutrinos):
    bbar axis [23] status 1 pT=114.0 eta=-3.74 phi=0.99: n=1 E=2413.9 GeV vector pT=114.0 GeV scalar pT=114.0 GeV
         members (pdg pT dR): -5 114.0 0.00
    b    axis [24] status 1 pT=47.0 eta=-1.37 phi=-2.19: n=6 E=256.9 GeV vector pT=113.6 GeV scalar pT=113.7 GeV
         members (pdg pT dR): 5 47.0 0.00, 21 43.2 0.16, 21 9.7 0.13, -1 6.3 0.31, 21 5.0 0.13, 1 2.5 0.05
```

No ISR group is printed for 02_PS because the record has no status 41-49 entry (the ledger prints only non-empty groups).

### 4.2 The analyzer logs

`test/02_PS/out/genPartAnalyzer_02_PS.log`:

```
== GenPartAnalyzer run:lumi:event 1:1:1 nGenParticles=34 nStatus1=11 hard b idx=5 hard bbar idx=4 
  stage 1 (hardProcess): nParticles=2 | b quark (status 23): pt=113.221 eta=-1.43851 phi=-2.15105 idx=[5] | #bar{b} quark (status 23): pt=113.221 eta=-3.75298 phi=0.990547 idx=[4] | net b+bbar: |sum pT|=9.89808e-06 (px=-8.27802e-06 py=5.42645e-06) scalar sum pT=226.442
  cone dR<0.4 (stage 1) | around b: n=1 pt=113.221 eta=-1.43851 phi=-2.15105 scalar sum pT=113.221 members [idx]pdg:pt:dR = [5]5:113.221:1.11022e-15 | around bbar: n=1 pt=113.221 eta=-3.75298 phi=0.990547 scalar sum pT=113.221 members [idx]pdg:pt:dR = [4]-5:113.221:3.06422e-14
  stage 2 (partonShower): nParticles=7 | b quark (status 52): pt=46.8792 eta=-1.36616 phi=-2.18942 idx=[20] | #bar{b} quark (status 23): pt=113.221 eta=-3.75298 phi=0.990547 idx=[4] | net b+bbar: |sum pT|=66.4005 (px=34.8855 py=56.498) scalar sum pT=160.1
  cone dR<0.4 (stage 2) | around b: n=6 pt=113.221 eta=-1.45084 phi=-2.15105 scalar sum pT=113.414 members [idx]pdg:pt:dR = [20]5:46.8792:0 [18]21:43.1201:0.161834 [19]21:9.65485:0.129597 [17]-1:6.28181:0.311934 [16]21:5.01816:0.127676 [12]1:2.46026:0.0498073 | around bbar: n=1 pt=113.221 eta=-3.75298 phi=0.990547 scalar sum pT=113.221 members [idx]pdg:pt:dR = [4]-5:113.221:3.06422e-14
```

`test/01_ME/out/genPartAnalyzer_01_ME.log` (stage 1 only, identical numbers):

```
== GenPartAnalyzer run:lumi:event 1:1:1 nGenParticles=14 nStatus1=6 hard b idx=5 hard bbar idx=4
  stage 1 (hardProcess): nParticles=2 | b quark (status 23): pt=113.221 eta=-1.43851 phi=-2.15105 idx=[5] | #bar{b} quark (status 23): pt=113.221 eta=-3.75298 phi=0.990547 idx=[4] | net b+bbar: |sum pT|=9.89808e-06 (px=-8.27802e-06 py=5.42645e-06) scalar sum pT=226.442
```

`test/04_MPI/out/genPartAnalyzer_04_MPI.log` (the same event with MPI on; the shower differs, see the caveat of the chapter):

```
== GenPartAnalyzer run:lumi:event 1:1:1 nGenParticles=745 nStatus1=169 hard b idx=5 hard bbar idx=4
  stage 1 (hardProcess): nParticles=2 | b quark (status 23): pt=113.221 eta=-1.43851 phi=-2.15105 idx=[5] | #bar{b} quark (status 23): pt=113.221 eta=-3.75298 phi=0.990547 idx=[4] | net b+bbar: |sum pT|=9.89808e-06 (px=-8.27802e-06 py=5.42645e-06) scalar sum pT=226.442
  stage 2 (partonShower): nParticles=14 | b quark (status 52): pt=91.5874 eta=-1.57249 phi=-2.08354 idx=[182] | #bar{b} quark (status 52): pt=118.574 eta=-3.72888 phi=1.01942 idx=[207] | net b+bbar: |sum pT|=27.2853 (px=17.1858 py=21.1929) scalar sum pT=210.162
  stage 3 (mpiBeamRemnants): nParticles=169 | b quark (status 1): pt=91.7437 eta=-1.56979 phi=-2.08372 idx=[515] | #bar{b} quark (status 1): pt=116.356 eta=-3.74787 phi=1.01139 idx=[514] | net b+bbar: |sum pT|=25.0768 (px=16.727 py=18.6829) scalar sum pT=208.1
```

### 4.3 The Pythia8 log (`test/02_PS/out/step1_GEN_02_PS.log`)

Version: `This is PYTHIA version 8.311`. The PDF: `NNPDF31_nnlo_as_0118 PDF set, member #0, version 1; LHAPDF ID = 303600` (LHAPDF 6.4.0).

The warning at initialisation (also in the logs of chapters 01 and 04 to 06):

```
 PYTHIA Warning in SimpleTimeShower::init: pTmin too low , raised to 0.611
```

The settings table, "changes only" (Now, Default, Min, Max); the shower-related rows and the switches of the script:

```
 | Beams:eCM                                     |                13600.000 |    14000.000         0.0             |
 | Check:epTolErr                                |                0.0100000 |   1.0000e-04                         |
 | HadronLevel:all                               |                      off |           on                         |
 | HadronLevel:Decay                             |                      off |           on                         |
 | HadronLevel:Hadronize                         |                      off |           on                         |
 | HardQCD:qqbar2bbbar                           |                       on |          off                         |
 | MultipartonInteractions:alphaSorder           |                        2 |            1           0           3 |
 | MultipartonInteractions:alphaSvalue           |                  0.11800 |      0.13000   0.0600000     0.25000 |
 | MultipartonInteractions:ecmPow                |                0.0334400 |      0.21500         0.0     0.50000 |
 | MultipartonInteractions:pT0Ref                |                  1.41000 |      2.28000     0.50000    10.00000 |
 | PartonLevel:MPI                               |                      off |           on                         |
 | PDF:pSet                                      | LHAPDF6:NNPDF31_nnlo_as_0118 |           13                     |
 | PhaseSpace:pTHatMin                           |                100.00000 |          0.0         0.0             |
 | SigmaProcess:alphaSorder                      |                        2 |            1           0           3 |
 | SigmaProcess:alphaSvalue                      |                  0.11800 |      0.13000   0.0600000     0.25000 |
 | SigmaTotal:mode                               |                        0 |            1           0           4 |
 | SigmaTotal:sigmaEl                            |                 21.89000 |     25.00000         0.0             |
 | SigmaTotal:sigmaTot                           |                100.30900 |    100.00000         0.0             |
 | SpaceShower:alphaSorder                       |                        2 |            1           0           3 |
 | SpaceShower:alphaSvalue                       |                  0.11800 |      0.13650   0.0600000     0.25000 |
 | TimeShower:alphaSorder                        |                        2 |            1           0           3 |
 | TimeShower:alphaSvalue                        |                  0.11800 |      0.13650   0.0600000     0.25000 |
```

`PartonLevel:ISR` and `PartonLevel:FSR` do not appear in this table because `on` is their default (in the 01_ME log
they appear as `off`). Parameters at their Pythia 8.311 default, from `$PYTHIA8_BASE/share/Pythia8/xmldoc`
(`PYTHIA8_BASE=/cvmfs/cms.cern.ch/el9_amd64_gcc12/external/pythia8/311-62f197bacb831a8defc23e576ff6b2ab`):
`PartonShowers:model = 1` (MasterSwitches.xml), `TimeShower:pTmin = 0.5` (min 0.1, max 2.0), `TimeShower:pTmaxMatch = 1`,
`TimeShower:recoilToColoured = on`, `TimeShower:dampenBeamRecoil = on` (TimelikeShowers.xml), `SpaceShower:pT0Ref = 2.0`,
`SpaceShower:ecmRef = 7000.0`, `SpaceShower:ecmPow = 0.0`, `SpaceShower:samePTasMPI = off`, `SpaceShower:pTmaxMatch = 0`,
`SpaceShower:rapidityOrder = on`, `SpaceShower:pTmin = 0.2` (SpacelikeShowers.xml), `MultipartonInteractions:ecmRef = 7000.0`
(MultipartonInteractions.xml). `Tune:pp 14` and `Tune:ee 7` are "the Monash 2013 tune by Peter Skands et al." (Tunes.xml).

The Info listing of the event:

```
 In 1: id =   -2, x =  2.174e-03, pdf =  1.145e+00 at Q2 =  1.284e+04.
 In 2: id =    2, x =  3.901e-01, pdf =  2.107e-01 at same Q2.
 Subprocess q qbar -> b bbar with code 124 is 2 -> 2.
 It has sHat =  1.569e+05,    tHat = -1.428e+05,    uHat = -1.409e+04,
       pTHat =  1.132e+02,   m3Hat =  4.800e+00,   m4Hat =  4.800e+00,
    thetaHat =  2.533e+00,  phiHat =  9.905e-01.
     alphaEM =  7.837e-03,  alphaS =  1.143e-01    at Q2 =  1.284e+04.
 Impact parameter b =  0.000e+00 gives enhancement factor =  1.000e+00.
 Max pT scale for MPI =  1.360e+04, ISR =  1.133e+02, FSR =  1.133e+02.
 Number of MPI =     1, ISR =     0, FSRproc =     5, FSRreson =     0.
```

The same lines in the other chapters: 01_ME `Max pT scale for MPI = 1.360e+04, ISR = 1.360e+04, FSR = 1.360e+04`,
`Number of MPI = 1, ISR = 0, FSRproc = 0` (showers off); 04_MPI and 06_Decay `Max pT scale for MPI = 1.133e+02, ISR = 1.133e+02, FSR = 1.133e+02`,
`Number of MPI = 32, ISR = 28, FSRproc = 64, FSRreson = 0`.

The hard-process listing (identical in chapter 01) and the cross section:

```
    no         id  name            status     mothers   daughters     colours      p_x        p_y        p_z         e          m
     3         -2  (ubar)             -21     1     0     5     6     0   101      0.000      0.000     14.785     14.785      0.000
     4          2  (u)                -21     2     0     5     6   102     0      0.000      0.000  -2652.934   2652.934      0.000
     5         -5  bbar                23     3     4     0     0     0   101     62.071     94.690  -2413.001   2415.661      4.800
     6          5  b                   23     3     4     0     0   102     0    -62.071    -94.690   -225.148    252.059      4.800
 | q qbar -> b bbar                               124 |           1          1          1 |   1.577e-07  1.577e-07 |
```

The colour columns show the dipoles of the shower: the b (colour 102) is connected to the incoming u (102), the bbar
(anticolour 101) to the incoming ubar (101).

### 4.4 The full record of `step1_GEN_02_PS.root` (the FWLite snippet of section 3)

```
  i    pdg   st       pT     eta     phi         E  mothers -> daughters
  0   2212    4    0.000     nan   0.000   6800.00  [] -> [21, 30, 31, 32]
  1   2212    4    0.000     nan   0.000   6800.00  [] -> [22, 33]
  2     -2   21    0.000     nan   0.000     14.79  [21] -> [4, 5]
  3      2   21    0.000     nan   0.000   2652.93  [8] -> [4, 5]
  4     -5   23  113.221  -3.753   0.991   2415.66  [2, 3] -> [23]
  5      5   23  113.221  -1.439  -2.151    252.06  [2, 3] -> [6, 7]
  6      5   51   47.701  -1.363  -2.190     99.39  [5] -> [20]
  7     21   51   65.583  -1.503  -2.122    154.75  [5] -> [9, 10]
  8      2   53    0.000     nan   0.000   2655.02  [11] -> [3]
  9     21   51   61.699  -1.508  -2.144    146.15  [7] -> [14]
 10     21   51    4.114  -1.495  -1.797      9.64  [7] -> [12, 13]
 11      2   53    0.000     nan   0.000   2656.06  [22] -> [8]
 12      1   51    2.460  -1.316  -2.190      4.93  [10] -> [26]
 13     -1   51    7.661  -1.567  -1.945     19.15  [10] -> [17]
 14     21   52   55.600  -1.508  -2.144    131.71  [9] -> [15, 16]
 15     21   51   51.941  -1.518  -2.145    124.17  [14] -> [18, 19]
 16     21   51    5.018  -1.421  -2.074     10.99  [14] -> [28]
 17     -1   52    6.282  -1.566  -1.950     15.69  [13] -> [27]
 18     21   51   43.120  -1.525  -2.157    103.74  [15] -> [25]
 19     21   51    9.655  -1.456  -2.096     21.83  [15] -> [29]
 20      5   52   46.879  -1.366  -2.189     97.99  [6] -> [24]
 21     -2   61    0.470   4.142  -2.311     14.79  [0] -> [2]
 22      2   61    1.043  -8.535   0.577   2656.06  [1] -> [11]
 23     -5    1  114.048  -3.745   0.987   2413.90  [4] -> []
 24      5    1   47.036  -1.371  -2.190     98.72  [20] -> []
 25     21    1   43.231  -1.529  -2.158    104.42  [18] -> []
 26      1    1    2.469  -1.321  -2.190      4.97  [12] -> []
 27     -1    1    6.297  -1.570  -1.950     15.79  [17] -> []
 28     21    1    5.033  -1.425  -2.075     11.07  [16] -> []
 29     21    1    9.682  -1.461  -2.097     21.98  [19] -> []
 30   2101    1    0.739   9.665  -0.252   5817.65  [0] -> []
 31      2    1    0.556   7.308   1.338    415.16  [0] -> []
 32      2    1    0.527   7.648  -3.123    552.40  [0] -> []
 33   2101    1    1.043  -8.980  -2.564   4143.94  [1] -> []
```

Status counts: 1: 11, 4: 2, 21: 2, 23: 2, 51: 10, 52: 3, 53: 2, 61: 2 (34 entries; the ledger group "FSR (51-59)" = 10 + 3 + 2 = 15).
The five branchings of table 2.2 are read off the mother/daughter links: 5 -> 6, 7 (recoiler 3 -> 8); 7 -> 9, 10
(recoiler 8 -> 11); 10 -> 12, 13 (recoiler 9 -> 14); 14 -> 15, 16 (recoiler 13 -> 17); 15 -> 18, 19 (recoiler 6 -> 20).
The vector sums of the daughters' pT: 113.22, 65.58, 10.07, 56.95, 52.76 GeV (section 4.5).

The record of `step1_GEN_01_ME.root` for comparison (14 entries):

```
  i    pdg   st       pT     eta     phi         E  mothers -> daughters
  0   2212    4    0.000     nan   0.000   6800.00  [] -> [6, 10, 11, 12]
  1   2212    4    0.000     nan   0.000   6800.00  [] -> [7, 13]
  2     -2   21    0.000     nan   0.000     14.79  [6] -> [4, 5]
  3      2   21    0.000     nan   0.000   2652.93  [7] -> [4, 5]
  4     -5   23  113.221  -3.753   0.991   2415.66  [2, 3] -> [8]
  5      5   23  113.221  -1.439  -2.151    252.06  [2, 3] -> [9]
  6     -2   61    1.370   3.072   2.760     14.82  [0] -> [2]
  7      2   61    0.965  -8.612  -2.305   2652.93  [1] -> [3]
  8     -5    1  112.329  -3.760   0.993   2414.61  [4] -> []
  9      5    1  113.560  -1.440  -2.162    253.15  [5] -> []
 10   2103    1    0.736   9.801  -0.863   6643.28  [0] -> []
 11      2    1    0.413   6.467  -0.853    132.84  [0] -> []
 12      2    1    0.634   3.355   0.604      9.10  [0] -> []
 13   2101    1    0.965  -9.058   0.837   4147.02  [1] -> []
```

### 4.5 Derived numbers (the FWLite snippet of section 3)

```
b system [24, 25, 26, 27, 28, 29]: vector pT=113.56 scalar pT=113.75 E=256.94 mass=13.82
  idx 25 pdg 21 pT=43.23 dR(b)=0.161
  idx 26 pdg 1 pT=2.47 dR(b)=0.050
  idx 27 pdg -1 pT=6.30 dR(b)=0.311
  idx 28 pdg 21 pT=5.03 dR(b)=0.127
  idx 29 pdg 21 pT=9.68 dR(b)=0.129
b system + bbar vector pT = 0.60
first branching: pT(6)=47.70 pT(7)=65.58 |sum|=113.22 m=11.64 dR=0.156
incoming u: E 2652.93 -> 2655.02 -> 2656.06, x 0.39014 -> 0.39060
branching 5 -> 6, 7: pT 113.22 -> 47.70, 65.58 |sum|=113.22
branching 7 -> 9, 10: pT 65.58 -> 61.70, 4.11 |sum|=65.58
branching 10 -> 12, 13: pT 4.11 -> 2.46, 7.66 |sum|=10.07
branching 14 -> 15, 16: pT 55.60 -> 51.94, 5.02 |sum|=56.95
branching 15 -> 18, 19: pT 51.94 -> 43.12, 9.65 |sum|=52.76
shower copies [12, 16, 17, 18, 19, 20]: vector pT=113.22, with the bbar (4): 0.00000; status-1 remnants [30..33]: vector pT=0.60
```

The b system (eta = -1.455, phi = -2.151) is back to back with the bbar: delta phi = 3.139. The sum of all 11
status-1 particles has vector pT = 0.000, sum pz = 0.00 and sum E = 13600.00 GeV.

### 4.6 The ledger table of the chapter (table 2.3)

| quantity | before (01_ME) | after (02_PS) | source |
|---|---|---|---|
| entries in genParticles | 14 | 34 | ledger_*.txt, first line |
| hard process / FSR / ISR / remnants / final state | 4 / 0 / 0 / 2 / 6 | 4 / 15 / 0 / 2 / 11 | ledger_*.txt, "record by status group" |
| beam protons (status 4) | 2 | 2 | 4.4, the records (not printed by the ledger) |
| final-state E / scalar pT [GeV] | 13600.0 / 228.6 | 13600.0 / 230.7 | ledger_*.txt, "final state (status 1)" |
| abs(eta) < 2.5: n / E / pT | 1 / 253.1 / 113.6 | 6 / 256.9 / 113.7 | ledger_*.txt, "by region" |
| 3.0 < abs(eta) < 5.2: n / E / pT | 2 / 2423.7 / 113.0 | 1 / 2413.9 / 114.0 | ledger_*.txt |
| abs(eta) > 5.5: n / E / pT | 3 / 10923.1 / 2.1 | 4 / 10929.2 / 2.9 | ledger_*.txt |
| visible: n / E | 3 / 2676.9 | 7 / 2670.8 | ledger_*.txt, "visible" |
| b quark pT [GeV] | 113.221 (status 23) | 46.8792 (status 52) | genPartAnalyzer_*.log |
| bbar quark pT [GeV] | 113.221 (status 23) | 113.221 (status 23) | genPartAnalyzer_*.log |
| b system vector pT [GeV], status 1 | 113.560 (the b alone, index 9 of the 01_ME record) | 113.56 | 4.4, 4.5 |
| b cone Delta R < 0.4: n / vector pT [GeV] | 1 / 113.6 (113.560) | 6 / 113.6 (113.555) | ledger_*.txt, "cone"; three decimals FWLite |
| bbar cone Delta R < 0.4: n / vector pT [GeV] | 1 / 112.3 (112.329) | 1 / 114.0 (114.048) | ledger_*.txt, "cone"; three decimals FWLite |
| net b+bbar [GeV] | 9.9e-06 | 66.4005 | genPartAnalyzer_*.log |
| scalar pT(b) + pT(bbar) [GeV] | 226.442 | 160.1 | genPartAnalyzer_*.log |

The difference column of table 2.3 is computed before rounding, from the same sums with three decimals (FWLite on
the status-1 particles of the two records, with the regions of `genLedger.py`): scalar pT of the final state
228.637 -> 230.662 (+2.024); |eta| < 2.5: E 253.145 -> 256.940 (+3.795), pT 113.560 -> 113.749 (+0.188);
3.0-5.2: E 2423.707 -> 2413.905 (-9.802), pT 112.963 -> 114.048 (+1.085); > 5.5: E 10923.144 -> 10929.153 (+6.009),
pT 2.114 -> 2.865 (+0.751); visible E 2676.852 -> 2670.845 (-6.007); b system 113.560 -> 113.555 (-0.005); b cone the same members as the b
system, 113.560 -> 113.555 (-0.005); bbar cone (the bbar alone in both) 112.329 -> 114.048 (+1.719), the primordial
kT of the re-drawn beam remnants. The
vector sum of the b system before the beam remnants (the shower copies, 113.22 GeV, section 4.5) equals the pT of
the hard b: the 0.6 GeV of the status-1 system against the bbar is the primordial kT, balanced by the remnants.

### 4.7 The 1000 events of the cross-section job (`test/02_PS/out/showerStats_02_PS.txt`, `xsec_GEN_02_PS.log`)

`showerStats_02_PS.txt` (genShowerStats.py on `xsec_GEN_02_PS.root`):

```
== genShowerStats xsec_GEN_02_PS.root: 1000 events
ISR branchings per event (status 43)     mean 3.34, none in 22 events (2.2%)
  distribution (value: events): 0: 22, 1: 101, 2: 203, 3: 220, 4: 226, 5: 133, 6: 69, 7: 23, 8: 2, 9: 1
FSR branchings per event (status 51 / 2) mean 13.99, none in 1 events (0.1%)
  distribution (value: events): 0: 1, 1: 10, 2: 12, 3: 26, 4: 32, 5: 35, 6: 35, 7: 48, 8: 45, 9: 56, 10: 59, 11: 63, 12: 64, 13: 59, 14: 43, 15: 44, 16: 40, 17: 40, 18: 38, 19: 41, 20: 36, 21: 17, 22: 25, 23: 20, 24: 19, 25: 17, 26: 14, 27: 15, 28: 7, 29: 2, 30: 5, 31: 6, 32: 4, 33: 2, 34: 3, 35: 2, 36: 3, 37: 1, 38: 2, 40: 1, 41: 3, 42: 1, 43: 1, 44: 1, 46: 1, 47: 1
b emissions (b -> b g) per event         mean 0.78, none in 355 events (35.5%)
  distribution (value: events): 0: 355, 1: 516, 2: 122, 3: 7
bbar emissions per event                 mean 0.78, none in 354 events (35.4%)
  distribution (value: events): 0: 354, 1: 522, 2: 117, 3: 7
b and bbar both without emission: 124 events (12.4%)
events without ISR: 22: 1:1:1 1:1:28 1:1:38 1:1:72 1:1:119 1:1:274 1:1:290 1:1:305 1:1:314 1:1:330 1:1:408 1:1:486 1:1:555 1:1:563 1:1:616 1:1:656 1:1:657 1:1:689 1:1:812 1:1:863 1:1:957 1:1:993
the event of the book 1:1:1: ISR 0, FSR 5, b emissions 1, bbar emissions 0, x = 0.0022 and 0.3901
events without ISR in bins of xmax (the larger momentum fraction of the two incoming partons):
  xmax in [0.000, 0.100):  587 events,  11 without ISR (1.9%), mean ISR 3.44
  xmax in [0.100, 0.200):  228 events,   5 without ISR (2.2%), mean ISR 3.22
  xmax in [0.200, 0.300):  113 events,   5 without ISR (4.4%), mean ISR 3.19
  xmax in [0.300, 1.000):   72 events,   1 without ISR (1.4%), mean ISR 3.04
events without ISR in bins of xmin (the smaller momentum fraction of the two incoming partons):
  xmin in [0.000, 0.003):  256 events,   9 without ISR (3.5%), mean ISR 3.07
  xmin in [0.003, 0.010):  429 events,  10 without ISR (2.3%), mean ISR 3.34
  xmin in [0.010, 0.030):  295 events,   3 without ISR (1.0%), mean ISR 3.56
  xmin in [0.030, 1.000):   20 events,   0 without ISR (0.0%), mean ISR 3.30
```

The cross section of the same job (`xsec_GEN_02_PS.log`; chapter 01: 348.6 +- 6.0 pb from 2913 trials):

```
 | q qbar -> b bbar                               124 |        2871       1000       1000 |   3.482e-07  6.127e-09 |
Before Filter: total cross section = 3.482e+02 +- 6.127e+00 pb
After filter: final cross section = 3.482e+02 +- 6.127e+00 pb
After filter: final equivalent lumi for 1M events (1/fb) = 2.872e+00 +- 5.063e-02
```

Quoted in the chapter: mean ISR 3.34 -> 3.3, mean FSR 13.99 -> 14.0; 22 of 1000 events without ISR (2.2%), event
1:1:1 among them; x_max > 0.3: 1 of 72 without ISR (1.4%); bbar without emission 35.4%, b 35.5%, both 12.4%;
sigma 348.2 +- 6.1 pb. ISR is on in this chapter: `PartonLevel:ISR = on` is the Pythia8 default and is therefore
missing from the "changed settings" table of `step1_GEN_02_PS.log`; the Info listing prints `Max pT scale for MPI =
1.360e+04, ISR = 1.133e+02, FSR = 1.133e+02`.

### 4.8 The release files of "Open problems for Phase-2" (`$CMSSW_RELEASE_BASE/src`)

Everything below was read in the release CMSSW_15_0_5 (`$CMSSW_RELEASE_BASE/src` and its Pythia8 8.311 external,
`$PYTHIA8DATA`), not in the CMSSW development branch.

`Configuration/Generator/python/MCTunes2017/PythiaCP5Settings_cfi.py` (imported by the fragment), the full block:

```
'Tune:pp 14', 'Tune:ee 7',
'MultipartonInteractions:ecmPow=0.03344', 'MultipartonInteractions:bProfile=2',
'MultipartonInteractions:pT0Ref=1.41', 'MultipartonInteractions:coreRadius=0.7634',
'MultipartonInteractions:coreFraction=0.63', 'ColourReconnection:range=5.176',
'SigmaTotal:zeroAXB=off',
'SpaceShower:alphaSorder=2', 'SpaceShower:alphaSvalue=0.118',
'SigmaProcess:alphaSvalue=0.118', 'SigmaProcess:alphaSorder=2',
'MultipartonInteractions:alphaSvalue=0.118', 'MultipartonInteractions:alphaSorder=2',
'TimeShower:alphaSorder=2', 'TimeShower:alphaSvalue=0.118',
'SigmaTotal:mode = 0', 'SigmaTotal:sigmaEl = 21.89', 'SigmaTotal:sigmaTot = 100.309',
'PDF:pSet=LHAPDF6:NNPDF31_nnlo_as_0118',
```

The file has no comment, and `grep -rn 'GEN-17-001' Configuration GeneratorInterface` finds nothing: the directory
name `MCTunes2017` is the only date the release gives for the tune.

`diff MCTunes2017/PythiaCP5Settings_cfi.py MCTunesRun3ECM13p6TeV/PythiaCP5Settings_cfi.py`:

```
23,24c23,24
<         'SigmaTotal:sigmaEl = 21.89',
<         'SigmaTotal:sigmaTot = 100.309',
---
>         'SigmaTotal:sigmaEl = 22.08',
>         'SigmaTotal:sigmaTot = 101.037',
```

The MPI cut-off from the formula of `MultipartonInteractions.xml`, pT0 = pT0Ref (ecmNow/ecmRef)^ecmPow with
pT0Ref = 1.41, ecmRef = 7000 (default), ecmPow = 0.03344: 1.41 x (13600/7000)^0.03344 = 1.4417 GeV at 13.6 TeV,
1.41 x 2^0.03344 = 1.4431 GeV at 14 TeV.

The shower defaults that no file sets (`grep -rl 'SpaceShower:pT0Ref\|SpaceShower:ecmPow\|samePTasMPI\|TimeShower:pTmin'
Configuration/Generator/python` finds nothing): Pythia8 `SpacelikeShowers.xml` `SpaceShower:samePTasMPI` default off
(line 245), `SpaceShower:pT0Ref` default 2.0 (line 287), `SpaceShower:ecmRef` 7000.0 (line 297), `SpaceShower:ecmPow`
default 0.0 (line 301); `TimelikeShowers.xml` `TimeShower:pTmin` default 0.5 (line 253); `Tunes.xml` `Tune:pp` default
14 and `Tune:ee` default 7, both "the Monash 2013 tune" (lines 347, 251). `test/02_PS/out/step1_GEN_02_PS.log` line 108:
`PYTHIA Warning in SimpleTimeShower::init: pTmin too low , raised to 0.611` (the same line in every `step1_GEN_*.log` and
`xsec_GEN_*.log` of the book); lines 142-145, the settings table: `SpaceShower:alphaSorder` 2 (default 1),
`SpaceShower:alphaSvalue` 0.11800 (default 0.13650), `TimeShower:alphaSorder` 2 (default 1), `TimeShower:alphaSvalue`
0.11800 (default 0.13650).

Counts in `Configuration/Generator/python/`: 30 files match `*14TeV*CP5*`; 40 files have both `14TeV` and `CP5` in the
name and all 40 import `MCTunes2017.PythiaCP5Settings`; 6 files in the directory import
`MCTunesRun3ECM13p6TeV.PythiaCP5Settings`, 0 of them with `14TeV` in the name. `TTbar_14TeV_TuneCP5_cfi.py`,
`MinBias_14TeV_pythia8_TuneCP5_cfi.py` and `QCD_Pt_80_120_14TeV_TuneCP5_cfi.py` all have
`from Configuration.Generator.MCTunes2017.PythiaCP5Settings_cfi import *` and `comEnergy = cms.double(14000.0)`.
`Configuration/PyReleaseValidation/python/relval_steps.py`, lines 4604-4617:
`key='MinBias_14TeV_pythia8_TuneCP5'+'_'+ds` and, for `'Run4' in ds`,
`PUDataSets[ds]={'-n':10,'--pileup':'AVE_200_BX_25ns','--pileup_input':'das:/RelValMinBias_14TeV/%s/GEN-SIM'%(name,)}`.

`Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py`, `upgradeFragments` (line 3255): 152 fragments,
52 with `14TeV` in the name: 37 import `MCTunes2017.PythiaCP5Settings`, 12 `Pythia8CUEP8M1Settings`
(`Wjet_Pt_80_120`, `Wjet_Pt_3000_3500`, `QCD_Pt_3000_3500`, `QCD_Pt_80_120`, `JpsiMM`, `WE`, `H130GGgluonfusion`,
`PhotonJet_Pt_10`, `QQH1352T_Tauola`, `QCD_Pt_600_800` `_14TeV_TuneCUETP8M1_cfi`, `DYToLL_M-50_14TeV_pythia8_cff`,
`DYToTauTau_M-50_14TeV_pythia8_tauola_cff`), 2 the Pythia6 `PythiaUEZ2starSettings`, 1 is Exhume.
`Configuration/Generator/python/Pythia8CUEP8M1Settings_cfi.py`, the full block:

```
'Tune:pp 14', 'Tune:ee 7',
'MultipartonInteractions:pT0Ref=2.4024', 'MultipartonInteractions:ecmPow=0.25208', 'MultipartonInteractions:expPow=1.6',
```

`Configuration/Generator/python/PSweightsPythia/PythiaPSweightsSettings_cfi.py`, first and last lines:

```
        'UncertaintyBands:doVariations = on',
        ...
        'UncertaintyBands:FSRpTmin2Fac = 20',
        'UncertaintyBands:ISRpTmin2Fac = 20' # for consistency with UL and P8.240 set to 20, to be optimized and changed for Run 3 re-MC
```

Pythia8 `Variations.xml`: `UncertaintyBands:doVariations` default off (line 32), `ISRpTmin2Fac` and `FSRpTmin2Fac`
default 4.0 (lines 81, 88). The block is imported by 6 files of `Configuration/Generator/python/`, 2 of them with
`14TeV` in the name (`HTo2LongLivedTo4bMH-125_MFF-12_CTau-900mm_TuneCP5_14TeV_pythia8_cfi.py`,
`DisplacedSUSY_stopToBottom_M_800_500mm_TuneCP5_14TeV_pythia8_cff.py`); not by the fragment of the book.

`Configuration/Generator/python/Pythia8CommonSettings_cfi.py`, the full block (imported by 262 files of
`Configuration/Generator/python/`):

```
'Tune:preferLHAPDF = 2', 'Main:timesAllowErrors = 10000', 'Check:epTolErr = 0.01',
'Beams:setProductionScalesFromLHEF = off', 'SLHA:minMassSM = 1000.',
'ParticleDecays:limitTau0 = on', 'ParticleDecays:tau0Max = 10', 'ParticleDecays:allowPhotonRadiation = on',
```

Pythia8 `ErrorChecks.xml` line 83: `Check:epTolErr` default 1e-4 (the log, line 115: `0.0100000 | 1.0000e-04`);
`ParticleDecays.xml`: `limitTau0` default off (line 43), `tau0Max` default 10. "expressed in mm/c" (lines 47-48).

The event (B5, last paragraph): `test/02_PS/out/ledger_02_PS.txt` lines 8-9, outgoing -5 at eta = -3.75, 5 at
eta = -1.44; `genPartAnalyzer_02_PS.log`, stage 2, the cone around the b: members b (dR 0), g 0.162, g 0.130,
dbar 0.312, g 0.128, d 0.050 (five partons besides the b, the farthest at 0.31), the cone around the bbar: the bbar
alone; `showerStats_02_PS.txt`: "bbar emissions per event ... none in 354 events (35.4%)".

`IOMC/RandomEngine/python/IOMC_cff.py`: `generator = cms.PSet(initialSeed = cms.untracked.uint32(123456789), engineName = FullSimEngine)`.
