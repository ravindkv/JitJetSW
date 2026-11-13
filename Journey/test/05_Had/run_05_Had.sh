#!/usr/bin/env bash
# Chapter 05, Had: hadronisation. The parton-level event of chapter 04 is handed to the Lund string model
# (HadronLevel:Hadronize on): the colour strings between the partons fragment into the primary hadrons. Hadron decays
# stay off (HadronLevel:Decay off), so the final state is made of the primary hadrons, including the b hadrons.
# Input (Rule E): the configuration of chapter 04 with the hadronisation switch turned on; the parton level of this
# event is identical to chapter 04 (same seeds, same interleaved evolution).
# Output in test/05_Had/out/: step1_GEN_05_Had.root/.log/.driver.log, genPartAnalyzer_05_Had.pdf/.root/.log (pages: hard
# process, parton shower, MPI, hadronisation; with the cones Delta R < CONE_R = 0.4 around the b and the bbar and a zoom
# on the fuller cone, as in chapters 02 and 04) + PNG pages, ledger_05_Had.txt (with the cones of R = 0.4).
set -euo pipefail
CHAPTER=05_Had
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
jitjet_prev 04_MPI step1_GEN_04_MPI.root > /dev/null
#         ISR FSR MPI Had Decay pages
CONE_R=${CONE_R:-0.4} gen_stage on  on  on  on  off  4 "hadronisation (Lund strings)"
