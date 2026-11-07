#!/usr/bin/env python3
# simLedger.py: the ledger of chapter 10 (SIM) from the analyzer logs, no ROOT needed.
#   python3 simLedger.py <test/10_SIM/out> 10_SIM [<simAnalyzer log of the transport-only stage of chapter 09>]
# Table 1, the Geant4 physics switched on stage by stage (SimAnalyzer logs): primaries, primaries reaching the calorimeter
# boundary, secondaries by process, tracker / muon hits and their energy loss, calorimeter cells and energies, the b and
# bbar systems. Table 2, the three samples station by station (SimHitAnalyzer logs): hits, cells, energies by kind of the
# primary, primaries with hits, and for the mixed sample the signal / pileup split.
import os, re, sys

out, chapter = sys.argv[1], sys.argv[2]
transport = sys.argv[3] if len(sys.argv) > 3 else ''

def num(pattern, text, default=''):
    m = re.search(pattern, text)
    return m.group(1) if m else default

print('== simLedger %s (%s)' % (chapter, out))
print('-- the Geant4 physics switched on stage by stage (SimAnalyzer, event 1)')
stages = [('Transport (09)', transport)] + [(s, os.path.join(out, 'simAnalyzer_%s_%s.log' % (s, chapter))) for s in ('B', 'B_Decay', 'B_Decay_EM', 'signal')]
hdr = '%-16s %9s %8s %11s %-22s %12s %10s %9s %-30s %-28s' % ('stage', 'primaries', 'reachCal', 'secondaries', '(decay/EM/had)', 'trk+mu hits', 'eLoss[GeV]', 'caloCell', 'E EB/EE/HB/HE/HF', 'b system: pt0 ptCal E_ECAL E_HCAL')
print(hdr)
for name, path in stages:
    if not path or not os.path.exists(path):
        continue
    t = open(path).read()
    p1 = num(r'page 1 \(primaries\): nPrimaries=(\d+)', t)
    ns1 = num(r'nStatus1=(\d+)', t)
    reach = num(r'nReaching=(\d+)', t)
    nsec = num(r'nSecondaries=(\d+)', t)
    dec, em, had = num(r'decay=(\d+)', t), num(r' EM=(\d+)', t), num(r'hadronic=(\d+)', t)
    nhits = num(r'page 4 \(tracker/muon hits\): nHits=(\d+)', t)
    eloss = num(r'sumELoss\[GeV\]=([0-9.e+-]+)', t)
    ncell = num(r'nCells=(\d+)', t)
    eb, ee, hb, he, hf = (num(r' %s=([0-9.e+-]+)\(' % s, t, '0') for s in ('EB', 'EE', 'HB', 'HE', 'HF'))
    bpt0 = num(r'page 1 .*?\| b system: n=\S+ pt=([0-9.]+)', t)
    bptc = num(r'page 2 .*?\| b system at the boundary: n=\S+ pt=([0-9.]+)', t)
    becal = num(r'\| b system: E_ECAL=([0-9.e+-]+)', t)
    bhcal = num(r'\| b system: E_ECAL=\S+ E_HCAL=([0-9.e+-]+)', t)
    print('%-16s %4s/%-4s %8s %11s %-22s %12s %10s %9s %-30s %-28s' % (
        name, p1, ns1, reach, nsec, '(%s/%s/%s)' % (dec, em, had), nhits, eloss, ncell,
        '%.0f/%.0f/%.2f/%.2f/%.0f' % tuple(float(x) for x in (eb, ee, hb, he, hf)), '%s %s %s %s' % (bpt0, bptc, becal, bhcal)))

print('-- the three samples, station by station (SimHitAnalyzer)')
for case in ('signal', 'pileup', 'mixed'):
    path = os.path.join(out, 'simHits_%s_%s.log' % (case, chapter))
    if not os.path.exists(path):
        continue
    t = open(path).read()
    head = num(r'(SimHitAnalyzer run \d+ event \d+: .*)', t)
    print('sample %s: %s' % (case, head))
    mix = num(r'(pileup mixed in .*)', t)
    if mix:
        print('  ' + mix)
    for block in re.findall(r'  station (\w+): (.*?)\n((?:    .*\n)+)', t):
        name, summary, lines = block
        kinds = num(r'hits by kind of the primary: (.*)', lines)
        prim = num(r'primaries with hits: (.*)', lines)
        print('  %-5s %s' % (name, summary))
        print('        by kind: %s' % kinds.strip())
        print('        primaries with hits: %s' % prim.strip())
