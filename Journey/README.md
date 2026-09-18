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
