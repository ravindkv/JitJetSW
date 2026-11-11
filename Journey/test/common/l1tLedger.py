#!/usr/bin/env python3
# l1tLedger.py: the ledger of chapter 12 (L1T) from the analyzer logs of the three samples (l1t_<case>_12_L1T.log).
#   python3 l1tLedger.py <test/12_L1T/out> 12_L1T
# The Level-1 trigger is a checkpoint: it does not change the recorded event, so its ledger has two parts. The energy
# part follows the transverse energy the trigger sees from step to step (trigger primitives -> Layer-1 towers -> jets
# and sums), for the signal alone (12a), one pileup collision alone (12b) and the signal with its pileup (12a+12b).
# The decision part lists the algorithms of the menu that fire in each sample and the margin by which the single-object
# seeds fire or fail.
import os, re, sys

out, chapter = sys.argv[1], sys.argv[2]
cases = ('signal', 'pileup', 'mixed')
names = {'signal': '(12a)', 'pileup': '(12b)', 'mixed': '(12a+12b)'}

def kv(line):
    return {k: v.rstrip(':,') for k, v in re.findall(r'([\w+\-\[\]]+)=([^\s|;]+)', line)}

logs = {}
for c in cases:
    p = os.path.join(out, 'l1t_%s_%s.log' % (c, chapter))
    if os.path.exists(p):
        logs[c] = open(p).read().splitlines()
cases = [c for c in cases if c in logs]

def lines(c, prefix):
    return [l for l in logs[c] if l.startswith(prefix)]

def first(c, prefix):
    l = lines(c, prefix)
    return kv(l[0]) if l else {}

def f(x, fmt='%.1f'):
    try:
        return fmt % float(x)
    except (TypeError, ValueError):
        return '-'

print('== l1tLedger %s' % chapter)
for c in cases:
    m = first(c, '  menu:')
    d = first(c, '  decision:')
    print('%-9s %s: menu %s, %s algorithms; finalOR=%s fired=%s' % (names[c], c, m.get('name'), m.get('nAlgorithms'), d.get('finalOR'), d.get('firedInitial')))
cp = [l for l in logs[cases[0]] if l.startswith('  caloParams:')]
if cp:
    print('caloParams:' + cp[0].split('caloParams:')[1])
for pre in ('  caloParams etSum', '  jet pileup estimate'):
    for l in lines(cases[0], pre):
        print(l.strip())

# ---------------------------------------------------------------------------------------------------------------- 1
print('\n-- 1. Steps 1 and 2: the transverse energy the trigger sees (towers with ET > 0 / sum ET in GeV / largest ET)')
print('%-26s' % '' + ''.join('%28s' % ('%s %s' % (names[c], c)) for c in cases))
rows = [('ECAL primitives, EB', '  TP EB:'), ('ECAL primitives, EE', '  TP EE:'), ('ECAL primitives, all', '  TP ECAL:'),
        ('HCAL primitives, HB', '  TP HB:'), ('HCAL primitives, HE', '  TP HE:'), ('HCAL primitives, HF', '  TP HF:'), ('HCAL primitives, all', '  TP HCAL:'),
        ('Layer-1 towers, barrel', '  Layer1 barrel:'), ('Layer-1 towers, endcap', '  Layer1 endcap:'), ('Layer-1 towers, forward', '  Layer1 forward:'),
        ('Layer-1 towers, all', '  Layer1 all:')]
for label, pre in rows:
    s = '%-26s' % label
    for c in cases:
        d = first(c, pre)
        s += '%28s' % ('%s / %s / %s' % (d.get('nonzero', '-'), f(d.get('sumEt')), f(d.get('maxEt'))))
    print(s)
s = '%-26s' % 'Layer-1 inputs, EM | HAD'
for c in cases:
    d = first(c, '  Layer1 inputs after calibration')
    s += '%28s' % ('%s | %s' % (f(d.get('sumEm')), f(d.get('sumHad'))))
print(s + '   (the calibrated ECAL and HB+HE inputs of the towers, sum ET)')
print('what Layer-1 changes (sum ET in, sum ET out, difference):')
for label, tin, tout in (('ECAL (EB + EE)', ('  TP ECAL:',), 'sumEm'), ('HCAL (HB + HE)', ('  TP HB:', '  TP HE:'), 'sumHad'), ('HF', ('  TP HF:',), None)):
    s = '  %-24s' % label
    for c in cases:
        a = sum(float(first(c, t).get('sumEt', 0)) for t in tin)
        b = float(first(c, '  Layer1 inputs after calibration').get(tout, 0)) if tout else float(first(c, '  Layer1 forward:').get('sumEt', 0))
        s += '%28s' % ('%.1f -> %.1f (%+.1f)' % (a, b, b - a))
    print(s)

# ---------------------------------------------------------------------------------------------------------------- 2
print('\n-- 2. Ring by ring (72 towers in phi): ECAL primitives + HCAL primitives -> Layer-1 towers, sum ET in GeV')
print('%5s' % 'ieta' + ''.join('%34s' % ('%s ecal + hcal -> layer1' % names[c]) for c in cases))
rings = {}
for c in cases:
    for l in lines(c, '  ring ieta='):
        d = kv(l)
        rings.setdefault(int(d['ieta']), {})[c] = d
for ieta in sorted(rings):
    s = '%5d' % ieta
    for c in cases:
        d = rings[ieta].get(c, {})
        s += '%34s' % ('%s + %s -> %s' % (f(d.get('ecalTP')), f(d.get('hcalTP')), f(d.get('layer1'))))
    print(s)

# ---------------------------------------------------------------------------------------------------------------- 3
print('\n-- 3. Step 3: the Layer-2 jets (GeV). window = sum of the 9 x 9 Layer-1 towers; pileup = the estimate subtracted;')
print('      calib = the calibration factor; ET = (window - pileup) x calib; ecalTP / hcalTP = the primitives in the window')
for c in cases:
    gj = [kv(l) for l in lines(c, '  GenJet:')]
    print('%s %s: %d L1 jets; GenJets: %s' % (names[c], c, len(lines(c, '  L1Jet ')), ', '.join('pt %s eta %s phi %s' % (f(g['pt']), f(g['eta'], '%.2f'), f(g['phi'], '%.2f')) for g in gj) or 'none'))
    print('   %6s %6s %6s %5s %5s %6s %7s %7s %7s %6s %7s %7s %8s' % ('ET', 'eta', 'phi', 'ieta', 'iphi', 'seed', 'window', 'pileup', 'win-pu', 'calib', 'ecalTP', 'hcalTP', 'GenJet'))
    for l in lines(c, '  L1Jet '):
        d = kv(l)
        gen = l.split('genJet:')[1] if 'genJet:' in l else ''
        g = kv(gen)
        print('   %6s %6s %6s %5s %5s %6s %7s %7s %7s %6s %7s %7s %8s' % (f(d['et']), f(d['eta'], '%.2f'), f(d['phi'], '%.2f'), d['ieta'], d['iphi'], f(d['seedEt']), f(d['rawEt']),
              f(d['puEt']), f(d['rawMinusPu']), f(d['calibration'], '%.2f'), f(d['ecalTP']), f(d['hcalTP']), f(g.get('pt')) if float(g.get('pt', 0)) > 0 else '-'))
        m = re.search(r'phiRing: ringSum=(\S+) rings: ([\d. ]+) recomputedPu=(\S+)', l)
        if m and float(g.get('pt', 0)) > 0:
            print('          phi rings of the 9 eta columns of the window (72 towers each): %s; sum %s; x 9/72 = %s' % (m.group(2).strip(), m.group(1), m.group(3)))

# ---------------------------------------------------------------------------------------------------------------- 4
print('\n-- 4. Step 3: what Layer-2 sends to the global trigger')
keys = [('jets', 'L1Jets'), ('e/gamma', 'L1EGs'), ('taus', 'L1Taus')]
print('%-22s' % '' + ''.join('%16s' % names[c] for c in cases))
for label, k in keys:
    print('%-22s' % label + ''.join('%16s' % first(c, '  counts:').get(k, '-') for c in cases))
for label, k in (('ETT (total ET)', 'ETT'), ('ETTEM (ECAL only)', 'ETTEM'), ('HTT (jets > 30 GeV)', 'HTT'), ('ETM', 'ETM'), ('ETMHF (with HF)', 'ETMHF'), ('HTM', 'HTM'),
                 ('HTMHF', 'HTMHF'), ('tower count', 'towerCount')):
    print('%-22s' % label + ''.join('%16s' % f(first(c, '  L1Sums:').get(k)) for c in cases))
for c in cases:
    for kind in ('L1EG', 'L1Tau'):
        ls = lines(c, '  %s ' % kind)
        if ls:
            d = kv(ls[0])
            print('%s leading %s: ET %s eta %s phi %s iso %s' % (names[c], kind, f(d['et']), f(d['eta'], '%.2f'), f(d['phi'], '%.2f'), d.get('iso')))

# ---------------------------------------------------------------------------------------------------------------- 5
print('\n-- 5. Steps 4 and 5: the muon trigger')
print('%-34s' % '' + ''.join('%16s' % names[c] for c in cases))
for label, pre, k in (('DT phi segments', '  TP counts:', 'DTphi'), ('DT theta segments', '  TP counts:', 'DTtheta'), ('CSC anode patterns (ALCT)', '  TP counts:', 'CSC_ALCT'),
                      ('CSC cathode patterns (CLCT)', '  TP counts:', 'CSC_CLCT'), ('CSC stubs (LCT)', '  TP counts:', 'CSC_LCT'), ('GEM pads', '  TP counts:', 'GEMpads'),
                      ('GEM pad clusters', '  TP counts:', 'GEMpadClusters'), ('stubs given to the EMTF', '  EMTF input stubs:', 'all'),
                      ('   of which neighbour copies', '  EMTF input stubs:', 'neighbourCopies'), ('EMTF tracks', '  TF counts', 'EMTFtracks'),
                      ('BMTF (Kalman) candidates', '  TF counts', 'kBMTF'), ('OMTF candidates', '  TF counts', 'OMTF'), ('EMTF candidates', '  TF counts', 'EMTF'),
                      ('uGMT muons', '  uGMT counts:', 'muons'), ('muon showers', '  uGMT counts:', 'muonShowers')):
    print('%-34s' % label + ''.join('%16s' % first(c, pre).get(k, '-') for c in cases))
for c in cases:
    for pre in ('  EMTFTrack:', '  TFCand:', '  uGMT muon:', '  GenMuon:'):
        for l in lines(c, pre):
            print('%s %s' % (names[c], l.strip()))

# ---------------------------------------------------------------------------------------------------------------- 6
print('\n-- 6. Step 6: the decision of the global trigger')
for c in cases:
    d = first(c, '  decision:')
    cic = lines(c, '  CICADA score:')
    print('%-9s FinalOR %s; %s of %s algorithms fire (%s after prescales and masks); %s' % (names[c], d.get('finalOR'), d.get('firedInitial'),
          first(c, '  menu:').get('nAlgorithms'), d.get('firedFinal'), cic[0].strip() if cic else ''))
fired = {}
for c in cases:
    for l in lines(c, '  fired:'):
        m = re.match(r'  fired: bit=(\d+) name=(\S+) final=(\d) expression="([^"]*)"(.*)', l)
        fired.setdefault(int(m.group(1)), {'name': m.group(2), 'expr': m.group(4), 'cases': {}})['cases'][c] = m.group(5)
print('the algorithms that fire (x) in each sample:')
print('  %4s %-58s' % ('bit', 'name') + ''.join('%10s' % names[c] for c in cases) + '  expression')
for bit in sorted(fired):
    a = fired[bit]
    print('  %4d %-58s' % (bit, a['name']) + ''.join('%10s' % ('x' if c in a['cases'] else '.') for c in cases) + '  ' + a['expr'])
last = cases[-1]
print('the objects behind the conditions that are true in %s %s (at most two combinations per condition):' % (names[last], last))
for bit in sorted(fired):
    a = fired[bit]
    if last in a['cases'] and a['cases'][last].strip():
        print('  %-46s%s' % (a['name'], a['cases'][last][:420]))
print('the ladders: the event against the thresholds of the single-object families (GeV); margin = value - threshold')
for c in cases:
    print('%s %s' % (names[c], c))
    for l in lines(c, '  ladder '):
        m = re.match(r'  ladder (\S+): value=(\S+) thresholds\(fired\):(.*)', l)
        value = float(m.group(2))
        th = [(float(a), int(b)) for a, b in re.findall(r'([\d.]+)\((\d)\)', m.group(3))]
        passed = [t for t, fl in th if fl]
        failed = [t for t, fl in th if not fl]
        s = '   %-20s value %7.1f' % (m.group(1), value)
        s += '   highest passed: %s' % ('%g (margin %+.1f)' % (max(passed), value - max(passed)) if passed else 'none')
        s += '   lowest failed: %s' % ('%g (margin %+.1f)' % (min(failed), value - min(failed)) if failed else 'none')
        print(s)
