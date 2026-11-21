#!/usr/bin/env bash
# run_11_DIGI_2024.sh: the year comparison of chapter 11 (Rule I3 of the Constitution: a chapter moved to 2025 is compared
# with its 2024 output, and the evidence is kept). The chapter runs with the 2025 settings (run_11_DIGI.sh); this script
# digitises the same three samples with the 2024 settings (auto:phase1_2024_realistic, Run3_2024), inside the repository,
# so that every 2024 number of chapter_11_DIGI.tex and of section 4.9 of its companion can be checked:
#   (11a) signal   step2_SIM_signal_10_SIM.root, event EVENT          -> out2024/step3_DIGI_signal_11_DIGI.root
#   (11b) pileup   step2_SIM_pileup_10_SIM.root, library event SKIP+1 -> out2024/step3_DIGI_pileup_11_DIGI.root
#   (11a+11b) mixed the 2024 mixed DIGI that test/12_L1T/run_12_L1T_2024.sh makes (the mixed configuration of this
#                  chapter with the 2024 settings and the playback of the chapter-10 draw, bx -12..3), linked as
#                  out2024/step3_DIGI_mixed_11_DIGI.root (run that script first)
# then the analyzers and ledgers of run_11_DIGI.sh on the three files (no PNG pages unless PNG=1): digiAnalyzer_*,
# trigPrim_*, ledger_11_DIGI.txt, trigprim_11_DIGI.txt, counts_11_DIGI.txt.
# Configurations: test/11_DIGI/step3_DIGI_{signal,pileup}_2024_11_DIGI_cfg.py
# Variables: REUSE=1 (keep existing DIGI files), ANALYZE=0 (only the cmsRun jobs), PNG=0
# Cost: the two DIGI jobs about one minute each, the analyzers a few minutes.
set -euo pipefail
CHAPTER=11_DIGI
YEAR=2024
PNG=${PNG:-0}
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
OUT=$CHDIR/out2024
mkdir -p "$OUT"
echo "   (the year comparison: $CONDITIONS, $ERA; output in $OUT)"
REUSE=${REUSE:-1}
signal=$(jitjet_prev 10_SIM step2_SIM_signal_10_SIM.root)
library=$(jitjet_prev 10_SIM step2_SIM_pileup_10_SIM.root)
mixed=$TEST/12_L1T/out2024/step3_DIGI_mixed_11_DIGI.root
[[ -s $mixed ]] || { echo "Missing $mixed: run test/12_L1T/run_12_L1T_2024.sh first" >&2; exit 1; }
pu_event=1:1:$((SKIP + 1))

run_digi() {  # run_digi <case> <input> <event>: the DIGI job of run_11_DIGI.sh, 2024 settings
  local case=$1 in=$2 ev=$3
  local out=step3_DIGI_${case}_${CHAPTER}.root
  if (( REUSE )) && [[ -s $OUT/$out ]]; then echo "($case) reusing $out"; return 0; fi
  echo "($case) DIGI:pdigi_valid on event $ev of $(basename "$in"), $YEAR settings"
  run_driver "$CHDIR/step3_DIGI_${case}_${YEAR}_${CHAPTER}_cfg.py" "$OUT/step3_DIGI_${case}_${CHAPTER}.log" step3 --filein "file:$in" \
    --conditions "$CONDITIONS" --era "$ERA" --geometry "$GEOMETRY" -n -1 -s DIGI:pdigi_valid --datatier GEN-SIM-DIGI \
    --eventcontent FEVTDEBUG --fileout "file:$out" --customise_commands "process.source.eventsToProcess = cms.untracked.VEventRange('$ev')"
  (( RUN )) && { [[ -s $OUT/$out ]] || { echo "No output produced: $OUT/$out" >&2; exit 1; }; }
  return 0
}
run_digi signal "$signal" "$EVENT"
run_digi pileup "$library" "$pu_event"
ln -sf "$mixed" "$OUT/step3_DIGI_mixed_${CHAPTER}.root"
(( RUN && ANALYZE )) || exit 0

for case in signal pileup mixed; do
  ev=$EVENT; [[ $case == pileup ]] && ev=$pu_event
  ismixed=0; [[ $case == mixed ]] && ismixed=1
  a=digiAnalyzer_${case}_${CHAPTER}
  run_cmsrun "$COMMON/digiAnalyzer_cfg.py" "$OUT/$a.log" inputFiles="file:$OUT/step3_DIGI_${case}_${CHAPTER}.root" outputFile="$a.root" \
    maxEvents=1 eventsToProcess="$ev" globalTag="$CONDITIONS" era="$ERA" geometry="$GEOMETRY" pdfFile="$a.pdf" pngPrefix="$a" savePNG="$PNG" \
    sampleLabel="chapter $CHAPTER, $YEAR settings (year comparison): $case"
  a=trigPrim_${case}_${CHAPTER}
  find "${OUT:?}" -maxdepth 1 -name "${a:?}_*.png" -delete
  run_cmsrun "$COMMON/trigPrimAnalyzer_cfg.py" "$OUT/$a.log" inputFiles="file:$OUT/step3_DIGI_${case}_${CHAPTER}.root" maxEvents=1 eventsToProcess="$ev" \
    globalTag="$CONDITIONS" era="$ERA" geometry="$GEOMETRY" pdfFile="$a.pdf" pngPrefix="$a" savePNG="$PNG" mixed="$ismixed" \
    sampleLabel="chapter $CHAPTER, $YEAR settings (year comparison): $case"
done
python3 "$COMMON/digiLedger.py" "$OUT" "$CHAPTER" > "$OUT/ledger_${CHAPTER}.txt" 2> "$OUT/ledger_${CHAPTER}.log" || { tail -20 "$OUT/ledger_${CHAPTER}.log" >&2; exit 1; }
python3 "$COMMON/trigPrimLedger.py" "$OUT" "$CHAPTER" > "$OUT/trigprim_${CHAPTER}.txt" 2> "$OUT/trigprim_${CHAPTER}.log" || { tail -20 "$OUT/trigprim_${CHAPTER}.log" >&2; exit 1; }
python3 "$CHDIR/digiFileCounts_${CHAPTER}.py" "$OUT" "$pu_event" > "$OUT/counts_${CHAPTER}.txt" 2> "$OUT/counts_${CHAPTER}.log" ||
  { tail -20 "$OUT/counts_${CHAPTER}.log" >&2; exit 1; }
echo "  The 2024 run: out2024/ledger_${CHAPTER}.txt, trigprim_${CHAPTER}.txt, counts_${CHAPTER}.txt"
