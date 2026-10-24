# JitJetSW/Journey

Per-event text dumps of every object at every step of the GEN -> SIM -> DIGI -> L1 -> DIGI2RAW
-> HLT -> RECO -> PAT -> NANO chain, so that the journey of every particle can be followed
in the cmsRun logs (and cross-checked against the ROOT files).

Enable it in `runCMSDrivers_mc2024_BBbar.sh` with the `DUMP` variable:

    DUMP=1 EVENTS=5 ./runCMSDrivers_mc2024_BBbar.sh          # every object of every step
    DUMP=2 ./runCMSDrivers_mc2024_BBbar.sh                   # + Pythia8/HepMC listings, all L1 bits
    DUMP=3 DUMP_G4_THRESHOLD=1.0 ./runCMSDrivers_mc2024_BBbar.sh  # + Geant4 track/step printout (Ekin >= 1 GeV), list of every product
    DUMP=1 DUMP_MAX=50 DUMP_SUB=20 ./runCMSDrivers_mc2024_BBbar.sh  # cap elements / sub-elements per collection

The customisation is `JitJetSW.Journey.jitJet_cff.customiseJitJet(process, level, maxElements, maxSub, g4Threshold)`;
it can also be added by hand to any of the `stepN_*_cfg.py` files. The step is detected from the
process name.

Links between steps that are printed so the same particle can be followed:

| step | key printed                                                                        |
|------|------------------------------------------------------------------------------------|
| GEN  | genParticles index `[i]` and HepMC `barcode`; GenJet constituents as genParticle keys; PV before/after smearing |
| SIM  | SimTrack `trackId`, `genBarcode -> genIdx` (+ pt ratio), vertex; every PSimHit/PCaloHit with its `trackId -> simTrackIdx` |
| DIGI | Pixel/Strip digis per det unit; `DigiSimLink` channel -> `simTrackId`; TrackingParticles with `genIdx` and `simTrackIds`; ECAL/HCAL digi samples; trigger primitives; muon digis |
| L1   | every L1 object per bx with hw and physical values, nearest gen jet / gen particle; uGT algo bits with names |
| HLT  | L1 objects seen by HLT, rechits/clusters/towers, HLT tracks/PF/jets/MET with gen match, all path results and all trigger-summary objects and filters |
| RECO | rechits/clusters/superclusters/towers; tracks with TrackingParticle -> genIdx association; PF candidates with track key; jets with constituent PF keys and gen-jet match; MET; leptons |
| PAT  | packed candidates with the AOD PF candidate they came from (packing precision); pat::Jets with all JEC levels, flavours, gen jet, discriminators, user floats and constituent keys; MET with all shifts; pruned/packed gen mapped to AOD `genParticles` index; trigger objects |
| NANO | every FlatTable (all columns of all rows) in the cmsRun log, and `scripts/dumpNanoTree.py` prints the written tree |

Notes
- All dump modules print with `std::cout`, flushed once per event, so each event block in the log is
  complete before the next `Begin processing` line.
- Indices printed as `[i]` are positions in the collection named in the section header; the gen
  index `genIdx` always refers to the `genParticles` collection of the GEN step (also in the
  RECO and PAT steps, where that collection is still in the input file), so a particle can be
  looked up with the same number in every log.
- `EventContentAnalyzer` with `verbose=True` (reflection dump of product contents) crashes in
  CMSSW_15_0_5 and is therefore not used; level 3 only lists the products.
- Each dump module is an ordinary EDAnalyzer with `cms.VInputTag` parameters, so collections
  can be added or removed in `python/jitJet_cff.py` (or on a `stepN_*_cfg.py` file) without
  recompiling.
- Rough log sizes with the default caps (all elements): ~10k lines/event for GEN, ~6k for SIM,
  ~50k for DIGI (mostly strip digis), ~12k for L1 (calo towers), ~15k for HLT and RECO, ~1k for
  PAT, ~0.4k for NANO. Use `DUMP_SUB` to cap hits/digis/constituents and `DUMP_MAX` for the
  number of objects per collection.

## GenPartAnalyzer: eta-phi maps per generation stage

`plugins/GenPartAnalyzer.cc` draws the genParticles of a GEN file as markers in the (eta, phi)
plane, marker size growing with pT and the pT value written next to the b, bbar and every
particle above `labelPtMin` (5 GeV). One plot is made per generation stage, so the event can be
followed from the matrix element to the final state:

| stage | particles drawn (Pythia8 status codes stored in genParticles)                                   |
|-------|--------------------------------------------------------------------------------------------------|
| 1     | hard process: outgoing b, bbar (status 23)                                                       |
| 2     | + parton shower: ISR (41-49) and FSR (51-59) partons, and the b / bbar copies after showering    |
| 3     | + MPI (31-39) partons, primordial-kT copies and beam remnants (61-69)                            |
| 4     | + hadronisation: the primary hadrons (daughters of the status 71-79 string partons)              |
| 5     | + hadron decays: the status 1 final state                                                        |

A particle is drawn at stage N if it exists at the end of stage N (produced in a stage <= N,
outgoing ancestors all from stages <= N, no daughter produced in a stage <= N). Incoming partons
and the beams (pT = 0, along z) are never drawn. The marker colour is the origin of the particle
(hard process, ISR, FSR, MPI, beam remnant, primary hadron, decay product); copies (status 44,
52, 62, ...) and FSR radiators inherit the origin of what they copy.

The hard-process b and bbar are followed through the stages in red / blue: the same-flavour quark
copies (stages 1-3), the primary b hadron nearest in Delta R to the last quark copy (stage 4) and
the stable decay products of that hadron (stage 5). Each plot shows pT, eta, phi of the b and bbar
systems, the net pT |pT(b) + pT(bbar)| (also drawn as a magenta star at the direction of the
vector sum), the scalar sum and Delta phi. The same numbers go to stdout, and with
`printParticles=True` every drawn particle of every stage is listed with its origin.

    cd test
    cmsRun genPartAnalyzer_cfg.py                                   # reads output_step1_GEN.root
    cmsRun genPartAnalyzer_cfg.py inputFiles=file:x.root maxEvents=3 printParticles=True

Output: `genPartAnalyzer_stage<N>_<name>_run<R>_event<E>.pdf` per stage and event, and
`genPartAnalyzer.root` (TFileService) with, per stage, `etaPhiPt` (TH2, eta vs phi weighted by
pT) and the `ptB`, `ptBbar`, `ptNetBBbar` distributions over events. Parameters: `genParticles`,
`outputPrefix`, `etaMax` (6, particles beyond are drawn on the border with an x marker),
`labelPtMin`, `savePDF`, `printParticles`.

## GenJet stage and GenJetAnalyzer: jets over the particles they were clustered from

The GEN step already runs the anti-kT clustering of the status 1 genParticles (`pgen` contains
`genJetMET`, i.e. `genParticlesForJets` + `FastjetJetProducer` for ak4GenJets, ak8GenJets and
the NoNu variants). `runCMSDrivers_mc2024_BBbar_GEN.sh` nevertheless has a sixth stage,
`ME_PS_MPI_Had_Decay_GenJet`, that runs the clustering again as a separate step 2 (process
`GENJET`, `python/genJet_cff.py`, scheduled by cmsDriver as
`-s USER:JitJetSW/Journey/genJet_cff.genJetSequence` on `step1_GEN_ME_PS_MPI_Had_Decay_out.root`,
which is generated first if missing) so that it is an explicit, configurable stage of the
journey after the full event: the jet pT threshold is set with `JETPTMIN` (default 3 GeV), R or
the algorithm can be changed with `--customise_commands`, and the stage output keeps only the
jets of that process (the GEN-process copies are dropped). It writes
`step2_GENJET_ME_PS_MPI_Had_Decay_GenJet_cfg.py`, `_out.root`, `_out.log` and the
`genJetAnalyzer_ME_PS_MPI_Had_Decay_GenJet_out.pdf`, `.root`, `.log`. Like the PDFs of the earlier
stages, that PDF starts every event with the five generator-stage pages of GenPartAnalyzer and
then adds one page per jet collection: `genJetAnalyzer_cfg.py` runs both analyzers in one job
(`maxStage`, default 5; 0 = jet pages only), GenPartAnalyzer opens the shared multi-page PDF and
GenJetAnalyzer closes it (`openPdf` / `closePdf` parameters of both modules), and an always-true
`HLTBool` filter between them fixes the order, since the framework orders the modules of a Path
only across filters. With `DUMP>0` the `GENJET` process prints every jet with its constituents
(GenJourney). GenJetAnalyzer can also be run by
hand on any step 1 file, where the status 1 particles of the parton-level stages are partons.

`plugins/GenJetAnalyzer.cc` draws, per jet collection and event, every jet as a circle of radius
R around its axis (star marker, label `[index] pT`), its constituents as filled markers in the
jet colour and the status 1 particles that ended up in no jet as grey open markers. R, the
algorithm, the input collection and the pT threshold are read from the producer configuration
stored in the file (provenance), and the process that produced the jets is printed in the title.
The hard-process b and bbar are followed to their last same-flavour copy and matched to the
nearest jet within R: b jet red, bbar jet blue, quark directions as open stars. The plot and
stdout give pT, eta, phi of both jets, the response pT(jet)/pT(quark), the net
|pT(b jet) + pT(bbar jet)|, Delta phi and the dijet mass; the right panel lists the leading jets.

    cd test
    cmsRun genJetAnalyzer_cfg.py                                        # reads output_step1_GEN.root
    cmsRun genJetAnalyzer_cfg.py inputFiles=file:x.root jets=ak4GenJets,ak8GenJets printConstituents=True

Output: `genJetAnalyzer_<collection>_run<R>_event<E>.pdf` per collection and event (or one
multi-page PDF with `pdfFile=`), and `genJetAnalyzer.root` with, per collection, `nJets`, `jetPt`,
`jetEta`, `nConstituents`, `etaPhiPt`, `ptBjet`, `ptBbarJet`, `ptNetBBbarJets` and `responseB`.
Parameters: `genParticles`, `genJets` (VInputTag), `outputPrefix`, `etaMax`, `labelPtMin`,
`savePDF`, `pdfFile`, `sampleLabel`, `printConstituents`, `openPdf`, `closePdf`.

## SIM stages and SimAnalyzer: the Geant4 simulation switched on stage by stage

`runCMSDrivers_mc2024_BBbar_SIM.sh` is the SIM-only companion of the GEN script: it runs step 2
(process `SIM`, generated by cmsDriver.py with the conditions, era, geometry and beamspot of the main
script) on the step 1 output of the full generator stage (`GEN/step1_GEN_ME_PS_MPI_Had_Decay_out.root`,
generated with the GEN script if missing) five times, with the Geant4 physics restricted through
`--customise_commands` so that every ingredient of the detector simulation is added separately:

| stage                      | Geant4 configuration                                                                         |
|----------------------------|----------------------------------------------------------------------------------------------|
| `Transport`                | transportation only: no magnetic field, no decays, no EM and no hadronic physics (physics list `JourneyTransportPhysics`, this package); the generator particles fly in straight lines |
| `Transport_B`              | + magnetic field (`UseMagneticField`): charged particles bend                                 |
| `Transport_B_Decay`        | + decays in flight (`FTFP_BERT_EMM` with `EMPhysics` and `HadPhysics` off, i.e. G4DecayPhysics only): pi, K, K0S, Lambda, mu ... decay |
| `Transport_B_Decay_EM`     | + electromagnetic physics (`EMPhysics`): ionisation and delta rays, multiple scattering, bremsstrahlung, pair production, Compton and photoelectric effect; EM showers, energy deposits |
| `Transport_B_Decay_EM_Had` | + hadronic physics (`HadPhysics`): nuclear interactions and hadronic showers; the standard SIM step |

Without electromagnetic physics nothing deposits energy: the tracker then records every sensor crossing
as a PSimHit with zero energy loss (`TrackerSD.ZeroEnergyLoss`), the calorimeters record nothing, and
the multiple-scattering coupling of the transportation and the gamma general process are switched off.
The crossing of the tracker / calorimeter boundary is stored on every SimTrack by `CaloTrkProcessing`
in all stages. The two stages without decays need the physics list `plugins/JourneyTransportPhysics.cc`
(all particles, transportation only), because `FTFP_BERT_EMM` always registers G4DecayPhysics and the
Geant4 UI commands of `G4Commands` are applied before the processes exist. A physics list built outside the
release cannot be mixed with the simulation "biglib" (`biglib/pluginSimulation.so` carries its own statically
linked, hidden copy of Geant4), so the script runs the SIM cmsRun with the biglib directories removed from
`LD_LIBRARY_PATH`: cmsRun then takes the ordinary shared-library build of the same simulation code.

    cd test
    ./runCMSDrivers_mc2024_BBbar_SIM.sh                       # all five stages, 1 event each
    EVENTS=10 STAGES="Transport Transport_B_Decay_EM_Had" ./runCMSDrivers_mc2024_BBbar_SIM.sh
    RUN=0 ...                                                 # only generate the configs
    ANALYZE=0 ...  ANALYZE_EVENTS=3 ...  GENSTAGES=0 ...      # analyzer options
    INPUT=file.root  BEAMSPOT=DBrealistic  OUTDIR=SIM  DUMP=1 ...

Output per stage in `SIM/`: `step2_SIM_<stage>_cfg.py` (+ `.driver.log`), `step2_SIM_<stage>_out.root`
(+ `.log`) and `simAnalyzer_<stage>_out.pdf`, `.root`, `.log`.

`runCMSDrivers_mcPhase2_BBbar_SIM.sh` is the same script for the Phase2 (HL-LHC) detector, with the
settings of the Run4D110 RelVal workflow of the release: geometry `ExtendedRun4D110`, era
`Phase2C17I13M9`, GT `auto:phase2_realistic_T33`, beamspot `DBrealisticHLLHC`, 14 TeV. Since the vertex
smearing happens in the GEN step, its default input is `GEN_Phase2/step1_GEN_ME_PS_MPI_Had_Decay_out.root`,
generated with the GEN script and the same Phase2 settings (the GEN script takes them as `CONDITIONS`,
`ERA`, `GEOMETRY`, `BEAMSPOT`, `COMENERGY`); `INPUT=GEN/step1_GEN_ME_PS_MPI_Had_Decay_out.root` runs the
Phase2 detector on the 2024 events instead. Output in `SIM_Phase2/`, same files and variables. In the
D110 detector the inner and outer tracker both write `TrackerHitsPixelBarrel/Endcap`, there is no ME0,
the endcap calorimeters are the HGCal (`HGCHitsEE`, `HGCHitsHEfront`, `HGCHitsHEback`, drawn as
`HGC EE` / `HGC HSi` / `HGC HSc` cells by SimAnalyzer, EE counted with ECAL and HSi / HSc with HCAL in the
system sums) and `EcalHitsEE`, `EcalHitsES` and the HE part of `HcalHits` do not exist; the MTD hits are
not drawn. `simAnalyzer_cfg.py` selects the Phase2 collections for a `Phase2*` era and loads the geometry
named by its `geometry` option (`DB:Extended` or a `Configuration.Geometry.Geometry<name>Reco_cff`).

    EVENTS=10 ./runCMSDrivers_mcPhase2_BBbar_SIM.sh            # Phase2 GEN input (if missing) + five SIM stages

`plugins/SimAnalyzer.cc` (`test/simAnalyzer_cfg.py`) draws, per event, five eta-phi pages that follow
the generator final state through the simulation, after the generator-record pages of GenPartAnalyzer
(`genStages`, default 5):

| page | content                                                                                               |
|------|-------------------------------------------------------------------------------------------------------|
| 1    | primaries: the status 1 genParticles handed to Geant4 (SimTracks with gen link, `genpartIndex` = HepMC barcode), coloured by particle kind; status 1 particles without SimTrack (|eta| > 5.5, p < 40 MeV, neutrinos) as grey crosses; the smeared primary vertex |
| 2    | the primaries where they cross the tracker / calorimeter boundary (`trackerSurfacePosition`, `trackerSurfaceMomentum`), with a line from the production direction showing the bend in the field; primaries that never get there (decayed, interacted, stopped, looping, along the beam line) as open markers |
| 3    | secondaries: the SimTracks created by Geant4 at their production vertex, coloured by the creator process (`SimVertex::processType`, the Geant4 process sub-type): decay, ionisation, bremsstrahlung, pair production / conversion, Compton, photoelectric, annihilation, hadron elastic / inelastic, capture, at rest |
| 4    | tracker and muon PSimHits at their global position (reco geometry from the EventSetup), coloured by subdetector |
| 5    | calorimeter PCaloHits summed per cell (EB, EE, ES, HB, HE, HO, HF, and for Phase2 the HGCal cells HGC EE, HGC HSi, HGC HSc; HCAL ids relabelled from the simulation test numbering), marker size growing with the cell energy |

The b and bbar are followed as in GenPartAnalyzer: the stable decay products of the primary b hadron
matched to each hard-process quark are the primaries of the b / bbar system (red / blue); secondaries
and hits belong to the system of their ancestor primary (SimVertex parent links, hit `trackId` /
`geantTrackId`). Every page prints the vector sums of the systems (pT at production, at the boundary,
ECAL and HCAL energies) and the net b + bbar, also to stdout. Only the tracks kept by the StackingAction
and the sensitive detectors are SimTracks (decay products and conversions of primaries, tracks with
tracker / muon hits above the persistency thresholds, tracks entering a calorimeter), so page 3 shows
the saved secondaries, not every Geant4 track; HCAL energies are the visible scintillator energies and
HF hits are kept as stored.

    cd test
    cmsRun simAnalyzer_cfg.py                                           # reads output_step2_SIM.root
    cmsRun simAnalyzer_cfg.py inputFiles=file:x.root maxEvents=2 printTracks=True genStages=0

Output: `simAnalyzer_pageN_<name>_run<R>_event<E>.pdf` per page and event (or one multi-page PDF with
`pdfFile=`), and `simAnalyzer.root` with, per page, `etaPhi` (TH2 weighted by pT, hits or energy), the
multiplicity, the kind / process / subdetector distribution and the b, bbar and net pT (pages 1-3) or
ECAL / HCAL energies (page 5). Parameters: `genParticles`, `simTracks`, `simVertices`, `trackerHits`,
`caloHits` (VInputTag), `outputPrefix`, `etaMax`, `labelPtMin`, `savePDF`, `pdfFile`, `sampleLabel`,
`maxPage`, `simStage` (stage highlighted in the right panel), `printTracks`, `hcalTestNumbering`,
`openPdf`, `closePdf`; the config adds `globalTag` and `era` for the geometry.

## SIM chapter: one event on the Phase2 detector, station by station (SimHitAnalyzer)

`runSimHitAnalyzer_mcPhase2_BBbar.sh` draws the SimHits of one event (default run 1, lumi 1, event 1 of
the full-physics stage `SIM_Phase2/step2_SIM_Transport_B_Decay_EM_Had_out.root`, generated with the Phase2
SIM script if missing) on the detector, one station at a time, with `plugins/SimHitAnalyzer.cc`
(`test/simHitAnalyzer_cfg.py`). The stations are TRK (tracker), ECAL (EB + HGCal EE), HCAL (HB, HO, HF +
HGCal HSi, HSc), HGCAL (the whole endcap calorimeter), MUON (DT, CSC, RPC, GEM) and ALL. Every station gets
three pages per event: the r-z view, the transverse view of its barrel part and the eta-phi view from the
primary vertex, each with the detector elements of the station from the reco geometry in grey (tracker
module centres, calorimeter cell centres subsampled to 12000 per subdetector, muon chamber outlines) and
the hits on top, coloured by the kind of the primary (charged hadron, neutral hadron, photon, electron,
muon) the hit descends from: every PSimHit / PCaloHit track id is followed up the SimVertex parent links to
the particle handed to Geant4, hits of tracks not kept as SimTracks are "no stored ancestor". The leading
primary of every kind with hits in the station is followed (hits connected in time order, cells circled,
label), and in the calorimeter stations the crosses mark where every primary crossed the tracker /
calorimeter boundary. Calorimeter cells are summed per cell and sized by energy; HF energies are the
shower-library units. ROOT keeps only one multi-page PDF open, so there is one PDF per station and event.

    cd test
    ./runSimHitAnalyzer_mcPhase2_BBbar.sh                      # event 1:1:1, all stations
    EVENT=1:1:3 STATIONS=TRK,HGCAL HIGHLIGHT=2 ./runSimHitAnalyzer_mcPhase2_BBbar.sh
    STAGE=Transport ./runSimHitAnalyzer_mcPhase2_BBbar.sh      # the same event of another SIM stage
    PNG=0 DOCS=0 PRINTHITS=1 ...

Output in `SIM_Phase2/<station>/`: `simHits_<station>_run<R>_event<E>.pdf` (three pages),
`simHits_<station>_<rz|xy|etaphi>_run<R>_event<E>.png`, and the chapter text `At<station>.md` copied from
`doc/sim/` (AtALL, AtTRK, AtECAL, AtHCAL, AtHGCAL, AtMUON: what the station is, what a hit is there, what the
station does to each kind of particle, and the numbers of event 1). The hit counts per subdetector, layer /
station and kind, and the extent and layer count of every subdetector from the geometry, are in
`SIM_Phase2/simHits.log`. The analyzer is detector-agnostic (`era`, `globalTag`, `geometry` options; a Run 3
era selects the Run 3 collections and stations with EE, ES and HE).
