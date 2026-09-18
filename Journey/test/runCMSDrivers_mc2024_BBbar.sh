#!/usr/bin/env bash
# Summer24-like bbbar MC (Run3 2024 detector conditions) in CMSSW_15_0_5.
# GEN: Pythia8 HardQCD gg/qqbar->bbbar (tune CP5) from the local fragment
#   JitJetSW/Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py (no bbbar
#   fragment ships with the release). cmsDriver expects a GEN fragment under
#   Configuration/GenProduction/python, so the script symlinks it there and runs
#   'scram b python' once if the module is not importable.
# Run from an initialized CMSSW environment.
# Settings follow the official 2024 RelVal chain (runTheMatrix.py -w upgrade -l 12834.0):
#   era Run3_2024, GT auto:phase1_2024_realistic, beamspot DBrealistic,
#   geometry DB:Extended. The com energy is set to 13.6 TeV to match Summer24.
# HLT: the RelVal key @relval2024 resolves to the HLT_Fake2 menu (no real trigger
#   paths), and the frozen 2024 menu (2024v14) is not shipped in CMSSW_15_0_5.
#   We therefore run the real GRun menu shipped with this release (HLT_GRun_cff,
#   /dev/CMSSW_15_0_0/GRun/V62). GRun seeds on 2025 L1 algorithms (e.g. L1_CICADA_*)
#   that are absent from the 2024 L1 menu in the 2024 GT, so the L1 menu record is
#   overridden with the 2025 L1 menu (taken from auto:phase1_2025_realistic) in the
#   L1 emulation step and in the HLT step. Everything else stays 2024 conditions.
# DUMP: per-event text dumps of every object at every step (JitJetSW/Journey package),
#   0 (default) off, 1 every physics object + links to the previous step, 2 + Pythia8/HepMC
#   listings and all L1 bits, 3 + Geant4 track/step printout and list of every product.
#   DUMP_MAX / DUMP_SUB cap elements / sub-elements per collection (-1 = all),
#   DUMP_G4_THRESHOLD is the Geant4 track Ekin threshold (GeV) for DUMP=3.
#   The dumps go to the step logs output_stepN_*.log; with DUMP>0 the NANO tree is also
#   printed to output_step9_NANOAODSIM.tree.log. See JitJetSW/Journey/README.md.
set -euo pipefail

events=${EVENTS:-5}
first=${FIRST_STEP:-1}
last=${LAST_STEP:-9}
conditions=auto:phase1_2024_realistic
era=Run3_2024
geometry=DB:Extended
beamspot=DBrealistic
com_energy=13600.0
l1_menu_override="L1Menu_Collisions2025_v1_0_0_xml,L1TUtmTriggerMenuRcd"
dump=${DUMP:-0}
dump_max=${DUMP_MAX:--1}
dump_sub=${DUMP_SUB:--1}
dump_g4_threshold=${DUMP_G4_THRESHOLD:-1.0}
dump_customise="from JitJetSW.Journey.jitJet_cff import customiseJitJet; process = customiseJitJet(process, level=${dump}, maxElements=${dump_max}, maxSub=${dump_sub}, g4Threshold=${dump_g4_threshold})"

if [[ ${CMSSW_VERSION:-} != CMSSW_15_0_5 ]]; then
  echo "Initialize CMSSW_15_0_5 with 'eval \"\$(scram runtime -sh)\"' first." >&2
  exit 1
fi

run_step() {
  local number=$1 name=$2 fragment=$3 sequence=$4 tier=$5 content=$6 input=$7
  local config="step${number}_${name}_cfg.py"
  local output="output_step${number}_${name}.root"
  local log="output_step${number}_${name}.log"
  local args=("$fragment" --conditions "$conditions" --era "$era" --geometry "$geometry"
              -n "$events" -s "$sequence" --datatier "$tier" --eventcontent "$content"
              --fileout "file:$output" --python_filename "$config" --no_exec)
  local -a customise=()
  if [[ -n $input ]]; then
    [[ -s $input ]] || { echo "Missing input: $input" >&2; exit 1; }
    args+=(--filein "file:$input")
  fi
  if (( number == 1 )); then
    args+=(--beamspot "$beamspot")
    customise+=("process.generator.comEnergy = cms.double($com_energy)")
  fi
  if (( number == 3 )); then
    # Run3 DIGI2RAW packs the PPS pixel digis (RPixDetDigitizer). Nothing in a
    # standalone DIGI job consumes them and FEVTDEBUGHLT does not keep them, so the
    # digitizer would never run; keep them explicitly so step 5 can find them.
    args+=(--outputCommands 'keep *_RPixDetDigitizer_*_*')
  fi
  if (( number == 4 )); then
    customise+=("process.load('Configuration.StandardSequences.Digi_cff')")
    args+=(--outputCommands 'keep *')
  fi
  if (( number == 4 || number == 6 )); then
    # L1 menu must match the HLT menu (see header note).
    args+=(--custom_conditions "$l1_menu_override")
  fi
  if (( dump > 0 )); then
    customise+=("$dump_customise")
  fi
  if (( ${#customise[@]} > 0 )); then
    # cmsDriver splits --customise_commands on a literal backslash-n
    local joined=${customise[0]} c
    for c in "${customise[@]:1}"; do joined+='\n'"$c"; done
    args+=(--customise_commands "$joined")
  fi
  echo "Running step $number/9: $name"
  cmsDriver.py "${args[@]}" > "${config%.py}.driver.log" 2>&1 || {
    cat "${config%.py}.driver.log" >&2; exit 1;
  }
  cmsRun "$config" > "$log" 2>&1 || { tail -80 "$log" >&2; exit 1; }
  [[ -s $output ]] || { echo "No output produced: $output" >&2; exit 1; }
  echo "Completed $output"
}

(( first >= 1 && first <= last && last <= 9 )) || { echo 'Invalid FIRST_STEP/LAST_STEP' >&2; exit 1; }

if (( dump > 0 )); then
  [[ -d $CMSSW_BASE/src/JitJetSW/Journey ]] || { echo "Missing package JitJetSW/Journey (needed for DUMP>0)" >&2; exit 1; }
  if [[ ! -s $CMSSW_BASE/lib/$SCRAM_ARCH/pluginJitJetSWJourneyAuto.so ]] ||
     ! python3 -c 'import JitJetSW.Journey.jitJet_cff' 2>/dev/null; then
    echo "Building JitJetSW/Journey"
    # full build (not only the package) so that the edm plugin cache is refreshed
    (cd "$CMSSW_BASE/src" && scram b -j 4 > jitjet_build.log 2>&1) || { cat "$CMSSW_BASE/src/jitjet_build.log" >&2; exit 1; }
  fi
fi

# The fragment lives in this package so that it is version controlled; cmsDriver
# resolves it as the python module Configuration.GenProduction.<name>, so it is
# symlinked into the standard location (see header note).
gen_fragment=Configuration/GenProduction/python/BBbar_14TeV_TuneCP5_cfi.py
gen_fragment_src=$CMSSW_BASE/src/JitJetSW/$gen_fragment
if (( first <= 1 && last >= 1 )); then
  [[ -s $gen_fragment_src ]] || { echo "Missing GEN fragment: $gen_fragment_src" >&2; exit 1; }
  if [[ ! -e $CMSSW_BASE/src/$gen_fragment ]]; then
    mkdir -p "$CMSSW_BASE/src/${gen_fragment%/*}"
    ln -s "$gen_fragment_src" "$CMSSW_BASE/src/$gen_fragment"
  fi
  python3 -c 'import Configuration.GenProduction.BBbar_14TeV_TuneCP5_cfi' 2>/dev/null ||
    (cd "$CMSSW_BASE/src/Configuration/GenProduction" && scram b python > /dev/null)
  run_step 1 GEN "$gen_fragment" GEN GEN FEVTDEBUG ''
fi
if (( first <= 2 && last >= 2 )); then
  run_step 2 SIM step2 SIM GEN-SIM FEVTDEBUG output_step1_GEN.root
fi
if (( first <= 3 && last >= 3 )); then
  run_step 3 DIGI step3 DIGI:pdigi_valid GEN-SIM-DIGI FEVTDEBUGHLT output_step2_SIM.root
fi
if (( first <= 4 && last >= 4 )); then
  run_step 4 L1 step4 L1 GEN-SIM-DIGI FEVTDEBUGHLT output_step3_DIGI.root
fi
if (( first <= 5 && last >= 5 )); then
  run_step 5 DIGI2RAW step5 DIGI2RAW GEN-SIM-DIGI-RAW FEVTDEBUGHLT output_step4_L1.root
fi
if (( first <= 6 && last >= 6 )); then
  run_step 6 HLT step6 'HLT:GRun' GEN-SIM-DIGI-RAW-HLTDEBUG FEVTDEBUGHLT output_step5_DIGI2RAW.root
fi
if (( first <= 7 && last >= 7 )); then
  run_step 7 AODSIM step7 RAW2DIGI,L1Reco,RECO,RECOSIM AODSIM AODSIM output_step6_HLT.root
fi
if (( first <= 8 && last >= 8 )); then
  run_step 8 MINIAODSIM step8 PAT MINIAODSIM MINIAODSIM output_step7_AODSIM.root
fi
if (( first <= 9 && last >= 9 )); then
  run_step 9 NANOAODSIM step9 NANO NANOAODSIM NANOAODSIM output_step8_MINIAODSIM.root
  if (( dump > 0 )); then
    echo "Dumping NANO tree to output_step9_NANOAODSIM.tree.log"
    python3 "$CMSSW_BASE/src/JitJetSW/Journey/scripts/dumpNanoTree.py" output_step9_NANOAODSIM.root \
      > output_step9_NANOAODSIM.tree.log 2>&1 || { tail -20 output_step9_NANOAODSIM.tree.log >&2; exit 1; }
  fi
fi
