# At SIM: one event through the whole Phase2 detector

This chapter follows the particles of one simulated event, run 1 / lumi 1 / event 1 of the Phase2 bbbar
sample, from the moment they are handed to Geant4 to the signals they leave in the detector. The pages
`simHits_ALL_*` show every hit of the event on the whole detector; the chapters AtTRK, AtECAL, AtHCAL,
AtHGCAL and AtMUON zoom into one station each. Everything is drawn from the SIM step output
(`step2_SIM_Transport_B_Decay_EM_Had_out.root`, the full Geant4 physics) with `SimHitAnalyzer`
(`runSimHitAnalyzer_mcPhase2_BBbar.sh`).

## What the SIM step does

The generator (Pythia8, 14 TeV, `q qbar -> b bbar` with pTHat > 100 GeV, HL-LHC vertex smearing) delivers
the stable final state: 590 status 1 particles in this event. The SIM step (cmsDriver `-s SIM`, Geant4
with the physics list FTFP_BERT_EMM, 3.8 T field) takes the ones inside its acceptance, 518 here, as
**primaries**: the neutrinos and the particles beyond |eta| = 5.5 or below 40 MeV are not tracked. Geant4 then
transports every primary through the detector material step by step. Charged particles ionise, scatter
and radiate; photons convert; hadrons interact with nuclei; unstable particles decay. Every new particle is
a **secondary** (4284 SimTracks are kept in this event, primaries and the secondaries worth storing), and
wherever a particle crosses a sensitive volume the detector records a hit:

| hit type | where | what is stored |
|----------|-------|----------------|
| `PSimHit` | tracker sensors, muon chambers | entry and exit point in the sensor, energy loss, time of flight, momentum, particle type, the Geant4 id of the track that made it |
| `PCaloHit` | calorimeter cells (EB crystals, HGCal cells, HCAL towers, HF) | energy deposited in the cell (visible energy for sampling calorimeters), time, the cell id, the id of the track that entered the calorimeter |

A calorimeter shower produces thousands of Geant4 tracks, but the sensitive detector books every deposit to
the track that entered the calorimeter, so a shower is one block of cells attributed to one incoming
particle. Hits in the tracker, on the other hand, are booked to the track that made them, which may be a
secondary that was not kept as a SimTrack (a delta ray, a soft conversion electron): those hits appear as
"no stored ancestor" in the pages.

## How the pages are drawn

Every page is one station of the journey: its detector elements from the reco geometry in grey (tracker
module centres, calorimeter cell centres, muon chamber outlines) and the hits on top, in three views:

- **r-z**: the longitudinal view, r = sqrt(x^2 + y^2) against z, where the layer structure is visible.
- **x-y**: the transverse view of the barrel part only (barrel elements, barrel hits). Charged tracks curve
  in the 3.8 T field here: pT [GeV] = 0.3 B [T] R [m].
- **eta-phi**: the hits as seen from the primary vertex, the view used by jets; calorimeter cells are drawn
  with a marker size growing with their energy (0.01, 1, 100 GeV in the scale).

The colour is the **kind of the primary a hit descends from**: for every hit the Geant4 track id is followed
up the SimVertex parent links to the primary. Charged hadrons (black), neutral hadrons (purple), photons
(green), electrons (orange), muons (cyan). A green hit in the tracker is therefore a photon conversion, a
purple one a decay product or a nuclear interaction of a neutral hadron.

For the leading primary of every kind (the highest-pT one with hits in the station) the hits are connected
in time order and labelled, and its calorimeter cells are circled, so that a single particle can be followed
through the layers. In the calorimeter chapters the crosses mark where every primary crossed the tracker /
calorimeter boundary (`SimTrack::trackerSurfacePosition`), i.e. where it arrived.

## The event at a glance

| station | hits | primaries that left hits (of 518) | note |
|---------|------|------------------------------------|------|
| TRK | 9940 PSimHits | 307 | IT 4321, OT 5619 |
| ECAL (EB + HGCal EE) | 50179 PCaloHits in 12476 cells | 323 | EB 53.1 GeV deposited; HGCal EE 1.35 GeV visible |
| HCAL (HB, HO, HF + HGCal HSi, HSc) | 4403 PCaloHits in 2099 cells | 206 | HB 0.38 GeV visible, HGCal H 0.47 GeV visible, HF 1410 shower-library units |
| HGCAL (whole endcap calorimeter) | 14817 PCaloHits in 8936 cells | 214 | 26 EE + 21 H layers hit |
| MUON | 30 PSimHits | 1 | no muon in this event; one hadron reaches ME1/GE0 |
| ALL | 9970 PSimHits + 14575 cells | 476 | 42 primaries left no hit at all |

The 42 primaries without any hit are neutral particles that neither converted nor interacted in the
sensitive volumes (very forward or very soft), and charged particles outside the tracker acceptance that
reached only insensitive material.

## Reading the ALL pages

- The r-z page shows the full detector at once: the tracker (r < 1.1 m, |z| < 2.7 m), the EB ring at
  r = 1.3 m, the HGCal at |z| = 3.2-5.1 m, HB at r = 1.8-2.4 m, HO at 4 m, the HF at |z| = 11 m, and the
  muon stations DT (r 4.4-7.9 m), CSC (|z| 5.9-10.4 m), RPC and GEM. The hits trace the b-jet at eta = -1.5
  (the K-, K0L and photon of the followed list) into the HGCal and the forward particles into the HF.
- The x-y page is the classic "event display" view of the barrel: concentric hit rings on the tracker
  layers and the EB ring.
- The eta-phi page is the one to compare with the generator-level pages of GenPartAnalyzer and the jets
  of GenJetAnalyzer: the same final state, now as energy in cells.

## Reproducing and extending

    cd test
    ./runSimHitAnalyzer_mcPhase2_BBbar.sh                      # event 1:1:1 of the full-physics stage
    EVENT=1:1:3 ./runSimHitAnalyzer_mcPhase2_BBbar.sh          # another event (the sample must contain it)
    STAGE=Transport_B_Decay_EM ./runSimHitAnalyzer_mcPhase2_BBbar.sh   # the same event without hadronic physics
    STATIONS=TRK,HGCAL HIGHLIGHT=2 ./runSimHitAnalyzer_mcPhase2_BBbar.sh

Running the earlier SIM stages (`STAGE=Transport`, `Transport_B`, ...) on the same event shows the detector
response being built up: straight tracks with zero energy loss, then curved tracks, then decays, then
electromagnetic showers, then hadronic showers.
