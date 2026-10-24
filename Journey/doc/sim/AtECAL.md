# At ECAL: the particles in the electromagnetic calorimeter

Pages: `simHits_ECAL_rz_*`, `simHits_ECAL_xy_*`, `simHits_ECAL_etaphi_*`.

## The detector

In Phase2 the electromagnetic calorimeter is the ECAL barrel (EB) plus the electromagnetic section of the
High Granularity Calorimeter (HGCal EE) in the endcaps; the Run 3 endcap ECAL (EE) and preshower (ES) no
longer exist.

| part | pages label | structure | extent (drawn cell positions) |
|------|-------------|-----------|-------------------------------|
| ECAL barrel | EB | 61200 PbWO4 crystals, 23 cm long (25.8 X0), homogeneous | front face at r = 129 cm, \|z\| < 270 cm, \|eta\| < 1.48 |
| HGCal electromagnetic section | HGC EE | 26 sampling layers of silicon sensors between lead / copper-tungsten absorbers, 3.7 million cells | \|z\| = 322-361 cm, r = 32-162 cm, 1.5 < \|eta\| < 3.0 |

The EB cells are drawn at their front face, so the EB appears as a single ring at r = 129-130 cm in the
r-z and x-y views; the HGCal EE appears as its 26 layers in z. The tracker is drawn in light grey for
orientation, and the crosses mark where every primary crossed the tracker / calorimeter boundary.

## What a PCaloHit is

The calorimeter sensitive detectors (`ECalSD`, `HGCalSD`) add up the energy deposited by every Geant4 step in
a cell, keyed by the cell id and by the track that entered the calorimeter, into PCaloHits (several per
cell when several incoming particles contribute: 50179 hits in 12476 cells in this event). In the EB the
deposited energy is the full energy left in the crystal, so the 53.1 GeV sum is what the showers deposited
(a 25.8 X0 crystal contains an electromagnetic shower almost completely). In the HGCal EE only the energy
in the thin silicon layers is recorded, a sampling fraction of about one per cent: the 1.35 GeV visible in
the HGCal EE corresponds to far more deposited energy. Both are "visible" energies before any calibration.

## What the ECAL does to the particles

- **Photons and electrons** start an electromagnetic shower: pair production and bremsstrahlung alternate
  every radiation length (X0 = 0.89 cm in PbWO4), the shower is contained in the ~ 25 X0 and spreads
  laterally over a Moliere radius (2.2 cm, about one crystal). In the pages a photon is a compact block of
  green cells; the whole energy of the particle ends here. The 4448 photon cells of this event hold
  22.1 GeV.
- **Charged hadrons** either pass as minimum-ionising particles (a few hundred MeV along a crystal) or
  start a hadronic shower, since the EB is about one interaction length deep: roughly two thirds of the
  charged hadrons interact in the ECAL. The 5478 charged-hadron cells hold 24.5 GeV.
- **Neutral hadrons** (K0L, neutrons) interact or pass; 1923 cells, 6.8 GeV.
- **Muons** pass as minimum-ionising particles (none in this event).

## This event

| | |
|---|---|
| cells | 12476 (EB 5080, 53.1 GeV; HGC EE 7396, 1.35 GeV visible) |
| by kind of the primary | charged hadron 5478 cells (24.5 GeV), neutral hadron 1923 (6.8 GeV), photon 4448 (22.1 GeV), e+- 13 (0.06 GeV), no stored ancestor 614 (1.1 GeV) |
| primaries with cells | 323 of 518 |
| followed | K- 18.1 GeV (eta -1.5): 0.41 GeV in 131 HGCal EE cells; K0L 23.7 GeV: 0.03 GeV in 52 cells; photon 10.6 GeV: 0.03 GeV in 84 cells |

The three followed particles are the leading members of the b-jet at eta = -1.5, which points into the
HGCal: that is why their visible energies are so small (silicon sampling), while the EB numbers are full
deposits. The HGCAL chapter shows the same particles layer by layer.

Things to see on the pages:

- **r-z**: the EB ring with the hits of the central particles, the HGCal EE block at |z| = 3.2-3.6 m with
  the b-jet; the boundary crosses at r = 1.22 m (barrel) and |z| = 3.0 m (endcap).
- **x-y**: the EB ring; the big green markers are the photon showers (each marker one crystal, its size the
  energy); charged hadrons appear as single black cells (MIPs) or small black clusters (interactions).
- **eta-phi**: the showers as energy blobs of a few cells; compare the green blobs with the photons of
  the GenPartAnalyzer final-state page.
