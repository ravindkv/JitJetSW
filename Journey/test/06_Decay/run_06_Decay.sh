#!/usr/bin/env bash
# Chapter 06, Decay: the hadron decays. The primary hadrons of chapter 05 decay (HadronLevel:Decay on) down to the
# particles Pythia8 leaves stable for the detector simulation (pythia8CommonSettings: ParticleDecays:limitTau0 with
# tau0Max = 10 mm, so pi0, eta, rho, B and D hadrons ... decay here, while K0S, Lambda, charged pions and kaons, neutrons
# are left to Geant4). This is the full generator event: the status 1 particles of this file are what every later
# chapter starts from.
# Input (Rule E): the configuration of chapter 05 with the decay switch turned on; parton level and hadronisation
# are identical to chapter 05.
# Output in test/06_Decay/out/: step1_GEN_06_Decay.root/.log/.driver.log, genPartAnalyzer_06_Decay.pdf/.root/.log
# (one page, DRAW_STAGES=5: the decays, with the cones Delta R < CONE_R = 0.4 around the b and the bbar, neutrinos not
# counted, and a zoom on the fuller cone, as in chapters 02, 04, 05; stages 1-4 are the pages of chapters 01, 04 and 05,
# the log still prints stages 1-5),
# ledger_06_Decay.txt (with the cones of R = 0.4). Chapter 07 reads step1_GEN_06_Decay.root: REGEN=0 redoes only the
# analysis on the existing file.
set -euo pipefail
CHAPTER=06_Decay
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
jitjet_prev 05_Had step1_GEN_05_Had.root > /dev/null
#                                             ISR FSR MPI Had Decay maxStage
DRAW_STAGES=5 CONE_R=${CONE_R:-0.4} gen_stage on  on  on  on  on   5 "hadron decays (the full generator event)"
