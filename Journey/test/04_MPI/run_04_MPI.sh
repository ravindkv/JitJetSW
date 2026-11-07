#!/usr/bin/env bash
# Chapter 04, MPI: the multiparton interactions and the beam remnants. The same hard process and showers as chapter 02,
# now with PartonLevel:MPI on: the other parton pairs of the two protons scatter too (the underlying event), the primordial
# kT is given to the hard system and the beam remnants are built; hadronisation and decays stay off.
# Input (Rule E): the configuration of chapter 02 (chapter 03 changed nothing) with the MPI switch turned on. From this
# chapter on the event is, emission by emission, the event of the book: chapters 05 and 06 add hadronisation and decays
# after the parton level, which Pythia8 finishes here.
# Output in test/04_MPI/out/: step1_GEN_04_MPI.root/.log/.driver.log, genPartAnalyzer_04_MPI.pdf/.root/.log (pages: hard
# process, parton shower, MPI and remnants) + PNG pages, ledger_04_MPI.txt.
set -euo pipefail
CHAPTER=04_MPI
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
jitjet_prev 03_MnM step1_GEN_03_MnM.root > /dev/null   # the chain: chapter 03 must have been run
#         ISR FSR MPI Had Decay pages
gen_stage on  on  on  off off  3 "multiparton interactions and beam remnants"
