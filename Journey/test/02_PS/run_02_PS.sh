#!/usr/bin/env bash
# Chapter 02, PS: the parton shower. The same hard process as chapter 01 (same fragment, same seeds), now with the
# initial- and final-state showers of Pythia8 switched on (PartonLevel:ISR, PartonLevel:FSR); MPI, hadronisation and
# decays stay off, so the final state (status 1) is made of partons.
# Input (Rule E): the configuration of chapter 01 with the two shower switches turned on (test/01_ME/step1_GEN_01_ME_cfg.py
# differs from test/02_PS/step1_GEN_02_PS_cfg.py only in these switches and the file names). Note for the reader:
# Pythia8 interleaves ISR, FSR and MPI in one pT-ordered evolution, so the shower of this chapter (MPI off) is not
# identical, emission by emission, to the shower inside the full event of chapter 06; the hard process is.
# Output in test/02_PS/out/: step1_GEN_02_PS.root/.log/.driver.log, genPartAnalyzer_02_PS.pdf/.root/.log (one page,
# DRAW_STAGES=2: the parton shower, with the cones Delta R < CONE_R = 0.4 around the b and the bbar and a zoom on the b
# cone; the hard process is the page of chapter 01, the log still prints both stages),
# ledger_02_PS.txt (with the cones of R = 0.4);
# xsec_GEN_02_PS.root/.log/.driver.log: XSEC_EVENTS = 1000 events with the same settings and seeds (common gen_xsec,
# XSEC_FILE=1; config test/02_PS/xsec_GEN_02_PS_cfg.py), the first one the event of the book: the cross section, and
# showerStats_02_PS.txt (common/genShowerStats.py): how many ISR / FSR branchings a shower of these settings makes, so
# that the shower of the book (no ISR emission, no bbar emission) can be compared with the typical one.
set -euo pipefail
CHAPTER=02_PS
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
#                                             ISR FSR MPI Had Decay maxStage
DRAW_STAGES=2 CONE_R=${CONE_R:-0.4} gen_stage on  on  off off off  2 "the parton shower (ISR + FSR)"
XSEC_FILE=1 gen_xsec                          on  on  off off off
if (( RUN && ANALYZE )); then
  python3 "$COMMON/genShowerStats.py" "$OUT/xsec_GEN_${CHAPTER}.root" --event "$EVENT" > "$OUT/showerStats_${CHAPTER}.txt" \
    2> "$OUT/showerStats_${CHAPTER}.log" || { tail -20 "$OUT/showerStats_${CHAPTER}.log" >&2; exit 1; }
  echo "  Shower statistics: showerStats_${CHAPTER}.txt"
  sed -n '2p;4p;6p;8p;12p' "$OUT/showerStats_${CHAPTER}.txt" | sed 's/^/    /'
fi
