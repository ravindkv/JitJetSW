# JitJetSW/Journey: JitJet, Journey is the Jet

ONE bbbar event (Pythia8 `q qbar -> b bbar`, 13.6 TeV, tune CP5, pTHat > 100 GeV; run 1, lumi 1, event 1) and the
SAME event followed through every stage of the CMS simulation, one chapter per stage. The rules of the book are
`Constitution.txt`; the book is `doc/JitJetSW.tex` (`pdflatex JitJetSW.tex` twice in `doc/`), which inputs the thirteen
chapters `doc/chapter/chapter_<XX_YYY>.tex`; every chapter has a technical companion `doc/chapter/chapter_<XX_YYY>.md`
and a script `test/<XX_YYY>/run_<XX_YYY>.sh` that produces, in `test/<XX_YYY>/out/`, exactly the files the chapter
quotes. `doc/chapter/README.md` says how a chapter is written.

| chapter | stage | script produces (in `test/<chapter>/out/`) |
|---|---|---|
| `01_ME` | the matrix element (Pythia8 with showers, MPI, hadronisation, decays off) | `step1_GEN_01_ME.root`, GenPartAnalyzer page, ledger |
| `02_PS` | + parton showers (ISR, FSR) | `step1_GEN_02_PS.root`, 2 pages, ledger |
| `03_MnM` | matching and merging: empty for this LO 2 -> 2 event (a check and a link to the 02 output) | `mnm_03_MnM.log` |
| `04_MPI` | + multiparton interactions and beam remnants | `step1_GEN_04_MPI.root`, 3 pages, ledger |
| `05_Had` | + hadronisation (Lund strings) | `step1_GEN_05_Had.root`, 4 pages, ledger |
| `06_Decay` | + hadron decays: the full generator event | `step1_GEN_06_Decay.root`, 5 pages, ledger |
| `07_GenJet` | anti-kT generator jets as a separate step (process GENJET) | `step2_GENJET_07_GenJet.root`, 5 + 3 pages (GenJetAnalyzer), ledger |
| `08_PU` | pileup at generator level: 250 minimum-bias collisions (GEN), one drawn; the signal + its 68 in-time collisions re-clustered | `step1_GEN_MinBias_08_PU.root`, `genstats_08_PU.txt`, `genJetAnalyzer_08_PU.pdf`, ledger |
| `09_G4` | the hand-over to Geant4: transport-only SIM of the signal and of all library collisions (what is accepted, what is dropped) | `step2_G4_signal_09_G4.root`, `step2_G4_pileup_09_G4.root`, SimAnalyzer pages, `ledger_09_G4.txt` (g4Ledger.py) |
| `10_SIM` | full Geant4 physics switched on stage by stage (B, decays, EM, hadronic); the signal, the library and the mixing at SimHit level, station by station | `step2_SIM_{B,B_Decay,B_Decay_EM,signal,pileup}_10_SIM.root`, SimAnalyzer and SimHitAnalyzer pages (`signal/`, `pileup/`, `mixed/`), `step3_MIX_10_SIM.root` (the playback record of the mixing), ledger |
| `11_DIGI` | digitisation: the signal alone, one collision alone, the signal with PU = 60 in bx -1..1 (the last chapter with signal and pileup apart); the calorimeter trigger primitives made here for the Level-1 trigger | `step3_DIGI_{signal,pileup,mixed}_11_DIGI.root`, DigiAnalyzer pages, `thresholds_11_DIGI.txt`, ledger, TrigPrimAnalyzer pages, `trigprim_11_DIGI.txt` |
| `12_L1T` | checkpoint (Rule G): the Level-1 trigger of 2025 in six steps (calorimeter trigger primitives, Layer-1, Layer-2, muon track finders, uGMT, uGT), for the mixed event only (the signal with its pileup) | `step4_L1_12_L1T.root`, L1TCaloAnalyzer / L1TMuonAnalyzer / L1TGlobalAnalyzer pages, `settings_12_L1T.txt`, `truth_12_L1T.txt`, ledger |
| `13_HLT` | checkpoint (Rule G): DIGI2RAW and the High Level Trigger (the 2025 menu of the release, `HLT:@relval2025` = `2025v10`, its seeds checked against the L1 menu of chapter 12 by `python/hltL1Menu_cff.py`) in five steps (raw event and L1 seed, calorimeter jets, tracking and vertices, particle flow and PF jets, the decision of the paths), for the mixed event only | `step5_HLT_13_HLT.root`, HLTPathAnalyzer / HLTCaloAnalyzer / HLTTrackAnalyzer / HLTPFAnalyzer pages, `menu_13_HLT.txt`, ledger |

    cd CMSSW_15_0_5/src && cmsenv && cd JitJetSW/Journey/test
    ./runAll.sh                       # the whole journey in order (about three hours, chapters 10 and 11 dominate)
    ./01_ME/run_01_ME.sh              # one chapter (its input chapter must have been run)
    FROM=09_G4 ./runAll.sh            # from a chapter on
    RUN=0 ./10_SIM/run_10_SIM.sh      # only write the cmsDriver configurations

The output of every chapter is the input of the next (Rule E): 01-06 regenerate the same event with one more Pythia8
step on (same fragment, same seeds, so the hard process is identical; MPI, hadronisation and decays leave the earlier
steps untouched), 07 clusters the jets of the 06 file, 08 generates the library and overlays its in-time collisions on
the 07 file (the 68 collisions that the MixingModule of chapter 10 draws into bunch crossing 0: `SKIP=71`, `NPU=68` in
`jitjet_common.sh`), 09 runs the transport-only Geant4 (process `SIM`) on the 07 file and on the library, 10 runs the full
Geant4 as process `SIMFULL` on the 09 files (the transport-only products are dropped) and the MixingModule without
digitisers (`python/mixOnly_cff.py`, which also writes the playback record of the draw), 11 runs the DIGI step three
times on the 10 files, the mixed one replaying the draw of 10 (`process.mix.playback = True`). The common machinery is in
`test/common/`: `jitjet_common.sh` (sourced by every script: environment check, plugin build, `run_driver`,
`run_cmsrun`, `pdf_pages_png`, `gen_stage`), the analyzer configurations and the ledger scripts
(`genLedger.py`, `g4Ledger.py`, `simLedger.py`, `digiLedger.py`, `digiThresholds.py`, `minBiasGenStats.py`).
Earlier layouts (the stage directories `GEN/`, `SIM/`, `PU_2024/`, the Phase-2 runs, the old run scripts and the
first pileup chapters) are parked, untouched, in `test/tmp/` and `doc/tmp/`.

## The analyzers (plugins/)

All analyzers draw with ROOT into a multi-page PDF (one PDF per job; ROOT keeps one PDF open) and print their numbers
to stdout; the chapter scripts convert the pages to PNG (`pdftoppm -cropbox`) for the book and keep the logs.

- **GenPartAnalyzer** (`test/common/genPartAnalyzer_cfg.py`): the genParticles of a GEN file as markers in the (eta, phi)
  plane, one page per generation stage that exists in the record (1 hard process, 2 parton shower, 3 MPI and beam
  remnants, 4 hadronisation, 5 hadron decays; `maxStage`), the hard-process b and bbar followed through the stages
  (red / blue), with pT, eta, phi of the b and bbar systems, their net and scalar pT sums. Options: `maxStage`,
  `pdfFile`, `sampleLabel`, `eventsToProcess`, `etaMax`, `labelPtMin`, `printParticles`.
- **GenJetAnalyzer** (`test/common/genJetAnalyzer_cfg.py`): per jet collection a page with every jet as a circle of
  radius R, its constituents in the jet colour, the unclustered status 1 particles in grey, the b / bbar jets matched
  to the quarks, the response, the net and the dijet mass; R, algorithm and threshold are read from the provenance.
  With `signalCollisionId` (chapter 08, `test/common/genJetPileup_cfg.py` + `python/genJetPileup_cff.py`: the
  MixingModule on the HepMC record only, `GenParticleProducer` with `useCrossingFrame`) the pileup particles are
  triangles and every jet gets its pileup pT share.
- **SimAnalyzer** (`test/common/simAnalyzer_cfg.py`): five pages per event after the generator pages: the primaries
  handed to Geant4 (and the status 1 particles without SimTrack: |eta| > 5.5, p < 40 MeV, neutrinos), the primaries at
  the tracker / calorimeter boundary, the secondaries by creator process, the tracker and muon PSimHits, the
  calorimeter PCaloHits per cell; b and bbar systems followed. Options: `simStage`, `genStages`, `maxPage`,
  `eventsToProcess`, `globalTag`, `era`, `geometry`.
- **SimHitAnalyzer** (`test/common/simHitAnalyzer_cfg.py`, `python/simHitAnalyzer_cff.py`): one event station by
  station (TRK, ECAL, HCAL, MUON, ALL; HGCAL for Phase-2) in three views (r-z, x-y, eta-phi) with the detector elements
  in grey and the hits coloured by the kind of the primary they descend from, the leading primary of every kind
  followed. In CrossingFrame mode (`addSimHitAnalyzer`, `mixOnly_cff.mixOnly`) it runs inside the mixing job and draws
  the pileup hits underneath, coloured by bunch crossing (early, in-time, late); the log lists every mixed collision.
  Options: `outputDir`, `prefix`, `stations`, `eventsToProcess`, `highlight`, `highlightPtMin`, `printHits`, `savePNG`.
- **DigiAnalyzer** (`test/common/digiAnalyzer_cfg.py`): the digis of one event of a GEN-SIM-DIGI file on an eta-phi
  and an r-z page: pixel and strip digis at the position of the pixel / strip, attributed through the DigiSimLinks to
  the signal, in-time pileup, early or late pileup or to nothing (noise); EB / EE / ES, HB / HE (QIE11), HF (QIE10) and
  HO data frames sized by their amplitude above pedestal (ADC counts); DT, CSC, RPC and GEM digis with their own
  links; the signal ak4GenJets as circles. The log has the counts per subdetector and origin and the summed
  amplitudes. Options: `eventsToProcess`, `pdfFile`, `pngPrefix`, `sampleLabel`, `jetPtMin`, `ecalMinAmp`, `hcalMinAmp`,
  `hcalSignalTS` (-1: the sample of interest of the frame), `hcalNSumTS`, `maxDrawn` (thinning for the drawing only).
- **TrigPrimAnalyzer** (`test/common/trigPrimAnalyzer_cfg.py`): the calorimeter trigger primitives of one event of a
  GEN-SIM-DIGI file (`simEcalTriggerPrimitiveDigis`, `simHcalTriggerPrimitiveDigis`, made by the DIGI step for the
  Level-1 trigger) on the trigger-tower grid, the towers around the generator jets one by one, the transverse energy
  deposited per ECAL tower (PCaloHits) against the primitive, the sum per bunch crossing of the primitive digi, and the
  ECAL trigger-tower flags that drive the selective readout. `test/common/trigPrimLedger.py` puts the three samples of
  chapter 11 side by side (`trigprim_11_DIGI.txt`).
- **HLTPathAnalyzer, HLTCaloAnalyzer, HLTTrackAnalyzer, HLTPFAnalyzer** (`test/common/hltAnalyzer_cfg.py`, shared page in
  `plugins/HLTDisplay.h` on top of `L1TDisplay.h`): the High Level Trigger of one event of a RAW + HLT file (chapter 13).
  HLTPathAnalyzer reads the menu from the file (`HLTConfigProvider`) and prints, for every path, whether it accepted the
  event, its L1 seed and the module that stopped it, the datasets and streams, the objects of every filter
  (`hltTriggerSummaryAOD`) and the raw-data size per subsystem; pages: the unpacked L1 objects and the seed objects, the
  ladders of the jet paths, the objects of the accepted paths. HLTCaloAnalyzer: rec-hits, calorimeter towers on the
  trigger-tower grid, AK4 calo jets with their corrections recomputed level by level from the global tag
  (`FactorizedJetCorrector`, payload `AK4CaloHLT`), the towers around the two signal jets. HLTTrackAnalyzer: clusters,
  pixel and full tracks, pixel, kept and PF vertices compared with the simulated collisions (`addPileupInfo`,
  `genParticles:xyz0`). HLTPFAnalyzer: the PF candidates by kind, the AK4 PF jets with their composition and their
  corrections (payload `AK4PFHLT`, binned in eta and phi), the two signal jets constituent by constituent.
  `test/common/hltMenu.py` dumps the menu as configured, `hltLedger.py` writes the ledger.
- **JourneyTransportPhysics**: a Geant4 physics list with all particles and no process, so that only the
  transportation acts (chapters 09 and 10). A physics list built outside the release cannot be mixed with the
  simulation "biglib" (its own static Geant4), so those cmsRun jobs run with the biglib directories removed from
  `LD_LIBRARY_PATH` (`sim_ld_library_path` in `jitjet_common.sh`).
- **GenJourney, SimJourney, DigiJourney, L1Journey, TriggerJourney, LocalRecoJourney, RecoJourney, PatJourney,
  FlatTableJourney** (`python/jitJet_cff.py`, `customiseJitJet(process, level, maxElements, maxSub, g4Threshold)`):
  per-event text dumps of every object of every step GEN -> SIM -> DIGI -> L1 -> HLT -> RECO -> PAT -> NANO with the
  keys that link them (genParticles index and HepMC barcode, SimTrack trackId and gen link, DigiSimLink channel ->
  simTrackId, TrackingParticles, L1 / HLT objects with their gen match, RECO tracks / PF candidates / jets, PAT and
  NANO tables). Add it to any `step*_cfg.py` with `--customise_commands "from JitJetSW.Journey.jitJet_cff import
  customiseJitJet; process = customiseJitJet(process, level=1)"`; `scripts/dumpNanoTree.py` prints a NanoAOD tree.
  The step is detected from the process name. `EventContentAnalyzer` with `verbose=True` crashes in CMSSW_15_0_5 and
  is not used.

## Phase-2 (HL-LHC)

The CMS chapters 08-13 run with the 2025 settings of the release (`auto:phase1_2025_realistic`, `Run3_2025`, and in
13_HLT the 2025 menu `HLT:@relval2025`; Rule I of the Constitution); the generator chapters 01-07 keep 2024 (at
generator level the two are the same). The year of a chapter is `YEAR` in `test/common/jitjet_common.sh`. In this
release the 2025 global tag has the geometry, the field map and the simulated beam spot of 2024, so 08-10 are identical
with either year; the two years part at the digitisation (11-13 say in a "What the year changed" paragraph how).
Every chapter ends with the open problems of its stage for Phase-2, dug out of the release configuration. The earlier Phase-2 runs (D110 geometry, `Phase2C17I13M9`, 14 TeV, PU 10 in bx -1..1, with the
HGCAL station of SimHitAnalyzer) and their scripts are kept in `test/tmp/` (`runCMSDrivers_mcPhase2_BBbar_SIM.sh`,
`runPileup_mcPhase2_BBbar.sh`, `runSimHitAnalyzer_mcPhase2_BBbar.sh`, `GEN_Phase2/`, `SIM_Phase2/`, `PU_Phase2/`)
and `doc/tmp/` (`Gen4Phase2.md`, `pu/Phase2/`, `sim/At*.md`); the chapter scripts take the scenario from
`YEAR` (or `CONDITIONS`, `ERA`), `GEOMETRY`, `BEAMSPOT`, `COMENERGY` (change all together).
