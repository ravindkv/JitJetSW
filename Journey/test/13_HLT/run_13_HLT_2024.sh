#!/usr/bin/env bash
# run_13_HLT_2024.sh: the year comparison of chapter 13 (Rule I3 of the Constitution: a chapter moved to 2025 is compared
# with its 2024 output, and the evidence is kept). The chapter runs with the 2025 settings (run_13_HLT.sh); this script
# makes the same event with the 2024 settings (auto:phase1_2024_realistic, Run3_2024), inside the repository, so that
# every 2024 number of chapter_13_HLT.tex and its companion can be checked:
#   (1) DIGI2RAW + HLT on the 2024 Level-1 file that test/12_L1T/run_12_L1T_2024.sh made (out2024/step4_L1_12_L1T.root:
#       the same SimHits and collisions of chapter 10, digitised and triggered with the 2024 settings)
#                                                              -> out2024/step5_HLT_13_HLT.root
#       The release has no 2024 physics menu (HLT:@relval2024 is the three-path Fake2), so the 2024 run takes the physics
#       menu of the release, -s HLT:GRun (/dev/CMSSW_15_0_0/GRun/V62, module for module the 2025v10 of run_13_HLT.sh),
#       and python/hltL1Menu_cff.py adapts its seeds to the 2024 Level-1 menu of chapter 12
#       (L1Menu_Collisions2024_v1_3_0, read from the 'algo:' lines of out2024/l1t_12_L1T.log): the algorithms that the
#       2024 menu does not have are dropped from the OR seeds, the paths whose seed can never fire are removed.
#   (2) the four analyzers, the menu and the ledger, as in run_13_HLT.sh -> out2024/hlt_13_HLT.log, menu_13_HLT.txt,
#       ledger_13_HLT.txt and the PNG pages
# Input: test/12_L1T/out2024/ (run test/12_L1T/run_12_L1T_2024.sh first). Configuration: test/13_HLT/step5_HLT_2024_13_HLT_cfg.py
# Variables: REUSE=1 (keep an existing HLT file), ANALYZE=0 (only the cmsRun job), PNG=0 (no PNG pages), MENU=GRun
# Cost: about two minutes (one minute to start the HLT job).
set -euo pipefail
CHAPTER=13_HLT
YEAR=2024
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
OUT=$CHDIR/out2024
mkdir -p "$OUT"
echo "   (the year comparison: $CONDITIONS, $ERA; output in $OUT)"
REUSE=${REUSE:-1}
MENU=${MENU:-GRun}
bx_min=${BUNCHES%,*}; bx_max=${BUNCHES#*,}
input=$TEST/12_L1T/out2024/step4_L1_12_L1T.root
l1log=$TEST/12_L1T/out2024/l1t_12_L1T.log
for f in "$input" "$l1log"; do
  [[ -s $f ]] || { echo "Missing $f: run test/12_L1T/run_12_L1T_2024.sh first" >&2; exit 1; }
done
cfg=$CHDIR/step5_HLT_${YEAR}_${CHAPTER}_cfg.py
out=step5_HLT_${CHAPTER}.root
# the customise lines of run_13_HLT.sh, the seeds checked against the 2024 Level-1 menu
customise="from JitJetSW.Journey.hltL1Menu_cff import adaptSeedsToL1Menu; process = adaptSeedsToL1Menu(process, '$l1log')"
keep="'keep *_hltFixedGridRhoFastjetAll_*_*', 'keep *_hltFixedGridRhoFastjetAllCalo_*_*', 'keep *_hltVerticesPF_*_*'"
keep+=", 'keep *_hltDoubletRecoveryPFlowTrackSelectionHighPurity_*_*', 'keep recoPFClusters_hltParticleFlowCluster*_*_*'"
keep+=", 'keep recoPFRecHits_hltParticleFlowRecHit*_*_*', 'keep EcalRecHitsSorted_hltEcalRecHit_*_*'"
keep+=", 'keep HBHERecHitsSorted_hltHbhereco_*_*', 'keep HFRecHitsSorted_hltHfreco_*_*'"
keep+=", 'keep SiPixelClusteredmNewDetSetVector_hltSiPixelClusters_*_*'"
keep+=", 'keep SiStripClusteredmNewDetSetVector_hltSiStripRawToClustersFacility_*_*'"
keep+=", 'keep recoRecoChargedCandidates_*_*_HLT', 'keep recoMuons_*_*_HLT', 'keep recoTracks_hltL2Muons_*_*'"
keep+=", 'keep triggerTriggerFilterObjectWithRefs_*_*_HLT'"
customise+='\n'"process.FEVTDEBUGHLToutput.outputCommands.extend([$keep])"
customise+='\n'"process.load('L1Trigger.L1TZDC.l1tZDCEtSums_cfi'); process.load('L1Trigger.L1TTwinMux.simTwinMuxDigis_cfi'); process.load('L1Trigger.Configuration.SimL1TechnicalTriggers_cff')"
customise+='\n'"process.digi2raw_step.insert(0, process.simGtExtFakeStage2Digis); process.digi2raw_step.insert(0, process.simTwinMuxDigis); process.digi2raw_step.insert(0, process.l1tZDCEtSums)"
customise+='\n'"from L1Trigger.L1TTwinMux.fakeTwinMuxParams_cff import twinmuxParamsSource, fakeTwinMuxParams; process.twinmuxParamsSource = twinmuxParamsSource; process.fakeTwinMuxParams = fakeTwinMuxParams"
customise+='\n'"del process.ctppsPixelRawData"
customise+='\n'"process.source.eventsToProcess = cms.untracked.VEventRange('$EVENT')"

if (( REUSE )) && [[ -s $OUT/$out ]]; then
  echo "(1) reusing $out"
else
  echo "(1) DIGI2RAW + HLT:$MENU on event $EVENT of 12_L1T/out2024/$(basename "$input"), $YEAR settings"
  run_driver "$cfg" "$OUT/step5_HLT_${CHAPTER}.log" step5 --filein "file:$input" --conditions "$CONDITIONS" --era "$ERA" \
    --geometry "$GEOMETRY" -n -1 -s "DIGI2RAW,HLT:$MENU" --datatier GEN-SIM-DIGI-RAW --eventcontent FEVTDEBUGHLT \
    --fileout "file:$out" --customise_commands "$customise"
  (( RUN )) && { [[ -s $OUT/$out ]] || { echo "No output produced: $OUT/$out" >&2; exit 1; }; }
  grep -E '^hltL1Menu_cff' "$OUT/step5_HLT_${CHAPTER}.log" "$OUT/$(basename "${cfg%.py}").driver.log" 2> /dev/null | head -3 || true
fi
(( RUN && ANALYZE )) || exit 0

# (2) the analyzers, the menu and the ledger of the 2024 run (as run_13_HLT.sh)
a=hlt_${CHAPTER}
find "${OUT:?}" -maxdepth 1 -name "${a:?}_*.png" -delete
run_cmsrun "$COMMON/hltAnalyzer_cfg.py" "$OUT/$a.log" inputFiles="file:$OUT/$out" maxEvents=1 eventsToProcess="$EVENT" \
  globalTag="$CONDITIONS" era="$ERA" geometry="$GEOMETRY" pdfPrefix="$a" savePNG="$PNG" \
  sampleLabel="chapter 13: the High Level Trigger of $YEAR (year comparison)|bbbar, pTHat > $PTHATMIN GeV, 13.6 TeV|with PU = $PU per bx, bx $bx_min..$bx_max"
grep -E '^== HLT|^  menu:|^  rawdata:|^  paths HLT_|^  towers all:|^  tracks merged: n|^  vertices PF:|^  pfcands all:|^  accepted:' "$OUT/$a.log" | cut -c1-200
python3 "$COMMON/hltMenu.py" "$cfg" > "$OUT/menu_${CHAPTER}.txt" 2> "$OUT/menu_${CHAPTER}.log" || { tail -20 "$OUT/menu_${CHAPTER}.log" >&2; exit 1; }
python3 "$COMMON/hltLedger.py" "$OUT" "$CHAPTER" > "$OUT/ledger_${CHAPTER}.txt" 2> "$OUT/ledger_${CHAPTER}.log" || { tail -20 "$OUT/ledger_${CHAPTER}.log" >&2; exit 1; }
echo "  The 2024 run: out2024/ledger_${CHAPTER}.txt, out2024/menu_${CHAPTER}.txt"
