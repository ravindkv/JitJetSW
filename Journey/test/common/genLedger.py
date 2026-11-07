#!/usr/bin/env python3
# genLedger.py: the energy ledger of one generated event (FWLite, no cmsRun), for the generator chapters 01-08 of the
# JitJet book: what the generator record of a GEN file holds, how the 13.6 TeV of the two protons are distributed at
# the stage the file was generated with, and how much of it a detector could see.
#
#   python3 genLedger.py <GEN file> [--event run:lumi:event] [--collection genParticles] [--jets ak4GenJets,ak8GenJets] [--pu]
#
# Prints, for the event (default: the first one):
#   the record by Pythia8 status group (hard process 21-29, MPI 31-39, ISR 41-49, FSR 51-59, beam remnants 61-69,
#   strings 71-79, decayed hadrons 2, final state 1), the incoming partons of the hard process (x1, x2, sqrt(s_hat)),
#   the outgoing partons, and the final state (status 1): number, energy, scalar pT by kind (partons / diquarks, charged
#   hadrons, neutral hadrons, photons, electrons, muons, neutrinos) and by region (|eta| < 2.5 tracker, 2.5-3.0, 3.0-5.2
#   forward calorimeter, 5.2-5.5, > 5.5 beyond what Geant4 is given), with the invisible (neutrino) energy;
#   with --jets the jets of each collection: number, scalar sum of pT, the two leading jets, the clustered final state.
# With --pu the collection carries a collisionId (genParticlesPU of chapter 08): every final-state line is given for
# the signal (collisionId 0), for the pileup collisions (collisionId > 0) and for the sum.
import sys, math, collections
from DataFormats.FWLite import Events, Handle

args = sys.argv[1:]
def opt(name, default=None):
    if name in args:
        i = args.index(name)
        v = args[i + 1]
        del args[i:i + 2]
        return v
    return default
pu = '--pu' in args
if pu:
    args.remove('--pu')
event = opt('--event', '')
collection = opt('--collection', 'genParticlesPU' if pu else 'genParticles')
jets = [j for j in opt('--jets', '').split(',') if j]
fname = args[0]
E_BEAM = 6800.

charged_leptons = {11, 13, 15}
neutrinos = {12, 14, 16}
def kind(p):
    a = abs(p.pdgId())
    if a == 22:
        return 'photon'
    if a == 11:
        return 'electron'
    if a == 13:
        return 'muon'
    if a in neutrinos:
        return 'neutrino'
    if a <= 21 or (1000 < a < 6000 and a % 100 < 10 and (a // 10) % 10 == 0):   # quarks, gluons, diquarks (qq_1, qq_3)
        return 'parton'
    if a > 100:
        return 'charged hadron' if p.charge() != 0 else 'neutral hadron'
    return 'other'
kinds = ['parton', 'charged hadron', 'neutral hadron', 'photon', 'electron', 'muon', 'neutrino', 'other']
regions = [('|eta|<2.5', 0., 2.5), ('2.5-3.0', 2.5, 3.0), ('3.0-5.2', 3.0, 5.2), ('5.2-5.5', 5.2, 5.5), ('>5.5', 5.5, 1e9)]
groups = [('hard process (21-29)', 21, 29), ('MPI (31-39)', 31, 39), ('ISR (41-49)', 41, 49), ('FSR (51-59)', 51, 59),
          ('beam remnants (61-69)', 61, 69), ('strings (71-79)', 71, 79), ('hadronisation (81-89)', 81, 89),
          ('decayed (status 2)', 2, 2), ('final state (status 1)', 1, 1)]

class Sum:
    def __init__(self):
        self.n = 0; self.e = 0.; self.pt = 0.
    def add(self, p):
        self.n += 1; self.e += p.energy(); self.pt += p.pt()
    def __str__(self):
        return '%6d %9.1f %8.1f' % (self.n, self.e, self.pt)

hGen = Handle('std::vector<reco::GenParticle>')
hJet = Handle('std::vector<reco::GenJet>')
for ev in Events(fname):
    aux = ev.eventAuxiliary()
    tag = '%d:%d:%d' % (aux.run(), aux.luminosityBlock(), aux.event())
    if event and tag != event:
        continue
    ev.getByLabel(collection, hGen)
    parts = hGen.product()
    print('== genLedger %s event %s: %d particles in %s' % (fname.split('/')[-1], tag, parts.size(), collection))
    samples = ['signal', 'pileup', 'all'] if pu else ['all']
    def sample_of(p):
        return 'signal' if (not pu or p.collisionId() == 0) else 'pileup'
    # the record by status group
    print('record by status group' + (' (signal / pileup / all)' if pu else '') + ':')
    for name, lo, hi in groups:
        c = collections.Counter()
        for p in parts:
            if lo <= p.status() <= hi:
                c[sample_of(p)] += 1; c['all'] += 1
        if c['all']:
            print('  %-26s %s' % (name, ' / '.join('%6d' % c[s] for s in samples)))
    # the hard process of the signal
    inc = [p for p in parts if 21 <= p.status() <= 29 and sample_of(p) == 'signal' and p.numberOfMothers() and p.mother(0).status() in (4, 0)]
    inc = [p for p in parts if p.status() == 21 and sample_of(p) == 'signal']
    out = [p for p in parts if p.status() == 23 and sample_of(p) == 'signal']
    if inc:
        xs = [p.energy() / E_BEAM for p in inc]
        shat = sum(p.energy() for p in inc) ** 2 - sum(p.pz() for p in inc) ** 2
        print('hard process: incoming ' + ' + '.join('pdg %d (E=%.1f GeV, x=%.4f)' % (p.pdgId(), p.energy(), x) for p, x in zip(inc, xs))
              + ' -> sqrt(s_hat) = %.1f GeV' % math.sqrt(max(shat, 0.)))
    for p in out:
        print('  outgoing pdg %d: pT=%.1f eta=%.2f phi=%.2f E=%.1f GeV' % (p.pdgId(), p.pt(), p.eta(), p.phi(), p.energy()))
    # the final state
    for s in samples:
        final = [p for p in parts if p.status() == 1 and (s == 'all' or sample_of(p) == s)]
        tot = Sum()
        for p in final:
            tot.add(p)
        print('final state (status 1)%s: n=%d sumE=%.1f GeV sumPt=%.1f GeV' % ('' if not pu else ' of the ' + s, tot.n, tot.e, tot.pt))
        print('  %-18s %6s %9s %8s' % ('by kind', 'n', 'E[GeV]', 'pT[GeV]'))
        byk = collections.defaultdict(Sum)
        for p in final:
            byk[kind(p)].add(p)
        for k in kinds:
            if byk[k].n:
                print('  %-18s %s' % (k, byk[k]))
        print('  %-18s %6s %9s %8s %8s' % ('by region', 'n', 'E[GeV]', 'pT[GeV]', 'E_nu'))
        for name, lo, hi in regions:
            r = Sum(); nu = 0.
            for p in final:
                if lo <= abs(p.eta()) < hi:
                    r.add(p)
                    if abs(p.pdgId()) in neutrinos:
                        nu += p.energy()
            print('  %-18s %s %8.1f' % (name, r, nu))
        vis = Sum(); inv = Sum()
        for p in final:
            if abs(p.pdgId()) in neutrinos:
                inv.add(p)
            elif abs(p.eta()) < 5.2:
                vis.add(p)
        print('  visible (|eta|<5.2, no neutrinos): n=%d E=%.1f GeV pT=%.1f GeV; neutrinos: n=%d E=%.1f GeV pT=%.1f GeV'
              % (vis.n, vis.e, vis.pt, inv.n, inv.e, inv.pt))
    # the jets
    for j in jets:
        ev.getByLabel(j, hJet)
        if not hJet.isValid():
            print('jets %s: not in the file' % j)
            continue
        js = sorted(hJet.product(), key=lambda x: -x.pt())
        nconst = sum(x.numberOfDaughters() for x in js)
        print('jets %s: n=%d sumPt=%.1f GeV sumE=%.1f GeV constituents=%d' % (j, len(js), sum(x.pt() for x in js), sum(x.energy() for x in js), nconst))
        for i, x in enumerate(js[:4]):
            print('  [%d] pT=%.1f eta=%.2f phi=%.2f mass=%.1f nConst=%d' % (i, x.pt(), x.eta(), x.phi(), x.mass(), x.numberOfDaughters()))
    break
else:
    print('event %s not found in %s' % (event, fname)); sys.exit(1)
