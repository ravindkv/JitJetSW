#!/usr/bin/env bash
# Chapter 01, ME: the hard matrix element q qbar -> b bbar of the ONE event of the book, and nothing else.
# Pythia8 generates the event with the initial- and final-state showers, the multiparton interactions, the
# hadronisation and the hadron decays switched off, so that the file holds the hard process (status 21 incoming,
# status 23 outgoing partons) and the beam remnants only.
# Input (Rule E): the generator fragment Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py of this package
# (HardQCD:qqbar2bbbar, tune CP5, pTHatMin raised to 100 GeV, 13.6 TeV) with the default random seeds of cmsDriver.py.
# Every later generator chapter keeps the fragment and the seeds and switches one more Pythia8 step on, so the hard
# process of this chapter is the hard process of the whole book (the same q qbar -> b bbar kinematics).
# Output in test/01_ME/out/:
#   step1_GEN_01_ME.root, .log, .driver.log       the GEN step (cmsDriver config: test/01_ME/step1_GEN_01_ME_cfg.py)
#   genPartAnalyzer_01_ME.pdf, .root, .log        the eta-phi page of the hard process (GenPartAnalyzer), + PNG pages
#   ledger_01_ME.txt                              the energy ledger of the event (common/genLedger.py)
#   xsec_GEN_01_ME.log, .driver.log               the cross section from XSEC_EVENTS=1000 events, same settings and seeds,
#                                                 no ROOT file (config: test/01_ME/xsec_GEN_01_ME_cfg.py; common gen_xsec);
#                                                 after the one event of the book Pythia8 can only quote sigma +- sigma
# Variables: see common/jitjet_common.sh (EVENTS, RUN=0, ANALYZE=0, PTHATMIN, ...); XSEC_EVENTS (default 1000).
set -euo pipefail
CHAPTER=01_ME
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
#         ISR FSR MPI Had Decay pages
gen_stage off off off off off  1 "the matrix element q qbar -> b bbar"
gen_xsec  off off off off off
