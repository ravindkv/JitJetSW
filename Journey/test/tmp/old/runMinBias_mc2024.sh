#!/usr/bin/env bash
# The pileup library of the JitJet book for the 2024 (Run 3) detector, and nothing else: minimum-bias events
# (Pythia8 SoftQCD:inelastic, tune CP5, the release fragment MinBias_14TeV_pythia8_TuneCP5_cfi with comEnergy set
# to 13.6 TeV like the bbbar signal) through the 2024 GEN step (Run 3 vertex smearing from the database) and the
# 2024 SIM step (Geant4, full physics), with their own random seeds. The result is the GEN-SIM file the
# MixingModule of runPileup_mc2024_BBbar.sh overlays on the signal; this script is the "(b) pileup library" part
# of that script on its own, without the signal, the mixing and the SimHitAnalyzer pages, so that a library can
# be made (or remade, enlarged, reseeded) in isolation. The chapter is doc/pu/2024/PUMinBias.md (and PULib.tex).
#   step 1  cmsDriver.py MinBias_14TeV_pythia8_TuneCP5_cfi -s GEN --beamspot DBrealistic ...  -> step1_GEN_MinBias_out.root
#   step 2  cmsDriver.py step2 -s SIM --filein step1 ...                                      -> step2_SIM_MinBias_out.root  (the library)
# Output in OUTDIR (default PU_2024/, the directory runPileup_mc2024_BBbar.sh reads the library from):
#   step1_GEN_MinBias_cfg.py, .driver.log, _out.root, _out.log      GEN: HepMC, GenParticles, gen jets / MET of PUEVENTS events
#   step2_SIM_MinBias_cfg.py, .driver.log, _out.root, _out.log      SIM: + SimTracks, SimVertices, PSimHits, PCaloHits = the library
#   libcheck.log                                                    the four checks of CHECK=1 (conditions, seeds, products, size)
#   libstats.py, libstats.txt                                       with STATS=1: one line per event (process, multiplicities, vertex)
# Run from an initialized CMSSW environment (cd CMSSW_15_0_5/src; cmsenv).
#   PUEVENTS=250              number of minimum-bias events (must cover PU x bunch crossings per signal event with margin
#                             when the mixing reads the library in sequence: 60 x 3 = 180 for the defaults of the book)
#   PUSEED=20241              random seed base: generator, VtxSmeared and g4SimHits get PUSEED, PUSEED+1, PUSEED+2
#                             (cmsDriver's default seeds are those of the signal: library event k would get signal vertex k)
#   COMENERGY=13600.0         centre-of-mass energy written over the fragment's comEnergy (the fragment says 14000)
#   OUTDIR=PU_2024            output directory
#   REUSE=1                   keep an existing step1 / step2 file whatever PUEVENTS, PUSEED, COMENERGY say (REUSE=0 regenerates)
#   RUN=0                     only write the two cmsDriver configurations
#   SIM=0                     stop after the GEN step (the generator chapter doc/pu/2024/PUMinBias.md): no Geant4, the GEN
#                             checks only, and STATS=1 then runs minBiasGenStats.py on the GEN file
#   CHECK=1                   after the SIM step, run the checks a library must pass before it is mixed (CHECK=0 skips)
#   STATS=1                   catalogue every event with FWLite into libstats.txt (default 0; about a minute for 250 events);
#                             with SIM=0 the generator-level catalogue genstats.txt (minBiasGenStats.py) instead
#   CONDITIONS, ERA, GEOMETRY, BEAMSPOT, FRAGMENT   the 2024 RelVal settings (auto:phase1_2024_realistic, Run3_2024,
#                             DB:Extended, DBrealistic, MinBias_14TeV_pythia8_TuneCP5_cfi); change all of them together
#                             for another scenario, e.g. 2023: CONDITIONS=auto:phase1_2023_realistic ERA=Run3_2023
set -euo pipefail

pu_events=${PUEVENTS:-250}
pu_seed=${PUSEED:-20241}
com_energy=${COMENERGY:-13600.0}
outdir=${OUTDIR:-PU_2024}
reuse=${REUSE:-1}
run=${RUN:-1}
do_sim=${SIM:-1}
check=${CHECK:-1}
stats=${STATS:-0}
# 2024 RelVal settings, as in runCMSDrivers_mc2024_BBbar_SIM.sh and runPileup_mc2024_BBbar.sh
conditions=${CONDITIONS:-auto:phase1_2024_realistic}
era=${ERA:-Run3_2024}
geometry=${GEOMETRY:-DB:Extended}
beamspot=${BEAMSPOT:-DBrealistic}
fragment=${FRAGMENT:-MinBias_14TeV_pythia8_TuneCP5_cfi}   # Configuration/Generator/python/: SoftQCD:inelastic, CP5

if [[ -z ${CMSSW_BASE:-} ]] || ! command -v cmsDriver.py > /dev/null; then
  echo "Initialize CMSSW first: cd CMSSW_15_0_5/src && eval \"\$(scram runtime -sh)\"" >&2
  exit 1
fi
[[ ${CMSSW_VERSION:-} == CMSSW_15_0_5 ]] || echo "Warning: the book was made with CMSSW_15_0_5, this is ${CMSSW_VERSION:-?}" >&2

script_dir=$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")   # for minBiasGenStats.py, before cd
gen=step1_GEN_MinBias_out.root
sim=step2_SIM_MinBias_out.root
gen_cfg=step1_GEN_MinBias_cfg.py
sim_cfg=step2_SIM_MinBias_cfg.py

mkdir -p "$outdir"
cd "$outdir"
echo "Pileup library in $PWD: $pu_events MinBias events ($fragment at $com_energy GeV, seeds $pu_seed/$((pu_seed + 1))/$((pu_seed + 2)))"
echo "  conditions $conditions, era $era, geometry $geometry, beamspot $beamspot"

nevents() {  # nevents <file.root>: number of events, from the "(N runs, M lumis, K events, ...)" line of edmFileUtil
  edmFileUtil -e "file:$1" 2>/dev/null | sed -n 's/.*, \([0-9][0-9]*\) events,.*/\1/p' | head -1
}

run_driver() {  # run_driver <config> <log> <cmsDriver args...>: write the configuration, then run it unless RUN=0
  local config=$1 log=$2
  shift 2
  cmsDriver.py "$@" --python_filename "$config" --no_exec > "${config%.py}.driver.log" 2>&1 || { cat "${config%.py}.driver.log" >&2; exit 1; }
  echo "  Generated $config"
  (( run )) || return 0
  echo "  Running cmsRun $config ($(date '+%H:%M:%S'))"
  cmsRun "$config" > "$log" 2>&1 || { tail -80 "$log" >&2; exit 1; }
  echo "  Finished ($(date '+%H:%M:%S')), log $log"
}

# ---------------------------------------------------------------------------------------------- step 1: GEN
# The fragment's comEnergy (14 TeV) is overridden to that of the signal, and the generator and the vertex smearing
# get their own seeds: cmsDriver writes the same default seeds into every configuration, so without this library
# event k would be placed at the vertex (z and t) of signal event k.
if (( reuse )) && [[ -s $gen ]]; then
  echo "Step 1 (GEN): reusing $gen ($(nevents "$gen") events)"
else
  echo "Step 1 (GEN): $pu_events events"
  run_driver "$gen_cfg" step1_GEN_MinBias_out.log "$fragment" --conditions "$conditions" --era "$era" \
    --geometry "$geometry" --beamspot "$beamspot" -n "$pu_events" -s GEN --datatier GEN --eventcontent FEVTDEBUG \
    --fileout "file:$gen" \
    --customise_commands "process.generator.comEnergy = cms.double($com_energy)"'\n'"process.RandomNumberGeneratorService.generator.initialSeed = $pu_seed"'\n'"process.RandomNumberGeneratorService.VtxSmeared.initialSeed = $((pu_seed + 1))"
  (( run )) && { [[ -s $gen ]] || { echo "No output produced: $gen" >&2; exit 1; }; echo "  Completed $gen"; }
fi

# ---------------------------------------------------------------------------------------------- GEN only
if (( ! do_sim )); then
  (( run )) || { echo "RUN=0: GEN configuration written, nothing run."; exit 0; }
  if (( check )); then
    {
      echo "== conditions, era, geometry, vertex smearing and energy of the GEN step ($gen_cfg)"
      grep -E "^process = cms.Process|GlobalTag\(|GeometryRecoDB|VtxSmeared|^process\.generator\.comEnergy" "$gen_cfg" | grep -v "^#" || true
      echo "== random seeds (must differ from the cmsDriver defaults and from the signal)"
      grep -H initialSeed "$gen_cfg" || true
      echo "== events in the GEN file (edmFileUtil -e $gen)"
      echo "   $(nevents "$gen") events"
      echo "== Pythia8 process statistics (step1_GEN_MinBias_out.log)"
      grep -E "^ \| (non-diffractive|A B -> |sum )" step1_GEN_MinBias_out.log | tail -6 || true
      grep -E "total cross section|Filter efficiency \(event-level\)" step1_GEN_MinBias_out.log | head -2 || true
      echo "== generator time: first and last event of step1_GEN_MinBias_out.log"
      grep "^Begin processing" step1_GEN_MinBias_out.log | sed -n '1p;$p' || true
    } > gencheck.log 2>&1
    cat gencheck.log
  fi
  if (( stats )); then
    echo "Cataloguing the GEN file with minBiasGenStats.py into genstats.txt"
    python3 "$script_dir/minBiasGenStats.py" "$gen" > genstats.txt 2> genstats.log || { tail -20 genstats.log >&2; exit 1; }
    head -4 genstats.txt; echo "  ..."; sed -n '/^== /,$p' genstats.txt
  fi
  echo "GEN file: $PWD/$gen (SIM=0: no Geant4 step; run again without SIM=0 to make the library)"
  exit 0
fi

# ---------------------------------------------------------------------------------------------- step 2: SIM
# The same Geant4 as the signal (FTFP_BERT_EMM, 2024 geometry from the DB, full physics), own seed for g4SimHits.
if (( reuse )) && [[ -s $sim ]]; then
  echo "Step 2 (SIM): reusing $sim"
else
  echo "Step 2 (SIM): all events of $gen through Geant4 (about 2.5 s per event)"
  run_driver "$sim_cfg" step2_SIM_MinBias_out.log step2 --filein "file:$gen" --conditions "$conditions" \
    --era "$era" --geometry "$geometry" --beamspot "$beamspot" -n -1 -s SIM --datatier GEN-SIM --eventcontent FEVTDEBUG \
    --fileout "file:$sim" --customise_commands "process.RandomNumberGeneratorService.g4SimHits.initialSeed = $((pu_seed + 2))"
  (( run )) && { [[ -s $sim ]] || { echo "No output produced: $sim" >&2; exit 1; }; echo "  Completed $sim"; }
fi
(( run )) || { echo "RUN=0: configurations written, nothing run."; exit 0; }

# ---------------------------------------------------------------------------------------------- checks
# The four checks of the chapter: same conditions as the signal, own seeds, the products the mixing reads, enough events.
if (( check )); then
  {
    echo "== conditions, era, geometry, vertex smearing and energy of the GEN step ($gen_cfg)"
    grep -E "^process = cms.Process|GlobalTag\(|GeometryRecoDB|VtxSmeared|^process\.generator\.comEnergy" "$gen_cfg" | grep -v "^#" || true
    echo "== random seeds (must differ from the cmsDriver defaults and from the signal)"
    grep -H initialSeed "$gen_cfg" "$sim_cfg" || true
    echo "== products read by the MixingModule (edmDumpEventContent $sim)"
    edmDumpEventContent "file:$sim" 2>/dev/null | grep -E "SimTrack|SimVertex|HepMCProduct.*generatorSmeared|PSimHit|PCaloHit" | grep -v "^ *$" || true
    echo "== events in the library (edmFileUtil -e $sim)"
    n=$(nevents "$sim")
    echo "   $n events; the mixing of the book (PU=60, bx -1..1) needs 180 per signal event when it reads in sequence"
    echo "== Pythia8 process statistics (step1_GEN_MinBias_out.log)"
    grep -E "^ \| (non-diffractive|A B -> )" step1_GEN_MinBias_out.log | tail -5 || true
    grep -E "total cross section" step1_GEN_MinBias_out.log | head -1 || true
    echo "== Geant4 time: first and last event of step2_SIM_MinBias_out.log"
    grep "^Begin processing" step2_SIM_MinBias_out.log | sed -n '1p;$p' || true
  } > libcheck.log 2>&1
  cat libcheck.log
fi

# ---------------------------------------------------------------------------------------------- catalogue (FWLite)
# One line per event: process code (101 ND, 103/104 SD, 105 DD, 106 CD), stable particles, primaries handed to Geant4,
# SimTracks, tracker PSimHits, HF PCaloHits, vertex z and t (the catalogue of the chapter).
if (( stats )); then
  cat > libstats.py << 'EOF'
# libstats.py: one line per library event with its process, multiplicities and vertex (python3 libstats.py gen.root sim.root)
import sys
from DataFormats.FWLite import Events, Handle
gen, sim = sys.argv[1], sys.argv[2]
hHep = Handle('edm::HepMCProduct'); hTrk = Handle('std::vector<SimTrack>')
hPS = Handle('std::vector<PSimHit>'); hPC = Handle('std::vector<PCaloHit>')
subdets = ('PixelBarrel', 'PixelEndcap', 'TIB', 'TID', 'TOB', 'TEC')
tracker = ['TrackerHits' + d + t for d in subdets for t in ('LowTof', 'HighTof')]
procs = {101: 'ND', 103: 'SD', 104: 'SD', 105: 'DD', 106: 'CD'}
geninfo = {}
for e in Events(gen):
    e.getByLabel('generatorSmeared', hHep); ev = hHep.product().GetEvent()
    nstable = 0; it = ev.particles_begin()
    while it != ev.particles_end():
        if it.__deref__().status() == 1: nstable += 1
        it.__preinc__()
    v = ev.vertices_begin().__deref__().position()                       # HepMC units: mm and mm/c
    pid = ev.signal_process_id()
    geninfo[e.eventAuxiliary().event()] = (procs.get(pid, str(pid)), nstable, v.z() / 10., v.t() / 299.792458)
print('%5s %4s %6s %5s %6s %6s %6s %8s %8s' % ('event', 'proc', 'stable', 'prim', 'SimTrk', 'TRKhit', 'HFhit', 'z[cm]', 't[ns]'))
for e in Events(sim):
    e.getByLabel('g4SimHits', hTrk); trks = hTrk.product()
    nprim = sum(1 for t in trks if t.genpartIndex() > 0)                 # SimTracks with a generator link = primaries
    nhits = 0
    for c in tracker:
        e.getByLabel(('g4SimHits', c), hPS); nhits += hPS.product().size()
    e.getByLabel(('g4SimHits', 'HcalHits'), hPC)
    nhf = sum(1 for h in hPC.product() if (h.id() >> 28) & 0xF == 4)   # test numbering: subdet in bits 28-31, 4 = HF
    k = e.eventAuxiliary().event()
    proc, nstable, z, t = geninfo.get(k, ('?', 0, 0., 0.))
    print('%5d %4s %6d %5d %6d %6d %6d %8.2f %8.3f' % (k, proc, nstable, nprim, trks.size(), nhits, nhf, z, t))
EOF
  echo "Cataloguing the library with FWLite into libstats.txt"
  python3 libstats.py "$gen" "$sim" > libstats.txt 2> libstats.log || { tail -20 libstats.log >&2; exit 1; }
  head -6 libstats.txt; echo "  ... $(( $(wc -l < libstats.txt) - 1 )) events in libstats.txt"
fi

echo "Library: $PWD/$sim"
echo "Next: CASES=mixed REUSE=1 ./runPileup_mc2024_BBbar.sh mixes it into the signal; CASES=pileup draws one of its events."
