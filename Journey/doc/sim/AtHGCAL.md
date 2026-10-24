# At HGCAL: the particles in the high-granularity endcap calorimeter

Pages: `simHits_HGCAL_rz_*`, `simHits_HGCAL_etaphi_*` (no transverse page: the HGCal has no barrel part).

## The detector

The HGCal (CE, "calorimeter endcap") is the Phase2 replacement of the endcap ECAL, preshower and endcap
HCAL for 1.5 < |eta| < 3.0. It is a sampling calorimeter with an unprecedented number of channels, read out
cell by cell in three dimensions:

| section | pages label | layers | active material | extent (cell positions) | cells |
|---------|-------------|--------|-----------------|--------------------------|-------|
| CE-E, electromagnetic | HGC EE | 1-26 | silicon sensors (hexagonal cells of 0.5 and 1.2 cm^2) between lead / CuW absorbers | \|z\| = 322-361 cm, r = 32-162 cm | 3.7 million |
| CE-H, silicon part | HGC HSi | 1-21 | silicon sensors in the inner, high-radiation region, steel absorbers | \|z\| = 368-513 cm, r = 35-189 cm | 2.1 million |
| CE-H, scintillator part | HGC HSc | 8-21 | plastic scintillator tiles with SiPMs in the outer region | \|z\| = 412-513 cm, r = 105-257 cm | 0.24 million |

The pages draw every 300th cell or so (12000 per section) to show the layer structure; the layer number of
every cell is in the DetId (`HGCSiliconDetId::layer`, `HGCScintillatorDetId::layer`) and the log lists
the hits per layer.

## What the HGCal does to the particles, layer by layer

Because the hits carry their layer, the longitudinal shower profile can be read off directly. The hits of
this event per CE-E layer (all primaries together):

| layer | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 | 25 | 26 |
|-------|---|---|---|---|---|---|---|---|---|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|
| cells | 444 | 430 | 581 | 480 | 587 | 418 | 468 | 357 | 395 | 303 | 298 | 242 | 237 | 199 | 227 | 179 | 227 | 199 | 177 | 143 | 157 | 125 | 149 | 130 | 118 | 126 |

and in the CE-H: HSi layers 1-7 have 51-111 cells each and layers 8-20 fall to 1-17 (none in layer 21),
while the HSc layers 8-14 hold 88-115 cells each, layers 15-16 about 60 and layers 17-21 between 9 and 24. The electromagnetic showers of the photons and of the neutral pions from
hadron interactions peak in the first third of the CE-E (the shower maximum of a 10 GeV photon is around
5-6 X0, a few layers) and die out by layer 26; what continues into the CE-H is hadronic. At larger r the
CE-H is scintillator (HSc), which is why the hadronic hits at r > 1 m are HSc cells.

Energies are the energy in the active layers only: 1.35 GeV in the CE-E and 0.47 GeV in the CE-H for this
event, a few per cent of what the particles carried in. The RECO step will multiply them back with the
sampling fractions and absorber weights.

## This event

| | |
|---|---|
| cells | 8936 (HGC EE 7396, 1.35 GeV; HGC HSi 626, 0.11 GeV; HGC HSc 914, 0.36 GeV, all visible) |
| by kind of the primary | charged hadron 4048 cells (0.90 GeV), neutral hadron 2076 (0.38 GeV), photon 2791 (0.54 GeV), no stored ancestor 21 |
| primaries with cells | 214 of 518 |
| followed | K- 18.1 GeV: 0.03 GeV in 117 cells; K0L 23.7 GeV: 0.13 GeV in 404 cells; photon 10.6 GeV: 0.01 GeV in 61 cells |

Things to see on the pages:

- **r-z**: the 26 CE-E layers as thin vertical lines at |z| = 3.2-3.6 m, the CE-H layers behind them; the
  b-jet (eta = -1.5) enters at r = 1.3-1.6 m: the photon stops in the CE-E, the K- and the K0L shower on
  into the CE-H (the purple circles of the K0L reach the last HSc layers). The crosses at |z| = 3.0 m are
  the primaries arriving from the tracker.
- **eta-phi**: the same showers as seen by a jet algorithm, 1.5 < |eta| < 3.0 only; the fine granularity
  resolves the individual particles of the jet.

The ECAL and HCAL chapters contain the CE-E and CE-H together with their barrel counterparts.
