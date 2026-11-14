# Chapter 04, MPI: technical companion

Multiparton interactions and beam remnants: the same hard process and seeds as chapters 01 and 02, with
`PartonLevel:MPI = on` and hadronisation / decays still off. Script `test/04_MPI/run_04_MPI.sh`, everything it
produces in `test/04_MPI/out/`. All commands below are run from `Journey/test` of an initialized CMSSW_15_0_5
(`cd CMSSW_15_0_5/src && cmsenv && cd JitJetSW/Journey/test`).

## 1. The cmsDriver.py commands

The script calls `gen_stage on on on off off 3 "multiparton interactions and beam remnants"` of
`common/jitjet_common.sh`, which runs exactly one `cmsDriver.py` (recorded as the "with command line options" header of
`04_MPI/step1_GEN_04_MPI_cfg.py` and in `04_MPI/out/step1_GEN_04_MPI_cfg.driver.log`):

```bash
cmsDriver.py Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py \
  --conditions auto:phase1_2024_realistic --era Run3_2024 --geometry DB:Extended --beamspot DBrealistic \
  -n 1 -s GEN --datatier GEN --eventcontent FEVTDEBUG --fileout file:step1_GEN_04_MPI.root \
  --customise_commands "process.generator.comEnergy = cms.double(13600.0)
process.generator.PythiaParameters.processParameters.extend(['PhaseSpace:pTHatMin = 100', 'PartonLevel:ISR = on', 'PartonLevel:FSR = on', 'PartonLevel:MPI = on', 'HadronLevel:all = off', 'HadronLevel:Hadronize = off', 'HadronLevel:Decay = off'])" \
  --python_filename 04_MPI/step1_GEN_04_MPI_cfg.py --no_exec
```

| argument | meaning |
|---|---|
| `Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py` | the generator fragment (`JitJetSW/Configuration/GenProduction/python/`, symlinked into `$CMSSW_BASE/src` by `jitjet_fragment`): Pythia8 `HardQCD:qqbar2bbbar = on`, `gg2bbbar = off`, tune CP5 (`pythia8CP5SettingsBlock` from `Configuration.Generator.MCTunes2017.PythiaCP5Settings_cfi`), `pythia8CommonSettingsBlock`, `comEnergy = 14000` (overridden below) |
| `--conditions auto:phase1_2024_realistic` | the global tag of the 2024 MC (alignment, calibrations; the GEN step only needs the beam spot and the particle table) |
| `--era Run3_2024` | the era modifiers of the 2024 detector |
| `--geometry DB:Extended` | the geometry from the database (loaded, not used at GEN) |
| `--beamspot DBrealistic` | the realistic 2024 beam spot for the vertex smearing (`VtxSmearedRealistic_cfi`), applied to the HepMC record (`generatorSmeared`) |
| `-n 1` | one event: the book follows ONE collision |
| `-s GEN` | the generation step only: Pythia8, the vertex smearing, `genParticles`, generator jets and MET |
| `--datatier GEN --eventcontent FEVTDEBUG` | the data tier and the (full) event content written out |
| `--fileout file:step1_GEN_04_MPI.root` | the output file (written in `04_MPI/out/`, where `cmsRun` is executed) |
| `--customise_commands ...` | python appended to the configuration: the energy 13.6 TeV and the Pythia8 switches of this stage, `PhaseSpace:pTHatMin = 100` (the hard process above 100 GeV), ISR and FSR on, **MPI on** (the only change with respect to chapter 02), `HadronLevel:all`, `Hadronize`, `Decay` off |
| `--python_filename ... --no_exec` | write the configuration next to the script, do not run it (the script runs it itself) |

With the same seeds (the default `RandomNumberGeneratorService` state, unchanged in every generator chapter) the
hard process is identical to chapters 01 and 02; Pythia8's interleaved ISR / FSR / MPI evolution draws its random
numbers differently as soon as MPI compete, so the shower differs from chapter 02 emission by emission
(section 4, table 4.3). Pythia8 has no setting that adds MPI on top of an existing shower; the chapter therefore shows
the two showers side by side (figure 4.1: the page of `02_PS/out/genPartAnalyzer_02_PS.pdf` and of
`04_MPI/out/genPartAnalyzer_04_MPI_shower.pdf`, both cut to the eta-phi frame with `\includegraphics[trim=0 0 170 0,clip]`).

## 2. The cmsRun on the python config

```bash
cd 04_MPI/out && cmsRun ../step1_GEN_04_MPI_cfg.py > step1_GEN_04_MPI.log 2>&1 && cd ../..
```

(`run_driver` in `common/jitjet_common.sh` does exactly this; `RUN=0 ./04_MPI/run_04_MPI.sh` only writes the
configuration.) A walk-through of `04_MPI/step1_GEN_04_MPI_cfg.py`:

- `process = cms.Process('GEN', Run3_2024)`: the process name `GEN` is the one the analyzers ask for
  (`InputTag('genParticles', '', 'GEN')`).
- `process.load(...)`: the standard sequences; `Generator_cff` holds `pgen` (generator, `genParticles`, `genJets`,
  `genMET`), `VtxSmearedRealistic_cfi` the beam-spot smearing, `mixNoPU_cfi` an empty mixing module (no pileup here).
- `process.source = cms.Source("EmptySource")`: no input file; the generator makes the events.
- `process.generator = cms.EDFilter("Pythia8ConcurrentGeneratorFilter", ...)`: the Pythia8 settings in three
  blocks, `pythia8CommonSettings` (CMS conventions: `ParticleDecays:limitTau0 = on`, `tau0Max = 10` mm, so that long-lived
  hadrons are left to Geant4), `pythia8CP5Settings` (the tune, section 4, table 4.8) and `processParameters`
  (the hard process and `pTHatMin = 30`, raised to 100 by the customisation).
- `process.FEVTDEBUGoutput`: the `PoolOutputModule` with `SelectEvents = ['generation_step']`.
- `process.schedule`: `generation_step` (the `pgen` sequence), `genfiltersummary_step`, `endjob_step`, the output.
- the customisation at the end: `comEnergy = 13600`, the Pythia8 switches of the stage, and `customiseEarlyDelete`.

The content of `04_MPI/out/step1_GEN_04_MPI.root` (`edmDumpEventContent 04_MPI/out/step1_GEN_04_MPI.root`), one event
(run 1, lumi 1, event 1):

```
Type                                   Module                       Label     Process
edm::HepMCProduct                      "generatorSmeared"           ""        "GEN"   the Pythia8 record (HepMC), vertex smeared
edm::HepMCProduct                      "LHCTransport"               ""        "GEN"   the protons transported to the PPS stations
vector<reco::GenParticle>              "genParticles"               ""        "GEN"   745 entries: the record with Pythia8 status codes
vector<int>                            "genParticles"               ""        "GEN"   the barcodes of the genParticles in the HepMC record
float                                  "genParticles"               "t0"      "GEN"   the time of the primary vertex
ROOT::Math::PositionVector3D<...>      "genParticles"               "xyz0"    "GEN"   the position of the primary vertex
vector<reco::GenJet>                   "ak4GenJets"                 ""        "GEN"   anti-kT 0.4 jets of the status-1 partons (chapter 07 for the real jets)
double / vector<double>                "ak4GenJets"                 "rho(s)", "sigma(s)"  the pileup density estimators of the jet producer
vector<reco::GenJet>                   "ak4GenJetsNoNu", "ak8GenJets", "ak8GenJetsNoNu"  the same without neutrinos / with R = 0.8, with their rho and sigma
vector<reco::GenMET>                   "genMetCalo", "genMetTrue"   ""        "GEN"   generator MET
edm::RandomEngineStates                "randomEngineStateProducer"  ""        "GEN"   the random engine states (to reproduce the event)
edm::TriggerResults                    "TriggerResults"             ""        "GEN"   the path results (generation_step)
GenEventInfoProduct                    "generator"                  ""        "GEN"   weight, pTHat, x1, x2, Q, the process id (124)
```

`genParticles` is the collection every analyzer of this chapter reads. With hadronisation off, the 169 partons without
daughters are the status-1 entries (the status-62 copies with primordial kT and the status-63 remnants of a
hadronised event, see section 4, table 4.6).

## 3. The analyzer configs

`gen_stage` runs, after `cmsRun`, the per-stage analyzer and the ledger; `run_04_MPI.sh` then
runs the analyzer once more for the shower page (`gen_part_pages`):

```bash
# (a) eta-phi map of the record: stages 1-3 analysed (log), one page drawn, stage 3 (MPI / beam remnants)
cd 04_MPI/out && cmsRun ../../common/genPartAnalyzer_cfg.py inputFiles=file:step1_GEN_04_MPI.root \
  outputFile=genPartAnalyzer_04_MPI.root maxEvents=1 eventsToProcess=1:1:1 maxStage=3 drawStages=3 \
  pdfFile=genPartAnalyzer_04_MPI.pdf \
  sampleLabel="chapter 04_MPI: multiparton interactions and beam remnants|ISR on, FSR on, MPI on|Hadronize off, Decay off|pTHatMin 100 GeV, 13.6 TeV" \
  coneR=0.4 > genPartAnalyzer_04_MPI.log 2>&1
# (a') the same, one page drawn, stage 2 (the parton shower with MPI on, figure 4.1(b)); maxStage=3 keeps the legend
#      of the stages of this sample
cmsRun ../../common/genPartAnalyzer_cfg.py inputFiles=file:step1_GEN_04_MPI.root \
  outputFile=genPartAnalyzer_04_MPI_shower.root maxEvents=1 eventsToProcess=1:1:1 maxStage=3 drawStages=2 \
  pdfFile=genPartAnalyzer_04_MPI_shower.pdf \
  sampleLabel="chapter 04_MPI: multiparton interactions and beam remnants|ISR on, FSR on, MPI on|Hadronize off, Decay off|pTHatMin 100 GeV, 13.6 TeV" \
  coneR=0.4 > genPartAnalyzer_04_MPI_shower.log 2>&1 && cd ../..
# (coneR=0.4: CONE_R=${CONE_R:-0.4} in run_04_MPI.sh, as in chapter 02; CONE_R=0 ./04_MPI/run_04_MPI.sh draws no cones)
# (b) the ledger of the event (FWLite, no cmsRun)
python3 common/genLedger.py 04_MPI/out/step1_GEN_04_MPI.root --event 1:1:1 > 04_MPI/out/ledger_04_MPI.txt
```

| file | content |
|---|---|
| `genPartAnalyzer_04_MPI_shower.pdf`; `genPartAnalyzer_04_MPI.pdf` | one page each (included as PDF by the .tex) (the hard process, stage 1, is the display of chapter 01; the log has every stage). The shower PDF (`drawStages=2`, figure 4.1(b)): after the parton shower of the hard process (14 partons: b, bbar, 1 hard-process ISR, 11 hard-process FSR; the initial-state emissions of an MPI, whose mother is an incoming parton, status 41, are produced in stage 3 with their interaction; section 4.6). The main PDF (`drawStages=3`, figure 4.3): the end of the parton level, the final partons with their primordial kT (169 partons, coloured by the scattering they belong to: b, bbar, 1 hard-process ISR, 11 hard-process FSR, 61 MPI partons, 25 MPI ISR, 56 MPI FSR, 13 beam remnants; 16 with eta beyond 6 on the border; the header counts them by scattering, 14 / 142 / 13). On both pages the dashed cones Delta R < 0.4 around the b and the bbar with their pT, and an inset that magnifies the fuller cone (the b cone) with the name and pT of every parton; at stage 3 the cone pT is followed by its underlying-event part (UE: the partons of the MPI and the remnants, scalar sum). The plugin `plugins/GenPartAnalyzer.cc` attributes every entry to the stage that produced it from its Pythia8 status code (21-29 hard process, 41-49 ISR, 51-59 FSR, 31-39 MPI, 61-69 primordial kT and remnants) and draws, at stage N, the particles alive at the end of stage N; in a record with MPI it also attributes every parton to its scattering (`scatteringOf`: up the mothers through copies, radiators and emissions, from an incoming parton down its chain to the scattering, a parton whose mother is a beam proton is a remnant: the walk of the FWLite script below) and draws the ISR / FSR emissions of an MPI in cyan (triangles), apart from those of the hard process (orange / green) |
| `genPartAnalyzer_04_MPI.root` | per stage: `etaPhiPt` (TH2, weight pT), `ptB`, `ptBbar`, `ptNet` |
| `genPartAnalyzer_04_MPI.log` | per stage: number of particles, the b and bbar copy (status, pT, eta, phi, index) and their net pT; the drawn particles by scattering and by the origin drawn ("by scattering"); the two cones with every member and its scattering (1 hard process, 3 MPI, 6 remnant) and, at stage 3, their hard-process and UE parts (section 4, table 4.2) |
| `ledger_04_MPI.txt` | the record by status group, the hard process, the final state by kind and region, the cones Delta R < 0.4 around the last copies of the b and bbar (section 4, table 4.1) |
| `step1_GEN_04_MPI.log` | the Pythia8 banner, the MPI initialisation (pT0, sigmaND, sigmaInteraction), the settings changed from the defaults, the Info Listing of the event (x1, x2, b, enhancement factor, number of MPI / ISR / FSR), the hard-process listing, the cross section |

The numbers of section 4 that the ledger does not give (the pT of each MPI, the primordial kT, the remnants, the
final state by origin, the pT around the b and bbar) are read from the ROOT file with FWLite:

```bash
python3 - <<'EOF'
import math, collections
from DataFormats.FWLite import Events, Handle
h = Handle('std::vector<reco::GenParticle>')
for ev in Events('04_MPI/out/step1_GEN_04_MPI.root'):
    ev.getByLabel('genParticles', h); P = h.product()
    print('status codes:', sorted(collections.Counter(p.status() for p in P).items()))
    inc = [p for p in P if p.status() == 31]; out = [p for p in P if p.status() == 33]
    print('MPI: status 31', len(inc), '-> interactions', len(inc) // 2, '; outgoing flavours',
          sorted(collections.Counter(abs(p.pdgId()) for p in out).items()))
    g = collections.defaultdict(list)                      # the two outgoing partons of one MPI share their mothers
    for p in out:
        g[tuple(sorted((p.mother(i).pt(), p.mother(i).pz()) for i in range(p.numberOfMothers())))].append(p)
    for pair in sorted(g.values(), key=lambda q: -max(x.pt() for x in q)):
        print('   MPI pT=%5.2f  %s  eta=%s' % (max(x.pt() for x in pair), sorted(x.pdgId() for x in pair), [round(x.eta(), 2) for x in pair]))
    xs = sorted(p.energy() / 6800. for p in inc); print('x of the MPI initiators: min %.2e median %.2e max %.2e' % (xs[0], xs[len(xs) // 2], xs[-1]))
    k = sorted(p.pt() for p in P if p.status() == 61); print('primordial kT (status 61): n %d mean %.3f median %.3f max %.3f' % (len(k), sum(k) / len(k), k[len(k) // 2], k[-1]))
    F = [p for p in P if p.status() == 1]
    def origin(p):                                         # the scattering a final parton belongs to
        if p.mother(0).status() == 4: return 'remnant'
        while p.status() not in (21, 23, 31, 33):          # up the outgoing line (copies, radiators, emissions)
            if p.status() in (41, 42, 53, 61):             # an incoming parton: down to the scattering it initiates
                p = [p.daughter(i) for i in range(p.numberOfDaughters()) if p.daughter(i).status() in (21, 31, 41, 42, 53, 61)][0]
            else: p = p.mother(0)
        return 'MPI' if p.status() in (31, 33) else 'hard+shower'
    rad = {(q.mother(0).pt(), q.mother(0).pz(), q.mother(0).pdgId()): q.mother(0) for q in P if q.status() == 51}
    print('ISR emissions (status 43) by scattering', dict(collections.Counter(origin(q) for q in P if q.status() == 43)),
          '; FSR branchings by scattering', dict(collections.Counter(origin(m) for m in rad.values())))
    for sel, name in ((lambda p: True, 'all'), (lambda p: abs(p.eta()) < 2.5, '|eta|<2.5')):
        c = collections.defaultdict(lambda: [0, 0., 0.])
        for p in F:
            if sel(p): o = origin(p); c[o][0] += 1; c[o][1] += p.energy(); c[o][2] += p.pt()
        for o in c: print('%-10s %-12s n=%4d E=%9.1f pT=%7.1f' % (name, o, *c[o]))
    for p in F:
        if p.mother(0).status() == 4: print('   remnant pdg %3d pt=%6.3f eta=%6.2f E=%8.2f' % (p.pdgId(), p.pt(), p.eta(), p.energy()))
    for name, b in (('b', P[182]), ('bbar', P[207])):
        s = collections.defaultdict(float)
        for p in F:
            d = abs(p.phi() - b.phi()); d = min(d, 2 * math.pi - d)
            if math.hypot(p.eta() - b.eta(), d) < 0.4: s[origin(p)] += p.pt()
        print('dR<0.4 of the %s: pT by origin' % name, {o: round(v, 1) for o, v in s.items()})
    for i in (182, 207): print('idx %d pdg %d status %d pt=%.3f' % (i, P[i].pdgId(), P[i].status(), P[i].pt()))
    print('final-state b quarks:', [(p.pdgId(), round(p.pt(), 2), round(p.eta(), 2), round(p.phi(), 2)) for p in F if abs(p.pdgId()) == 5])
    print('sum E=%.3f px=%.3f py=%.3f' % (sum(p.energy() for p in F), sum(p.px() for p in F), sum(p.py() for p in F)))
EOF
```

## 4. The tables

### 4.1 The ledger (`04_MPI/out/ledger_04_MPI.txt`, complete)

```
== genLedger step1_GEN_04_MPI.root event 1:1:1: 745 particles in genParticles
record by status group:
  hard process (21-29)            4
  MPI (31-39)                   124
  ISR (41-49)                   190
  FSR (51-59)                   192
  beam remnants (61-69)          64
  final state (status 1)        169
hard process: incoming pdg -2 (E=14.8 GeV, x=0.0022) + pdg 2 (E=2652.9 GeV, x=0.3901) -> sqrt(s_hat) = 396.1 GeV
  outgoing pdg -5: pT=113.2 eta=-3.75 phi=0.99 E=2415.7 GeV
  outgoing pdg 5: pT=113.2 eta=-1.44 phi=-2.15 E=252.1 GeV
final state (status 1): n=169 sumE=13600.0 GeV sumPt=481.2 GeV
  by kind                 n    E[GeV]  pT[GeV]
  parton                169   13600.0    481.2
  by region               n    E[GeV]  pT[GeV]     E_nu
  |eta|<2.5              80     642.1    254.5      0.0
  2.5-3.0                17     190.5     22.3      0.0
  3.0-5.2                50    4907.4    183.0      0.0
  5.2-5.5                 3     245.3      2.5      0.0
  >5.5                   19    7614.7     18.9      0.0
  visible (|eta|<5.2, no neutrinos): n=147 E=5740.0 GeV pT=459.8 GeV; neutrinos: n=0 E=0.0 GeV pT=0.0 GeV
  cone Delta R < 0.4 around the last copy of each outgoing parton (status 1, no neutrinos):
    bbar axis [514] status 1 pT=116.4 eta=-3.75 phi=1.01: n=2 E=2485.9 GeV vector pT=117.1 GeV scalar pT=117.1 GeV
         members (pdg pT dR): -5 116.4 0.00, -4 0.8 0.13
    b    axis [515] status 1 pT=91.7 eta=-1.57 phi=-2.08: n=9 E=325.3 GeV vector pT=127.3 GeV scalar pT=127.5 GeV
         members (pdg pT dR): 5 91.7 0.00, 21 15.4 0.07, 21 5.3 0.30, -2 3.7 0.27, -3 3.5 0.19, 21 2.9 0.36, 21 2.3 0.39, 21 1.7 0.34, 21 0.9 0.35
```

The "before" column of the ledger table of the chapter is `02_PS/out/ledger_02_PS.txt`:

```
== genLedger step1_GEN_02_PS.root event 1:1:1: 34 particles in genParticles
record by status group:
  hard process (21-29)            4
  FSR (51-59)                    15
  beam remnants (61-69)           2
  final state (status 1)         11
final state (status 1): n=11 sumE=13600.0 GeV sumPt=230.7 GeV
  by region               n    E[GeV]  pT[GeV]     E_nu
  |eta|<2.5               6     256.9    113.7      0.0
  2.5-3.0                 0       0.0      0.0      0.0
  3.0-5.2                 1    2413.9    114.0      0.0
  5.2-5.5                 0       0.0      0.0      0.0
  >5.5                    4   10929.2      2.9      0.0
  visible (|eta|<5.2, no neutrinos): n=7 E=2670.8 GeV pT=227.8 GeV; neutrinos: n=0 E=0.0 GeV pT=0.0 GeV
```

Differences (04 minus 02): record +711 entries; final state +158 partons, 0.0 GeV, +250.5 GeV pT; |eta|<2.5 +74 / +385.2 / +140.8;
2.5-3.0 +17 / +190.5 / +22.3; 3.0-5.2 +49 / +2493.5 / +69.0; 5.2-5.5 +3 / +245.3 / +2.5; >5.5 +15 / -3314.5 / +16.0;
visible +140 / +3069.2 / +232.0.

### 4.2 The analyzer log (`04_MPI/out/genPartAnalyzer_04_MPI.log`, the stage lines)

```
== GenPartAnalyzer run:lumi:event 1:1:1 nGenParticles=745 nStatus1=169 hard b idx=5 hard bbar idx=4
  stage 1 (hardProcess): nParticles=2 | b quark (status 23): pt=113.221 eta=-1.43851 phi=-2.15105 idx=[5] | #bar{b} quark (status 23): pt=113.221 eta=-3.75298 phi=0.990547 idx=[4] | net b+bbar: |sum pT|=9.89808e-06 (px=-8.27802e-06 py=5.42645e-06) scalar sum pT=226.442
  by scattering (stage 1): hard process 2, MPI 0 (of 31 interactions), beam remnants 0, none 0 | by origin drawn: hard process 2; (the b and bbar copies are counted under their origin)
  cone dR<0.4 (stage 1) | around b: n=1 pt=113.221 eta=-1.43851 phi=-2.15105 scalar sum pT=113.221 members [idx]pdg:pt:dR:scattering(1 hard, 3 MPI, 6 remnant) = [5]5:113.221:1.11022e-15:1 | around bbar: n=1 pt=113.221 eta=-3.75298 phi=0.990547 scalar sum pT=113.221 members [idx]pdg:pt:dR:scattering(1 hard, 3 MPI, 6 remnant) = [4]-5:113.221:3.06422e-14:1
  stage 2 (partonShower): nParticles=14 | b quark (status 52): pt=91.5874 eta=-1.57249 phi=-2.08354 idx=[182] | #bar{b} quark (status 52): pt=118.574 eta=-3.72888 phi=1.01942 idx=[207] | net b+bbar: |sum pT|=27.2853 (px=17.1858 py=21.1929) scalar sum pT=210.162
  by scattering (stage 2): hard process 14, MPI 0 (of 31 interactions), beam remnants 0, none 0 | by origin drawn: hard process 2; hard-process ISR 1; hard-process FSR 11; (the b and bbar copies are counted under their origin)
  cone dR<0.4 (stage 2) | around b: n=6 pt=120.27 eta=-1.58969 phi=-2.08609 scalar sum pT=120.405 members [idx]pdg:pt:dR:scattering(1 hard, 3 MPI, 6 remnant) = [182]5:91.5874:4.44089e-16:1 [235]21:15.3477:0.0675868:1 [373]21:5.31642:0.295817:1 [214]-3:3.50206:0.195194:1 [236]21:2.91632:0.363979:1 [372]21:1.73549:0.340223:1 | around bbar: n=1 pt=118.574 eta=-3.72888 phi=1.01942 scalar sum pT=118.574 members [idx]pdg:pt:dR:scattering(1 hard, 3 MPI, 6 remnant) = [207]-5:118.574:4.44089e-14:1
  stage 3 (mpiBeamRemnants): nParticles=169 | b quark (status 1): pt=91.7437 eta=-1.56979 phi=-2.08372 idx=[515] | #bar{b} quark (status 1): pt=116.356 eta=-3.74787 phi=1.01139 idx=[514] | net b+bbar: |sum pT|=25.0768 (px=16.727 py=18.6829) scalar sum pT=208.1
  by scattering (stage 3): hard process 14, MPI 142 (of 31 interactions), beam remnants 13, none 0 | by origin drawn: hard process 2; hard-process ISR 1; hard-process FSR 11; MPI parton 61; MPI ISR 25; MPI FSR 56; beam remnant 13; (the b and bbar copies are counted under their origin)
  cone dR<0.4 (stage 3) | around b: n=9 pt=127.253 eta=-1.58967 phi=-2.09225 scalar sum pT=127.526 hard process: n=6 pt=120.48 scalar sum pT=120.616 UE (MPI + remnants): n=3 scalar sum pT=6.9105 members [idx]pdg:pt:dR:scattering(1 hard, 3 MPI, 6 remnant) = [515]5:91.7437:4.44089e-16:1 [520]21:15.3744:0.0674676:1 [519]21:5.33019:0.295186:1 [548]-2:3.72603:0.267234:3 [516]-3:3.50953:0.194818:1 [523]21:2.91884:0.363519:1 [581]21:2.2518:0.386309:3 [527]21:1.73925:0.339568:1 [640]21:0.932667:0.350387:3 | around bbar: n=2 pt=117.121 eta=-3.74775 phi=1.01223 scalar sum pT=117.127 hard process: n=1 pt=116.356 scalar sum pT=116.356 UE (MPI + remnants): n=1 scalar sum pT=0.770667 members [idx]pdg:pt:dR:scattering(1 hard, 3 MPI, 6 remnant) = [514]-5:116.356:5.68434e-14:1 [637]-4:0.770667:0.130228:3
```

Chapter 02 (`02_PS/out/genPartAnalyzer_02_PS.log`, run with `coneR=0.4`), for the shower comparison of table 4.1 and figure 4.1 of
the chapter (b cone 6 / 113.22 GeV, bbar cone 1 / 113.22 GeV there; 6 / 120.27 and 1 / 118.57 GeV here):

```
== GenPartAnalyzer run:lumi:event 1:1:1 nGenParticles=34 nStatus1=11 hard b idx=5 hard bbar idx=4
  stage 2 (partonShower): nParticles=7 | b quark (status 52): pt=46.8792 eta=-1.36616 phi=-2.18942 idx=[20] | #bar{b} quark (status 23): pt=113.221 eta=-3.75298 phi=0.990547 idx=[4] | net b+bbar: |sum pT|=66.4005 (px=34.8855 py=56.498) scalar sum pT=160.1
  cone dR<0.4 (stage 2) | around b: n=6 pt=113.221 eta=-1.45084 phi=-2.15105 scalar sum pT=113.414 members [idx]pdg:pt:dR = [20]5:46.8792:0 [18]21:43.1201:0.161834 [19]21:9.65485:0.129597 [17]-1:6.28181:0.311934 [16]21:5.01816:0.127676 [12]1:2.46026:0.0498073 | around bbar: n=1 pt=113.221 eta=-3.75298 phi=0.990547 scalar sum pT=113.221 members [idx]pdg:pt:dR = [4]-5:113.221:3.06422e-14
```

Chapter 05 (`05_Had/out/genPartAnalyzer_05_Had.log`), where the status-62 copies with primordial kT are found by the analyzer:

```
  stage 3 (mpiBeamRemnants): nParticles=169 | b quark (status 62): pt=91.7437 eta=-1.56979 phi=-2.08372 idx=[515] | #bar{b} quark (status 62): pt=116.356 eta=-3.74787 phi=1.01139 idx=[514] | net b+bbar: |sum pT|=25.0768 (px=16.727 py=18.6829) scalar sum pT=208.1
```

### 4.3 The Pythia8 log (`04_MPI/out/step1_GEN_04_MPI.log`)

The MPI initialisation and the Info Listing of the event:

```
 *-------  PYTHIA Multiparton Interactions Initialization  ---------*
 |                   sigmaNonDiffractive =    55.42 mb              |
 |    pT0 =  1.44 gives sigmaInteraction =   500.67 mb: accepted    |
 PYTHIA Warning in MultipartonInteractions::init: maximum increased by factor 1.090

 Beam A: id =   2212, pz =  6.800e+03, e =  6.800e+03, m =  9.383e-01.
 Beam B: id =   2212, pz = -6.800e+03, e =  6.800e+03, m =  9.383e-01.
 In 1: id =   -2, x =  2.174e-03, pdf =  1.145e+00 at Q2 =  1.284e+04.
 In 2: id =    2, x =  3.901e-01, pdf =  2.107e-01 at same Q2.
 Subprocess q qbar -> b bbar with code 124 is 2 -> 2.
 It has sHat =  1.569e+05,    tHat = -1.428e+05,    uHat = -1.409e+04,
       pTHat =  1.132e+02,   m3Hat =  4.800e+00,   m4Hat =  4.800e+00,
    thetaHat =  2.533e+00,  phiHat =  9.905e-01.
     alphaEM =  7.837e-03,  alphaS =  1.143e-01    at Q2 =  1.284e+04.
 Impact parameter b =  3.136e-01 gives enhancement factor =  3.709e+00.
 Max pT scale for MPI =  1.133e+02, ISR =  1.133e+02, FSR =  1.133e+02.
 Number of MPI =    32, ISR =    28, FSRproc =    64, FSRreson =     0.
```

The same lines of `02_PS/out/step1_GEN_02_PS.log` (MPI off): `Max pT scale for MPI = 1.360e+04, ISR = 1.133e+02, FSR = 1.133e+02.`
and `Number of MPI = 1, ISR = 0, FSRproc = 5, FSRreson = 0.` (the "1" is the hard process itself). `05_Had/out/step1_GEN_05_Had.log`
has the same MPI initialisation and the same `Number of MPI = 32, ISR = 28, FSRproc = 64` as this chapter.

The settings changed from the Pythia8 (8.311) defaults, from the same log (the MPI, colour-reconnection and
cross-section lines; the full table is in the log):

```
 | Beams:eCM                                     |                13600.000 |    14000.000         0.0             |
 | ColourReconnection:range                      |                  5.17600 |      1.80000         0.0    10.00000 |
 | HadronLevel:all / Decay / Hadronize           |                      off |           on                         |
 | MultipartonInteractions:alphaSorder           |                        2 |            1           0           3 |
 | MultipartonInteractions:alphaSvalue           |                  0.11800 |      0.13000   0.0600000     0.25000 |
 | MultipartonInteractions:bProfile              |                        2 |            3           0           4 |
 | MultipartonInteractions:coreFraction          |                  0.63000 |      0.50000         0.0     1.00000 |
 | MultipartonInteractions:coreRadius            |                  0.76340 |      0.40000     0.10000     1.00000 |
 | MultipartonInteractions:ecmPow                |                0.0334400 |      0.21500         0.0     0.50000 |
 | MultipartonInteractions:pT0Ref                |                  1.41000 |      2.28000     0.50000    10.00000 |
 | PDF:pSet                                      | LHAPDF6:NNPDF31_nnlo_as_0118 |           13                     |
 | PhaseSpace:pTHatMin                           |                100.00000 |          0.0         0.0             |
 | SigmaTotal:mode                               |                        0 |            1           0           4 |
 | SigmaTotal:sigmaEl                            |                 21.89000 |     25.00000         0.0             |
 | SigmaTotal:sigmaTot                           |                100.30900 |    100.00000         0.0             |
 | SigmaTotal:zeroAXB                            |                      off |           on                         |
```

`MultipartonInteractions:ecmRef` is not in the table, i.e. at its default 7000 GeV; `MultipartonInteractions:pTmin` at its
default 0.2 GeV; `ColourReconnection:mode` at its default 0 (the MPI-based scheme). Defaults from the Pythia8 xmldoc of the
release, `$(scram tool info pythia8 | grep PYTHIA8DATA)` = `/cvmfs/cms.cern.ch/el9_amd64_gcc12/external/pythia8/311-.../share/Pythia8/xmldoc/MultipartonInteractions.xml`, `ColourReconnection.xml`.

### 4.4 The regulator pT0 at the three energies

pT0 = pT0Ref (sqrt(s) / ecmRef)^ecmPow with pT0Ref = 1.41 GeV, ecmRef = 7000 GeV, ecmPow = 0.03344 (CP5):

| sqrt(s) [TeV] | pT0 [GeV] |
|---|---|
| 7 | 1.4100 |
| 13 | 1.4395 |
| 13.6 | 1.4417 (the "pT0 = 1.44" of the log) |
| 14 | 1.4431 |

`python3 -c "for e in (7000., 13000., 13600., 14000.): print(e, round(1.41 * (e / 7000.) ** 0.03344, 4))"`.
sigmaInteraction / sigmaNonDiffractive = 500.67 / 55.42 = 9.03.

### 4.5 The 31 multiparton interactions (FWLite, the script of section 3)

```
status codes: [(1, 169), (4, 2), (21, 2), (23, 2), (31, 62), (33, 62), (41, 28), (42, 28), (43, 28), (44, 106), (51, 128), (52, 55), (53, 9), (61, 64)]
MPI: status 31 62 -> interactions 31 ; outgoing flavours [(1, 7), (2, 11), (3, 4), (4, 1), (21, 39)]
   MPI pT= 6.65  [3, 21]  eta=[1.32, -1.62]
   MPI pT= 6.13  [21, 21]  eta=[1.41, 3.28]
   MPI pT= 4.39  [21, 21]  eta=[-1.99, -0.05]
   MPI pT= 4.10  [1, 21]  eta=[4.7, -5.04]
   MPI pT= 4.01  [21, 21]  eta=[1.91, 2.21]
   MPI pT= 3.58  [21, 21]  eta=[3.08, -4.7]
   MPI pT= 2.57  [21, 21]  eta=[-0.11, -2.42]
   MPI pT= 2.45  [2, 21]  eta=[2.83, -3.87]
   MPI pT= 2.23  [21, 21]  eta=[6.81, 2.29]
   MPI pT= 2.05  [1, 21]  eta=[-2.98, -3.12]
   MPI pT= 1.95  [-2, -1]  eta=[-0.89, -2.88]
   MPI pT= 1.93  [-4, 21]  eta=[4.14, -3.25]
   MPI pT= 1.03  [-1, 21]  eta=[6.05, 4.29]
   MPI pT= 0.98  [-2, 21]  eta=[-2.65, -5.65]
   MPI pT= 0.97  [2, 21]  eta=[7.66, 5.23]
   MPI pT= 0.87  [1, 21]  eta=[6.48, 3.45]
   MPI pT= 0.81  [-1, 3]  eta=[5.17, -6.1]
   MPI pT= 0.80  [2, 3]  eta=[-5.01, -8.27]
   MPI pT= 0.77  [2, 21]  eta=[6.46, 5.27]
   MPI pT= 0.73  [-3, 2]  eta=[5.18, -4.01]
   MPI pT= 0.67  [2, 21]  eta=[6.72, -3.41]
   MPI pT= 0.56  [-2, 1]  eta=[1.87, 2.2]
   MPI pT= 0.51  [21, 21]  eta=[4.07, 0.75]
   MPI pT= 0.51  [21, 21]  eta=[-3.0, 2.44]
   MPI pT= 0.50  [21, 21]  eta=[0.05, -5.47]
   MPI pT= 0.48  [2, 21]  eta=[3.6, -4.6]
   MPI pT= 0.37  [21, 21]  eta=[4.22, 6.75]
   MPI pT= 0.37  [-2, 21]  eta=[6.48, -3.57]
   MPI pT= 0.30  [21, 21]  eta=[2.95, 4.46]
   MPI pT= 0.24  [21, 21]  eta=[6.61, -6.38]
   MPI pT= 0.24  [21, 21]  eta=[-6.81, 5.53]
x of the MPI initiators: min 2.87e-07 median 2.96e-03 max 2.39e-01
primordial kT (status 61): n 64 mean 1.085 median 0.773 max 3.889
```

Histogram of the chapter (pT of the pair): < 0.5 GeV: 7; 0.5-1: 11; 1-2: 3; 2-4: 5; > 4: 5 (31 in total; 18 below 1 GeV,
5 above 4 GeV; the pair printed `0.50` above has pT = 0.4975 GeV and is in the first bin). The median x of the 62
initiators is the mean of the two middle values, 2.94e-03 (the script prints the upper one, 2.96e-03). Status 31 (incoming) and 33 (outgoing) are 62 each: two per interaction. The 124 entries of the
"MPI (31-39)" line of the ledger are these 62 + 62. Incoming flavours (|pdg|): 39 gluons, 11 u, 7 d, 4 s, 1 c, same
as the outgoing ones.

### 4.6 The final state by origin, the remnants, the b jets (FWLite, the script of section 3)

```
ISR emissions (status 43) by scattering {'hard+shower': 2, 'MPI': 26} ; FSR branchings by scattering {'hard+shower': 10, 'MPI': 54}
all        hard+shower  n=  14 E=   2862.2 pT=  266.8
all        MPI          n= 142 E=   8954.8 pT=  207.9
all        remnant      n=  13 E=   1783.0 pT=    6.5
|eta|<2.5  hard+shower  n=  13 E=    392.3 pT=  150.5
|eta|<2.5  MPI          n=  66 E=    249.3 pT=  103.8
|eta|<2.5  remnant      n=   1 E=      0.5 pT=    0.3
   remnant pdg   2 pt= 0.737 eta=  7.03 E=  416.11
   remnant pdg  -1 pt= 0.565 eta=  4.52 E=   25.93
   remnant pdg   1 pt= 0.626 eta=  6.74 E=  265.17
   remnant pdg   2 pt= 0.630 eta=  3.55 E=   10.97
   remnant pdg  -3 pt= 0.867 eta=  6.22 E=  217.72
   remnant pdg   1 pt= 0.707 eta=  5.08 E=   57.11
   remnant pdg  -1 pt= 0.402 eta=  5.32 E=   41.27
   remnant pdg   2 pt= 0.298 eta= -7.69 E=  324.54
   remnant pdg   1 pt= 0.295 eta= -0.65 E=    0.49
   remnant pdg  -2 pt= 0.204 eta= -4.37 E=    8.08
   remnant pdg  -2 pt= 0.496 eta= -7.25 E=  350.24
   remnant pdg  -2 pt= 0.518 eta= -5.51 E=   63.96
   remnant pdg   2 pt= 0.169 eta= -2.76 E=    1.39
dR<0.4 of the b: pT by origin {'hard+shower': 120.6, 'MPI': 6.9}
dR<0.4 of the bbar: pT by origin {'hard+shower': 116.4, 'MPI': 0.8}
idx 182 pdg 5 status 52 pt=91.587
idx 207 pdg -5 status 52 pt=118.574
final-state b quarks: [(-5, 116.36, -3.75, 1.01), (5, 91.74, -1.57, -2.08)]
sum E=13600.000 px=-0.000 py=0.000
```

The origin of a final-state parton is the scattering it belongs to: "remnant" if its mother is a beam proton (status 4);
otherwise the line of mothers is followed up through copies, radiators and emissions until a parton of a scattering is
reached (status 21/23: "hard+shower", 31/33: "MPI"). An initial-state emission (status 43) has as mother the new
incoming parton (status 41), whose ancestors are the beam, not the scattering: from an incoming parton (status 41, 42,
53, 61) the walk therefore goes down to the incoming parton it branched into, until the status-21 or status-31 parton
of its scattering. 14 + 142 + 13 = 169; 2862.2 + 8954.8 + 1783.0 = 13600.0 GeV; 266.8 + 207.9 + 6.5 = 481.2 GeV. Of the 28
ISR emissions and the 64 FSR branchings of the Info Listing, 2 and 10 are in the shower of the hard process. In the cone
of the b the MPI partons are a ubar of 3.73 GeV and a gluon of 2.25 GeV from the initial-state showers of two MPI and a
gluon of 0.93 GeV radiated by the cbar of a third (6.91 GeV; 6.9 / (120.6 + 6.9) = 5 %); in |eta| < 2.5 the MPI and
the remnant carry (103.8 + 0.3) / 254.5 = 41 % of the pT. (An earlier version of this script walked only up the
mothers, so that every initial-state emission of an MPI and its radiation counted as "hard+shower": 57 / 99 instead
of 14 / 142.) The 13 remnants: 11 with |eta| > 3.5, the two hardest (416 and 350 GeV) at eta = 7.03 and
-7.25; no diquark (pdg 2101, 2103, 2203). In `05_Had/out/step1_GEN_05_Had.root` the same 13 partons (same pT, eta, E)
carry status 63 and the 156 other final partons of this file carry status 62 (156 + 13 = 169). The primordial kT moves
the b from 91.587 to 91.744 GeV and the bbar from 118.574 to 116.356 GeV (the status-1 copies here, the status-62 copies
in chapter 05, table 4.2), the net b+bbar from 27.29 to 25.08 GeV.

The legends of the GenPartAnalyzer pages (figures 4.1(b) and 4.3 of the chapter; "page 2" and "page 3" below are
the stage-2 and stage-3 pages, since 14 November 2026 `genPartAnalyzer_04_MPI_shower.pdf` and
`genPartAnalyzer_04_MPI.pdf`, one page each). Since 13 November 2026 (second
change of the day, chapter 04 review) the plugin attributes, in a record with MPI, every parton to the scattering it
belongs to (`scatteringOf`, the walk of the `origin` function above) and draws an ISR / FSR emission of an MPI as
"MPI ISR" / "MPI FSR" (cyan triangles), apart from the "hard-process ISR" / "hard-process FSR" (orange / green), so that
the partons of page 2 keep their colour on page 3. Page 2: 14 drawn, b system (1), bbar system (1), hard-process ISR (1),
hard-process FSR (11). Page 3: 169 drawn, b system (1), bbar system (1), hard-process ISR (1), hard-process FSR (11),
MPI parton (61), MPI ISR (25), MPI FSR (56), beam remnant (13); 16 with |eta| > 6 on the border; the header "hard
process + showers 14, MPI + showers 142, beam remnants 13", the same split as the FWLite count above (14 + 142 + 13).
Before, the legend of page 3 labelled an emitted parton by the shower only: ISR (26) = 1 + 25, FSR (67) = 11 + 56,
MPI (61). Earlier the same day the plugin (`classify` and `scatteringStage`) had started to follow the chain of incoming
partons of an initial-state emission down to its scattering, so an emission of an MPI is produced in stage 3, and to
take a final parton written with status 1 (no hadronisation) for the copy with the primordial kT, produced in stage 3,
not for a primary hadron; page 3 shows the b at 91.74 GeV (status 1). Before that, page 2 drew 57 partons (the 14 above
and 43 emissions of the MPI with their radiation) and page 3 the copies before the primordial kT (b at 91.59 GeV,
status 52).

The partons of the two cones, by ancestry (FWLite, the script below; indices of `genParticles`). The two ISR emissions
of the hard process (status 43, chain leading to the hard process) are a gluon [10] of 8.58 GeV at eta = -1.93,
phi = -1.55 (Delta R = 0.64 from the b at stage 2) and a gluon [16] of 15.41 GeV at eta = 1.61, phi = 1.71. [16]
radiates once and ends as the status-1 gluon [517] of 12.35 GeV at eta = 1.57 (12.05 GeV before the primordial kT,
status 52 [326]): the one "hard-process ISR" parton of the pages, the "12.0" / "12.3" label. [10] branches into
gluons and an s sbar pair; its radiator line ends in a g -> s sbar splitting, so none of its final partons keeps the ISR
label (they are drawn as hard-process FSR), and five of them are the five partons next to the b in the b cone of
stage 2: g [235] 15.35 GeV, g [373] 5.32, sbar [214] 3.50, g [236] 2.92, g [372] 1.74 GeV (status-1 copies [520],
[519], [516], [523], [527] at stage 3). The UE partons of the stage-3 cones:

```
[548] ubar 3.73 GeV  <- [158] 44 <- [37] 43 (ISR emission) <- [34] 41 <- ... incoming chain of an MPI   b cone, drawn "MPI ISR"
[581] g    2.25 GeV  <- 44 <- 51 <- 51 <- 44 <- 44 <- [63] 43 (ISR emission) <- incoming chain of an MPI b cone, drawn "MPI ISR" (radiator line)
[640] g    0.93 GeV  <- 51 <- 51 <- 51 <- [184] 51 (FSR off the cbar [172]) <- [147] 44 <- [143] cbar 33   b cone, drawn "MPI FSR"
[637] cbar 0.77 GeV  <- [383] 52 <- [183] 51 (radiator) <- [172] 44 <- [147] 44 <- [143] cbar 33       bbar cone, drawn "MPI parton"
```

[143] is the outgoing cbar (pT 1.93 GeV, eta = -3.25) of the interaction "MPI pT= 1.93 [-4, 21]" of section 4.5: the
same cbar ends in the bbar cone (Delta R = 0.13) and the gluon it radiated in the b cone (Delta R = 0.35). Cone sums
(`genPartAnalyzer_04_MPI.log`, stage 3): b cone n = 9, vector pT 127.25 GeV, scalar 127.53 GeV, hard process n = 6,
vector 120.48 GeV, scalar 120.62 GeV, UE n = 3, scalar 6.91 GeV (6.91 / 127.53 = 5.4 %); bbar cone n = 2, 117.12 GeV,
hard process 116.36 GeV, UE 0.77 GeV (0.66 %). Against chapter 02 (ledger cones, status-1 copies): b cone 127.3 - 113.6
= +13.7 GeV, about 6.8 GeV of UE (127.25 - 120.48, vector) + 6.9 GeV of shower (120.48 - 113.6). The script:

```bash
python3 - <<'PYEOF'
from DataFormats.FWLite import Events, Handle
h = Handle('std::vector<reco::GenParticle>')
for ev in Events('04_MPI/out/step1_GEN_04_MPI.root'):
    ev.getByLabel('genParticles', h); P = h.product()
    key = lambda p: (p.pdgId(), p.status(), round(p.energy(), 6), round(p.pz(), 6))
    idx = {key(p): i for i, p in enumerate(P)}
    def scat(p):                                           # the walk of the origin function above
        while p.status() not in (21, 23, 31, 33):
            if p.status() in (41, 42, 53, 61):
                p = [p.daughter(i) for i in range(p.numberOfDaughters()) if p.daughter(i).status() in (21, 31, 41, 42, 53, 61)][0]
            else: p = p.mother(0)
        return 'hard' if p.status() in (21, 23) else 'MPI'
    def desc(i, d=0):                                      # the tree below an entry
        p = P[i]; print('  ' * d + '[%d] pdg %d st %d pt %.2f eta %.2f phi %.2f' % (i, p.pdgId(), p.status(), p.pt(), p.eta(), p.phi()))
        for j in range(p.numberOfDaughters()): desc(idx[key(p.daughter(j))], d + 1)
    for i in [i for i in range(len(P)) if P[i].status() == 43 and scat(P[i]) == 'hard']: desc(i)
    for i in (548, 581, 640, 637):                         # the UE members of the cones (genPartAnalyzer_04_MPI.log)
        p = P[i]; chain = []
        while p.numberOfMothers() > 0 and len(chain) < 40:
            chain.append('[%d]%d/%d/%.2f' % (idx[key(p)], p.pdgId(), p.status(), p.pt()))
            if p.status() in (21, 23, 31, 33): break
            p = p.mother(0)
        print(' <- '.join(chain))
PYEOF
```

### 4.7 The pileup catalogue (`08_PU/out/genstats_08_PU.txt`, the lines quoted)

```
== 250 events, beam energy 6800 GeV
process shares: ND 183 (73.2%), SD 45 (18.0%), DD 19 (7.6%), CD 3 (1.2%)
interactions mean    7.03  median    3.00  min    1.00  max   39.00
```

(`common/minBiasGenStats.py`: interactions = 1 + (number of status-31 partons) / 2, the same count as here.)

### 4.8 The tune files of the release (`$CMSSW_RELEASE_BASE/src/Configuration/Generator/python/`)

`MCTunes2017/PythiaCP5Settings_cfi.py` (imported by the fragment of this book and by `MinBias_14TeV_pythia8_TuneCP5_cfi.py`):

```
'Tune:pp 14', 'Tune:ee 7',
'MultipartonInteractions:ecmPow=0.03344', 'MultipartonInteractions:bProfile=2',
'MultipartonInteractions:pT0Ref=1.41', 'MultipartonInteractions:coreRadius=0.7634', 'MultipartonInteractions:coreFraction=0.63',
'ColourReconnection:range=5.176', 'SigmaTotal:zeroAXB=off',
'SpaceShower:alphaSorder=2', 'SpaceShower:alphaSvalue=0.118', 'SigmaProcess:alphaSvalue=0.118', 'SigmaProcess:alphaSorder=2',
'MultipartonInteractions:alphaSvalue=0.118', 'MultipartonInteractions:alphaSorder=2', 'TimeShower:alphaSorder=2', 'TimeShower:alphaSvalue=0.118',
'SigmaTotal:mode = 0', 'SigmaTotal:sigmaEl = 21.89', 'SigmaTotal:sigmaTot = 100.309',
'PDF:pSet=LHAPDF6:NNPDF31_nnlo_as_0118'
```

`diff MCTunes2017/PythiaCP5Settings_cfi.py MCTunesRun3ECM13p6TeV/PythiaCP5Settings_cfi.py`:

```
<         'SigmaTotal:sigmaEl = 21.89',
<         'SigmaTotal:sigmaTot = 100.309',
---
>         'SigmaTotal:sigmaEl = 22.08',
>         'SigmaTotal:sigmaTot = 101.037',
```

Variants in `MCTunes2017/`: `PythiaCP5TuneUpSettings_cfi.py` (`pT0Ref=1.407`, `coreRadius=0.6671`, `coreFraction=0.4281`,
`ColourReconnection:range=4.881`), `PythiaCP5TuneDownSettings_cfi.py` (`pT0Ref=1.46`, `coreRadius=0.6879`, `coreFraction=0.7253`,
`range=4.691`), `PythiaCP5CR1TuneSettings_cfi.py` (`ColourReconnection:mode=1`, `BeamRemnants:remnantMode=1`, `pT0Ref=1.375`,
`ecmPow=0.03283`, `coreFraction=0.4446`, `coreRadius=0.6046`, junction / time-dilation / Lund parameters),
`PythiaCP5CR2TuneSettings_cfi.py` (`ColourReconnection:mode=2`, `pT0Ref=1.454`, `ecmPow=0.0555`, `coreFraction=0.4392`,
`coreRadius=0.6532`, `m2Lambda=4.395`, `fracGluon=0.9896`). None of the four CP5 variants is imported anywhere outside the
two tune directories: `grep -rlE "PythiaCP5(CR1|CR2)TuneSettings|PythiaCP5Tune(Up|Down)" --include=*.py $CMSSW_RELEASE_BASE/src
| grep -v "MCTunes2017/\|MCTunesRun3ECM13p6TeV/"` returns nothing. The minimum-bias fragment switches on
`'SoftQCD:inelastic = on'` (`MinBias_14TeV_pythia8_TuneCP5_cfi.py` line 16); the 31 MPI of our event are in
`test/04_MPI/out/genPartAnalyzer_04_MPI.log` ("of 31 interactions"). All 17 blocks of `MCTunes2017/` (CP1-CP5, their Up / Down
variants, CR1, CR2) exist in `MCTunesRun3ECM13p6TeV/` and differ from them only in the two `SigmaTotal` lines above
(`diff` of each pair); the whole release sets no other values (`grep -rhoE "SigmaTotal:sigma(Tot|El) *= *[0-9.]+"
--include=*.py` gives 35 x `100.309` / `21.89` and 17 x `101.037` / `22.08`). None of the blocks carries a comment.
Fragments importing the 13.6 TeV directory (`grep -rl MCTunesRun3ECM13p6TeV`):
`B0ToJpsiK0s_JMM_Filter_DGamma0_TuneCP5_13p6TeV-pythia8-evtgen_cfi.py`, `DStarToD0Pi_D0ToKsPiPi_inclusive_SoftQCD_TuneCP5_13p6TeV-pythia8-evtgen.py`,
`LbToJpsiLambda_JMM_Filter_DGamma0_TuneCP5_13p6TeV-pythia8-evtgen_cfi.py`, `LbToJpsiXiK0sPi_JMM_Filter_DGamma0_TuneCP5_13p6TeV-pythia8-evtgen_cfi.py`,
`OmegaMinus_13p6TeV_SoftQCDInel_TuneCP5_cfi.py`, `XiMinus_13p6TeV_SoftQCDInel_TuneCP5_cfi.py`. No other tune directory
exists under `Configuration/Generator/python/` (`Herwig7Settings/`, `MCTunes2017/`, `MCTunesRun3ECM13p6TeV/`, `PSweightsPythia/`).

`Configuration/PyReleaseValidation/python/relval_steps.py` (the pileup input of the relval workflows, lines 4605 and 4616-4617):

```
    key='MinBias_14TeV_pythia8_TuneCP5'+'_'+ds
    ...
    elif 'Run4' in ds:
        PUDataSets[ds]={'-n':10,'--pileup':'AVE_200_BX_25ns','--pileup_input':'das:/RelValMinBias_14TeV/%s/GEN-SIM'%(name,)}
```

No era modifier touches a generator setting: `grep -rln "toModify\|toReplaceWith" --include=*.py
Configuration/Generator/python GeneratorInterface/*/python` returns nothing.

The tunes of the Phase-2 relval fragments. `relval_upgrade.py` runs every entry of `upgradeFragments`
(`upgradeWorkflowComponents.py` lines 3256-3407, 152 fragments) for every upgrade key, the `Run4` ones included. The 52
with `14TeV` in their name (all `comEnergy` 14000 GeV) import: `MCTunes2017.PythiaCP5Settings_cfi` 37,
`Pythia8CUEP8M1Settings_cfi` 12 (`Wjet_Pt_80_120`, `Wjet_Pt_3000_3500`, `QCD_Pt_3000_3500`, `QCD_Pt_80_120`, `JpsiMM`,
`WE`, `H130GGgluonfusion`, `PhotonJet_Pt_10`, `QQH1352T_Tauola`, `QCD_Pt_600_800` `_14TeV_TuneCUETP8M1_cfi`, `DYToLL_M-50_14TeV_pythia8_cff`,
`DYToTauTau_M-50_14TeV_pythia8_tauola_cff`), the Pythia6 `PythiaUEZ2starSettings_cfi` 2 (`LM1_sfts_14TeV_cfi`,
`H200ChargedTaus_Tauola_14TeV_cfi`, `Pythia6GeneratorFilter`), ExHuME 1 (`GluGluTo2Jets_M_300_2000_14TeV_Exhume_cff`).
`QCDForPF_14TeV_TuneCP5_cfi`, `QCD_Pt_1800_2400_14TeV_TuneCP5_cfi`, `QCD_Pt15To7000_Flat_14TeV_TuneCP5_cff` use CP5.
`Pythia8CUEP8M1Settings_cfi.py`:

```
'Tune:pp 14', 'Tune:ee 7',
'MultipartonInteractions:pT0Ref=2.4024', 'MultipartonInteractions:ecmPow=0.25208', 'MultipartonInteractions:expPow=1.6',
```

pT0 with `ecmRef` = 7000 GeV (default in both blocks):

| sqrt(s) [TeV] | CP5 [GeV] | CUETP8M1 [GeV] |
|---|---|---|
| 13 | 1.4395 | 2.8081 |
| 14 | 1.4431 (+0.25 %) | 2.8611 (+1.9 %) |

`python3 -c "for e in (13000.,14000.): print(e, round(1.41*(e/7000.)**0.03344,4), round(2.4024*(e/7000.)**0.25208,4))"`.

The HGCal region of the Phase-2 L1 correlator, `L1Trigger/Phase2L1ParticleFlow/python/l1ctLayer1_cff.py` lines 119-129:
`_hgcalSectors` with `etaBoundaries = cms.vdouble(-3.0, -1.5)` and `cms.vdouble(+1.5, +3.0)`.

Pythia8 defaults quoted from the xmldoc of the release: `MultipartonInteractions:pT0Ref` 2.28, `ecmRef` 7000, `ecmPow` 0.215,
`pTmin` 0.2, `bProfile` 3, `coreRadius` 0.4, `coreFraction` 0.5; `BeamRemnants:primordialKThard` 1.8, `primordialKTsoft` 0.9,
`halfScaleForKT` 1.5, `primordialKTremnant` 0.4 (none of them changed by CP5); `ColourReconnection:mode` 0, `range` 1.8;
`Tune:pp 14` = "the Monash 2013 tune by Peter Skands et al." (`Tunes.xml`).
