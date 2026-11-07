#!/usr/bin/env bash
# runAll.sh: the whole journey of the ONE bbbar event, chapter by chapter (Rule E: every chapter reads the output of the
# previous one). Each chapter is test/<chapter>/run_<chapter>.sh, writing to test/<chapter>/out/; the log of this run is
# test/runAll.log. Run from an initialized CMSSW_15_0_5 environment (cd CMSSW_15_0_5/src && cmsenv).
#   ./runAll.sh                      all eleven chapters
#   CHAPTERS="09_G4 10_SIM" ./runAll.sh      a subset (their inputs must exist)
#   FROM=08_PU ./runAll.sh           from a chapter on
# Rough cost on one core: chapters 01-08 a few minutes, 09 about 5 minutes, 10 about one hour (library of 250 collisions
# through Geant4, SimHitAnalyzer pages of three samples, the mixing), 11 about one hour (the PU = 60 DIGI step).
set -euo pipefail
test_dir=$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")
all="01_ME 02_PS 03_MnM 04_MPI 05_Had 06_Decay 07_GenJet 08_PU 09_G4 10_SIM 11_DIGI"
chapters=${CHAPTERS:-$all}
if [[ -n ${FROM:-} ]]; then
  chapters=${all#*$FROM}; chapters="$FROM$chapters"
fi
log=$test_dir/runAll.log
echo "runAll.sh $(date '+%Y-%m-%d %H:%M:%S'): chapters $chapters" | tee -a "$log"
for ch in $chapters; do
  script=$test_dir/$ch/run_$ch.sh
  [[ -x $script ]] || { echo "Missing $script" | tee -a "$log" >&2; exit 1; }
  start=$(date +%s)
  "$script" 2>&1 | tee -a "$log"
  status=${PIPESTATUS[0]}
  echo "== chapter $ch finished with status $status after $(( ($(date +%s) - start) / 60 )) min ($(date '+%H:%M:%S'))" | tee -a "$log"
  (( status == 0 )) || exit "$status"
done
echo "runAll.sh done $(date '+%Y-%m-%d %H:%M:%S')" | tee -a "$log"
