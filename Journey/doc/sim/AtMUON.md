# At MUON: the particles in the muon system

Pages: `simHits_MUON_rz_*`, `simHits_MUON_xy_*`, `simHits_MUON_etaphi_*`.

## The detector

The muon chambers sit outside the solenoid, interleaved with the iron of the magnet return yoke. The pages
draw every chamber as its outline (the four corners of the chamber surface), from the reco geometry:

| part | pages label | chambers | stations | extent |
|------|-------------|----------|----------|--------|
| drift tubes (barrel) | DT | 250 | MB1-MB4 | r = 436-788 cm, \|z\| < 659 cm |
| cathode strip chambers (endcap) | CSC | 612 | ME1-ME4 | r = 105-700 cm, \|z\| = 587-1038 cm |
| resistive plate chambers (barrel and endcap) | RPC | 1128 | RB1-RB4, RE1-RE4 | r = 175-770 cm, \|z\| < 1066 cm |
| GEM chambers (endcap, Phase2) | GEM | 432 | GE0, GE1/1, GE2/1 | r = 67-330 cm, \|z\| = 529-799 cm |

The GEM station 0 (GE0, the Phase2 replacement of the ME0 proposal) extends the muon coverage to
|eta| = 2.8, close to the beam pipe behind the HGCal.

## What a PSimHit is here

As in the tracker: a charged particle crossing the gas volume of a chamber layer makes one PSimHit with its
entry and exit point, energy loss, time of flight and momentum (the muon sensitive detector,
`MuonSensitiveDetector`). The time of flight is tens of nanoseconds here (a particle at the speed of light
needs 25 ns for 7.5 m), which is what the trigger uses to assign the hit to a bunch crossing.

## What the muon system does to the particles

Only muons are supposed to get here. Between the interaction point and the first muon station lie the
calorimeters and the first iron layer, 10 or more interaction lengths: hadrons shower and stop, electrons and
photons stop in the ECAL. A muon loses only about 2 GeV by ionisation through all of it and arrives as a
minimum-ionising particle, leaving one hit per chamber layer along a track that is bent again by the field
in the return yoke. The few non-muon hits are "punch-through": the last charged fragments of a hadronic
shower that leak out of the calorimeter into the first station, and the decay muons of pions and kaons.

## This event

| | |
|---|---|
| PSimHits | 30 (CSC station 1: 13; GEM station 0: 15, station 1: 2) |
| by kind of the primary | charged hadron 9, no stored ancestor 21 |
| primaries with hits | 1 of 518 |

There is **no muon among the primaries of this event**: the b and bbar hadrons decayed hadronically (a b
hadron gives a muon in about 10 % of its decays, plus a few per cent from the c hadron of the cascade). The
30 hits all descend from one primary, a pi+ of the b-jet with pT = 1.3 GeV at eta = -1.55, which is the
other way a muon gets here: the pion decayed in flight inside the tracker, at r = 73 cm and z = -166 cm,
into a mu+ of pT = 1.15 GeV (SimTrack list of SimAnalyzer, process "decay"), and the muon crossed the HGCal
and reached the GE0 and ME1 chambers at |z| = 5.3-6.3 m, where it left the 9 hits of stored tracks; the 21
hits "without stored ancestor" are most likely the delta rays it knocked out in the chamber gas. On the pages the hits
are black, because the colour is the kind of the *primary* (the charged hadron), not of the muon itself.
No hadronic shower leaked through the calorimeter (no punch-through) in this event: that is the normal
picture of a hadronic event in the muon system and the reason the muon chambers identify muons so cleanly.

To see a prompt muon track from a b decay, draw an event that contains one: generate a few events of the sample
(`EVENTS=20 STAGES=Transport_B_Decay_EM_Had ./runCMSDrivers_mcPhase2_BBbar_SIM.sh`), find the ones with a
muon among the primaries in the SimAnalyzer page 1 logs (`#mu^{#pm}` kind) or by running this chapter with
`PRINTHITS=1`, and draw it with `EVENT=1:1:<n> ./runSimHitAnalyzer_mcPhase2_BBbar.sh`. A muon shows up as a
cyan line of hits through all four stations in the r-z and x-y views.
