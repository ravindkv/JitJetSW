#!/usr/bin/env bash
# Pileup chapter of the JitJet book for the Phase2 detector: what the mixing of pileup does to the SimHits of
# one event, shown on three samples drawn with the same SimHitAnalyzer pages as runSimHitAnalyzer_mcPhase2_BBbar.sh
# (per station TRK, ECAL, HCAL, HGCAL, MUON, ALL: r-z view, x-y view, eta-phi view):
#   (a)   signal           the bbbar event through Geant4: the full-physics SIM stage of
#                          runCMSDrivers_mcPhase2_BBbar_SIM.sh (SIM_Phase2/step2_SIM_Transport_B_Decay_EM_Had_out.root,
#                          generated if missing)
#   (b)   pileup           minimum-bias events (Pythia8 SoftQCD inelastic at 14 TeV, tune CP5, the release fragment
#                          MinBias_14TeV_pythia8_TuneCP5_cfi) through the same GEN step (HL-LHC vertex smearing) and
#                          the same SIM step (Geant4, full physics): the pileup library of PUEVENTS events. One of
#                          its events (PUEVENT) is drawn.
#   (a+b) signal + pileup  the DIGI step with pileup mixing (cmsDriver -s DIGI:pdigi_valid --pileup ... --pileup_input):
#                          for every signal event the MixingModule takes a Poisson(PU) number of library events for
#                          every bunch crossing of the window BUNCHES, stamps them with their bunch crossing and event
#                          number, shifts their hit times by 25 ns x bx and hands the signal hits and the pileup hits
#                          together (CrossingFrame collections) to the digitisers. SimHitAnalyzer runs inside this job
#                          (the CrossingFrames are transient, not written) and draws the signal hits coloured by primary
#                          kind as in (a) with the pileup hits underneath, coloured by bunch crossing (early, in-time,
#                          late); a calorimeter cell takes the colour of the largest energy share.
# Output in OUTDIR (default PU_Phase2/):
#   step1_GEN_MinBias_cfg.py, _out.root, _out.log          (b) GEN of the pileup library
#   step2_SIM_MinBias_cfg.py, _out.root, _out.log          (b) SIM of the pileup library
#   step3_DIGI_PU<PU>_cfg.py, _out.root, _out.log          (a+b) mixing + digitisation (the DIGI file is written too)
#   signal/<station>/simHits_<station>_run<R>_event<E>.pdf, .png   (a), with signal/simHits.log
#   pileup/<station>/...                                   (b), with pileup/simHits.log
#   mixed/<station>/...                                    (a+b), with mixed/simHits.log (the analyzer lines of the DIGI log)
#   AtPileup.md                                            the chapter text (copied from doc/sim/, DOCS=1)
# Run from an initialized CMSSW environment.
#   EVENT=1:1:1                            signal event drawn and mixed (default 1:1:1)
#   PU=10                                  average number of pileup events per bunch crossing (Poisson; HL-LHC: 200)
#   BUNCHES=-1,1                           bunch-crossing window min,max (default -1,1; the Phase2 production uses -3,3)
#   PUEVENTS=50                            size of the pileup library generated in (b) (default 50; must cover
#                                          PU x number of bunch crossings with margin, the events are used in sequence)
#   PUEVENT=1                              library event drawn in (b) (default 1)
#   PUSEED=20261                           random seed base of the library (generator, VtxSmeared, g4SimHits get PUSEED, +1, +2);
#                                          with cmsDriver's default seeds library event k would have the vertex of signal event k
#   SIGNAL_INPUT=SIM_Phase2/step2_SIM_Transport_B_Decay_EM_Had_out.root   signal SIM file (default; generated if missing)
#   OUTDIR=PU_Phase2                       output directory
#   STATIONS=TRK,ECAL                      subset of the stations (default all)
#   HIGHLIGHT=1 HIGHLIGHT_PTMIN=2 PNG=0 PRINTHITS=1 DOCS=0   as in runSimHitAnalyzer_mcPhase2_BBbar.sh
#   CASES="signal pileup mixed"            subset of the cases (default all three)
#   REUSE=1                                keep existing step1/step2/step3 ROOT files (default 1; REUSE=0 regenerates)
#   RUN=0                                  only generate the cmsDriver configs
set -euo pipefail

event=${EVENT:-1:1:1}
pu=${PU:-10}
bunches=${BUNCHES:--1,1}
pu_events=${PUEVENTS:-50}
pu_event=${PUEVENT:-1}
signal_input=${SIGNAL_INPUT:-SIM_Phase2/step2_SIM_Transport_B_Decay_EM_Had_out.root}
outdir=${OUTDIR:-PU_Phase2}
stations=${STATIONS:-}
highlight=${HIGHLIGHT:-1}
highlight_ptmin=${HIGHLIGHT_PTMIN:-2}
png=${PNG:-1}
printhits=${PRINTHITS:-0}
docs=${DOCS:-1}
cases=${CASES:-"signal pileup mixed"}
reuse=${REUSE:-1}
run=${RUN:-1}
# Run4D110 RelVal settings, as in runCMSDrivers_mcPhase2_BBbar_SIM.sh
conditions=auto:phase2_realistic_T33
era=Phase2C17I13M9
geometry=ExtendedRun4D110
beamspot=DBrealisticHLLHC
pu_fragment=MinBias_14TeV_pythia8_TuneCP5_cfi   # Configuration/Generator/python/, 14 TeV, SoftQCD:inelastic, CP5
pu_seed=${PUSEED:-20261}                         # random seeds of the library (generator, vertex smearing, Geant4): cmsDriver's
                                                 # defaults are those of the signal, so library event k would get the signal's vertex k
bx_min=${bunches%,*}
bx_max=${bunches#*,}

if [[ ${CMSSW_VERSION:-} != CMSSW_15_0_5 ]]; then
  echo "Initialize CMSSW_15_0_5 with 'eval \"\$(scram runtime -sh)\"' first." >&2
  exit 1
fi
journey=$CMSSW_BASE/src/JitJetSW/Journey
cfg=$journey/test/simHitAnalyzer_cfg.py
sim_script=$journey/test/runCMSDrivers_mcPhase2_BBbar_SIM.sh

plugin_lib=$CMSSW_BASE/lib/$SCRAM_ARCH/pluginJitJetSWJourneyAuto.so
if [[ ! -s $plugin_lib ]] || [[ -n $(find "$journey/plugins" -name '*.cc' -newer "$plugin_lib") ]] ||
   ! python3 -c 'import JitJetSW.Journey.simHitAnalyzer_cff' 2>/dev/null; then
  echo "Building JitJetSW/Journey"
  (cd "$CMSSW_BASE/src" && scram b -j 4 > jitjet_build.log 2>&1) || { cat "$CMSSW_BASE/src/jitjet_build.log" >&2; exit 1; }
fi

# (a) the signal: the full-physics SIM stage of the Phase2 SIM script
if [[ ! -s $signal_input ]]; then
  if [[ -n ${SIGNAL_INPUT:-} ]] || (( ! run )); then
    echo "Missing signal input: $signal_input" >&2; exit 1
  fi
  echo "Signal input $signal_input missing: running runCMSDrivers_mcPhase2_BBbar_SIM.sh (stage Transport_B_Decay_EM_Had) first"
  STAGES=Transport_B_Decay_EM_Had ANALYZE=0 OUTDIR="$(dirname "$signal_input")" "$sim_script"
  [[ -s $signal_input ]] || { echo "SIM script did not produce $signal_input" >&2; exit 1; }
fi
signal_input=$(readlink -f "$signal_input")

mkdir -p "$outdir"
cd "$outdir"
echo "Writing to $PWD (signal event $event of $signal_input; pileup PU=$pu per bx, bunch crossings $bx_min..$bx_max, library of $pu_events MinBias events)"

# common analyzer arguments
analyzer_args=(era="$era" globalTag="$conditions" geometry="$geometry" highlight="$highlight" highlightPtMin="$highlight_ptmin"
               savePNG="$png" printHits="$printhits" maxEvents=-1)
[[ -n $stations ]] && analyzer_args+=(stations="$stations")
# python keyword arguments of addSimHitAnalyzer for the mixing job (same settings)
py_bool() { (( $1 )) && echo True || echo False; }
analyzer_kw="highlight=$highlight, highlightPtMin=$highlight_ptmin, savePNG=$(py_bool "$png"), printHits=$(py_bool "$printhits")"
[[ -n $stations ]] && analyzer_kw+=", stations='$stations'"

run_driver() {  # run_driver <config> <log> <cmsDriver args...>
  local config=$1 log=$2
  shift 2
  cmsDriver.py "$@" --python_filename "$config" --no_exec > "${config%.py}.driver.log" 2>&1 || { cat "${config%.py}.driver.log" >&2; exit 1; }
  echo "  Generated $config"
  (( run )) || return 0
  cmsRun "$config" > "$log" 2>&1 || { tail -80 "$log" >&2; exit 1; }
}

draw() {  # draw <case> <input> <event> <label>: SimHitAnalyzer on a SIM file into <case>/<station>/
  local case=$1 input=$2 ev=$3 label=$4
  mkdir -p "$case"
  echo "  Drawing event $ev of $input into $case/<station>/"
  cmsRun "$cfg" inputFiles="file:$input" outputDir="$case" eventsToProcess="$ev" sampleLabel="$label" "${analyzer_args[@]}" \
    > "$case/simHits.log" 2>&1 || { tail -40 "$case/simHits.log" >&2; exit 1; }
  grep -E "^SimHitAnalyzer run|^  station|^    primaries with hits" "$case/simHits.log"
}

# ---------------------------------------------------------------------------------------------- (a) signal
if [[ " $cases " == *" signal "* ]] && (( run )); then
  echo "(a) signal: $signal_input"
  draw signal "$signal_input" "$event" "sample (a) signal|bbbar, pTHat > 100 GeV|Phase2 D110, 14 TeV|no pileup"
fi

# ---------------------------------------------------------------------------------------------- (b) pileup library
pu_gen=step1_GEN_MinBias_out.root
pu_sim=step2_SIM_MinBias_out.root
if [[ " $cases " == *" pileup "* || " $cases " == *" mixed "* ]]; then
  echo "(b) pileup library: $pu_events MinBias events ($pu_fragment) through GEN and SIM"
  if (( reuse )) && [[ -s $pu_sim ]]; then
    echo "  Reusing $pu_sim"
  else
    if (( reuse )) && [[ -s $pu_gen ]]; then
      echo "  Reusing $pu_gen"
    else
      run_driver step1_GEN_MinBias_cfg.py step1_GEN_MinBias_out.log "$pu_fragment" --conditions "$conditions" --era "$era" \
        --geometry "$geometry" --beamspot "$beamspot" -n "$pu_events" -s GEN --datatier GEN --eventcontent FEVTDEBUG \
        --fileout "file:$pu_gen" --customise_commands "process.RandomNumberGeneratorService.generator.initialSeed = $pu_seed"'\n'"process.RandomNumberGeneratorService.VtxSmeared.initialSeed = $((pu_seed + 1))"
      (( run )) && { [[ -s $pu_gen ]] || { echo "No output produced: $pu_gen" >&2; exit 1; }; echo "  Completed $pu_gen"; }
    fi
    run_driver step2_SIM_MinBias_cfg.py step2_SIM_MinBias_out.log step2 --filein "file:$pu_gen" --conditions "$conditions" \
      --era "$era" --geometry "$geometry" --beamspot "$beamspot" -n -1 -s SIM --datatier GEN-SIM --eventcontent FEVTDEBUG \
      --fileout "file:$pu_sim" --customise_commands "process.RandomNumberGeneratorService.g4SimHits.initialSeed = $((pu_seed + 2))"
    (( run )) && { [[ -s $pu_sim ]] || { echo "No output produced: $pu_sim" >&2; exit 1; }; echo "  Completed $pu_sim"; }
  fi
fi
if [[ " $cases " == *" pileup "* ]] && (( run )); then
  draw pileup "$PWD/$pu_sim" "1:1:$pu_event" "sample (b) pileup|MinBias event $pu_event of the library ($pu_events)|SoftQCD inelastic, 14 TeV|Phase2 D110, no signal"
fi

# ---------------------------------------------------------------------------------------------- (a+b) mixing
if [[ " $cases " == *" mixed "* ]]; then
  digi=step3_DIGI_PU${pu}_out.root
  echo "(a+b) signal + pileup: DIGI step with the MixingModule, PU=$pu (Poisson) in bunch crossings $bx_min..$bx_max, library $pu_sim"
  label="sample (a+b) signal + pileup|PU = $pu per bx (Poisson), bx $bx_min..$bx_max|MinBias library in sequence|Phase2 D110, 14 TeV"
  customise=(
    "process.mix.input.sequential = cms.untracked.bool(True)"                 # library events in order, no event twice
    "process.source.eventsToProcess = cms.untracked.VEventRange('$event')"
    "from JitJetSW.Journey.simHitAnalyzer_cff import addSimHitAnalyzer; process = addSimHitAnalyzer(process, outputDir='mixed', sampleLabel='$label', $analyzer_kw)"
  )
  joined=${customise[0]}
  for c in "${customise[@]:1}"; do joined+='\n'"$c"; done   # cmsDriver splits --customise_commands on a literal backslash-n
  mkdir -p mixed
  run_driver "step3_DIGI_PU${pu}_cfg.py" "step3_DIGI_PU${pu}_out.log" step3 --filein "file:$signal_input" \
    --pileup "AVE_200_BX_25ns,{'N': $pu, 'B': ($bx_min, $bx_max)}" --pileup_input "file:$PWD/$pu_sim" \
    --conditions "$conditions" --era "$era" --geometry "$geometry" -n -1 -s DIGI:pdigi_valid --datatier GEN-SIM-DIGI \
    --eventcontent FEVTDEBUG --fileout "file:$digi" --customise_commands "$joined"
  if (( run )); then
    [[ -s $digi ]] || { echo "No output produced: $digi" >&2; exit 1; }
    echo "  Completed $digi"
    grep -E "^SimHitAnalyzer|^  pileup mixed|^    pileup event|^  station|^    hits by|^    primaries with hits" "step3_DIGI_PU${pu}_out.log" > mixed/simHits.log
    grep -E "^SimHitAnalyzer run|^  pileup mixed|^  station|^    primaries with hits" mixed/simHits.log
  fi
fi

if (( docs )) && [[ -s $journey/doc/sim/AtPileup.md ]]; then
  cp "$journey/doc/sim/AtPileup.md" .
fi
echo "Pages per case and station:"
for c in signal pileup mixed; do
  for d in "$c"/*/; do
    [[ -d $d ]] || continue
    n=$(ls "$d"simHits_*.pdf 2>/dev/null | wc -l)
    (( n )) && echo "  $d: $(ls "$d" | tr '\n' ' ')"
  done
done
