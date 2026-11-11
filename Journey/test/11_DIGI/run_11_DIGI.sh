#!/usr/bin/env bash
# Chapter 11, DIGI: the digitisation. The SimHits of chapter 10 go through the detector electronics of the DIGI step
# (cmsDriver -s DIGI:pdigi_valid): the pixel and strip digitisers turn the energy loss in the sensors into ADC counts
# above threshold, the ECAL and HCAL digitisers into pulse shapes sampled in time slices with noise, zero suppression
# and selective readout, the muon digitisers into wire / strip hits with their timing. Three samples:
#   (11a) signal    the signal event alone through the DIGI step (no pileup mixing)
#   (11b) pileup    one library collision (library event SKIP+1) alone through the DIGI step
#   (11a+11b) mixed the production case: the signal with PU = Poisson(PU) collisions per bunch crossing in BUNCHES mixed in
#                   (the MixingModule of chapter 10, now with its digitisers). The collisions are exactly the ones chapter 10
#                   mixed: the MixingModule runs in playback mode and reads the CrossingFramePlaybackInfoNew that the
#                   chapter 10 mixing job wrote (a Poisson draw would differ here, because the digitisers of the signal
#                   consume the same random engine before the number of collisions is drawn). At this stage signal and
#                   pileup are no longer separable in the calorimeters; in the tracker and muon system the DigiSimLinks
#                   still tell which digi came from the signal, from in-time pileup or from an out-of-time bunch crossing.
# Input (Rule E): test/10_SIM/out/step2_SIM_signal_10_SIM.root (signal), step2_SIM_pileup_10_SIM.root (library) and
#                 step3_MIX_10_SIM.root (the signal with the playback record of the chapter 10 mixing).
# Output in test/11_DIGI/out/:
#   step3_DIGI_<case>_11_DIGI.root, .log, .driver.log   case = signal, pileup, mixed (configs in test/11_DIGI/)
#   digiAnalyzer_<case>_11_DIGI.pdf, .root, .log        eta-phi and r-z pages of the digis with the counts (DigiAnalyzer) + PNG
#   trigPrim_<case>_11_DIGI.pdf, .log + PNG             the calorimeter trigger primitives the DIGI step makes for the Level-1
#                                                       trigger of chapter 12 (TrigPrimAnalyzer): tower maps, the towers around the jets
#   trigprim_11_DIGI.txt                                the trigger primitives of the three samples side by side (common/trigPrimLedger.py)
#   thresholds_11_DIGI.txt                              the thresholds and settings of every digitiser (common/digiThresholds.py)
#   ledger_11_DIGI.txt                                  the ledger of the three samples (common/digiLedger.py)
# Variables: PU=60 BUNCHES=-1,1 SKIP=71 (jitjet_common.sh) CASES="signal pileup mixed" REUSE=1 (keep existing DIGI files)
set -euo pipefail
CHAPTER=11_DIGI
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
CASES=${CASES:-"signal pileup mixed"}
REUSE=${REUSE:-1}
bx_min=${BUNCHES%,*}; bx_max=${BUNCHES#*,}
signal=$(jitjet_prev 10_SIM step2_SIM_signal_10_SIM.root)
library=$(jitjet_prev 10_SIM step2_SIM_pileup_10_SIM.root)
mixed=$(jitjet_prev 10_SIM step3_MIX_10_SIM.root)
pu_event=1:1:$((SKIP + 1))

run_digi() {  # run_digi <case> <input> <event> <pileup args...>
  local case=$1 in=$2 ev=$3
  shift 3
  local cfg=$CHDIR/step3_DIGI_${case}_${CHAPTER}_cfg.py out=step3_DIGI_${case}_${CHAPTER}.root log=$OUT/step3_DIGI_${case}_${CHAPTER}.log
  if (( REUSE )) && [[ -s $OUT/$out ]]; then echo "($case) reusing $out"; return 0; fi
  local customise="process.source.eventsToProcess = cms.untracked.VEventRange('$ev')"
  (( $# )) && customise+='\n'"process.mix.input.sequential = cms.untracked.bool(True)"'\n'"process.mix.playback = cms.untracked.bool(True)"
  echo "($case) DIGI:pdigi_valid on event $ev of $(basename "$in") $*"
  run_driver "$cfg" "$log" step3 --filein "file:$in" "$@" --conditions "$CONDITIONS" --era "$ERA" --geometry "$GEOMETRY" \
    -n -1 -s DIGI:pdigi_valid --datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG --fileout "file:$out" --customise_commands "$customise"
  (( RUN )) && { [[ -s $OUT/$out ]] || { echo "No output produced: $OUT/$out" >&2; exit 1; }; }
  return 0
}
# the three DIGI jobs in parallel: the mixed one takes much longer (PU x bunch crossings library events are read and digitised)
pids=()
if [[ " $CASES " == *" signal "* ]]; then run_digi signal "$signal" "$EVENT" > "$OUT/run_signal.log" 2>&1 & pids+=($!); fi
if [[ " $CASES " == *" pileup "* ]]; then run_digi pileup "$library" "$pu_event" > "$OUT/run_pileup.log" 2>&1 & pids+=($!); fi
if [[ " $CASES " == *" mixed "* ]]; then
  run_digi mixed "$mixed" "$EVENT" --pileup "AVE_60_BX_25ns,{'N': $PU, 'B': ($bx_min, $bx_max)}" --pileup_input "file:$library"
fi
for p in "${pids[@]}"; do wait "$p" || { cat "$OUT"/run_*.log >&2; exit 1; }; done
cat "$OUT"/run_signal.log "$OUT"/run_pileup.log 2> /dev/null || true
(( RUN && ANALYZE )) || exit 0

# ------------------------------------------------------------------------------------------ the digis drawn and counted
draw() {  # draw <case> <event> <label>
  local case=$1 ev=$2 label=$3
  local a=digiAnalyzer_${case}_${CHAPTER}
  run_cmsrun "$COMMON/digiAnalyzer_cfg.py" "$OUT/$a.log" inputFiles="file:$OUT/step3_DIGI_${case}_${CHAPTER}.root" outputFile="$a.root" \
    maxEvents=1 eventsToProcess="$ev" globalTag="$CONDITIONS" era="$ERA" geometry="$GEOMETRY" pdfFile="$a.pdf" pngPrefix="$a" savePNG="$PNG" sampleLabel="$label"
  grep -E '^== DigiAnalyzer|^  [A-Za-z]+: nDigis|^  total' "$OUT/$a.log"
}
[[ " $CASES " == *" signal "* ]] && draw signal "$EVENT" "chapter $CHAPTER (11a): the signal digitised|bbbar, pTHat > $PTHATMIN GeV|2024 detector, 13.6 TeV|no pileup"
[[ " $CASES " == *" pileup "* ]] && draw pileup "$pu_event" "chapter $CHAPTER (11b): one pileup collision digitised|library event $((SKIP + 1))|SoftQCD inelastic, 13.6 TeV|2024 detector, no signal"
[[ " $CASES " == *" mixed "* ]] && draw mixed "$EVENT" "chapter $CHAPTER (11a+11b): signal + pileup digitised|PU = $PU per bx (Poisson), bx $bx_min..$bx_max|the collisions of the chapter 10 mixing (playback)|2024 detector, 13.6 TeV"
# ------------------------------------------------------------------------------------------ the trigger primitives
# The DIGI step also makes simEcalTriggerPrimitiveDigis and simHcalTriggerPrimitiveDigis, the coarse copy of the calorimeter
# data that the Level-1 trigger of chapter 12 will use. This is the last chapter with the signal and the pileup apart
# (from chapter 12 on only the mixed event is followed), so the three samples are compared here.
trigprim() {  # trigprim <case> <event> <mixed 0/1> <label>
  local case=$1 ev=$2 mixed=$3 label=$4
  local a=trigPrim_${case}_${CHAPTER}
  find "${OUT:?}" -maxdepth 1 -name "${a:?}_*.png" -delete
  run_cmsrun "$COMMON/trigPrimAnalyzer_cfg.py" "$OUT/$a.log" inputFiles="file:$OUT/step3_DIGI_${case}_${CHAPTER}.root" maxEvents=1 eventsToProcess="$ev" \
    globalTag="$CONDITIONS" era="$ERA" geometry="$GEOMETRY" pdfFile="$a.pdf" pngPrefix="$a" savePNG="$PNG" mixed="$mixed" sampleLabel="$label"
  grep -E '^== TrigPrimAnalyzer|^  TP (ECAL|HCAL):' "$OUT/$a.log"
}
[[ " $CASES " == *" signal "* ]] && trigprim signal "$EVENT" 0 "chapter $CHAPTER (11a): the signal alone|trigger primitives made by the DIGI step|bbbar, pTHat > $PTHATMIN GeV, no pileup"
[[ " $CASES " == *" pileup "* ]] && trigprim pileup "$pu_event" 0 "chapter $CHAPTER (11b): one pileup collision|trigger primitives made by the DIGI step|library event $((SKIP + 1)), no signal"
[[ " $CASES " == *" mixed "* ]] && trigprim mixed "$EVENT" 1 "chapter $CHAPTER (11a+11b): signal + pileup|trigger primitives made by the DIGI step|PU = $PU per bx, bx $bx_min..$bx_max (playback)"
python3 "$COMMON/trigPrimLedger.py" "$OUT" "$CHAPTER" > "$OUT/trigprim_${CHAPTER}.txt" 2> "$OUT/trigprim_${CHAPTER}.log" || { tail -20 "$OUT/trigprim_${CHAPTER}.log" >&2; exit 1; }
python3 "$COMMON/digiThresholds.py" "$CHDIR/step3_DIGI_mixed_${CHAPTER}_cfg.py" > "$OUT/thresholds_${CHAPTER}.txt" 2> "$OUT/thresholds_${CHAPTER}.log" ||
  { tail -20 "$OUT/thresholds_${CHAPTER}.log" >&2; exit 1; }
python3 "$COMMON/digiLedger.py" "$OUT" "$CHAPTER" > "$OUT/ledger_${CHAPTER}.txt" 2> "$OUT/ledger_${CHAPTER}.log" || { tail -20 "$OUT/ledger_${CHAPTER}.log" >&2; exit 1; }
echo "  Thresholds: thresholds_${CHAPTER}.txt; ledger: ledger_${CHAPTER}.txt; trigger primitives: trigprim_${CHAPTER}.txt"
