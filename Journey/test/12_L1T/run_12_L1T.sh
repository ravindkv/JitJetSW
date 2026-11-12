#!/usr/bin/env bash
# Chapter 12, L1T: the Level-1 trigger, the first checkpoint of the journey. The digis of chapter 11 go through the
# emulator of the Run-3 (stage-2) Level-1 trigger (cmsDriver -s L1 = the SimL1Emulator sequence of the era Run3_2025),
# which does not change the event: it decides, in six steps, whether the event is kept.
#   step 1  the calorimeter trigger primitives (made in the DIGI step of chapter 11: simEcalTriggerPrimitiveDigis,
#           simHcalTriggerPrimitiveDigis): one transverse energy per trigger tower and bunch crossing
#   step 2  calorimeter Layer-1 (simCaloStage2Layer1Digis): ECAL + HCAL of a tower calibrated and summed
#   step 3  calorimeter Layer-2 (simCaloStage2Digis): jets, e/gamma, taus, energy sums
#   step 4  the muon trigger primitives (simDtTriggerPrimitiveDigis, simCscTriggerPrimitiveDigis, simMuonGEMPadDigis)
#           and the muon track finders (simKBmtfDigis, simOmtfDigis, simEmtfDigis)
#   step 5  the global muon trigger (simGmtStage2Digis)
#   step 6  the global trigger (simGtStage2Digis): every algorithm of the menu, the FinalOR
# ONE sample: the event as CMS sees it, the signal with its PU = 60 bunch crossings. Chapter 11 (DIGI) is the last
# stage at which the signal and the pileup exist apart; from this chapter on only the mixed event is followed.
# Input (Rule E): test/11_DIGI/out/step3_DIGI_mixed_11_DIGI.root
# Output in test/12_L1T/out/:
#   step4_L1_12_L1T.root, .log, _cfg.driver.log        the L1 step (config test/12_L1T/step4_L1_12_L1T_cfg.py)
#   l1t_12_L1T.log                                     the three analyzers (L1TCaloAnalyzer, L1TMuonAnalyzer, L1TGlobalAnalyzer)
#   l1t_12_L1T_{calo,muon,global}.pdf + PNG            their pages: the eta-phi display of the event at every step
#   settings_12_L1T.txt                                the parameters of every module of the L1 step (common/l1tSettings.py)
#   truth_12_L1T.txt                                   who made the L1 muons: the simulated particles behind them (common/l1tTruth.py)
#   ledger_12_L1T.txt                                  the ledger, step by step (common/l1tLedger.py)
# Variables: REUSE=1 (keep an existing L1 file); PU, BUNCHES of jitjet_common.sh label the pages
set -euo pipefail
CHAPTER=12_L1T
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
REUSE=${REUSE:-1}
bx_min=${BUNCHES%,*}; bx_max=${BUNCHES#*,}
input=$(jitjet_prev 11_DIGI step3_DIGI_mixed_11_DIGI.root)
cfg=$CHDIR/step4_L1_${CHAPTER}_cfg.py
out=step4_L1_${CHAPTER}.root
# -s L1 alone is not a production configuration (the L1 emulator always runs behind the DIGI step): two lines are
# added. (1) Layer-1 needs the HCAL trigger-tower geometry and the decoder of the HCAL trigger-primitive scale, two
# event-setup producers that come with the HCAL trigger-primitive emulator of the DIGI step. (2) The anomaly score
# of CICADA (simCaloStage2Layer1Summary), which the eight L1_CICADA_* algorithms of the menu cut on, is not in the event content.
customise="process.load('SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff')"
customise+='\n'"process.FEVTDEBUGoutput.outputCommands.append('keep *_simCaloStage2Layer1Summary_*_*')"
customise+='\n'"process.source.eventsToProcess = cms.untracked.VEventRange('$EVENT')"

if (( REUSE )) && [[ -s $OUT/$out ]]; then
  echo "reusing $out"
else
  echo "L1 on event $EVENT of $(basename "$input")"
  run_driver "$cfg" "$OUT/step4_L1_${CHAPTER}.log" step4 --filein "file:$input" --conditions "$CONDITIONS" --era "$ERA" --geometry "$GEOMETRY" \
    -n -1 -s L1 --datatier GEN-SIM-DIGI --eventcontent FEVTDEBUG --fileout "file:$out" --customise_commands "$customise"
  (( RUN )) && { [[ -s $OUT/$out ]] || { echo "No output produced: $OUT/$out" >&2; exit 1; }; }
fi
(( RUN && ANALYZE )) || exit 0

# ------------------------------------------------------------------------------------------ the six steps drawn and counted
a=l1t_${CHAPTER}
# the pages of an earlier run (the number of jet pages depends on the event)
find "${OUT:?}" -maxdepth 1 -name "${a:?}_*.png" -delete
run_cmsrun "$COMMON/l1tAnalyzer_cfg.py" "$OUT/$a.log" inputFiles="file:$OUT/$out" maxEvents=1 eventsToProcess="$EVENT" \
  globalTag="$CONDITIONS" era="$ERA" geometry="$GEOMETRY" pdfPrefix="$a" savePNG="$PNG" \
  sampleLabel="chapter 12: the Level-1 trigger of $YEAR|bbbar, pTHat > $PTHATMIN GeV, 13.6 TeV|with PU = $PU per bx, bx $bx_min..$bx_max"
grep -E '^== L1T|^  TP (ECAL|HCAL):|^  Layer1 all|^  L1Jet [01]:|^  counts|^  TF counts|^  uGMT counts|^  decision' "$OUT/$a.log" | cut -c1-200
python3 "$COMMON/l1tSettings.py" "$cfg" "$TEST/11_DIGI/step3_DIGI_mixed_11_DIGI_cfg.py" \
  > "$OUT/settings_${CHAPTER}.txt" 2> "$OUT/settings_${CHAPTER}.log" || { tail -20 "$OUT/settings_${CHAPTER}.log" >&2; exit 1; }
python3 "$COMMON/l1tTruth.py" "$OUT" "$CHAPTER" > "$OUT/truth_${CHAPTER}.txt" 2> "$OUT/truth_${CHAPTER}.log" || { tail -20 "$OUT/truth_${CHAPTER}.log" >&2; exit 1; }
python3 "$COMMON/l1tLedger.py" "$OUT" "$CHAPTER" > "$OUT/ledger_${CHAPTER}.txt" 2> "$OUT/ledger_${CHAPTER}.log" || { tail -20 "$OUT/ledger_${CHAPTER}.log" >&2; exit 1; }
echo "  Settings: settings_${CHAPTER}.txt; truth of the L1 muons: truth_${CHAPTER}.txt; ledger: ledger_${CHAPTER}.txt"
