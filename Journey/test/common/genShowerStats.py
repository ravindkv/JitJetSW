#!/usr/bin/env python3
# genShowerStats.py: how typical is the shower of the event of the book? The statistics of the parton shower over the
# many events of the cross-section job of a generator chapter (gen_xsec of jitjet_common.sh with XSEC_FILE=1: the same
# fragment, settings and seeds as the chapter, so its first event is the event of the book). FWLite, no cmsRun.
#
#   python3 genShowerStats.py <GEN file with many events> [--event run:lumi:event]
#
# Counted per event from the Pythia8 status codes of genParticles (hadronisation off or on: the codes are kept):
#   ISR branchings   the entries with status 43 (the outgoing parton of an initial-state branching; one per branching,
#                    as many as the status-41 new incoming partons)
#   FSR branchings   half the entries with status 51 (each final-state branching writes its two daughters with 51);
#                    this includes the FSR of the partons emitted by the ISR
#   b / bbar emissions  the branchings q -> q g in which the b (bbar) of the hard process, or one of its copies, is the
#                    radiator (two status-51 daughters, one of them of the same flavour)
# Prints the mean, the distribution, the number of events with none, the values of the event of the book, and the
# fraction of events without ISR in bins of the larger and of the smaller momentum fraction x of the incoming partons.
import sys, collections
from DataFormats.FWLite import Events, Handle

args = sys.argv[1:]
book = '1:1:1'
if '--event' in args:
    i = args.index('--event'); book = args[i + 1]; del args[i:i + 2]
fname = args[0]
E_BEAM = 6800.

def emissions(q):
    n, todo, seen = 0, [q], set()
    while todo:
        p = todo.pop()
        key = (p.pdgId(), p.status(), round(p.energy(), 6), round(p.pz(), 6))
        if key in seen:
            continue
        seen.add(key)
        ds = [p.daughter(j) for j in range(p.numberOfDaughters())]
        if len(ds) == 2 and all(d.status() == 51 for d in ds) and any(d.pdgId() == p.pdgId() for d in ds):
            n += 1
        todo += [d for d in ds if d.pdgId() == p.pdgId()]
    return n

h = Handle('std::vector<reco::GenParticle>')
rows, tags = [], []
for ev in Events(fname):
    aux = ev.eventAuxiliary()
    ev.getByLabel('genParticles', h)
    ps = h.product()
    n_isr = sum(1 for p in ps if p.status() == 43)
    n_fsr = sum(1 for p in ps if p.status() == 51) // 2
    xs = sorted(p.energy() / E_BEAM for p in ps if p.status() == 21)
    out = {p.pdgId(): p for p in ps if p.status() == 23}
    rows.append(dict(isr=n_isr, fsr=n_fsr, xmin=xs[0], xmax=xs[-1],
                     b=emissions(out[5]) if 5 in out else -1, bbar=emissions(out[-5]) if -5 in out else -1))
    tags.append('%d:%d:%d' % (aux.run(), aux.luminosityBlock(), aux.event()))
N = len(rows)
print('== genShowerStats %s: %d events' % (fname.split('/')[-1], N))
def summary(key, what):
    v = [r[key] for r in rows]
    dist = sorted(collections.Counter(v).items())
    print('%-40s mean %.2f, none in %d events (%.1f%%)' % (what, sum(v) / N, v.count(0), 100. * v.count(0) / N))
    print('  distribution (value: events): ' + ', '.join('%d: %d' % kv for kv in dist))
summary('isr', 'ISR branchings per event (status 43)')
summary('fsr', 'FSR branchings per event (status 51 / 2)')
summary('b', 'b emissions (b -> b g) per event')
summary('bbar', 'bbar emissions per event')
both = sum(1 for r in rows if r['b'] == 0 and r['bbar'] == 0)
print('b and bbar both without emission: %d events (%.1f%%)' % (both, 100. * both / N))
zero = [t for t, r in zip(tags, rows) if r['isr'] == 0]
print('events without ISR: %d: %s' % (len(zero), ' '.join(zero)))
if book in tags:
    r = rows[tags.index(book)]
    print('the event of the book %s: ISR %d, FSR %d, b emissions %d, bbar emissions %d, x = %.4f and %.4f'
          % (book, r['isr'], r['fsr'], r['b'], r['bbar'], r['xmin'], r['xmax']))
for key, edges in (('xmax', (0., 0.1, 0.2, 0.3, 1.)), ('xmin', (0., 0.003, 0.01, 0.03, 1.))):
    print('events without ISR in bins of %s (the %s momentum fraction of the two incoming partons):'
          % (key, 'larger' if key == 'xmax' else 'smaller'))
    for lo, hi in zip(edges[:-1], edges[1:]):
        sel = [r for r in rows if lo <= r[key] < hi]
        if sel:
            nz = sum(1 for r in sel if r['isr'] == 0)
            print('  %s in [%.3f, %.3f): %4d events, %3d without ISR (%.1f%%), mean ISR %.2f'
                  % (key, lo, hi, len(sel), nz, 100. * nz / len(sel), sum(r['isr'] for r in sel) / len(sel)))
