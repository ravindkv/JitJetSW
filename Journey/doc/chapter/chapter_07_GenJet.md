# Chapter 07, GenJet: technical companion

The anti-kT generator jets of the ONE bbbar event (run 1, lumi 1, event 1), clustered as a separate step (process
`GENJET`) on the full generator event of chapter 06. Script: `test/07_GenJet/run_07_GenJet.sh`; sequence:
`python/genJet_cff.py`; outputs in `test/07_GenJet/out/`. All commands below are run from `test/` of an initialized
`CMSSW_15_0_5` (`cd CMSSW_15_0_5/src && cmsenv && cd JitJetSW/Journey/test`).

```bash
07_GenJet/run_07_GenJet.sh                 # everything: cmsDriver.py, cmsRun, analyzer, PNG pages, ledger
JETPTMIN=10 07_GenJet/run_07_GenJet.sh     # jets with pT > 10 GeV instead of 3
JETS=ak4GenJets,ak8GenJetsNoNu 07_GenJet/run_07_GenJet.sh   # other collections drawn (the four are always produced)
RUN=0 07_GenJet/run_07_GenJet.sh           # only write the configuration
ANALYZE=0 07_GenJet/run_07_GenJet.sh       # the GENJET step without the analyzer and the ledger
```

## 1. The cmsDriver.py commands

The script runs one `cmsDriver.py` (the exact line is the first comment of `test/07_GenJet/step2_GENJET_07_GenJet_cfg.py`,
"with command line options", and the output of the call is `out/step2_GENJET_07_GenJet_cfg.driver.log`):

```bash
cmsDriver.py step2 \
  --filein file:/afs/cern.ch/work/r/rverma/public/JME/JitJet/CMSSW_15_0_5/src/JitJetSW/Journey/test/06_Decay/out/step1_GEN_06_Decay.root \
  --conditions auto:phase1_2024_realistic --era Run3_2024 --geometry DB:Extended -n -1 \
  -s USER:JitJetSW/Journey/genJet_cff.genJetSequence --processName GENJET \
  --datatier GEN --eventcontent FEVTDEBUG --fileout file:step2_GENJET_07_GenJet.root \
  --outputCommands "drop *_ak4GenJets*_*_GEN,drop *_ak8GenJets*_*_GEN" \
  --customise_commands "for m in ('ak4GenJets', 'ak8GenJets', 'ak4GenJetsNoNu', 'ak8GenJetsNoNu'): getattr(process, m).jetPtMin = 3" \
  --python_filename /afs/cern.ch/work/r/rverma/public/JME/JitJet/CMSSW_15_0_5/src/JitJetSW/Journey/test/07_GenJet/step2_GENJET_07_GenJet_cfg.py \
  --no_exec
```

| argument | meaning |
|---|---|
| `step2` | the name of the configuration (`process.configurationMetadata.annotation = 'step2 nevts:-1'`); any word works |
| `--filein file:...step1_GEN_06_Decay.root` | the input: the output of chapter 06 (Rule E), `file:` means a local file, not a dataset |
| `--conditions auto:phase1_2024_realistic` | the global tag; `auto:` resolves it through `Configuration/AlCa/python/autoCond.py` to `150X_mcRun3_2024_realistic_v2`. Not used by the clustering, kept identical to all chapters |
| `--era Run3_2024` | the era (set of modifiers) of the 2024 detector; `process = cms.Process('GENJET', Run3_2024)` |
| `--geometry DB:Extended` | the geometry from the database (not used by the clustering either) |
| `-n -1` | process all events of the input (`process.maxEvents.input = -1`); the file holds one event |
| `-s USER:JitJetSW/Journey/genJet_cff.genJetSequence` | the only step: a `USER` step, i.e. "load the python module `JitJetSW.Journey.genJet_cff` and put its `genJetSequence` in a `cms.Path`" (`process.user_step`) |
| `--processName GENJET` | the process name stamped on every product made here (the four jet collections carry the process `GENJET`, the copied input products keep `GEN`) |
| `--datatier GEN` | the data tier written in the output file's metadata |
| `--eventcontent FEVTDEBUG` | the output event content: everything (the full `genParticles`, the jets, the `HepMCProduct`), from `Configuration/EventContent/python/EventContent_cff.py` |
| `--fileout file:step2_GENJET_07_GenJet.root` | the output file (written in `out/`, where `cmsRun` is executed) |
| `--outputCommands "drop *_ak4GenJets*_*_GEN,drop *_ak8GenJets*_*_GEN"` | two extra output commands appended to the event content: drop the jets the GEN step of chapter 06 had already clustered (process `GEN`: `pgen` contains `genJetMET`), so that the file holds one set of jets. The wildcard `ak4GenJets*` also covers `ak4GenJetsNoNu` |
| `--customise_commands "for m in (...): getattr(process, m).jetPtMin = 3"` | python appended at the end of the configuration: sets `jetPtMin` of the four producers to `$JETPTMIN` (default 3, the value of the `cfi` anyway) |
| `--python_filename .../step2_GENJET_07_GenJet_cfg.py` | where to write the configuration (next to the script, Rule A) |
| `--no_exec` | write the configuration, do not run it (`run_driver` in `common/jitjet_common.sh` runs `cmsRun` afterwards) |

The sequence itself, `python/genJet_cff.py`, is the standard one of `Configuration/StandardSequences/python/Generator_cff.py`:

```python
from RecoJets.Configuration.GenJetParticles_cff import genParticlesForJets, genParticlesForJetsNoNu
from RecoJets.JetProducers.ak4GenJets_cfi import ak4GenJets
from RecoJets.JetProducers.ak8GenJets_cfi import ak8GenJets
ak4GenJetsNoNu = ak4GenJets.clone(src='genParticlesForJetsNoNu')
ak8GenJetsNoNu = ak8GenJets.clone(src='genParticlesForJetsNoNu')
genJetInputTask = cms.Task(genParticlesForJets, genParticlesForJetsNoNu)
genJetTask = cms.Task(genJetInputTask, ak4GenJets, ak8GenJets, ak4GenJetsNoNu, ak8GenJetsNoNu)
genJetSequence = cms.Sequence(genJetTask)
```

- `genParticlesForJets` (`InputGenJetsParticleSelector`, `RecoJets/Configuration/python/GenJetParticles_cff.py`): `src = genParticles`,
  keeps status 1 particles whose |pdgId| is not in `ignoreParticleIDs = (1000022, 1000012, 1000014, 1000016, 2000012, 2000014, 2000016,
  1000039, 5100039, 4000012, 4000014, 4000016, 9900012, 9900014, 9900016, 39)`; `partonicFinalState = False`, `excludeResonances = False`,
  `excludeFromResonancePids = (12, 13, 14, 16)` (unused when `excludeResonances` is False), `tausAsJets = False`.
  `genParticlesForJetsNoNu` is the clone with `12, 14, 16` added to the list.
- `ak4GenJets` (`FastjetJetProducer`, `RecoJets/JetProducers/python/ak4GenJets_cfi.py`): `jetAlgorithm = "AntiKt"`, `rParam = 0.4`, plus
  `GenJetParameters` (`GenJetParameters_cfi.py`): `src = genParticlesForJets`, `jetType = 'GenJet'`, `jetPtMin = 3.0`, `inputEtMin = 0.0`,
  `inputEMin = 0.0`, `doAreaFastjet = False`, `doRhoFastjet = False`, `useDeterministicSeed = True`, `minSeed = 14327`, and the
  `AnomalousCellParameters` (all limits 9999999, calorimeter-only, not used for GenJets). `ak8GenJets` is the clone with `rParam = 0.8`.

## 2. The cmsRun on the python config

```bash
cd 07_GenJet/out && cmsRun ../step2_GENJET_07_GenJet_cfg.py > step2_GENJET_07_GenJet.log 2>&1; cd -
```

Walk-through of `test/07_GenJet/step2_GENJET_07_GenJet_cfg.py` (generated, 5.4 kB):

| part | content |
|---|---|
| `process = cms.Process('GENJET', Run3_2024)` | the process name and the era |
| `process.load(...)` | the standard services, the particle table (`pythiapdt_cfi`), the MessageLogger, the event contents, `mixNoPU_cfi` (no pileup), the geometry and magnetic field from the DB, `JitJetSW.Journey.genJet_cff` (the sequence), `EndOfProcess_cff`, the global tag |
| `process.maxEvents.input = -1` | all events |
| `process.source = cms.Source("PoolSource", fileNames = ['file:.../step1_GEN_06_Decay.root'])` | the input |
| `process.options` | framework options as generated (1 thread, `deleteNonConsumedUnscheduledModules = True`) |
| `process.FEVTDEBUGoutput = cms.OutputModule("PoolOutputModule", ...)` | the output module: `dataTier = 'GEN'`, `fileName = 'file:step2_GENJET_07_GenJet.root'`, `outputCommands = process.FEVTDEBUGEventContent.outputCommands` |
| `process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase1_2024_realistic', '')` | the conditions |
| `process.FEVTDEBUGoutput.outputCommands.append('drop *_ak4GenJets*_*_GEN')`, `...ak8GenJets...` | the two `--outputCommands` |
| `process.user_step = cms.Path(process.genJetSequence)` | the clustering path; the modules of the `cms.Task` run unscheduled, i.e. when their product is needed by the output module |
| `process.endjob_step`, `process.FEVTDEBUGoutput_step` | the end paths; `process.schedule = cms.Schedule(user_step, endjob_step, FEVTDEBUGoutput_step)` |
| `for m in (...): getattr(process, m).jetPtMin = 3` | the `--customise_commands` |
| `customiseEarlyDelete(process)` | the standard early deletion of unused products |

The log `out/step2_GENJET_07_GenJet.log` holds only the file open / close messages and the FastJet banner (release 3.4.1).
The content of `out/step2_GENJET_07_GenJet.root` (`edmDumpEventContent file:07_GenJet/out/step2_GENJET_07_GenJet.root`):

```
Type                                  Module                      Label      Process
GenEventInfoProduct                   "generator"                 ""         "GEN"
ROOT::Math::PositionVector3D<...>     "genParticles"              "xyz0"     "GEN"
edm::HepMCProduct                     "LHCTransport"              ""         "GEN"
edm::HepMCProduct                     "generatorSmeared"          ""         "GEN"
edm::RandomEngineStates               "randomEngineStateProducer" ""         "GEN"
edm::TriggerResults                   "TriggerResults"            ""         "GEN"
float                                 "genParticles"              "t0"       "GEN"
vector<int>                           "genParticles"              ""         "GEN"
vector<reco::GenMET>                  "genMetCalo"                ""         "GEN"
vector<reco::GenMET>                  "genMetTrue"                ""         "GEN"
vector<reco::GenParticle>             "genParticles"              ""         "GEN"
double                                "ak4GenJets"                "rho"      "GENJET"     (and ak4GenJetsNoNu, ak8GenJets, ak8GenJetsNoNu)
double                                "ak4GenJets"                "sigma"    "GENJET"     (idem)
edm::TriggerResults                   "TriggerResults"            ""         "GENJET"
vector<double>                        "ak4GenJets"                "rhos"     "GENJET"     (idem)
vector<double>                        "ak4GenJets"                "sigmas"   "GENJET"     (idem)
vector<reco::GenJet>                  "ak4GenJets"                ""         "GENJET"
vector<reco::GenJet>                  "ak4GenJetsNoNu"            ""         "GENJET"
vector<reco::GenJet>                  "ak8GenJets"                ""         "GENJET"
vector<reco::GenJet>                  "ak8GenJetsNoNu"            ""         "GENJET"
```

Everything with process `GEN` is copied from the input (the `vector<reco::GenJet>` of process `GEN` present in
`step1_GEN_06_Decay.root` are dropped by the two output commands); the four `vector<reco::GenJet>` of process `GENJET` are the
jets of this chapter. `rho`, `sigma`, `rhos`, `sigmas` are the pileup-density products every `FastjetJetProducer` writes (empty here,
`doRhoFastjet = False`). The input selections `genParticlesForJets(NoNu)` (`reco::CandidatePtrVector`) are not in the file: the
standard event content drops them (`drop *_genParticlesForJets_*_*` in `EventContent_cff.py`); the jets keep `edm::Ptr`s to
`genParticles`, so every constituent can be traced back to its `genParticles` index.

## 3. The analyzer configs

```bash
cd 07_GenJet/out && cmsRun ../../common/genJetAnalyzer_cfg.py inputFiles=file:step2_GENJET_07_GenJet.root \
   outputFile=genJetAnalyzer_07_GenJet.root maxEvents=1 eventsToProcess=1:1:1 maxStage=5 \
   jets=ak4GenJets,ak4GenJetsNoNu,ak8GenJets pdfFile=genJetAnalyzer_07_GenJet.pdf \
   sampleLabel="chapter 07_GenJet: generator jets|anti-kT R = 0.4 / 0.8 on the status 1 particles|jet pT > 3 GeV|bbbar pTHat > 100 GeV, 13.6 TeV" \
   > genJetAnalyzer_07_GenJet.log 2>&1
pdftoppm -cropbox -r 110 -png genJetAnalyzer_07_GenJet.pdf genJetAnalyzer_07_GenJet_page
python3 ../../common/genLedger.py step2_GENJET_07_GenJet.root --event 1:1:1 --jets ak4GenJets,ak4GenJetsNoNu,ak8GenJets > ledger_07_GenJet.txt
cd -
```

- `common/genJetAnalyzer_cfg.py` runs two `EDAnalyzer`s of `plugins/`: `GenPartAnalyzer` (with `maxStage=5`: the five generator-stage
  pages of chapters 01-06, so that the PDF of this chapter starts like the earlier ones) and `GenJetAnalyzer` (one page per
  collection in `jets`). An always-true `HLTBool` filter between them keeps the stage pages in front of the jet pages. `R`,
  `jetAlgorithm`, `jetPtMin` and `src` of every collection are read from the producer ParameterSet stored in the file (the page
  titles say "process GENJET, AntiKt R=0.4, jetPtMin=3 GeV, src=genParticlesForJets"). The hard-process b / bbar are followed to
  their last same-flavour copy (status 71) and matched to the nearest jet within R.
- Produces: `genJetAnalyzer_07_GenJet.pdf` (8 pages: 1-5 the generator stages, 6 `ak4GenJets`, 7 `ak4GenJetsNoNu`, 8 `ak8GenJets`),
  `genJetAnalyzer_07_GenJet_page-1..8.png` (110 dpi, `pdftoppm -cropbox`), `genJetAnalyzer_07_GenJet.root` (TFileService: the
  `genPartAnalyzer` and `genJetAnalyzer` directories with per-collection histograms nJets, jetPt, jetEta, nConstituents, etaPhiPt,
  ptBjet, ptBbarJet, ptNetBBbarJets, responseB), `genJetAnalyzer_07_GenJet.log` (the printout of section 4).
- `common/genLedger.py` (FWLite, no cmsRun) writes `ledger_07_GenJet.txt`: the record by status group, the hard process, the
  final state by kind and region, and with `--jets` the number of jets, the sum of pT and E, the leading four jets and the
  number of constituents of every collection.

## 4. The tables

### 4.1 genJetAnalyzer_07_GenJet.log (the jets)

```
== GenJetAnalyzer run:lumi:event 1:1:1 nGenParticles=1846 nStatus1=634 | hard b: [5] pt=113.221 -> last copy [824] status=71 pt=91.7437 eta=-1.56979 phi=-2.08372 | hard bbar: [4] pt=113.221 -> last copy [964] status=71 pt=116.356 eta=-3.74787 phi=1.01139
  ak4GenJets (process GENJET, AntiKt R=0.4, jetPtMin=3 GeV, src=genParticlesForJets): nJets=27 nClustered=239/634 status 1 particles
    b quark -> jet [0] pt=117.788 eta=-1.58908 phi=-2.08403 nConst=24 dR=0.0192893 pt(jet)/pt(quark)=1.28389
    bbar quark -> jet [1] pt=115.944 eta=-3.74635 phi=1.00816 nConst=18 dR=0.00357334 pt(jet)/pt(quark)=0.996452
    net b jet + bbar jet: |sum pT|=6.06053 (px=4.01277 py=-4.54178) scalar sum pT=233.732 dphi=3.09219
    [0] pt=117.788 eta=-1.58908 phi=-2.08403 mass=16.7186 nConst=24
    [1] pt=115.944 eta=-3.74635 phi=1.00816 mass=7.32812 nConst=18
    [2] pt=13.759 eta=1.14207 phi=2.42033 mass=5.67954 nConst=11
    [3] pt=13.3261 eta=1.73863 phi=1.84424 mass=6.28552 nConst=12
    [4] pt=11.8909 eta=-2.12338 phi=-1.88194 mass=5.89159 nConst=11
    [5] pt=11.3556 eta=3.34487 phi=1.87412 mass=4.26183 nConst=10
    [6] pt=7.12342 eta=1.59135 phi=-0.786465 mass=1.5503 nConst=9
    [7] pt=6.3664 eta=-1.90769 phi=-1.24854 mass=2.25057 nConst=7
    [8] pt=6.1768 eta=-1.2832 phi=-0.71992 mass=2.3137 nConst=7
    [9] pt=5.67523 eta=-1.28875 phi=-1.96849 mass=3.56475 nConst=7
    [10] pt=5.61936 eta=-2.9572 phi=-1.0124 mass=1.50845 nConst=6
    [11] pt=5.2635 eta=2.33567 phi=2.94089 mass=3.82824 nConst=6
    [12] pt=5.21862 eta=2.66462 phi=-0.375172 mass=5.12055 nConst=12
    [13] pt=5.08595 eta=-4.70241 phi=0.201231 mass=2.02783 nConst=6
    [14] pt=5.06796 eta=1.26096 phi=-2.11261 mass=3.3597 nConst=11
    [15] pt=4.71421 eta=-3.4524 phi=0.497507 mass=1.47168 nConst=6
    [16] pt=4.62365 eta=4.41241 phi=1.53566 mass=0.81341 nConst=4
    [17] pt=4.59176 eta=-3.73795 phi=0.129922 mass=0.931667 nConst=9
    [18] pt=4.25842 eta=-0.984076 phi=2.58407 mass=1.93563 nConst=8
    [19] pt=3.99184 eta=-2.666 phi=1.92182 mass=2.51934 nConst=9
    [20] pt=3.83145 eta=-0.307395 phi=-0.138889 mass=1.23644 nConst=6
    [21] pt=3.69653 eta=-2.57501 phi=-2.92775 mass=1.79825 nConst=7
    [22] pt=3.67175 eta=-1.97064 phi=-0.730276 mass=2.02292 nConst=7
    [23] pt=3.60893 eta=-2.17915 phi=1.50712 mass=1.81199 nConst=7
    [24] pt=3.53882 eta=-0.840291 phi=-2.66439 mass=0.901041 nConst=7
    [25] pt=3.41077 eta=-1.35548 phi=-2.96583 mass=0.599044 nConst=5
    [26] pt=3.18283 eta=1.55384 phi=1.11998 mass=2.02762 nConst=7
  ak4GenJetsNoNu (process GENJET, AntiKt R=0.4, jetPtMin=3 GeV, src=genParticlesForJetsNoNu): nJets=27 nClustered=238/634 status 1 particles
    b quark -> jet [1] pt=79.817 eta=-1.58604 phi=-2.07723 nConst=22 dR=0.0174989 pt(jet)/pt(quark)=0.87
    bbar quark -> jet [0] pt=115.944 eta=-3.74635 phi=1.00816 nConst=18 dR=0.00357334 pt(jet)/pt(quark)=0.996452
    net b jet + bbar jet: |sum pT|=36.5289 (px=23.1306 py=28.2725) scalar sum pT=195.761 dphi=3.08539
    [0] pt=115.944 eta=-3.74635 phi=1.00816 mass=7.32812 nConst=18
    [1] pt=79.817 eta=-1.58604 phi=-2.07723 mass=13.3627 nConst=22
    [2] pt=13.759 eta=1.14207 phi=2.42033 mass=5.67954 nConst=11
    [3] pt=13.3261 eta=1.73863 phi=1.84424 mass=6.28552 nConst=12
    [4] pt=12.3968 eta=-2.11993 phi=-1.89219 mass=6.0863 nConst=12
    [5] pt=11.3556 eta=3.34487 phi=1.87412 mass=4.26183 nConst=10
    (jets [6] to [26] identical to ak4GenJets)
  ak8GenJets (process GENJET, AntiKt R=0.8, jetPtMin=3 GeV, src=genParticlesForJets): nJets=28 nClustered=488/634 status 1 particles
    b quark -> jet [0] pt=141.096 eta=-1.65098 phi=-2.05072 nConst=55 dR=0.087646 pt(jet)/pt(quark)=1.53793
    bbar quark -> jet [1] pt=121.012 eta=-3.75087 phi=0.989872 nConst=32 dR=0.021726 pt(jet)/pt(quark)=1.04001
    net b jet + bbar jet: |sum pT|=24.0294 (px=1.26595 py=-23.996) scalar sum pT=262.107 dphi=3.04059
    [0] pt=141.096 eta=-1.65098 phi=-2.05072 mass=47.2599 nConst=55
    [1] pt=121.012 eta=-3.75087 phi=0.989872 mass=18.999 nConst=32
    [2] pt=29.0512 eta=1.52067 phi=2.11488 mass=20.8926 nConst=37
    [3] pt=15.0248 eta=1.77447 phi=-0.779849 mass=9.79966 nConst=26
    [4] pt=14.8512 eta=-1.72505 phi=-0.880965 mass=8.96513 nConst=21
    [5] pt=13.5059 eta=3.37206 phi=1.78998 mass=6.39159 nConst=17
    [6] pt=11.7393 eta=-2.47116 phi=1.82228 mass=8.17709 nConst=26
    [7] pt=10.3019 eta=-1.2646 phi=2.83 mass=6.4513 nConst=24
    [8] pt=9.03119 eta=-0.613171 phi=0.0903051 mass=4.65294 nConst=15
    [9] pt=7.80511 eta=2.55488 phi=2.98719 mass=6.49414 nConst=11
    [10] pt=7.68857 eta=-4.76192 phi=0.0705466 mass=3.70383 nConst=14
    [11] pt=7.38151 eta=-2.74431 phi=3.11846 mass=5.05413 nConst=19
    [12] pt=7.30939 eta=-2.90258 phi=-1.02412 mass=3.18912 nConst=10
    [13] pt=6.9072 eta=1.27769 phi=-2.01967 mass=4.76237 nConst=19
    [14] pt=6.56685 eta=-3.74555 phi=-2.64074 mass=4.83833 nConst=16
    [15] pt=6.27965 eta=2.51855 phi=0.264773 mass=4.95787 nConst=19
    [16] pt=6.06806 eta=5.57772 phi=0.809369 mass=6.10309 nConst=13
    [17] pt=6.06075 eta=1.53532 phi=0.95481 mass=5.51094 nConst=16
    [18] pt=5.73071 eta=5.23285 phi=-2.55966 mass=3.71511 nConst=10
    [19] pt=5.68126 eta=4.60337 phi=1.54592 mass=3.16328 nConst=10
    [20] pt=5.30153 eta=-3.67203 phi=0.126168 mass=1.79412 nConst=10
    [21] pt=5.2904 eta=-0.450421 phi=-2.55029 mass=2.72567 nConst=13
    [22] pt=4.5239 eta=2.51057 phi=-1.98092 mass=3.08878 nConst=12
    [23] pt=4.46889 eta=-4.72484 phi=2.8554 mass=2.07348 nConst=7
    [24] pt=4.33213 eta=6.12158 phi=-1.43913 mass=2.41877 nConst=8
    [25] pt=3.86053 eta=4.80076 phi=-0.482717 mass=5.45898 nConst=9
    [26] pt=3.19441 eta=3.49443 phi=0.47852 mass=1.62654 nConst=10
    [27] pt=3.07219 eta=3.78225 phi=-2.42274 mass=2.37219 nConst=9
```

The dijet masses quoted in the .tex (386.8, 319.7, 439.5 GeV) are in the header line of pages 6, 7, 8 of the PDF / PNG
("b jet + bbar jet: ... m = 386.8 GeV"), together with "38 beyond |eta| = 6 on the border" and "status 1, not in a jet (395 /
396 / 146)".

### 4.2 genJetAnalyzer_07_GenJet.log (the generator stages, GenPartAnalyzer, same as chapter 06)

```
== GenPartAnalyzer run:lumi:event 1:1:1 nGenParticles=1846 nStatus1=634 hard b idx=5 hard bbar idx=4
  stage 1 (hardProcess): nParticles=2 | b quark (status 23): pt=113.221 eta=-1.43851 phi=-2.15105 idx=[5] | #bar{b} quark (status 23): pt=113.221 eta=-3.75298 phi=0.990547 idx=[4] | net b+bbar: |sum pT|=9.89808e-06 (px=-8.27802e-06 py=5.42645e-06) scalar sum pT=226.442
  stage 2 (partonShower): nParticles=57 | b quark (status 52): pt=91.5874 eta=-1.57249 phi=-2.08354 idx=[182] | #bar{b} quark (status 52): pt=118.574 eta=-3.72888 phi=1.01942 idx=[207] | net b+bbar: |sum pT|=27.2853 (px=17.1858 py=21.1929) scalar sum pT=210.162
  stage 3 (mpiBeamRemnants): nParticles=169 | b quark (status 62): pt=91.7437 eta=-1.56979 phi=-2.08372 idx=[515] | #bar{b} quark (status 62): pt=116.356 eta=-3.74787 phi=1.01139 idx=[514] | net b+bbar: |sum pT|=25.0768 (px=16.727 py=18.6829) scalar sum pT=208.1
  stage 4: b quark [824] pt=91.7437 -> primary hadron [829] pdg=-521 pt=94.2789 dR=0.00887172
  stage 4: bbar quark [964] pt=116.356 -> primary hadron [968] pdg=523 pt=114.174 dR=0.00134803
  stage 4 (hadronisation): nParticles=320 | b hadron B^{-}: pt=94.2789 eta=-1.56724 phi=-2.09222 idx=[829] | #bar{b} hadron B^{*+}: pt=114.174 eta=-3.74697 phi=1.01039 idx=[968] | net b+bbar: |sum pT|=20.302 (px=13.7251 py=14.9597) scalar sum pT=208.453
  stage 5 (hadronDecays): nParticles=634 | b hadron B^{-} decay products (n=9): pt=94.2789 ... | #bar{b} hadron B^{*+} decay products (n=16): pt=114.174 ... | net b+bbar: |sum pT|=20.302 (px=13.7251 py=14.9597) scalar sum pT=208.453
```

### 4.3 ledger_07_GenJet.txt

```
== genLedger step2_GENJET_07_GenJet.root event 1:1:1: 1846 particles in genParticles
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
jets ak4GenJets: n=27 sumPt=382.8 GeV sumE=3985.2 GeV constituents=239
  [0] pT=117.8 eta=-1.59 phi=-2.08 mass=16.7 nConst=24
  [1] pT=115.9 eta=-3.75 phi=1.01 mass=7.3 nConst=18
  [2] pT=13.8 eta=1.14 phi=2.42 mass=5.7 nConst=11
  [3] pT=13.3 eta=1.74 phi=1.84 mass=6.3 nConst=12
jets ak4GenJetsNoNu: n=27 sumPt=345.3 GeV sumE=3889.7 GeV constituents=238
  [0] pT=115.9 eta=-3.75 phi=1.01 mass=7.3 nConst=18
  [1] pT=79.8 eta=-1.59 phi=-2.08 mass=13.4 nConst=22
  [2] pT=13.8 eta=1.14 phi=2.42 mass=5.7 nConst=11
  [3] pT=13.3 eta=1.74 phi=1.84 mass=6.3 nConst=12
jets ak8GenJets: n=28 sumPt=473.1 GeV sumE=7613.4 GeV constituents=488
  [0] pT=141.1 eta=-1.65 phi=-2.05 mass=47.3 nConst=55
  [1] pT=121.0 eta=-3.75 phi=0.99 mass=19.0 nConst=32
  [2] pT=29.1 eta=1.52 phi=2.11 mass=20.9 nConst=37
  [3] pT=15.0 eta=1.77 phi=-0.78 mass=9.8 nConst=26
```

The ledger row "|eta| > 5.2" of the .tex is the sum of the lines 5.2-5.5 and >5.5: n = 74, E = 7653.5 GeV, pT = 28.1 GeV.
The ratios quoted in the .tex: 117.79 / 113.22 = 1.040 and 141.10 / 113.22 = 1.246 (response w.r.t. the hard-process b),
117.79 / 91.74 = 1.284, 79.82 / 91.74 = 0.870, 141.10 / 91.74 = 1.538 (w.r.t. the status-71 b, as in the log).

### 4.4 FWLite extras (read from step2_GENJET_07_GenJet.root, not in the log)

The numbers of the .tex that are not in the log or the ledger (the origin of the jet constituents, the b-quark chain, the
unclustered particles, the jets by region) come from this FWLite script, run in `07_GenJet/out/`
(`python3 jetExtras.py step2_GENJET_07_GenJet.root`):

```python
import collections
from DataFormats.FWLite import Events, Handle
import sys
hGen = Handle('std::vector<reco::GenParticle>'); hJet = Handle('std::vector<reco::GenJet>')
ev = next(iter(Events(sys.argv[1]))); ev.getByLabel('genParticles', hGen); parts = hGen.product()
def strings_of(i):                      # the status 71-79 partons (the colour string) a stable particle came from
    seen, stack, strs = set(), [i], set()
    while stack:
        j = stack.pop()
        if j in seen: continue
        seen.add(j); p = parts[j]
        if 71 <= p.status() <= 79: strs.add(j); continue
        if p.status() in (1, 2) or 81 <= p.status() <= 89:
            stack += [p.motherRef(m).key() for m in range(p.numberOfMothers())]
    return tuple(sorted(strs))
def from_hadron(i, h):                  # does stable particle i descend from genParticles[h] (the B hadron)?
    seen, stack = set(), [i]
    while stack:
        j = stack.pop()
        if j == h: return True
        if j in seen: continue
        seen.add(j); p = parts[j]
        if p.status() in (1, 2): stack += [p.motherRef(m).key() for m in range(p.numberOfMothers())]
    return False
for coll, bidx in (('ak4GenJets', (829, 968)), ('ak4GenJetsNoNu', (829, 968)), ('ak8GenJets', (829, 968))):
    ev.getByLabel(coll, hJet); jets = hJet.product()
    for jn, j in enumerate(jets):
        if j.pt() < 50: continue
        byB = collections.defaultdict(lambda: [0, 0.]); byS = collections.defaultdict(lambda: [0, 0.])
        for c in range(j.numberOfDaughters()):
            k = j.daughterPtr(c).key(); p = parts[k]
            key = 'B decay' if any(from_hadron(k, h) for h in bidx) else 'other'
            byB[key][0] += 1; byB[key][1] += p.pt()
            byS[strings_of(k)][0] += 1; byS[strings_of(k)][1] += p.pt()
        print(coll, 'jet', jn, 'pt %.2f n %d' % (j.pt(), j.numberOfDaughters()), dict(byB),
              'by string:', sorted(((n, round(pt, 2), s[:2]) for s, (n, pt) in byS.items()), reverse=True))
i = 5                                   # the b quark chain (same for the bbar from 4)
while True:
    p = parts[i]; print('[%d] pdg %d status %d pt %.2f' % (i, p.pdgId(), p.status(), p.pt()))
    nxt = [p.daughterRef(k).key() for k in range(p.numberOfDaughters()) if p.daughter(k).pdgId() == p.pdgId()]
    if not nxt: break
    i = nxt[0]
ev.getByLabel('ak4GenJets', hJet); keys = {j.daughterPtr(c).key() for j in hJet.product() for c in range(j.numberOfDaughters())}
un = [parts[i] for i in range(parts.size()) if parts[i].status() == 1 and i not in keys]
print('unclustered ak4: n=%d sumPt=%.1f sumE=%.1f maxPt=%.2f' % (len(un), sum(p.pt() for p in un), sum(p.energy() for p in un), max(p.pt() for p in un)),
      collections.Counter('<0.5' if p.pt() < 0.5 else '0.5-1' if p.pt() < 1 else '1-2' if p.pt() < 2 else '>2' for p in un))
for coll in ('ak4GenJets', 'ak8GenJets'):
    ev.getByLabel(coll, hJet); reg = collections.defaultdict(lambda: [0, 0.])
    for j in hJet.product():
        r = '<2.5' if abs(j.eta()) < 2.5 else '2.5-3.0' if abs(j.eta()) < 3 else '3.0-5.2' if abs(j.eta()) < 5.2 else '>5.2'
        reg[r][0] += 1; reg[r][1] += j.pt()
    print(coll, 'jets by region', {k: (n, round(pt, 1)) for k, (n, pt) in reg.items()})
```

Its results (the values quoted in the .tex):

| quantity | value |
|---|---|
| b quark chain | [5] status 23 pt 113.22 -> [9] 44 112.58 -> [14] 44 122.83 -> [49] 52 105.99 -> [182] 52 91.59 -> [515] 62 91.74 -> [824] 71 91.74 -> B- [829] pt 94.28 |
| bbar quark chain | [4] status 23 pt 113.22 -> [8] 44 119.74 -> [13] 44 118.60 -> [207] 52 118.57 -> [514] 62 116.36 -> [964] 71 116.36 -> B*+ [968] pt 114.17 |
| the B- decay | [829] B- (pdg -521, pt 94.28, E 235.9) -> anti-nu_e [1434] (pdg -12, pt 37.46, E 95.5), e- [1435] (pt 4.95), D0*(2300)0 [pdg 10421] (pt 51.91); 9 stable products in all |
| the B*+ decay | [968] B*+ (pdg 523, pt 114.17) -> B+ (pt 112.62) + photon (1.56); 16 stable products, no neutrino |
| ak4GenJets b jet [0], 117.79 GeV, 24 constituents | B- decay products: 9, 94.33 GeV; same string as the b (partons [824-828]: b, three gluons, sbar): 16 particles 111.79 GeV, i.e. 7 non-B hadrons with 17.46 GeV; other strings: 8 particles, 6.36 GeV (4.93 beam-remnant string, 0.75 + 0.53 + 0.15 MPI strings). Scalar sum 118.15 GeV |
| ak4GenJets bbar jet [1], 115.94 GeV, 18 constituents | B*+ decay products: 16, 114.22 GeV; bbar string (partons [963-964]): 17 particles 115.86 GeV (one K- of 1.64 GeV); other strings: 1 photon, 0.15 GeV |
| ak4GenJetsNoNu b jet [1], 79.82 GeV, 22 constituents | B- decay products: 8, 56.87 GeV (the neutrino removed); the pi- [1359] (pt 0.52) of the ak4GenJets b jet is now in jet [4] (11.89 -> 12.40 GeV, 11 -> 12 constituents) |
| ak8GenJets b jet [0], 141.10 GeV, 55 constituents | B- decay products: 9, 94.33 GeV; b string: 17 particles 114.17 GeV (8 non-B hadrons, 19.84 GeV); other strings: 38 particles 28.54 GeV (15.56 beam-remnant string, 3.48 + 3.16 + 1.76 + 1.32 + 0.90 + 0.90 + 0.43 + 0.37 + 0.29 + 0.19 + 0.10 + 0.08 MPI / remnant strings). Scalar sum 142.71 GeV |
| ak8GenJets bbar jet [1], 121.01 GeV, 32 constituents | bbar string: 17 particles 115.86 GeV; other strings: 15 particles 5.92 GeV |
| unclustered, ak4GenJets | 395 particles, scalar pT 144.6 GeV, E 9614.8 GeV, hardest 2.34 GeV; 298 below 0.5 GeV, 78 in 0.5-1, 17 in 1-2, 2 above 2 GeV; within abs(eta) < 5.2: 322 particles, 116.5 GeV pT, 1967.6 GeV E; 38 stable particles beyond abs(eta) = 6 |
| ak4GenJets by region | abs(eta) < 2.5: 17 jets, 217.9 GeV; 2.5-3.0: 4 jets, 18.5 GeV; 3.0-5.2: 6 jets, 146.3 GeV; > 5.2: none |
| ak8GenJets by region | abs(eta) < 2.5: 10 jets, 249.4 GeV; 2.5-3.0: 5 jets, 33.3 GeV; 3.0-5.2: 10 jets, 174.4 GeV; > 5.2: 3 jets, 16.1 GeV |

### 4.5 The release files quoted in "Open problems for Phase-2"

All under `$CMSSW_RELEASE_BASE/src` = `/cvmfs/cms.cern.ch/el9_amd64_gcc12/cms/cmssw/CMSSW_15_0_5/src`:

| file | what was read |
|---|---|
| `RecoJets/JetProducers/python/GenJetParameters_cfi.py` | `jetPtMin = 3.0`, `inputEtMin = 0.0`, `inputEMin = 0.0`, `doAreaFastjet = False`, `doRhoFastjet = False`, `useDeterministicSeed = True`, `minSeed = 14327` |
| `RecoJets/JetProducers/python/ak4GenJets_cfi.py`, `ak8GenJets_cfi.py` | `jetAlgorithm = "AntiKt"`, `rParam = 0.4` / `0.8` (`ak8GenJetsSoftDrop` with `jetPtMin = 100.0` is defined but not run) |
| `RecoJets/JetProducers/plugins/VirtualJetProducer.cc` | `inputEtMin` / `inputEMin` applied per input in `inputTowers()`; `fillDescriptions` default `jetPtMin = 5.` |
| `RecoJets/Configuration/python/GenJetParticles_cff.py` | header "ShR 27 Mar 07"; the `ignoreParticleIDs` list; `NoNu` adds 12, 14, 16; `NoMuNoNu` adds 12, 13, 14, 16 |
| `RecoJets/JetProducers/plugins/InputGenJetsParticleSelector.cc` | status 1 selection, `isIgnored`, `excludeResonances` (status 3 or 22 resonances), `partonicFinalState` |
| `grep -rl phase2 RecoJets JetMETCorrections Validation/RecoJets` | only `RecoJets/Configuration/python/RecoJets_EventContent_cff.py` (`phase2_pp_on_AA`) and `CaloTowersES_cfi.py`; nothing in `JetMETCorrections` or `Validation/RecoJets` |
| `PhysicsTools/PatAlgos/python/slimming/slimmedGenJets_cfi.py` | `slimmedGenJets` from `ak4GenJetsNoNu`, `cut = ""`; `slimmedGenJetsAK8` from `ak8GenJetsNoNu`, `cut = "pt > 80"`, `cutLoose = "pt > 10."`, `nLoose = 3` |
| `PhysicsTools/NanoAOD/python/jetMC_cff.py` | `genJetTable cut = "pt > 10"`, `genJetAK8Table cut = "pt > 100."` |
| `CommonTools/PileupAlgos/python/Puppi_cff.py` | `phase2_common.toModify(puppi, EtaMinUseDeltaZ = 4.0, DeltaZCut = 0.1, algos = [eta 0-2.5 / 2.5-3.5: MinNeutralPt 0.2, 0.2, MinNeutralPtSlope 0.015, 0.030; eta 3.5-10: MinNeutralPt 2.0, slope 0.08, MedEtaSF 0.75])` |
| `L1Trigger/Phase2L1ParticleFlow/python/l1ctLayer1_cff.py` | region `etaBoundaries`: barrel (-1.5, 1.5), HGCal (1.5, 3.0) and (-3.0, -1.5), HF (3.0, 5.5) and (-5.5, -3.0) |
| `Configuration/AlCa/python/autoCond.py` | `phase2_realistic = 150X_mcRun4_realistic_v1`; `phase1_2024_realistic = 150X_mcRun3_2024_realistic_v2` |
| `conddb --db pro list 150X_mcRun4_realistic_v1` | `JetCorrectionsRecord`: AK4PF, AK8PF, AK8PFchs = `JetCorrectorParametersCollection_PhaseIIFall17_V5_MC_*`; AK4PFPuppi, AK4PFchs, AK8PFPuppi = `..._Winter22Run3_V1_MC_*`; AK4PFPuppiHLT = `..._Phase2Spring23ticlv4_MC_AK4PFPuppiHLT_v1`; AK4CaloHLT, AK4PFHLT etc. = `..._Run3Summer21_MC_*`; `JetResolutionRcd` and `JetResolutionScaleFactorRcd`: all `JR_Spring16_25nsV6_MC_*` |
| `conddb --db pro list 150X_mcRun3_2024_realistic_v2` | AK4PFPuppi = `..._Winter24_AK4PFPuppi_offline_v1`, AK4PFchs = `..._Winter23_AK4PFchs_offline_v1`, AK8PFPuppi = `..._Winter23_AK8PFPuppi_offline_v1`, AK4PF = `..._Summer19UL17_V5_MC_AK4PF`; `JetResolutionRcd`: `JR_Summer19UL18_JRV2_MC_*` (Puppi, chs), `JR_Autumn18_V1_106X_MC_*` (PF) |
| `CondFormats/JetMETObjects/data/` | only `CMSSW_3110_*`, `CMSSW_362_*`, `CMSSW_538*` response / track files: no Run-3 or Phase-2 JEC text file in the release |
| `doc/tmp/Gen4Phase2.md` (this package) | the Phase-2 GEN step has byte-identical `genParticlesForJets`, `ak4GenJets`, `ak4GenJetsNoNu`, `ak8GenJets` producers |
