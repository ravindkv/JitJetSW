#!/usr/bin/env python3
# g4Extras_09_G4.py: the extra numbers of chapter 09 (G4) that the analyzers do not print, read with FWLite from the
# transport-only SIM files of the chapter (no field, no physics: every primary flies in a straight line):
#   (1) where the primaries cross the tracker/calorimeter boundary (SimTrack::trackerSurfacePosition, filled by
#       CaloTrkProcessing), by surface of the geometry (Geometry/CMSCommonData/data/cms/2021/v3/cms.xml: TrackCalorR =
#       123.3 cm, TrackBeamZ2 = 293.2 cm, TrackBeamZ2 + PIXNoseLength = 355.15 cm), for the signal, its b and bbar systems
#       and one pileup collision;
#   (2) the eta cut of the Generator (SimG4Core/Generators/src/Generator.cc) as a line: for the stable particles of the
#       signal near |eta| = 5.5, the radius of the straight line from their production vertex at |z| = 2.9 cm * sinh(5.5);
#   (3) the HF shower library: photoelectrons per GeV of the tracks entering the HF;
#   (4) the tracker crossings per primary, by |eta|.
#   python3 g4Extras_09_G4.py <signal SIM file> <pileup SIM file> [pileup event, default 1:1:72] > out/extras_09_G4.txt
import sys, math, collections
from DataFormats.FWLite import Events, Handle

signal, pileup = sys.argv[1], sys.argv[2]
puevent = tuple(int(x) for x in (sys.argv[3] if len(sys.argv) > 3 else '1:1:72').split(':'))
R_CUT, ETA_CUT = 2.9, 5.5                     # Generator: RDecLenCut [cm], MaxEtaCut
Z_LMAX = R_CUT * math.sinh(ETA_CUT)           # 354.8 cm
Z_HECTOR = R_CUT * math.sinh(8.2)             # 5279 cm: EtaCutForHector
R_TRK, Z_TRK, Z_NOSE, Z_HF = 123.3, 293.2, 355.15, 1114.9
TRK = ['TrackerHitsPixelBarrelLowTof', 'TrackerHitsPixelEndcapLowTof', 'TrackerHitsTIBLowTof',
       'TrackerHitsTIDLowTof', 'TrackerHitsTOBLowTof', 'TrackerHitsTECLowTof']

hTrk, hVtx = Handle('std::vector<SimTrack>'), Handle('std::vector<SimVertex>')
hCalo, hHit = Handle('std::vector<PCaloHit>'), Handle('std::vector<PSimHit>')
hGen, hHep = Handle('std::vector<reco::GenParticle>'), Handle('edm::HepMCProduct')

def get_event(fname, want):
    for ev in Events(fname):
        a = ev.eventAuxiliary()
        if (a.run(), a.luminosityBlock(), a.event()) == want:
            return ev
    raise SystemExit('event %s not found in %s' % (want, fname))

def surface(t):
    p = t.trackerSurfacePosition()
    r, z = math.hypot(p.x(), p.y()), abs(p.z())
    if abs(r - R_TRK) < 0.5: return 'barrel cylinder r = 123.3 cm', r, p.z()
    if abs(z - Z_TRK) < 0.5: return 'endcap disk |z| = 293.2 cm', r, p.z()
    if abs(z - Z_NOSE) < 0.2: return 'nose ring |z| = 355.1 cm', r, p.z()
    if abs(z - Z_HF) < 1.0: return 'HF front face |z| = 1114.9 cm', r, p.z()
    return 'inner wall along the beam pipe', r, p.z()

def boundary(trks, title):
    prim = [t for t in trks if t.genpartIndex() > 0]
    c, ranges = collections.Counter(), collections.defaultdict(list)
    for t in prim:
        if not t.crossedBoundary():
            c['not reaching'] += 1
            continue
        k, r, z = surface(t)
        c[k] += 1
        ranges[k].append((abs(t.momentum().eta()), r, abs(z)))
    print('%s: %d primaries, %d reach the boundary' % (title, len(prim), len(prim) - c['not reaching']))
    for k, v in sorted(ranges.items(), key=lambda kv: min(x[0] for x in kv[1])):
        print('  %-32s %4d  |eta| %.2f-%.2f  r %.1f-%.1f cm  |z| %.1f-%.1f cm' % (
            k, len(v), min(x[0] for x in v), max(x[0] for x in v), min(x[1] for x in v), max(x[1] for x in v),
            min(x[2] for x in v), max(x[2] for x in v)))
    print('  %-32s %4d' % ('not reaching', c['not reaching']))

print('== g4Extras chapter 09_G4: %s, %s' % (signal.split('/')[-1], pileup.split('/')[-1]))
print('Generator cuts: Z_lmax = 2.9 cm * sinh(5.5) = %.1f cm; Z_hector = 2.9 cm * sinh(8.2) = %.1f cm = %.1f m'
      % (Z_LMAX, Z_HECTOR, Z_HECTOR / 100.))
ev = get_event(signal, (1, 1, 1))
ev.getByLabel('g4SimHits', hTrk); ev.getByLabel('g4SimHits', hVtx)
trks = list(hTrk.product()); tid = {t.trackId(): t for t in trks}
vtx0 = hVtx.product()[0].position()
print('signal primary vertex (%.3f, %.3f, %.2f) cm' % (vtx0.x(), vtx0.y(), vtx0.z()))
eta_corner = math.asinh((Z_TRK + vtx0.z()) / R_TRK)
print('corner of the tracker volume (r, z) = (123.3, -293.2) cm seen from the vertex: eta = -%.3f' % eta_corner)

print('\n-- (1) the tracker/calorimeter boundary')
boundary(trks, 'signal 1:1:1')
# the b and bbar systems: the stable descendants of the B- (pdg -521) and the B*+ (523) of chapter 06
ev.getByLabel('genParticles', hGen)
gps = hGen.product()
def stable_desc(i, out):
    g = gps[i]
    for k in range(g.numberOfDaughters()):
        d = g.daughterRef(k).key()
        if gps[d].status() == 1: out.add(d)
        else: stable_desc(d, out)
by_barcode = {t.genpartIndex(): t for t in trks if t.genpartIndex() > 0}
for i, g in enumerate(gps):
    if g.pdgId() in (-521, 523) and g.pt() > 90.:
        s = set(); stable_desc(i, s)
        name = 'b system (B-)' if g.pdgId() == -521 else 'bbar system (B*+)'
        c = collections.Counter(); rs, zs = [], []
        for d in sorted(s):
            t = by_barcode.get(d + 1)            # genParticles index + 1 = HepMC barcode
            if t is None:
                c['no SimTrack (pdg %d)' % gps[d].pdgId()] += 1
                continue
            k, r, z = surface(t)
            c[k] += 1; rs.append(r); zs.append(z)
        print('  %s: %d stable; %s; r %.1f-%.1f cm, z %.1f to %.1f cm' % (
            name, len(s), ', '.join('%s %d' % kv for kv in sorted(c.items())), min(rs), max(rs), min(zs), max(zs)))

print('\n-- (2) the eta cut as a line: stable particles of the signal with 5.3 < |eta| < 5.7')
ev.getByLabel('generatorSmeared', hHep)
hep = hHep.product().GetEvent()
it = hep.particles_begin()
rows = []
while it != hep.particles_end():
    p = it.__deref__(); it.__preinc__()
    if p.status() != 1: continue
    m = p.momentum(); pt = math.hypot(m.px(), m.py())
    if pt == 0: continue
    eta = math.asinh(m.pz() / pt)
    if not 5.3 < abs(eta) < 5.7: continue
    v = p.production_vertex().position()      # mm
    x1, y1, z1 = v.x() / 10., v.y() / 10., v.z() / 10.
    zi = Z_LMAX if m.pz() > 0 else -Z_LMAX
    d = (zi - z1) / m.pz()
    r_vtx = math.hypot(x1 + d * m.px(), y1 + d * m.py())
    r_nom = Z_LMAX / math.sinh(abs(eta))
    rows.append((eta, p.pdg_id(), m.e(), pt, z1, r_nom, r_vtx, p.barcode() in by_barcode))
print('  %7s %6s %7s %6s %7s %12s %12s %s' % ('eta', 'pdg', 'E[GeV]', 'pT', 'vtx z', 'r(nominal)', 'r(vertex)', 'primary'))
for row in sorted(rows):
    print('  %+7.4f %6d %7.1f %6.3f %7.2f %12.3f %12.3f %s' % row)

print('\n-- (3) the HF shower library (all HcalHits of the transport-only run are HF library hits)')
ev.getByLabel(('g4SimHits', 'HcalHits'), hCalo)
hits = list(hCalo.product())
pe = collections.defaultdict(float); cells = set()
for h in hits:
    pe[h.geantTrackId()] += h.energy(); cells.add(h.id())
e_in = sum(tid[k].momentum().E() for k in pe)
print('  %d hits in %d cells, %.0f photoelectrons, from %d tracks with E = %.1f GeV: %.3f photoelectrons per GeV'
      % (len(hits), len(cells), sum(pe.values()), len(pe), e_in, sum(pe.values()) / e_in))
for k, v in sorted(pe.items(), key=lambda kv: -kv[1])[:6]:
    t = tid[k]
    print('  track %4d pdg %5d E %6.1f GeV eta %+.2f: %4.0f photoelectrons, %.2f per GeV'
          % (k, t.type(), t.momentum().E(), t.momentum().eta(), v, v / t.momentum().E()))

print('\n-- (4) tracker crossings (PSimHits, zero energy loss) per primary')
nh = collections.Counter(); tof = []
for c in TRK:
    ev.getByLabel(('g4SimHits', c), hHit)
    for h in hHit.product():
        nh[h.trackId()] += 1; tof.append(h.timeOfFlight())
print('  %d hits from %d tracks; time of flight %.2f-%.2f ns' % (sum(nh.values()), len(nh), min(tof), max(tof)))
for lo, hi in ((0., 1.), (1., 2.), (2., 2.5)):
    sel = [nh.get(t.trackId(), 0) for t in trks if t.charge() != 0 and lo <= abs(t.momentum().eta()) < hi]
    print('  charged primaries %.1f < |eta| < %.1f: %3d, hits per track %.1f (min %d, max %d)'
          % (lo, hi, len(sel), sum(sel) / len(sel), min(sel), max(sel)))
sel = [nh.get(t.trackId(), 0) for t in trks if t.type() == 22 and abs(t.momentum().eta()) < 1.]
print('  photons |eta| < 1: %d, hits per photon %.1f' % (len(sel), sum(sel) / len(sel)))
k, n = max(nh.items(), key=lambda kv: kv[1])
print('  most hits: %d, track %d pdg %d pT %.3f GeV eta %+.2f' % (n, k, tid[k].type(), tid[k].momentum().pt(),
                                                                tid[k].momentum().eta()))

print('\n-- (1b) one pileup collision')
ev = get_event(pileup, puevent)
ev.getByLabel('g4SimHits', hTrk)
boundary(list(hTrk.product()), 'pileup %d:%d:%d' % puevent)
ev.getByLabel(('g4SimHits', 'HcalHits'), hCalo)
print('  HF: %d hits in %d cells, %.0f photoelectrons' % (hCalo.product().size(), len({h.id() for h in hCalo.product()}),
                                                         sum(h.energy() for h in hCalo.product())))
