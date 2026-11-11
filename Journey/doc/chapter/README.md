# Writing a chapter of the JitJet book

The rules are `Journey/Constitution.txt`; this file is the practical companion for whoever writes or extends a chapter
(a student, or a tool). One chapter = one stage of the journey of the ONE bbbar event (run 1, lumi 1, event 1).

## Files (Rule A)

| file | content |
|---|---|
| `test/<XX_YYY>/run_<XX_YYY>.sh` | the script of the chapter; `cmsDriver.py` configurations are written next to it as `*_cfg.py` |
| `test/<XX_YYY>/out/` | everything the script produces: `*.root`, `*.log`, `*.driver.log`, `*.pdf`, `*_page-N.png`, `ledger_<XX_YYY>.txt` |
| `doc/chapter/chapter_<XX_YYY>.tex` | the book chapter (physics), `\input` by `doc/JitJetSW.tex`; no preamble, starts with `\chapter{...}` |
| `doc/chapter/chapter_<XX_YYY>.md` | the technical companion, not compiled into the book |

The chapters are `01_ME 02_PS 03_MnM 04_MPI 05_Had 06_Decay 07_GenJet 08_PU 09_G4 10_SIM 11_DIGI 12_L1T`; the output of chapter N
is the input of chapter N+1 (Rule E), and `test/runAll.sh` runs them in order. Common configurations and helpers live in
`test/common/` (Rule F): `jitjet_common.sh` (sourced by every script), the analyzer configurations
(`genPartAnalyzer_cfg.py`, `genJetAnalyzer_cfg.py`, `genJetPileup_cfg.py`, `simAnalyzer_cfg.py`, `simHitAnalyzer_cfg.py`,
`digiAnalyzer_cfg.py`, `trigPrimAnalyzer_cfg.py`, `l1tAnalyzer_cfg.py`) and the ledger scripts (`genLedger.py`, `g4Ledger.py`,
`simLedger.py`, `digiLedger.py`, `digiThresholds.py`, `trigPrimLedger.py`, `minBiasGenStats.py`, `l1tLedger.py`, `l1tSettings.py`,
`l1tTruth.py`).

Signal and pileup (Rule H): up to `11_DIGI` a chapter may follow three samples (the signal alone, one pileup collision
alone, the signal with its pileup). `11_DIGI` is the last stage at which they exist apart; from `12_L1T` on there is one
sample, the mixed event, and no `<case>` in the file names. Whatever a later chapter needs to know about the signal
alone is computed in chapter 11 and quoted from there.

## The .tex chapter (Rule B): exactly five sections, 5 +- 2 pages

```latex
\chapter{XX -- Name: one line that says what happens to the event here}
\label{ch:XX_YYY}
\section{What this chapter is about}          % B1: 1-2 paragraphs
\section{From the previous chapter}            % B2: 1-2 paragraphs, what comes in, what is kept, what changes
\section{The numbers of the event}             % B3: the numbers that explain the physics (tables, a TikZ histogram if useful)
\section{Ledger and display}                   % B4: the ledger table (energy / particles added or removed) + the eta-phi display
\section{Open problems for Phase-2}            % B5: after digging in CMSSW: cuts / thresholds / calibrations not updated since Run 2/3
```

- Written for a student who has read the previous chapters: explain every CMSSW or Pythia word the first time; one
  `plainwords` box per key idea (`\begin{plainwords}[Title] ... \end{plainwords}`, keep it under half a page, it cannot break).
- Every number comes from `test/<XX_YYY>/out/` (logs, `ledger_*.txt`, `genstats_*.txt`, `thresholds_*.txt`), never from
  memory; the companion lists them in full. Use the macros of `doc/JitJetSW.tex`: `\pt`, `\GeV`, `\TeV`, `\code{}`, `\bbbar`,
  `\ptHat`, `\abseta`; long file names go in `\path{...}` or `\code{...}` with escaped underscores.
- Figures: the PNG pages of the chapter, `\includegraphics[width=\textwidth]{<XX_YYY>/out/<prefix>_page-N.png}`
  (`\graphicspath` is `../test/`); the SimHitAnalyzer pages are `<XX_YYY>/out/<case>/<station>/<prefix>_<station>_<view>_run1_event1.png`.
  Never include the ROOT PDFs directly (A4 MediaBox, rotated). One or two figures per chapter, the eta-phi display in B4.
- The ledger table of B4 has one row per quantity the stage changed (particles, energy, pT, by region / kind), with the
  value before and after the stage and the difference, in `booktabs` style.
- B5 is the result of a real look into the release (`$CMSSW_RELEASE_BASE/src/<package>/python/*.py`, the `_cfi.py` of
  the module in question, the global-tag payloads named in the logs): quote the parameter, its value, where it is set,
  since when, and why it matters for the HL-LHC (PU 200, 14 TeV, the new detectors). No speculation without a file name.
- No `siunitx`, `tcolorbox`, `pgfplots` (TeX Live 2020 on lxplus): plain TikZ with `\histbar{bin}{height}` for histograms.
- Check the page count and the overfull lines after `pdflatex JitJetSW.tex` (twice), or for one chapter alone with
  `doc/chapter/checkChapter.sh <XX_YYY>` (pages in `doc/build/`), and render the pages with
  `pdftoppm -r 60 -png JitJet.pdf check` to look at tables and figures.

## The hand-over section (Rule B6): at most one, 2-3 pages more

When a stage makes something that only a later stage uses, it is explained where it is made, in one extra section
between B3 and B4:

```latex
\section{Made here for the next stage: ...}    % B6: what it is, how it is made, why it is made HERE, the numbers of the event
\label{sec:XX_...}                             % the later chapter refers back to it
```

Chapter 11 is the model: the calorimeter `TriggerPrimitiveDigis` (`simEcalTriggerPrimitiveDigis`,
`simHcalTriggerPrimitiveDigis`) are made by the DIGI job and used by the Level-1 trigger of chapter 12
(`plugins/TrigPrimAnalyzer.cc`, `test/common/trigPrimAnalyzer_cfg.py`, `trigPrimLedger.py` ->
`out/trigprim_11_DIGI.txt`). The section title must start with "Made here for the next stage"; `checkChapter.sh`
then accepts six sections and 2-3 more pages. The later chapter recalls the result in a few lines and refers back.

## A checkpoint chapter (Rule G): 12_L1T, 13_HLT, 20-25 pages

A stage that decides whether the event is kept, instead of changing its energy, is written step by step:

```latex
\chapter{XX -- Name: ...}
\section{What this chapter is about}          % B1, with a diagram of the chain and a table of the steps at a glance
\section{From the previous chapter}            % B2
\section{Step 1: ...}                          % one section per major step, 2-3 pages each (titles start with "Step N:")
\section{Step 2: ...}
...
\section{Ledger and display}                   % B4: the energy the trigger sees step by step + the decision (what fired, by which margin)
\section{Open problems for Phase-2}            % B5
```

- Every step section has the same four parts: **What the step does** (the hardware or software in plain words, the
  module that emulates it), **The parameters** (where they are: python, global-tag payload), **The numbers of the event**
  (one sample, the mixed event: Rule H), and a display of the event *after* the step. Who made an object (the signal, the
  pileup, noise) is read from the truth in the file: the generator jets, the `TrackingParticle`s behind a muon.
  Chapter 12 is the model: `plugins/L1TCaloAnalyzer.cc`, `L1TMuonAnalyzer.cc`, `L1TGlobalAnalyzer.cc` (one analyzer per
  group of steps, one PDF each, the pages as PNG; shared layout in `plugins/L1TDisplay.h`).
- The pages have a text panel on the right: include them at full width (`width=0.93\textwidth`) when the panel matters,
  or crop the panel away for a side-by-side pair: `\includegraphics[width=0.49\textwidth,trim=0 0 475 0,clip]{...}`
  (the PNG is 1396 pixels wide, the panel is the right 34 %).
- `\FloatBarrier` before the sections where a figure must not drift into the next step.
- The ledger of B4 has two parts that never mix: energy seen (nothing is added to or removed from the recorded event)
  and the decision. Say which of the fired algorithms the signal objects satisfy on their own and which need an object
  the hard collision did not make: that is what the pileup changes in the decision.

## The .md companion (Rule C): exactly four sections

```markdown
# Chapter XX, Name: technical companion
## 1. The cmsDriver.py commands          % C1: the exact command(s) of run_<XX_YYY>.sh, every argument explained
## 2. The cmsRun on the python config    % C2: how to run, walk-through of the config parts, the content of the output ROOT file
## 3. The analyzer configs               % C3: the analyzer cmsRun commands, what each produces (pdf, log, root, png)
## 4. The tables                         % C4: every number the .tex quotes, in full (copied from out/, with the file name)
```

Every command must be copy-pastable from the `test/` directory of an initialized CMSSW_15_0_5 (`cmsenv`).
