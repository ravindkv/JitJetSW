#!/usr/bin/env python3
# digiLedger.py: the ledger of chapter 11 (DIGI) from the DigiAnalyzer logs of the three samples.
#   python3 digiLedger.py <test/11_DIGI/out> 11_DIGI
# One row per subdetector: digis of the signal alone (11a), of one pileup collision alone (11b), of the mixed event (11a+11b)
# with, where the DigiSimLinks allow it (tracker and muon system), the split of the mixed digis into signal / in-time /
# early / late pileup / unlinked (noise), and for the calorimeters the summed amplitude above pedestal in ADC counts.
import os, re, sys

out, chapter = sys.argv[1], sys.argv[2]
order = ['PXB', 'PXF', 'TIB', 'TID', 'TOB', 'TEC', 'EB', 'EE', 'ES', 'HB', 'HE', 'HO', 'HF', 'DT', 'CSCstrip', 'CSCwire', 'RPC', 'GEM']
data = {}
heads = {}
for case in ('signal', 'pileup', 'mixed'):
    path = os.path.join(out, 'digiAnalyzer_%s_%s.log' % (case, chapter))
    if not os.path.exists(path):
        continue
    t = open(path).read()
    heads[case] = re.search(r'== DigiAnalyzer (.*)', t).group(1) if '== DigiAnalyzer' in t else ''
    d = {}
    for m in re.finditer(r'^  (\w+): nDigis=(\d+)(.*)$', t, re.M):
        name, n, rest = m.group(1), int(m.group(2)), m.group(3)
        row = {'n': n}
        for key in ('sumAmplitude[ADC]', 'signal', 'inTime', 'early', 'late', 'noLink'):
            mm = re.search(re.escape(key) + r'=([0-9.e+-]+)', rest)
            if mm:
                row[key] = float(mm.group(1))
        d[name] = row
    tot = re.search(r'total digis=(\d+)', t)
    d['total'] = {'n': int(tot.group(1)) if tot else 0}
    data[case] = d

print('== digiLedger %s' % chapter)
for case, h in heads.items():
    print('%s: %s' % (case, h))
print('%-9s %10s %10s %10s | %s' % ('subdet', '(11a)', '(11b)', '(11a+11b)', 'mixed by origin: signal / in-time / early / late / no link ; calorimeters: sum amplitude [ADC] (11a) (11b) (11a+11b)'))
for name in order + ['total']:
    cols = []
    for case in ('signal', 'pileup', 'mixed'):
        row = data.get(case, {}).get(name)
        cols.append('%10d' % row['n'] if row else '%10s' % '-')
    extra = ''
    m = data.get('mixed', {}).get(name)
    if m and 'signal' in m:
        extra = '%d / %d / %d / %d / %d' % (m['signal'], m['inTime'], m['early'], m['late'], m['noLink'])
    elif m and 'sumAmplitude[ADC]' in m:
        amps = [data.get(c, {}).get(name, {}).get('sumAmplitude[ADC]', 0.) for c in ('signal', 'pileup', 'mixed')]
        extra = 'amplitude %.0f %.0f %.0f' % tuple(amps)
    print('%-9s %s | %s' % (name, ' '.join(cols), extra))
