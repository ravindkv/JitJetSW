#!/usr/bin/env python3
# yearCompare_12_L1T.py: the year comparison of chapter 12 (Rule I3 of the Constitution), run at the end of
# run_12_L1T_2024.sh. The same event with the 2024 settings (test/12_L1T/out2024/) and with the 2025 settings of the
# chapter (test/12_L1T/out/); FWLite on the ROOT files, the two ledgers, conddb dumps of the ECAL payloads.
#   python3 12_L1T/yearCompare_12_L1T.py 12_L1T/out 12_L1T/out2024 > 12_L1T/out2024/compare_12_L1T.txt
# Sections:
#   (1) the two ledgers (ledger_12_L1T.txt): every line that differs, 2024 then 2025
#   (2) the ECAL trigger primitives by tower ring (simEcalTriggerPrimitiveDigis of the two L1 files, made by the DIGI
#       job: 0.5 GeV per count): number above zero, sum and largest per ring, both years
#   (3) the algorithms that fire: in both years, only in 2024, only in 2025; the VBF, ETMHF and HTMHF families
#   (4) the CICADA anomaly score (simCaloStage2Layer1Summary:CICADAScore): each input with each model
#   (5) the ECAL conditions behind the primitives, from the payloads of the two global tags (conddb dump): per tower
#       ring the light a crystal delivers to the digitiser, (p/L)^alpha x L/p_ref (EcalHitResponse.cc, the laser
#       correction of EcalLaserDbService: p of EcalLaserAPDPNRatios, p_ref of ...Ref, alpha of EcalLaserAlphas, L of
#       EcalLinearCorrections; with p_ref = L = 1 in both tags it is p^alpha), the linearisation factor of the trigger-primitive generator mult_x12 / 2^shift_x12
#       (EcalTPGLinearizationConst), and their product: the primitive per GeV deposited, up to the common factors
#   (6) the menus and the algorithms that fire, classified (l1t_12_L1T.log of the two runs): the algorithms of each
#       menu by name (a name is counted in every group it matches), new and gone; the algorithms that fire without
#       looking at any object (external conditions, minimum-bias HF counts) and those satisfied by the two signal
#       jets alone (the expression of the algorithm is still true when a condition on objects counts only if one of its
#       combinations is made of the L1 jets within Delta R 0.4 of the two leading generator jets; the CICADA score is
#       an object)
import collections, math, os, re, shutil, subprocess, sys, tempfile
import xml.etree.ElementTree as ET
import ROOT
from DataFormats.FWLite import Events, Handle

out25, out24 = sys.argv[1], sys.argv[2]
YEARS = (('2024', out24), ('2025', out25))
print('== yearCompare_12_L1T: the event with the 2024 settings (%s) and with the 2025 settings (%s)' % (out24, out25))

# ------------------------------------------------------------------------------------------------------------- (1)
print('\n-- (1) the two ledgers (ledger_12_L1T.txt): the lines that differ')
import difflib
la = open(os.path.join(out24, 'ledger_12_L1T.txt')).read().splitlines()
lb = open(os.path.join(out25, 'ledger_12_L1T.txt')).read().splitlines()
section = ''
for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, la, lb, autojunk=False).get_opcodes():
    for l in la[:i2]:
        if l.startswith('-- '):
            section = l
    if tag == 'equal':
        continue
    print('  [%s]' % section[3:60])
    for l in la[i1:i2]:
        print('    2024| ' + l)
    for l in lb[j1:j2]:
        print('    2025| ' + l)

# ------------------------------------------------------------------------------------------------------------- (2)
print('\n-- (2) the ECAL trigger primitives by tower ring (0.5 GeV per count): n above 0 / sum ET / largest, GeV')
hTP = Handle('edm::SortedCollection<EcalTriggerPrimitiveDigi,edm::StrictWeakOrdering<EcalTriggerPrimitiveDigi> >')
rings = {}
for year, d in YEARS:
    r = collections.defaultdict(lambda: [0, 0., 0.])
    for ev in Events(os.path.join(d, 'step4_L1_12_L1T.root')):
        ev.getByLabel('simEcalTriggerPrimitiveDigis', hTP)
        for t in hTP.product():
            et = 0.5 * t.compressedEt()
            if et > 0:
                x = r[t.id().ieta()]; x[0] += 1; x[1] += et; x[2] = max(x[2], et)
        break
    rings[year] = r
def ringSum(r, keys):
    return [sum(r[k][0] for k in keys), sum(r[k][1] for k in keys), max([r[k][2] for k in keys] or [0])]
print('  %-12s %-24s %s' % ('ring', '2024', '2025'))
for a in range(1, 29):
    for s in (-1, 1):
        k = s * a
        print('  %-12d %-24s %s' % (k, '%d / %.1f / %.1f' % tuple(rings['2024'][k]), '%d / %.1f / %.1f' % tuple(rings['2025'][k])))
for name, keys in (('EB (1-17)', [s * a for a in range(1, 18) for s in (-1, 1)]), ('EE (18-28)', [s * a for a in range(18, 29) for s in (-1, 1)]),
                   ('rings 26-28', [s * a for a in (26, 27, 28) for s in (-1, 1)]), ('rings 27, 28', [s * a for a in (27, 28) for s in (-1, 1)])):
    print('  %-12s %-24s %s' % (name, '%d / %.1f / %.1f' % tuple(ringSum(rings['2024'], keys)), '%d / %.1f / %.1f' % tuple(ringSum(rings['2025'], keys))))

# ------------------------------------------------------------------------------------------------------------- (3)
print('\n-- (3) the algorithms that fire (section 6 of the two ledgers)')
def fired(lines):
    names, on = [], False
    for l in lines:
        if re.match(r'the \d+ algorithms that fire', l):
            on = True
            continue
        if on:
            m = re.match(r'\s+\d+ (L1_\S+)', l)
            if not m:
                break
            names.append(m.group(1))
    return names
f24, f25 = fired(la), fired(lb)
both = [n for n in f25 if n in f24]
print('  fired: 2024 %d, 2025 %d; in both %d, only 2024 %d, only 2025 %d' % (len(f24), len(f25), len(both), len(set(f24) - set(f25)), len(set(f25) - set(f24))))
for label, rx in (('VBF (L1_DoubleJet..._Mass...)', r'^L1_DoubleJet.*Mass'), ('ETMHF (L1_ETMHF...)', r'^L1_ETMHF'), ('HTMHF (L1_HTMHF...)', r'^L1_HTMHF'),
                  ('CICADA (L1_CICADA_...)', r'^L1_CICADA')):
    print('  %-32s 2024 %d, 2025 %d' % (label, sum(bool(re.match(rx, n)) for n in f24), sum(bool(re.match(rx, n)) for n in f25)))
print('  only 2024: ' + ' '.join(n for n in f24 if n not in f25))
print('  only 2025: ' + ' '.join(n for n in f25 if n not in f24))

# ------------------------------------------------------------------------------------------------------------- (4)
print('\n-- (4) the CICADA anomaly score (simCaloStage2Layer1Summary:CICADAScore, bunch crossing 0)')
hC = Handle('BXVector<float>')
def score(path):
    for ev in Events(path):
        ev.getByLabel(('simCaloStage2Layer1Summary', 'CICADAScore'), hC)
        p = hC.product()
        return p.at(0, 0) if p.size(0) else float('nan')
for label, path in (('2024 input, 2024 model v2p1p2', os.path.join(out24, 'step4_L1_12_L1T.root')),
                    ('2024 input, 2025 model v2p2p0', os.path.join(out24, 'cicada_input2024_v2p2p0.root')),
                    ('2025 input, 2024 model v2p1p2', os.path.join(out24, 'cicada_input2025_v2p1p2.root')),
                    ('2025 input, 2025 model v2p2p0', os.path.join(out25, 'step4_L1_12_L1T.root'))):
    print('  %-32s %.3f' % (label, score(path)))

# ------------------------------------------------------------------------------------------------------------- (5)
print('\n-- (5) the ECAL conditions by tower ring (conddb dump of the payloads of the two global tags)')
from Configuration.AlCa.autoCond import autoCond
gts = {'2024': autoCond['phase1_2024_realistic'], '2025': autoCond['phase1_2025_realistic']}
records = ('EcalLaserAPDPNRatiosRcd', 'EcalLaserAPDPNRatiosRefRcd', 'EcalLaserAlphasRcd', 'EcalLinearCorrectionsRcd', 'EcalTPGLinearizationConstRcd')
tags = {}
for year, gt in gts.items():
    lst = subprocess.run(['conddb', 'list', gt], capture_output=True, text=True).stdout.splitlines()
    for l in lst:
        w = l.split()
        if len(w) >= 3 and w[0] in records:
            tags[(year, w[0])] = w[2]
for year in gts:
    print('  %s (%s): %s' % (year, gts[year], ', '.join('%s %s' % (r.replace('Rcd', ''), tags[(year, r)]) for r in records)))
tmp = tempfile.mkdtemp(prefix='yearCompare_')
def dump(tag):
    path = os.path.join(tmp, tag + '.xml')
    if not os.path.exists(path):
        with open(path, 'w') as f:
            subprocess.run(['conddb', 'dump', tag], stdout=f, stderr=subprocess.DEVNULL, check=True, cwd=tmp)
    return path
def items(tag, kind):
    # the first 61200 + 14648 items (EB then EE, in hashed-index order) of the first map of the payload
    vals = []
    for _, el in ET.iterparse(dump(tag)):
        if el.tag != 'item':
            continue
        if kind == 'float' and len(el) == 0 and el.text and el.text.strip():
            vals.append(float(el.text))
        elif kind == 'p' and el.find('p2') is not None:
            vals.append(float(el.find('p2').text))
        elif kind == 'lin' and el.find('mult-x12') is not None:
            vals.append(int(el.find('mult-x12').text) / 2. ** int(el.find('shift-x12').text))
        el.clear()
        if len(vals) == 61200 + 14648:
            break
    if len(vals) != 61200 + 14648:
        raise RuntimeError('%s: %d items' % (tag, len(vals)))
    return vals
# the tower ring of every crystal: EB from EBDetId, EE from the crystal -> trigger tower map of the release
ROOT.gSystem.Load('libDataFormatsEcalDetId')
ttmap = {}
for l in open(os.path.join(os.environ['CMSSW_RELEASE_BASE'], 'src/Geometry/EcalMapping/data/EndCap_TTMap.txt')):
    w = l.split()
    if len(w) == 5:
        ttmap[(int(w[0]), int(w[1]), int(w[2]))] = int(w[3])
ring = [abs(ROOT.EBDetId.unhashIndex(i).tower_ieta()) for i in range(61200)]
for i in range(14648):
    d = ROOT.EEDetId.unhashIndex(i)
    ring.append(ttmap[(d.ix(), d.iy(), d.zside())])
light, lin = {}, {}
for year in gts:
    p = items(tags[(year, 'EcalLaserAPDPNRatiosRcd')], 'p')
    ref = items(tags[(year, 'EcalLaserAPDPNRatiosRefRcd')], 'float')
    alpha = items(tags[(year, 'EcalLaserAlphasRcd')], 'float')
    L = items(tags[(year, 'EcalLinearCorrectionsRcd')], 'p')
    print('  %s: linear correction L from %.3f to %.3f' % (year, min(L), max(L)))
    # EcalLaserDbService::getLaserCorrection: T = (p / p_ref) / (L / p_ref), correction = 1 / (T^alpha x L / p_ref);
    # EcalHitResponse multiplies the light by 1 / correction
    light[year] = [(p[i] / L[i]) ** alpha[i] * L[i] / ref[i] for i in range(len(p))]
    lin[year] = items(tags[(year, 'EcalTPGLinearizationConstRcd')], 'lin')
shutil.rmtree(tmp)
print('  per tower ring (both sides, crystal averages): light delivered 2024, 2025 | linearisation factor 2025 / 2024 |'
      ' primitive per GeV deposited 2025 / 2024 (= mean(light x lin) 2025 / 2024)')
for r in range(1, 29):
    idx = [i for i in range(len(ring)) if ring[i] == r]
    m = lambda v: sum(v[i] for i in idx) / len(idx)
    l24, l25 = m(light['2024']), m(light['2025'])
    q = m(lin['2025']) / m(lin['2024'])
    prim = sum(light['2025'][i] * lin['2025'][i] for i in idx) / sum(light['2024'][i] * lin['2024'][i] for i in idx)
    print('  ring %2d (%5d crystals): %.3f %.3f | %.3f | %.2f' % (r, len(idx), l24, l25, q, prim))

# ------------------------------------------------------------------------------------------------------------- (6)
print('\n-- (6) the menus and the fired algorithms, classified (l1t_12_L1T.log of the two runs)')
def menuGroups(names):
    g = collections.OrderedDict((k, 0) for k in ('Mu', 'EG', 'Tau', 'jets and sums only', 'CICADA', 'AXO'))
    for n in names:
        has = {k: k in n for k in ('Mu', 'EG', 'Tau')}
        for k in ('Mu', 'EG', 'Tau'):
            g[k] += has[k]
        if not any(has.values()) and any(x in n for x in ('Jet', 'HTT', 'HTM', 'ETM', 'ETT')):
            g['jets and sums only'] += 1
        g['CICADA'] += n.startswith('L1_CICADA_')
        g['AXO'] += n.startswith('L1_AXO_')
    return g
def dR(e1, p1, e2, p2):
    d = abs(p1 - p2)
    d = min(d, 2 * math.pi - d)
    return math.hypot(e1 - e2, d)
algos, classes = {}, {}
for year, d in YEARS:
    log = open(os.path.join(d, 'l1t_12_L1T.log')).read().splitlines()
    algos[year] = [re.search(r' name=(\S+)', l).group(1) for l in log if l.startswith('  algo: ')]
    gen = [tuple(float(x) for x in re.search(r'pt=([\d.]+) eta=([-\d.]+) phi=([-\d.]+)', l).groups()) for l in log if l.startswith('  GenJet: ')][:2]
    l1 = {}
    for l in log:
        m = re.match(r'  L1Jet (\d+): et=([\d.]+) eta=([-\d.]+) phi=([-\d.]+)', l)
        if m:
            l1[int(m.group(1))] = (float(m.group(2)), float(m.group(3)), float(m.group(4)))
    signal = {'Jet[%d]' % k for k, (et, eta, phi) in l1.items() if any(dR(eta, phi, g[1], g[2]) < 0.4 for g in gen)}
    noObj, sigOnly = [], []
    for l in log:
        if not l.startswith('  fired: '):
            continue
        name = re.search(r' name=(\S+)', l).group(1)
        expr = re.search(r' expression="([^"]*)"', l).group(1)
        external = {'External', 'MinBiasHFP0', 'MinBiasHFM0'}
        # every condition of the algorithm, in the order of the terms of its expression: is it true, is it a condition
        # on objects (beam-pickup and minimum-bias HF conditions are not: they are true for any collision of a filled
        # bunch crossing), and is it true with the two signal jets alone (one of its combinations is made of them only)
        conds = []
        for c in l.split(' | ')[1:]:
            m = re.match(r'(\S+)=([01]) ?(.*)', c)
            body = m.group(3).split(': ', 1)[1] if ': ' in m.group(3) else ''
            cc = [re.findall(r'(\w+\[\d+\]|External|MinBiasHF[PM]0|\b[A-Z]+\b)', x) for x in body.split(' or ')]
            if m.group(1).startswith('Cicada'):
                cc = [['CICADA']]  # the anomaly score of the Layer-1 regions: an object, without a combination
            objs = {o for x in cc for o in x}
            true = m.group(2) == '1'
            onObj = not (objs and objs <= external)
            conds.append((true, onObj, true and (not onObj or any(x and set(x) <= signal for x in cc))))
        if not any(onObj for true, onObj, sig in conds if true):
            noObj.append(name)
            continue
        # the expression evaluated with the signal-jets-only truth of every condition (AND, OR, NOT, parentheses)
        terms = [t for t in re.split(r'\s*(\(|\)|\bAND\b|\bOR\b|\bNOT\b)\s*', expr) if t and t.strip()]
        ops = {'AND': 'and', 'OR': 'or', 'NOT': 'not', '(': '(', ')': ')'}
        k, py = 0, []
        for t in terms:
            if t in ops:
                py.append(ops[t])
            else:
                py.append(str(conds[k][2]) if k < len(conds) else 'False')
                k += 1
        if k != len(conds):
            print('  WARNING %s %s: %d terms, %d conditions' % (year, name, k, len(conds)))
        if eval(' '.join(py)):
            sigOnly.append(name)
    classes[year] = (signal, noObj, sigOnly)
for year, d in YEARS:
    g = menuGroups(algos[year])
    signal, noObj, sigOnly = classes[year]
    nf = len(fired(la if year == '2024' else lb))
    print('  %s menu: %d algorithms; by name: %s' % (year, len(algos[year]), ', '.join('%s %d' % kv for kv in g.items())))
    print('  %s fired: %d; without any object %d (%s); on an object %d; satisfied by the two signal jets alone (%s) %d:'
          % (year, nf, len(noObj), ' '.join(noObj), nf - len(noObj), ' '.join(sorted(signal)), len(sigOnly)))
    print('      ' + ' '.join(sigOnly))
print('  menu 2025 against 2024: %d new, %d gone' % (len(set(algos['2025']) - set(algos['2024'])), len(set(algos['2024']) - set(algos['2025']))))
