#!/usr/bin/env bash
# checkChapter.sh: compile one chapter of the book on its own (with the preamble of doc/JitJetSW.tex) and report what
# the Constitution asks for: the page count (5 +- 2, plus 2-3 for a hand-over section; 20-25 for the checkpoint
# chapters 12_L1T and 13_HLT of Rule G), the sections (the five of Rule B, plus at most one hand-over section "Made
# here for the next stage: ..." of Rule B6; for a checkpoint chapter B1, B2, one section per step, B4, B5), the
# overfull lines, the missing figures; render the pages as PNG to look at them.
#   doc/chapter/checkChapter.sh 01_ME            # -> doc/build/chapter_01_ME.pdf, chapter_01_ME_page-N.png, the report
#   doc/chapter/checkChapter.sh 01_ME nopng      # without the PNG rendering
set -euo pipefail
ch=$1
png=${2:-png}
doc=$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/..
build=$doc/build
mkdir -p "$build"
tex=$build/chapter_$ch.tex
# the preamble of the book up to \begin{document}, then the chapter alone
sed -n '1,/^\\begin{document}/p' "$doc/JitJetSW.tex" | sed 's#\\graphicspath{{../test/}}#\\graphicspath{{../../test/}}#' > "$tex"
cat >> "$tex" <<EOF
\setcounter{chapter}{$((10#${ch%%_*} - 1))}
\input{../chapter/chapter_$ch}
\end{document}
EOF
cd "$build"
for i in 1 2; do pdflatex -interaction=nonstopmode -halt-on-error "chapter_$ch.tex" > "chapter_$ch.build.log" 2>&1 || { grep -A3 '^!' "chapter_$ch.log" | head -40; exit 1; }; done
pages=$(pdfinfo "chapter_$ch.pdf" 2>/dev/null | awk '/^Pages:/ {print $2}')
checkpoint=0; [[ $ch == 12_L1T || $ch == 13_HLT ]] && checkpoint=1
if (( checkpoint )); then
  echo "== chapter $ch: $pages pages (a checkpoint chapter: the Constitution, Rule G, asks for 20-25)"
  echo "-- sections (Rule G: B1, B2, one section per step, B4, B5):"
else
  handover=$(grep -cE '^\\section\{Made here for the next stage' "$doc/chapter/chapter_$ch.tex" || true)
  if (( handover )); then
    echo "== chapter $ch: $pages pages (the Constitution asks for 5 +- 2, plus 2-3 for the hand-over section of Rule B6)"
    echo "-- sections (Rule B: five, plus the hand-over section):"
  else
    echo "== chapter $ch: $pages pages (the Constitution asks for 5 +- 2)"
    echo "-- sections (Rule B asks for exactly five):"
  fi
fi
{ grep -oE '^\\section\{[^}]*\}' "$doc/chapter/chapter_$ch.tex" || true; } | sed 's/^/   /'
n=$(grep -cE '^\\section\{' "$doc/chapter/chapter_$ch.tex" || true)
if (( checkpoint )); then
  steps=$(grep -cE '^\\section\{Step [0-9]' "$doc/chapter/chapter_$ch.tex" || true)
  (( n == steps + 4 )) || echo "   WARNING: $n sections, of which $steps step sections (expected B1, B2, the steps, B4, B5)"
else
  (( n == 5 + handover && handover <= 1 )) || echo "   WARNING: $n sections"
fi
echo "-- overfull / underfull lines:"
{ grep -cE '^(Overfull|Underfull)' "chapter_$ch.log" || true; } | sed 's/^/   /'
{ grep -E '^Overfull' "chapter_$ch.log" || true; } | head -5 | sed 's/^/   /'
echo "-- missing figures or files:"
{ grep -E 'File .* not found|LaTeX Warning: Reference|Citation' "chapter_$ch.log" || true; } | head -10 | sed 's/^/   /'
if [[ $png == png ]] && command -v pdftoppm > /dev/null; then
  rm -f "chapter_${ch}_page-"*.png
  pdftoppm -r 60 -png "chapter_$ch.pdf" "chapter_${ch}_page" 2> /dev/null
  echo "-- pages rendered: $(ls "chapter_${ch}_page-"*.png | tr '\n' ' ')"
fi
