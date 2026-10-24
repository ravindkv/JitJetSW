#!/usr/bin/env bash
# SIM chapter of the JitJet book for the Phase2 detector: the SimHits of one event drawn on the detector,
# station by station, with SimHitAnalyzer (plugins/SimHitAnalyzer.cc, test/simHitAnalyzer_cfg.py).
# Input: a Phase2 SIM stage sample of runCMSDrivers_mcPhase2_BBbar_SIM.sh (default the full-physics stage
# SIM_Phase2/step2_SIM_Transport_B_Decay_EM_Had_out.root, generated with that script if missing). For the
# event EVENT (default run 1, lumi 1, event 1) and every station it writes into OUTDIR/<station>/:
#   simHits_<station>_run<R>_event<E>.pdf          three pages: r-z view, x-y view (barrel), eta-phi view
#   simHits_<station>_<view>_run<R>_event<E>.png   the same pages as PNG (view = rz, xy, etaphi)
#   At<station>.md                                 the chapter text (copied from doc/sim/, DOCS=1)
# with the stations TRK (tracker), ECAL (EB + HGCal EE), HCAL (HB, HO, HF + HGCal HSi, HSc), HGCAL (the whole
# endcap calorimeter), MUON (DT, CSC, RPC, GEM) and ALL (everything). The pages show the detector elements of
# the station from the reco geometry in grey and the hits coloured by the kind of the primary (the particle
# handed to Geant4) they descend from; the leading primary of every kind is followed through the layers.
# The hit counts per subdetector, layer and kind are in OUTDIR/simHits.log.
# Run from an initialized CMSSW environment.
#   EVENT=1:1:1                            run:lumi:event drawn (default 1:1:1); EVENT="" draws the first EVENTS events
#   EVENTS=1                               events of the SIM sample to generate if the input is missing (default 1)
#   STAGE=Transport_B_Decay_EM_Had         SIM stage sample to draw (default the full physics; e.g. Transport_B_Decay_EM)
#   INPUT=SIM_Phase2/step2_SIM_<STAGE>_out.root  input file (default built from OUTDIR and STAGE)
#   OUTDIR=SIM_Phase2                      output directory (default SIM_Phase2, i.e. SIM_Phase2/TRK, ... as the book chapters)
#   STATIONS=TRK,ECAL                      subset of the stations (default all)
#   HIGHLIGHT=1                            followed primaries per kind (default 1; 0 = none)
#   HIGHLIGHT_PTMIN=2                      minimum pT of a followed primary in GeV (default 2)
#   PNG=0                                  no PNGs, only the PDFs
#   DOCS=0                                 do not copy the chapter texts
#   PRINTHITS=1                            list every primary with its hit counts per station in the log
set -euo pipefail

event=${EVENT-1:1:1}
events=${EVENTS:-1}
stage=${STAGE:-Transport_B_Decay_EM_Had}
outdir=${OUTDIR:-SIM_Phase2}
input=${INPUT:-$outdir/step2_SIM_${stage}_out.root}
stations=${STATIONS:-}
highlight=${HIGHLIGHT:-1}
highlight_ptmin=${HIGHLIGHT_PTMIN:-2}
png=${PNG:-1}
docs=${DOCS:-1}
printhits=${PRINTHITS:-0}
# Run4D110 RelVal settings, as in runCMSDrivers_mcPhase2_BBbar_SIM.sh
conditions=auto:phase2_realistic_T33
era=Phase2C17I13M9
geometry=ExtendedRun4D110

if [[ ${CMSSW_VERSION:-} != CMSSW_15_0_5 ]]; then
  echo "Initialize CMSSW_15_0_5 with 'eval \"\$(scram runtime -sh)\"' first." >&2
  exit 1
fi
journey=$CMSSW_BASE/src/JitJetSW/Journey
cfg=$journey/test/simHitAnalyzer_cfg.py
sim_script=$journey/test/runCMSDrivers_mcPhase2_BBbar_SIM.sh

plugin_lib=$CMSSW_BASE/lib/$SCRAM_ARCH/pluginJitJetSWJourneyAuto.so
if [[ ! -s $plugin_lib ]] || [[ -n $(find "$journey/plugins" -name '*.cc' -newer "$plugin_lib") ]]; then
  echo "Building JitJetSW/Journey"
  (cd "$CMSSW_BASE/src" && scram b -j 4 > jitjet_build.log 2>&1) || { cat "$CMSSW_BASE/src/jitjet_build.log" >&2; exit 1; }
fi

if [[ ! -s $input ]]; then
  if [[ -n ${INPUT:-} ]]; then
    echo "Missing input: $input" >&2; exit 1
  fi
  echo "Input $input missing: running runCMSDrivers_mcPhase2_BBbar_SIM.sh (stage $stage, $events event(s)) first"
  EVENTS=$events STAGES=$stage ANALYZE=0 OUTDIR=$outdir "$sim_script"
  [[ -s $input ]] || { echo "SIM script did not produce $input" >&2; exit 1; }
fi

mkdir -p "$outdir"
label="sample $stage|Phase2 D110, 14 TeV"   # '|' = new line
args=(inputFiles="file:$input" outputDir="$outdir" era="$era" globalTag="$conditions" geometry="$geometry"
      sampleLabel="$label" highlight="$highlight" highlightPtMin="$highlight_ptmin" savePNG="$png" printHits="$printhits")
if [[ -n $event ]]; then
  args+=(eventsToProcess="$event" maxEvents=-1)
else
  args+=(maxEvents="$events")
fi
[[ -n $stations ]] && args+=(stations="$stations")
echo "Drawing event ${event:-<first $events>} of $input into $outdir/<station>/"
cmsRun "$cfg" "${args[@]}" > "$outdir/simHits.log" 2>&1 || { tail -40 "$outdir/simHits.log" >&2; exit 1; }
grep -E "^SimHitAnalyzer run|^  station|^    primaries with hits" "$outdir/simHits.log"

if (( docs )); then
  for d in "$journey"/doc/sim/At*.md; do
    st=$(basename "$d" .md); st=${st#At}
    [[ -d $outdir/$st ]] && cp "$d" "$outdir/$st/"
  done
fi
echo "Pages per station:"
for d in "$outdir"/*/; do
  n=$(ls "$d"simHits_*.pdf 2>/dev/null | wc -l)
  (( n )) && echo "  $d: $(ls "$d" | tr '\n' ' ')"
done
