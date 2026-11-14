#!/usr/bin/env bash
# jitjet_common.sh: the machinery shared by the chapter scripts test/<chapter>/run_<chapter>.sh (Rule F of the
# Constitution: common scripts live in test/ to avoid duplication). Every chapter script sets CHAPTER and sources it:
#
#   CHAPTER=01_ME
#   source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
#   jitjet_init; jitjet_build
#
# Functions
#   jitjet_init                 checks CMSSW_15_0_5, sets JOURNEY, TEST, COMMON, CHDIR (= test/<chapter>), OUT (= test/<chapter>/out)
#                               and the Run-3 settings of the chapter (YEAR: 2025 or 2024); creates OUT
#   jitjet_build                rebuilds the JitJetSW/Journey plugins when the plugin library is older than the sources
#   jitjet_fragment             symlinks the bbbar fragment into Configuration/GenProduction, where cmsDriver.py looks for it
#   jitjet_prev <chapter> <file>  prints test/<chapter>/out/<file> (the output of a previous chapter, Rule E) and checks it exists
#   run_driver <cfg> <log> <cmsDriver args...>  writes <cfg> with cmsDriver.py (its log next to <log> as <cfg>.driver.log),
#                               then runs cmsRun <cfg> in OUT with the output in <log>, unless RUN=0
#   run_cmsrun <cfg> <log> [key=value ...]      runs an analyzer config (cmsRun, in OUT) with VarParsing arguments
#   pdf_pages_png <pdf> [dpi]   every page of a ROOT PDF as <pdf minus .pdf>_page-N.png (pdftoppm -cropbox), for the book
#   gen_stage <isr> <fsr> <mpi> <had> <decay> <maxStage> <description>   the generator chapters 01, 02, 04, 05, 06
#   gen_xsec <isr> <fsr> <mpi> <had> <decay>   the cross section from XSEC_EVENTS events with the settings of gen_stage
#   sim_ld_library_path         LD_LIBRARY_PATH without the simulation biglib (a physics list built in this package cannot be
#                               mixed with the statically linked Geant4 of biglib/pluginSimulation.so)
# Variables understood by every chapter script
#   RUN=0          only write the cmsDriver configurations, run nothing
#   ANALYZE=0      skip the analyzers (plots, ledgers)
#   EVENTS=1       events generated / simulated (the book follows ONE event; more only for illustration)
#   EVENT=1:1:1    the event of the book (run:lumi:event), the one drawn and mixed
#   PNG=1          convert the PDF pages to PNG (pdftoppm). Default 0 for chapters 01-08, whose .tex include the PDFs
#                  themselves, 1 for 09-13
#   CONE_R=0.4     gen_stage: the GenPartAnalyzer pages draw the cone Delta R < CONE_R around the b and the bbar, with its
#                  pT and a zoom on the fuller cone (default 0: no cones; the ledger always lists the cones of R = 0.4)
#   DRAW_STAGES=2  gen_stage: the GenPartAnalyzer PDF gets only the pages of these stages (comma list; default: every
#                  stage up to maxStage); the other stages are still analysed and printed in the log
#   XSEC_EVENTS=1000, XSEC_FILE=1   gen_xsec: the number of events, and keep them in out/xsec_GEN_<chapter>.root
#   REGEN=0        gen_stage: keep the existing out/step1_GEN_<chapter>.root (do not run the GEN job again) and redo only
#                  the analyzer pages and the ledger, e.g. for a chapter whose GEN file a later chapter reads (06 -> 07);
#                  run_07_GenJet.sh and run_08_PU.sh honour it too (step2_GENJET_07_GenJet.root, the minimum-bias library)
#   YEAR           2025 for the CMS chapters 08-13, 2024 for the generator chapters 01-07 (the defaults of CONDITIONS, ERA)
#   CONDITIONS, ERA, GEOMETRY, BEAMSPOT, COMENERGY, PTHATMIN   the Run-3 settings (change all together for another scenario)

jitjet_init() {
  if [[ ${CMSSW_VERSION:-} != CMSSW_15_0_5 ]]; then
    echo "Initialize CMSSW_15_0_5 first: cd CMSSW_15_0_5/src && cmsenv" >&2
    exit 1
  fi
  [[ -n ${CHAPTER:-} ]] || { echo "CHAPTER is not set" >&2; exit 1; }
  JOURNEY=$CMSSW_BASE/src/JitJetSW/Journey
  TEST=$JOURNEY/test
  COMMON=$TEST/common
  CHDIR=$TEST/$CHAPTER
  OUT=$CHDIR/out
  mkdir -p "$OUT"
  # the Run-3 RelVal settings of the chapter (upgradeWorkflowComponents.py, keys '2024' and '2025'). CMSSW_15_0_5 is a
  # release of 2025, so the CMS chapters 08-13 (detector, conditions) use YEAR=2025: the global tag
  # auto:phase1_2025_realistic (150X_mcRun3_2025_realistic_v1) and the era Run3_2025 (Rule I of the Constitution).
  # The generator chapters 01-07 keep 2024: in a GEN job the conditions only give the beam spot of the vertex
  # smearing, the same SimBeamSpotObjects payload in both global tags.
  case $CHAPTER in
    0[1-7]_*) YEAR=${YEAR:-2024} ;;
    *)        YEAR=${YEAR:-2025} ;;
  esac
  CONDITIONS=${CONDITIONS:-auto:phase1_${YEAR}_realistic}
  ERA=${ERA:-Run3_${YEAR}}
  GEOMETRY=${GEOMETRY:-DB:Extended}
  BEAMSPOT=${BEAMSPOT:-DBrealistic}
  COMENERGY=${COMENERGY:-13600.0}
  PTHATMIN=${PTHATMIN:-100}
  # the pileup of the book (chapters 08 to 11): PU collisions per bunch crossing on average, in bunch crossings
  # BUNCHES, the library read in sequence. The MixingModule of chapter 10 (seed 12345, Poisson(PU) per bunch crossing)
  # draws for this signal event SKIP collisions in bx -1 (library events 1..SKIP) and NPU in bx 0 (library events
  # SKIP+1..SKIP+NPU): chapters 08 and 09 use these NPU in-time collisions, chapter 10 checks the draw against them
  # (WARNING_intime_10_SIM.txt if they differ) and chapter 11 replays the same draw (MixingModule playback).
  PU=${PU:-60}
  BUNCHES=${BUNCHES:--1,1}
  SKIP=${SKIP:-71}
  NPU=${NPU:-68}
  EVENTS=${EVENTS:-1}
  EVENT=${EVENT:-1:1:1}
  RUN=${RUN:-1}
  ANALYZE=${ANALYZE:-1}
  case $CHAPTER in  # the .tex of chapters 01-08 include the analyzer PDFs, no PNG needed (14 November 2026)
    0[1-8]_*) PNG=${PNG:-0} ;;
    *)        PNG=${PNG:-1} ;;
  esac
  PLUGIN_LIB=$CMSSW_BASE/lib/$SCRAM_ARCH/pluginJitJetSWJourneyAuto.so
  GEN_FRAGMENT=Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py
  CMSRUN_PREFIX=${CMSRUN_PREFIX:-}
  echo "== Chapter $CHAPTER ($(date '+%Y-%m-%d %H:%M:%S')): scripts in $CHDIR, output in $OUT"
}

jitjet_build() {
  [[ -d $JOURNEY ]] || { echo "Missing package JitJetSW/Journey" >&2; exit 1; }
  if [[ ! -s $PLUGIN_LIB ]] || [[ -n $(find "$JOURNEY/plugins" -name '*.cc' -newer "$PLUGIN_LIB") ]] ||
     ! python3 -c 'import JitJetSW.Journey.simHitAnalyzer_cff, JitJetSW.Journey.genJet_cff, JitJetSW.Journey.genJetPileup_cff, JitJetSW.Journey.mixOnly_cff' 2>/dev/null; then
    echo "Building JitJetSW/Journey (full build, so that the edm plugin cache is refreshed)"
    (cd "$CMSSW_BASE/src" && scram b -j 4 > jitjet_build.log 2>&1) || { cat "$CMSSW_BASE/src/jitjet_build.log" >&2; exit 1; }
  fi
}

jitjet_fragment() {
  # cmsDriver.py resolves the fragment as the python module Configuration.GenProduction.<name>: symlink ours there
  local src=$CMSSW_BASE/src/JitJetSW/$GEN_FRAGMENT
  [[ -s $src ]] || { echo "Missing GEN fragment: $src" >&2; exit 1; }
  if [[ ! -e $CMSSW_BASE/src/$GEN_FRAGMENT ]]; then
    mkdir -p "$CMSSW_BASE/src/${GEN_FRAGMENT%/*}"
    ln -s "$src" "$CMSSW_BASE/src/$GEN_FRAGMENT"
  fi
  python3 -c 'import Configuration.GenProduction.BBbar_14TeV_TuneCP5_cfi' 2>/dev/null ||
    (cd "$CMSSW_BASE/src/Configuration/GenProduction" && scram b python > /dev/null)
}

jitjet_prev() {  # jitjet_prev <chapter> <file>
  local f=$TEST/$1/out/$2
  if [[ ! -s $f ]]; then
    echo "Missing input $f: run test/$1/run_$1.sh first (Rule E: the output of chapter $1 is the input of chapter $CHAPTER)" >&2
    exit 1
  fi
  readlink -f "$f"
}

run_driver() {  # run_driver <cfg> <log> <cmsDriver args...>
  local cfg=$1 log=$2
  shift 2
  local dlog=$OUT/$(basename "${cfg%.py}").driver.log
  cmsDriver.py "$@" --python_filename "$cfg" --no_exec > "$dlog" 2>&1 || { cat "$dlog" >&2; exit 1; }
  echo "  Generated $(basename "$cfg")"
  (( RUN )) || return 0
  echo "  Running cmsRun $(basename "$cfg") ($(date '+%H:%M:%S'))"
  (cd "$OUT" && $CMSRUN_PREFIX cmsRun "$cfg" > "$log" 2>&1) || { tail -80 "$log" >&2; exit 1; }
  echo "  Finished ($(date '+%H:%M:%S')), log $(basename "$log")"
}

run_cmsrun() {  # run_cmsrun <cfg> <log> [VarParsing key=value ...]
  local cfg=$1 log=$2
  shift 2
  echo "  Running cmsRun $(basename "$cfg") $* ($(date '+%H:%M:%S'))"
  (cd "$OUT" && cmsRun "$cfg" "$@" > "$log" 2>&1) || { tail -40 "$log" >&2; exit 1; }
  echo "  Finished ($(date '+%H:%M:%S')), log $(basename "$log")"
}

pdf_pages_png() {  # pdf_pages_png <pdf> [dpi]: ROOT PDFs carry an A4 MediaBox and a CropBox around the canvas, hence -cropbox
  (( PNG )) || return 0
  local pdf=$1 dpi=${2:-110}
  [[ -s $pdf ]] || { echo "No PDF to convert: $pdf" >&2; return 0; }
  command -v pdftoppm > /dev/null || { echo "  pdftoppm not found, no PNG pages" >&2; return 0; }
  rm -f "${pdf%.pdf}"_page-*.png
  pdftoppm -cropbox -r "$dpi" -png "$pdf" "${pdf%.pdf}_page" 2> /dev/null
  echo "  PNG pages: $(ls "${pdf%.pdf}"_page-*.png 2>/dev/null | wc -l) of $(basename "$pdf")"
}

sim_ld_library_path() {
  echo "$LD_LIBRARY_PATH" | tr ':' '\n' | grep -v '/biglib/' | paste -sd:
}

# the generator chapters: the ONE bbbar event with the Pythia8 steps switched on one at a time. Same fragment, seeds,
# conditions, era, geometry, beamspot and energy in every chapter, so that the hard process is the same event.
gen_customise() {  # gen_customise <isr> <fsr> <mpi> <had> <decay>: the --customise_commands of the generator chapters
  local isr=$1 fsr=$2 mpi=$3 had=$4 decay=$5
  local joined="process.generator.comEnergy = cms.double($COMENERGY)"
  joined+='\n'"process.generator.PythiaParameters.processParameters.extend(['PhaseSpace:pTHatMin = $PTHATMIN', 'PartonLevel:ISR = $isr', 'PartonLevel:FSR = $fsr', 'PartonLevel:MPI = $mpi', 'HadronLevel:all = $had', 'HadronLevel:Hadronize = $had', 'HadronLevel:Decay = $decay'])"
  printf '%s' "$joined"
}

gen_stage() {  # gen_stage <isr> <fsr> <mpi> <had> <decay> <maxStage> <description>
  local isr=$1 fsr=$2 mpi=$3 had=$4 decay=$5 max_stage=$6 desc=$7
  jitjet_fragment
  local cfg=$CHDIR/step1_GEN_${CHAPTER}_cfg.py out=step1_GEN_${CHAPTER}.root log=$OUT/step1_GEN_${CHAPTER}.log
  local joined
  joined=$(gen_customise "$isr" "$fsr" "$mpi" "$had" "$decay")
  echo "Pythia8: ISR=$isr FSR=$fsr MPI=$mpi Hadronize=$had Decay=$decay (pTHatMin $PTHATMIN GeV, $COMENERGY GeV, $EVENTS event(s))"
  if (( ${REGEN:-1} )) || [[ ! -s $OUT/$out ]]; then
    run_driver "$cfg" "$log" "$GEN_FRAGMENT" --conditions "$CONDITIONS" --era "$ERA" --geometry "$GEOMETRY" --beamspot "$BEAMSPOT" \
      -n "$EVENTS" -s GEN --datatier GEN --eventcontent FEVTDEBUG --fileout "file:$out" --customise_commands "$joined"
  else
    echo "  REGEN=0: keeping $out ($(date -r "$OUT/$out" '+%Y-%m-%d %H:%M')), only the analysis is redone"
  fi
  (( RUN )) || return 0
  [[ -s $OUT/$out ]] || { echo "No output produced: $OUT/$out" >&2; exit 1; }
  (( ANALYZE )) || return 0
  GEN_MAX_STAGE=$max_stage
  GEN_LABEL="chapter $CHAPTER: $desc|ISR $isr, FSR $fsr, MPI $mpi|Hadronize $had, Decay $decay|pTHatMin $PTHATMIN GeV, 13.6 TeV"
  gen_part_pages "genPartAnalyzer_${CHAPTER}" "${DRAW_STAGES:-}"
  python3 "$COMMON/genLedger.py" "$OUT/$out" --event "$EVENT" > "$OUT/ledger_${CHAPTER}.txt" 2> "$OUT/ledger_${CHAPTER}.log" ||
    { tail -20 "$OUT/ledger_${CHAPTER}.log" >&2; exit 1; }
  echo "  Ledger: $(basename "$OUT/ledger_${CHAPTER}.txt")"
}

# the GenPartAnalyzer pages of out/step1_GEN_<chapter>.root: out/<name>.pdf/.root/.log (+ PNG pages if PNG=1). Every stage up to
# GEN_MAX_STAGE is analysed (log), only <drawStages> (comma list; empty: all) get a page; GEN_MAX_STAGE and GEN_LABEL
# are set by gen_stage, so a chapter can call it again after gen_stage for an extra PDF (04_MPI: the shower page).
gen_part_pages() {  # gen_part_pages <name> [drawStages]
  local a=$1 draw=${2:-}
  local opt=()
  [[ -n $draw ]] && opt=(drawStages="$draw")
  run_cmsrun "$COMMON/genPartAnalyzer_cfg.py" "$OUT/$a.log" inputFiles="file:$OUT/step1_GEN_${CHAPTER}.root" outputFile="$a.root" \
    maxEvents=1 eventsToProcess="$EVENT" maxStage="$GEN_MAX_STAGE" "${opt[@]}" pdfFile="$a.pdf" sampleLabel="$GEN_LABEL" coneR="${CONE_R:-0}"
  grep -E '^== GenPartAnalyzer|^  stage|^  by scattering|^  cone' "$OUT/$a.log"
  pdf_pages_png "$OUT/$a.pdf"
}

# the cross section of a generator chapter. Pythia8 estimates sigma from the events it generated, with the statistical
# error sigma/sqrt(N accepted): after the ONE event of the book it is sigma +- sigma. This second GEN job, with the
# same fragment, settings and seeds as gen_stage, generates XSEC_EVENTS events (default 1000) without writing them
# (cmsDriver --no_output) and keeps only the log, with the PYTHIA statistics and the GenXsecAnalyzer summary; with
# XSEC_FILE=1 the events are kept in out/xsec_GEN_<chapter>.root, for statistics over many events (genShowerStats.py).
# Its first event is the event of the book (same seeds); the output of gen_stage is not touched.
gen_xsec() {  # gen_xsec <isr> <fsr> <mpi> <had> <decay>
  local n=${XSEC_EVENTS:-1000}
  jitjet_fragment
  local cfg=$CHDIR/xsec_GEN_${CHAPTER}_cfg.py log=$OUT/xsec_GEN_${CHAPTER}.log out=(--no_output) what="no output file"
  if (( ${XSEC_FILE:-0} )); then
    out=(--fileout "file:xsec_GEN_${CHAPTER}.root")
    what="events kept in xsec_GEN_${CHAPTER}.root"
  fi
  echo "Cross section: $n events, same settings and seeds, $what"
  run_driver "$cfg" "$log" "$GEN_FRAGMENT" --conditions "$CONDITIONS" --era "$ERA" --geometry "$GEOMETRY" --beamspot "$BEAMSPOT" \
    -n "$n" -s GEN --datatier GEN --eventcontent FEVTDEBUG "${out[@]}" --customise_commands "$(gen_customise "$@")"
  (( RUN )) || return 0
  grep -E 'total cross section|final cross section' "$log" | sed 's/^/  /'
}
