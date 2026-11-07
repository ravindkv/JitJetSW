#!/usr/bin/env bash
# Chapter 09, G4: the hand-over to Geant4. The generator final state is given to the SIM step (cmsDriver -s SIM, Geant4)
# with the physics reduced to the transportation alone: no magnetic field, no decays, no electromagnetic and no hadronic
# processes (physics list JitJetSW/Journey JourneyTransportPhysics), so that the file shows exactly which stable particles
# Geant4 accepted as primaries, which it dropped (|eta| > 5.5, p < 40 MeV, neutrinos: the Generator and StackingAction cuts
# of SimG4Core/Application/python/g4SimHits_cfi.py), and where they would go if nothing happened to them.
#   (9a) the signal: the bbbar event of chapter 07 through the transport-only Geant4
#   (9b) the pileup: all library collisions of chapter 08 through the same transport-only Geant4 (fast: nothing interacts)
#   (9a+9b) the ledger of signal + the NPU in-time collisions (library events SKIP+1 .. SKIP+NPU): what enters Geant4
# Input (Rule E): test/07_GenJet/out/step2_GENJET_07_GenJet.root (signal), test/08_PU/out/step1_GEN_MinBias_08_PU.root (pileup).
# Output in test/09_G4/out/:
#   step2_G4_signal_09_G4.root, .log, .driver.log     (9a), config test/09_G4/step2_G4_signal_09_G4_cfg.py (process SIM)
#   step2_G4_pileup_09_G4.root, .log, .driver.log     (9b), all PUEVENTS collisions, config step2_G4_pileup_09_G4_cfg.py
#   simAnalyzer_signal_09_G4.pdf, .root, .log         (9a) 5 generator pages + primaries, boundary, secondaries (none), hits + PNG
#   simAnalyzer_pileup_09_G4.pdf, .root, .log         (9b) the same pages for library event SKIP+1
#   ledger_09_G4.txt                                  (9a), (9b: the NPU in-time collisions) and the sum (common/g4Ledger.py)
# Variables: NPU=68 SKIP=71 (jitjet_common.sh, as in chapter 08), PUSEED=20241 (g4SimHits seed of the library = PUSEED + 2).
set -euo pipefail
CHAPTER=09_G4
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
# NPU, SKIP: the in-time collisions of the mixing of chapter 10 (jitjet_common.sh, jitjet_init)
PUSEED=${PUSEED:-20241}
signal=$(jitjet_prev 07_GenJet step2_GENJET_07_GenJet.root)
library=$(jitjet_prev 08_PU step1_GEN_MinBias_08_PU.root)
# transportation only: our physics list (all particles, no process), no field, no multiple-scattering coupling, no gamma
# general process, and the tracker records every sensor crossing as a hit with zero energy loss
transport="process.g4SimHits.UseMagneticField = cms.bool(False)"
transport+='\n'"process.g4SimHits.Physics.type = cms.string('SimG4Core/Physics/JourneyTransportPhysics')"
transport+='\n'"process.g4SimHits.Physics.G4TransportWithMSC = cms.int32(0)"
transport+='\n'"process.g4SimHits.Physics.G4GammaGeneralProcess = cms.bool(False)"
transport+='\n'"process.g4SimHits.TrackerSD.ZeroEnergyLoss = cms.bool(True)"
# a physics list built outside the release cannot be mixed with the simulation biglib (its own static Geant4)
CMSRUN_PREFIX="env LD_LIBRARY_PATH=$(sim_ld_library_path)"

run_g4() {  # run_g4 <case> <input> <events> <extra customise>
  local case=$1 in=$2 n=$3 extra=$4
  local cfg=$CHDIR/step2_G4_${case}_${CHAPTER}_cfg.py out=step2_G4_${case}_${CHAPTER}.root
  echo "(9${case:0:1}) transport-only Geant4 on $(basename "$in")"
  run_driver "$cfg" "$OUT/step2_G4_${case}_${CHAPTER}.log" step2 --filein "file:$in" --conditions "$CONDITIONS" --era "$ERA" \
    --geometry "$GEOMETRY" --beamspot "$BEAMSPOT" -n "$n" -s SIM --datatier GEN-SIM --eventcontent FEVTDEBUG \
    --fileout "file:$out" --customise_commands "$transport$extra"
  (( RUN )) && { [[ -s $OUT/$out ]] || { echo "No output produced: $OUT/$out" >&2; exit 1; }; }
}
run_g4 signal "$signal" "$EVENTS" ""
run_g4 pileup "$library" -1 '\n'"process.RandomNumberGeneratorService.g4SimHits.initialSeed = $((PUSEED + 2))"
(( RUN && ANALYZE )) || exit 0

draw() {  # draw <case> <event> <label>
  local case=$1 ev=$2 label=$3
  local a=simAnalyzer_${case}_${CHAPTER}
  run_cmsrun "$COMMON/simAnalyzer_cfg.py" "$OUT/$a.log" inputFiles="file:$OUT/step2_G4_${case}_${CHAPTER}.root" outputFile="$a.root" \
    maxEvents=1 eventsToProcess="$ev" simStage=1 genStages=5 globalTag="$CONDITIONS" era="$ERA" geometry="$GEOMETRY" \
    pdfFile="$a.pdf" sampleLabel="$label"
  grep -E '^== SimAnalyzer|^  page' "$OUT/$a.log"
  pdf_pages_png "$OUT/$a.pdf"
}
draw signal "$EVENT" "chapter $CHAPTER (9a): the signal handed to Geant4|transport only: no B field, no decays|no EM, no hadronic physics|bbbar pTHat > $PTHATMIN GeV, 13.6 TeV"
draw pileup "1:1:$((SKIP + 1))" "chapter $CHAPTER (9b): one pileup collision handed to Geant4|library event $((SKIP + 1)), transport only|no B field, no decays, no EM, no hadronic physics|SoftQCD inelastic, 13.6 TeV"
{
  python3 "$COMMON/g4Ledger.py" "$OUT/step2_G4_signal_${CHAPTER}.root" --events "$EVENT" --label "(9a) signal"
  echo
  python3 "$COMMON/g4Ledger.py" "$OUT/step2_G4_pileup_${CHAPTER}.root" --events "1:1:$((SKIP + 1))-1:1:$((SKIP + NPU))" --label "(9b) the $NPU in-time pileup collisions"
} > "$OUT/ledger_${CHAPTER}.txt" 2> "$OUT/ledger_${CHAPTER}.log" || { tail -20 "$OUT/ledger_${CHAPTER}.log" >&2; exit 1; }
grep -E '^== |^total' "$OUT/ledger_${CHAPTER}.txt"
echo "  Ledger: ledger_${CHAPTER}.txt"
