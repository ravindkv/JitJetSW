#!/usr/bin/env python3
# trigPrimLedger.py: the calorimeter trigger primitives of chapter 11 (DIGI) for the three samples side by side, from
# the TrigPrimAnalyzer logs (trigPrim_<case>_11_DIGI.log).
#   python3 trigPrimLedger.py <test/11_DIGI/out> 11_DIGI
# The trigger primitives are made in the DIGI step for the Level-1 trigger of chapter 12; this is the last stage at
# which the signal alone (11a), one pileup collision alone (11b) and the signal with its pileup (11a+11b) can be
# compared, so the comparison is made here.
import os, re, sys

out, chapter = sys.argv[1], sys.argv[2]
names = {'signal': '(11a)', 'pileup': '(11b)', 'mixed': '(11a+11b)'}
logs = {}
for c in names:
    p = os.path.join(out, 'trigPrim_%s_%s.log' % (c, chapter))
    if os.path.exists(p):
        logs[c] = open(p).read().splitlines()
cases = [c for c in names if c in logs]

def kv(line):
    return {k: v.rstrip(':,') for k, v in re.findall(r'([\w+\-\[\]\(\)>=.]+)=([^\s|;]+)', line)}

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

W = 30
print('== trigPrimLedger %s' % chapter)
for c in cases:
    print('%-10s %s: %s' % (names[c], c, (lines(c, '== TrigPrimAnalyzer') or [''])[0].replace('== TrigPrimAnalyzer ', '')))
print((lines(cases[0], '  format:') or [''])[0].strip())

print('\n-- 1. The trigger primitives: primitives with ET > 0 / sum ET [GeV] / largest ET [GeV]')
print('%-26s' % '' + ''.join('%*s' % (W, '%s %s' % (names[c], c)) for c in cases))
for label, pre, total in (('ECAL, EB (of 2448)', '  TP EB:', 0), ('ECAL, EE (of 1584)', '  TP EE:', 0), ('ECAL, all (of 4032)', '  TP ECAL:', 0),
                          ('HCAL, HB (of 2304)', '  TP HB:', 0), ('HCAL, HE (of 1728)', '  TP HE:', 0), ('HCAL, HF (of 792)', '  TP HF:', 0),
                          ('HCAL, all (of 4824)', '  TP HCAL:', 0)):
    s = '%-26s' % label
    for c in cases:
        d = first(c, pre)
        s += '%*s' % (W, '%s / %s / %s' % (d.get('nonzero', '-'), f(d.get('sumEt')), f(d.get('maxEt'))))
    print(s)
s = '%-26s' % 'all primitives (of 8856)'
for c in cases:
    e, h = first(c, '  TP ECAL:'), first(c, '  TP HCAL:')
    s += '%*s' % (W, '%d / %.1f' % (int(e.get('nonzero', 0)) + int(h.get('nonzero', 0)), float(e.get('sumEt', 0)) + float(h.get('sumEt', 0))))
print(s)

print('\n-- 2. One primitive per bunch crossing: sum ET [GeV] of the time samples stored with every primitive')
print('      (25 ns per sample; [ ] is the sample of interest, the bunch crossing of the event)')
for c in cases:
    e = (lines(c, '  ECAL TP time samples') or [''])[0]
    h = (lines(c, '  HCAL TP time samples') or [''])[0]
    me = re.search(r'ECAL=([\d.]+) ([\d.]+) ([\d.]+) ([\d.]+) ([\d.]+)', e)
    mh = re.search(r'HBHE=([\d.]+) ([\d.]+) ([\d.]+) ([\d.]+) HF=([\d.]+) ([\d.]+) ([\d.]+) ([\d.]+)', h)
    print('%-10s %s' % (names[c], c))
    if me:
        print('   ECAL     bx -2..+2: %8s %8s [%8s] %8s %8s' % me.groups())
    if mh:
        g = mh.groups()
        print('   HB + HE  bx -2..+1: %8s %8s [%8s] %8s' % g[:4])
        print('   HF       bx -1.. 0:          %8s [%8s]' % (g[5], g[6]))

print('\n-- 3. What the ECAL primitives do inside the DIGI step: the trigger-tower flag of every primitive (low / mid / high')
print('      interest) and the selective-readout decision of the ECAL digitisation that follows from it')
print('%-44s' % '' + ''.join('%16s' % names[c] for c in cases))
for label, pre, key in (('EB towers flagged mid interest', '  ECAL TT flags EB:', 'mid(1)'), ('EB towers flagged high interest', '  ECAL TT flags EB:', 'high(3)'),
                        ('EB towers read in full (of 2448)', '  ECAL SR flags EB', 'full(3)'),
                        ('   with a high flag themselves', '  ECAL SR flags EB', 'high'), ('   others (mid flag, or next to a high one)', '  ECAL SR flags EB', 'one'),
                        ('EE towers flagged mid interest', '  ECAL TT flags EE:', 'mid(1)'), ('EE towers flagged high interest', '  ECAL TT flags EE:', 'high(3)'),
                        ('EE supercrystals read in full (of 624)', '  ECAL SR flags EE', 'full(3)')):
    print('%-44s' % label + ''.join('%16s' % first(c, pre).get(key, '-') for c in cases))
for c in cases:
    for l in lines(c, '  EB tower read in full'):
        print('%-10s %s' % (names[c], l.strip()))

print('\n-- 4. What an ECAL primitive measures: the primitives against the transverse energy that the particles of the sample')
print('      left in the crystals of each tower (PCaloHits; in the mixed sample only the hits of the signal are in the file)')
for c in cases:
    print('%-10s %s' % (names[c], c))
    for r in ('ECAL', 'EB', 'EE'):
        l = (lines(c, '  ECAL deposits %s:' % r) or [''])[0]
        m = re.search(r'towersWithDeposit=(\d+) sumDepositEt=([\d.]+) \| deposit>=0.25GeV and TP: towers=(\d+) depositEt=([\d.]+) tpEt=([\d.]+) \| TP without deposit>=0.25GeV: '
                      r'towers=(\d+) tpEt=([\d.]+) \| deposit>=0.25GeV without TP: towers=(\d+) depositEt=([\d.]+)', l)
        if not m:
            continue
        g = m.groups()
        lost = float(g[1]) - float(g[3]) - float(g[8])
        print('   %-4s deposits: %s towers, %s GeV | towers with a deposit >= 0.25 GeV and a primitive: %s (deposit %s GeV, primitives %s GeV)' % (
            r, g[0], f(g[1]), g[2], f(g[3]), f(g[4])))
        print('        primitives without a deposit >= 0.25 GeV: %s (%s GeV) | deposits >= 0.25 GeV without a primitive: %s (%s GeV) | deposits below 0.25 GeV per tower: %.1f GeV' % (
            g[5], f(g[6]), g[7], f(g[8]), lost))

print('\n-- 5. The two generator jets in the primitives: the 9 x 9 towers around the jet axis, sum ET [GeV]')
for c in cases:
    for l in lines(c, '  GenJet '):
        print('%-10s %s' % (names[c], l.strip()))
print('\n-- 6. The largest primitives (ECAL + HCAL of the tower)')
for c in cases:
    for l in lines(c, '  top tower ')[:6]:
        print('%-10s %s' % (names[c], l.strip()))
