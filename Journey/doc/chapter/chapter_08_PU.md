# The minimum-bias event: how Pythia8 generates the pileup collisions (2024, Run 3)

Pileup is made of minimum-bias proton-proton collisions, and before anything is simulated in the detector
those collisions have to be *generated*: a Monte Carlo program has to decide, for each one, whether the two
protons break up or survive, how many parton pairs interact, how the colour strings fragment into hadrons,
and which of those hadrons decay before they reach the beam pipe. This chapter is about that step alone,
the GEN step of the pileup library of the book: what an inelastic collision is in Pythia8, which settings
CMS uses (the fragment, the CP5 tune, the common decay settings), how `cmsDriver.py` wraps it into a CMSSW
job with the Run-3 vertex smearing and its own random seeds, what the generator writes into the file, and
what the 250 generated events of `test/PU_2024/step1_GEN_MinBias_out.root` look like, event by event.
Nothing here involves Geant4: the SIM step that turns these events into hits, and the library it produces,
are the subject of `PULib.md` (and `PULib.tex`); the mixing is `PUSIM.tex`. Section 8 closes the chapter
with the generated collisions put to their use: the in-time ones are overlaid on the bbbar signal event of
the GEN chapter, still at generator level, and the anti-kT clustering is run again to show what pileup does
to the jets before any detector is involved. The chapter is written for a
student who has followed the GEN chapter of the book for the bbbar signal; the boxes marked "In plain words"
restate each key idea without the Pythia or CMSSW vocabulary.

All numbers were produced with CMSSW_15_0_5 (Pythia 8.311, LHAPDF 6.4.0) on 5 November 2026; the
per-event ones come from `minBiasGenStats.py` (section 7), those of section 8 from
`runGenJetPileup_mc2024_BBbar.sh` on 6 November 2026.

| word / symbol | meaning in this chapter |
|---|---|
| minimum bias | the inelastic proton-proton collisions in their natural proportions (Pythia8 `SoftQCD:inelastic`) |
| ND, SD, DD, CD | non-diffractive, single-, double- and central-diffractive collision (Pythia8 process codes 101, 103/104, 105, 106) |
| sigma_tot, sigma_el, sigma_inel | total, elastic and inelastic pp cross section; sigma_inel = sigma_tot - sigma_el is the pileup cross section |
| MPI | multiple-parton interactions: several parton pairs scattering in one collision |
| interaction | one parton-parton scattering in a collision: the hardest one plus the MPIs |
| stable particle | a particle with HepMC status 1 after Pythia8 has done its decays |
| central | a stable particle with \|eta\| < 2.5 and pT > 0.5 GeV, roughly what the tracker will see |
| M_X, xi | mass of the dissociated system of a diffractive collision, and the momentum fraction lost by the surviving proton |
| event k | the k-th event of the GEN file (run 1, lumi 1, event k) |
| in-time pileup | the pileup collisions of the same bunch crossing as the signal (bunch crossing 0 of the mixing); the only ones that matter at generator level |
| collisionId | the index of the collision a GenParticle came from in a merged collection: 0 the signal, 1 .. N the pileup collisions |

---

## 1. What is being generated, and what it must satisfy

In 2024 every bunch crossing at CMS contained on average 60 inelastic proton-proton collisions. One of them
may be the hard scattering an analysis is after; the other 59 are the pileup. Pileup is therefore not a
special process: it is *the inelastic cross section itself*, sampled in its natural proportions, and the
job of the generator is to produce collisions exactly as the LHC produces them, with no trigger, no
pT cut, no filter.

What the generated events will be used for puts four requirements on the GEN step, all of which are met by
the configuration of section 3:

1. **the right collisions**: all inelastic processes, in the proportions of their cross sections
   (`SoftQCD:inelastic`), with the same tune as the signal they will be mixed with (CP5);
2. **the right energy**: 13.6 TeV, the Run-3 energy of the signal, although the fragment is a 14 TeV one;
3. **the right beam spot**: the vertex smearing runs in the GEN step, so the generated event already carries
   the vertex position and time it will have in the detector; it must be the 2024 luminous region;
4. **independent random numbers**: the generator and the vertex smearing must not repeat the sequences of
   the signal job.

> **In plain words: minimum bias.** The name is historical: a "minimum-bias trigger" was the loosest trigger
> an experiment could run, firing on essentially any inelastic collision. A minimum-bias *sample* is thus a
> sample of ordinary inelastic collisions, with nothing selected and nothing enriched. That is exactly what
> the other 59 collisions of a bunch crossing are, so a minimum-bias generator run is the raw material of
> pileup.

---

## 2. The inelastic collision in Pythia8

### 2.1 The total cross section and its pieces

At 13.6 TeV two protons that meet interact with a total cross section of about 100 mb. Pythia8 splits it as

| component | Pythia8 process | code | sigma [mb] | share of inelastic |
|---|---|---|---:|---:|
| elastic, pp -> pp | `SoftQCD:elastic` (not generated here) | 102 | 21.89 | - |
| non-diffractive (ND), pp -> X | `SoftQCD:nonDiffractive` | 101 | 55.42 | 70.7 % |
| single diffractive (SD), pp -> pX and Xp | `SoftQCD:singleDiffractive` | 103 + 104 | 2 x 6.43 | 16.4 % |
| double diffractive (DD), pp -> XX | `SoftQCD:doubleDiffractive` | 105 | 8.85 | 11.3 % |
| central diffractive (CD), pp -> pXp | `SoftQCD:centralDiffractive` | 106 | 1.29 | 1.6 % |
| **inelastic** (`SoftQCD:inelastic`) | ND + SD + DD + CD | | **78.42** | 100 % |
| total | | | 100.31 | |

These are the numbers Pythia8 printed at the initialisation of the job (`step1_GEN_MinBias_out.log`,
"PYTHIA Process Initialization", "We collide p+ with p+ at a CM energy of 1.360e+04 GeV"). They are not
computed from the parton densities: with `SigmaTotal:mode = 0` the CP5 tune *fixes* sigma_tot = 100.309 mb
and sigma_el = 21.89 mb, so that sigma_inel = 78.42 mb whatever the energy; the diffractive pieces come from
Pythia's own parametrisation (the Schuler-Sjostrand model, with `SigmaTotal:zeroAXB = off` keeping the
central-diffractive piece), and the non-diffractive cross section is what remains. This is why the 14 TeV
Phase-2 library of the book has the same cross sections to the second decimal (55.36, 6.44, 8.88, 1.29 mb):
the tune, not the energy, sets them. The `crossSection = 71.39e9` (pb) written in the fragment is older
metadata; the `GenXsecAnalyzer` at the end of the job reports the real one:

```
Before Filter: total cross section = 7.842e+10 +- 3.028e+02 pb
Filter efficiency (event-level)= (250) / (250) = 1.000e+00 +- 0.000e+00    [TO BE USED IN MCM]
```

Pythia chooses the process of each event with probabilities proportional to these cross sections. In the
250 events of the file: 183 ND, 45 SD (27 of type 103, 18 of type 104), 19 DD, 3 CD, against 176.7, 41.0,
28.2 and 4.1 expected. The 19 double-diffractive events are 1.7 sigma low; everything else is within one
standard deviation.

> **In plain words: four ways for two protons to collide inelastically.** Picture the two protons as bags
> of quarks and gluons. In a *non-diffractive* collision the bags tear open and their contents interact
> directly; the whole rapidity range fills with particles. In a *diffractive* collision the protons exchange
> something colourless (a "Pomeron"), so that one of them (SD) or both (DD) keep their identity or dissolve
> into a low-mass bunch of hadrons that flies on along the beam, and a *gap* with no particles at all
> separates the two sides. In a *central-diffractive* collision both protons survive and only a small system
> is created in the middle. The LHC does not pick: it makes all four in the proportions of the table, and so
> does the generator.

### 2.2 The non-diffractive collision: multiple-parton interactions

A non-diffractive collision is where the multiplicity comes from. Pythia8 builds it as follows:

1. **the hardest parton-parton scattering** is generated first as an ordinary 2 -> 2 QCD process
   (q q -> q q, q g -> q g, g g -> g g, ...), at a transverse momentum pT-hat drawn from the perturbative
   cross section regularised at low pT: the divergence d(sigma)/d(pT^2) ~ 1/pT^4 of the massless
   propagators is damped by the factor pT^4 / (pT0^2 + pT^2)^2. This scattering appears in the event record
   with the Pythia status codes 21 (incoming) and 23 (outgoing). In the 183 ND events its pT-hat is
   soft: median 4.0 GeV, mean 4.9 GeV, maximum 20.8 GeV (`GenEventInfoProduct::qScale()`; nothing like
   the pT-hat > 100 GeV of the signal);
2. **additional parton-parton scatterings** (the MPIs) are then generated in decreasing order of pT, with
   the same regularised cross section, down to a lower cut-off, until the available energy of the
   remnants runs out. Their partons carry the status codes 31 (incoming) and 33 (outgoing). How many there
   are depends on the *impact parameter* of the collision: central collisions, where the two protons
   overlap fully, have many MPIs; peripheral ones have few. The proton's matter profile is the tune's
   choice: CP5 uses a double Gaussian (`bProfile = 2`) with a dense core holding 63 % of the matter
   (`coreFraction = 0.63`) inside 0.76 of the proton radius (`coreRadius = 0.7634`);
3. **initial- and final-state radiation** (status 41-44 and 51-53) dress every scattering with gluon and
   quark emissions; the MPIs and the showers are interleaved in pT;
4. **the beam remnants** (status 61-63): what is left of each proton once the scattered partons are taken
   out, given a primordial kT and colour-connected to the rest;
5. **colour reconnection**: the colour strings between the many partons are rearranged to minimise their
   total length (`ColourReconnection:range = 5.176`, a tune parameter that controls how aggressively);
6. **hadronisation** with the Lund string model (status 71-74 for the string partons), and
7. **decays** of the unstable hadrons (section 2.4).

The regularisation scale is the single most important parameter of the model. CP5 sets it at a reference
energy of 7 TeV and lets it grow with the collision energy as a power law:

    pT0 = pT0Ref x (E_cm / 7 TeV)^ecmPow = 1.41 x (13.6 / 7)^0.03344 = 1.44 GeV

and this is what the log prints: "pT0 = 1.44 gives sigmaInteraction = 492.18 mb: accepted". The number
492 mb is the integrated parton-parton cross section above the cut-off, nine times the non-diffractive
cross section of 55 mb: on average nine parton pairs interact per non-diffractive collision. The 250 events
confirm it. Counting one interaction for the hard scattering plus one per pair of status-31 partons,
the 183 ND events have between 1 and 39 interactions, mean 9.0, median 6, and the multiplicity follows
them almost linearly:

| interactions in the ND event | 1 | 2-3 | 4-6 | 7-10 | 11-15 | 16-20 | 21-30 | >30 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| events | 44 | 27 | 25 | 23 | 24 | 17 | 19 | 4 |
| mean stable particles | 85 | 113 | 177 | 269 | 333 | 409 | 499 | 579 |

Event 1 (18 interactions, 242 stable particles) and event 247 (39 interactions, 723 stable particles, the
largest of the file) are both non-diffractive; so is event 13, with a single interaction and 78 particles.
This spread, and not the process type, is what makes two minimum-bias events differ by a factor of ten in
multiplicity.

> **In plain words: MPI.** Protons are not points: when two of them pass through each other, more than one
> pair of their constituents can scatter. Each such scattering is soft (a few GeV) and alone would hardly be
> noticed, but their number fluctuates from one to a few dozen, and every one of them adds a dozen or more
> hadrons. The "size" of a minimum-bias collision is essentially the number of these scatterings, which is
> set by how centrally the two protons hit each other.

### 2.3 The diffractive collisions

In a diffractive collision one proton (SD) or both (DD) are excited into a system X of mass M_X by the
exchange of a Pomeron, and the system then hadronises. Pythia8 (the Schuler-Sjostrand model) draws M_X
from a spectrum falling like 1/M_X^2 between the proton mass and about the full energy, and treats the
system as a Pomeron-proton collision with its own MPIs: the log shows this initialisation too, a block
"diffraction XB for p+ on p+" that finds a regularisation pT0 for every diffractive mass from 10 GeV to
13.6 TeV. The surviving proton loses the momentum fraction xi = M_X^2 / s and stays within a fraction of a
milliradian of the beam.

The 45 single-diffractive events of the file span the whole spectrum. The mass of the dissociated system
(the invariant mass of all stable particles except the most energetic proton) has a median of 167 GeV and
runs from 1.2 GeV to 6754 GeV; xi from 3.7 x 10^-9 to 0.25:

| SD event | M_X [GeV] | xi | stable particles | what it looks like |
|---|---:|---:|---:|---|
| 210 | 1.2 | 3.7e-9 | 3 | the proton (6800.0 GeV, eta = 10.4) and an n pi+ system of 1.25 GeV at eta = -10: a proton excited by little more than a pion mass |
| 12 | 2.6 | 3.1e-8 | 4 | the proton (6800.0 GeV, eta = -10.8) and a K0L, a Lambda and a pi+ at eta = 9: nothing central |
| 216 | 6754 | 0.25 | 111 | indistinguishable from a non-diffractive event in the central detector |

On average an SD event has 70 stable particles and 7 central ones, a DD event 57 and 2, against 241 and
27 for ND: the diffractive events are nearly empty for the tracker and the calorimeters, and they are 27 %
of the pileup collisions. The three central-diffractive events (two surviving protons and a small central
system: 35 stable particles, 2 central) are the rarest and the emptiest.

### 2.4 Hadronisation and decays: what reaches the detector

The end of the generation is a list of *stable* particles (HepMC status 1), those Pythia hands to the
detector simulation. Which particles count as stable is a setting of the common block:
`ParticleDecays:limitTau0 = on` with `tau0Max = 10` mm decays everything with c tau < 10 mm and leaves the
rest. So the pi0 (c tau = 25 nm), eta, the D and B hadrons, the tau, the Sigma0 are decayed by Pythia, while
the K0S (c tau = 2.7 cm), the Lambda (7.9 cm), the charged pions and kaons, the muons and the neutrons are
left to Geant4. `ParticleDecays:allowPhotonRadiation = on` lets the decays radiate photons.

Over the 48 437 stable particles of the 250 events:

| species | share | note |
|---|---:|---|
| photons | 45.9 % | almost all from pi0 -> gamma gamma; the ECAL sees a minimum-bias event mostly through these |
| pi+- | 37.1 % | the charged tracks |
| K+- | 4.8 % | |
| p, pbar | 2.7 % | includes the surviving and the beam-remnant protons |
| n, nbar | 2.5 % | |
| K0L | 2.4 % | stable for Pythia and for the central detector (c tau = 15 m) |
| K0S | 2.2 % | will decay in the tracker (c tau = 2.7 cm) |
| Lambda | 0.9 % | will decay in the tracker (c tau = 7.9 cm) |
| e+-, mu+-, neutrinos | 0.7 % | the electrons from Dalitz decays and conversions in the generator, 14 muons in 250 events |
| Sigma+-, Xi, Omega | 0.8 % | |

and their kinematics are soft and forward:

| pT of the stable particles | < 0.2 GeV | 0.2-0.5 | 0.5-1 | 1-2 | 2-5 | > 5 GeV |
|---|---:|---:|---:|---:|---:|---:|
| particles | 18 209 | 17 452 | 8 978 | 3 094 | 687 | 17 |

| \|eta\| of the stable particles | 0-1 | 1-2 | 2-3 | 3-4 | 4-5 | 5-6 | 6-7 | 7-8 | 8-9 | 9-10 | > 10 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| particles | 6950 | 7498 | 7816 | 7227 | 6406 | 5365 | 3509 | 2041 | 1039 | 419 | 167 |

The density is flat in pseudorapidity out to |eta| = 4 and only then falls off, so that 20 % of the particles
are beyond |eta| = 5.5, where no CMS subdetector is; the hardest particle of an event has a pT of 2.2 GeV on
average and never more than 8.1 GeV; and 17 of 48 437 particles are above 5 GeV. Only 21 of the 250 events
contain an anti-kT (R = 0.4) generator jet above 10 GeV, and one event a jet above 20 GeV.

> **In plain words: what a minimum-bias event is made of.** Mostly pions, in equal numbers charged and
> neutral, the neutral ones already decayed into pairs of photons; a sprinkling of kaons, protons, neutrons
> and strange baryons; spread evenly across the whole rapidity range with transverse momenta of a few
> hundred MeV. The signal bbbar event has a few particles with tens of GeV; a minimum-bias event has none
> above a few GeV.

---

## 3. The configuration

### 3.1 The generator fragment

There is no 13.6 TeV minimum-bias fragment in the release (`Configuration/Generator/python/` has
`MinBias_13TeV_pythia8_TuneCUETP8M1_cfi` and `MinBias_14TeV_pythia8_TuneCP5_cfi`), and the Run-3 RelVal
workflows of `Configuration/PyReleaseValidation` mix their 13.6 TeV signals with the *14 TeV* library
`RelValMinBias_14TeV`. The book takes the same fragment and sets the energy to that of the signal through
`--customise_commands`, exactly as the signal's GEN step did with its 14 TeV bbbar fragment:

```python
# Configuration/Generator/python/MinBias_14TeV_pythia8_TuneCP5_cfi.py
from Configuration.Generator.Pythia8CommonSettings_cfi import *
from Configuration.Generator.MCTunes2017.PythiaCP5Settings_cfi import *

generator = cms.EDFilter("Pythia8ConcurrentGeneratorFilter",
    crossSection = cms.untracked.double(71.39e+09),      # pb, metadata only (the GenXsecAnalyzer reports the real one)
    maxEventsToPrint = cms.untracked.int32(0),           # how many event records to print to the log
    pythiaPylistVerbosity = cms.untracked.int32(1),
    filterEfficiency = cms.untracked.double(1.0),
    pythiaHepMCVerbosity = cms.untracked.bool(False),
    comEnergy = cms.double(14000.0),                     # overridden: process.generator.comEnergy = cms.double(13600.0)
    PythiaParameters = cms.PSet(
        pythia8CommonSettingsBlock,
        pythia8CP5SettingsBlock,
        processParameters = cms.vstring('SoftQCD:inelastic = on'),
        parameterSets = cms.vstring('pythia8CommonSettings', 'pythia8CP5Settings', 'processParameters')))

ProductionFilterSequence = cms.Sequence(generator)
```

Three named blocks of Pythia8 settings are handed to Pythia in the order of `parameterSets`.

**The process** is one line, `SoftQCD:inelastic = on`: the four subprocesses of section 2.1 and nothing
else. No hard QCD is switched on; a minimum-bias event with a 20 GeV jet is possible (one of the 250 has
one) but comes from the tail of the MPI and the showers, not from a 2 -> 2 process with a pT-hat cut.

**The tune CP5** (`Configuration/Generator/python/MCTunes2017/PythiaCP5Settings_cfi.py`) is the set of
parameters CMS fitted to Run-2 underlying-event and minimum-bias data; it is the tune of the signal too, so
the underlying event of the signal and the pileup collisions are described by the same model:

| setting | value | meaning |
|---|---|---|
| `Tune:pp 14`, `Tune:ee 7` | | start from the Monash 2013 tune |
| `PDF:pSet = LHAPDF6:NNPDF31_nnlo_as_0118` | | the parton densities, NNPDF3.1 at NNLO, read through LHAPDF |
| `SigmaProcess`, `SpaceShower`, `TimeShower`, `MultipartonInteractions:alphaSvalue` | 0.118 | alpha_s(M_Z) in the hard process, the two showers and the MPI, consistent with the PDF |
| `...:alphaSorder` | 2 | alpha_s run at two loops everywhere |
| `MultipartonInteractions:pT0Ref` | 1.41 | the MPI regularisation scale at 7 TeV (GeV) |
| `MultipartonInteractions:ecmPow` | 0.03344 | its energy dependence; gives pT0 = 1.44 GeV at 13.6 TeV |
| `MultipartonInteractions:bProfile` | 2 | double-Gaussian matter distribution of the proton |
| `MultipartonInteractions:coreRadius`, `coreFraction` | 0.7634, 0.63 | the inner Gaussian: 63 % of the matter within 0.76 of the outer radius |
| `ColourReconnection:range` | 5.176 | how far colour reconnection may rearrange the strings |
| `SigmaTotal:mode` | 0 | use the cross sections given by hand rather than the parametrisation |
| `SigmaTotal:sigmaTot`, `sigmaEl` | 100.309, 21.89 | the total and elastic cross sections (mb) |
| `SigmaTotal:zeroAXB` | off | keep the central-diffractive process |

**The common settings** (`Configuration/Generator/python/Pythia8CommonSettings_cfi.py`) are the same for
every CMS Pythia8 fragment:

| setting | value | meaning |
|---|---|---|
| `Tune:preferLHAPDF` | 2 | take the PDF from LHAPDF rather than Pythia's internal copy |
| `Main:timesAllowErrors` | 10000 | how many generation errors are tolerated before the job stops |
| `Check:epTolErr` | 0.01 | tolerance of the energy-momentum conservation check |
| `Beams:setProductionScalesFromLHEF` | off | (only matters for LHE input) |
| `SLHA:minMassSM` | 1000 | (only matters for SUSY spectra) |
| `ParticleDecays:limitTau0`, `tau0Max` | on, 10 | decay particles with c tau < 10 mm, leave the rest (section 2.4) |
| `ParticleDecays:allowPhotonRadiation` | on | QED radiation in the decays |

Pythia prints every setting that differs from its default at initialisation ("Settings (changes only)" in
the log); the table there is the sum of these three blocks plus `Beams:eCM = 13600` from `comEnergy`.

### 3.2 The generator module

`Pythia8ConcurrentGeneratorFilter` (`GeneratorInterface/Pythia8Interface`, a
`ConcurrentGeneratorFilter<Pythia8Hadronizer>`) is the module that runs Pythia. Three things about it:

- it is a **filter**: a fragment can reject events (a bbbar filter, a pT-hat cut), and only the events that
  pass reach the output. This fragment accepts everything (`filterEfficiency = 1.0`; the `GenXsecAnalyzer`
  confirms 250 tried, 250 accepted). The `ProductionFilterSequence` that holds it is inserted at the start
  of every path by `cmsDriver.py`, so no later module ever sees a rejected event;
- it is **concurrent**: there is one Pythia instance per framework stream, each with its own random
  engine, plus a master instance used for the initialisation and the cross-section bookkeeping. This is why
  the Pythia banner and the initialisation tables appear twice in the log of a one-stream job;
- it **produces** two things per event: the generated record as an `edm::HepMCProduct` with the label
  `generator:unsmeared` (the collision at the origin) and a `GenEventInfoProduct` labelled `generator`
  (process id, event weight, the scale `qScale` of the hardest interaction, the alpha_s values).

`maxEventsToPrint = 0` keeps the log short. Set it to 1 (section 7.5) and Pythia lists the first event in
full: the "info" block (process, x_1, x_2, pT-hat, number of MPIs) and the event record, one line per
particle with its status code, mother and daughter indices and four-momentum.

### 3.3 The energy and the random seeds

Two lines of `--customise_commands` complete the generator configuration, and a third concerns the vertex
smearing:

```python
process.generator.comEnergy = cms.double(13600.0)
process.RandomNumberGeneratorService.generator.initialSeed = 20241
process.RandomNumberGeneratorService.VtxSmeared.initialSeed = 20242
```

The first overrides the 14 TeV of the fragment with the 13.6 TeV of the Run-3 signal. The difference is
small (a few per cent in multiplicity, through the slow growth of pT0 and of the PDFs with the energy), and
the RelVal workflows ignore it; the book keeps the library and the signal at the same energy because that is
what a physics production would do.

The seeds matter more than they look. Every module that draws random numbers has its own engine in the
`RandomNumberGeneratorService`, and `cmsDriver.py` writes the *same default seeds* into every configuration
it generates. Had the minimum-bias job been run with the defaults, its `VtxSmeared` engine would have drawn
the same sequence as the signal's: minimum-bias event k would have had *exactly* the vertex of signal event
k, in space and in time, and the first mixed pileup collision would have sat on the signal vertex. The
scripts therefore give the two GEN engines their own seeds from the `PUSEED` variable (default 20241):

| engine | seed | what it drives |
|---|---|---|
| `generator` | `PUSEED` = 20241 | Pythia8: the process of every event, the MPIs, the showers, the hadronisation, the decays |
| `VtxSmeared` | `PUSEED` + 1 = 20242 | the vertex position and time of every event |

A different `PUSEED` is a different set of collisions; the same `PUSEED` always reproduces this one. The
`randomEngineStateProducer` of the GEN sequence records the state of both engines at the start of every
event in the file (`edm::RandomEngineStates`), so that any single event can be regenerated on its own.

### 3.4 The `cmsDriver.py` command and the GEN process

```bash
cmsDriver.py MinBias_14TeV_pythia8_TuneCP5_cfi --conditions auto:phase1_2024_realistic --era Run3_2024 \
  --geometry DB:Extended --beamspot DBrealistic -n 250 -s GEN --datatier GEN --eventcontent FEVTDEBUG \
  --fileout file:step1_GEN_MinBias_out.root --python_filename step1_GEN_MinBias_cfg.py --no_exec \
  --customise_commands "process.generator.comEnergy = cms.double(13600.0)\nprocess.RandomNumberGeneratorService.generator.initialSeed = 20241\nprocess.RandomNumberGeneratorService.VtxSmeared.initialSeed = 20242"
cmsRun step1_GEN_MinBias_cfg.py > step1_GEN_MinBias_out.log 2>&1
```

| option | what it sets in a GEN job |
|---|---|
| first argument | the generator fragment to load from `Configuration/Generator/python/` |
| `-s GEN` | the step: the generator sequence `pgen` |
| `-n 250` | number of events to generate (after the filter) |
| `--conditions auto:phase1_2024_realistic` | the global tag (`150X_mcRun3_2024_realistic_v2`); in a GEN job it supplies the beam-spot payload of the vertex smearing |
| `--era Run3_2024` | the era modifiers; in a GEN job they switch on the PPS transport (`run3_common`) |
| `--geometry DB:Extended` | not used by the generator; written into the configuration so that the SIM step inherits it |
| `--beamspot DBrealistic` | the vertex-smearing module: `BetafuncEvtVtxGenerator` reading the database |
| `--datatier GEN`, `--eventcontent FEVTDEBUG` | the label of the output and the list of products kept (everything) |
| `--customise_commands "..."` | Python lines appended verbatim to the configuration (a literal `\n` separates them) |
| `--fileout`, `--python_filename`, `--no_exec` | the output file, the name of the configuration, "write it, do not run it" |

The generated configuration is a process named `GEN` with the era `Run3_2024`, one path and three end
paths:

```python
process.generation_step = cms.Path(process.pgen)
process.genfiltersummary_step = cms.EndPath(process.genFilterSummary)
process.endjob_step = cms.EndPath(process.endOfProcess)
process.FEVTDEBUGoutput_step = cms.EndPath(process.FEVTDEBUGoutput)
for path in process.paths:
    getattr(process, path).insert(0, process.ProductionFilterSequence)   # the generator filter runs first
```

with `pgen` (`Configuration/StandardSequences/python/Generator_cff.py`):

```python
pgen = cms.Sequence(randomEngineStateProducer + VertexSmearing + GenSmeared + GeneInfo + genJetMET, PPSTransportTask)
```

Module by module, for every event:

```
generator (Pythia8)  -->  HepMCProduct generator:unsmeared, GenEventInfoProduct     the collision at the origin
        |
VtxSmeared           -->  HepMCProduct VtxSmeared                                   the collision moved to a vertex of the 2024 beam spot
        |
generatorSmeared     -->  HepMCProduct generatorSmeared                             the same, under the fixed label every later step reads
        |
genParticles         -->  reco::GenParticles (+ xyz0, t0)                           the record as a flat collection, with the vertex
genJetMET            -->  ak4GenJets, ak8GenJets, (NoNu), genMetTrue, genMetCalo    anti-kT jets and MET of the stable particles
LHCTransport         -->  HepMCProduct LHCTransport                                 the forward protons carried 200 m to the PPS Roman pots
        |
FEVTDEBUGoutput      -->  step1_GEN_MinBias_out.root
```

1. `randomEngineStateProducer` records the engine states (section 3.3);
2. `generator` runs Pythia8 (section 3.2);
3. `VtxSmeared` moves the whole event to a vertex drawn from the beam model (section 3.5) and
   `generatorSmeared` republishes it under the label that the SIM step, the mixing and every analyzer
   read, so that none of them depends on which smearing module was used;
4. `genParticles` converts the HepMC record into `reco::GenParticle`s (every particle of the record, with
   its Pythia status code, mothers and daughters), together with the vertex (`genParticles:xyz0`) and time
   (`genParticles:t0`) of the event; `genJetMET` clusters the stable particles into anti-kT jets of R = 0.4
   and 0.8, with and without neutrinos, and computes the generator MET. None of this is used by the mixing
   or by Geant4; it is in the file because the standard GEN sequence makes it, and it is what the
   `GenPartAnalyzer` and `GenJetAnalyzer` of the GEN chapter read;
5. `LHCTransport` (the `PPSTransportTask`, Run 3 only) takes the stable protons that keep most of the beam
   energy and tracks them through the LHC magnets to the Roman pots of the Precision Proton Spectrometer,
   about 200 m from CMS. In the 250 events it carried 93 protons in 81 events: 51 from the 45 SD events, 10
   from DD, 6 from the 3 CD events, and 26 beam-remnant protons of ND events that happened to fall into the
   acceptance; their xi = 1 - E/E_beam runs from zero to 0.55 with a median of 0.1. The SIM step starts these
   protons *at the pots* and this is where their hits will be.

> **In plain words: modules, products and the schedule.** A CMSSW job is a set of *modules* (producers that
> add products to the event, filters that can stop the processing of an event, analyzers that only read,
> output modules that write) and a *schedule* that says in which order they run for every event. Every
> product is identified by its type, the module that made it, an optional instance label and the process
> name: "`edm::HepMCProduct generatorSmeared GEN`" is the smeared HepMC event made by the module
> `generatorSmeared` in the process `GEN`. `edmDumpEventContent file.root` lists them.

### 3.5 The vertex smearing

Pythia8 puts every collision at the origin. The real collisions are spread over the *luminous region*, the
overlap of the two bunches, and this is what `VtxSmeared` reproduces. For Run 3 (`--beamspot DBrealistic`)
the module is the `BetafuncEvtVtxGenerator` of `IOMC/EventVertexGenerators`, a Gaussian in z and in time
whose transverse width follows the beta function of the beams:

```python
# IOMC/EventVertexGenerators/python/VtxSmearedRealistic_cfi.py
from IOMC.EventVertexGenerators.BetafuncEvtVtxGenerator_cfi import BetafuncEvtVtxGenerator
VtxSmeared = BetafuncEvtVtxGenerator.clone(src = "generator:unsmeared", readDB = True)
```

With `readDB = True` the beam parameters come from the `SimBeamSpotObjectsRcd` payload of the global tag,
here `SimBeamSpot_Realistic25ns13p6TeVEarly2024CollisionVtxSmearingParameters_v1_mc` (`conddb list`,
`conddb dump`):

| parameter | meaning | 2024 payload |
|---|---|---:|
| `X0`, `Y0`, `Z0` [cm] | the centre of the luminous region | 0.0285, -0.0166, 0.416 |
| `SigmaZ` [cm] | r.m.s. length of the luminous region along the beam | 3.6 |
| `BetaStar` [cm] | beta* at the interaction point | 30 |
| `Emittance` [cm] | the (non-normalised) emittance | 5.725e-8 |
| `Phi`, `Alpha` | tilt angles of the luminous region | 0, 0 |
| `TimeOffset` [ns] | the centre in time | 0 |

The module draws z from a Gaussian of width `SigmaZ` around `Z0`; x and y from a Gaussian of width
sigma_xy(z) = sqrt(emittance x beta* x [1 + ((z - z0)/beta*)^2]) / sqrt(2), the beam size at that z divided
by sqrt(2) for the overlap of two beams (9 um at the centre); and the time from a Gaussian of the same width
as z in units of mm/c, i.e. sigma_t = 3.6 cm / c = 120 ps. In the 250 events the vertices have (sample
r.m.s.) sigma_x = sigma_y = 10 um, sigma_z = 3.56 cm around z = 0.08 cm and sigma_t = 122 ps, with z from
-9.5 to +10.1 cm and t from -0.39 to +0.37 ns: the payload within the statistics of 250 events. The time
is *part of the generated event*: every later step starts the particles at it, and the mixing will only add
the bunch-crossing offset.

> **In plain words: the same beam spot as the signal, on purpose.** The signal bbbar event was generated
> with the very same module and payload; its vertex is at z = 3.11 cm. The pileup collisions have to be
> spread the same way, because nothing downstream will move them again. A minimum-bias sample generated
> with the HL-LHC beam spot (sigma_z = 4.3 cm, sigma_t = 190 ps, a crossing angle) mixed into a 2024 signal
> would simply be wrong, and nothing in the mixing job would notice.

---

## 4. Reading the log

`step1_GEN_MinBias_out.log` is 800 lines, almost all of them Pythia's. In order:

| lines | block | what to look for |
|---|---|---|
| banner | "Welcome to the Lund Monte Carlo! This is PYTHIA version 8.311" | the Pythia version; it appears twice (master and stream instance) |
| LHAPDF | "LHAPDF 6.4.0 loading .../NNPDF31_nnlo_as_0118_0000.dat" | the PDF set actually used (LHAPDF ID 303600) |
| Process Initialization | "We collide p+ with p+ at a CM energy of 1.360e+04 GeV" and the table of the five subprocesses with their cross sections | the energy really is 13.6 TeV; the cross sections of section 2.1 |
| Multiparton Interactions Initialization | "sigmaNonDiffractive = 55.42 mb", "pT0 = 1.44 gives sigmaInteraction = 492.18 mb: accepted", then the "diffraction XB / AX / AXB" blocks with a pT0 per diffractive mass | the MPI regularisation of section 2.2 and 2.3 |
| Settings (changes only) | every Pythia parameter that differs from its default, with the default beside it | the three blocks of section 3.1 plus `Beams:eCM`; the place to check that a setting was really applied |
| Particle Data Table (changed only) | "no particle data has been changed" | no masses or branching ratios were overridden |
| `Begin processing the 1st record ... 250th record` | one line per event with a timestamp | 250 events in 2.6 s, after about 30 s of initialisation |
| Event and Cross Section Statistics | the five subprocesses with tried / selected / accepted and sigma | 183 / 27 / 18 / 19 / 3, sum 250; the observed process mix |
| Error and Warning Messages Statistics | "1 Error in BeamRemnants::setKinematics: ... failed after retrying 10 times", 44 "weight above unity" warnings | one event was regenerated after a kinematics failure; the shower warnings are benign |
| GenXsecAnalyzer | "total cross section = 7.842e+10 +- 3.028e+02 pb", "Filter efficiency (event-level) = 250/250" | the cross section the sample corresponds to and the filter efficiency (what McM records) |

The timing is worth a glance: the generation of a minimum-bias event costs 10 ms, the initialisation
(Pythia's MPI integration tables, the LHAPDF grids) half a minute. A GEN-only job of a million minimum-bias
events would take three hours; it is the SIM step, at 2.5 s per event, that makes a library expensive.

---

## 5. What is in the GEN file

`edmDumpEventContent step1_GEN_MinBias_out.root` lists 31 products in every event, all from the process
`GEN`:

| product (type, module:label) | content |
|---|---|
| `edm::HepMCProduct generatorSmeared` | the full generator record of the event, vertex smeared: every particle of every stage with its status code, mothers, daughters and four-momentum, and every vertex with its position and time. The only HepMC record the next steps read |
| `edm::HepMCProduct LHCTransport` | the forward protons at the PPS Roman pots (empty in 169 of the 250 events) |
| `GenEventInfoProduct generator` | process id, event weight (1 here), `qScale` = pT-hat of the hardest interaction, alpha_s values, PDF information |
| `vector<reco::GenParticle> genParticles`, `vector<int> genParticles`, `genParticles:xyz0`, `genParticles:t0` | the record as a flat collection with mother/daughter links, the HepMC barcode of every entry, and the smeared vertex position and time |
| `vector<reco::GenJet> ak4GenJets`, `ak8GenJets`, `ak4GenJetsNoNu`, `ak8GenJetsNoNu` (+ `rho`, `sigma`, `rhos`, `sigmas`) | anti-kT jets of the stable particles and the pileup-density estimates of the jet area method |
| `vector<reco::GenMET> genMetTrue`, `genMetCalo` | the generator MET from all stable particles, and from the ones a calorimeter would see |
| `edm::RandomEngineStates randomEngineStateProducer` | the state of the `generator` and `VtxSmeared` engines at the start of the event |
| `edm::TriggerResults` | the result of the generator filter (always true here) |

The unsmeared record `generator:unsmeared` is *not* in the file: the event content keeps only
`GenEventInfoProduct` from the module `generator` and takes the HepMC record from `generatorSmeared`.

The file is 11 MB for 250 events, 44 kB per event after compression, of which `edmEventSize -v` attributes
29 kB to the HepMC record, 11.5 kB to the `GenParticle`s, 0.8 kB to the engine states, 1.5 kB to the four
jet collections and 50 bytes to the PPS protons.

**The record of one event.** Event 1, a non-diffractive collision with 18 interactions, holds 677 particles
and 364 vertices, of which 242 are stable. The others are the intermediate stages that Pythia keeps so
that the history of every hadron can be traced; the conversion to HepMC keeps the Pythia status codes for
them. Over the 250 events:

| status | Pythia meaning | entries | per event |
|---|---|---:|---:|
| 1 | stable final-state particle | 48 437 | 194 |
| 2 | decayed hadron or tau | 23 116 | 92 |
| 4 | the two incoming beam protons | 500 | 2 |
| 13, 15 | incoming / outgoing particle of a diffractive subsystem | 198 | 0.8 |
| 21, 23 | incoming / outgoing parton of the hardest interaction | 956 | 3.8 |
| 31, 33 | incoming / outgoing parton of an additional interaction (MPI) | 6 028 | 24 |
| 41-44 | initial-state radiation | 11 832 | 47 |
| 51-53 | final-state radiation | 12 852 | 51 |
| 61-63 | beam remnants (with primordial kT) | 14 573 | 58 |
| 71-74 | partons copied into the strings for hadronisation | 10 964 | 44 |

(With 3014 status-31 entries in 183 ND events, i.e. 8 per event, plus the hard interaction, the mean number
of interactions of section 2.2 follows.)

**The stable particles of two events** (`minBiasGenStats.py ... --event 1 12`):

```
event 1 (ND, 18 interactions, 677 particles and 364 vertices in the record): stable particles by pT
    pi+-    pT   2.357  eta   -2.34  E      12.4 GeV
    pi+-    pT   1.841  eta   -2.46  E      10.9 GeV
    n       pT   1.839  eta    5.17  E     161.8 GeV
    pi+-    pT   1.439  eta    2.55  E       9.2 GeV
    ... 242 in all: 118 photons, 87 pi+-, 9 K+-, 7 K0S, 6 n, 5 K0L, 3 Sigma-, 2 p, 2 Lambda, 2 Sigma+, 1 Xi-
event 12 (SD, 1 interaction, 15 particles and 9 vertices in the record): stable particles by pT
    Lambda  pT   0.997  eta    9.08  E    4399.0 GeV
    K0L     pT   0.737  eta    8.68  E    2173.3 GeV
    p       pT   0.276  eta  -10.81  E    6800.0 GeV
    pi+-    pT   0.066  eta    8.84  E     227.7 GeV
```

Event 1 is the one drawn on the detector in `PULib.md`; its hardest particle is a 2.4 GeV pion. Event 12
is a single-diffractive collision at the bottom of the mass spectrum: the proton keeps the full beam energy
(xi = 3 x 10^-8) and the other proton turns into a 2.6 GeV system of three hadrons at eta = 9, all of which
will fly down the beam pipe. Four stable particles, no detector hit.

---

## 6. The 250 events at generator level

The catalogue written by `minBiasGenStats.py` (`PU_2024/genstats.txt`, one line per event), first rows:

| event | proc. | interactions | stable | charged | central | \|eta\| > 5.5 | lead pT [GeV] | sum pT [GeV] | jets > 10 GeV | PPS protons | z [cm] | t [ns] |
|--:|:--|--:|--:|--:|--:|--:|--:|--:|--:|--:|--:|--:|
| 1 | ND | 18 | 242 | 104 | 16 | 62 | 2.36 | 89.0 | 0 | 0 | -3.30 | 0.365 |
| 2 | DD | 1 | 66 | 28 | 0 | 35 | 1.34 | 21.3 | 0 | 0 | -8.20 | -0.179 |
| 3 | ND | 25 | 402 | 188 | 32 | 68 | 2.12 | 134.1 | 0 | 0 | 0.69 | -0.031 |
| 4 | ND | 15 | 234 | 96 | 22 | 53 | 3.76 | 113.5 | 0 | 0 | 6.24 | -0.172 |
| 5 | DD | 1 | 21 | 8 | 0 | 18 | 0.61 | 4.5 | 0 | 0 | -0.20 | -0.301 |
| 6 | ND | 1 | 65 | 34 | 4 | 29 | 0.86 | 18.9 | 0 | 0 | 6.21 | -0.110 |
| 7 | ND | 2 | 83 | 46 | 8 | 33 | 1.67 | 30.5 | 0 | 0 | -3.51 | 0.008 |
| 8 | ND | 9 | 185 | 82 | 9 | 45 | 2.07 | 70.7 | 0 | 0 | 1.49 | -0.109 |
| 9 | ND | 20 | 323 | 130 | 48 | 47 | 3.00 | 128.7 | 0 | 0 | -2.68 | 0.065 |
| 10 | ND | 26 | 390 | 170 | 43 | 66 | 2.46 | 160.7 | 0 | 0 | 1.21 | 0.129 |
| 11 | ND | 5 | 119 | 66 | 8 | 41 | 2.53 | 50.2 | 0 | 0 | -3.92 | -0.193 |
| 12 | SD | 1 | 4 | 2 | 0 | 4 | 1.00 | 2.1 | 0 | 1 | -1.17 | 0.032 |
| 13 | ND | 1 | 78 | 34 | 4 | 31 | 0.85 | 22.0 | 0 | 1 | 6.57 | -0.196 |
| 247 | ND | 39 | 723 | 320 | 126 | 87 | 4.76 | 376.1 | 2 | 0 | 0.29 | 0.040 |

(the column "interactions" counts the hardest scattering plus the MPIs; "central" is |eta| < 2.5 and
pT > 0.5 GeV; "PPS protons" is the size of the `LHCTransport` record). The summary of the sample:

| per event | mean | median | min | max |
|---|--:|--:|--:|--:|
| interactions | 7.0 | 3 | 1 | 39 |
| stable particles | 194 | 132 | 3 | 723 |
| charged stable particles | 89 | 59 | 2 | 320 |
| central (\|eta\| < 2.5, pT > 0.5 GeV) | 21 | 10 | 0 | 126 |
| beyond \|eta\| = 5.5 | 39 | 37 | 3 | 93 |
| pT of the hardest particle [GeV] | 2.2 | 1.8 | 0.2 | 8.1 |
| scalar sum of pT [GeV] | 79 | 45 | 0.5 | 376 |
| ak4 jets above 10 GeV | 0.15 | 0 | 0 | 3 |
| protons at the PPS | 0.37 | 0 | 0 | 2 |
| vertex z [cm] | 0.08 | -0.05 | -9.5 | 10.1 |
| vertex t [ns] | -0.007 | -0.015 | -0.39 | 0.37 |

| per process | events | interactions | stable | charged | central | lead pT [GeV] | sum pT [GeV] |
|---|--:|--:|--:|--:|--:|--:|--:|
| non-diffractive (ND) | 183 | 9.0 | 241 | 111 | 27 | 2.5 | 100 |
| single diffractive (SD) | 45 | 2.0 | 70 | 33 | 7 | 1.4 | 26 |
| double diffractive (DD) | 19 | 1.2 | 57 | 24 | 2 | 1.1 | 17 |
| central diffractive (CD) | 3 | 2.0 | 35 | 16 | 2 | 0.8 | 9 |

Four things to take from these tables:

- **The sample is not 250 copies of the same event.** The multiplicity spans a factor of 240 between event
  132 (3 stable particles) and event 247 (723). The median event has two thirds of the mean; the top 10 %
  of the events hold a third of all the particles. When the mixing later stacks 60 of these for a bunch
  crossing, the sum is dominated by the few large non-diffractive ones.
- **Within the non-diffractive class the number of interactions sets the scale**, almost linearly
  (section 2.2): one interaction gives 85 particles, twenty give about 440, thirty-nine give 723. The process type
  then separates the diffractive events below.
- **The events are forward.** A fifth of the particles are beyond |eta| = 5.5, where nothing will see them
  (except the PPS, for the protons), and the density is flat out to |eta| = 4. Only 21 particles per event,
  10 in the median one, are "central" by the loose definition of the table. This is what will make the EE
  and the HF, not the barrel, the detectors that collect most of a minimum-bias event's energy.
- **The events are soft.** The hardest particle has 2.2 GeV on average and 8.1 GeV at most; a jet above
  10 GeV appears once in twelve events. The signal event of the book has a single parton pair at
  pT-hat > 100 GeV; a minimum-bias event has nothing comparable, and nothing that a trigger would fire on.

> **In plain words: mean, median and the big events.** Ask "how many particles does a pileup collision
> add?" and the honest answer is "between 3 and 700, typically 130, on average 190". Any expectation built
> from the average is right over many bunch crossings, but a single bunch crossing of 60 collisions
> fluctuates more than sqrt(60) would suggest, because its sum is a sum of very unequal terms.

---

## 7. Running it

Everything below runs from `JitJetSW/Journey/test` in an initialised CMSSW_15_0_5 environment:

```bash
cd CMSSW_15_0_5/src
eval "$(scram runtime -sh)"        # or: cmsenv
cd JitJetSW/Journey/test
```

### 7.1 Generating the events: `SIM=0 ./runMinBias_mc2024.sh`

`runMinBias_mc2024.sh` is the script that makes the pileup library (GEN then SIM, `PULib.md`). With `SIM=0`
it stops after the GEN step and does what this chapter describes and nothing more: it writes the
configuration of section 3.4 with `cmsDriver.py`, runs it, and prints the generator-level checks.

```bash
SIM=0 ./runMinBias_mc2024.sh                       # 250 events at 13.6 TeV, seeds 20241/20242, into PU_2024/ (about 1 min)
SIM=0 STATS=1 ./runMinBias_mc2024.sh               # same, plus the per-event catalogue PU_2024/genstats.txt (section 6)
SIM=0 RUN=0 OUTDIR=configs ./runMinBias_mc2024.sh  # only write the cmsDriver configuration, to read or edit it
```

| variable (default) | meaning |
|---|---|
| `SIM=0` | stop after the GEN step (default 1: continue with Geant4 and make the library) |
| `PUEVENTS=250` | number of events generated |
| `PUSEED=20241` | random-seed base: `generator` and `VtxSmeared` get `PUSEED` and `PUSEED` + 1; a new seed is a new sample |
| `COMENERGY=13600.0` | centre-of-mass energy written over the fragment's `comEnergy` |
| `OUTDIR=PU_2024` | where everything goes |
| `REUSE=1` | keep an existing `step1_GEN_MinBias_out.root` whatever `PUEVENTS`, `PUSEED` and `COMENERGY` say; `REUSE=0` regenerates |
| `RUN=0` | only write the configuration |
| `CHECK=1` | print the checks of section 7.4 into `gencheck.log` (`CHECK=0` skips) |
| `STATS=1` | run `minBiasGenStats.py` on the GEN file into `genstats.txt` (default 0; about a minute) |
| `CONDITIONS`, `ERA`, `BEAMSPOT`, `FRAGMENT` | the 2024 settings (`auto:phase1_2024_realistic`, `Run3_2024`, `DBrealistic`, `MinBias_14TeV_pythia8_TuneCP5_cfi`); change them together for another scenario |

Variations:

```bash
SIM=0 REUSE=0 PUSEED=777 ./runMinBias_mc2024.sh                   # another 250 collisions
SIM=0 REUSE=0 PUEVENTS=10000 ./runMinBias_mc2024.sh               # 10 000 collisions (about 2 minutes: 10 ms each)
SIM=0 REUSE=0 COMENERGY=14000 ./runMinBias_mc2024.sh              # the RelVal choice: 14 TeV
SIM=0 REUSE=0 FRAGMENT=MinBias_13TeV_pythia8_TuneCUETP8M1_cfi COMENERGY=13600 ./runMinBias_mc2024.sh   # the Run-2 tune
SIM=0 OUTDIR=PU_2023 CONDITIONS=auto:phase1_2023_realistic ERA=Run3_2023 ./runMinBias_mc2024.sh        # the 2023 beam spot
SIM=0 OUTDIR=GEN_NoSmear BEAMSPOT=NoSmear ./runMinBias_mc2024.sh                                       # no vertex smearing
```

Output in `OUTDIR`:

| file | content |
|---|---|
| `step1_GEN_MinBias_cfg.py` (+ `.driver.log`) | the configuration written by `cmsDriver.py` (and `cmsDriver.py`'s own output) |
| `step1_GEN_MinBias_out.root` (11 MB) | the 250 generated events (section 5) |
| `step1_GEN_MinBias_out.log` | the Pythia8 initialisation, the event loop, the statistics (section 4) |
| `gencheck.log` | the checks of section 7.4 |
| `genstats.txt`, `genstats.log` | with `STATS=1`: the catalogue of section 6 and the FWLite warnings |

The same file is what the full pileup script makes first: `CASES=pileup ./runPileup_mc2024_BBbar.sh`
and `./runMinBias_mc2024.sh` without `SIM=0` run this GEN step and then Geant4 on its output.

### 7.2 By hand

The two commands of section 3.4 are all there is. To change a Pythia setting, append a line to
`--customise_commands`, e.g. non-diffractive collisions only:

```bash
cmsDriver.py MinBias_14TeV_pythia8_TuneCP5_cfi --conditions auto:phase1_2024_realistic --era Run3_2024 \
  --geometry DB:Extended --beamspot DBrealistic -n 1000 -s GEN --datatier GEN --eventcontent FEVTDEBUG \
  --fileout file:step1_GEN_MinBiasND_out.root --python_filename step1_GEN_MinBiasND_cfg.py --no_exec \
  --customise_commands "process.generator.comEnergy = cms.double(13600.0)\nprocess.RandomNumberGeneratorService.generator.initialSeed = 20241\nprocess.RandomNumberGeneratorService.VtxSmeared.initialSeed = 20242\nprocess.generator.PythiaParameters.processParameters = cms.vstring('SoftQCD:nonDiffractive = on')"
cmsRun step1_GEN_MinBiasND_cfg.py > step1_GEN_MinBiasND_out.log 2>&1
```

or edit the generated `_cfg.py` (the `generator` module is spelled out in full, all three blocks included)
and run `cmsRun` on it.

### 7.3 The catalogue: `minBiasGenStats.py`

```bash
python3 minBiasGenStats.py PU_2024/step1_GEN_MinBias_out.root                  # one line per event + the summary of section 6
python3 minBiasGenStats.py PU_2024/step1_GEN_MinBias_out.root 20               # the first 20 events only
python3 minBiasGenStats.py PU_2024/step1_GEN_MinBias_out.root --event 1 12     # + the stable particles of events 1 and 12
```

The script reads the GEN file with FWLite (no `cmsRun`, no configuration): the process code from
`signal_process_id()` of the HepMC record, the interactions from the status-31 partons, the stable
particles (status 1) with their species, pT and eta, the vertex of the first HepMC vertex (positions in mm,
times in mm/c), the ak4 jets and the size of the `LHCTransport` record; and for the single-diffractive
events the mass of everything but the most energetic proton. It is 90 lines of Python and meant to be
edited: another column is a few lines.

### 7.4 Checks before the events are used

A generated sample that passes these checks can go through the SIM step and be mixed into a signal made
with the same settings (`gencheck.log` holds them):

1. `grep -E "Process\(|GlobalTag\(|VtxSmeared|comEnergy" step1_GEN_MinBias_cfg.py`: the same era, global tag,
   vertex-smearing module and energy as the signal's GEN configuration;
2. `grep initialSeed step1_GEN_MinBias_cfg.py`: seeds that differ from the signal's and from the
   `cmsDriver.py` defaults;
3. `grep -A12 "PYTHIA Event and Cross Section Statistics" step1_GEN_MinBias_out.log`: the five subprocesses
   with the cross sections of section 2.1 and a sum equal to the number of events;
4. `edmFileUtil -e step1_GEN_MinBias_out.root`: the number of events written.

### 7.5 Inspecting the events

```bash
edmDumpEventContent PU_2024/step1_GEN_MinBias_out.root        # the products of section 5
edmEventSize -v PU_2024/step1_GEN_MinBias_out.root            # bytes per event and per branch
edmFileUtil -e PU_2024/step1_GEN_MinBias_out.root             # run / lumi / event numbers
grep -B2 -A12 "PYTHIA Process Initialization" PU_2024/step1_GEN_MinBias_out.log      # energy and cross sections
grep -A40 "Settings (changes only)" PU_2024/step1_GEN_MinBias_out.log | head -45     # every non-default Pythia setting
conddb list 150X_mcRun3_2024_realistic_v2 | grep SimBeamSpot  # the beam-spot tag; conddb dump <hash> prints the payload
```

**To see a full Pythia event record** in the log, let the generator print the first event:

```bash
SIM=0 RUN=0 OUTDIR=listing ./runMinBias_mc2024.sh
cd listing
cat >> step1_GEN_MinBias_cfg.py << 'EOF'
process.generator.maxEventsToPrint = cms.untracked.int32(1)      # the first event of every stream
process.generator.pythiaPylistVerbosity = cms.untracked.int32(1)  # Pythia's info.list() and event.list()
process.generator.pythiaHepMCVerbosity = cms.untracked.bool(True) # and the HepMC record after conversion
process.maxEvents.input = 3
EOF
cmsRun step1_GEN_MinBias_cfg.py > step1_GEN_MinBias_out.log 2>&1
```

The listing has one line per particle (index, id, name, status, mothers, daughters, colour tags, px, py,
pz, E, m), from the two beam protons (status 12 in Pythia's own numbering, 4 in HepMC) through the hard
interaction, the MPIs, the showers, the remnants and the strings to the stable hadrons. The GEN chapter's
`GenPartAnalyzer` (`cmsRun genPartAnalyzer_cfg.py inputFiles=file:PU_2024/step1_GEN_MinBias_out.root`)
draws the same stages in the eta-phi plane, stage by stage, for any event of the file; on a minimum-bias
event the "hard process" page shows the soft 2 -> 2 scattering of section 2.2 and the MPI page the rest.

### 7.6 Other minimum-bias samples

The recipe is the same for any scenario; only the fragment and the settings change:

- **Phase 2**: the same fragment at its nominal 14 TeV with `--beamspot DBrealisticHLLHC` and the Run4D110
  conditions and era of `runCMSDrivers_mcPhase2_BBbar_SIM.sh`: the GEN step of `doc/pu/Phase2/PULib.tex`;
- **another year of Run 3**: `CONDITIONS=auto:phase1_2022_realistic ERA=Run3` or
  `CONDITIONS=auto:phase1_2023_realistic ERA=Run3_2023`, whose global tags carry their own beam-spot
  payloads;
- **another tune**: `MinBias_13TeV_pythia8_TuneCUETP8M1_cfi` is the Run-2 (2016) tune of the same process;
  the CP5 settings can also be overridden one by one in `--customise_commands`
  (`process.generator.PythiaParameters.pythia8CP5Settings = cms.vstring(...)`);
- **one process class**: replace `processParameters` by `SoftQCD:nonDiffractive = on`,
  `SoftQCD:singleDiffractive = on`, ... (section 7.2) to study a class alone; `SoftQCD:all` adds the elastic
  collisions, which leave nothing in the detector and are not pileup;
- **another generator**: EPOS, Herwig7 or any fragment that produces inelastic collisions works, as long as
  it runs through the same `pgen` sequence and vertex smearing;
- **no vertex smearing** (`BEAMSPOT=NoSmear`): every collision stays at the origin; a sample used for
  detector studies only, and one that must never be mixed into a smeared signal.

---

## 8. The signal event under pileup: the generator jets with 55 collisions on top

Everything so far was about the pileup collisions on their own. This section puts them where they belong,
*on top of the signal*, and asks the first question an analysis asks of pileup: what does it do to the
jets? It stays at generator level, so that the answer is not yet mixed with anything the detector does: the
stable particles of the in-time pileup collisions are added to the stable particles of the bbbar signal
event, and the anti-kT clustering of the GenJet stage of the GEN chapter is run again on the sum.

The signal event is event 1 of the GEN chapter, the one followed from the matrix element to the jets in
the eight pages of `GEN/genJetAnalyzer_ME_PS_MPI_Had_Decay_GenJet_out.pdf` (five generator stages, then
ak4GenJets, ak4GenJetsNoNu, ak8GenJets). The pileup collisions are the 55 events of the library that the
DIGI step of the pileup chapter (`runPileup_mc2024_BBbar.sh`, `PUSIM.tex`) mixed into the *in-time* bunch
crossing of that same signal event: the MixingModule reads the library in sequence, bunch crossing by
bunch crossing, and `PU_2024/mixed/simHits.log` records 66 events in bunch crossing -1, 55 in bunch
crossing 0 and 77 in bunch crossing 1 ("events per bx: -1:66 0:55 1:77"), so the in-time collisions are
library events 67 to 121. Only these matter for generator jets: the early and late collisions are seen by
the detector through its time response (the subject of the mixing chapter), but a jet of particles has no
time axis, and anti-kT on a generator record is by construction an in-time picture. The result is the ninth
page of `PU_2024/genJetAnalyzer_ME_PS_MPI_Had_Decay_GenJet_PU_out.pdf`, a copy of the eight GEN pages
followed by the same event with the pileup on it:

![ak4 jets (with neutrinos) of signal event 1 with the 55 in-time pileup collisions overlaid](../../test/PU_2024/genJetAnalyzer_ME_PS_MPI_Had_Decay_GenJet_PU_out_page-9.png)

The page is drawn like page 6 (ak4GenJets without pileup), with three additions: the pileup particles are
triangles (filled in the jet colour when clustered, light grey and open when not), the signal particles
circles as before; the jets above 30 GeV, and the b and bbar jets whatever their pT, are drawn with their
circle of radius R and their label, the others only through their constituents, because there are now
262 jets; and every jet in the right-hand list carries the share of its pT that comes from pileup
(scalar sum of the pT of its pileup constituents over the jet pT; it can exceed 100 % for a jet made of
pileup only, since the constituents are not collinear).

### 8.1 What the page shows

| | page 6: signal alone | page 9: signal + 55 in-time collisions |
|---|---:|---:|
| stable particles (status 1) | 634 | 634 signal + 11 309 pileup (12 to 606 per collision, median 161) |
| ak4 jets with pT > 3 GeV | 27 | 262 |
| jets with pT > 20 / 30 / 50 GeV | 2 / 2 / 2 | 105 / 67 / 8 |
| b jet: pT, constituents | 117.8 GeV, 24 | 152.5 GeV, 112 (24 signal + 88 pileup), 35.5 GeV from pileup (23 %) |
| bbar jet: pT, constituents | 115.9 GeV, 18 | 150.2 GeV, 83 (18 signal + 65 pileup), 35.0 GeV from pileup (23 %) |
| response pT(jet) / pT(quark), b / bbar | 1.284 / 0.996 | 1.662 / 1.291 |
| Delta R (jet axis, quark), b / bbar | 0.019 / 0.004 | 0.057 / 0.029 |
| dijet mass m(b jet + bbar jet) | 386.8 GeV | 520.5 GeV |
| \|pT(b jet) + pT(bbar jet)\| | 6.1 GeV | 7.8 GeV |
| signal particles inside a jet | 239 of 634 | 629 of 634 |
| jets without a single signal particle | - | 77 |

Four things to read off the page and the table:

1. **The pileup is a uniform pedestal.** The 55 collisions put 2067 GeV of transverse momentum into
   \|eta\| < 2.5, that is 66 GeV per unit of eta-phi area (68 GeV at 1.2 < \|eta\| < 2.0, 60 GeV at
   3.4 < \|eta\| < 4.2: flat within what one event can tell). A cone of R = 0.4 covers pi R^2 = 0.50 units
   of area and so collects about 33 GeV wherever it is placed; the b jet at eta = -1.6 and the bbar jet at
   eta = -3.8 collected 35.5 and 35.0 GeV. This is the number that the pileup corrections of the
   reconstruction (the "L1 offset" of the jet energy corrections, rho x area) will later subtract, and the
   page shows where it comes from: 88 and 65 soft triangles, 0.4 GeV each on average, inside the circles.
2. **The b and bbar jets kept all their signal particles** (24 and 18, the same ones as on page 6, 118.1
   and 116.0 GeV of scalar pT), but their axes moved by 0.04 and 0.03 in Delta R towards the pileup inside
   the cone, and their pT, their response and their invariant mass are shifted by the full pedestal: the
   dijet mass goes from 387 to 521 GeV. Nothing in the event tells anti-kT which triangle belongs to the
   signal; the algorithm clusters what it is given.
3. **Pileup makes jets of its own.** 262 jets instead of 27, 105 of them above 20 GeV and six between 50
   and 60 GeV ([2] to [7] in the list, 95-101 % pileup): 55 soft collisions, each with its MPIs, fluctuate
   enough in one place for a 0.4 cone to reach 50 GeV. In the detector these are the "pileup jets" that a
   pileup-jet identification has to remove, and at generator level the page shows what they are made of:
   many soft particles from many collisions (jet [2] has 102 pileup constituents from a sample of 55
   collisions, and 3 from the signal).
4. **The distinction "signal jet / pileup jet" is not sharp.** Only 77 of the 262 jets contain no signal
   particle at all; 64 of the 67 jets above 30 GeV contain at least one, because the 634 signal particles
   (MPI, beam remnants, the underlying event of the GEN chapter) are spread over the whole plane and a cone
   anywhere catches a few. That is why 629 of the 634 signal particles are now inside a jet, against 239
   without pileup: they were not in a jet because no jet reached 3 GeV around them; now one always does.

> **In plain words: pileup and jets.** Spread 11 000 soft particles evenly over the detector and draw a
> circle of radius 0.4 anywhere: it contains about 35 GeV of them. A jet is such a circle, so every jet of
> the signal is 35 GeV heavier than it was, and wherever the soft particles happen to be a little denser
> the circle alone passes the jet threshold and a jet appears that nobody produced. The first effect is
> corrected on average (one subtracts what a circle contains on average); the second is fought by asking
> whether the particles in the circle come from one vertex or from many, which is what the tracker will
> make possible and the next chapters will come to.

### 8.2 How the overlay is made

No new code touches the particles: the overlay is the MixingModule of the DIGI step, restricted to the
generator record, followed by the standard modules of the GEN step (`python/genJetPileup_cff.py`):

1. **`mix` (MixingModule)** reads the `generatorSmeared` HepMCProduct of the signal event from the source
   and those of N library events from its secondary input (`EmbeddedRootSource` on the GEN file of the
   library, `sequential = True`, the first `skipEvents` skipped), and puts them together in a
   `CrossingFrame<HepMCProduct>` for the single bunch crossing `minBunch = maxBunch = 0`. Its `mixObjects`
   holds only `mixHepMC` and its `digitizers` PSet is empty, so no SimHit is read and nothing is digitised;
   the `PileupMixingContent` it writes records which library events went in. With `type = 'fixed'` and
   `averageNumber = 55` exactly 55 collisions are taken; `type = 'poisson'` draws the number as the DIGI
   step does.
2. **`genParticlesPU` (GenParticleProducer)** is the module that makes `genParticles` in every GEN job,
   with `useCrossingFrame = True`: it converts every HepMC event of the frame, the signal first and then
   the pileup collisions, into one `reco::GenParticle` collection, and stamps every particle with the
   index of its collision, `GenParticle::collisionId()` (0 = signal, 1 to 55 = the pileup collisions; the
   heavy-ion workflows use the same mechanism for their sub-events). Mother-daughter links stay within
   a collision; the hard-process flags of the signal are untouched, so the b and bbar can be followed as
   before.
3. **`genParticlesForJetsPU`, `ak4GenJetsPU`** (and the NoNu and ak8 variants) are the input selector and
   the FastjetJetProducer of the GenJet stage with `src` pointed at the merged collection: the status 1
   particles of all 56 collisions, anti-kT with R = 0.4, jet pT > 3 GeV.
4. **GenJetAnalyzer** draws `ak4GenJetsPU` over `genParticlesPU` with `signalCollisionId = 0`: particles
   of another collision are pileup (triangles), the hard-process quarks are searched in the signal only,
   and the pileup pT of every jet is the scalar sum of its pileup constituents.

The page is therefore not an approximation of the mixing but the same mixing stopped before the detector:
the collisions on page 9 are, event by event, the in-time collisions of the DIGI step of the mixing chapter.

### 8.3 Running it: `./runGenJetPileup_mc2024_BBbar.sh`

```bash
./runGenJetPileup_mc2024_BBbar.sh                          # signal event 1 + the in-time collisions of PU_2024/mixed/simHits.log (8 s)
NPU=55 SKIP=66 ./runGenJetPileup_mc2024_BBbar.sh           # the same, given by hand
NPU=20 SKIP=0 ./runGenJetPileup_mc2024_BBbar.sh            # library events 1-20 instead
PUTYPE=poisson NPU=60 PUSEED=7 ./runGenJetPileup_mc2024_BBbar.sh   # Poisson(60) collisions, as the DIGI step draws them
JETS=ak4GenJetsPU,ak4GenJetsNoNuPU,ak8GenJetsPU ./runGenJetPileup_mc2024_BBbar.sh   # three new pages (9, 10, 11)
LABELPTMIN=20 ./runGenJetPileup_mc2024_BBbar.sh            # draw the circles and labels of the jets above 20 GeV
```

| variable (default) | meaning |
|---|---|
| `EVENT=1:1:1` | the signal event |
| `NPU`, `SKIP` | in-time collisions and library events skipped before them; default: read from `OUTDIR/mixed/simHits.log` (66 and 55 for this event), else 60 and 0 |
| `PUTYPE=fixed` | `fixed`: exactly `NPU` collisions; `poisson`: Poisson(`NPU`) with seed `PUSEED` (20243) |
| `JETS=ak4GenJetsPU` | collections drawn, one page each: `ak4GenJetsPU`, `ak4GenJetsNoNuPU`, `ak8GenJetsPU`, `ak8GenJetsNoNuPU` |
| `JETPTMIN=3` | jet pT threshold of the producers, as in the GenJet stage |
| `LABELPTMIN=30` | jets drawn with circle and label (the b / bbar jets always) |
| `STAGE=ME_PS_MPI_Had_Decay` | generator stage of the signal: `GEN/step1_GEN_<STAGE>_out.root` and `GEN/genJetAnalyzer_<STAGE>_GenJet_out.pdf` (made with the GEN script if missing) |
| `PU_INPUT=PU_2024/step1_GEN_MinBias_out.root` | the GEN file of the library (section 7.1; made with `SIM=0 ./runMinBias_mc2024.sh` if missing) |
| `OUTDIR=PU_2024` | output directory |

Output in `OUTDIR`:

| file | content |
|---|---|
| `genJetPileup_<STAGE>_PU_out.log` | the cmsRun log: every jet with pT, eta, phi, mass, constituents and pileup pT; the b / bbar matching |
| `genJetPileup_<STAGE>_PU_out.pdf`, `.root` | the new page(s) alone, and the jet histograms (TFileService) |
| `step2_GENJETPU_<STAGE>_PU_out.root` | EDM file with `genParticlesPU` (33 028 particles, `collisionId` per particle), the four jet collections and the `mix` products; `edmDumpEventContent` lists them |
| `genJetAnalyzer_<STAGE>_GenJet_PU_out.pdf` | the book PDF: the 8 pages of `GEN/genJetAnalyzer_<STAGE>_GenJet_out.pdf` + the new page(s) (`pdfunite`) |
| `genJetAnalyzer_<STAGE>_GenJet_PU_out_page-9.png` | the new page as PNG (`pdftoppm -cropbox -r 110`), the figure above |

By hand, the script runs one `cmsRun` (`test/genJetPileup_cfg.py`, all options are VarParsing arguments)
and one `pdfunite`:

```bash
cd PU_2024
cmsRun ../genJetPileup_cfg.py inputFiles=file:../GEN/step1_GEN_ME_PS_MPI_Had_Decay_out.root \
  pileupFiles=file:step1_GEN_MinBias_out.root eventsToProcess=1:1:1 nPileup=55 skipPileup=66 pileupType=fixed \
  jets=ak4GenJetsPU jetPtMin=3 labelPtMin=30 outputFile=genJetPileup_ME_PS_MPI_Had_Decay_PU_out.root \
  edmOutput=step2_GENJETPU_ME_PS_MPI_Had_Decay_PU_out.root pdfFile=genJetPileup_ME_PS_MPI_Had_Decay_PU_out.pdf \
  sampleLabel="sample ME_PS_MPI_Had_Decay_GenJet|+ 55 MinBias collisions in time|library events 67-121 (bx 0)|bbbar pTHat > 100 GeV, 13.6 TeV|jet pT > 3 GeV, drawn > 30 GeV" \
  > genJetPileup_ME_PS_MPI_Had_Decay_PU_out.log 2>&1
pdfunite ../GEN/genJetAnalyzer_ME_PS_MPI_Had_Decay_GenJet_out.pdf genJetPileup_ME_PS_MPI_Had_Decay_PU_out.pdf \
  genJetAnalyzer_ME_PS_MPI_Had_Decay_GenJet_PU_out.pdf
```

To look at the merged particles afterwards, FWLite reads the EDM file like any other
(`DataFormats.FWLite.Events`, label `genParticlesPU`), and `collisionId()` tells the collision of every
particle; `printConstituents=True` on the `cmsRun` line prints, for every jet, the index of every
constituent in that collection.

---

## 9. Questions to test yourself

1. The fragment says 14 TeV and `crossSection = 71.39e9`. What energy and cross section did the job
   actually use, and where is each written?
   *13.6 TeV, from the `comEnergy` override in `--customise_commands`, visible as `Beams:eCM = 13600` in the
   log's settings table; 78.42 mb, fixed by the CP5 tune's `SigmaTotal` settings and reported by the
   `GenXsecAnalyzer` ("total cross section = 7.842e+10 pb"). The fragment's `crossSection` is metadata and is
   used by nothing.*
2. Why do the 13.6 TeV and the 14 TeV samples have the same cross sections to the second decimal?
   *Because `SigmaTotal:mode = 0` makes Pythia take sigma_tot and sigma_el from the tune instead of
   computing them, and the diffractive fractions from a parametrisation that barely moves between the two
   energies. The non-diffractive cross section is the remainder.*
3. Event 13 is non-diffractive with 78 stable particles; event 247 is non-diffractive with 723. What
   distinguishes them?
   *The number of parton-parton interactions: 1 against 39. Both are the same process; event 247 is a
   central collision with many MPIs, event 13 a peripheral one with only the hardest scattering.*
4. Event 12 has 4 stable particles. What are they, and will any of them be seen?
   *A proton with the full beam energy (xi = 3 x 10^-8) at eta = -10.8, and a Lambda, a K0L and a pi+ at
   eta = 9 forming a 2.6 GeV diffractive system. The three hadrons fly down the beam pipe. The proton is
   carried by `LHCTransport` to the Roman pots, but at that xi it stays inside the beam envelope; the
   SIM step gives it its hits there.*
5. Why does the GEN step, which knows nothing about the detector, take `--conditions` and `--era`?
   *For the vertex smearing: the beam-spot parameters are a payload of the global tag, and the era switches
   on the Run-3 PPS transport. The geometry option is only carried along for the SIM step.*
6. If `PUSEED` had been left at the `cmsDriver.py` default, what would be wrong with the sample?
   *Nothing visible in the sample itself. But its vertices would be, event by event, those of the signal
   sample generated with the same defaults, so that pileup collision k would sit exactly on signal event k
   when mixed.*
7. Half of the stable particles are photons. Where do they come from, and what does this mean for the
   detector?
   *From pi0 -> gamma gamma, decayed by Pythia because c tau(pi0) is 25 nm, far below the 10 mm limit. A
   minimum-bias event deposits most of its ECAL energy through these photons, spread over hundreds of
   crystals at a few MeV each.*
8. On page 9 the b jet gained 35 GeV from pileup although it kept exactly its 24 signal particles. Estimate
   that number from section 8.1 without looking at the jet, and say what a reconstruction can do about it.
   *55 collisions put 66 GeV per unit area into the central region; a cone of R = 0.4 has an area of
   pi R^2 = 0.50, so about 33 GeV. The offset is the same for any jet at that eta and is subtracted on
   average (rho x area, the L1 pileup correction); only its fluctuation from jet to jet remains.*
9. Why does the overlay of section 8 use only the 55 in-time collisions and not the 198 of the three bunch
   crossings of the DIGI step, and why library events 67 to 121 rather than 1 to 55?
   *A generator record has no time: anti-kT on particles is an in-time picture, and the early and late
   collisions reach the detector only through the time response of its channels. The mixing reads the
   library in sequence, bunch crossing by bunch crossing, early ones first, so for this signal event the
   first 66 library events went into bunch crossing -1 and the next 55 into bunch crossing 0; taking those
   makes page 9 the same collisions, event by event, that the DIGI step mixed in time.*

---

## Glossary

| term | meaning |
|---|---|
| minimum bias | the inelastic collisions selected with the least possible bias; in Pythia8 `SoftQCD:inelastic` = non-diffractive + single, double and central diffractive |
| non-diffractive, diffractive | collisions in which both protons break up (ND), or in which one (SD), both (DD) or neither (CD, with a central system) survive after a colourless exchange; the diffractive ones leave a rapidity gap |
| Pomeron | the colourless object exchanged in a diffractive collision, in Pythia's Schuler-Sjostrand model |
| M_X, xi | the mass of the dissociated system of a diffractive collision and the momentum fraction lost by the surviving proton, xi = M_X^2 / s |
| total, elastic, inelastic cross section | sigma_tot = 100.3 mb, sigma_el = 21.9 mb, sigma_inel = 78.4 mb with the CP5 tune, fixed by the tune and not by the energy; the rate of pileup collisions is L x sigma_inel |
| multiple-parton interactions (MPI) | several parton pairs scattering in one collision; their number, set by the impact parameter, is what makes the multiplicity of non-diffractive events fluctuate |
| pT0 | the scale below which the parton-parton cross section is regularised; 1.44 GeV at 13.6 TeV with CP5 |
| tune (CP5) | the set of Pythia8 parameters (PDF, alpha_s, MPI profile and pT0, colour reconnection) fitted to CMS data; the same for the signal and the pileup |
| colour reconnection | the rearrangement of the colour strings between the partons of all the interactions before hadronisation |
| Lund string | Pythia's hadronisation model: a colour string stretched between partons breaks into hadrons |
| status code | the integer that tells which stage of the generation a particle belongs to (1 stable, 2 decayed, 21-23 hardest interaction, 31-33 MPI, 41-53 showers, 61-63 remnants, 71-74 strings) |
| stable particle | HepMC status 1: what is left after Pythia8 decayed everything with c tau < 10 mm |
| generator fragment | the `_cfi.py` file that configures the generator (process, tune, energy); the first argument of `cmsDriver.py` in a GEN step |
| `Pythia8ConcurrentGeneratorFilter` | the CMSSW module that runs Pythia8, one instance per stream, and can reject events |
| `GenEventInfoProduct` | the per-event summary of the generator: process id, weight, scale of the hardest interaction |
| `HepMCProduct` | the full generator record (particles, vertices, status codes, mother/daughter links) in the HepMC format |
| `generator:unsmeared`, `generatorSmeared` | the record at the origin, and the record moved to the smeared vertex; only the second is written and read |
| luminous region, beam spot | the overlap volume of the two colliding bunches, where the vertices are; in 2024 sigma_z = 3.6 cm, sigma_t = 120 ps, about 10 um transversely |
| `VtxSmeared` | the GEN-step module that moves every generated event to a vertex drawn from the beam model; `BetafuncEvtVtxGenerator` with database parameters for `--beamspot DBrealistic` |
| `LHCTransport`, PPS | the GEN-step module that carries the surviving protons to the Roman pots of the Precision Proton Spectrometer, 200 m from CMS |
| `RandomNumberGeneratorService`, `initialSeed` | the service that owns one random engine per module; the seed fixes the whole sequence, and `cmsDriver.py` writes the same defaults into every configuration |
| `GenXsecAnalyzer` | the end-of-job module that reports the cross section and the filter efficiency of the sample |
| GEN | the data tier of a file that holds generator products only |
| `MixingModule`, `CrossingFrame` | the DIGI-step module that overlays pileup events on the signal, and the container it fills per product type; section 8 runs it on the HepMC record alone |
| `GenParticleProducer`, `useCrossingFrame` | the module that converts the HepMC record into `genParticles`; with `useCrossingFrame` it merges all events of the CrossingFrame and sets `collisionId` |
| pileup pT density | transverse momentum of the pileup per unit of eta-phi area (66 GeV for 55 collisions in this event); times pi R^2 it is the pileup offset of a jet of radius R |
| pileup jet | a jet made (almost) only of pileup particles, where the soft particles of many collisions happen to be dense enough to pass the jet threshold |
