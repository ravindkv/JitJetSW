# At HCAL: the particles in the hadron calorimeter

Pages: `simHits_HCAL_rz_*`, `simHits_HCAL_xy_*`, `simHits_HCAL_etaphi_*`.

## The detector

The Phase2 hadron calorimeter is the HCAL barrel (HB) and outer (HO) scintillator calorimeters, the
hadronic section of the HGCal (CE-H) in the endcaps instead of the Run 3 HE, and the forward calorimeter
(HF).

| part | pages label | structure | extent (drawn cell positions) |
|------|-------------|-----------|-------------------------------|
| HCAL barrel | HB | brass absorber with plastic scintillator tiles, towers of 0.087 x 0.087 in eta-phi, 4 depth segments | r = 181-245 cm (tower positions), \|eta\| < 1.4 |
| HCAL outer | HO | scintillator layer outside the solenoid, catches the tails of the barrel showers | r = 390-413 cm, \|z\| < 670 cm |
| HGCal hadronic section, silicon | HGC HSi | 21 sampling layers, silicon in the inner (high radiation) region | \|z\| = 368-513 cm, r = 35-189 cm |
| HGCal hadronic section, scintillator | HGC HSc | layers 8-21, scintillator tiles in the outer region | \|z\| = 412-513 cm, r = 105-257 cm |
| forward calorimeter | HF | steel absorber with quartz fibres, Cherenkov light | \|z\| = 1115-1280 cm, r = 15-123 cm, 3 < \|eta\| < 5 |

## What a PCaloHit is here

All of these are sampling calorimeters: the PCaloHit energy is the energy deposited in the active material
(scintillator or silicon), a few per cent of the shower energy, before the calibration of the RECO step
turns it back into GeV of the incoming particle. The HF is different again: Geant4 does not track the
showers in the HF but takes them from a shower library, and the stored "energy" of an HF hit is the
number of Cherenkov photoelectrons, not GeV; the pages say "shower-library units" for the HF. The HCAL hit
ids are written in the simulation "test numbering" and are relabelled to the usual HcalDetId
(`HcalHitRelabeller`) before the cell is looked up in the geometry.

## What the HCAL does to the particles

- **Charged and neutral hadrons** that survived the ECAL shower here: a nuclear interaction every
  interaction length (about 17 cm of brass), secondaries of a few hundred MeV, neutral pions that turn into
  electromagnetic sub-showers. The HB plus the ECAL in front add up to about 10 interaction lengths, so
  the hadrons are stopped (what leaks is caught by the HO). The visible energy fluctuates strongly from
  shower to shower, which is why hadron calorimeters have a worse resolution than electromagnetic ones.
- **Photons and electrons** hardly reach the HCAL: the 100 photon cells here are the tails of the showers
  that started in the ECAL (the followed 10.6 GeV photon leaves 0.01 GeV in 9 cells).
- **Muons** cross as minimum-ionising particles (none in this event).

## This event

| | |
|---|---|
| cells | 2099 (HB 171, 0.38 GeV visible; HF 388, 1410 shower-library units; HGC HSi 626, 0.11 GeV; HGC HSc 914, 0.36 GeV) |
| by kind of the primary | charged hadron 1149 cells, neutral hadron 840, photon 100, no stored ancestor 10 |
| primaries with cells | 206 of 518 |
| followed | Sigma- 19.9 GeV (eta -3.8): 408 units in 10 HF cells; K0L 23.7 GeV (eta -1.5): 0.14 GeV in 378 HGCal H cells; photon 10.6 GeV: 0.01 GeV in 9 cells |

Things to see on the pages:

- **r-z**: the HB towers at r = 1.8-2.4 m with a few black cells (the central hadrons), the HO at 4 m,
  the HGCal hadronic block at |z| = 3.7-5.1 m with the b-jet (purple = the K0L, 378 cells: a neutral
  hadron that left nothing in the tracker and little in the EE now makes its shower), and the HF at
  |z| = 11 m with the forward Sigma-.
- **x-y**: only the HB and HO are barrel elements; 171 cells. The ring of the HB towers at r ~ 1.8 m.
- **eta-phi**: the hadronic showers are wider than the electromagnetic ones (compare the K0L block
  with the photon block of the ECAL page); beyond |eta| = 3 the HF cells of the forward particles.

The HGCAL chapter shows the hadronic section layer by layer; the ECAL chapter has the same jet one step
earlier.
