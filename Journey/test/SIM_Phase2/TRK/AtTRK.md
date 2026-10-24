# At TRK: the particles in the Phase2 tracker

Pages: `simHits_TRK_rz_*`, `simHits_TRK_xy_*`, `simHits_TRK_etaphi_*` (one PDF with the three).

## The detector

The Phase2 tracker (geometry D110, T33 layout) is all silicon, inside the 3.8 T solenoid field. The reco
geometry of this release gives, with the layer numbers as they appear in the log and the DetId
subdetectors used in the pages:

| part | pages label | layers | extent of the module centres |
|------|-------------|--------|------------------------------|
| Inner Tracker barrel (TBPX, pixels) | PXB | 4 layers | r = 2.8-15 cm, \|z\| < 19 cm |
| Inner Tracker forward (TFPX + TEPX, pixels) | PXF | 12 disks per side | r = 5-23 cm, \|z\| = 24-266 cm |
| Outer Tracker barrel (TBPS + TB2S) | TOB | 6 layers (3 PS, 3 2S) | r = 22-110 cm, \|z\| < 119 cm |
| Outer Tracker endcap (TEDD) | TID | 5 double-disks per side | r = 26-102 cm, \|z\| = 129-268 cm |

In Phase2 the inner and the outer tracker both write their hits into the "pixel" collections
(`TrackerHitsPixelBarrel/Endcap`), and the DetId subdetector numbers of the outer tracker are the old TOB
(barrel) and TID (endcap) numbers; there are no TIB/TEC. The Outer Tracker modules carry two closely
spaced sensors each (for the Level-1 track trigger stubs), so a track crossing a TOB layer makes two
PSimHits. The IT covers |eta| up to 4, the OT up to 2.4.

## What a PSimHit is

A charged particle crossing a sensor ionises the silicon; the sensitive detector (`TkAccumulatingSensitiveDetector`)
records one PSimHit per sensor crossing with the entry and exit point in the local frame of the sensor,
the energy loss (of order 100 keV for a 300 um sensor; 1130 hits in this event come from tracks that are
not stored), the time of flight since the collision, the momentum and the particle type. The stage
`Transport` of the SIM script produces the same crossings with zero energy loss, which is why the hit
pattern is a pure geometry picture there.

Neutral particles leave no hit. They show up only through their charged descendants: a photon that
converts (green hits that start somewhere inside the tracker, the followed 5.0 GeV photon of this event
has 10 hits from its e+e- pair), a K0S that decays to pi+pi- (the followed 5.7 GeV K0S: 27 hits on two
tracks starting at the decay point), a neutron that interacts.

## This event

| | |
|---|---|
| PSimHits | 9940 (PXB 542, PXF 3779, TID 3007, TOB 2612) |
| by layer | PXB 1-4: 211, 133, 106, 92; PXF disks 1-12: 214 ... 422; TOB 1-6: 659, 611, 356, 377, 433, 176; TID 1-5: 591, 656, 526, 572, 662 |
| by kind of the primary | charged hadron 6074, neutral hadron 1111, photon 1577, e+- 48, no stored ancestor 1130 |
| primaries with hits | 307 of 518 |
| followed | Sigma- 19.9 GeV (eta -3.8): 12 hits on the forward pixel disks; K0S 5.7 GeV (eta -1.5): 27 hits of its decay pions; photon 5.0 GeV (eta -1.5): 10 hits of its conversion pair |

Things to see on the pages:

- **r-z**: the barrel layers as horizontal rows of hits, the disks as vertical columns; the forward
  Sigma- running along the pixel disks at small r. The decay of the K0S shows up as two hit sequences
  that start together inside the tracker.
- **x-y**: concentric rings of hits on the barrel layers; the curvature of the low-pT tracks in the
  3.8 T field (a 1 GeV track has R = 0.88 m). Only barrel hits are drawn here (3154 of the 9940).
- **eta-phi**: the hits now line up along the directions of the particles; the b-jet at eta = -1.5 is
  the dense cluster. Compare with the GenPartAnalyzer stage 5 page: every charged primary inside |eta| < 4
  is here, the neutral ones are missing unless they converted or decayed.

The number of hits per primary (10-16 for a central track crossing all barrel layers, both sensors of
the OT counted) is what the track reconstruction of the RECO step will work with.

## What the tracker does to the particles

Little, by design. The tracker material (sensors, cables, cooling, support) amounts to a few tenths of a
radiation length in the barrel and more than one radiation length in the transition region, so some of
the photons convert before the calorimeter (the 1577 green hits), electrons radiate, and hadrons
occasionally interact. The multiple scattering is small. Nearly all primaries leave the tracker with
their momentum intact and arrive at the tracker / calorimeter boundary, which is where the ECAL chapter
picks them up (the crosses on the ECAL pages).
