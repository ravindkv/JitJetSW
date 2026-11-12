#!/usr/bin/env bash
# Chapter 10, SIM: the detector simulation. The primaries of chapter 09 are now transported with the full Geant4 physics
# (FTFP_BERT_EMM, 3.8 T field, the geometry of the 2025 global tag from the database, YEAR=2025 of jitjet_common.sh):
# they bend, decay, shower, interact, and the sensitive detectors record the SimHits. To show what each ingredient does,
# the signal is simulated four times on top of the transport-only file of chapter 09 (process SIMFULL, the transport-only
# products of process SIM are dropped from the output):
#   B            + magnetic field (UseMagneticField), still no decays / EM / hadronic physics (JourneyTransportPhysics)
#   B_Decay      + decays in flight (G4DecayPhysics; FTFP_BERT_EMM with EMPhysics and HadPhysics off)
#   B_Decay_EM   + electromagnetic physics (ionisation, bremsstrahlung, pair production, Compton, photoelectric, multiple scattering)
#   signal       + hadronic physics: the full SIM step (B_Decay_EM_Had), the file every later chapter uses
# and the three samples of the chapter are drawn station by station with SimHitAnalyzer (TRK, ECAL, HCAL, MUON, ALL):
#   (10a) signal   the full-physics signal event
#   (10b) pileup   the pileup library: all collisions of chapter 09 through the full Geant4 (the file the MixingModule reads
#                  in chapters 10 and 11, i.e. the GEN-SIM library the premixing of the production starts from); library
#                  event SKIP+1 is drawn
#   (10a+10b)      the mixing at SimHit level: the MixingModule of the DIGI step (PU = Poisson(PU) collisions per bunch crossing
#                  in BUNCHES, library read in sequence, process MIX) with the digitisers removed (python/mixOnly_cff.py),
#                  SimHitAnalyzer reading the CrossingFrames: the signal hits with the pileup hits of bx -1, 0, +1 underneath.
#                  The job writes step3_MIX_10_SIM.root: the signal event with the playback record of the draw
#                  (CrossingFramePlaybackInfoNew: which library events went into which bunch crossing), the input of the
#                  mixed DIGI of chapter 11 (MixingModule playback), so that 11 digitises exactly these collisions.
# Input (Rule E): test/09_G4/out/step2_G4_signal_09_G4.root and step2_G4_pileup_09_G4.root.
# Output in test/10_SIM/out/:
#   step2_SIM_<stage>_10_SIM.root, .log, .driver.log       stage = B, B_Decay, B_Decay_EM, signal (configs in test/10_SIM/)
#   step2_SIM_pileup_10_SIM.root, .log, .driver.log        (10b) the library, PUEVENTS collisions
#   simAnalyzer_<stage>_10_SIM.pdf, .root, .log            the five SimAnalyzer pages per stage (+ 5 generator pages for signal) + PNG
#   signal/<station>/simHits_signal_10_SIM_<station>_run1_event1.pdf, .png   (10a), log simHits_signal_10_SIM.log
#   pileup/<station>/simHits_pileup_10_SIM_<station>_...                     (10b), log simHits_pileup_10_SIM.log
#   mixed/<station>/simHits_mixed_10_SIM_<station>_...                       (10a+10b), log simHits_mixed_10_SIM.log, job log step3_MIX_10_SIM.log
#   step3_MIX_10_SIM.root, .driver.log                     (10a+10b) the signal with the playback record of the mixing (config step3_MIX_10_SIM_cfg.py)
#   ledger_10_SIM.txt                                      the ledger of the stages and of the three samples (common/simLedger.py)
# Variables: PU=60 BUNCHES=-1,1 SKIP=71 NPU=68 (jitjet_common.sh) PUSEED=20241 STATIONS= (subset) HIGHLIGHT=1 HIGHLIGHT_PTMIN=2 PRINTHITS=0
#            STAGES="B B_Decay B_Decay_EM signal" CASES="signal pileup mixed" REUSE=1 (keep existing SIM files)
set -euo pipefail
CHAPTER=10_SIM
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
# PU, BUNCHES, SKIP, NPU: the pileup convention of the book (jitjet_common.sh, jitjet_init)
PUSEED=${PUSEED:-20241}
STATIONS=${STATIONS:-}
HIGHLIGHT=${HIGHLIGHT:-1}
HIGHLIGHT_PTMIN=${HIGHLIGHT_PTMIN:-2}
PRINTHITS=${PRINTHITS:-0}
STAGES=${STAGES:-"B B_Decay B_Decay_EM signal"}
CASES=${CASES:-"signal pileup mixed"}
REUSE=${REUSE:-1}
bx_min=${BUNCHES%,*}; bx_max=${BUNCHES#*,}
signal_in=$(jitjet_prev 09_G4 step2_G4_signal_09_G4.root)
library_in=$(jitjet_prev 09_G4 step2_G4_pileup_09_G4.root)
no_biglib="env LD_LIBRARY_PATH=$(sim_ld_library_path)"

# ------------------------------------------------------------------------------------------ the SIM step, stage by stage
# Geant4 switches per stage (B field, decays, EM, hadronic physics) and the SimAnalyzer stage number
stage_switches() {
  case $1 in
    B)            echo on off off off 2 ;;
    B_Decay)      echo on on  off off 3 ;;
    B_Decay_EM)   echo on on  on  off 4 ;;
    signal|pileup) echo on on on on 5 ;;
    *) echo "Unknown stage '$1' (B, B_Decay, B_Decay_EM, signal, pileup)" >&2; exit 1 ;;
  esac
}
py_bool() { [[ $1 == on ]] && echo True || echo False; }

run_sim() {  # run_sim <stage> <input> <events> <extra customise>
  local stage=$1 in=$2 n=$3 extra=$4 bfield decay em had num
  read -r bfield decay em had num < <(stage_switches "$stage")
  local cfg=$CHDIR/step2_SIM_${stage}_${CHAPTER}_cfg.py out=step2_SIM_${stage}_${CHAPTER}.root log=$OUT/step2_SIM_${stage}_${CHAPTER}.log
  if (( REUSE )) && [[ -s $OUT/$out ]]; then echo "Stage $stage: reusing $out"; return 0; fi
  local -a c=("process.g4SimHits.UseMagneticField = cms.bool($(py_bool "$bfield"))")
  if [[ $decay == off ]]; then
    c+=("process.g4SimHits.Physics.type = cms.string('SimG4Core/Physics/JourneyTransportPhysics')")
  else
    c+=("process.g4SimHits.Physics.EMPhysics = cms.untracked.bool($(py_bool "$em"))" "process.g4SimHits.Physics.HadPhysics = cms.untracked.bool($(py_bool "$had"))")
  fi
  if [[ $em == off ]]; then
    c+=("process.g4SimHits.Physics.G4TransportWithMSC = cms.int32(0)" "process.g4SimHits.Physics.G4GammaGeneralProcess = cms.bool(False)" "process.g4SimHits.TrackerSD.ZeroEnergyLoss = cms.bool(True)")
  fi
  local joined=${c[0]} x
  for x in "${c[@]:1}"; do joined+='\n'"$x"; done
  joined+=$extra
  echo "Stage $stage: B=$bfield Decay=$decay EM=$em Had=$had on $(basename "$in")"
  local prefix=""
  [[ $decay == off ]] && prefix=$no_biglib   # the JourneyTransportPhysics list needs the shared-library Geant4
  CMSRUN_PREFIX=$prefix run_driver "$cfg" "$log" step2 --filein "file:$in" --conditions "$CONDITIONS" --era "$ERA" --geometry "$GEOMETRY" \
    --beamspot "$BEAMSPOT" -n "$n" -s SIM --processName SIMFULL --datatier GEN-SIM --eventcontent FEVTDEBUG \
    --outputCommands "drop *_*_*_SIM" --fileout "file:$out" --customise_commands "$joined"
  (( RUN )) && { [[ -s $OUT/$out ]] || { echo "No output produced: $OUT/$out" >&2; exit 1; }; }
  return 0
}

# the library (long: PUEVENTS collisions, about 2.5 s each) runs in the background while the signal stages run
if [[ " $CASES " == *" pileup "* || " $CASES " == *" mixed "* ]]; then
  run_sim pileup "$library_in" -1 '\n'"process.RandomNumberGeneratorService.g4SimHits.initialSeed = $((PUSEED + 2))" > "$OUT/run_pileup.log" 2>&1 &
  lib_pid=$!
else
  lib_pid=
fi
for stage in $STAGES; do
  run_sim "$stage" "$signal_in" "$EVENTS" ""
done
if [[ -n $lib_pid ]]; then
  echo "Waiting for the library SIM (step2_SIM_pileup_${CHAPTER}.root) ..."
  wait "$lib_pid" || { cat "$OUT/run_pileup.log" >&2; exit 1; }
  cat "$OUT/run_pileup.log"
fi
# RUN=0: only the configurations are written (the mixing one included, as the ANALYZE setting asks), nothing is run
if (( RUN && ANALYZE )); then

# ------------------------------------------------------------------------------------------ the SimAnalyzer pages per stage
for stage in $STAGES; do
  read -r bfield decay em had num < <(stage_switches "$stage")
  a=simAnalyzer_${stage}_${CHAPTER}
  gen_pages=0; [[ $stage == signal ]] && gen_pages=5
  run_cmsrun "$COMMON/simAnalyzer_cfg.py" "$OUT/$a.log" inputFiles="file:$OUT/step2_SIM_${stage}_${CHAPTER}.root" outputFile="$a.root" \
    maxEvents=1 eventsToProcess="$EVENT" simStage="$num" genStages="$gen_pages" globalTag="$CONDITIONS" era="$ERA" geometry="$GEOMETRY" \
    pdfFile="$a.pdf" sampleLabel="chapter $CHAPTER: stage $stage|B field $bfield, Decay $decay|EM $em, Had $had|bbbar pTHat > $PTHATMIN GeV, 13.6 TeV"
  grep -E '^== SimAnalyzer|^  page' "$OUT/$a.log"
  pdf_pages_png "$OUT/$a.pdf"
done

# ------------------------------------------------------------------------------------------ the three samples, station by station
analyzer_args=(era="$ERA" globalTag="$CONDITIONS" geometry="$GEOMETRY" highlight="$HIGHLIGHT" highlightPtMin="$HIGHLIGHT_PTMIN" savePNG="$PNG" printHits="$PRINTHITS" maxEvents=-1)
[[ -n $STATIONS ]] && analyzer_args+=(stations="$STATIONS")
draw() {  # draw <case> <input> <event> <label>
  local case=$1 in=$2 ev=$3 label=$4
  mkdir -p "$OUT/$case"
  run_cmsrun "$COMMON/simHitAnalyzer_cfg.py" "$OUT/simHits_${case}_${CHAPTER}.log" inputFiles="file:$in" outputDir="$OUT/$case" \
    prefix="simHits_${case}_${CHAPTER}" eventsToProcess="$ev" sampleLabel="$label" "${analyzer_args[@]}"
  grep -E "^SimHitAnalyzer run|^  station|^    primaries with hits" "$OUT/simHits_${case}_${CHAPTER}.log"
}
if [[ " $CASES " == *" signal "* ]]; then
  draw signal "$OUT/step2_SIM_signal_${CHAPTER}.root" "$EVENT" "chapter $CHAPTER (10a): the signal|bbbar, pTHat > $PTHATMIN GeV|full Geant4 physics, $YEAR detector|no pileup"
fi
if [[ " $CASES " == *" pileup "* ]]; then
  draw pileup "$OUT/step2_SIM_pileup_${CHAPTER}.root" "1:1:$((SKIP + 1))" "chapter $CHAPTER (10b): one pileup collision|library event $((SKIP + 1)), full Geant4 physics|SoftQCD inelastic, 13.6 TeV|$YEAR detector, no signal"
fi
fi  # ANALYZE
if [[ " $CASES " == *" mixed "* ]]; then
  mkdir -p "$OUT/mixed"
  label="chapter $CHAPTER (10a+10b): signal + pileup at SimHit level|PU = $PU per bx (Poisson), bx $bx_min..$bx_max|library in sequence, no digitisers|$YEAR detector, 13.6 TeV"
  py_true() { (( $1 )) && echo True || echo False; }
  kw="highlight=$HIGHLIGHT, highlightPtMin=$HIGHLIGHT_PTMIN, savePNG=$(py_true "$PNG"), printHits=$(py_true "$PRINTHITS")"
  [[ -n $STATIONS ]] && kw+=", stations='$STATIONS'"
  customise="process.mix.input.sequential = cms.untracked.bool(True)"
  customise+='\n'"process.source.eventsToProcess = cms.untracked.VEventRange('$EVENT')"
  if (( ANALYZE )); then
    customise+='\n'"from JitJetSW.Journey.mixOnly_cff import mixOnly; process = mixOnly(process, outputDir='$OUT/mixed', sampleLabel='$label', prefix='simHits_mixed_${CHAPTER}', $kw)"
  else   # the mixing and its playback record without the drawing
    customise+='\n'"from JitJetSW.Journey.mixOnly_cff import mixOnly; process = mixOnly(process)"
  fi
  echo "(10a+10b) MixingModule only: PU=$PU (Poisson) in bunch crossings $bx_min..$bx_max from step2_SIM_pileup_${CHAPTER}.root, SimHitAnalyzer on the CrossingFrames"
  run_driver "$CHDIR/step3_MIX_${CHAPTER}_cfg.py" "$OUT/step3_MIX_${CHAPTER}.log" step3 --filein "file:$OUT/step2_SIM_signal_${CHAPTER}.root" \
    --pileup "AVE_60_BX_25ns,{'N': $PU, 'B': ($bx_min, $bx_max)}" --pileup_input "file:$OUT/step2_SIM_pileup_${CHAPTER}.root" \
    --conditions "$CONDITIONS" --era "$ERA" --geometry "$GEOMETRY" -n -1 -s DIGI:pdigi_valid --processName MIX --datatier GEN-SIM-DIGI \
    --eventcontent FEVTDEBUG --fileout "file:step3_MIX_${CHAPTER}.root" --outputCommands "drop *_mix_*_*,keep CrossingFramePlaybackInfoNew_mix_*_*" \
    --customise_commands "$customise"
  (( RUN )) && { [[ -s $OUT/step3_MIX_${CHAPTER}.root ]] || { echo "No output produced: $OUT/step3_MIX_${CHAPTER}.root" >&2; exit 1; }; }
  (( RUN && ANALYZE )) || exit 0
  grep -E "^SimHitAnalyzer|^  pileup mixed|^    pileup event|^  station|^    hits by|^    primaries with hits" "$OUT/step3_MIX_${CHAPTER}.log" > "$OUT/simHits_mixed_${CHAPTER}.log"
  grep -E "^SimHitAnalyzer run|^  pileup mixed|^  station|^    primaries with hits" "$OUT/simHits_mixed_${CHAPTER}.log"
  # the in-time collisions chapters 08 and 09 assumed (SKIP, NPU) must be the ones the MixingModule drew
  read -r early intime < <(grep -m1 "events per bx:" "$OUT/simHits_mixed_${CHAPTER}.log" | sed 's/.*events per bx://' | awk '
    { e = 0; i = 0; for (k = 1; k <= NF; ++k) { split($k, a, ":"); if (a[1] < 0) e += a[2]; if (a[1] == 0) i = a[2] } print e, i }')
  if [[ $early != "$SKIP" || $intime != "$NPU" ]]; then
    echo "WARNING: the mixing put library events $((early + 1))-$((early + intime)) in time, chapters 08 / 09 used SKIP=$SKIP NPU=$NPU; set SKIP=$early NPU=$intime in common/jitjet_common.sh and rerun 08, 09, 11" | tee "$OUT/WARNING_intime_${CHAPTER}.txt"
  else
    echo "  in-time collisions of the mixing: library events $((early + 1))-$((early + intime)), as assumed in chapters 08 and 09"
  fi
fi
(( ANALYZE )) || exit 0
python3 "$COMMON/simLedger.py" "$OUT" "$CHAPTER" "$TEST/09_G4/out/simAnalyzer_signal_09_G4.log" > "$OUT/ledger_${CHAPTER}.txt" 2> "$OUT/ledger_${CHAPTER}.log" ||
  { tail -20 "$OUT/ledger_${CHAPTER}.log" >&2; exit 1; }
echo "  Ledger: ledger_${CHAPTER}.txt"
# the FWLite numbers the analyzers do not print (decays, conversions, nuclear interactions, the b-system budget, the
# followed photon, the vertices of the mixing), read from the SIM files of chapters 09 and 10; quoted in the chapter
python3 "$CHDIR/simExtras_${CHAPTER}.py" > "$OUT/extras_${CHAPTER}.txt" 2> "$OUT/extras_${CHAPTER}.log" &&
  echo "  FWLite extras: extras_${CHAPTER}.txt" || echo "  FWLite extras not made (see extras_${CHAPTER}.log)"
