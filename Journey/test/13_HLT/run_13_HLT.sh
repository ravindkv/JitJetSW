#!/usr/bin/env bash
# Chapter 13, HLT: the High Level Trigger, the second checkpoint of the journey. The event accepted by the Level-1
# trigger of chapter 12 is read out (DIGI2RAW: the digis and the Level-1 record packed into the raw data of the
# readout boards, as the detector would send them) and given to the HLT menu of the release (HLT:GRun), which
# reconstructs it with fast versions of the offline algorithms and decides, path by path, whether it is kept.
# Five steps (Rule G of the Constitution), all in the one HLT job:
#   step 1  the raw event, the L1 seed and the prescale (hltGtStage2Digis, hltGtStage2ObjectMap, HLTL1TSeed, HLTPrescaler)
#   step 2  calorimeter jets: ECAL/HCAL local reconstruction, calorimeter towers, AK4 calo jets and their corrections,
#           the calo-jet pre-filter (hltTowerMakerForAll, hltAK4CaloJets, hltAK4CaloJetsCorrected, hltSingleAK4CaloJet80)
#   step 3  tracking and vertices: pixel tracks and vertices, the iterative tracking (hltPixelTracks, hltPixelVertices,
#           hltIter0PFlowTrackSelectionHighPurity, hltDoubletRecoveryPFlowTrackSelectionHighPurity, hltMergedTracks)
#   step 4  particle flow and PF jets (hltParticleFlow, hltAK4PFJets, hltFixedGridRhoFastjetAll, hltAK4PFJetsCorrected)
#   step 5  the decision of the paths: the PF-jet filters, TriggerResults of every path, the datasets and streams
# ONE sample: the mixed event (Rule H). Input (Rule E): test/12_L1T/out/step4_L1_12_L1T.root
# The menu is the one the release gives the 2025 RelVals, HLT:@relval2025 = 2025v10 (Configuration/HLT/python/autoHLT.py),
# the frozen 2025 menu /frozen/2025/2e34/v1.0/HLT/V2, module for module the GRun menu /dev/CMSSW_15_0_0/GRun/V62 of the
# release. Its seeds are checked against the L1 menu of chapter 12 by python/hltL1Menu_cff.py (reads the 'algo:' lines
# of 12_L1T/out/l1t_12_L1T.log): with the 2025 L1 menu of the global tag every seed is there and nothing is changed.
# Output in test/13_HLT/out/:
#   step5_HLT_13_HLT.root, .log, _cfg.driver.log       the DIGI2RAW + HLT step (config test/13_HLT/step5_HLT_13_HLT_cfg.py)
#   hlt_13_HLT.log                                     the four analyzers (HLTPathAnalyzer, HLTCaloAnalyzer,
#                                                      HLTTrackAnalyzer, HLTPFAnalyzer)
#   hlt_13_HLT_{seed,calo,track,pf}.pdf + PNG          their pages: the eta-phi display of the event at every step
#   menu_13_HLT.txt                                    the menu: paths, seeds, filters and their parameters (common/hltMenu.py)
#   ledger_13_HLT.txt                                  the ledger, step by step (common/hltLedger.py)
# Variables: REUSE=1 (keep an existing HLT file); PU, BUNCHES of jitjet_common.sh label the pages
set -euo pipefail
CHAPTER=13_HLT
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
REUSE=${REUSE:-1}
MENU=${MENU:-@relval2025}
bx_min=${BUNCHES%,*}; bx_max=${BUNCHES#*,}
input=$(jitjet_prev 12_L1T step4_L1_12_L1T.root)
l1log=$(jitjet_prev 12_L1T l1t_12_L1T.log)
cfg=$CHDIR/step5_HLT_${CHAPTER}_cfg.py
out=step5_HLT_${CHAPTER}.root
# Customise lines. (1) The seeds of the menu checked against the L1 menu of chapter 12 (see above). (2) The
# HLT keeps in FEVTDEBUGHLT the jets, the tracks, the vertices, the PF candidates and the trigger summaries; the
# energy density rho, the PF vertices, the second tracking iteration, the calorimeter rec-hits, the PF clusters, the
# HLT muons and the objects of every filter are added for the analyzers. (3) The event of the book (last line).
# (cmsDriver executes every customise line on its own: the import and the call share one line)
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
# (4) DIGI2RAW packs the Level-1 record of chapter 12 into the raw data of the trigger boards. In production the L1 and
# DIGI2RAW run in the same job; three of the L1 products the packers read are not in the FEVTDEBUG content that chapter
# 12 wrote, and are remade here, with the same configuration as in the L1 job, at the head of the DIGI2RAW path: the
# ZDC energy sums (l1tZDCEtSums, from the HCAL trigger primitives; for the heavy-ion menus), the DT stubs merged with
# the RPC hits by the TwinMux (simTwinMuxDigis, from simDtTriggerPrimitiveDigis and simMuonRPCDigis; packed with the
# BMTF and OMTF records) and the external conditions of the global trigger (simGtExtFakeStage2Digis: the beam
# pick-ups, all true); the TwinMux takes its parameters from the event-setup producer of the L1 job
# (L1Trigger/L1TTwinMux/python/fakeTwinMuxParams_cff.py). (5) The packer of the PPS pixels (ctppsPixelRawData) is removed: the forward proton
# spectrometer is not simulated in this chain, there are no PPS digis to pack.
customise+='\n'"process.load('L1Trigger.L1TZDC.l1tZDCEtSums_cfi'); process.load('L1Trigger.L1TTwinMux.simTwinMuxDigis_cfi'); process.load('L1Trigger.Configuration.SimL1TechnicalTriggers_cff')"
customise+='\n'"process.digi2raw_step.insert(0, process.simGtExtFakeStage2Digis); process.digi2raw_step.insert(0, process.simTwinMuxDigis); process.digi2raw_step.insert(0, process.l1tZDCEtSums)"
customise+='\n'"from L1Trigger.L1TTwinMux.fakeTwinMuxParams_cff import twinmuxParamsSource, fakeTwinMuxParams; process.twinmuxParamsSource = twinmuxParamsSource; process.fakeTwinMuxParams = fakeTwinMuxParams"
customise+='\n'"del process.ctppsPixelRawData"
customise+='\n'"process.source.eventsToProcess = cms.untracked.VEventRange('$EVENT')"

if (( REUSE )) && [[ -s $OUT/$out ]]; then
  echo "reusing $out"
else
  echo "DIGI2RAW + HLT:$MENU on event $EVENT of $(basename "$input")"
  run_driver "$cfg" "$OUT/step5_HLT_${CHAPTER}.log" step5 --filein "file:$input" --conditions "$CONDITIONS" --era "$ERA" \
    --geometry "$GEOMETRY" -n -1 -s "DIGI2RAW,HLT:$MENU" --datatier GEN-SIM-DIGI-RAW --eventcontent FEVTDEBUGHLT \
    --fileout "file:$out" --customise_commands "$customise"
  (( RUN )) && { [[ -s $OUT/$out ]] || { echo "No output produced: $OUT/$out" >&2; exit 1; }; }
fi
(( RUN && ANALYZE )) || exit 0

# ------------------------------------------------------------------------------------------ the five steps drawn and counted
a=hlt_${CHAPTER}
# the pages of an earlier run
find "${OUT:?}" -maxdepth 1 -name "${a:?}_*.png" -delete
run_cmsrun "$COMMON/hltAnalyzer_cfg.py" "$OUT/$a.log" inputFiles="file:$OUT/$out" maxEvents=1 eventsToProcess="$EVENT" \
  globalTag="$CONDITIONS" era="$ERA" geometry="$GEOMETRY" pdfPrefix="$a" savePNG="$PNG" \
  sampleLabel="chapter 13: the High Level Trigger|bbbar, pTHat > $PTHATMIN GeV, 13.6 TeV|with PU = $PU per bx, bx $bx_min..$bx_max"
grep -E '^== HLT|^  menu:|^  rawdata:|^  paths HLT_|^  towers all:|^  tracks merged: n|^  vertices PF:|^  pfcands all:|^  accepted:' "$OUT/$a.log" | cut -c1-200
python3 "$COMMON/hltMenu.py" "$cfg" > "$OUT/menu_${CHAPTER}.txt" 2> "$OUT/menu_${CHAPTER}.log" || { tail -20 "$OUT/menu_${CHAPTER}.log" >&2; exit 1; }
python3 "$COMMON/hltLedger.py" "$OUT" "$CHAPTER" > "$OUT/ledger_${CHAPTER}.txt" 2> "$OUT/ledger_${CHAPTER}.log" || { tail -20 "$OUT/ledger_${CHAPTER}.log" >&2; exit 1; }
echo "  Menu: menu_${CHAPTER}.txt; ledger: ledger_${CHAPTER}.txt"
