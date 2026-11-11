#!/usr/bin/env bash
# Chapter 12, L1T: the Level-1 trigger, the first checkpoint of the journey. The digis of chapter 11 go through the
# emulator of the Run-3 (stage-2) Level-1 trigger (cmsDriver -s L1 = the SimL1Emulator sequence of the era Run3_2024),
# which does not change the event: it decides, in six steps, whether the event is kept.
#   step 1  the calorimeter trigger primitives (made in the DIGI step of chapter 11: simEcalTriggerPrimitiveDigis,
#           simHcalTriggerPrimitiveDigis): one transverse energy per trigger tower and bunch crossing
#   step 2  calorimeter Layer-1 (simCaloStage2Layer1Digis): ECAL + HCAL of a tower calibrated and summed
#   step 3  calorimeter Layer-2 (simCaloStage2Digis): jets, e/gamma, taus, energy sums
#   step 4  the muon trigger primitives (simDtTriggerPrimitiveDigis, simCscTriggerPrimitiveDigis, simMuonGEMPadDigis)
#           and the muon track finders (simKBmtfDigis, simOmtfDigis, simEmtfDigis)
#   step 5  the global muon trigger (simGmtStage2Digis)
#   step 6  the global trigger (simGtStage2Digis): every algorithm of the menu, the FinalOR
# The three samples of chapters 10 and 11, with the same configuration:
#   (12a) signal    the signal event alone
#   (12b) pileup    one library collision alone (library event SKIP+1)
#   (12a+12b) mixed the signal with its PU = 60 bunch crossings, the production case
# Input (Rule E): test/11_DIGI/out/step3_DIGI_{signal,pileup,mixed}_11_DIGI.root
# Output in test/12_L1T/out/:
#   step4_L1_<case>_12_L1T.root, .log, .driver.log     the L1 step (configs in test/12_L1T/)
#   l1t_<case>_12_L1T.log                              the three analyzers (L1TCaloAnalyzer, L1TMuonAnalyzer, L1TGlobalAnalyzer)
#   l1t_<case>_12_L1T_{calo,muon,global}.pdf + PNG     their pages: the eta-phi display of the event at every step
#   settings_12_L1T.txt                                the parameters of every module of the L1 step (common/l1tSettings.py)
#   truth_12_L1T.txt                                   who made the L1 muons: the simulated particles behind them (common/l1tTruth.py)
#   ledger_12_L1T.txt                                  the ledger of the three samples (common/l1tLedger.py)
# Variables: CASES="signal pileup mixed" REUSE=1 (keep existing L1 files); PU, BUNCHES, SKIP of jitjet_common.sh label the pages
set -euo pipefail
CHAPTER=12_L1T
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
CASES=${CASES:-"signal pileup mixed"}
REUSE=${REUSE:-1}
bx_min=${BUNCHES%,*}; bx_max=${BUNCHES#*,}
pu_event=1:1:$((SKIP + 1))
# -s L1 alone is not a production configuration (the L1 emulator always runs behind the DIGI step): two lines are
# added. (1) Layer-1 needs the HCAL trigger-tower geometry and the decoder of the HCAL trigger-primitive scale, two
# event-setup producers that come with the HCAL trigger-primitive emulator of the DIGI step. (2) The anomaly score
# of CICADA (simCaloStage2Layer1Summary), which five algorithms of the 2024 menu cut on, is not in the event content.
customise="process.load('SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff')"
customise+='\n'"process.FEVTDEBUGoutput.outputCommands.append('keep *_simCaloStage2Layer1Summary_*_*')"

run_l1() {  # run_l1 <case> <event>
  local case=$1 ev=$2
  local in; in=$(jitjet_prev 11_DIGI "step3_DIGI_${case}_11_DIGI.root")
  local cfg=$CHDIR/step4_L1_${case}_${CHAPTER}_cfg.py out=step4_L1_${case}_${CHAPTER}.root log=$OUT/step4_L1_${case}_${CHAPTER}.log
  if (( REUSE )) && [[ -s $OUT/$out ]]; then echo "($case) reusing $out"; return 0; fi
  echo "($case) L1 on event $ev of $(basename "$in")"
  run_driver "$cfg" "$log" step4 --filein "file:$in" --conditions "$CONDITIONS" --era "$ERA" --geometry "$GEOMETRY" \
    -n -1 -s L1 --datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG --fileout "file:$out" \
    --customise_commands "$customise"'\n'"process.source.eventsToProcess = cms.untracked.VEventRange('$ev')"
  (( RUN )) && { [[ -s $OUT/$out ]] || { echo "No output produced: $OUT/$out" >&2; exit 1; }; }
  return 0
}
# the three L1 jobs in parallel (15 s each, most of it the conditions)
pids=()
for case in $CASES; do
  ev=$EVENT; [[ $case == pileup ]] && ev=$pu_event
  run_l1 "$case" "$ev" > "$OUT/run_$case.log" 2>&1 & pids+=($!)
done
for p in "${pids[@]}"; do wait "$p" || { cat "$OUT"/run_*.log >&2; exit 1; }; done
cat "$OUT"/run_*.log
(( RUN && ANALYZE )) || exit 0

# ------------------------------------------------------------------------------------------ the six steps drawn and counted
draw() {  # draw <case> <event> <label>
  local case=$1 ev=$2 label=$3
  local a=l1t_${case}_${CHAPTER}
  # the pages of an earlier run (the number of jet pages depends on the event)
  find "${OUT:?}" -maxdepth 1 -name "${a:?}_*.png" -delete
  run_cmsrun "$COMMON/l1tAnalyzer_cfg.py" "$OUT/$a.log" inputFiles="file:$OUT/step4_L1_${case}_${CHAPTER}.root" maxEvents=1 eventsToProcess="$ev" \
    globalTag="$CONDITIONS" era="$ERA" geometry="$GEOMETRY" pdfPrefix="$a" savePNG="$PNG" sampleLabel="$label"
  grep -E '^== L1T|^  TP (ECAL|HCAL):|^  Layer1 all|^  L1Jet [01]:|^  counts|^  TF counts|^  uGMT counts|^  decision' "$OUT/$a.log" | cut -c1-200
}
for case in $CASES; do
  case $case in
    signal) draw signal "$EVENT" "chapter 12 (12a): the signal alone|bbbar, pTHat > $PTHATMIN GeV, 13.6 TeV|2024 Level-1 trigger, no pileup" ;;
    pileup) draw pileup "$pu_event" "chapter 12 (12b): one pileup collision|library event $((SKIP + 1)), no signal|2024 Level-1 trigger" ;;
    mixed)  draw mixed "$EVENT" "chapter 12 (12a+12b): signal + pileup|PU = $PU per bx, bx $bx_min..$bx_max (playback)|2024 Level-1 trigger" ;;
  esac
done
first=${CASES%% *}
python3 "$COMMON/l1tSettings.py" "$CHDIR/step4_L1_${first}_${CHAPTER}_cfg.py" "$TEST/11_DIGI/step3_DIGI_${first}_11_DIGI_cfg.py" \
  > "$OUT/settings_${CHAPTER}.txt" 2> "$OUT/settings_${CHAPTER}.log" || { tail -20 "$OUT/settings_${CHAPTER}.log" >&2; exit 1; }
python3 "$COMMON/l1tTruth.py" "$OUT" "$CHAPTER" > "$OUT/truth_${CHAPTER}.txt" 2> "$OUT/truth_${CHAPTER}.log" || { tail -20 "$OUT/truth_${CHAPTER}.log" >&2; exit 1; }
python3 "$COMMON/l1tLedger.py" "$OUT" "$CHAPTER" > "$OUT/ledger_${CHAPTER}.txt" 2> "$OUT/ledger_${CHAPTER}.log" || { tail -20 "$OUT/ledger_${CHAPTER}.log" >&2; exit 1; }
echo "  Settings: settings_${CHAPTER}.txt; truth of the L1 muons: truth_${CHAPTER}.txt; ledger: ledger_${CHAPTER}.txt"
