#!/usr/bin/env python3
# minBiasGenStats.py: the generator-level catalogue of a minimum-bias GEN file (FWLite, no cmsRun).
#   python3 common/minBiasGenStats.py 08_PU/out/step1_GEN_MinBias_08_PU.root [maxEvents] [--event N ...]   (from test/)
# One line per event: process (Pythia8 signal_process_id: 101 ND, 103/104 SD, 105 DD, 106 CD), number of parton-parton
# interactions (1 hard + MPI, counted from the status-31 partons), stable particles (HepMC status 1), charged and
# central (|eta| < 2.5, pT > 0.5 GeV) ones, particles beyond |eta| = 5.5, pT of the hardest stable particle, scalar sum of
# pT, ak4 gen jets above 10 GeV, number of protons LHCTransport carried to the PPS, smeared vertex z and t;
# then the summary of the sample: process shares, species of the stable particles, |eta| and pT distributions, the
# multiplicity against the number of interactions, the diffractive masses. With --event N the stable particles of
# event N are listed. The companion doc/chapter/chapter_08_PU.md (sections 3 and 4.3) explains every column.
import sys, math, collections, statistics
from DataFormats.FWLite import Events, Handle

args = [a for a in sys.argv[1:] if not a.startswith('--')]
listed = set()
if '--event' in sys.argv:
    listed = {int(a) for a in sys.argv[sys.argv.index('--event') + 1:] if a.isdigit()}
    args = [a for a in args if a not in {str(n) for n in listed}]
fname = args[0]
maxev = int(args[1]) if len(args) > 1 else -1

hHep = Handle('edm::HepMCProduct'); hLHC = Handle('edm::HepMCProduct'); hJ = Handle('std::vector<reco::GenJet>')
procs = {101: 'ND', 103: 'SD', 104: 'SD', 105: 'DD', 106: 'CD'}
names = {22: 'gamma', 211: 'pi+-', 321: 'K+-', 2212: 'p', 2112: 'n', 130: 'K0L', 310: 'K0S', 3122: 'Lambda', 11: 'e',
         3112: 'Sigma-', 3222: 'Sigma+', 3312: 'Xi-', 3322: 'Xi0', 13: 'mu', 12: 'nu_e', 14: 'nu_mu', 16: 'nu_tau', 3334: 'Omega'}
charged = {211, 321, 2212, 11, 13, 3112, 3222, 3312, 3334}
E_BEAM = None

rows = []; species = collections.Counter(); etabins = collections.Counter(); ptbins = collections.Counter()
by_nint = collections.defaultdict(list); sdmass = []
print('%5s %4s %4s %6s %6s %6s %6s %7s %7s %5s %4s %7s %7s' %
      ('event', 'proc', 'nInt', 'stable', 'charg', 'centr', '|eta|>5.5', 'leadpT', 'sumpT', 'jet10', 'PPS', 'z[cm]', 't[ns]'))
for i, e in enumerate(Events(fname)):
    if 0 <= maxev <= i:
        break
    e.getByLabel('generatorSmeared', hHep); ev = hHep.product().GetEvent()
    e.getByLabel('LHCTransport', hLHC); npps = hLHC.product().GetEvent().particles_size()
    e.getByLabel('ak4GenJets', hJ); nj = sum(1 for j in hJ.product() if j.pt() > 10)
    k = e.eventAuxiliary().event(); proc = procs.get(ev.signal_process_id(), str(ev.signal_process_id()))
    if E_BEAM is None:
        E_BEAM = ev.beam_particles()[0].momentum().e()
    n31 = 0; stable = []
    it = ev.particles_begin()
    while it != ev.particles_end():
        p = it.__deref__(); st = p.status(); m = p.momentum()
        if st == 31:
            n31 += 1
        elif st == 1:
            pt = m.perp(); eta = m.eta() if pt > 1e-6 else math.copysign(99., m.pz())
            stable.append((p.pdg_id(), m.px(), m.py(), m.pz(), m.e(), pt, eta))
        it.__preinc__()
    nint = 1 + n31 // 2                                   # the hard interaction (status 21/23) + one MPI per pair of status-31 partons
    nst = len(stable); nch = sum(1 for s in stable if abs(s[0]) in charged)
    ncen = sum(1 for s in stable if abs(s[6]) < 2.5 and s[5] > 0.5); nfwd = sum(1 for s in stable if abs(s[6]) > 5.5)
    lead = max((s[5] for s in stable), default=0.); sumpt = sum(s[5] for s in stable)
    v = ev.vertices_begin().__deref__().position()        # HepMC: mm and mm/c
    rows.append((k, proc, nint, nst, nch, ncen, nfwd, lead, sumpt, nj, npps, v.z() / 10., v.t() / 299.792458))
    print('%5d %4s %4d %6d %6d %6d %6d %7.2f %7.1f %5d %4d %7.2f %7.3f' % rows[-1])
    for s in stable:
        species[abs(s[0])] += 1; etabins[min(int(abs(s[6])), 10)] += 1
        ptbins[0 if s[5] < 0.2 else 1 if s[5] < 0.5 else 2 if s[5] < 1 else 3 if s[5] < 2 else 4 if s[5] < 5 else 5] += 1
    if proc == 'ND':
        by_nint[nint].append(nst)
    if proc == 'SD':                                      # diffractive system X = everything but the most energetic proton
        protons = [s for s in stable if s[0] == 2212]
        if protons:
            lp = max(protons, key=lambda s: s[4]); X = [s for s in stable if s is not lp]
            E, px, py, pz = (sum(s[j] for s in X) for j in (4, 1, 2, 3))
            sdmass.append((k, math.sqrt(max(E * E - px * px - py * py - pz * pz, 0.)), 1. - lp[4] / E_BEAM))
    if k in listed:
        print('  event %d (%s, %d interactions, %d particles and %d vertices in the record): stable particles by pT' %
              (k, proc, nint, ev.particles_size(), ev.vertices_size()))
        for s in sorted(stable, key=lambda s: -s[5]):
            print('    %-7s pT %7.3f  eta %7.2f  E %9.1f GeV' % (names.get(abs(s[0]), s[0]), s[5], s[6], s[4]))

n = len(rows)
if not n:
    sys.exit(0)
print('\n== %d events, beam energy %.0f GeV' % (n, E_BEAM))
cnt = collections.Counter(r[1] for r in rows)
print('process shares:', ', '.join('%s %d (%.1f%%)' % (p, c, 100. * c / n) for p, c in cnt.most_common()))
for name, col in (('interactions', 2), ('stable', 3), ('charged', 4), ('central', 5), ('|eta|>5.5', 6), ('lead pT', 7), ('sum pT', 8), ('jets>10', 9), ('PPS protons', 10)):
    l = [r[col] for r in rows]
    print('%-12s mean %7.2f  median %7.2f  min %7.2f  max %7.2f' % (name, statistics.mean(l), statistics.median(l), min(l), max(l)))
print('per process (events, mean interactions, stable, charged, central, lead pT, sum pT):')
for p in ('ND', 'SD', 'DD', 'CD'):
    l = [r for r in rows if r[1] == p]
    if l:
        print('  %s %4d  %5.1f %6.1f %6.1f %5.1f %5.2f %6.1f' % ((p, len(l)) + tuple(statistics.mean(r[c] for r in l) for c in (2, 3, 4, 5, 7, 8))))
tot = sum(species.values())
print('stable particles by species: ' + ', '.join('%s %.1f%%' % (names.get(pdg, pdg), 100. * c / tot) for pdg, c in species.most_common(12)))
print('|eta| of the stable particles, per unit (last bin >= 10): ' + ' '.join('%d' % etabins[b] for b in range(11)))
print('pT of the stable particles (<0.2, 0.2-0.5, 0.5-1, 1-2, 2-5, >5 GeV): ' + ' '.join('%d' % ptbins[b] for b in range(6)))
if by_nint:
    print('ND: mean stable particles against the number of interactions: ' +
          ', '.join('%d: %.0f (%d ev)' % (k, statistics.mean(v), len(v)) for k, v in sorted(by_nint.items())))
if sdmass:
    ms = sorted(s[1] for s in sdmass); xs = sorted(s[2] for s in sdmass)
    print('SD: diffractive mass M_X median %.1f GeV (min %.1f, max %.0f), xi = 1 - E_p/E_beam median %.1e (min %.1e, max %.2f)' %
          (statistics.median(ms), ms[0], ms[-1], statistics.median(xs), xs[0], xs[-1]))
