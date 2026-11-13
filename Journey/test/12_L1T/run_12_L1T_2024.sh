#!/usr/bin/env bash
# run_12_L1T_2024.sh: the year comparison of chapter 12 (Rule I3 of the Constitution: a chapter moved to 2025 is compared
# with its 2024 output, and the evidence is kept). The chapter runs with the 2025 settings (run_12_L1T.sh); this script
# makes the same event with the 2024 settings (auto:phase1_2024_realistic, Run3_2024), inside the repository, so that
# every 2024 number of chapter_12_L1T.tex and its companion can be checked:
#   (1) the mixed DIGI of chapter 11 remade with the 2024 settings: the same SimHits of chapter 10, the same collisions
#       (MixingModule playback)                                    -> out2024/step3_DIGI_mixed_11_DIGI.root
#   (2) the L1 step of chapter 12 with the 2024 settings on it     -> out2024/step4_L1_12_L1T.root
#   (3) the three analyzers, the settings, the truth and the ledger, as in run_12_L1T.sh
#                                                                  -> out2024/l1t_12_L1T.log, ledger_12_L1T.txt, ...
#   (4) the CICADA anomaly score with the model of the other year: the L1 step once more on each input, only
#       simCaloStage2Layer1Summary kept                            -> out2024/cicada_input2024_v2p2p0.root (2024 input,
#       2025 model), out2024/cicada_input2025_v2p1p2.root (2025 input, 2024 model)
#   (5) the comparison (12_L1T/yearCompare_12_L1T.py: FWLite on the two L1 files and the CICADA files, the two ledgers,
#       conddb dumps of the ECAL laser and linearisation payloads of the two global tags)
#                                                                  -> out2024/compare_12_L1T.txt
# Input: test/10_SIM/out/ (the input of chapter 11) and test/12_L1T/out/ (the 2025 run: run_12_L1T.sh first).
# Configurations: test/12_L1T/step3_DIGI_mixed_2024_12_L1T_cfg.py, step4_L1_2024_12_L1T_cfg.py, step4_CICADA_*_12_L1T_cfg.py
# Variables: REUSE=1 (keep existing ROOT files), ANALYZE=0 (only the cmsRun jobs), PNG=0 (no PNG pages)
# Cost: the PU = 60 DIGI step about 15 minutes, the three L1 jobs a minute each, conddb a few minutes.
set -euo pipefail
CHAPTER=12_L1T
YEAR=2024
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
OUT=$CHDIR/out2024
mkdir -p "$OUT"
echo "   (the year comparison: $CONDITIONS, $ERA; output in $OUT)"
REUSE=${REUSE:-1}
bx_min=${BUNCHES%,*}; bx_max=${BUNCHES#*,}
library=$(jitjet_prev 10_SIM step2_SIM_pileup_10_SIM.root)
mixed=$(jitjet_prev 10_SIM step3_MIX_10_SIM.root)
input2025=$(jitjet_prev 11_DIGI step3_DIGI_mixed_11_DIGI.root)
l1t2025=$(jitjet_prev 12_L1T step4_L1_12_L1T.root)

# (1) the DIGI step of chapter 11 (run_11_DIGI.sh, case mixed) with the 2024 settings
digi=step3_DIGI_mixed_11_DIGI.root
if (( REUSE )) && [[ -s $OUT/$digi ]]; then
  echo "(1) reusing $digi"
else
  echo "(1) DIGI:pdigi_valid on event $EVENT of $(basename "$mixed"), PU = $PU, bx $bx_min..$bx_max (playback), $YEAR settings"
  customise="process.source.eventsToProcess = cms.untracked.VEventRange('$EVENT')"
  customise+='\n'"process.mix.input.sequential = cms.untracked.bool(True)"'\n'"process.mix.playback = cms.untracked.bool(True)"
  run_driver "$CHDIR/step3_DIGI_mixed_${YEAR}_${CHAPTER}_cfg.py" "$OUT/step3_DIGI_mixed_11_DIGI.log" step3 --filein "file:$mixed" \
    --pileup "AVE_60_BX_25ns,{'N': $PU, 'B': ($bx_min, $bx_max)}" --pileup_input "file:$library" \
    --conditions "$CONDITIONS" --era "$ERA" --geometry "$GEOMETRY" -n -1 -s DIGI:pdigi_valid --datatier GEN-SIM-DIGI \
    --eventcontent FEVTDEBUG --fileout "file:$digi" --customise_commands "$customise"
  (( RUN )) && { [[ -s $OUT/$digi ]] || { echo "No output produced: $OUT/$digi" >&2; exit 1; }; }
fi

# (2) the L1 step of chapter 12 (the same two added lines as run_12_L1T.sh) with the 2024 settings
l1common="process.load('SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff')"
l1common+='\n'"process.source.eventsToProcess = cms.untracked.VEventRange('$EVENT')"
out=step4_L1_${CHAPTER}.root
if (( REUSE )) && [[ -s $OUT/$out ]]; then
  echo "(2) reusing $out"
else
  echo "(2) L1 on event $EVENT of out2024/$digi, $YEAR settings"
  run_driver "$CHDIR/step4_L1_${YEAR}_${CHAPTER}_cfg.py" "$OUT/step4_L1_${CHAPTER}.log" step4 --filein "file:$OUT/$digi" \
    --conditions "$CONDITIONS" --era "$ERA" --geometry "$GEOMETRY" -n -1 -s L1 --datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG \
    --fileout "file:$out" --customise_commands "$l1common"'\n'"process.FEVTDEBUGoutput.outputCommands.append('keep *_simCaloStage2Layer1Summary_*_*')"
  (( RUN )) && { [[ -s $OUT/$out ]] || { echo "No output produced: $OUT/$out" >&2; exit 1; }; }
fi

# (4) the CICADA score with the model of the other year (the model is CICADAModelVersion of simCaloStage2Layer1Summary,
# L1Trigger/L1TCaloLayer1/python/simCaloStage2Layer1Summary_cfi.py: v2p1p2, and v2p2p0 with the modifier stage2L1Trigger_2025)
cicada() {  # cicada <input file> <input year> <model> <conditions> <era>
  local in=$1 year=$2 model=$3 cond=$4 era=$5
  local f=cicada_input${year}_${model}.root
  if (( REUSE )) && [[ -s $OUT/$f ]]; then echo "(4) reusing $f"; return 0; fi
  echo "(4) L1 on the $year input with the CICADA model $model ($cond, $era)"
  local c=$l1common
  c+='\n'"process.simCaloStage2Layer1Summary.CICADAModelVersion = cms.string('CICADAModel_$model')"
  c+='\n'"process.FEVTDEBUGoutput.outputCommands = cms.untracked.vstring('drop *', 'keep *_simCaloStage2Layer1Summary_*_*')"
  run_driver "$CHDIR/step4_CICADA_input${year}_${model}_${CHAPTER}_cfg.py" "$OUT/${f%.root}.log" step4 --filein "file:$in" \
    --conditions "$cond" --era "$era" --geometry "$GEOMETRY" -n -1 -s L1 --datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG \
    --fileout "file:$f" --customise_commands "$c"
}
cicada "$OUT/$digi" 2024 v2p2p0 "$CONDITIONS" "$ERA"
cicada "$input2025" 2025 v2p1p2 auto:phase1_2025_realistic Run3_2025
(( RUN && ANALYZE )) || exit 0

# (3) the analyzers, the settings, the truth and the ledger of the 2024 run (as run_12_L1T.sh)
a=l1t_${CHAPTER}
find "${OUT:?}" -maxdepth 1 -name "${a:?}_*.png" -delete
run_cmsrun "$COMMON/l1tAnalyzer_cfg.py" "$OUT/$a.log" inputFiles="file:$OUT/$out" maxEvents=1 eventsToProcess="$EVENT" \
  globalTag="$CONDITIONS" era="$ERA" geometry="$GEOMETRY" pdfPrefix="$a" savePNG="$PNG" \
  sampleLabel="chapter 12: the Level-1 trigger of $YEAR (year comparison)|bbbar, pTHat > $PTHATMIN GeV, 13.6 TeV|with PU = $PU per bx, bx $bx_min..$bx_max"
python3 "$COMMON/l1tSettings.py" "$CHDIR/step4_L1_${YEAR}_${CHAPTER}_cfg.py" "$CHDIR/step3_DIGI_mixed_${YEAR}_${CHAPTER}_cfg.py" \
  > "$OUT/settings_${CHAPTER}.txt" 2> "$OUT/settings_${CHAPTER}.log" || { tail -20 "$OUT/settings_${CHAPTER}.log" >&2; exit 1; }
python3 "$COMMON/l1tTruth.py" "$OUT" "$CHAPTER" > "$OUT/truth_${CHAPTER}.txt" 2> "$OUT/truth_${CHAPTER}.log" || { tail -20 "$OUT/truth_${CHAPTER}.log" >&2; exit 1; }
python3 "$COMMON/l1tLedger.py" "$OUT" "$CHAPTER" > "$OUT/ledger_${CHAPTER}.txt" 2> "$OUT/ledger_${CHAPTER}.log" || { tail -20 "$OUT/ledger_${CHAPTER}.log" >&2; exit 1; }

# (5) the comparison of the two years
python3 "$CHDIR/yearCompare_${CHAPTER}.py" "$TEST/$CHAPTER/out" "$OUT" > "$OUT/compare_${CHAPTER}.txt" 2> "$OUT/compare_${CHAPTER}.log" ||
  { tail -20 "$OUT/compare_${CHAPTER}.log" >&2; exit 1; }
echo "  The 2024 run: out2024/ledger_${CHAPTER}.txt; the comparison: out2024/compare_${CHAPTER}.txt"
