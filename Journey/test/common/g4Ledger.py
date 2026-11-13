#!/usr/bin/env python3
# g4Ledger.py: the ledger of the hand-over to Geant4 (chapter 09, G4), FWLite: which of the stable generator particles
# of an event Geant4 accepted as primaries and which it dropped, with the energy and pT that went with them.
#
#   python3 g4Ledger.py <SIM file> [--events 1:1:1 | --events 1:1:67-1:1:121] [--label signal]
#
# For every selected event the HepMC record (generatorSmeared) gives the status 1 particles; the SimTracks of g4SimHits
# with a generator link (genpartIndex = HepMC barcode) are the primaries Geant4 tracked. The particles without a
# SimTrack are classified with the cuts of the SIM step (SimG4Core/Application/python/g4SimHits_cfi.py, Generator PSet
# and StackingAction): |eta| > 5.5 (MinEtaCut / MaxEtaCut), p < 40 MeV (MinPCut), neutrinos (TrackNeutrino = False),
# other (e.g. a decay vertex beyond the RDecLenCut / LDecLenCut volume). One line per event, then the totals.
# PPS: the forward protons that the beam-line transport (LHCTransport, PPSSimTrackProducer) carries to the Roman pots
# and hands to Geant4 there, at |z| = 203 m. Their SimTracks carry the index of the transported HepMC record, not the
# barcode of the generator proton, so they are recognised by their vertex (|z| > 100 m) and matched to the stable
# generator proton of the same side with the nearest energy; they are a column of their own, not primaries of the
# vertex and not dropped.
import sys, math, collections
import ROOT
from DataFormats.FWLite import Events, Handle

args = sys.argv[1:]
def opt(name, default=None):
    if name in args:
        i = args.index(name); v = args[i + 1]; del args[i:i + 2]; return v
    return default
events = opt('--events', '')
label = opt('--label', '')
fname = args[0]

def parse_range(s):
    if not s:
        return None
    lo, _, hi = s.partition('-')
    r, l, e = (int(x) for x in lo.split(':'))
    if hi:
        r2, l2, e2 = (int(x) for x in hi.split(':'))
    else:
        r2, l2, e2 = r, l, e
    return (r, l, e), (r2, l2, e2)
rng = parse_range(events)
neutrinos = {12, 14, 16}

class Sum:
    def __init__(self):
        self.n = 0; self.e = 0.; self.pt = 0.
    def add(self, e, pt):
        self.n += 1; self.e += e; self.pt += pt

hHep = Handle('edm::HepMCProduct'); hTrk = Handle('std::vector<SimTrack>'); hVtx = Handle('std::vector<SimVertex>')
tot = collections.defaultdict(Sum); nev = 0
# the stable particles, the primaries and the PPS protons by region and by kind (totals of the selected events)
regions = (('abs(eta) < 2.5', 0., 2.5), ('2.5 to 3.0', 2.5, 3.0), ('3.0 to 5.2', 3.0, 5.2), ('5.2 to 5.5', 5.2, 5.5), ('above 5.5', 5.5, 1e9))
kinds = ('charged hadrons', 'neutral hadrons', 'photons', 'electrons', 'muons', 'neutrinos')
byRegion = collections.defaultdict(Sum); byKind = collections.defaultdict(Sum)
pdgDB = ROOT.TDatabasePDG.Instance()
def kindOf(pdg):
    a = abs(pdg)
    if a in neutrinos: return 'neutrinos'
    if a == 22: return 'photons'
    if a == 11: return 'electrons'
    if a == 13: return 'muons'
    q = pdgDB.GetParticle(pdg)
    return 'charged hadrons' if q and q.Charge() != 0 else 'neutral hadrons'
print('== g4Ledger %s %s (events %s)' % (label, fname.split('/')[-1], events or 'all'))
print('%-9s %7s %9s %8s | %7s %9s %8s | %s' % ('event', 'status1', 'E[GeV]', 'pT[GeV]', 'primary', 'E[GeV]', 'pT[GeV]',
      'dropped: |eta|>5.5 (E) / p<40MeV (E) / neutrino (E) / other (E) | PPS (E)'))
worst = 0.  # the largest relative energy difference of a PPS match
for ev in Events(fname):
    aux = ev.eventAuxiliary()
    key = (aux.run(), aux.luminosityBlock(), aux.event())
    if rng and not (rng[0] <= key <= rng[1]):
        continue
    ev.getByLabel('generatorSmeared', hHep)
    ev.getByLabel('g4SimHits', hTrk)
    ev.getByLabel('g4SimHits', hVtx)
    hep = hHep.product().GetEvent()
    vtx = hVtx.product()
    def far(t):
        return t.vertIndex() >= 0 and abs(vtx[t.vertIndex()].position().z()) > 10000.  # cm: the Roman pots
    prim = {t.genpartIndex() for t in hTrk.product() if t.genpartIndex() > 0 and not far(t)}
    ppsTracks = [t.momentum() for t in hTrk.product() if t.genpartIndex() > 0 and far(t)]
    gen = []
    s = collections.defaultdict(Sum)
    it = hep.particles_begin()
    while it != hep.particles_end():
        p = it.__deref__(); it.__preinc__()
        if p.status() != 1:
            continue
        gen.append(p)
    # the generator protons the PPS transport handed over: for every PPS SimTrack the stable proton of the same side
    # with the nearest energy
    pps = set()
    for m in ppsTracks:
        cands = [p for p in gen if p.pdg_id() == 2212 and p.momentum().pz() * m.pz() > 0 and p.barcode() not in prim and p.barcode() not in pps]
        if cands:
            best = min(cands, key=lambda p: abs(p.momentum().e() - m.e()))
            pps.add(best.barcode())
            worst = max(worst, abs(best.momentum().e() - m.e()) / m.e())
    for p in gen:
        m = p.momentum(); px, py, pz, e = m.px(), m.py(), m.pz(), m.e()
        pt = math.hypot(px, py); mom = math.sqrt(px * px + py * py + pz * pz)
        eta = 0.5 * math.log((mom + pz) / (mom - pz)) if mom > abs(pz) else (99. if pz > 0 else -99.)
        s['status1'].add(e, pt)
        what = 'pps' if p.barcode() in pps else ('primary' if p.barcode() in prim else 'dropped')
        reg = next(r[0] for r in regions if r[1] <= abs(eta) < r[2])
        for k in ('status1', what):
            byRegion[(reg, k)].add(e, pt); byKind[(kindOf(p.pdg_id()), k)].add(e, pt)
        if p.barcode() in pps:
            s['pps'].add(e, pt)
        elif p.barcode() in prim:
            s['primary'].add(e, pt)
        elif abs(eta) > 5.5:
            s['eta'].add(e, pt)
        elif mom < 0.04:
            s['p'].add(e, pt)
        elif abs(p.pdg_id()) in neutrinos:
            s['nu'].add(e, pt)
        else:
            s['other'].add(e, pt)
    for k, v in s.items():
        tot[k].n += v.n; tot[k].e += v.e; tot[k].pt += v.pt
    nev += 1
    print('%-9s %7d %9.1f %8.1f | %7d %9.1f %8.1f | %d (%.1f) / %d (%.1f) / %d (%.1f) / %d (%.1f) | %d (%.1f)' % (
        '%d:%d:%d' % key, s['status1'].n, s['status1'].e, s['status1'].pt, s['primary'].n, s['primary'].e, s['primary'].pt,
        s['eta'].n, s['eta'].e, s['p'].n, s['p'].e, s['nu'].n, s['nu'].e, s['other'].n, s['other'].e, s['pps'].n, s['pps'].e))
print('%-9s %7d %9.1f %8.1f | %7d %9.1f %8.1f | %d (%.1f) / %d (%.1f) / %d (%.1f) / %d (%.1f) | %d (%.1f)' % (
    'total(%d)' % nev, tot['status1'].n, tot['status1'].e, tot['status1'].pt, tot['primary'].n, tot['primary'].e, tot['primary'].pt,
    tot['eta'].n, tot['eta'].e, tot['p'].n, tot['p'].e, tot['nu'].n, tot['nu'].e, tot['other'].n, tot['other'].e, tot['pps'].n, tot['pps'].e))
if tot['pps'].n:
    print('PPS: %d forward protons with %.1f GeV handed to Geant4 at the Roman pots (largest energy difference of a match %.1e)' % (
        tot['pps'].n, tot['pps'].e, worst))
print('by region and by kind, n / E[GeV] / pT[GeV]: %-32s | %-32s | %s' % ('stable', 'primaries (from the vertex)', 'PPS (at the Roman pots)'))
fmt = lambda v: '%d / %.1f / %.1f' % (v.n, v.e, v.pt)
for name, d, keys in (('region', byRegion, [r[0] for r in regions]), ('kind', byKind, kinds)):
    for k in keys:
        if d[(k, 'status1')].n:
            print('  %-16s %-34s | %-32s | %s' % (k, fmt(d[(k, 'status1')]), fmt(d[(k, 'primary')]), fmt(d[(k, 'pps')])))
if tot['status1'].n:
    print('handed to Geant4: %.1f %% of the stable particles, %.1f %% of their energy, %.1f %% of their pT' % (
        100. * tot['primary'].n / tot['status1'].n, 100. * tot['primary'].e / max(tot['status1'].e, 1e-9), 100. * tot['primary'].pt / max(tot['status1'].pt, 1e-9)))
