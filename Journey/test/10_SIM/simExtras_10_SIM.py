#!/usr/bin/env python3
# simExtras_10_SIM.py: the FWLite numbers of chapter 10 that the analyzers do not print, read from the SIM files of
# chapters 09 and 10 (nothing is rerun). Written for the "numbers of the event" subsections of chapter_10_SIM.tex.
#   cd $CMSSW_BASE/src/JitJetSW/Journey/test && python3 10_SIM/simExtras_10_SIM.py > 10_SIM/out/extras_10_SIM.txt
# Sections: (1) the field: primaries that stop reaching the calorimeter boundary, the bending of a followed track;
# (2) the decays in flight: parents, daughters, decay radii; (3) the EM physics: conversions and bremsstrahlung,
# where they happen, the followed photon; (4) the hadronic physics: inelastic interactions in the tracker; (5) the
# b system: its energy and where it goes; (6) the followed photon: its crystals in two runs; (7) the vertices of
# the 193 mixed collisions (from simHits_mixed_10_SIM.log); (8) the library collisions with a vertex at |z| = 203 m.
import math, os, re, sys
from collections import Counter, defaultdict
import ROOT
from DataFormats.FWLite import Events, Handle

ROOT.gROOT.SetBatch(True)
here = os.path.dirname(os.path.abspath(__file__))
test = os.path.dirname(here)
out = os.path.join(here, 'out')
files = [('transport (09)', os.path.join(test, '09_G4', 'out', 'step2_G4_signal_09_G4.root'), 'SIM')] + \
        [(s, os.path.join(out, 'step2_SIM_%s_10_SIM.root' % s), 'SIMFULL') for s in ('B', 'B_Decay', 'B_Decay_EM', 'signal')]
NAMES = {11: 'e', 13: 'mu', 22: 'gamma', 211: 'pi+-', 321: 'K+-', 130: 'K0L', 310: 'K0S', 2212: 'p', 2112: 'n',
         3122: 'Lambda', 3112: 'Sigma-', 3222: 'Sigma+', 3312: 'Xi-', 3322: 'Xi0', 3334: 'Omega-', 12: 'nu_e', 14: 'nu_mu', 16: 'nu_tau'}
def name(pdg):
    return NAMES.get(abs(pdg), str(pdg)) + ('bar' if pdg < 0 and abs(pdg) in (2212, 2112, 3122, 3112, 3222, 3312, 3322, 3334) else '')
TRK_HITS = ['TrackerHitsPixelBarrelLowTof', 'TrackerHitsPixelBarrelHighTof', 'TrackerHitsPixelEndcapLowTof', 'TrackerHitsPixelEndcapHighTof',
            'TrackerHitsTIBLowTof', 'TrackerHitsTIBHighTof', 'TrackerHitsTIDLowTof', 'TrackerHitsTIDHighTof',
            'TrackerHitsTOBLowTof', 'TrackerHitsTOBHighTof', 'TrackerHitsTECLowTof', 'TrackerHitsTECHighTof']
CALO_HITS = ['EcalHitsEB', 'EcalHitsEE', 'EcalHitsES', 'HcalHits']
# the tracker volume used here: r < 110 cm and |z| < 280 cm (the outermost TOB / TEC elements of simHits_signal_10_SIM.log)
def inTracker(r, z):
    return r < 110. and abs(z) < 280.
def where(r, z):
    if r < 2.2 and abs(z) < 50: return 'beam pipe (r < 2.2 cm)'
    if r < 20. and abs(z) < 55: return 'pixel volume (r < 20 cm)'
    if inTracker(r, z): return 'strip volume (20 < r < 110 cm)'
    # the calorimeter faces of simHits_signal_10_SIM.log: EB r >= 129.1 cm, ES |z| >= 303.8 cm, EE |z| >= 317.6 cm
    if r >= 129.0 or abs(z) >= 317.5: return 'in the calorimeters (r > 129 cm or |z| > 317 cm)'
    if abs(z) >= 303.5: return 'in the preshower (303.5 < |z| < 317.5 cm)'
    return 'between the tracker and the calorimeters'
# Geant4 process sub-types (G4EmProcessSubType, G4HadronicProcessType, G4DecayProcessType), as in plugins/SimAnalyzer.cc
def proc(t):
    if t == 0: return 'primary'
    if 201 <= t <= 239: return 'decay'
    if t == 2: return 'ionisation'
    if t == 3: return 'bremsstrahlung'
    if t in (4, 14, 15, 49): return 'conversion'
    if t == 13: return 'compton'
    if t == 12: return 'photoelectric'
    if t in (5, 6, 7, 18): return 'annihilation'
    if t <= 50: return 'em other'
    if t == 111: return 'hadron elastic'
    if t in (116, 121): return 'hadron inelastic'
    if t in (131, 132): return 'capture'
    if t in (151, 152): return 'at rest'
    return 'other %d' % t

def load(path, process):
    # the Events object and the handles are kept in d: the products live as long as they do
    d = {'events': Events(path), 'handles': []}
    ev = next(iter(d['events']))
    d['event'] = ev
    def get(typ, label, inst=''):
        hd = Handle(typ)
        d['handles'].append(hd)
        ev.getByLabel(label, inst, process if label == 'g4SimHits' else '', hd)
        if not hd.isValid():
            sys.exit('missing %s %s:%s in %s' % (typ, label, inst, path))
        return hd.product()
    d['gens'] = get('std::vector<reco::GenParticle>', 'genParticles')
    d['barcodes'] = list(get('std::vector<int>', 'genParticles'))
    d['tracks'] = list(get('std::vector<SimTrack>', 'g4SimHits'))
    d['vertices'] = list(get('std::vector<SimVertex>', 'g4SimHits'))
    d['trk'] = {c: list(get('std::vector<PSimHit>', 'g4SimHits', c)) for c in TRK_HITS}
    d['calo'] = {c: list(get('std::vector<PCaloHit>', 'g4SimHits', c)) for c in CALO_HITS}
    return d

def hsub(idx):
    # HcalHits are in the HCAL test numbering of the simulation (HCalSD.TestNumberingScheme, run2_HCAL_2017):
    # DataFormats/HcalDetId/src/HcalTestNumbering.cc, det = (idx >> 28) & 15: 1 HB, 2 HE, 3 HO, 4 HF
    return {1: 'HB', 2: 'HE', 3: 'HO', 4: 'HF'}.get((idx >> 28) & 15, 'H?')

def rz(v):
    p = v.position()
    return math.hypot(p.x(), p.y()), p.z()

def analyse(d):
    tracks, vertices, gens = d['tracks'], d['vertices'], d['gens']
    byId = {t.trackId(): i for i, t in enumerate(tracks)}
    # ancestor primary of every SimTrack: walk vertex -> parent track id
    anc = {}
    def ancestor(i):
        path = []
        cur = i
        while True:
            if cur in anc:
                res = anc[cur]; break
            t = tracks[cur]
            if t.genpartIndex() > 0:
                res = cur; break
            v = t.vertIndex()
            if v < 0 or v >= len(vertices) or vertices[v].noParent():
                res = -1; break
            p = byId.get(vertices[v].parentIndex())
            if p is None:
                res = -1; break
            path.append(cur)
            cur = p
        for k in path + [cur]:
            anc[k] = res
        return res
    for i in range(len(tracks)):
        ancestor(i)
    # b / bbar systems: status-1 descendants of the B- [829] and of the B*+ [968] (simAnalyzer_signal_10_SIM.log)
    sysOfGen = {}
    for k, root in ((0, 829), (1, 968)):
        stack = [gens[root]]
        while stack:
            g = stack.pop()
            for j in range(g.numberOfDaughters()):
                dau = g.daughter(j)
                stack.append(dau)
                if dau.status() == 1:
                    sysOfGen[(round(dau.pt(), 6), round(dau.eta(), 6), dau.pdgId())] = k
    bc2gen = {b: i for i, b in enumerate(d['barcodes'])}
    sysOfTrack = {}
    for i, t in enumerate(tracks):
        if t.genpartIndex() > 0 and t.genpartIndex() in bc2gen:
            g = gens[bc2gen[t.genpartIndex()]]
            k = sysOfGen.get((round(g.pt(), 6), round(g.eta(), 6), g.pdgId()))
            if k is not None:
                sysOfTrack[i] = k
    return byId, anc, sysOfTrack

def secondaries(d, byId, kinds):
    """(vertex, parent track index or None, list of daughter track indices) for the vertices of the given creator processes"""
    tracks, vertices = d['tracks'], d['vertices']
    dau = defaultdict(list)
    for i, t in enumerate(tracks):
        if t.genpartIndex() > 0:
            continue
        v = t.vertIndex()
        if 0 <= v < len(vertices) and proc(vertices[v].processType()) in kinds:
            dau[v].append(i)
    res = []
    for v, ds in sorted(dau.items()):
        vx = vertices[v]
        p = None if vx.noParent() else byId.get(vx.parentIndex())
        res.append((vx, p, ds))
    return res

data = {}
for stage, path, process in files:
    data[stage] = load(path, process)
    data[stage]['an'] = analyse(data[stage])
print('== simExtras 10_SIM: FWLite on the SIM files of chapters 09 and 10 (event 1)')
for stage, path, _ in files:
    d = data[stage]
    print('  %-15s %s: %d SimTracks, %d SimVertices, %d primaries' % (stage, os.path.relpath(path, test), len(d['tracks']), len(d['vertices']),
                                                                  sum(1 for t in d['tracks'] if t.genpartIndex() > 0)))

# ---------------------------------------------------------------------------------------------------- (1) the field
print('\n-- (1) the field (stage B against the transport-only run of chapter 09)')
def primaries(d):
    return {t.genpartIndex(): t for t in d['tracks'] if t.genpartIndex() > 0}
P0, P1 = primaries(data['transport (09)']), primaries(data['B'])
lost = [P0[b] for b in P0 if P0[b].crossedBoundary() and b in P1 and not P1[b].crossedBoundary()]
print('  primaries reaching the boundary: transport %d, B %d; reaching without field but not with it: %d' % (
    sum(t.crossedBoundary() for t in P0.values()), sum(t.crossedBoundary() for t in P1.values()), len(lost)))
for t in sorted(lost, key=lambda t: t.momentum().pt()):
    m = t.momentum()
    print('    %-6s charge %+d  pT = %.3f GeV  eta = %+.2f' % (name(t.type()), t.charge(), m.pt(), m.eta()))
ch = [t for t in P1.values() if t.charge() != 0]
loopB = [t for t in ch if t.momentum().pt() < 0.735 and abs(t.momentum().eta()) < 1.479]
print('  charged primaries: %d; with pT < 0.735 GeV and |eta| < 1.479 (would curl inside r = 1.29 m): %d, of which reaching: %d' % (
    len(ch), len(loopB), sum(t.crossedBoundary() for t in loopB)))
ch25 = [t for t in ch if abs(t.momentum().eta()) < 2.5]
print('  charged primaries with |eta| < 2.5: %d; pT < 0.735 GeV: %d; median pT %.2f GeV' % (
    len(ch25), sum(t.momentum().pt() < 0.735 for t in ch25), sorted(t.momentum().pt() for t in ch25)[len(ch25) // 2]))
# tracker hits of slow charged primaries in the two runs
def hitsOf(d, sel):
    ids = {t.trackId() for t in d['tracks'] if t.genpartIndex() > 0 and sel(t)}
    return sum(1 for c in TRK_HITS for h in d['trk'][c] if h.trackId() in ids)
slow = lambda t: t.charge() != 0 and t.momentum().pt() < 0.735
fast = lambda t: t.charge() != 0 and t.momentum().pt() >= 0.735
neutral = lambda t: t.charge() == 0
for lab, sel in (('charged, pT < 0.735', slow), ('charged, pT >= 0.735', fast), ('neutral', neutral)):
    print('  tracker hits of the %-20s primaries: transport %5d, B %5d' % (lab, hitsOf(data['transport (09)'], sel), hitsOf(data['B'], sel)))
# the bending of the followed pi+ (pT 13.3 GeV, eta -1.54) and of the softest charged primary reaching the boundary
for lab, t in (('pi+ followed', max((t for t in ch if t.type() == 211 and abs(t.momentum().eta() + 1.54) < 0.01), key=lambda t: t.momentum().pt())),):
    m, pos = t.momentum(), t.trackerSurfacePosition()
    dphi = math.atan2(pos.y(), pos.x()) - m.phi()
    dphi = (dphi + math.pi) % (2 * math.pi) - math.pi
    print('  %s: pT = %.2f GeV, eta = %.3f, phi at the vertex %.4f, boundary point r = %.1f cm z = %.1f cm phi %.4f: dphi = %.4f rad (%.2f cm at that r)' % (
        lab, m.pt(), m.eta(), m.phi(), math.hypot(pos.x(), pos.y()), pos.z(), math.atan2(pos.y(), pos.x()), dphi, dphi * math.hypot(pos.x(), pos.y())))

# ---------------------------------------------------------------------------------------------------- (2) the decays
print('\n-- (2) the decays in flight (stage B_Decay)')
d = data['B_Decay']; byId, anc, sysT = d['an']; tracks = d['tracks']
dec = secondaries(d, byId, ('decay',))
print('  %d decay vertices with a stored daughter, %d daughters' % (len(dec), sum(len(x[2]) for x in dec)))
chan = Counter()
radii = Counter()
for vx, p, ds in dec:
    par = name(tracks[p].type()) if p is not None else '?'
    chan[(par, ' '.join(sorted(name(tracks[i].type()) for i in ds)))] += 1
    r, z = rz(vx)
    radii[where(r, z)] += 1
for (par, dd), n in chan.most_common():
    print('    %-8s -> %-24s %3d' % (par, dd, n))
for w, n in radii.most_common():
    print('    decay vertex %-32s %3d' % (w, n))
bdec = [(vx, p, ds) for vx, p, ds in dec if p is not None and anc.get(p, -1) in sysT and sysT[anc[p]] == 0]
for vx, p, ds in bdec:
    r, z = rz(vx); m = tracks[p].momentum()
    print('  b system: %s of pT %.2f GeV (E %.2f) decayed at r = %.1f cm, z = %.1f cm into %s' % (
        name(tracks[p].type()), m.pt(), m.e(), r, z, ' '.join(name(tracks[i].type()) for i in ds)))

# ---------------------------------------------------------------------------------------------------- (3) EM physics
for stage in ('B_Decay_EM', 'signal'):
    print('\n-- (3) the electromagnetic physics (stage %s)' % stage)
    d = data[stage]; byId, anc, sysT = d['an']; tracks = d['tracks']
    for kind in ('conversion', 'bremsstrahlung'):
        vs = secondaries(d, byId, (kind,))
        loc = Counter(where(*rz(vx)) for vx, p, ds in vs)
        print('  %s: %d vertices, %d stored secondaries; %s' % (kind, len(vs), sum(len(x[2]) for x in vs),
                                                              ', '.join('%s %d' % kv for kv in loc.most_common())))
        if kind == 'conversion':
            fromPrim = sum(1 for vx, p, ds in vs if p is not None and tracks[p].genpartIndex() > 0)
            print('    of which conversions of a primary photon: %d' % fromPrim)
    # primary photons in the tracker acceptance and how many convert inside the tracker volume
    conv = secondaries(d, byId, ('conversion',))
    convParent = {}
    for vx, p, ds in conv:
        if p is not None:
            r, z = rz(vx)
            convParent.setdefault(p, (r, z))
    for lo, hi in ((0., 1.0), (1.0, 2.5), (0., 2.5)):
        ph = [i for i, t in enumerate(tracks) if t.genpartIndex() > 0 and t.type() == 22 and lo <= abs(t.momentum().eta()) < hi and t.momentum().e() > 1.0]
        c = [i for i in ph if i in convParent and inTracker(*convParent[i])]
        print('  primary photons with E > 1 GeV, %.1f <= |eta| < %.1f: %d, converted inside the tracker volume: %d (%.0f %%)' % (
            lo, hi, len(ph), len(c), 100. * len(c) / max(1, len(ph))))
    # the followed photon (pT 17.7 GeV, eta -1.57) and electron (pT 5.0 GeV, eta -1.43)
    for pdg, pt in ((22, 17.7), (11, 5.0)):
        cand = [i for i, t in enumerate(tracks) if t.genpartIndex() > 0 and abs(t.type()) == pdg and abs(t.momentum().pt() - pt) < 0.06]
        for i in cand:
            t = tracks[i]; m = t.momentum()
            s = '  followed %s: pT = %.2f GeV, eta = %.3f, E = %.2f GeV' % (name(t.type()), m.pt(), m.eta(), m.e())
            if i in convParent:
                s += '; converts at r = %.1f cm, z = %.1f cm (%s)' % (convParent[i][0], convParent[i][1], where(*convParent[i]))
            print(s)

# ---------------------------------------------------------------------------------------------------- (4) hadronic
print('\n-- (4) the hadronic physics (stage signal)')
d = data['signal']; byId, anc, sysT = d['an']; tracks = d['tracks']
had = secondaries(d, byId, ('hadron inelastic', 'hadron elastic'))
loc = Counter(where(*rz(vx)) for vx, p, ds in had)
print('  hadronic vertices with a stored secondary: %d, %d secondaries; %s' % (len(had), sum(len(x[2]) for x in had), ', '.join('%s %d' % kv for kv in loc.most_common())))
print('  their secondaries by kind: %s' % ', '.join('%s %d' % kv for kv in Counter(name(tracks[i].type()) for vx, p, ds in had for i in ds).most_common(8)))
print('  parents: %s' % ', '.join('%s %d' % kv for kv in Counter(name(tracks[p].type()) if p is not None else '?' for vx, p, ds in had).most_common(6)))
hadParent = {}
for vx, p, ds in had:
    if p is not None:
        hadParent.setdefault(p, rz(vx))
for lo, hi in ((0., 1.0), (1.0, 2.5), (0., 2.5)):
    hs = [i for i, t in enumerate(tracks) if t.genpartIndex() > 0 and abs(t.type()) in (211, 321, 2212) and lo <= abs(t.momentum().eta()) < hi and t.momentum().pt() > 0.5]
    c = [i for i in hs if i in hadParent and inTracker(*hadParent[i])]
    print('  primary charged pi/K/p with pT > 0.5 GeV, %.1f <= |eta| < %.1f: %d, with a nuclear interaction inside the tracker volume: %d (%.0f %%)' % (
        lo, hi, len(hs), len(c), 100. * len(c) / max(1, len(hs))))

for pdg, pt, eta in ((310, 15.7, -3.77), (211, 30.6, -3.74)):
    for i, t in enumerate(tracks):
        m = t.momentum()
        if t.genpartIndex() > 0 and t.type() == pdg and abs(m.pt() - pt) < 0.06 and abs(m.eta() - eta) < 0.02:
            ends = [(proc(vx.processType()), rz(vx)) for vx in d['vertices'] if not vx.noParent() and vx.parentIndex() == t.trackId()]
            print('  followed %s of the bbar jet: pT = %.2f GeV, E = %.1f GeV, eta = %.3f; vertices it ends in or creates: %s' % (
                name(t.type()), m.pt(), m.e(), m.eta(), '; '.join('%s at r = %.1f cm, z = %.1f cm' % (p, r, z) for p, (r, z) in ends[:4]) or 'none'))

# ---------------------------------------------------------------------------------------------------- (5) the b system
tot = Counter()
for h in data['signal']['calo']['HcalHits']:
    tot[hsub(h.id())] += h.energy()
print('\n-- check: HcalHits of the signal by subdetector (test numbering): %s' % ', '.join('%s %.4f' % kv for kv in sorted(tot.items())))
print('\n-- (5) the b system (B- decay products) and the bbar system (B*+ decay products), stage signal')
for k, lab in ((0, 'b'), (1, 'bbar')):
    prim = [i for i, s in sysT.items() if s == k]
    E = sum(tracks[i].momentum().e() for i in prim)
    print('  %s system: %d primaries, E = %.1f GeV: %s' % (lab, len(prim), E, ', '.join('%s %.1f' % (name(tracks[i].type()), tracks[i].momentum().e())
                                                                                           for i in sorted(prim, key=lambda i: -tracks[i].momentum().e()))))
    eSub = Counter(); nCell = Counter()
    for c in CALO_HITS:
        cells = defaultdict(float)
        for h in d['calo'][c]:
            j = byId.get(h.geantTrackId())
            if j is None:
                continue
            a = anc.get(j, -1)
            if a in sysT and sysT[a] == k:
                sub = c.replace('EcalHits', '')
                if c == 'HcalHits':
                    sub = hsub(h.id())
                eSub[sub] += h.energy()
                cells[(sub, h.id())] += h.energy()
        for (sub, _), e in cells.items():
            nCell[sub] += 1
    print('    deposits: %s' % ', '.join('%s %.3f (%d cells)' % (s, eSub[s], nCell[s]) for s in ('EB', 'EE', 'ES', 'HB', 'HE', 'HO', 'HF') if s in eSub))

# ---------------------------------------------------------------------------------------------------- (6) followed primaries
for stage in ('B_Decay_EM', 'signal'):
    print('\n-- (6) the followed photon: its own deposits in the EB/EE crystals (stage %s)' % stage)
    d = data[stage]; byId, anc, sysT = d['an']; tracks = d['tracks']
    i = [i for i, t in enumerate(tracks) if t.genpartIndex() > 0 and t.type() == 22 and abs(t.momentum().pt() - 17.7) < 0.06][0]
    cells = defaultdict(float)
    for c in ('EcalHitsEB', 'EcalHitsEE', 'EcalHitsES'):
        for h in d['calo'][c]:
            j = byId.get(h.geantTrackId())
            if j is not None and anc.get(j, -1) == i:
                cells[(c, h.id())] += h.energy()
    es = sorted((e for (c, _), e in cells.items() if c != 'EcalHitsES'), reverse=True)
    tot = sum(es)
    n90 = next(k + 1 for k in range(len(es)) if sum(es[:k + 1]) >= 0.9 * tot)
    nTrk = sum(1 for c in TRK_HITS for h in d['trk'][c] if byId.get(h.trackId()) is not None and anc.get(byId[h.trackId()], -1) == i)
    print('  photon pT %.2f E %.2f: %d EB/EE cells, %.2f GeV (%.0f %% of E); hottest %.2f GeV (%.0f %%), 5 hottest %.2f GeV (%.0f %%), 90 %% in %d cells; cells above 1 GeV: %d; ES %.4f GeV; tracker hits of its descendants (stored tracks) %d' % (
        tracks[i].momentum().pt(), tracks[i].momentum().e(), len(es), tot, 100 * tot / tracks[i].momentum().e(), es[0], 100 * es[0] / tot, sum(es[:5]), 100 * sum(es[:5]) / tot, n90,
        sum(e > 1 for e in es), sum(e for (c, _), e in cells.items() if c == 'EcalHitsES'), nTrk))

# ---------------------------------------------------------------------------------------------------- (7) the vertices of the mixing
print('\n-- (7) the 193 mixed collisions (simHits_mixed_10_SIM.log, lines "pileup event")')
zs = defaultdict(list); prims = defaultdict(list)
for line in open(os.path.join(out, 'simHits_mixed_10_SIM.log')):
    m = re.match(r'\s+pileup event (\d+) in bx (-?\d+): (\d+) primaries, (\d+) SimTracks, (\d+) SimVertices, vertex z = (\S+) cm', line)
    if m:
        zs[int(m.group(2))].append(float(m.group(6))); prims[int(m.group(2))].append(int(m.group(3)))
allz = [z for b in zs for z in zs[b] if abs(z) < 100]
far = [(b, k + 1, z) for b in zs for k, z in enumerate(zs[b]) if abs(z) >= 100]
mean = sum(allz) / len(allz)
rms = math.sqrt(sum((z - mean) ** 2 for z in allz) / len(allz))
print('  %d collisions with |z| < 1 m: mean z = %.2f cm, rms %.2f cm, min %.2f, max %.2f; within |z - 3.11| < 0.1 cm of the signal vertex: %d' % (
    len(allz), mean, rms, min(allz), max(allz), sum(abs(z - 3.11) < 0.1 for z in allz)))
for b, k, z in far:
    print('  outside: bx %+d pileup event %d at z = %.0f cm (%d primary)' % (b, k, z, prims[b][k - 1]))
for b in sorted(prims):
    p = sorted(prims[b])
    med = p[len(p) // 2] if len(p) % 2 else 0.5 * (p[len(p) // 2 - 1] + p[len(p) // 2])
    print('  bx %+d: %d collisions, primaries per collision: min %d, median %g, max %d, sum %d' % (b, len(p), p[0], med, p[-1], sum(p)))

# ---------------------------------------------------------------------------------------------------- (8) far vertices
print('\n-- (8) the library collisions with a vertex at |z| = 203 m (step2_SIM_pileup_10_SIM.root, library events 12, 124, 179)')
lib = Events(os.path.join(out, 'step2_SIM_pileup_10_SIM.root'))
ht, hv = Handle('std::vector<SimTrack>'), Handle('std::vector<SimVertex>')
for k, e in enumerate(lib):
    if e.eventAuxiliary().event() not in (12, 124, 179):
        continue
    e.getByLabel('g4SimHits', ht); e.getByLabel('g4SimHits', hv)
    for t in ht.product():
        v = hv.product()[t.vertIndex()].position() if t.vertIndex() >= 0 else None
        print('  library event %d: SimTrack pdg %d, E = %.0f GeV, primary %s, vertex z = %s cm' % (
            e.eventAuxiliary().event(), t.type(), t.momentum().e(), t.genpartIndex() > 0, '%.0f' % v.z() if v else '-'))
