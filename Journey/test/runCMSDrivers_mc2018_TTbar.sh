#!/usr/bin/env bash
# 2018 ttbar MC in CMSSW_15_0_5. Run from an initialized CMSSW environment.
set -euo pipefail

events=${EVENTS:-5}
first=${FIRST_STEP:-1}
last=${LAST_STEP:-9}
conditions=auto:phase1_2018_realistic
era=Run2_2018

if [[ ${CMSSW_VERSION:-} != CMSSW_15_0_5 ]]; then
  echo "Initialize CMSSW_15_0_5 with 'eval \"\$(scram runtime -sh)\"' first." >&2
  exit 1
fi

run_step() {
  local number=$1 name=$2 fragment=$3 sequence=$4 tier=$5 content=$6 input=$7
  local config="step${number}_${name}_cfg.py"
  local output="output_step${number}_${name}.root"
  local log="output_step${number}_${name}.log"
  local args=("$fragment" --conditions "$conditions" --era "$era" -n "$events"
              -s "$sequence" --datatier "$tier" --eventcontent "$content"
              --fileout "file:$output" --python_filename "$config" --no_exec)
  if [[ -n $input ]]; then
    [[ -s $input ]] || { echo "Missing input: $input" >&2; exit 1; }
    args+=(--filein "file:$input")
  fi
  if (( number == 1 )); then
    args+=(--beamspot Realistic25ns13TeVEarly2018Collision)
  fi
  if (( number == 4 )); then
    args+=(--customise_commands "process.load('Configuration.StandardSequences.Digi_cff')")
    args+=(--outputCommands 'keep *')
  fi
  if (( number == 5 )); then
    args+=(--customise_commands "process.DigiToRawTask.remove(process.castorRawData); process.rawDataCollector.RawCollectionList.remove(cms.InputTag('castorRawData'))")
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

if (( first <= 1 && last >= 1 )); then
  run_step 1 GEN TTbar_13TeV_TuneCUETP8M1_cfi GEN GEN FEVTDEBUG ''
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
  run_step 6 HLT step6 'HLT:@relval2018' GEN-SIM-DIGI-RAW-HLTDEBUG FEVTDEBUGHLT output_step5_DIGI2RAW.root
fi
if (( first <= 7 && last >= 7 )); then
  run_step 7 AODSIM step7 RAW2DIGI,L1Reco,RECO,RECOSIM AODSIM AODSIM output_step6_HLT.root
fi
if (( first <= 8 && last >= 8 )); then
  run_step 8 MINIAODSIM step8 PAT MINIAODSIM MINIAODSIM output_step7_AODSIM.root
fi
if (( first <= 9 && last >= 9 )); then
  run_step 9 NANOAODSIM step9 NANO NANOAODSIM NANOAODSIM output_step8_MINIAODSIM.root
fi
