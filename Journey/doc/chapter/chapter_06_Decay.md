# Chapter 06, Decay: technical companion

The technical companion of `doc/chapter/chapter_06_Decay.tex` (Rule C of `Constitution.txt`): the commands of
`test/06_Decay/run_06_Decay.sh`, the walk-through of the configuration it writes, the analyzers, and every
number the chapter quotes, copied from `test/06_Decay/out/`. All commands are run from `test/` of an
initialized CMSSW_15_0_5 (`cd CMSSW_15_0_5/src && cmsenv && cd JitJetSW/Journey/test`). The whole chapter is

```bash
./06_Decay/run_06_Decay.sh          # needs test/05_Had/out/step1_GEN_05_Had.root (Rule E), ~1 min
```

which sources `common/jitjet_common.sh` and calls `DRAW_STAGES=5 gen_stage on on on on on 5 "hadron decays ..."`: the same
generator chapter machinery as chapters 01 to 05, with the fifth switch, `HadronLevel:Decay`, now `on`.

## 1. The cmsDriver.py commands

`gen_stage` runs exactly one `cmsDriver.py` (written to `test/06_Decay/step1_GEN_06_Decay_cfg.py`, its log in
`out/step1_GEN_06_Decay_cfg.driver.log`); the command is recorded in the header of the configuration
(`# with command line options:`):

```bash
cmsDriver.py Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py \
  --conditions auto:phase1_2024_realistic --era Run3_2024 --geometry DB:Extended --beamspot DBrealistic \
  -n 1 -s GEN --datatier GEN --eventcontent FEVTDEBUG --fileout file:step1_GEN_06_Decay.root \
  --customise_commands "process.generator.comEnergy = cms.double(13600.0)
process.generator.PythiaParameters.processParameters.extend(['PhaseSpace:pTHatMin = 100', 'PartonLevel:ISR = on', 'PartonLevel:FSR = on', 'PartonLevel:MPI = on', 'HadronLevel:all = on', 'HadronLevel:Hadronize = on', 'HadronLevel:Decay = on'])" \
  --python_filename /afs/cern.ch/work/r/rverma/public/JME/JitJet/CMSSW_15_0_5/src/JitJetSW/Journey/test/06_Decay/step1_GEN_06_Decay_cfg.py --no_exec
```

(in the script the two customise lines are joined with a literal `\n`, which `cmsDriver.py` expands into two
python lines at the end of the configuration). The arguments:

| argument | meaning |
|---|---|
| `Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py` | the generator fragment: Pythia8 `HardQCD:qqbar2bbbar = on`, `gg2bbbar = off`, `PhaseSpace:pTHatMin = 30.`, with the blocks `pythia8CommonSettings` and `pythia8CP5Settings` imported from `Configuration/Generator`. It lives in `JitJetSW/Configuration/GenProduction/python/`; `jitjet_fragment` symlinks it into `$CMSSW_BASE/src/Configuration/GenProduction/python/`, where `cmsDriver.py` looks for fragments |
| `--conditions auto:phase1_2024_realistic` | the global tag (alignment, calibrations, beam spot) of the 2024 realistic MC; at GEN only the beam spot (`SimBeamSpotObjectsRcd`) is read |
| `--era Run3_2024` | the era modifiers of the 2024 detector (no effect on the generator; kept identical in all chapters so that the configurations differ only in the Pythia switches) |
| `--geometry DB:Extended` | the geometry from the database (loaded by `GeometryRecoDB_cff`; not used at GEN) |
| `--beamspot DBrealistic` | the vertex smearing module `VtxSmearedRealistic_cfi` (`BetafuncEvtVtxGenerator` reading the beam spot from the global tag): the primary vertex of the event is at x = 0.28, y = -0.17, z = 31.1 mm |
| `-n 1` | one event: the book follows ONE event (`EVENTS=1` in `jitjet_common.sh`) |
| `-s GEN` | the step: generation only (`generation_step = Path(pgen)`: the generator, the vertex smearing, `genParticles`, the generator jets and MET) |
| `--datatier GEN --eventcontent FEVTDEBUG` | the data tier label and the output content: `FEVTDEBUG` keeps everything the GEN step produced (the HepMC record, `genParticles`, `ak4GenJets`, `genMetTrue`, ...) |
| `--fileout file:step1_GEN_06_Decay.root` | the output file, written in `test/06_Decay/out/` because `run_driver` runs `cmsRun` there |
| `--customise_commands "..."` | python appended to the configuration: the centre-of-mass energy 13.6 TeV (the fragment says 14 TeV) and the Pythia8 switches of the chapter appended to `processParameters`: `PhaseSpace:pTHatMin = 100` (overriding the 30 GeV of the fragment), ISR, FSR, MPI, `HadronLevel:all`, `HadronLevel:Hadronize` and, new in this chapter, `HadronLevel:Decay = on` |
| `--python_filename ... --no_exec` | where to write the configuration, and do not run it (the script runs `cmsRun` itself, in `out/`) |

The driver log (`out/step1_GEN_06_Decay_cfg.driver.log`) says what `cmsDriver.py` understood:

```
GEN,ENDJOB
We have determined that this is simulation (if not, rerun cmsDriver.py with --data)
with DB:
Step: GEN Spec:
Loading generator fragment from Configuration.GenProduction.BBbar_14TeV_TuneCP5_cfi
Step: ENDJOB Spec:
Config file .../test/06_Decay/step1_GEN_06_Decay_cfg.py created
```

## 2. The cmsRun on the python config

```bash
cd 06_Decay/out && cmsRun ../step1_GEN_06_Decay_cfg.py > step1_GEN_06_Decay.log 2>&1; cd ../..
```

(this is what `run_driver` does; `RUN=0 ./06_Decay/run_06_Decay.sh` writes the configuration without running).
A walk-through of `test/06_Decay/step1_GEN_06_Decay_cfg.py`:

- `process = cms.Process('GEN', Run3_2024)`: the process name `GEN` (it becomes the process label of every
  product in the file) with the 2024 era.
- `process.load(...)`: the standard sequences; the ones that matter here are
  `SimGeneral.HepPDTESSource.pythiapdt_cfi` (the particle table that `genParticles` uses for names and charges),
  `Configuration.StandardSequences.Generator_cff` (the `pgen` sequence: generator, `VtxSmeared`,
  `genParticles`, `genJetParticles`, `recoGenJets`, `recoGenMET`), `IOMC.EventVertexGenerators.VtxSmearedRealistic_cfi`
  (the beam-spot smearing) and `SimGeneral.MixingModule.mixNoPU_cfi` (no pileup at this stage).
- `process.maxEvents.input = 1`, `process.source = EmptySource`: the generator makes the event, nothing is read.
- `process.FEVTDEBUGoutput`: the `PoolOutputModule` with `fileName = file:step1_GEN_06_Decay.root`, keeping only
  events that passed `generation_step` (`SelectEvents`).
- `process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase1_2024_realistic', '')`.
- `process.generator = cms.EDFilter("Pythia8ConcurrentGeneratorFilter", ...)`: the fragment, expanded. The
  three parameter sets, in the order of `parameterSets`:
  - `pythia8CommonSettings` (from `Configuration/Generator/python/Pythia8CommonSettings_cfi.py`):
    `Tune:preferLHAPDF = 2`, `Main:timesAllowErrors = 10000`, `Check:epTolErr = 0.01`,
    `Beams:setProductionScalesFromLHEF = off`, `SLHA:minMassSM = 1000.`, **`ParticleDecays:limitTau0 = on`**,
    **`ParticleDecays:tau0Max = 10`** (mm/c: only species with c tau0 below 10 mm are decayed by Pythia8),
    `ParticleDecays:allowPhotonRadiation = on`.
  - `pythia8CP5Settings` (from `Configuration/Generator/python/MCTunes2017/PythiaCP5Settings_cfi.py`): the CP5
    tune, `Tune:pp 14`, the MPI parameters, `ColourReconnection:range=5.176`, alpha_s = 0.118 at NLO in all
    showers, `SigmaTotal:sigmaEl = 21.89`, `SigmaTotal:sigmaTot = 100.309`, `PDF:pSet=LHAPDF6:NNPDF31_nnlo_as_0118`.
  - `processParameters`: `HardQCD:gg2bbbar = off`, `HardQCD:qqbar2bbbar = on`, `PhaseSpace:pTHatMin = 30.`, and
    the seven strings appended by `--customise_commands` (the last one wins for `pTHatMin`).
  - `comEnergy = 14000.0`, overridden to 13600.0 at the end of the file.
- The paths and the schedule: `generation_step`, `genfiltersummary_step`, `endjob_step`, `FEVTDEBUGoutput_step`;
  every path is prefixed with `ProductionFilterSequence` (the generator filter).
- The random seeds are the defaults of `Configuration.StandardSequences.Services_cff` (`RandomNumberGeneratorService`),
  identical in all generator chapters: the hard process is the same event.

The output file (`edmDumpEventContent file:06_Decay/out/step1_GEN_06_Decay.root`):

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
edm::RandomEngineStates               "randomEngineStateProducer" ""         "GEN"
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

- `generatorSmeared` (`edm::HepMCProduct`): the Pythia8 event in HepMC format after the vertex smearing; this is
  what Geant4 reads in chapter 09. `LHCTransport` is the same product label for the forward-proton transport (empty here).
- `genParticles` (`vector<reco::GenParticle>`, 1846 entries): the HepMC record converted to CMSSW objects, with
  pdgId, status (Pythia8 codes: 21-29 hard process, 31-39 MPI, 41-49 ISR, 51-59 FSR, 61-69 beam remnants, 71-79
  strings, 2 decayed, 1 stable), four-momentum, production vertex (`vx, vy, vz` in cm) and the mother / daughter
  links; `genParticles:xyz0` and `:t0` are the primary vertex position and time; `vector<int> genParticles` the
  HepMC barcodes. This is the collection every analyzer and ledger of the book reads.
- `ak4GenJets`, `ak8GenJets`, the `NoNu` variants, `rho`, `sigma`: the anti-kT generator jets (R = 0.4 and 0.8)
  and the pileup densities, made by the GEN step from the stable particles; chapter 07 is about them.
- `genMetTrue` (all stable particles, including neutrinos: pT = 37.46 GeV at phi = -2.10, the one neutrino of
  the event), `genMetCalo` (without neutrinos and muons, and |eta| < 5: pT = 35.36 GeV).
- `GenEventInfoProduct`: the weight (1), the pTHat (113.2 GeV), the process id (124), the PDF information.

The log (`out/step1_GEN_06_Decay.log`) contains the Pythia8 banner (PYTHIA version 8.311, LHAPDF 6.4.0 loading
`NNPDF31_nnlo_as_0118`), the process initialization, the MPI initialization (`sigmaNonDiffractive = 55.42 mb`,
`pT0 = 1.44`), the table of changed settings (section 4), the particle data table (`no particle data has been
changed from its default value`), the Info listing of the event (32 MPI, 28 ISR, 64 FSR), the hard-process
listing, and the cross-section statistics (`q qbar -> b bbar`, code 124, sigma = 1.577e-07 mb for pTHat > 100 GeV).

## 3. The analyzer configs

Three things are run by `gen_stage` after `cmsRun`, all in `test/06_Decay/out/` (`CONE_R=${CONE_R:-0.4}` in
`run_06_Decay.sh`). Chapter 07 reads `step1_GEN_06_Decay.root`, so the pages with the cones were made on 13 November
2026 with `REGEN=0 ./06_Decay/run_06_Decay.sh`, which keeps the GEN file (md5 `4490ef15...`, unchanged) and redoes only
these three steps:

```bash
cd 06_Decay/out
# 1. GenPartAnalyzer: stages 1-5 analysed (the b / bbar tracking printed to the log), one page drawn, stage 5
cmsRun ../../common/genPartAnalyzer_cfg.py inputFiles=file:step1_GEN_06_Decay.root \
   outputFile=genPartAnalyzer_06_Decay.root maxEvents=1 eventsToProcess=1:1:1 maxStage=5 drawStages=5 \
   pdfFile=genPartAnalyzer_06_Decay.pdf \
   sampleLabel="chapter 06_Decay: hadron decays (the full generator event)|ISR on, FSR on, MPI on|Hadronize on, Decay on|pTHatMin 100 GeV, 13.6 TeV" \
   coneR=0.4 > genPartAnalyzer_06_Decay.log 2>&1
# 2. the ledger (FWLite, no cmsRun)
python3 ../../common/genLedger.py step1_GEN_06_Decay.root --event 1:1:1 > ledger_06_Decay.txt 2> ledger_06_Decay.log
cd ../..
```

- `common/genPartAnalyzer_cfg.py` (`VarParsing`: `inputFiles`, `outputFile`, `maxEvents`, `eventsToProcess`,
  `maxStage`, `drawStages`, `pdfFile`, `sampleLabel`, and `etaMax` = 6, `labelPtMin` = 5 GeV left at their defaults) runs the
  `GenPartAnalyzer` plugin of `JitJetSW/Journey/plugins/GenPartAnalyzer.cc` on `genParticles`. It produces
  - `genPartAnalyzer_06_Decay.pdf`: one page (`drawStages=5`), the eta-phi map of the particles alive at the end of
    stage 5 (+ hadron decays: the 634 status-1 particles), marker size growing with pT, colour = origin;
    the page is figure 6.1 of the chapter (the .tex includes the PDF). Stages 1 (hard process), 2 (+ parton
    shower), 3 (+ MPI and beam remnants) and 4 (+ hadronisation: the 320 primary hadrons) are analysed for the log,
    their pages are those of chapters 01, 04 and 05. On the page the dashed cones Delta R < 0.4 around the two systems
    with their pT (neutrinos not counted), an inset of the fuller cone and a second inset, since the other cone holds
    at least three particles: the b cone (21 particles, 79.1 GeV) bottom right, the bbar cone (18, 115.9 GeV) top
    right, each with the name and pT of its 12 hardest particles within 1.25 x 0.4 of the axis ("+N more" for the
    others), the antineutrino in parentheses; the decay products of a magnified cone are labelled only in its inset;
  - `genPartAnalyzer_06_Decay.root`: the TH2 (eta, phi, weight pT) per stage (`TFileService`);
  - `genPartAnalyzer_06_Decay.log`: the `== GenPartAnalyzer` line and one `stage N` line per stage with the b and
    bbar (quark, then primary hadron, then its stable decay products) pT, eta, phi, indices and the net b + bbar pT.
- `common/genLedger.py` reads the file with FWLite (`DataFormats.FWLite`) and writes `ledger_06_Decay.txt`: the
  record by status group, the hard process, and the final state (status 1) by kind and by region, with the
  visible and invisible energy.
- The decay trees, the species counts, the flight distances and the generator MET quoted in the chapter are not
  produced by the script; they come from short FWLite reads of the same file, reproducible with:

```python
# python3 -I decayChain.py 06_Decay/out/step1_GEN_06_Decay.root   (from test/, after cmsenv)
import sys, math, collections
from DataFormats.FWLite import Events, Handle
h = Handle('std::vector<reco::GenParticle>'); hm = Handle('std::vector<reco::GenMET>')
for ev in Events(sys.argv[1]):
    ev.getByLabel('genParticles', h); P = h.product()
    def index(p):                      # index of a daughter in the collection
        return next(i for i, q in enumerate(P) if q.pdgId() == p.pdgId() and abs(q.pt() - p.pt()) < 1e-9 and abs(q.eta() - p.eta()) < 1e-9)
    def v(p): return (p.vx() * 10, p.vy() * 10, p.vz() * 10)          # mm
    def dist(a, b): return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))
    def tree(i, depth=0):
        p = P[i]; d = v(p)
        print('%s[%d] pdg=%d st=%d pt=%.2f eta=%.2f E=%.2f r=%.3f mm z=%.3f mm' % ('  ' * depth, i, p.pdgId(), p.status(), p.pt(), p.eta(), p.energy(), math.hypot(d[0], d[1]), d[2]))
        for k in range(p.numberOfDaughters()): tree(index(p.daughter(k)), depth + 1)
    tree(829); tree(968)                                               # the B- and the B*+
    for i in (829, 1503, 1817):                                        # flight paths: B-, B+, D0bar
        p = P[i]; print(i, p.pdgId(), 'gamma*beta=%.1f flight=%.3f mm' % (p.p() / p.mass(), dist(v(p), v(p.daughter(0)))))
    print('status 2:', collections.Counter(p.pdgId() for p in P if p.status() == 2).most_common())
    print('status 1:', collections.Counter(p.pdgId() for p in P if p.status() == 1).most_common())
    print('photons from pi0:', sum(1 for p in P if p.status() == 1 and p.pdgId() == 22 and p.mother(0).pdgId() == 111))
    ev.getByLabel('genMetTrue', hm); m = hm.product()[0]; print('genMetTrue pt=%.2f phi=%.2f' % (m.pt(), m.phi()))
    break
```

## 4. The tables

### 4.1 The ledger, `test/06_Decay/out/ledger_06_Decay.txt` (after the decays)

```
== genLedger step1_GEN_06_Decay.root event 1:1:1: 1846 particles in genParticles
record by status group:
  hard process (21-29)            4
  MPI (31-39)                   124
  ISR (41-49)                   190
  FSR (51-59)                   192
  beam remnants (61-69)         233
  strings (71-79)               165
  decayed (status 2)            302
  final state (status 1)        634
hard process: incoming pdg -2 (E=14.8 GeV, x=0.0022) + pdg 2 (E=2652.9 GeV, x=0.3901) -> sqrt(s_hat) = 396.1 GeV
  outgoing pdg -5: pT=113.2 eta=-3.75 phi=0.99 E=2415.7 GeV
  outgoing pdg 5: pT=113.2 eta=-1.44 phi=-2.15 E=252.1 GeV
final state (status 1): n=634 sumE=13600.0 GeV sumPt=528.9 GeV
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
         members (pdg pT dR): 211 30.6 0.01, 310 15.7 0.03, 310 12.9 0.01, 130 11.8 0.07, 22 8.2 0.03, 211 7.5 0.04, 22 6.4 0.06, 22 5.8 0.00, -211 4.3 0.06, 22 2.9 0.07, 22 2.5 0.04, 22 2.3 0.06, -321 1.6 0.18, 22 1.6 0.02, 22 0.8 0.07, 22 0.7 0.13, 22 0.3 0.20, 22 0.1 0.14
    b    axis [824] status 71 pT=91.7 eta=-1.57 phi=-2.08: n=21 E=200.6 GeV vector pT=79.1 GeV scalar pT=79.5 GeV
         members (pdg pT dR): 22 17.7 0.00, 211 13.3 0.03, -211 11.0 0.04, 310 8.0 0.04, 22 5.8 0.09, 11 5.0 0.19, 22 3.5 0.14, 22 3.0 0.16, 22 2.4 0.26, 211 2.1 0.29, 22 2.0 0.35, 22 1.5 0.11, 22 1.4 0.04, 310 0.9 0.25, 211 0.5 0.36, 22 0.5 0.33, 22 0.3 0.21, 22 0.3 0.30, 22 0.2 0.08, -211 0.2 0.27, -211 0.1 0.28
```

### 4.2 The ledger before the decays, `test/05_Had/out/ledger_05_Had.txt` (the "before" column of table 6.2)

```
== genLedger step1_GEN_05_Had.root event 1:1:1: 1230 particles in genParticles
record by status group:
  hard process (21-29)            4
  MPI (31-39)                   124
  ISR (41-49)                   190
  FSR (51-59)                   192
  beam remnants (61-69)         233
  strings (71-79)               165
  final state (status 1)        320
hard process: incoming pdg -2 (E=14.8 GeV, x=0.0022) + pdg 2 (E=2652.9 GeV, x=0.3901) -> sqrt(s_hat) = 396.1 GeV
  outgoing pdg -5: pT=113.2 eta=-3.75 phi=0.99 E=2415.7 GeV
  outgoing pdg 5: pT=113.2 eta=-1.44 phi=-2.15 E=252.1 GeV
final state (status 1): n=320 sumE=13600.0 GeV sumPt=505.7 GeV
  by kind                 n    E[GeV]  pT[GeV]
  charged hadron        169    8863.8    355.0
  neutral hadron        151    4736.2    150.7
  by region               n    E[GeV]  pT[GeV]     E_nu
  |eta|<2.5             141     684.6    259.9      0.0
  2.5-3.0                30     196.8     24.6      0.0
  3.0-5.2                94    4897.9    192.4      0.0
  5.2-5.5                15     810.7      7.9      0.0
  >5.5                   40    7010.1     20.9      0.0
  visible (|eta|<5.2, no neutrinos): n=265 E=5779.2 GeV pT=476.9 GeV; neutrinos: n=0 E=0.0 GeV pT=0.0 GeV
  cone Delta R < 0.4 around the last copy of each outgoing parton (status 1, no neutrinos):
    bbar axis [964] status 71 pT=116.4 eta=-3.75 phi=1.01: n=2 E=2465.3 GeV vector pT=116.0 GeV scalar pT=116.0 GeV
         members (pdg pT dR): 523 114.2 0.00, -323 1.8 0.18
    b    axis [824] status 71 pT=91.7 eta=-1.57 phi=-2.08: n=9 E=290.6 GeV vector pT=114.4 GeV scalar pT=114.7 GeV
         members (pdg pT dR): -521 94.3 0.01, 111 7.2 0.08, 111 6.5 0.15, 111 2.8 0.27, 111 2.0 0.36, 311 0.9 0.25, 111 0.5 0.26, -313 0.3 0.40, -211 0.1 0.28
```

Differences quoted in the chapter: record 1230 -> 1846 (+616); stable 320 -> 634 (+314); decayed 0 -> 302;
sumPt 505.7 -> 528.9 (+23.2); |eta| < 2.5: 684.6 -> 733.8 GeV (+49.2); |eta| >= 5.5: 7010.1 -> 6817.1 GeV (-193.0);
visible 5779.2 -> 5851.0 GeV (+71.8); invisible 0 -> 95.5 GeV. Of the 320 primary hadrons 132 stay stable
(legend of the stage-5 page: "primary hadron (132)"), so 188 decayed; 302 - 188 = 114 decays of decay products;
616 new particles = 502 stable (634 - 132) + 114 decayed.

### 4.3 The analyzer log, `test/06_Decay/out/genPartAnalyzer_06_Decay.log`

```
== GenPartAnalyzer run:lumi:event 1:1:1 nGenParticles=1846 nStatus1=634 hard b idx=5 hard bbar idx=4
  stage 1 (hardProcess): nParticles=2 | b quark (status 23): pt=113.221 eta=-1.43851 phi=-2.15105 idx=[5] | #bar{b} quark (status 23): pt=113.221 eta=-3.75298 phi=0.990547 idx=[4] | net b+bbar: |sum pT|=9.89808e-06 (px=-8.27802e-06 py=5.42645e-06) scalar sum pT=226.442
  by scattering (stage 1): hard process 2, MPI 0 (of 31 interactions), beam remnants 0, none 0 | by origin drawn: hard process 2; (the b and bbar copies are counted under their origin)
  cone dR<0.4 (stage 1) | around b: n=1 pt=113.221 eta=-1.43851 phi=-2.15105 scalar sum pT=113.221 members [idx]pdg:pt:dR:scattering(1 hard, 3 MPI, 6 remnant) = [5]5:113.221:1.11022e-15:1 | around bbar: n=1 pt=113.221 eta=-3.75298 phi=0.990547 scalar sum pT=113.221 members [idx]pdg:pt:dR:scattering(1 hard, 3 MPI, 6 remnant) = [4]-5:113.221:3.06422e-14:1
  stage 2 (partonShower): nParticles=14 | b quark (status 52): pt=91.5874 eta=-1.57249 phi=-2.08354 idx=[182] | #bar{b} quark (status 52): pt=118.574 eta=-3.72888 phi=1.01942 idx=[207] | net b+bbar: |sum pT|=27.2853 (px=17.1858 py=21.1929) scalar sum pT=210.162
  by scattering (stage 2): hard process 14, MPI 0 (of 31 interactions), beam remnants 0, none 0 | by origin drawn: hard process 2; hard-process ISR 1; hard-process FSR 11; (the b and bbar copies are counted under their origin)
  cone dR<0.4 (stage 2) | around b: n=6 pt=120.27 eta=-1.58969 phi=-2.08609 scalar sum pT=120.405 members [idx]pdg:pt:dR:scattering(1 hard, 3 MPI, 6 remnant) = [182]5:91.5874:4.44089e-16:1 [235]21:15.3477:0.0675868:1 [373]21:5.31642:0.295817:1 [214]-3:3.50206:0.195194:1 [236]21:2.91632:0.363979:1 [372]21:1.73549:0.340223:1 | around bbar: n=1 pt=118.574 eta=-3.72888 phi=1.01942 scalar sum pT=118.574 members [idx]pdg:pt:dR:scattering(1 hard, 3 MPI, 6 remnant) = [207]-5:118.574:4.44089e-14:1
  stage 3 (mpiBeamRemnants): nParticles=169 | b quark (status 62): pt=91.7437 eta=-1.56979 phi=-2.08372 idx=[515] | #bar{b} quark (status 62): pt=116.356 eta=-3.74787 phi=1.01139 idx=[514] | net b+bbar: |sum pT|=25.0768 (px=16.727 py=18.6829) scalar sum pT=208.1
  by scattering (stage 3): hard process 14, MPI 142 (of 31 interactions), beam remnants 13, none 0 | by origin drawn: hard process 2; hard-process ISR 1; hard-process FSR 11; MPI parton 61; MPI ISR 25; MPI FSR 56; beam remnant 13; (the b and bbar copies are counted under their origin)
  cone dR<0.4 (stage 3) | around b: n=9 pt=127.253 eta=-1.58967 phi=-2.09225 scalar sum pT=127.526 hard process: n=6 pt=120.48 scalar sum pT=120.616 UE (MPI + remnants): n=3 scalar sum pT=6.9105 members [idx]pdg:pt:dR:scattering(1 hard, 3 MPI, 6 remnant) = [515]5:91.7437:4.44089e-16:1 [520]21:15.3744:0.0674676:1 [519]21:5.33019:0.295186:1 [548]-2:3.72603:0.267234:3 [516]-3:3.50953:0.194818:1 [523]21:2.91884:0.363519:1 [581]21:2.2518:0.386309:3 [527]21:1.73925:0.339568:1 [640]21:0.932667:0.350387:3 | around bbar: n=2 pt=117.121 eta=-3.74775 phi=1.01223 scalar sum pT=117.127 hard process: n=1 pt=116.356 scalar sum pT=116.356 UE (MPI + remnants): n=1 scalar sum pT=0.770667 members [idx]pdg:pt:dR:scattering(1 hard, 3 MPI, 6 remnant) = [514]-5:116.356:5.68434e-14:1 [637]-4:0.770667:0.130228:3
  stage 4: b quark [824] pt=91.7437 -> primary hadron [829] pdg=-521 pt=94.2789 dR=0.00887172
  stage 4: bbar quark [964] pt=116.356 -> primary hadron [968] pdg=523 pt=114.174 dR=0.00134803
  stage 4 (hadronisation): nParticles=320 | b hadron B^{-}: pt=94.2789 eta=-1.56724 phi=-2.09222 idx=[829] | #bar{b} hadron B^{*+}: pt=114.174 eta=-3.74697 phi=1.01039 idx=[968] | net b+bbar: |sum pT|=20.302 (px=13.7251 py=14.9597) scalar sum pT=208.453
  cone dR<0.4 (stage 4) | around b: n=9 pt=114.446 eta=-1.58091 phi=-2.08909 scalar sum pT=114.654 members [idx]pdg:pt:dR = [829]-521:94.2789:2.22045e-16 [830]111:7.16559:0.0763584 [832]111:6.54856:0.154071 [833]111:2.83494:0.260293 [1131]111:1.97535:0.362117 [834]311:0.906323:0.244158 [1147]111:0.496996:0.269733 [959]-313:0.347957:0.389463 [804]-211:0.0993873:0.285037 | around bbar: n=2 pt=115.983 eta=-3.74922 phi=1.00843 scalar sum pT=115.997 members [idx]pdg:pt:dR = [968]523:114.174:5.06262e-14 [967]-323:1.82287:0.178355
  stage 5 (hadronDecays): nParticles=634 | b hadron B^{-} decay products (n=9): pt=94.2789 eta=-1.56724 phi=-2.09222 idx=[1434,1435,1812,1813,1827,1828,1838,1839,...] | #bar{b} hadron B^{*+} decay products (n=16): pt=114.174 eta=-3.74697 phi=1.01039 idx=[1504,1753,1818,1819,1820,1821,1831,1832,...] | net b+bbar: |sum pT|=20.302 (px=13.7251 py=14.9597) scalar sum pT=208.453
  cone dR<0.4 (stage 5) | around b: n=21 pt=79.1207 eta=-1.58026 phi=-2.07815 scalar sum pT=79.467 members [idx]pdg:pt:dR = [1813]22:17.7277:0.00938886 [1828]211:13.3128:0.0251742 [1827]-211:10.9951:0.0438498 [1838]310:7.95839:0.0499624 [1438]22:5.77836:0.0854139 [1435]11:4.95115:0.179882 [1440]22:3.51683:0.147643 [1439]22:3.0324:0.16388 [1442]22:2.3646:0.251945 [1374]211:2.14455:0.29478 [1586]22:1.96827:0.356353 [1840]22:1.46305:0.115927 [1437]22:1.38732:0.0378915 [1236]310:0.906323:0.244158 [1451]211:0.478113:0.355992 [1441]22:0.470439:0.323616 [1839]22:0.301431:0.201592 [1593]22:0.292582:0.310631 [1812]22:0.163679:0.08783 [1348]-211:0.154445:0.274446 [804]-211:0.0993873:0.285037 | around bbar: n=18 pt=115.944 eta=-3.74635 phi=1.00816 scalar sum pT=116.011 members [idx]pdg:pt:dR = [1753]211:30.6144:0.0106927 [1821]310:15.6545:0.0340961 [1841]310:12.873:0.00906361 [1835]130:11.7772:0.0673147 [1842]22:8.23657:0.0256749 [1832]211:7.51836:0.0450077 [1844]22:6.40256:0.0549173 [1818]22:5.76164:0.00230666 [1831]-211:4.27048:0.0579937 [1819]22:2.94065:0.0676758 [1820]22:2.52864:0.0392866 [1845]22:2.2628:0.0541929 [1282]-321:1.64258:0.18046 [1504]22:1.55608:0.023542 [1843]22:0.817201:0.07345 [1836]22:0.696542:0.134685 [1837]22:0.307863:0.198276 [1797]22:0.149714:0.141523
```

The stage-5 display (`genPartAnalyzer_06_Decay.pdf`) header: 634 particles drawn, 38 with |eta| > 6 on the
border; legend: b system (9), bbar system (16), primary hadron (132), decay product (477); net b+bbar
|pT| = 20.30 GeV (px = 13.73, py = 14.96), sum |pT| = 208.45 GeV, Delta phi(b, bbar) = 3.10. The stage-2 and
stage-3 lines are identical to those of `test/05_Had/out/genPartAnalyzer_05_Had.log` (same parton level).

The cones of stage 5 (figure 6.1, table 6.2; the "cone" line above and the cone lines of the two ledgers; unrounded sums
from FWLite, the script of chapter_05_Had.md section 4.3 run as `python3 -I cones0506.py
06_Decay/out/step1_GEN_06_Decay.root 829 968`): b cone 9 hadrons, 290.560 GeV, vector pT 114.446 GeV (chapter 05) -> 21
stable particles, 200.624 GeV, 79.121 GeV (differences +12, -89.936, -35.325); bbar cone 2, 2465.317, 115.983 -> 18,
2457.411, 115.944 (+16, -7.906, -0.039). Members of the b cone (pdg pT dR, "product" = stable descendant of the B-):
22 17.73 0.009 product, [-12 37.46 0.022 product: the antineutrino, not counted], 211 13.31 0.025 product, 22 1.39 0.038,
-211 11.00 0.044 product, 310 7.96 0.050 product, 22 5.78 0.085, 22 0.16 0.088 product, 22 1.46 0.116 product,
22 3.52 0.148, 22 3.03 0.164, 11 4.95 0.180 product, 22 0.30 0.202 product, 310 0.91 0.244, 22 2.37 0.252,
-211 0.15 0.274, -211 0.10 0.285, 211 2.14 0.295, 22 0.29 0.311, 22 0.47 0.324, 211 0.48 0.356, 22 1.97 0.356: 8 of the
9 products (all but the neutrino, the farthest at 0.202) and 13 others, 8 photons, 4 charged pions and a K0S. Their
mothers (FWLite, up the mothers to the first string parton): the K0S 0.906 [1236] from the K0 [834] of the b string,
the pi- 0.099 [804] a primary hadron, the pi+ 2.145 [1374] from the Delta++ [1146] (pT 2.80, Delta R = 0.51 from the
B- in chapter 05, outside the cone then), the pi+ 0.478 [1451] from an eta [878], the pi- 0.154 [1348] from a rho0
[1108], the photons 1.968 [1586] and 0.293 [1593] from the pi0 [1131] and [1147] that were in the chapter-05 cone.
Members of the bbar cone: the 16 products of the B*+, all within Delta R = 0.198 and 14 within 0.073 (dR 0.002, 0.009,
0.011, 0.024, 0.026, 0.034, 0.039, 0.045, 0.054, 0.055, 0.058, 0.067, 0.068, 0.073, 0.135, 0.198), the K- 1.643
[1282] from the K*- [967] of chapter 05, and a photon 0.150 [1797] from a pi0 of an eta. Just outside the b cone
(0.4-0.6, pT > 1 GeV): pi+ 2.39 at 0.418, pbar 5.30 at 0.453, pi+ 1.99 at 0.462.

### 4.4 The decay trees of the b hadrons (FWLite on `step1_GEN_06_Decay.root`, section 3 script)

Indices are positions in `genParticles`; E, pT in GeV; r and z the production vertex in mm (the primary vertex
is at x = 0.277, y = -0.169, z = 31.119 mm).

```
=== decay tree of [829] B-
[829] B-         st=2 pt=94.28 eta=-1.57 E=235.85 vtx r=0.324 mm z=31.119 mm
  [1434] nu_ebar    st=1 pt=37.46 eta=-1.59 E=95.51 vtx r=3.155 mm z=23.944 mm
  [1435] e-         st=1 pt=4.95 eta=-1.43 E=10.95 vtx r=3.155 mm z=23.944 mm
  [1436] D0*0       st=2 pt=51.91 eta=-1.56 E=129.40 vtx r=3.155 mm z=23.944 mm      (pdg 10421, D*_0(2300)^0)
    [1725] D0         st=2 pt=34.02 eta=-1.56 E=84.73 vtx r=3.155 mm z=23.944 mm
      [1810] K*-        st=2 pt=18.95 eta=-1.56 E=47.27 vtx r=3.196 mm z=23.852 mm
        [1826] K0bar      st=2 pt=7.96 eta=-1.61 E=20.63 vtx r=3.196 mm z=23.852 mm
          [1838] K0S        st=1 pt=7.96 eta=-1.61 E=20.63 vtx r=3.196 mm z=23.852 mm
        [1827] pi-        st=1 pt=11.00 eta=-1.53 E=26.64 vtx r=3.196 mm z=23.852 mm
      [1811] rho+       st=2 pt=15.07 eta=-1.56 E=37.46 vtx r=3.196 mm z=23.852 mm
        [1828] pi+        st=1 pt=13.31 eta=-1.54 E=32.62 vtx r=3.196 mm z=23.852 mm
        [1829] pi0        st=2 pt=1.76 eta=-1.67 E=4.85 vtx r=3.196 mm z=23.852 mm
          [1839] gamma      st=1 pt=0.30 eta=-1.60 E=0.78 vtx r=3.196 mm z=23.851 mm
          [1840] gamma      st=1 pt=1.46 eta=-1.68 E=4.07 vtx r=3.196 mm z=23.851 mm
    [1726] pi0        st=2 pt=17.89 eta=-1.57 E=44.67 vtx r=3.155 mm z=23.944 mm
      [1812] gamma      st=1 pt=0.16 eta=-1.52 E=0.39 vtx r=3.158 mm z=23.938 mm
      [1813] gamma      st=1 pt=17.73 eta=-1.57 E=44.27 vtx r=3.158 mm z=23.938 mm
stable descendants: n=9 sumE=235.85 sumPt(vector)=94.28
   {'nu_ebar': 1, 'e-': 1, 'K0S': 1, 'pi-': 1, 'pi+': 1, 'gamma': 4}
  invisible E = 95.51

=== decay tree of [968] B*+
[968] B*+        st=2 pt=114.17 eta=-3.75 E=2421.40 vtx r=0.324 mm z=31.119 mm
  [1503] B+         st=2 pt=112.62 eta=-3.75 E=2388.20 vtx r=0.324 mm z=31.119 mm
    [1752] D*0bar     st=2 pt=48.13 eta=-3.76 E=1033.83 vtx r=5.544 mm z=-86.041 mm
      [1817] D0bar      st=2 pt=42.37 eta=-3.76 E=911.91 vtx r=5.544 mm z=-86.041 mm
        [1830] K0bar      st=2 pt=12.87 eta=-3.76 E=275.47 vtx r=12.077 mm z=-226.582 mm
          [1841] K0S        st=1 pt=12.87 eta=-3.76 E=275.47 vtx r=12.077 mm z=-226.583 mm
        [1831] pi-        st=1 pt=4.27 eta=-3.80 E=95.92 vtx r=12.077 mm z=-226.582 mm
        [1832] pi+        st=1 pt=7.52 eta=-3.78 E=164.63 vtx r=12.077 mm z=-226.582 mm
        [1833] pi0        st=2 pt=9.05 eta=-3.77 E=197.23 vtx r=12.077 mm z=-226.582 mm
          [1842] gamma      st=1 pt=8.24 eta=-3.77 E=178.99 vtx r=12.077 mm z=-226.589 mm
          [1843] gamma      st=1 pt=0.82 eta=-3.80 E=18.24 vtx r=12.077 mm z=-226.589 mm
        [1834] pi0        st=2 pt=8.66 eta=-3.72 E=178.66 vtx r=12.077 mm z=-226.582 mm
          [1844] gamma      st=1 pt=6.40 eta=-3.73 E=132.89 vtx r=12.077 mm z=-226.584 mm
          [1845] gamma      st=1 pt=2.26 eta=-3.70 E=45.77 vtx r=12.077 mm z=-226.584 mm
      [1818] gamma      st=1 pt=5.76 eta=-3.74 E=121.92 vtx r=5.544 mm z=-86.041 mm
    [1753] pi+        st=1 pt=30.61 eta=-3.74 E=642.92 vtx r=5.544 mm z=-86.041 mm
    [1754] pi0        st=2 pt=5.47 eta=-3.70 E=110.29 vtx r=5.544 mm z=-86.041 mm
      [1819] gamma      st=1 pt=2.94 eta=-3.68 E=58.38 vtx r=5.546 mm z=-86.078 mm
      [1820] gamma      st=1 pt=2.53 eta=-3.71 E=51.91 vtx r=5.546 mm z=-86.078 mm
    [1755] K0         st=2 pt=15.65 eta=-3.77 E=338.45 vtx r=5.544 mm z=-86.041 mm
      [1821] K0S        st=1 pt=15.65 eta=-3.77 E=338.45 vtx r=5.544 mm z=-86.041 mm
    [1756] K*0bar     st=2 pt=12.77 eta=-3.72 E=262.71 vtx r=5.544 mm z=-86.041 mm
      [1822] K0bar      st=2 pt=11.78 eta=-3.71 E=240.99 vtx r=5.544 mm z=-86.041 mm
        [1835] K0L        st=1 pt=11.78 eta=-3.71 E=240.99 vtx r=5.544 mm z=-86.041 mm
      [1823] pi0        st=2 pt=1.00 eta=-3.77 E=21.73 vtx r=5.544 mm z=-86.041 mm
        [1836] gamma      st=1 pt=0.70 eta=-3.85 E=16.32 vtx r=5.545 mm z=-86.046 mm
        [1837] gamma      st=1 pt=0.31 eta=-3.56 E=5.41 vtx r=5.545 mm z=-86.046 mm
  [1504] gamma      st=1 pt=1.56 eta=-3.75 E=33.20 vtx r=0.324 mm z=31.119 mm
stable descendants: n=16 sumE=2421.40 sumPt(vector)=114.17
   {'K0S': 2, 'pi-': 1, 'pi+': 2, 'gamma': 10, 'K0L': 1}
  invisible E = 0.00
```

Neutrino fraction of the B-: 95.51 / 235.85 = 40.5% of the energy, 37.46 / 94.28 = 39.7% of the pT.
Generator MET: `genMetTrue` pt = 37.46 GeV, phi = -2.10, sumEt = 491.5 GeV; `genMetCalo` pt = 35.36 GeV,
phi = -1.92, sumEt = 460.6 GeV.

### 4.5 Flight paths (FWLite, distance between the production vertex of the particle and that of its daughters)

```
primary vertex (mm): x=0.277 y=-0.169 z=31.119
[829]  pdg=-521 p=235.8  m=5.279 gamma*beta=44.7   decays at d=7.828 mm from PV,   flight=7.828 mm   (r_decay=3.155 mm)
[1503] pdg=521  p=2388.2 m=5.279 gamma*beta=452.4  decays at d=117.290 mm from PV, flight=117.290 mm (r_decay=5.544 mm)
[1725] pdg=421  p=84.7   m=1.865 gamma*beta=45.4   produced at 7.828 mm, decays at 7.928 mm, flight=0.100 mm
[1817] pdg=-421 p=911.9  m=1.865 gamma*beta=489.0  produced at 117.290 mm, decays at 257.983 mm, flight=140.693 mm (r_decay=12.077 mm, z=-226.58 mm)
largest flight of a Pythia-decayed particle: 140.69 mm [1817] pdg -421
status 2 decays: 302; flight >0.01 mm: 17, >1 mm: 4, >10 mm: 2
```

### 4.6 Species counts (FWLite), table 6.1 and figure 6.2

Status 2 (decayed by Pythia8), 302 in total:

```
pi0 131 | rho0 25, rho+ 21, rho- 14 (60) | K0bar 21, K0 12 (33) | omega 18 | eta 17 | K*- 7, K*0bar 6, K*0 4, K*+ 2 (19)
eta' 3 | phi 1 | Delta0 2, Delta-bar 1, Delta++ 1, Delta- 1, Sigma*-bar 1, Sigma*0 1, Sigma*+bar 1, Sigma*0bar 1, Sigma0bar 1 (10)
D0bar 2, D0 2, D*+ 1, D0*0 1, D*0bar 1 (7) | B- 1, B*+ 1, B+ 1 (3)
```

(the K*, omega, eta, eta', phi group of table 6.1 is 19 + 18 + 17 + 3 + 1 = 58.)

Status 1 (stable), 634 in total:

```
gamma 273 | pi- 134, pi+ 127 (261) | K0L 22 | K+ 16, K- 13 (29) | K0S 13 | p 8, pbar 3 (11) | n 7, nbar 6 (13)
Lambdabar 5, Lambda 3 (8) | Sigma-bar 2 | nu_ebar 1 | e- 1
```

(the two `Sigma-bar` are pdg -3112, charge +1, Pythia8's `Sigmabar+`: the antiparticle of the Sigma-, c tau0 = 44.34 mm;
the `Delta-bar` of the status-2 list is pdg -1114.)

Photons: 273 in total, E = 2875.9 GeV; 262 from pi0 (E = 2703.2 GeV); 8 from eta; 3 others (one each from the
B*+, a D*0bar and a Sigma0bar). Photons + charged pions = 534 / 634 = 84%; photon energy / 13600 = 21%.

### 4.7 The Pythia8 settings and lifetimes

From `out/step1_GEN_06_Decay.log`, the "Settings (changes only)" table (the lines relevant to this chapter; the
full table has the CP5 tune, `Beams:eCM = 13600`, `PhaseSpace:pTHatMin = 100`):

```
 | Check:epTolErr                                |                0.0100000 |   1.0000e-04                         |
 | Main:timesAllowErrors                         |                    10000 |           10           0             |
 | ParticleDecays:allowPhotonRadiation           |                       on |          off                         |
 | ParticleDecays:limitTau0                      |                       on |          off                         |
 | SLHA:minMassSM                                |                 1000.000 |    100.00000                         |
 | SigmaTotal:sigmaEl                            |                 21.89000 |     25.00000         0.0             |
 | SigmaTotal:sigmaTot                           |                100.30900 |    100.00000         0.0             |
 no particle data has been changed from its default value
```

`ParticleDecays:tau0Max = 10` does not appear in the table because 10 mm is Pythia8's default
(`share/Pythia8/xmldoc/ParticleDecays.xml`: `<parm name="ParticleDecays:tau0Max" default="10." min="0.">`);
`Tune:preferLHAPDF = 2` and `Beams:setProductionScalesFromLHEF = off` are defaults too (`Tunes.xml`, `BeamParameters.xml`).

Lifetimes from `share/Pythia8/xmldoc/ParticleData.xml` of Pythia 8.311 (`tau0` in mm; a resonance has `mWidth` and
no `tau0`):

```
   111 pi0      tau0=2.55313e-05       421 D0       tau0=0.1229         310 K_S0     tau0=26.842
   221 eta      tau0=1.51e-07          411 D+       tau0=0.3118         130 K_L0     tau0=15330
   113 rho0     mWidth=0.14910         431 D_s+     tau0=0.1499        3122 Lambda0  tau0=78.9
   223 omega    mWidth=0.00849         413 D*+      tau0=2.36e-09      3222 Sigma+   tau0=24.04
   331 eta'     mWidth=0.00020         423 D*0      (none)             3112 Sigma-   tau0=44.34
   333 phi      mWidth=0.00426       10421 D*_00    mWidth=0.27000     3212 Sigma0   tau0=2.22e-08
   311 K0       tau0=1e-6              521 B+       tau0=0.4911        3312 Xi-      tau0=49.1
   211 pi+      tau0=7804.5            511 B0       tau0=0.4587        3322 Xi0      tau0=87.1
   321 K+       tau0=3713              531 B_s0     tau0=0.439         3334 Omega-   tau0=24.61
  2112 n0       tau0=2.655e+14         523 B*+      (none)               13 mu-      tau0=658654
    15 tau-     tau0=0.08711          4122 Lambda_c+ tau0=0.0599
```

### 4.8 The release files quoted in "Open problems for Phase-2"

The state of CMSSW_15_0_5 as installed, not of the development branch. All under `$CMSSW_RELEASE_BASE/src` =
`/cvmfs/cms.cern.ch/el9_amd64_gcc12/cms/cmssw/CMSSW_15_0_5/src` unless said otherwise; versions of the externals from
`$CMSSW_RELEASE_BASE/config/toolbox/el9_amd64_gcc12/tools/selected/`: `pythia8.xml` 311, `evtgen.xml` 2.0.0, `lhapdf.xml`
6.4.0, `cmsswdata.xml` `GeneratorInterface/EvtGenInterface=V02-11-00`.

**Decay tables.**

- The Phase-2 list: `Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py`, `upgradeFragments` (line 3255),
  52 names with `14TeV`. `TTbar_14TeV_TuneCP5_cfi.py`, `MinBias_14TeV_pythia8_TuneCP5_cfi.py` have no `ExternalDecays`
  (`grep -c ExternalDecays` gives 0). The 11 with EvtGen (all with the label `EvtGen130`; `operates_on_particles` empty except
  the last two):

  | fragment | `decay_table` |
  |---|---|
  | `B0ToKstarMuMu_14TeV_TuneCP5_cfi`, `BsToEleEle_14TeV_TuneCP5_cfi`, `BsToJpsiGamma_14TeV_TuneCP5_cfi`, `BsToJpsiPhi_mumuKK_14TeV_TuneCP5_cfi`, `BsToMuMu_14TeV_TuneCP5_cfi` (line 16), `BsToPhiPhi_KKKK_14TeV_TuneCP5_cfi`, `BuMixing_BMuonFilter_forSTEAM_14TeV_TuneCP5_cfi` | `DECAY_2010.DEC` |
  | `BdToKstarEleEle_14TeV_TuneCP5_cfi`, `BuToJPsiPrimeKToJPsiPiPiK_14TeV_TuneCP5_pythia8_cfi`, `Chib1PToUpsilon1SGamma_MuFilter_TuneCP5_14TeV-pythia8_evtgen_cfi` (`(20553,555)`), `ChicToJpsiGamma_MuFilter_TuneCP5_14TeV_pythia8_evtgen_cfi` (`(20443,445)`) | `DECAY_2014_NOLONGLIFE.DEC` |

- Active `decay_table` lines in `Configuration/Generator/python` (39 files):
  `grep -rhE "^[^#]*decay_table *= *cms" --include=*.py . | grep -oE "DECAY[A-Za-z0-9_]*\.DEC" | sort | uniq -c`:
  23 `DECAY_2010.DEC`, 8 `DECAY_2014_NOLONGLIFE.DEC`, 8 `DECAY_NOLONGLIFE.DEC`; `grep -rln "DECAY_2020\|evt_2020"` over the
  whole release (`*.py`, `*.cc`, `*.h`, `*.txt`) finds nothing. Example,
  `Hadronizer_TuneCP5_13TeV_aMCatNLO_FXFX_5f_max2j_max1p_LHE_pythia8_evtgen_cff.py` lines 13-21 (workflow 523 of
  `relval_extendedgen.py`, line 56): `ExternalDecays = cms.PSet(EvtGen130 = cms.untracked.PSet(decay_table =
  cms.string('GeneratorInterface/EvtGenInterface/data/DECAY_2010.DEC'), particle_property_file =
  cms.FileInPath('GeneratorInterface/EvtGenInterface/data/evt.pdl'), list_forced_decays = cms.vstring(),
  operates_on_particles = cms.vint32()), parameterSets = cms.vstring('EvtGen130'))`.
  `GeneratorInterface/EvtGenInterface/python/EvtGenSetting_cff.py`: `EvtGenExtraParticles` (extra particle definitions
  appended to `processParameters` by the EvtGen fragments).
- `GeneratorInterface/EvtGenInterface/plugins/EvtGen/EvtGenInterface.cc`: line 60 `SetDefault_m_PDGs()` (lines 60-285, 191
  active `m_PDGs.push_back`: `sed -n 60,285p EvtGenInterface.cc | sed 's#//.*##' | grep -c "m_PDGs.push_back"`); line 294
  `decay_table` read with no default; line 299 `use_internal_photos` default `true` (PHOTOS); lines 387-404: an empty
  `operates_on_particles` falls back to `SetDefault_m_PDGs()`; line 678 `DEFINE_EDM_PLUGIN(..., "EvtGen130")`.
  `GeneratorInterface/ExternalDecays/src/ExternalDecayDriver.cc` line 36: `"EvtGen1"` or `"EvtGen130"` create the plugin.
- The tables: `$CMSSW_RELEASE_BASE/external/el9_amd64_gcc12/data/GeneratorInterface/EvtGenInterface` is a link to
  `/cvmfs/cms.cern.ch/el9_amd64_gcc12/cms/data-GeneratorInterface-EvtGenInterface/V02-11-00/GeneratorInterface/EvtGenInterface`
  (`edmFileInPath GeneratorInterface/EvtGenInterface/data/DECAY_2010.DEC` resolves there; V02-12-00 on cvmfs is not the one
  of this release). `GeneratorInterface/EvtGenInterface/data/README` in `src` only says the files moved to the data
  package. In that `data/` directory:

  ```
  diff DECAY_2014.DEC DECAY_2020.DEC                     # only "0a1,20": the EvtGen licence header
  cmp $EVTGEN_BASE/share/EvtGen/DECAY.DEC DECAY_2020.DEC  # identical (EvtGen 2.0.0's own table)
  cmp DECAY.DEC DECAY_2020.DEC; cmp DECAY_NOLONGLIFE.DEC DECAY_2020_NOLONGLIFE.DEC   # identical
  grep -oE "PDG ?20[0-9]{2}" DECAY_2020.DEC | sed 's/ //' | sort | uniq -c
      # PDG2000 2, PDG2004 1, PDG2008 12, PDG2010 1, PDG2011 2018, PDG2014 105, PDG2015 3
  grep -oE "PDG ?20[0-9]{2}" DECAY_2010.DEC | sed 's/ //' | sort | uniq -c
      # PDG2000 2, PDG2004 1, PDG2008 12, PDG2010 1, PDG2011 2235
  ```

  (among all `*.DEC` and `*.dec` files only the user decay file `BdBstohhtoMuMu.dec` names "PDG 2020").
  `DECAY_2020.DEC` line 2620: `Decay B-` / `# Updated to PDG 2008` / `0.0569 D*0 e- anti-nu_e PHOTOS HQET2 1.207 0.920
  1.406 0.853; # 1.0 * PDG2014`; `DECAY_2010.DEC` line 2706: `Decay B-` / `# Updated to PDG 2008` / `0.056800000 D*0 e-
  anti-nu_e PHOTOS HQET 0.77 1.33 0.92; #[Reconstructed PDG2011]`.
- Semileptonic branching fractions, sum of the channels with `e- anti-nu_e` in the `Decay B-` block:

  ```
  awk '/^Decay B-$/{on=1;next} on&&/^Enddecay/{exit} on{sub(/#.*/,""); if($0 ~ / e- / && $0 ~ /anti-nu_e/) s+=$1}
       END{print s}' DECAY_2010.DEC          # 0.0980 (19 channels); DECAY_2014_NOLONGLIFE.DEC, DECAY_2020.DEC: 0.1072
  ```

  Pythia 8.311, `$PYTHIA8DATA/ParticleData.xml` line 2335 (`id="521"`, B+/B-): the channels on for the particle (`onMode`
  1 or 2) sum to 1.000; those with `12 -11` sum to 0.1129, `12 -11 -423` (anti-D*0 e+ nu_e) is 0.0617.

**The lifetime boundary and Geant4.**

- `Configuration/Generator/python/Pythia8CommonSettings_cfi.py` lines 5-12 (the eight lines of `pythia8CommonSettings`,
  section 2); `grep -rl "Pythia8CommonSettings_cfi import" --include=*.py Configuration/Generator/python | wc -l` gives 262
  (63 with `14TeV` in the name). Pythia8 documentation (`$PYTHIA8DATA`): `ParticleDecays.xml` lines 43 (`limitTau0` default
  off), 47 (`tau0Max` default 10, mm/c), 272-276 (`allowPhotonRadiation` default off, "in order to avoid double-counting of
  emissions if you link to an external QED-radiation program"); `ErrorChecks.xml` line 83 (`Check:epTolErr` default 1e-4,
  "as a fraction of the initial energy"); `MainProgramSettings.xml` lines 22-23 ("It is up to you to decide which ones, if
  any, you actually want to use when you write your main program"), line 221 (`Main:timesAllowErrors` default 10); `grep -rn
  timesAllowErrors --include=*.cc --include=*.h $CMSSW_RELEASE_BASE/src` finds nothing; `Tunes.xml` line 256
  (`Tune:preferLHAPDF` default 2); `BeamParameters.xml` line 170 (`Beams:setProductionScalesFromLHEF` default off);
  `SUSYLesHouchesAccord.xml` line 151 (`SLHA:minMassSM` default 100); `TotalCrossSections.xml` lines 88-91
  (`SigmaTotal:mode` 0: "Make your own choices (the "own model"), set as fixed values").
- `Configuration/Generator/python/MCTunes2017/PythiaCP5Settings_cfi.py` lines 22-24: `SigmaTotal:mode = 0`,
  `SigmaTotal:sigmaEl = 21.89`, `SigmaTotal:sigmaTot = 100.309`; `MCTunesRun3ECM13p6TeV/PythiaCP5Settings_cfi.py`:
  `22.08`, `101.037` (all other lines identical). The fragment of the book and the 37 CP5 fragments at 14 TeV of
  `upgradeFragments` import the `MCTunes2017` version.
- `SimG4Core/Application/python/g4SimHits_cfi.py` lines 271-283, the `Generator` PSet: `RDecLenCut = cms.double(2.9), ## (cm)
  the cut on vertex radius` (line 282, set nowhere else in the python of the release), `LDecLenCut = cms.double(30.0), ##
  (cm) decay volume length`, `MinPCut = 0.04`, `MinEtaCut = -5.5`, `MaxEtaCut = 5.5`; `SimG4Core/Application/python/hectorParameter_cfi.py`
  line 4: `EtaCutForHector = cms.double(8.2)`.
- `SimG4Core/Generators/src/Generator.cc`: lines 51-52 (`RDecLenCut` -> `theDecRCut2`), line 56 (`double theDecLenCut =
  p.getParameter<double>("LDecLenCut") * CLHEP::cm;`, a local variable, used only in the printout of line 90), line 87
  (`Z_hector = RDecLenCut * sinh(EtaCutForHector)` = 2.9 cm x sinh(8.2) = 5279 cm), lines 185-187 ("The selection is made
  considering if the partcile with status = 2 have the end_vertex with a radius greater than the radius of beampipe cylinder
  (no requirement on the Z of the vertex is applyed)."), lines 366-369 ("Decay chain outside the fiducial cylinder defined
  by theRDecLenCut are used for Geant4 tracking with predefined decay channel").
- `Validation/RecoTrack/python/TrackingParticleSelectionsForEfficiency_cff.py` lines 29 and 49: `pset.tip = 2.5 # beampipe
  is around 2.0, BPIX1 is at 2.9` (Phase 1); line 36: `# IT1 will be around 3.0 (as in Phase1)` (Phase 2).
- Default Phase-2 geometry: `relval_Run4.py` line 14 `prefixDet = 29600 #update this line when change the default version`
  (Run4D110); `Configuration/Geometry/python/dictRun4Geometry.py` line 1553 (`D110` = `T35` + ...), line 351 (`T35` lists
  `Geometry/TrackerCommonData/data/PhaseII/Tracker_DD4hep_compatible_IT711_2023_05/pixel.xml`); in that file lines
  12376-12397, `DDTrackerAngular` with parent `pixel:ITLayer1`: `ITLayer1RodFlipped` at `Radius = 27.5*mm` (line 12381),
  `ITLayer1RodUnflipped` at `Radius = 32.5*mm` (line 12392); layer 2 at 59 and 64 mm (lines 12425, 12436).
- The decay vertices of the event: section 4.5 above (`[1503] pdg=521 ... r_decay=5.544 mm`, `[1817] pdg=-421 ... r_decay=12.077 mm`).

**Vertex smearing of the Phase-2 relvals.**

- `Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py` line 3171, `'Run4D110'`: `'Geom':
  'ExtendedRun4D110'`, `'GT': 'auto:phase2_realistic_T33'`, `'Era': 'Phase2C17I13M9'`, `'ScenToRun': ['GenSimHLBeamSpot',
  ...]` (no `'BeamSpot'` key; lines 3208 and 3216 set `'DBrealisticHLLHC'` only for `Run4D110GenOnly` and
  `Run4D110SimOnGen`). `relval_steps.py` lines 4665-4672 (`GenSimHLBeamSpot`: `'--conditions' : gt+'_13TeV'`,
  `'--beamspot' : 'DBrealisticHLLHC'`) and 4674-4681 (`GenSimHLBeamSpot14`: `'--conditions' : gt`, same beam spot);
  `relval_upgrade.py` lines 40-42: a fragment with `14TeV` in its name runs `GenSimHLBeamSpot14`.
- `Configuration/AlCa/python/autoCond.py` line 111: `'phase2_realistic' : '150X_mcRun4_realistic_v1'`; lines 138-139 apply
  `autoCondBSHLLHC13TeV`; `autoCondModifiers.py` line 131: the `_13TeV` keys add
  `SimBeamSpotHLLHCObjects_z4p3cm_t193ns_13TeV_nominal_mc_v1` for `SimBeamSpotHLLHCObjectsRcd`.
- `Configuration/StandardSequences/python/VtxSmeared.py` line 4: `'DBrealisticHLLHC'` ->
  `IOMC.EventVertexGenerators.VtxSmearedRealisticHLLHC_cfi` (`HLLHCEvtVtxGenerator`, `readDB = True`, "read parameters from
  GT (SimBeamSpotHLLHCObjectsRcd)"); lines 44-45: `'HLLHC'`, `'HLLHC14TeV'` (python parameters). `IOMC/EventVertexGenerators/src/HLLHCEvtVtxGenerator.cc`
  lines 50-57: the python parameters are read only `if (!readDB_)`. `VtxSmearedParameters_cfi.py` lines 1100-1117:
  `# Parameters for HL-LHC operation at 13TeV`, `HLLHCVtxSmearingParameters`: `EprotonInGeV = 6500.0`,
  `CrossingAngleInurad = 510.0`, `CrabFrequencyInMHz = 400.0`, `BetaCrossingPlaneInm = 0.20`, `BetaSeparationPlaneInm =
  0.20`; `VtxSmearedHLLHC14TeV_cfi.py`: `EprotonInGeV = 7000`. The 2024 beam spot of this chapter:
  `VtxSmearedRealistic_cfi.py` (`BetafuncEvtVtxGenerator`, `readDB = True`, `VtxSmeared.py` line 3, `'DBrealistic'`).
