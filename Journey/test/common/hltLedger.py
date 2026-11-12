#!/usr/bin/env python3
# hltLedger.py: the ledger of chapter 13 (HLT) from the log of the four HLT analyzers (hlt_13_HLT.log).
#   python3 hltLedger.py <test/13_HLT/out> 13_HLT
# The High Level Trigger is a checkpoint: it does not change the recorded event, so its ledger has two parts. The
# energy part follows what the HLT reconstructs from step to step (raw data -> rec-hits -> towers -> calo jets ->
# tracks -> particle-flow candidates -> PF jets), with the two jets of the signal followed through every step. The
# decision part lists the paths that accept the event, sorted by what satisfies them (the signal jets, an object the
# hard collision did not make, or nothing at all), the margins of the jet paths, the datasets and the streams.
import os, re, sys

out, chapter = sys.argv[1], sys.argv[2]
log = open(os.path.join(out, 'hlt_%s.log' % chapter)).read().splitlines()

def kv(line):
    return {k: v.rstrip(':,') for k, v in re.findall(r'([\w+\-\[\]]+)=([^\s|]+)', line)}

def lines(prefix):
    return [l for l in log if l.startswith(prefix)]

def first(prefix):
    l = lines(prefix)
    return kv(l[0]) if l else {}

def num(x, default=0.):
    try:
        return float(x)
    except (TypeError, ValueError):
        return default

print('== hltLedger %s' % chapter)
print('event: %s' % (lines('== HLTPathAnalyzer') or [''])[0].replace('== HLTPathAnalyzer ', ''))
menu = lines('  menu:')[0] if lines('  menu:') else ''
print(menu.strip())
print((lines('  menu modules:') or [''])[0].strip())

# ---------------------------------------------------------------------------------------------------------------- 1
print('\n-- 1. Step 1: the raw event, the Level-1 record, the seeds and the prescales')
raw = first('  rawdata:')
print('raw event: %s FEDs, %.1f kB' % (raw.get('feds'), num(raw.get('bytes')) / 1024))
subs = []
for l in lines('  rawdata '):
    m = re.match(r'  rawdata (.+): feds=(\d+) bytes=(\d+)', l)
    if m:
        subs.append((int(m.group(3)), m.group(1), int(m.group(2))))
for b, name, n in sorted(subs, reverse=True):
    print('   %-18s %4d FEDs %9.1f kB %5.1f %%' % (name, n, b / 1024, 100. * b / max(num(raw.get('bytes')), 1)))
print((lines('  L1 unpacked:') or [''])[0].strip())
p = first('  paths HLT_+DST_:')
print('paths HLT_ and DST_: %s; stopped by the L1 seed %s; by the prescale %s; start their reconstruction %d (of which %s stopped by a filter, %s accept)'
      % (p.get('total'), p.get('seedFailed'), p.get('prescaled'), int(num(p.get('total'))) - int(num(p.get('seedFailed'))) - int(num(p.get('prescaled'))),
         p.get('filterFailed'), p.get('accept')))
for l in lines('  seedObjects:'):
    print(l.strip())

# ---------------------------------------------------------------------------------------------------------------- 2
print('\n-- 2. Step 2: calorimeter rec-hits, towers, calo jets')
print('%-10s %8s %10s %10s' % ('rec-hits', 'n', 'sum E', 'sum ET'))
for r in ('EB', 'EE', 'HB', 'HE', 'HF'):
    d = first('  rechits %s:' % r)
    print('%-10s %8s %10.1f %10.1f' % (r, d.get('n'), num(d.get('sumE')), num(d.get('sumEt'))))
print('%-10s %8s %10s %10s %10s' % ('towers', 'n', 'sum ET', 'EM', 'HAD'))
for r in ('barrel', 'endcap', 'forward', 'all'):
    d = first('  towers %s:' % r)
    print('%-10s %8s %10.1f %10.1f %10.1f' % (r, d.get('n'), num(d.get('sumEt')), num(d.get('sumEmEt')), num(d.get('sumHadEt'))))
for l in lines('  rho calo') + lines('  calo jets:') + lines('  jec calo:'):
    print(l.strip()[:400])
for l in lines('  CaloJet '):
    if 'gen=-' not in l or num(re.search(r'correctedPt=([\d.]+)', l).group(1) if re.search(r'correctedPt=([\d.]+)', l) else 0) > 15:
        print(l.strip())
for l in lines('  zoom gen jet ')[:2]:
    print(l.strip())

# ---------------------------------------------------------------------------------------------------------------- 3
print('\n-- 3. Step 3: tracking and vertices')
for pre in ('  clusters:', '  tracks pixel:', '  tracks iter0:', '  tracks doubletRecovery:', '  tracks merged:', '  truth:', '  vertices pixel:',
            '  vertices trimmed', '  vertices PF:', '  vertex PF', '  signal vertex rank:', '  jet tracks:', '  tracks from the signal vertex'):
    for l in lines(pre):
        print(l.strip())
for l in lines('  vertex pixel ')[:8]:
    print(l.strip())

# ---------------------------------------------------------------------------------------------------------------- 4
print('\n-- 4. Step 4: particle flow and PF jets')
for pre in ('  pfcands ', '  rho (', '  pf jets:', '  jec pf:'):
    for l in lines(pre):
        print(l.strip()[:420])
for l in lines('  PFJet '):
    if num(re.search(r'pt=([\d.]+)', l).group(1)) > 20:
        print(l.strip())
for l in lines('  zoom gen jet ')[2:]:
    print(l.strip())

# ---------------------------------------------------------------------------------------------------------------- 5
print('\n-- 5. Step 5: the decision')
acc = [kv(l) for l in lines('  path: ') if ' accept=1 ' in l]
names = [a['name'] for a in acc]
# what satisfies each accepted HLT_/DST_ path (the reasons are those of the chapter: the muon is the kaon decay of a
# pileup collision found in chapter 12, the calorimeter MET is 3.5 times the neutrino of the signal)
signalJets = re.compile(r'^HLT_(PFJet|PFJetFwd|AK8PFJet|AK8PFJetFwd|DiPFJetAve)\d+_v\d+$')
pileupObject = {'HLT_Mu3_PFJet40': 'the muon of a pileup kaon decay (3.1 GeV) and the b jet',
                'HLT_CaloMET90_NotCleaned': 'a calorimeter MET of 133.3 GeV (the neutrino has 37.5)'}
l1only = {'HLT_L1ETMHadSeeds': 'the L1 seed alone (ETMHF)', 'HLT_L1SingleMuCosmics': 'the L1 seed alone (the pileup muon)',
          'HLT_L1AlwaysTrue': 'the L1 seed alone (always true)'}
groups = {'signal jets': [], 'a pileup object': [], 'the L1 seed alone': [], 'no object (zero bias, random, calibration, monitoring)': [],
          'scouting (DST_)': []}
for n in names:
    stem = re.sub(r'_v\d+$', '', n)
    if not (n.startswith('HLT_') or n.startswith('DST_')):
        continue
    if n.startswith('DST_'):
        groups['scouting (DST_)'].append(n)
    elif signalJets.match(n):
        groups['signal jets'].append(n)
    elif stem in pileupObject:
        groups['a pileup object'].append('%s: %s' % (n, pileupObject[stem]))
    elif stem in l1only:
        groups['the L1 seed alone'].append('%s: %s' % (n, l1only[stem]))
    else:
        groups['no object (zero bias, random, calibration, monitoring)'].append(n)
nacc = sum(len(v) for v in groups.values())
print('HLT_ and DST_ paths that accept the event: %d' % nacc)
for g, v in groups.items():
    print('  satisfied by %s: %d' % (g, len(v)))
    for x in v:
        print('     ' + x)
# the paths whose seed fired (they start their reconstruction): by kind, accepted or stopped, and by which filter type
from collections import Counter, OrderedDict
cats = [('single jets, dijets', r'^HLT_(AK8)?PFJet(Fwd)?\d+_v|^HLT_DiPFJetAve'), ('multijets', r'^HLT_QuadPFJet'),
        ('VBF (two jets far apart in eta)', r'^HLT_VBF'), ('missing energy, HT', r'MET|MHT|^HLT_HT|ECALHT'),
        ('muons', r'^HLT_(L2|L3|DoubleL2)?Mu|^HLT_Mu'), ('electrons', r'^HLT_Ele'), ('scouting', r'^DST_'),
        ('no object, L1 seed only, calibration', r'ZeroBias|Physics|Random|Calibration|HcalNZS|HcalPhiSym|PPSRandom|^HLT_L1'),
        ('others (isolated tracks, PPS, monitoring)', r'.')]
tab = OrderedDict((c, [0, 0, Counter()]) for c, _ in cats)
for l in lines('  path: '):
    d = kv(l)
    n, st = d.get('name', ''), d.get('stage')
    if not (n.startswith('HLT_') or n.startswith('DST_')) or st not in ('accept', 'filter'):
        continue
    for c, rx in cats:
        if re.search(rx, n):
            tab[c][0 if st == 'accept' else 1] += 1
            if st == 'filter':
                tab[c][2][d.get('lastType')] += 1
            break
print('\nthe paths whose L1 seed fired (they start their reconstruction), by kind:')
for c, (a, f, t) in tab.items():
    print('   %-42s accepted %2d, stopped %2d  %s' % (c, a, f, ', '.join('%s %d' % kv for kv in t.most_common())))
print('   %-42s accepted %2d, stopped %2d' % ('all', sum(v[0] for v in tab.values()), sum(v[1] for v in tab.values())))
print('other accepted paths (AlCa_, MC_, Dataset_, Status_): %d' % len([n for n in names if not (n.startswith('HLT_') or n.startswith('DST_'))]))
print('\nthe jet paths (ladders: threshold(stage), stage = accept, seed, or the filter that stopped the path)')
for l in lines('  leading jets:') + lines('  ladder '):
    print(l.strip())
for l in lines('  cut: '):
    print(l.strip())
for lab in ('hltSingleAK4CaloJet80', 'hltSinglePFJet110', 'hltSingleCaloJet110', 'hltSinglePFJet140', 'hltSinglePFFwdJet140', 'hltDiPFJetAve100ForHFJEC',
            'hltDiCaloJetAve110', 'hltMET90', 'hltL2fL1sMu3L1f0L2Filtered0', 'hltL3fL1sMu3L1f0L2f0L3Filtered3', 'hltMu3PFJet40MuCleaned'):
    for l in lines('  filter: label=%s ' % lab):
        print(l.strip()[:300])
for pn in ('HLT_PFJet110_v', 'HLT_PFJet140_v', 'HLT_PFJetFwd140_v', 'HLT_DiPFJetAve100_HFJEC_v', 'HLT_PFJet40_v', 'HLT_Mu3_PFJet40_v'):
    for l in lines('  path: name=%s' % pn):
        print(l.strip()[:220])
print('\ndatasets that accept the event:')
for l in lines('  dataset: '):
    if ' accept=1 ' in l:
        d = kv(l)
        print('   %-30s %s of %s paths' % (d.get('name'), d.get('fired'), d.get('paths')))
print('streams that accept the event:')
st = [kv(l).get('name') for l in lines('  stream: ') if ' accept=1 ' in l]
print('   %d: %s' % (len(st), ' '.join(st)))
print((lines('  accepted:') or [''])[0].strip())

# ---------------------------------------------------------------------------------------------------------------- 6
print('\n-- 6. The ledger: what the HLT sees, step by step (nothing is added to or removed from the recorded event)')
def jets(prefix, key):
    out = []
    for l in lines(prefix):
        d = kv(l)
        if d.get('gen', '-') != '-':
            out.append((num(d.get('gen')), num(d.get(key)), num(d.get('rawPt'))))
    return sorted(out, reverse=True)
cj = jets('  CaloJet ', 'correctedPt')
pj = jets('  PFJet ', 'pt')
l1 = [kv(l) for l in lines('  L1Jet:')]
print('%-52s %12s' % ('', 'GeV'))
print('%-52s %12.1f' % ('step 2: rec-hits, sum ET (ECAL + HCAL + HF)', sum(num(first('  rechits %s:' % r).get('sumEt')) for r in ('EB', 'EE', 'HB', 'HE', 'HF'))))
print('%-52s %12.1f' % ('step 2: calorimeter towers, sum ET', num(first('  towers all:').get('sumEt'))))
print('%-52s %12.1f' % ('step 2: rho of the calorimeter (per unit area)', num(re.search(r': ([\d.]+)', (lines('  rho calo') or [': 0'])[0]).group(1))))
print('%-52s %12.1f' % ('step 3: merged tracks, sum pT', num(first('  tracks merged: n').get('sumPt'))))
print('%-52s %12.1f' % ('step 3: tracks of the signal vertex, sum pT', num(first('  tracks from the signal vertex').get('sumPt'))))
print('%-52s %12.1f' % ('step 4: PF candidates, sum pT', num(first('  pfcands all:').get('sumPt'))))
print('%-52s %12.1f' % ('step 4: rho of particle flow (per unit area)', num(re.search(r': ([\d.]+)', (lines('  rho (') or [': 0'])[0]).group(1))))
print('the two jets of the signal (generator pT; L1 jet; calo jet raw -> corrected; PF jet raw -> corrected):')
for k, (g, c, craw) in enumerate(cj[:2]):
    pf = [x for x in pj if abs(x[0] - g) < 0.05]
    pfc, pfr = (pf[0][1], pf[0][2]) if pf else (0, 0)
    print('   gen %.1f: calo %.1f -> %.1f, PF %.1f -> %.1f' % (g, craw, c, pfr, pfc))
print('L1 jets of chapter 12 (unpacked by the HLT): %s' % ' '.join('%s' % d.get('et') for d in l1[:3]))
