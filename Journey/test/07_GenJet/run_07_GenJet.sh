#!/usr/bin/env bash
# Chapter 07, GenJet: the generator jets. The anti-kT clustering (fastjet, R = 0.4 and 0.8, with and without neutrinos)
# runs on the status 1 particles of the full generator event as a separate step 2 of the journey (process GENJET,
# JitJetSW/Journey/python/genJet_cff.py scheduled by cmsDriver.py as a USER step), with the jet pT threshold JETPTMIN.
# Input (Rule E): test/06_Decay/out/step1_GEN_06_Decay.root. The jets the GEN step itself clustered (pgen contains
# genJetMET) are dropped from the output, so this file holds exactly one set of jets, the ones of this chapter.
# Output in test/07_GenJet/out/:
#   step2_GENJET_07_GenJet.root, .log, .driver.log    the GENJET step (config: test/07_GenJet/step2_GENJET_07_GenJet_cfg.py)
#   genJetAnalyzer_07_GenJet_AK4.pdf, _AK4NoNu.pdf, _AK8.pdf
#                                                     one one-page PDF per jet collection of JETS (ak4GenJets -> AK4, ...)
#   genJetAnalyzer_07_GenJet.root, .log               the jets of all collections, and the generator stages 1-5
#                                                     (GenPartAnalyzer: analysed and printed, not drawn; pages of 01-06)
#   ledger_07_GenJet.txt                              the energy ledger with the jets (common/genLedger.py --jets)
# Variables: JETPTMIN=3 (GeV), JETS=ak4GenJets,ak4GenJetsNoNu,ak8GenJets (collections drawn), REGEN=0 keeps the existing
# step2_GENJET_07_GenJet.root (read by chapters 08 and 09) and redoes only the analysis.
set -euo pipefail
CHAPTER=07_GenJet
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
JETPTMIN=${JETPTMIN:-3}
JETS=${JETS:-ak4GenJets,ak4GenJetsNoNu,ak8GenJets}
in=$(jitjet_prev 06_Decay step1_GEN_06_Decay.root)
cfg=$CHDIR/step2_GENJET_${CHAPTER}_cfg.py
out=step2_GENJET_${CHAPTER}.root
echo "anti-kT R = 0.4 / 0.8 (fastjet) on the status 1 particles of $(basename "$in"), jet pT > $JETPTMIN GeV"
if (( ${REGEN:-1} )) || [[ ! -s $OUT/$out ]]; then
  run_driver "$cfg" "$OUT/step2_GENJET_${CHAPTER}.log" step2 --filein "file:$in" --conditions "$CONDITIONS" --era "$ERA" \
    --geometry "$GEOMETRY" -n -1 -s USER:JitJetSW/Journey/genJet_cff.genJetSequence --processName GENJET --datatier GEN \
    --eventcontent FEVTDEBUG --fileout "file:$out" --outputCommands "drop *_ak4GenJets*_*_GEN,drop *_ak8GenJets*_*_GEN" \
    --customise_commands "for m in ('ak4GenJets', 'ak8GenJets', 'ak4GenJetsNoNu', 'ak8GenJetsNoNu'): getattr(process, m).jetPtMin = $JETPTMIN"
else
  echo "  REGEN=0: keeping $out ($(date -r "$OUT/$out" '+%Y-%m-%d %H:%M')), only the analysis is redone"
fi
(( RUN )) || exit 0
[[ -s $OUT/$out ]] || { echo "No output produced: $OUT/$out" >&2; exit 1; }
(( ANALYZE )) || exit 0
a=genJetAnalyzer_${CHAPTER}
# one PDF per collection: ak4GenJets -> ${a}_AK4.pdf, ak4GenJetsNoNu -> ${a}_AK4NoNu.pdf, ak8GenJets -> ${a}_AK8.pdf
pdfs=()
for j in ${JETS//,/ }; do
  t=${j/GenJets/}
  pdfs+=("${a}_AK${t#ak}.pdf")
done
rm -f "$OUT/$a.pdf" "$OUT/$a"_page-*.png  # the former 8-page PDF (5 generator stages + 3 jet pages) and its PNG pages
label="chapter $CHAPTER: generator jets|anti-kT R = 0.4 / 0.8 on the status 1 particles|jet pT > $JETPTMIN GeV|bbbar pTHat > $PTHATMIN GeV, 13.6 TeV"
run_cmsrun "$COMMON/genJetAnalyzer_cfg.py" "$OUT/$a.log" inputFiles="file:$OUT/$out" outputFile="$a.root" maxEvents=1 \
  eventsToProcess="$EVENT" maxStage=5 jets="$JETS" jetPdfFiles="$(IFS=,; echo "${pdfs[*]}")" sampleLabel="$label"
grep -E '^== GenJetAnalyzer|^  ak|^    (b|bbar) quark|^    net' "$OUT/$a.log"
for p in "${pdfs[@]}"; do
  pdf_pages_png "$OUT/$p"
done
python3 "$COMMON/genLedger.py" "$OUT/$out" --event "$EVENT" --jets "$JETS" > "$OUT/ledger_${CHAPTER}.txt" 2> "$OUT/ledger_${CHAPTER}.log" ||
  { tail -20 "$OUT/ledger_${CHAPTER}.log" >&2; exit 1; }
echo "  Ledger: ledger_${CHAPTER}.txt"
