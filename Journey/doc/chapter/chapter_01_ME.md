# Chapter 01, ME: technical companion

The technical companion of `doc/chapter/chapter_01_ME.tex` (Rule C of `Constitution.txt`): the exact commands of
`test/01_ME/run_01_ME.sh`, a walk-through of the configuration it writes, the analyzers, and every number the chapter
quotes, copied from `test/01_ME/out/`. All commands are run from `test/` of an initialized CMSSW_15_0_5
(`cd CMSSW_15_0_5/src && cmsenv && cd JitJetSW/Journey/test`).

    ./01_ME/run_01_ME.sh              # the whole chapter: cmsDriver.py, cmsRun, GenPartAnalyzer, ledger
    RUN=0 ./01_ME/run_01_ME.sh        # only write 01_ME/step1_GEN_01_ME_cfg.py
    ANALYZE=0 ./01_ME/run_01_ME.sh    # generate, no analyzer / ledger
    EVENTS=10 ./01_ME/run_01_ME.sh    # more events (only for illustration; the book follows event 1:1:1)
    XSEC_EVENTS=10000 ./01_ME/run_01_ME.sh   # the cross-section job with 10000 events instead of 1000

The script sets `CHAPTER=01_ME`, sources `common/jitjet_common.sh` (`jitjet_init`: checks `CMSSW_15_0_5`, sets the 2024
settings `CONDITIONS=auto:phase1_2024_realistic ERA=Run3_2024 GEOMETRY=DB:Extended BEAMSPOT=DBrealistic COMENERGY=13600.0
PTHATMIN=100 EVENTS=1 EVENT=1:1:1`; `jitjet_build`: rebuilds the `JitJetSW/Journey` plugins if they are older than the
sources) and calls `gen_stage off off off off off 1 "the matrix element q qbar -> b bbar"` (ISR, FSR, MPI, Hadronize,
Decay all off; one GenPartAnalyzer page), then `gen_xsec off off off off off`: the same GEN job with `XSEC_EVENTS=1000`
events and no output file, for the cross section (section 1 below, table 4.2b).

## 1. The cmsDriver.py commands

`gen_stage` first makes the fragment visible to `cmsDriver.py` (`jitjet_fragment`): `cmsDriver.py` resolves a fragment
as the python module `Configuration.GenProduction.<name>`, so the fragment of the package is symlinked there and the
python directory is built once:

    mkdir -p $CMSSW_BASE/src/Configuration/GenProduction/python
    ln -s $CMSSW_BASE/src/JitJetSW/Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py \
          $CMSSW_BASE/src/Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py
    (cd $CMSSW_BASE/src/Configuration/GenProduction && scram b python)

Then the one `cmsDriver.py` command of the chapter (the header of `01_ME/step1_GEN_01_ME_cfg.py`, "with command line
options"; `01_ME/out/step1_GEN_01_ME_cfg.driver.log` is its output). The `\n` inside `--customise_commands` is the two
characters backslash and n, which `cmsDriver.py` turns into a line break, so two python statements are appended:

    cmsDriver.py Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py \
      --conditions auto:phase1_2024_realistic --era Run3_2024 --geometry DB:Extended --beamspot DBrealistic \
      -n 1 -s GEN --datatier GEN --eventcontent FEVTDEBUG --fileout file:step1_GEN_01_ME.root \
      --customise_commands "process.generator.comEnergy = cms.double(13600.0)\nprocess.generator.PythiaParameters.processParameters.extend(['PhaseSpace:pTHatMin = 100', 'PartonLevel:ISR = off', 'PartonLevel:FSR = off', 'PartonLevel:MPI = off', 'HadronLevel:all = off', 'HadronLevel:Hadronize = off', 'HadronLevel:Decay = off'])" \
      --python_filename 01_ME/step1_GEN_01_ME_cfg.py --no_exec

| argument | meaning |
|---|---|
| `Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py` | the generator fragment: `Pythia8ConcurrentGeneratorFilter` with `HardQCD:qqbar2bbbar = on`, `HardQCD:gg2bbbar = off`, `PhaseSpace:pTHatMin = 30.`, the blocks `pythia8CommonSettings` and `pythia8CP5Settings`, `comEnergy = 14000.0` |
| `--conditions auto:phase1_2024_realistic` | the global tag, resolved by `Configuration/AlCa/python/autoCond.py` to `150X_mcRun3_2024_realistic_v2`; in a GEN job only the beam-spot record `SimBeamSpotObjectsRcd` is read from it |
| `--era Run3_2024` | the era modifiers (`Configuration/Eras/python/Era_Run3_2024_cff.py`); in the GEN path the only effect is `ctpps_2022` (inherited from `Run3`), which adds the PPS proton transport `LHCTransport` |
| `--geometry DB:Extended` | the detector geometry from the database (`GeometryRecoDB_cff`); loaded, used by no module of the GEN path |
| `--beamspot DBrealistic` | the vertex-smearing module: `Configuration/StandardSequences/python/VtxSmeared.py` maps the key to `IOMC.EventVertexGenerators.VtxSmearedRealistic_cfi` (`BetafuncEvtVtxGenerator`, `readDB = True`); mandatory in a GEN step (`ConfigBuilder.py`: "GEN step always requires to have a VtxSmearing scenario") |
| `-n 1` | one event: run 1, lumi 1, event 1 (`EmptySource`) |
| `-s GEN` | the step: the path `generation_step` = `ProductionFilterSequence` (the generator) + `pgen` of `Configuration/StandardSequences/python/Generator_cff.py` |
| `--datatier GEN --eventcontent FEVTDEBUG` | the data tier written into the file metadata, and the keep list of the output module (`FEVTDEBUGEventContent`: everything the GEN step produces except the unsmeared HepMC record) |
| `--fileout file:step1_GEN_01_ME.root` | the output file (written in `01_ME/out/`, where `cmsRun` is executed) |
| `--customise_commands "..."` | python appended at the end of the configuration: the centre-of-mass energy 13600 GeV instead of the 14000 of the fragment, and seven Pythia8 settings appended to `processParameters`: `PhaseSpace:pTHatMin = 100` (overrides the 30 of the fragment; Pythia8 keeps the last value) and the five switches of the stage |
| `--python_filename ... --no_exec` | write the configuration to `01_ME/step1_GEN_01_ME_cfg.py` and do not run it (`run_driver` runs `cmsRun` afterwards) |

What the script does not pass: random seeds. They are the defaults of `IOMC/RandomEngine/python/IOMC_cff.py`
(`generator` 123456789, `VtxSmeared` 98765432, `LHCTransport` 87654321, engine `MixMaxRng`), identical in every generator
chapter, which is why chapters 01 to 06 share the same hard process.

The second command, for the cross section (`gen_xsec` of `common/jitjet_common.sh`; the header of
`01_ME/xsec_GEN_01_ME_cfg.py`, output `01_ME/out/xsec_GEN_01_ME_cfg.driver.log`, log of `cmsRun`
`01_ME/out/xsec_GEN_01_ME.log`). With one event Pythia8 can only quote sigma +- sigma (157.7 +- 157.7 pb), so the same
fragment, settings and seeds are run for `XSEC_EVENTS` (default 1000) events; `--no_output` drops the output module,
so only the log is kept, and `step1_GEN_01_ME.root` is not touched. Its first event is event 1:1:1 of the book (same
"PYTHIA Info Listing" and "Event Listing (hard process)" as `step1_GEN_01_ME.log`):

    cmsDriver.py Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py \
      --conditions auto:phase1_2024_realistic --era Run3_2024 --geometry DB:Extended --beamspot DBrealistic \
      -n 1000 -s GEN --datatier GEN --eventcontent FEVTDEBUG --no_output \
      --customise_commands "<the same two statements as above>" \
      --python_filename 01_ME/xsec_GEN_01_ME_cfg.py --no_exec

## 2. The cmsRun on the python config

    cd 01_ME/out
    cmsRun ../step1_GEN_01_ME_cfg.py > step1_GEN_01_ME.log 2>&1     # about 10 s, dominated by initialisation
    cd ../..

Walk-through of `01_ME/step1_GEN_01_ME_cfg.py` (top to bottom):

- `process = cms.Process('GEN', Run3_2024)`: the process name `GEN` becomes the third field of every product
  (`genParticles::GEN`); the era modifiers are applied to everything loaded afterwards.
- `process.load(...)`: the standard sequences. `Services_cff` (the `RandomNumberGeneratorService` with the seeds above),
  `pythiapdt_cfi` (the particle table), `MessageLogger_cfi`, `EventContent_cff`, `mixNoPU_cfi` (loaded by every MC
  configuration, not run here), `GeometryRecoDB_cff`, `MagneticField_cff`, `Generator_cff` (the `pgen` sequence:
  `randomEngineStateProducer`, `VertexSmearing`, `generatorSmeared`, `genParticles`, `genJetMET`),
  `VtxSmearedRealistic_cfi` (the `VtxSmeared` module), `genFilterSummary_cff`, `EndOfProcess_cff`,
  `FrontierConditions_GlobalTag_cff`.
- `process.maxEvents` (1), `process.source = cms.Source("EmptySource")` (no input: the generator makes the events),
  `process.options` (one thread, one stream).
- `process.FEVTDEBUGoutput`: the `PoolOutputModule`, `SelectEvents = ['generation_step']` (an event is written only if
  the generator filter accepted it), `dataTier = 'GEN'`, `outputCommands = FEVTDEBUGEventContent`.
- `process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase1_2024_realistic', '')`.
- `process.generator`: the fragment, copied in full (the `Pythia8ConcurrentGeneratorFilter` with its three parameter
  sets `pythia8CommonSettings`, `pythia8CP5Settings`, `processParameters`), and `ProductionFilterSequence`.
- the paths: `generation_step = cms.Path(process.pgen)`, `genfiltersummary_step`, `endjob_step`, `FEVTDEBUGoutput_step`
  (end paths), the `schedule`, and the loop that inserts `ProductionFilterSequence` at the front of every path, so that
  Pythia8 runs first and everything else runs on its output.
- the customisation lines of `--customise_commands` (energy, Pythia8 switches) and the `customiseEarlyDelete`.

What `pgen` runs, in order, on this event (the `cmsRun` log and `edmProvDump file:01_ME/out/step1_GEN_01_ME.root`):

| module | type | what it does here |
|---|---|---|
| `generator` | `Pythia8ConcurrentGeneratorFilter` | Pythia 8.311 with LHAPDF 6.4.0 (`NNPDF31_nnlo_as_0118`, member 0, LHAPDF ID 303600): one `q qbar -> b bbar` event (process code 124), written as `edm::HepMCProduct generator:unsmeared` (not kept in the file) with `GenEventInfoProduct`, `GenRunInfoProduct`, `GenLumiInfo*` |
| `randomEngineStateProducer` | | the state of the random engines, product `randomEngineStateProducer` |
| `VtxSmeared` | `BetafuncEvtVtxGenerator` | moves the vertex of every HepMC vertex to one point drawn from the 2024 luminous region of the global tag (section 4, table "vertex"); product `VtxSmeared` (not kept) |
| `generatorSmeared` | `GeneratorSmearedProducer` | copies `VtxSmeared` under the label `generatorSmeared` (kept: 14 particles, 7 vertices) |
| `genParticles` | `GenParticleProducer` (`src = generatorSmeared`, `saveBarCodes = True`) | the `reco::GenParticle` collection (14 entries), the barcode map (`vector<int>`), `xyz0` (the vertex) and `t0` (the time) |
| `genParticlesForJets`, `genParticlesForJetsNoNu` | `InputGenJetsParticleSelector` | the status-1 particles for the jet clustering (6 here; `NoNu` drops neutrinos, none here) |
| `ak4GenJets`, `ak8GenJets`, `ak4GenJetsNoNu`, `ak8GenJetsNoNu` | `FastjetJetProducer` (anti-kT, R 0.4 / 0.8, `jetPtMin = 3`) | 2 jets each (the b, the bbar); the `rho`, `sigma`, `rhos`, `sigmas` products are the pileup densities (0 here) |
| `genCandidatesForMET`, `genParticlesForMETAllVisible`, `genMetCalo`, `genMetTrue` | `GenMETProducer` | the generator missing transverse momentum, with (`genMetCalo`, `|eta| < 5`) and without the fiducial cut |
| `LHCTransport` | `PPSSimTrackProducer` (era `ctpps_2022`) | the transport of intact protons to the PPS Roman pots; none in this event |
| `genFilterSummary`, `endOfProcess`, `FEVTDEBUGoutput` | | end paths: the filter statistics, the `TriggerResults`, the output file |

The content of the output file, `edmDumpEventContent file:01_ME/out/step1_GEN_01_ME.root` (114675 bytes, 1 event):

    Type                                  Module                      Label      Process
    ---------------------------------------------------------------------------------------
    GenEventInfoProduct                   "generator"                 ""         "GEN"
    ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<float>,ROOT::Math::DefaultCoordinateSystemTag>    "genParticles"              "xyz0"     "GEN"
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

The keep list comes from `GeneratorInterface/Configuration/python/GeneratorInterface_EventContent_cff.py`
(`keep edmHepMCProduct_generatorSmeared_*_*`, `keep edmHepMCProduct_LHCTransport_*_*`, `keep *_genParticles_*_*`,
`keep GenEventInfoProduct_generator_*_*`, ...) plus the GenJet / GenMET keeps; the unsmeared record `generator:unsmeared`
is not kept, which is why `genLedger.py` and the analyzers read `genParticles` (built from `generatorSmeared`).

The log `01_ME/out/step1_GEN_01_ME.log` holds, in order: the Pythia8 banner (twice: the generator is initialised once
for the configuration check and once for the run), the two `LHAPDF 6.4.0 loading` lines, the "PYTHIA Process
Initialization" table (one subprocess, `q qbar -> b bbar`, code 124, estimated maximum 1.004e-06 mb), one warning
(`SimpleTimeShower::init: pTmin too low, raised to 0.611`, harmless: the shower is off), the table of all settings that
differ from the Pythia8 defaults (section 4), the "PYTHIA Info Listing" and the "Event Listing (hard process)" of event 1
(printed because Pythia8's defaults `Next:numberShowInfo = 1` and `Next:numberShowProcess = 1` are left on, while CMSSW
sets `Next:numberShowEvent = 0`), the FastJet banner (the GenJet producers), the "Event and Cross Section Statistics"
(1 tried, 1 selected, 1 accepted, sigma 1.577e-07 mb), and the `GenXsecAnalyzer` summary (157.7 +- 157.7 pb).

To read the file without `cmsRun` (FWLite python; this is how table 1.2 of the chapter and the "genParticles" table of
section 4 were made):

    python3 - <<'EOF'
    from DataFormats.FWLite import Events, Handle
    h = Handle('std::vector<reco::GenParticle>')
    for ev in Events('file:01_ME/out/step1_GEN_01_ME.root'):
        ev.getByLabel('genParticles', h)
        for i, p in enumerate(h.product()):
            print(i, p.pdgId(), p.status(), round(p.energy(), 3), round(p.pt(), 3), round(p.eta(), 3) if p.pt() > 1e-9 else 'inf',
                  round(p.phi(), 3), p.numberOfMothers(), p.numberOfDaughters(), p.vx(), p.vy(), p.vz())
    EOF

## 3. The analyzer configs

`gen_stage` runs the `GenPartAnalyzer` (`plugins/GenPartAnalyzer.cc`, configuration `common/genPartAnalyzer_cfg.py`)
and writes the ledger with `common/genLedger.py` (FWLite). From `test/`:

    cd 01_ME/out
    cmsRun ../../common/genPartAnalyzer_cfg.py inputFiles=file:$PWD/step1_GEN_01_ME.root outputFile=genPartAnalyzer_01_ME.root \
        maxEvents=1 eventsToProcess=1:1:1 maxStage=1 pdfFile=genPartAnalyzer_01_ME.pdf \
        sampleLabel="chapter 01_ME: the matrix element q qbar -> b bbar|ISR off, FSR off, MPI off|Hadronize off, Decay off|pTHatMin 100 GeV, 13.6 TeV" \
        > genPartAnalyzer_01_ME.log 2>&1
    python3 ../../common/genLedger.py step1_GEN_01_ME.root --event 1:1:1 > ledger_01_ME.txt 2> ledger_01_ME.log
    cd ../..

| file in `01_ME/out/` | produced by | content |
|---|---|---|
| `genPartAnalyzer_01_ME.pdf` | GenPartAnalyzer | 1 page (`maxStage=1`): the eta-phi map of the hard process, the b (red) and bbar (blue) with their pT, eta, phi, the net and scalar pT sums, the sample label in the right panel; the figure of the chapter (the .tex includes the PDF) |
| `genPartAnalyzer_01_ME.root` | GenPartAnalyzer (TFileService) | the TH2 (eta, phi, weight pT) of stage 1 |
| `genPartAnalyzer_01_ME.log` | cmsRun | the per-stage b / bbar line (section 4) |
| `ledger_01_ME.txt` | `genLedger.py` | the record by status group, the hard process (x1, x2, sqrt(s_hat), the outgoing partons), the final state by kind and by region (section 4) |
| `ledger_01_ME.log` | `genLedger.py` (stderr) | empty |

The `GenPartAnalyzer` options used: `maxStage=1` (only the hard-process page exists for this file), `pdfFile` (all pages
in one PDF), `sampleLabel` (`|` separates the lines of the right panel), `eventsToProcess=1:1:1` (the event of the
book), `etaMax=6.0` and `labelPtMin=5.0` (defaults: the eta range of the plot, and the pT above which the pT value is
printed next to a marker). The analyzer draws a particle on the page of stage N if it was produced in a stage <= N and
not yet replaced by a later copy; at stage 1 that is the two status-23 quarks (the status-61 and status-1 copies belong
to the beam-remnant treatment and to the final state, which the analyzer assigns to stage 3 and later).

## 4. The tables

Every number quoted in `chapter_01_ME.tex`, with the file it comes from.

### 4.1 The Pythia8 "Info Listing" and "Event Listing (hard process)" (`01_ME/out/step1_GEN_01_ME.log`)

    Beam A: id =   2212, pz =  6.800e+03, e =  6.800e+03, m =  9.383e-01.
    Beam B: id =   2212, pz = -6.800e+03, e =  6.800e+03, m =  9.383e-01.
    In 1: id =   -2, x =  2.174e-03, pdf =  1.145e+00 at Q2 =  1.284e+04.
    In 2: id =    2, x =  3.901e-01, pdf =  2.107e-01 at same Q2.
    Subprocess q qbar -> b bbar with code 124 is 2 -> 2.
    It has sHat =  1.569e+05,    tHat = -1.428e+05,    uHat = -1.409e+04,
          pTHat =  1.132e+02,   m3Hat =  4.800e+00,   m4Hat =  4.800e+00,
       thetaHat =  2.533e+00,  phiHat =  9.905e-01.
        alphaEM =  7.837e-03,  alphaS =  1.143e-01    at Q2 =  1.284e+04.
    Impact parameter b =  0.000e+00 gives enhancement factor =  1.000e+00.
    Max pT scale for MPI =  1.360e+04, ISR =  1.360e+04, FSR =  1.360e+04.
    Number of MPI =     1, ISR =     0, FSRproc =     0, FSRreson =     0.

    no         id  name            status     mothers   daughters     colours      p_x        p_y        p_z         e          m
     0         90  (system)           -11     0     0     0     0     0     0      0.000      0.000      0.000  13600.000  13600.000
     1       2212  (p+)               -12     0     0     3     0     0     0      0.000      0.000   6800.000   6800.000      0.938
     2       2212  (p+)               -12     0     0     4     0     0     0      0.000      0.000  -6800.000   6800.000      0.938
     3         -2  (ubar)             -21     1     0     5     6     0   101      0.000      0.000     14.785     14.785      0.000
     4          2  (u)                -21     2     0     5     6   102     0      0.000      0.000  -2652.934   2652.934      0.000
     5         -5  bbar                23     3     4     0     0     0   101     62.071     94.690  -2413.001   2415.661      4.800
     6          5  b                   23     3     4     0     0   102     0    -62.071    -94.690   -225.148    252.059      4.800
                                   Charge sum:  0.000           Momentum sum:      0.000      0.000  -2638.149   2667.719    396.101

Derived in the chapter: sqrt(sHat) = sqrt(1.569e5) = 396.1 GeV (also the "Momentum sum" mass, 396.101); Q = sqrt(1.284e4)
= 113.3 GeV = sqrt(pTHat^2 + m_b^2) = sqrt(113.22^2 + 4.8^2) (the transverse mass of the b); thetaHat = 2.533 rad =
145.1 degrees; the rapidity of the parton-parton system y = 0.5 ln(x1/x2) = 0.5 ln(2.174e-3 / 0.3901) = -2.595 (the same
from the momentum sum: 0.5 ln((2667.719 - 2638.149) / (2667.719 + 2638.149)) = -2.595); the rapidity of each b quark
in the parton-parton rest frame, with p* = sqrt(sHat/4 - m_b^2) = 198.0 GeV and pz* = p* cos(thetaHat) = -162.4 GeV
for the bbar: y* = 0.5 ln((198.05 - 162.4) / (198.05 + 162.4)) = -1.157 (bbar), +1.157 (b); y_lab = -2.595 -+ 1.157 =
-3.752 (bbar), -1.438 (b) (the same from the listing: 0.5 ln((E+pz)/(E-pz)) = -3.752 and -1.438; the pseudorapidities
-3.753 and -1.439 differ by 0.001); E = pT cosh(eta): 113.22 cosh(1.4385) = 252.0 GeV, 113.22 cosh(3.753) = 2415.4 GeV.

The GenEventInfoProduct of the file (FWLite, `generator`): weight 1.0, qScale 113.3225, alphaQCD 0.114255, alphaQED
0.00783681, signalProcessID 124, pdf id1 -2, id2 2, x1 0.00217429, x2 0.390137, scalePDF 113.3225, xPDF1 1.14545,
xPDF2 0.210684.

### 4.2 The Pythia8 settings table (changes only; `step1_GEN_01_ME.log`)

    Beams:eCM                                     13600.000   (default 14000)
    Check:epTolErr                                0.01        (pythia8CommonSettings)
    ColourReconnection:range                      5.176       (CP5)
    HadronLevel:all / Decay / Hadronize           off         (this chapter)
    HardQCD:qqbar2bbbar                           on          (fragment; gg2bbbar stays at its default, off)
    Main:timesAllowErrors                         10000       (pythia8CommonSettings)
    MultipartonInteractions:alphaSorder 2, alphaSvalue 0.118, bProfile 2, coreFraction 0.63, coreRadius 0.7634,
                            ecmPow 0.03344, pT0Ref 1.41      (CP5)
    Next:numberShowEvent                          0           (CMSSW)
    ParticleDecays:allowPhotonRadiation on, limitTau0 on     (pythia8CommonSettings; tau0Max = 10 is the Pythia8 default and is not listed)
    PartonLevel:FSR / ISR / MPI                   off         (this chapter)
    PDF:pSet                                      LHAPDF6:NNPDF31_nnlo_as_0118   (CP5)
    PhaseSpace:pTHatMin                           100.0       (script; the fragment's 30 is overridden)
    SigmaProcess:alphaSorder 2, alphaSvalue 0.118             (CP5)
    SigmaTotal:mode 0, sigmaEl 21.89, sigmaTot 100.309, zeroAXB off   (CP5)
    SLHA:minMassSM                                1000.0      (pythia8CommonSettings)
    SpaceShower / TimeShower: alphaSorder 2, alphaSvalue 0.118          (CP5)

Cross-section statistics: `q qbar -> b bbar  124 | 1 tried, 1 selected, 1 accepted | sigma 1.577e-07 +- 1.577e-07 mb`;
`GenXsecAnalyzer`: "Before Filter: total cross section = 1.577e+02 +- 1.577e+02 pb", "After filter: final equivalent
lumi for 1M events (1/fb) = 6.340e+00". Pythia 8.311 (8 Mar 2024), LHAPDF 6.4.0, FastJet 3.4.1.
The initialisation table ("PYTHIA Process Initialization") gives the estimated maximum `q qbar -> b bbar  124 | 1.004e-06`
mb = 1004 pb; the one trial was kept with the probability 157.7 / 1004 = 16%.

### 4.2b The cross section from 1000 events (`01_ME/out/xsec_GEN_01_ME.log`)

    | q qbar -> b bbar                               124 |        2913       1000       1000 |   3.486e-07  6.035e-09 |
    | sum                                                |        2913       1000       1000 |   3.486e-07  6.035e-09 |

    Before Filter: total cross section = 3.486e+02 +- 6.035e+00 pb
    Filter efficiency (event-level)= (1000) / (1000) = 1.000e+00 +- 0.000e+00    [TO BE USED IN MCM]
    After filter: final cross section = 3.486e+02 +- 6.035e+00 pb
    After filter: final equivalent lumi for 1M events (1/fb) = 2.869e+00 +- 4.975e-02

Derived in the chapter: relative error 6.035 / 348.6 = 1.7%; trials kept times the maximum 1000 / 2913 x 1004 = 345 pb
(within the error of 348.6); one event / 1000 events: 348.6 / 157.7 = 2.2. Only warnings: the two
`SimpleTimeShower::init: pTmin too low` of the configuration check and the run (harmless, the shower is off).

### 4.3 The 14 entries of genParticles (FWLite dump of `step1_GEN_01_ME.root`, event 1:1:1)

      i    pdg   st         px         py         pz          E       pt      eta      phi    nMo    nDa
      0   2212    4      0.000      0.000   6800.000   6800.000    0.000      inf    0.000      0      4
      1   2212    4      0.000      0.000  -6800.000   6800.000    0.000     -inf    0.000      0      2
      2     -2   21      0.000      0.000     14.785     14.785    0.000      inf    0.000      1      2
      3      2   21      0.000      0.000  -2652.934   2652.934    0.000     -inf    0.000      1      2
      4     -5   23     62.071     94.690  -2413.001   2415.661  113.221   -3.753    0.991      2      1
      5      5   23    -62.071    -94.689   -225.148    252.059  113.221   -1.439   -2.151      2      1
      6     -2   61     -1.272      0.510     14.753     14.817    1.370    3.072    2.760      1      1
      7      2   61     -0.647     -0.717  -2652.935   2652.935    0.965   -8.612   -2.305      1      1
      8     -5    1     61.369     94.083  -2411.987   2414.606  112.329   -3.760    0.993      1      0
      9      5    1    -63.287    -94.290   -226.194    253.145  113.560   -1.440   -2.162      1      0
     10   2103    1      0.479     -0.559   6643.285   6643.285    0.736    9.801   -0.863      1      0
     11      2    1      0.271     -0.311    132.840    132.841    0.413    6.467   -0.853      1      0
     12      2    1      0.522      0.360      9.073      9.101    0.634    3.355    0.604      1      0
     13   2101    1      0.647      0.717  -4147.018   4147.018    0.965   -9.058    0.837      1      0

pdg 2103 = ud_1 diquark, 2101 = ud_0 diquark. Checks used in the chapter: primordial kT of the status-61 partons
(-1.272 - 0.647, 0.510 - 0.717) = (-1.919, -0.207) GeV, |kT| = 1.93 GeV; the net pT of the final b + bbar
(-63.287 + 61.369, -94.290 + 94.083) = (-1.918, -0.207) GeV, the same 1.93 GeV; the energies of beam A's partons
14.785 + 6643.285 + 132.841 + 9.101 = 6800.0, of beam B's 2652.934 + 4147.018 = 6800.0; the six status-1 energies
sum to 13600.0 GeV. The vertex of every entry is (0.0277, -0.0169, 3.112) cm.

The vertex and time of the event (`genParticles:xyz0`, `genParticles:t0`, and the HepMC vertices of `generatorSmeared`,
all seven at the same point): xyz0 = (0.027660, -0.016904, 3.11193) cm; t0 = 0.120866 ns; HepMC position
(0.2766, -0.169, 31.119) mm, t = 36.2347 mm (c t, i.e. 0.1209 ns). The 2024 beam-spot payload read by `VtxSmeared`
(`conddb --db pro list 150X_mcRun3_2024_realistic_v2 | grep SimBeamSpot`, then `conddb dump` of the payload):
tag `SimBeamSpot_Realistic25ns13p6TeVEarly2024CollisionVtxSmearingParameters_v1_mc`, record `SimBeamSpotObjectsRcd`,
X0 = 0.0284683 cm, Y0 = -0.016576 cm, Z0 = 0.416038 cm, SigmaZ = 3.6 cm, betastar = 30 cm, emittance = 5.725e-8 cm,
Phi = Alpha = TimeOffset = 0; sigma_xy = sqrt(emittance * betastar) = sqrt(5.725e-8 * 30) cm = 13.1 um; the time width
sigma_t = SigmaZ / c = 3.6 cm / (29.98 cm/ns) = 120 ps (`BetafuncEvtVtxGenerator.cc`: `tmp_sigt = RandGaussQ::shoot(engine, 0., fSigmaZ)`).

### 4.4 The analyzer log (`01_ME/out/genPartAnalyzer_01_ME.log`)

    == GenPartAnalyzer run:lumi:event 1:1:1 nGenParticles=14 nStatus1=6 hard b idx=5 hard bbar idx=4
      stage 1 (hardProcess): nParticles=2 | b quark (status 23): pt=113.221 eta=-1.43851 phi=-2.15105 idx=[5] | #bar{b} quark (status 23): pt=113.221 eta=-3.75298 phi=0.990547 idx=[4] | net b+bbar: |sum pT|=9.89808e-06 (px=-8.27802e-06 py=5.42645e-06) scalar sum pT=226.442

The page itself (`genPartAnalyzer_01_ME.pdf`) prints: b quark (status 23): pT = 113.22 GeV, eta = -1.44,
phi = -2.15; bbar quark (status 23): pT = 113.22 GeV, eta = -3.75, phi = 0.99; net b+bbar 0.00 GeV, sum |pT| = 226.44 GeV,
Delta phi(b, bbar) = 3.14.

### 4.5 The ledger (`01_ME/out/ledger_01_ME.txt`, in full)

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

The ledger table of the chapter: "before" is the two protons (2 particles, 13600 GeV, pT 0, both at |eta| > 5.5);
"after" is the status-1 lines above; the differences are after minus before. 3.0 < |eta| < 5.2 holds the bbar
(2414.6 GeV) and the u quark at eta 3.36 (9.1 GeV); |eta| > 5.5 holds the two diquarks and the u quark at eta 6.47
(6643.3 + 4147.0 + 132.8 = 10923.1 GeV); the scalar pT of the remnants is 0.736 + 0.413 + 0.634 + 0.965 = 2.75 GeV
(228.6 - 225.9). The cone lines (`genLedger.py --cone`, default 0.4) are quoted in chapter 02 (the "before" column
of its cone rows): each quark is alone in its cone here, with the primordial kT of its status-1 copy.

### 4.6 The generator jets and MET of the file (FWLite)

    ak4GenJets      n=2  (pT 113.56, eta -1.44, phi -2.16, E 253.1, 1 constituent), (pT 112.33, eta -3.76, phi 0.99, E 2414.6, 1 constituent)
    ak8GenJets      n=2  the same two jets
    ak4GenJetsNoNu  n=2  the same two jets
    ak8GenJetsNoNu  n=2  the same two jets
    genMetTrue      pt = 4.78e-14 GeV, sumEt = 228.637 GeV
    genMetCalo      pt = 1.405 GeV, phi = -0.109, sumEt = 226.523 GeV   (only |eta| < 5.0: the b, the bbar, the u at eta 3.36)

### 4.7 The release files of "Open problems for Phase-2" (`$CMSSW_RELEASE_BASE/src`, CMSSW_15_0_5)

Everything below was read in the release CMSSW_15_0_5 (`$CMSSW_RELEASE_BASE/src`, its Pythia8 8.311 external in
`$PYTHIA8DATA`, and the global tags named by its `Configuration/AlCa/python/autoCond.py`), not in the CMSSW
development branch.

- `Configuration/Generator/python/Pythia8CommonSettings_cfi.py`: `Tune:preferLHAPDF = 2`, `Main:timesAllowErrors = 10000`,
  `Check:epTolErr = 0.01`, `Beams:setProductionScalesFromLHEF = off`, `SLHA:minMassSM = 1000.`,
  `ParticleDecays:limitTau0 = on`, `ParticleDecays:tau0Max = 10`, `ParticleDecays:allowPhotonRadiation = on`.
  Imported by 262 files of `Configuration/Generator/python/` (`grep -l Pythia8CommonSettings_cfi *.py | wc -l`); no
  `toModify` of it (or of any `generator`) in `Configuration/Generator`, `Configuration/StandardSequences`,
  `GeneratorInterface`.
- `Configuration/Generator/python/MCTunes2017/PythiaCP5Settings_cfi.py`: `Tune:pp 14`, `Tune:ee 7`,
  `MultipartonInteractions:ecmPow=0.03344`, `bProfile=2`, `pT0Ref=1.41`, `coreRadius=0.7634`, `coreFraction=0.63`,
  `ColourReconnection:range=5.176`, `SigmaTotal:zeroAXB=off`, `SpaceShower:alphaSorder=2`, `SpaceShower:alphaSvalue=0.118`,
  `SigmaProcess:alphaSvalue=0.118`, `SigmaProcess:alphaSorder=2`, `MultipartonInteractions:alphaSvalue=0.118`,
  `MultipartonInteractions:alphaSorder=2`, `TimeShower:alphaSorder=2`, `TimeShower:alphaSvalue=0.118`,
  `SigmaTotal:mode = 0`, `SigmaTotal:sigmaEl = 21.89`, `SigmaTotal:sigmaTot = 100.309`, `PDF:pSet=LHAPDF6:NNPDF31_nnlo_as_0118`.
- `Configuration/Generator/python/MCTunesRun3ECM13p6TeV/PythiaCP5Settings_cfi.py`: identical except
  `SigmaTotal:sigmaEl = 22.08`, `SigmaTotal:sigmaTot = 101.037`. The tune directories under `Configuration/Generator/python/`
  are `MCTunes2017` and `MCTunesRun3ECM13p6TeV` only (the other two directories are `Herwig7Settings` and
  `PSweightsPythia`); 40 files have both `14TeV` and `CP5` in the name (30 of them match the glob `*14TeV*CP5*`), all
  40 import `MCTunes2017.PythiaCP5Settings`; 20 match `*14TeV*CUETP8M1*`; 6 files are named `*13p6TeV*`, all importing
  `MCTunesRun3ECM13p6TeV.PythiaCP5Settings_cfi`. pT0 at the two energies:
  1.41 * (13600/7000)^0.03344 = 1.4417 GeV, 1.41 * (14000/7000)^0.03344 = 1.4431 GeV (`MultipartonInteractions:ecmRef`
  default 7000, Pythia8 `MultipartonInteractions.xml`).
- `Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py`, `upgradeFragments` (line 3255): 152
  fragments, 52 with `14TeV` in the name. Tune of the 52 (from their imports): 37 `MCTunes2017.PythiaCP5Settings`;
  12 `Pythia8CUEP8M1Settings` (`Wjet_Pt_80_120`, `Wjet_Pt_3000_3500`, `QCD_Pt_3000_3500`, `QCD_Pt_80_120`, `JpsiMM`, `WE`,
  `H130GGgluonfusion`, `PhotonJet_Pt_10`, `QQH1352T_Tauola`, `QCD_Pt_600_800` `_14TeV_TuneCUETP8M1_cfi`,
  `DYToLL_M-50_14TeV_pythia8_cff`, `DYToTauTau_M-50_14TeV_pythia8_tauola_cff`); 2 `PythiaUEZ2starSettings` with
  `Pythia6GeneratorFilter` (`LM1_sfts_14TeV_cfi`, `H200ChargedTaus_Tauola_14TeV_cfi`); 1 `GluGluTo2Jets_M_300_2000_14TeV_Exhume_cff`.
  `Configuration/Generator/python/Pythia8CUEP8M1Settings_cfi.py`: `Tune:pp 14`, `Tune:ee 7`,
  `MultipartonInteractions:pT0Ref=2.4024`, `MultipartonInteractions:ecmPow=0.25208`, `MultipartonInteractions:expPow=1.6`;
  none of the 22 files with `14TeV` in the name that import `Pythia8CUEP8M1Settings` sets `PDF:pSet`. The Pythia8 defaults are printed next to the CP5 values
  in `test/01_ME/out/step1_GEN_01_ME.log`: line 135 `PDF:pSet` default `13`, line 138 `SigmaProcess:alphaSvalue`
  default `0.13000`; `PDFSelection.xml`: `PDF:pSet` default 13 = "NNPDF2.3 QCD+QED LO alpha_s(M_Z) = 0.130".
  `PDF:pSet` values in `Configuration/Generator/python/` (`grep -rho`): `LHAPDF6:NNPDF30_lo_as_0130` (1),
  `LHAPDF6:NNPDF31_lo_as_0130` (12), `LHAPDF6:NNPDF31_nlo_as_0118` (6), `LHAPDF6:NNPDF31_nnlo_as_0118` (16), `5` (1).
- `Configuration/Generator/python/TTbar_14TeV_TuneCP5_cfi.py`: `comEnergy = cms.double(14000.0)`, the same two blocks;
  the model of the fragment of the book (its header says so).
- `Configuration/StandardSequences/python/VtxSmeared.py`: `'DBrealistic': 'IOMC.EventVertexGenerators.VtxSmearedRealistic_cfi'`,
  `'DBrealisticHLLHC': 'IOMC.EventVertexGenerators.VtxSmearedRealisticHLLHC_cfi'`, `'HLLHC': '...VtxSmearedHLLHC_cfi'`,
  `'HLLHC14TeV': '...VtxSmearedHLLHC14TeV_cfi'`, `VtxSmearedDefaultKey='DBrealistic'`.
- `IOMC/EventVertexGenerators/python/VtxSmearedRealistic_cfi.py`: `BetafuncEvtVtxGenerator.clone(src = "generator:unsmeared", readDB = True)`
  ("read parameters from GT (SimBeamSpotObjectRcd)"); `VtxSmearedRealisticHLLHC_cfi.py`:
  `HLLHCEvtVtxGenerator.clone(src = "generator:unsmeared", readDB = True)` ("SimBeamSpotHLLHCObjectsRcd").
- `IOMC/EventVertexGenerators/python/VtxSmearedParameters_cfi.py` line 1100: "# Parameters for HL-LHC operation at 13TeV",
  `HLLHCVtxSmearingParameters`: `EprotonInGeV = 6500.0`, `CrossingAngleInurad = 510.0`, `CrabFrequencyInMHz = 400.0`,
  `RF800 = False`, `BetaCrossingPlaneInm = 0.20`, `BetaSeparationPlaneInm = 0.20`, `HorizontalEmittance = 2.5e-06`,
  `VerticalEmittance = 2.05e-06`, `BunchLengthInm = 0.090`, `CrabbingAngleCrossingInurad = 380.0`,
  `CrabbingAngleSeparationInurad = 0.0`; `VtxSmearedHLLHC14TeV_cfi.py`: `HLLHCVtxSmearingParameters.clone(EprotonInGeV = cms.double(7000))`.
- `IOMC/EventVertexGenerators/src/BetafuncEvtVtxGenerator.cc` lines 84-98: `Z = RandGaussQ(0, fSigmaZ) + fZ0`,
  `X = RandGaussQ(0, BetaFunction(Z, fZ0)/sqrt(2)) + fX0` ("need sqrt(2) for beamspot width relative to single beam
  width"), `Y` likewise, `T = RandGaussQ(0, fSigmaZ) + fTimeOffset`.
  `HLLHCEvtVtxGenerator.cc`: `vertexShift` (line 120) draws `x, y, z, t` together by accept-reject on `intensity(x, y, z, t)`
  (line 165) = `exp(-dy*dy/(sigmay*sigmay)) * integrandCC(x, z, t) / (2 pi sigmay)` with `dy = -(z - t) sin(alphay) - y cos(alphay)`
  (`alphay` = 0 for `CrabbingAngleSeparation = 0`) and `sigmay = sigma(z, fVerticalEmittance, fBetaSeparationPlane, betagamma)`;
  `sigma` (line 160) = `sqrt(epsilon (beta + z^2/beta) / betagamma)`, so the vertical size goes as 1/sqrt(beta gamma):
  sqrt(7000/6500) = 1.038. `integrandCC` (line 185) is the overlap of the two crabbed bunches in `(x, z, ct)`.
  Cross-check of the payload name (a numerical integration of the `integrandCC` formula above, `fRF800 = false`, on a
  401 x 401 x 201 grid in z, ct, x, done for this section and not part of the scripts of the chapter): EProton 7000 gives
  sigma_z = 4.27 cm, sigma(ct)/c = 193.3 ps; EProton 6500 gives 4.33 cm, 193.8 ps. The "t193ns" of the name is
  therefore 193 ps.
- The beam-spot payloads (`conddb list <tag>`, `conddb dump <hash>`):
  `SimBeamSpotHLLHCObjects_z4p3cm_t193ns_14TeV_nominal_mc_v1` (record `SimBeamSpotHLLHCObjectsRcd`, IOV 1 inserted
  2024-07-04): EProton 7000, CrabFrequency 400, RF800 0, CrossingAngle 510, CrabbingAngleCrossing 380,
  CrabbingAngleSeparation 0, BetaCrossingPlane 0.2, BetaSeparationPlane 0.2, HorizontalEmittance 2.5e-6,
  VerticalEmittance 2.05e-6, BunchLength 0.09, MeanX = MeanY = MeanZ = TimeOffset = 0; `..._13TeV_nominal_mc_v1`: the
  same with EProton 6500. `SimBeamSpot_Realistic25ns13p6TeVEarly2024CollisionVtxSmearingParameters_v1_mc` (record
  `SimBeamSpotObjectsRcd`, in `150X_mcRun3_2024_realistic_v2`): X0 = 0.0284683, Y0 = -0.016576, Z0 = 0.416038 cm,
  SigmaZ = 3.6 cm, betastar = 30 cm, emittance = 5.725e-8 cm, TimeOffset = 0. `conddb list 150X_mcRun4_realistic_v1`:
  `SimBeamSpotHLLHCObjectsRcd` -> `SimBeamSpotHLLHCObjects_z4p3cm_t193ns_14TeV_nominal_mc_v1`,
  `BeamSpotObjectsRcd` -> `BeamSpotObjects_HLLHC_z4p3cm_t193ns_14TeV_nominal_mc`.
- `Configuration/PyReleaseValidation/python/upgradeWorkflowComponents.py`, `upgradeProperties['Run4']['Run4D110']`:
  `'Geom': 'ExtendedRun4D110'`, `'GT': 'auto:phase2_realistic_T33'`, `'Era': 'Phase2C17I13M9'`,
  `'ScenToRun': ['GenSimHLBeamSpot', 'DigiTrigger', 'RecoGlobal', 'HARVESTGlobal', 'ALCAPhase2']` (the `PU` keys:
  `['GenSimHLBeamSpot', 'DigiTriggerPU', 'RecoGlobalPU', 'HARVESTGlobalPU']`); every `Run4` key starts with
  `GenSimHLBeamSpot`, except `Run4D110GenOnly` and `Run4D110SimOnGen` (`GenHLBeamSpot`, `'BeamSpot': 'DBrealisticHLLHC'`).
- `Configuration/PyReleaseValidation/python/relval_steps.py`: line 4651 `upgradeStepDict['GenHLBeamSpot'][k]` with
  `'--conditions': gt+'_13TeV'`; line 4665 `upgradeStepDict['GenSimHLBeamSpot'][k] = {'-s': 'GEN,SIM', '-n': 10,
  '--conditions': gt+'_13TeV', '--beamspot': 'DBrealisticHLLHC', '--datatier': 'GEN-SIM', '--eventcontent': 'FEVTDEBUG',
  '--geometry': geom}`; line 4674 `upgradeStepDict['GenSimHLBeamSpot14'][k]`: the same with `'--conditions': gt`;
  `upgradeStepDict['GenSim'][k]`: `'--beamspot': 'Realistic25ns13TeVEarly2017Collision'`;
  line 40 `step1Up2024Defaults`: `'--conditions': 'auto:phase1_2024_realistic', '--era': 'Run3', '--beamspot': 'DBrealistic'`.
- `Configuration/PyReleaseValidation/python/relval_upgrade.py` lines 39-49: for a step with `'Sim'` and
  `'HLBeamSpot'` in its name, `if '14TeV' in frag: step = 'GenSimHLBeamSpot14'`, `if 'CloseByParticle' in frag or 'CE_E'
  in frag or 'CE_H' in frag: step = 'GenSimHLBeamSpotHGCALCloseBy'`; for a GEN-only `HLBeamSpot` step,
  `if '14TeV' in frag: step = 'GenHLBeamSpot14'`.
- `Configuration/AlCa/python/autoCondModifiers.py` line 128, `autoCondBSHLLHC13TeV`: for every key containing
  `phase2_realistic`, `key + '_13TeV'` = the key's tags plus `SimBeamSpotHLLHCObjects_z4p3cm_t193ns_13TeV_nominal_mc_v1,
  SimBeamSpotHLLHCObjectsRcd` (line 131). `autoCond.py`: `'phase1_2024_realistic': '150X_mcRun3_2024_realistic_v2'`,
  `'phase2_realistic': '150X_mcRun4_realistic_v1'`.
- `Configuration/Applications/python/ConfigBuilder.py` lines 1068-1074: "GEN step always requires to have a VtxSmearing
  scenario (--beamspot) defined ... unless it's a special gen-only request (GEN:pgen_genonly)"; the smearing cff is
  loaded only in `prepare_GEN` (line 1407; `cffToBeLoaded=VtxSmeared[self._options.beamspot]`, line 1468).
- `RecoLocalFastTime/FTLRecProducers/python/mtdUncalibratedRecHits_cfi.py`: endcap (`ETLUncalibRecHitAlgo`) line 25
  `timeResolutionInNs = cms.string("0.0370")`; barrel (`BTLUncalibRecHitAlgo`) line 11 `"0.308*pow(x,-0.4175)"`.
- The forward-proton transport: `Configuration/StandardSequences/python/Generator_cff.py` line 59, `pgen = cms.Sequence(...,
  PPSTransportTask)`; `SimPPS/Configuration/python/GenPPS_cff.py` line 19 `ctpps_2022.toReplaceWith(PPSTransportTask,
  cms.Task(LHCTransport))` (the 2016-2018 lines are commented out); `SimTransport/PPSProtonTransport/python/CommonParameters_cfi.py`
  `PPSRegionStart_45 = PPSRegionStart_56 = 203`; `edmDumpEventContent test/01_ME/out/step1_GEN_01_ME.root`:
  `edm::HepMCProduct "LHCTransport" "" "GEN"`. `Configuration/Eras/python/Era_Phase2_cff.py` line 23:
  `Phase2 = cms.ModifierChain(Run3_noMkFit.copyAndExclude([phase1Pixel, trackingPhase1, seedingDeepCore, displacedRegionalTracking, ctpps_2022, dd4hep]), ...)`.
  (Not an open problem: the `BeamEnergy = 7000` of `opticalfunctionsTransportSetup_2021`,
  `SimTransport/PPSProtonTransport/python/OpticalFunctionsConfig_cfi.py` line 184, is read only by `HectorTransport.cc`
  line 158; the `OpticalFunctions` method used here takes the beam momentum from `CTPPSBeamParameters`,
  `OpticalFunctionsTransport.cc` lines 104 and 112.)
- `IOMC/RandomEngine/python/IOMC_cff.py`: `generator` 123456789, `VtxSmeared` 98765432, `LHCTransport` 87654321,
  `g4SimHits` 11, `mix` 12345.
- Pythia8 8.311 `xmldoc` (`$PYTHIA8DATA`): `Tunes.xml` `Tune:pp` default 14, option 14 "the Monash 2013 tune" (line 347),
  `Tune:ee` default 7 (option 7 Monash, line 251); `PDFSelection.xml` `PDF:pSet` default 13 (lines 159, 184);
  `MultipartonInteractions.xml` lines 179-182 (pT0 rescaled against "the nondiffractive inelastic" cross section),
  `ecmRef` default 7000.0, `pT0Ref` default 2.28; `ParticleDecays.xml` `limitTau0` default off (line 43), `tau0Max`
  default 10. "expressed in mm/c" (line 47); `BeamRemnants.xml` `BeamRemnants:primordialKThard` default 1.8;
  `TotalCrossSections.xml` `SigmaTotal:mode` default 1, option 0 = "Make your own choices (the own model), set as fixed
  values"; `ParticleProperties.xml`: 21 incoming, 23 outgoing (hardest subprocess), 61 "incoming subprocess particle with
  primordial kT included", 62 "outgoing subprocess particle with primordial kT included", 63 "outgoing beam remnant".
