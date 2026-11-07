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
import sys, math, collections
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

hHep = Handle('edm::HepMCProduct'); hTrk = Handle('std::vector<SimTrack>')
tot = collections.defaultdict(Sum); nev = 0
print('== g4Ledger %s %s (events %s)' % (label, fname.split('/')[-1], events or 'all'))
print('%-9s %7s %9s %8s | %7s %9s %8s | %s' % ('event', 'status1', 'E[GeV]', 'pT[GeV]', 'primary', 'E[GeV]', 'pT[GeV]', 'dropped: |eta|>5.5 (E) / p<40MeV (E) / neutrino (E) / other (E)'))
for ev in Events(fname):
    aux = ev.eventAuxiliary()
    key = (aux.run(), aux.luminosityBlock(), aux.event())
    if rng and not (rng[0] <= key <= rng[1]):
        continue
    ev.getByLabel('generatorSmeared', hHep)
    ev.getByLabel('g4SimHits', hTrk)
    hep = hHep.product().GetEvent()
    prim = {t.genpartIndex() for t in hTrk.product() if t.genpartIndex() > 0}
    s = collections.defaultdict(Sum)
    it = hep.particles_begin()
    while it != hep.particles_end():
        p = it.__deref__(); it.__preinc__()
        if p.status() != 1:
            continue
        m = p.momentum(); px, py, pz, e = m.px(), m.py(), m.pz(), m.e()
        pt = math.hypot(px, py); mom = math.sqrt(px * px + py * py + pz * pz)
        eta = 0.5 * math.log((mom + pz) / (mom - pz)) if mom > abs(pz) else (99. if pz > 0 else -99.)
        s['status1'].add(e, pt)
        if p.barcode() in prim:
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
    print('%-9s %7d %9.1f %8.1f | %7d %9.1f %8.1f | %d (%.1f) / %d (%.1f) / %d (%.1f) / %d (%.1f)' % (
        '%d:%d:%d' % key, s['status1'].n, s['status1'].e, s['status1'].pt, s['primary'].n, s['primary'].e, s['primary'].pt,
        s['eta'].n, s['eta'].e, s['p'].n, s['p'].e, s['nu'].n, s['nu'].e, s['other'].n, s['other'].e))
print('%-9s %7d %9.1f %8.1f | %7d %9.1f %8.1f | %d (%.1f) / %d (%.1f) / %d (%.1f) / %d (%.1f)' % (
    'total(%d)' % nev, tot['status1'].n, tot['status1'].e, tot['status1'].pt, tot['primary'].n, tot['primary'].e, tot['primary'].pt,
    tot['eta'].n, tot['eta'].e, tot['p'].n, tot['p'].e, tot['nu'].n, tot['nu'].e, tot['other'].n, tot['other'].e))
if tot['status1'].n:
    print('handed to Geant4: %.1f %% of the stable particles, %.1f %% of their energy, %.1f %% of their pT' % (
        100. * tot['primary'].n / tot['status1'].n, 100. * tot['primary'].e / max(tot['status1'].e, 1e-9), 100. * tot['primary'].pt / max(tot['status1'].pt, 1e-9)))
