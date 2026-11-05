# At the mixing: the signal, the pileup, and the two together

This chapter follows the same event as the SIM chapters (run 1 / lumi 1 / event 1 of the Phase2 bbbar sample)
one step further: into the DIGI step, where the minimum-bias collisions of the same and the neighbouring bunch
crossings are added to it. Everything is drawn with `SimHitAnalyzer` on three samples, with the same stations
and the same three views (`runPileup_mcPhase2_BBbar.sh`), so that the pages can be put side by side:

| case | directory | what is in it |
|------|-----------|---------------|
| (a) signal | `signal/` | the bbbar event through Geant4, exactly the SIM chapter (`step2_SIM_Transport_B_Decay_EM_Had_out.root`) |
| (b) pileup | `pileup/` | one minimum-bias event of the pileup library through the same Geant4 (`step2_SIM_MinBias_out.root`, event 1 of 50) |
| (a+b) signal + pileup | `mixed/` | the signal event as the MixingModule hands it to the digitisers: its own hits plus the hits of the minimum-bias events it put into the bunch crossings -1, 0 and +1 |

## Why pileup exists and how CMS simulates it

At the HL-LHC every bunch crossing (every 25 ns) contains on average about 200 inelastic proton-proton
collisions. Only one of them is the hard scattering we generated; the others are **pileup**: soft, inelastic
collisions (`SoftQCD:inelastic` in Pythia8), each with its own primary vertex spread over the luminous region
(sigma_z = 4.3 cm, sigma_t = 190 ps for the HL-LHC beams). The detector does not know which collision a hit
belongs to: it integrates everything that arrives inside its readout window. Slow detectors (the HGCal and
the HCAL time samples, the drift tubes, the strip readout) also see collisions of the bunch crossings before
and after the one that was triggered: **out-of-time pileup**, early (bx < 0: their hits arrive 25 ns x |bx|
before the signal) and late (bx > 0).

CMS simulates this with the **MixingModule** in the DIGI step, i.e. after Geant4 and before the detector
electronics:

1. The pileup events are simulated once, as a library: minimum-bias events through the GEN step (with the
   same vertex smearing as the signal) and the SIM step (the same Geant4). This is case (b). In production
   the library is huge and, since Run 2, **premixed**: the minimum-bias events are mixed and digitised once
   at PU = 200 and the signal is added to the digitised pileup event ("premixing"). Here the mixing is done
   from the SimHits, which is the classical way and the one where the hits of both can still be drawn.
2. For every signal event the MixingModule draws, for every bunch crossing of the window (`minBunch` ..
   `maxBunch`, -3 .. 3 in the Phase2 production, -1 .. 1 here), a Poisson-distributed number of library events
   with mean `averageNumber` (200 in production, 10 here), and reads them from the library file (in sequence
   here, `sequential = True`, so that no event is used twice).
3. Every pileup SimTrack, SimVertex, PSimHit and PCaloHit is stamped with an `EncodedEventId` (bunch crossing,
   event number within the crossing), and the times of the vertices and hits are shifted by 25 ns x bx. The
   Geant4 track ids are not changed: a hit is identified by (event id, track id).
4. The signal objects and the pileup objects of every collection are handed together to the digitisers as a
   `CrossingFrame<T>` (`mix:g4SimHits<Collection>`): one list of signal hits and one list of pileup hits, with
   the pileup list ordered by bunch crossing. The digitisers (pixel, strip, HGCal, ECAL, HCAL, muon ...) loop
   over the signal hits and the pileup hits in the same way, add the energies in the same cells, apply the
   time windows and produce the digis. The `CrossingFrame`s are transient: they are never written to a file,
   which is why `SimHitAnalyzer` draws them from inside the mixing job.
5. The truth is kept: the `TrackingParticle`s and `CaloParticle`s built in the same step carry the event id,
   so that a reconstructed track or cluster can later be traced to "the signal" or to "pileup event 7 of bx -1".

## How the pages are drawn

The pages of (a) and (b) are the SIM chapter pages: every hit coloured by the kind of the primary (the
particle handed to Geant4) it descends from, the leading primary of every kind followed. In (a+b) the signal
hits keep exactly these colours, and the pileup hits are drawn underneath in three new colours, by bunch
crossing: early (bx < 0, dark yellow), in-time (bx 0, light blue) and late (bx > 0, salmon). A calorimeter
cell is summed over everything that hit it and takes the colour of the largest energy share, so a cell that
turns from green to blue between (a) and (a+b) is a cell of a signal photon that pileup now dominates. The
header of every (a+b) page states how many pileup events were mixed per bunch crossing and how many of the
hits of the station are signal and pileup; `mixed/simHits.log` lists every mixed event with its primaries,
SimTracks and vertex position and time.

## The event at a glance

The library event drawn in (b) is a typical minimum-bias collision: 182 primaries handed to Geant4 (the bbbar
signal has 518), its vertex at z = 0.03 cm, 3463 tracker hits and 15811 calorimeter hits in 5228 cells, 210 GeV
in the HCAL stations of which most is a single forward hadron in the HF. Other library events are much smaller
or larger: in the mixing log they range from 1 primary (a diffractive event whose only tracked particle is a
forward proton) to 614 primaries.

For signal event 1 the MixingModule drew 7 library events for bx -1, 8 for bx 0 and 3 for bx +1 (Poisson with
mean 10; 18 events in total, 2691 primaries, 19090 SimTracks, vertices spread between z = -11 cm and +15 cm,
times -25 ns, 0 and +25 ns). What that adds to the signal event, station by station:

| station | (a) signal | (b) one pileup event | (a+b) signal + 18 pileup events | signal primaries with hits (a) -> (a+b) |
|---------|-----------|----------------------|--------------------------------|------------------------------------------|
| TRK | 9940 PSimHits | 3439 | 59685 (9940 signal + 49745 pileup: 19170 early, 27076 in-time, 3499 late) | 307 -> 307 |
| ECAL (EB + HGCal EE) | 50179 PCaloHits in 12476 cells | 15156 in 4713 cells | 275466 in 60604 cells; 11441 cells dominated by the signal, 49163 by pileup | 323 -> 321 |
| HCAL (HB, HO, HF + HGCal H) | 4403 in 2099 cells | 655 in 515 cells | 12126 in 6594 cells; 1874 signal-dominated, 4720 pileup-dominated | 206 -> 168 |
| HGCAL | 14817 in 8936 cells | 4652 in 3225 cells | 64275 in 42159 cells; 8860 signal-dominated, 33299 pileup-dominated | 214 -> 214 |
| MUON | 30 PSimHits | 24 | 150 (30 signal + 120 pileup) | 1 -> 1 |
| ALL | 9970 PSimHits + 14575 cells | 3463 + 5228 cells | 59835 PSimHits + 67198 cells; 13315 cells signal-dominated, 53883 pileup-dominated | 476 -> 462 |

What to read from the pages:

- **Tracker (TRK)**: the pileup hits outnumber the signal hits five to one, although the signal is a hard event
  and every pileup event is soft. Pileup adds hits everywhere, with no structure in phi; the signal b-jets are
  still visible as the dense black / purple / green bundles at eta = -1.5 (and the forward particles beyond
  eta = -3), and in the x-y view as the curved bundles of tracks. The hits of the three bunch crossings sit on top of each other: at the SimHit level
  only the time distinguishes them (the pixel and strip readout windows will later drop most of the early and
  late hits; the x-y view of the first layers shows how the in-time pileup tracks come from vertices spread
  along z and therefore do not point back to the signal vertex).
- **ECAL**: the EB barrel ring and the HGCal EE fill up with low-energy pileup cells. The energy scale of the page
  tells the story: the signal deposits 54 GeV in the ECAL stations, the 18 pileup events 263 GeV, spread over
  five times more cells. Only 11441 of the 60604 hit cells are dominated by the signal: the b-jet cores keep
  their colour (the K-, K0L and photon of the followed list are still found with the same cells), while
  isolated low-energy signal cells are drowned.
- **HCAL**: here the pileup dominates even in energy, because the minimum-bias events are forward: the HF
  collects most of the pileup energy (the HF numbers are shower-library units), and in the HGCal hadronic part
  four cells out of five are pileup-dominated. 38 of the 206 signal primaries with HCAL hits in (a) have no
  cell left where they dominate in (a+b).
- **HGCAL**: the endcap calorimeter is where the HL-LHC pileup hurts most: 33299 of the 42159 hit cells are
  pileup-dominated already at PU = 10 x 3 bunch crossings; the eta-phi page of (a+b) is almost uniformly blue
  and yellow in |eta| > 1.5, with the signal b-jet at eta = -1.5 standing out only because of its energy.
  This is why the HGCal measures the time of every cell (and the MTD the time of every track): the early and
  late pileup, 25 ns away, and the in-time pileup from vertices 1-15 cm away (tens to hundreds of ps)
  can be separated in time.
- **MUON**: pileup reaches the muon system only through punch-through and the GEM / CSC forward stations:
  120 pileup hits, no muon.

The numbers behind the pages are in `signal/simHits.log`, `pileup/simHits.log` and `mixed/simHits.log` (the
latter with the list of mixed events); the DIGI step of the production (`PU = 200`, bx -3..3) mixes 20 x 2.3 = 47
times more pileup than shown here.

## Reproducing and extending

    cd test
    ./runPileup_mcPhase2_BBbar.sh                                 # (a), (b), (a+b) with PU = 10 in bx -1..1, library of 50
    CASES=mixed PU=50 BUNCHES=0,0 PUEVENTS=100 ./runPileup_mcPhase2_BBbar.sh   # denser in-time pileup (library regenerated if too small: REUSE=0)
    CASES=mixed PU=10 BUNCHES=-3,3 ./runPileup_mcPhase2_BBbar.sh   # the production window (needs PUEVENTS >= ~100)
    PUEVENT=7 CASES=pileup ./runPileup_mcPhase2_BBbar.sh           # draw another library event

The DIGI file of the mixing job (`step3_DIGI_PU10_out.root`) holds the digis of the signal + pileup event and the
`PileupSummaryInfo` (the number of interactions per bunch crossing): it is the input of the next chapter.
