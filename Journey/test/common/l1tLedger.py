#!/usr/bin/env python3
# l1tLedger.py: the ledger of chapter 12 (L1T) from the log of the three L1 analyzers (l1t_12_L1T.log).
#   python3 l1tLedger.py <test/12_L1T/out> 12_L1T
# The Level-1 trigger is a checkpoint: it does not change the recorded event, so its ledger has two parts. The energy
# part follows the transverse energy the trigger sees from step to step (trigger primitives -> Layer-1 towers -> jets
# and sums). The decision part lists the algorithms of the menu that fire, the objects behind them and the margin by
# which the single-object seeds fire or fail. One sample: the event with its pileup (from chapter 12 on the signal
# and the pileup are no longer followed apart).
import os, re, sys

out, chapter = sys.argv[1], sys.argv[2]
log = open(os.path.join(out, 'l1t_%s.log' % chapter)).read().splitlines()

def kv(line):
    return {k: v.rstrip(':,') for k, v in re.findall(r'([\w+\-\[\]]+)=([^\s|;]+)', line)}

def lines(prefix):
    return [l for l in log if l.startswith(prefix)]

def first(prefix):
    l = lines(prefix)
    return kv(l[0]) if l else {}

def f(x, fmt='%.1f'):
    try:
        return fmt % float(x)
    except (TypeError, ValueError):
        return '-'

menu, dec = first('  menu:'), first('  decision:')
print('== l1tLedger %s' % chapter)
print('event: %s' % (lines('== L1TCaloAnalyzer') or [''])[0].replace('== L1TCaloAnalyzer ', ''))
print('menu %s, %s algorithms, %s conditions; finalOR=%s fired=%s' % (menu.get('name'), menu.get('nAlgorithms'), menu.get('nConditions'), dec.get('finalOR'), dec.get('firedInitial')))
for pre in ('  caloParams:', '  caloParams etSum', '  jet pileup estimate'):
    for l in lines(pre):
        print(l.strip()[:200])

# ---------------------------------------------------------------------------------------------------------------- 1
print('\n-- 1. Steps 1 and 2: the transverse energy the trigger sees')
print('%-30s %10s %12s %12s' % ('', 'ET > 0', 'sum ET [GeV]', 'max ET [GeV]'))
tot = {}
for label, pre in (('ECAL primitives, EB', '  TP EB:'), ('ECAL primitives, EE', '  TP EE:'), ('ECAL primitives, all', '  TP ECAL:'),
                   ('HCAL primitives, HB', '  TP HB:'), ('HCAL primitives, HE', '  TP HE:'), ('HCAL primitives, HF', '  TP HF:'), ('HCAL primitives, all', '  TP HCAL:'),
                   ('Layer-1 towers, barrel', '  Layer1 barrel:'), ('Layer-1 towers, endcap', '  Layer1 endcap:'), ('Layer-1 towers, forward', '  Layer1 forward:'),
                   ('Layer-1 towers, all', '  Layer1 all:')):
    d = first(pre)
    tot[pre] = d
    print('%-30s %10s %12s %12s' % (label, d.get('nonzero', '-'), f(d.get('sumEt')), f(d.get('maxEt'))))
tp_n = int(tot['  TP ECAL:'].get('nonzero', 0)) + int(tot['  TP HCAL:'].get('nonzero', 0))
tp_et = float(tot['  TP ECAL:'].get('sumEt', 0)) + float(tot['  TP HCAL:'].get('sumEt', 0))
print('%-30s %10d %12.1f' % ('all primitives (of 8856)', tp_n, tp_et))
l1in = first('  Layer1 inputs after calibration')
print('what Layer-1 changes (sum ET in -> out, difference):')
for label, tin, tout in (('ECAL (EB + EE)', ('  TP ECAL:',), 'sumEm'), ('HCAL (HB + HE)', ('  TP HB:', '  TP HE:'), 'sumHad'), ('HF', ('  TP HF:',), None)):
    a = sum(float(first(t).get('sumEt', 0)) for t in tin)
    b = float(l1in.get(tout, 0)) if tout else float(first('  Layer1 forward:').get('sumEt', 0))
    print('  %-24s %8.1f -> %8.1f (%+.1f)' % (label, a, b, b - a))
l1all = float(first('  Layer1 all:').get('sumEt', 0))
print('  %-24s %8.1f -> %8.1f (%+.1f)' % ('all', tp_et, l1all, l1all - tp_et))
for l in lines('  Layer1 top tower'):
    print(l.strip())

# ---------------------------------------------------------------------------------------------------------------- 2
print('\n-- 2. Ring by ring (72 towers in phi): ECAL primitives + HCAL primitives -> Layer-1 towers, sum ET in GeV')
print('%5s %10s %10s %10s' % ('ieta', 'ecalTP', 'hcalTP', 'layer1'))
for l in lines('  ring ieta='):
    d = kv(l)
    print('%5s %10s %10s %10s' % (d['ieta'], f(d.get('ecalTP')), f(d.get('hcalTP')), f(d.get('layer1'))))

# ---------------------------------------------------------------------------------------------------------------- 3
print('\n-- 3. Step 3: the Layer-2 jets (GeV). window = sum of the 9 x 9 Layer-1 towers; pileup = the estimate subtracted;')
print('      calib = the calibration factor; ET = (window - pileup) x calib; ecalTP / hcalTP = the primitives in the window')
gj = [kv(l) for l in lines('  GenJet:')]
print('%d L1 jets; GenJets: %s' % (len(lines('  L1Jet ')), ', '.join('pt %s eta %s phi %s' % (f(g['pt']), f(g['eta'], '%.2f'), f(g['phi'], '%.2f')) for g in gj) or 'none'))
print('   %6s %6s %6s %5s %5s %6s %7s %7s %7s %6s %7s %7s %8s' % ('ET', 'eta', 'phi', 'ieta', 'iphi', 'seed', 'window', 'pileup', 'win-pu', 'calib', 'ecalTP', 'hcalTP', 'GenJet'))
sums = [0., 0., 0.]
for l in lines('  L1Jet '):
    d = kv(l)
    g = kv(l.split('genJet:')[1]) if 'genJet:' in l else {}
    matched = float(g.get('pt', 0)) > 0
    sums[0] += float(d['rawEt']); sums[1] += float(d['puEt']); sums[2] += float(d['et'])
    print('   %6s %6s %6s %5s %5s %6s %7s %7s %7s %6s %7s %7s %8s' % (f(d['et']), f(d['eta'], '%.2f'), f(d['phi'], '%.2f'), d['ieta'], d['iphi'], f(d['seedEt']), f(d['rawEt']),
          f(d['puEt']), f(d['rawMinusPu']), f(d['calibration'], '%.2f'), f(d['ecalTP']), f(d['hcalTP']), f(g.get('pt')) if matched else '-'))
    m = re.search(r'phiRing: ringSum=(\S+) rings: ([\d. ]+) recomputedPu=(\S+)', l)
    if m and matched:
        print('          phi rings of the 9 eta columns of the window (72 towers each): %s; sum %s; x 9/72 = %s (the emulator: %s)' % (m.group(2).strip(), m.group(1), m.group(3), f(d['puEt'])))
print('   all jets: sum of the windows %.1f, of the pileup estimates %.1f, of the jet ET %.1f' % tuple(sums))

# ---------------------------------------------------------------------------------------------------------------- 4
print('\n-- 4. Step 3: what Layer-2 sends to the global trigger')
c = first('  counts:')
print('jets %s, e/gamma %s, taus %s' % (c.get('L1Jets'), c.get('L1EGs'), c.get('L1Taus')))
s = first('  L1Sums:')
for label, k in (('ETT (total ET, towers |ieta| <= 28)', 'ETT'), ('ETTEM (ECAL only)', 'ETTEM'), ('HTT (jets > 30 GeV, |ieta| <= 26)', 'HTT'), ('ETM', 'ETM'), ('ETMHF (with HF)', 'ETMHF'),
                 ('HTM', 'HTM'), ('HTMHF', 'HTMHF'), ('tower count', 'towerCount')):
    print('%-38s %10s' % (label, f(s.get(k))))
for kind in ('L1EG', 'L1Tau'):
    for l in lines('  %s ' % kind):
        print(l.strip())

# ---------------------------------------------------------------------------------------------------------------- 5
print('\n-- 5. Steps 4 and 5: the muon trigger')
for label, pre, k in (('DT phi segments', '  TP counts:', 'DTphi'), ('DT theta segments', '  TP counts:', 'DTtheta'), ('CSC anode patterns (ALCT)', '  TP counts:', 'CSC_ALCT'),
                      ('CSC cathode patterns (CLCT)', '  TP counts:', 'CSC_CLCT'), ('CSC stubs (LCT)', '  TP counts:', 'CSC_LCT'), ('GEM pads', '  TP counts:', 'GEMpads'),
                      ('GEM pad clusters', '  TP counts:', 'GEMpadClusters'), ('stubs given to the EMTF', '  EMTF input stubs:', 'all'),
                      ('   of which neighbour copies', '  EMTF input stubs:', 'neighbourCopies'), ('EMTF tracks', '  TF counts', 'EMTFtracks'),
                      ('BMTF (Kalman) candidates, all bx / bx 0', '  TF counts', 'kBMTF'), ('OMTF candidates', '  TF counts', 'OMTF'), ('EMTF candidates', '  TF counts', 'EMTF'),
                      ('uGMT muons', '  uGMT counts:', 'muons'), ('muon showers', '  uGMT counts:', 'muonShowers')):
    print('%-42s %8s' % (label, first(pre).get(k, '-')))
for pre in ('  EMTF input stubs:', '  EMTFHit:', '  EMTFTrack:', '  TFCand:', '  uGMT muon:', '  GenMuon:', '  truth:'):
    for l in lines(pre):
        print(l.strip())

# ---------------------------------------------------------------------------------------------------------------- 6
print('\n-- 6. Step 6: the decision of the global trigger')
cic = lines('  CICADA score:')
print('FinalOR %s; %s of %s algorithms fire (%s after prescales and masks); %s' % (dec.get('finalOR'), dec.get('firedInitial'), menu.get('nAlgorithms'), dec.get('firedFinal'),
                                                                                    cic[0].strip() if cic else ''))
print((lines('  uGT inputs') or [''])[0].strip())
fired = []
for l in lines('  fired:'):
    m = re.match(r'  fired: bit=(\d+) name=(\S+) final=(\d) expression="([^"]*)"(.*)', l)
    fired.append((int(m.group(1)), m.group(2), m.group(4), m.group(5)))
print('the %d algorithms that fire, with their expression:' % len(fired))
for bit, name, expr, _ in fired:
    print('  %4d %-58s %s' % (bit, name, expr))
print('the objects behind the conditions that are true (at most two combinations per condition):')
for bit, name, expr, objs in fired:
    if objs.strip():
        print('  %-46s%s' % (name, objs[:420]))
print('the ladders: the event against the thresholds of the single-object families (GeV); margin = value - threshold')
for l in lines('  ladder '):
    m = re.match(r'  ladder (\S+): value=(\S+) thresholds\(fired\):(.*)', l)
    value = float(m.group(2))
    th = [(float(a), int(b)) for a, b in re.findall(r'([\d.]+)\((\d)\)', m.group(3))]
    passed = [t for t, fl in th if fl]
    failed = [t for t, fl in th if not fl]
    s = '   %-20s value %7.1f' % (m.group(1), value)
    s += '   highest passed: %s' % ('%g (margin %+.1f)' % (max(passed), value - max(passed)) if passed else 'none')
    s += '   lowest failed: %s' % ('%g (margin %+.1f)' % (min(failed), value - min(failed)) if failed else 'none')
    print(s)
