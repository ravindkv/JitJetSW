#!/usr/bin/env bash
# Chapter 02, PS: the parton shower. The same hard process as chapter 01 (same fragment, same seeds), now with the
# initial- and final-state showers of Pythia8 switched on (PartonLevel:ISR, PartonLevel:FSR); MPI, hadronisation and
# decays stay off, so the final state (status 1) is made of partons.
# Input (Rule E): the configuration of chapter 01 with the two shower switches turned on (test/01_ME/step1_GEN_01_ME_cfg.py
# differs from test/02_PS/step1_GEN_02_PS_cfg.py only in these switches and the file names). Note for the reader:
# Pythia8 interleaves ISR, FSR and MPI in one pT-ordered evolution, so the shower of this chapter (MPI off) is not
# identical, emission by emission, to the shower inside the full event of chapter 06; the hard process is.
# Output in test/02_PS/out/: step1_GEN_02_PS.root/.log/.driver.log, genPartAnalyzer_02_PS.pdf/.root/.log (pages: hard
# process, parton shower) + PNG pages, ledger_02_PS.txt.
set -euo pipefail
CHAPTER=02_PS
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
#         ISR FSR MPI Had Decay pages
gen_stage on  on  off off off  2 "the parton shower (ISR + FSR)"
