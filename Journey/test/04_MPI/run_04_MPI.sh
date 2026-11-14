#!/usr/bin/env bash
# Chapter 04, MPI: the multiparton interactions and the beam remnants. The same hard process and showers as chapter 02,
# now with PartonLevel:MPI on: the other parton pairs of the two protons scatter too (the underlying event), the primordial
# kT is given to the hard system and the beam remnants are built; hadronisation and decays stay off.
# Input (Rule E): the configuration of chapter 02 (chapter 03 changed nothing) with the MPI switch turned on. From this
# chapter on the event is, emission by emission, the event of the book: chapters 05 and 06 add hadronisation and decays
# after the parton level, which Pythia8 finishes here.
# Output in test/04_MPI/out/: step1_GEN_04_MPI.root/.log/.driver.log, genPartAnalyzer_04_MPI.pdf/.root/.log (one page,
# DRAW_STAGES=3: MPI and remnants, the end of the parton level; with the cones Delta R < CONE_R = 0.4 around the b and
# the bbar as in chapter 02, a zoom on the fuller cone and the part of each cone that belongs to the underlying event; the
# partons coloured by the scattering they belong to: the ISR / FSR of the hard process, the MPI with their ISR / FSR, the
# beam remnants; the log still prints stages 1-3),
# genPartAnalyzer_04_MPI_shower.pdf/.root/.log (one page, stage 2: the shower of the hard process with MPI on, drawn
# next to the shower of chapter 02 in the figure of the two showers), ledger_04_MPI.txt (with the cones of
# R = 0.4).
set -euo pipefail
CHAPTER=04_MPI
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
jitjet_prev 03_MnM step1_GEN_03_MnM.root > /dev/null   # the chain: chapter 03 must have been run
#                                             ISR FSR MPI Had Decay maxStage
DRAW_STAGES=3 CONE_R=${CONE_R:-0.4} gen_stage on  on  on  off off  3 "multiparton interactions and beam remnants"
# the stage-2 page in its own PDF: the parton shower with MPI on, for the side-by-side with the shower of chapter 02
if (( RUN && ANALYZE )); then
  CONE_R=${CONE_R:-0.4} gen_part_pages genPartAnalyzer_04_MPI_shower 2
fi
