#!/usr/bin/env bash
# Chapter 08, PU: pileup at generator level. Three samples:
#   (8a) the signal: the bbbar event with its jets, the output of chapter 07 (nothing is rerun)
#   (8b) the pileup: PUEVENTS minimum-bias collisions (Pythia8 SoftQCD:inelastic, tune CP5, the release fragment
#        MinBias_14TeV_pythia8_TuneCP5_cfi with comEnergy set to 13.6 TeV like the signal) through the GEN step of 2025
#        (YEAR=2025 of jitjet_common.sh; Run-3 vertex smearing from the database, the same beam-spot payload as the 2024
#        global tag of the signal in chapter 07) with their own random seeds (PUSEED, +1): the generator-level pileup
#        library of the book. One collision, library event SKIP+1 (the first in-time one of the mixing), is drawn.
#   (8a+8b) the overlay: the stable particles of the NPU in-time collisions (library events SKIP+1 .. SKIP+NPU, the ones
#        the MixingModule puts into bunch crossing 0 of this signal event in chapter 10, replayed in 11) are added to the signal
#        final state with the MixingModule restricted to the HepMC record (python/genJetPileup_cff.py), and the anti-kT
#        clustering of chapter 07 is run again on the sum: what pileup does to the jets before any detector.
# Input (Rule E): test/07_GenJet/out/step2_GENJET_07_GenJet.root (signal; its jets alone are the pages of chapter 07).
# Output in test/08_PU/out/:
#   step1_GEN_MinBias_08_PU.root, .log, .driver.log   (8b) the library at generator level (config: step1_GEN_MinBias_08_PU_cfg.py)
#   gencheck_08_PU.log, genstats_08_PU.txt            (8b) the checks and the catalogue of the library (common/minBiasGenStats.py)
#   genPartAnalyzer_MinBias_08_PU.pdf, .root, .log    (8b) the five generator-stage pages of one minimum-bias collision
#   step2_GENJETPU_08_PU.root                         (8a+8b) the merged genParticlesPU (collisionId 0 = signal) and the PU jets
#   genJetPileup_08_PU.root, .log                     (8a+8b) the jets of the overlay (histograms, printout)
#   genJetAnalyzer_08_PU.pdf                          (8a+8b) the overlay, one page per jet collection in JETS (signal circles,
#                                                     pileup triangles)
#   ledger_08_PU.txt                                  the ledger of the overlay: signal / pileup / sum (common/genLedger.py --pu)
# Variables: PUEVENTS=250 PUSEED=20241 NPU=68 SKIP=71 (jitjet_common.sh) PUTYPE=fixed JETS=ak4GenJetsPU JETPTMIN=3 LABELPTMIN=30
# REGEN=0 keeps the existing library step1_GEN_MinBias_08_PU.root (read by chapter 09) and redoes only the analysis.
set -euo pipefail
CHAPTER=08_PU
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../common/jitjet_common.sh"
jitjet_init
jitjet_build
PUEVENTS=${PUEVENTS:-250}
PUSEED=${PUSEED:-20241}
# NPU, SKIP: the in-time collisions of the mixing of chapter 10 for this signal event (jitjet_common.sh, jitjet_init)
PUTYPE=${PUTYPE:-fixed}
JETS=${JETS:-ak4GenJetsPU}
JETPTMIN=${JETPTMIN:-3}
LABELPTMIN=${LABELPTMIN:-30}
FRAGMENT=MinBias_14TeV_pythia8_TuneCP5_cfi
signal=$(jitjet_prev 07_GenJet step2_GENJET_07_GenJet.root)

# ------------------------------------------------------------------------------------------ (8b) the minimum-bias collisions
lib=step1_GEN_MinBias_${CHAPTER}.root
cfg=$CHDIR/step1_GEN_MinBias_${CHAPTER}_cfg.py
echo "(8b) $PUEVENTS minimum-bias collisions: $FRAGMENT at $COMENERGY GeV, seeds $PUSEED / $((PUSEED + 1))"
if (( ${REGEN:-1} )) || [[ ! -s $OUT/$lib ]]; then
  run_driver "$cfg" "$OUT/step1_GEN_MinBias_${CHAPTER}.log" "$FRAGMENT" --conditions "$CONDITIONS" --era "$ERA" --geometry "$GEOMETRY" \
    --beamspot "$BEAMSPOT" -n "$PUEVENTS" -s GEN --datatier GEN --eventcontent FEVTDEBUG --fileout "file:$lib" \
    --customise_commands "process.generator.comEnergy = cms.double($COMENERGY)"'\n'"process.RandomNumberGeneratorService.generator.initialSeed = $PUSEED"'\n'"process.RandomNumberGeneratorService.VtxSmeared.initialSeed = $((PUSEED + 1))"
else
  echo "  REGEN=0: keeping $lib ($(date -r "$OUT/$lib" '+%Y-%m-%d %H:%M')), only the analysis is redone"
fi
(( RUN )) || exit 0
[[ -s $OUT/$lib ]] || { echo "No output produced: $OUT/$lib" >&2; exit 1; }
{
  echo "== conditions, era, geometry, vertex smearing and energy of the GEN step ($(basename "$cfg"))"
  grep -E "^process = cms.Process|GlobalTag\(|GeometryRecoDB|VtxSmeared|^process\.generator\.comEnergy" "$cfg" | grep -v "^#" || true
  echo "== random seeds (must differ from the cmsDriver defaults, which the signal uses)"
  grep -H initialSeed "$cfg" || true
  echo "== events in the file"
  edmFileUtil -e "file:$OUT/$lib" 2>/dev/null | grep -oE "[0-9]+ events" | head -1
  echo "== Pythia8 process statistics (step1_GEN_MinBias_${CHAPTER}.log)"
  grep -E "^ \| (non-diffractive|A B -> |sum )" "$OUT/step1_GEN_MinBias_${CHAPTER}.log" | tail -6 || true
  grep -E "total cross section" "$OUT/step1_GEN_MinBias_${CHAPTER}.log" | head -1 || true
} > "$OUT/gencheck_${CHAPTER}.log" 2>&1
(( ANALYZE )) || exit 0
echo "  Cataloguing the collisions with minBiasGenStats.py"
python3 "$COMMON/minBiasGenStats.py" "$OUT/$lib" > "$OUT/genstats_${CHAPTER}.txt" 2> "$OUT/genstats_${CHAPTER}.log" || { tail -20 "$OUT/genstats_${CHAPTER}.log" >&2; exit 1; }
sed -n '/^== /,$p' "$OUT/genstats_${CHAPTER}.txt" | head -12
a=genPartAnalyzer_MinBias_${CHAPTER}
run_cmsrun "$COMMON/genPartAnalyzer_cfg.py" "$OUT/$a.log" inputFiles="file:$OUT/$lib" outputFile="$a.root" maxEvents=1 \
  eventsToProcess="1:1:$((SKIP + 1))" maxStage=5 pdfFile="$a.pdf" \
  sampleLabel="chapter $CHAPTER (8b): one minimum-bias collision|library event $((SKIP + 1)) of $PUEVENTS|SoftQCD inelastic, CP5, 13.6 TeV|no hard process"
pdf_pages_png "$OUT/$a.pdf"

# ------------------------------------------------------------------------------------------ (8a+8b) the overlay
prefix=genJetPileup_${CHAPTER}
book=genJetAnalyzer_${CHAPTER}.pdf  # the overlay page(s), the PDF of the chapter
edm=step2_GENJETPU_${CHAPTER}.root
rm -f "$OUT/$prefix.pdf"  # before 14 Nov 2026 the overlay went to $prefix.pdf and was appended to the 8 pages of chapter 07
if [[ $PUTYPE == fixed ]]; then pu_text="$NPU minimum-bias collisions in time"; else pu_text="Poisson($NPU) minimum-bias collisions in time"; fi
label="chapter $CHAPTER (8a+8b): signal + pileup|+ $pu_text|library events $((SKIP + 1))-$((SKIP + NPU)) (bx 0)|bbbar pTHat > $PTHATMIN GeV, 13.6 TeV|jet pT > $JETPTMIN GeV, drawn > $LABELPTMIN GeV"
echo "(8a+8b) signal event $EVENT + $pu_text (library events $((SKIP + 1))-$((SKIP + NPU))), anti-kT on the sum: $JETS"
run_cmsrun "$COMMON/genJetPileup_cfg.py" "$OUT/$prefix.log" inputFiles="file:$signal" pileupFiles="file:$OUT/$lib" eventsToProcess="$EVENT" \
  nPileup="$NPU" skipPileup="$SKIP" pileupType="$PUTYPE" pileupSeed=20243 jetPtMin="$JETPTMIN" labelPtMin="$LABELPTMIN" jets="$JETS" \
  outputFile="$prefix.root" edmOutput="$edm" pdfFile="$book" sampleLabel="$label" maxEvents=-1
grep -E "^== GenJetAnalyzer|^  ak|^    (b|bbar) quark|^    net" "$OUT/$prefix.log"
pdf_pages_png "$OUT/$book"
python3 "$COMMON/genLedger.py" "$OUT/$edm" --event "$EVENT" --pu --jets "$JETS" > "$OUT/ledger_${CHAPTER}.txt" 2> "$OUT/ledger_${CHAPTER}.log" ||
  { tail -20 "$OUT/ledger_${CHAPTER}.log" >&2; exit 1; }
echo "  Ledger: ledger_${CHAPTER}.txt"
