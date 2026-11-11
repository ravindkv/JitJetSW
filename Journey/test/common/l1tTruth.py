#!/usr/bin/env python3
# l1tTruth.py: who made the Level-1 muons of chapter 12. For every muon the global muon trigger sends to the global
# trigger, the simulated particles (TrackingParticles of mix:MergedTrackTruth, signal and pileup of all bunch
# crossings) that point to it at the vertex: the muons within dR < 0.5 of the L1 muon extrapolated to the vertex, and
# the charged particles above 1.5 GeV within dR < 0.3 (a hadron that decays in flight or punches through). A muon
# 'produced' far from the beam line is the daughter of a decay in flight.
#   python3 l1tTruth.py <test/12_L1T/out> 12_L1T
import math, os, sys
import ROOT
ROOT.gROOT.SetBatch(True)
ROOT.gErrorIgnoreLevel = ROOT.kError
from DataFormats.FWLite import Events, Handle

out, chapter = sys.argv[1], sys.argv[2]
print('== l1tTruth %s' % chapter)
path = os.path.join(out, 'step4_L1_%s.root' % chapter)
for e in Events(path):
    hm = Handle('BXVector<l1t::Muon>')
    e.getByLabel(('simGmtStage2Digis', '', 'L1'), hm)
    ht = Handle('std::vector<TrackingParticle>')
    e.getByLabel(('mix', 'MergedTrackTruth'), ht)
    mus = hm.product()
    n = sum(mus.size(bx) for bx in range(mus.getFirstBX(), mus.getLastBX() + 1))
    print('%d L1 muon(s); %d TrackingParticles' % (n, ht.product().size() if ht.isValid() else -1))
    if not n or not ht.isValid():
        break
    tps = ht.product()
    for bx in range(mus.getFirstBX(), mus.getLastBX() + 1):
        for i in range(mus.size(bx)):
            m = mus.at(bx, i)
            print('  L1 muon bx %d: pt %.1f eta %.3f phi %.3f (at the vertex: eta %.3f phi %.3f) charge %d quality %d' %
                  (bx, m.pt(), m.eta(), m.phi(), m.etaAtVtx(), m.phiAtVtx(), m.charge(), m.hwQual()))
            rows = []
            for tp in tps:
                if tp.charge() == 0 or tp.pt() < 0.5:
                    continue
                dphi = abs(tp.phi() - m.phiAtVtx())
                if dphi > math.pi:
                    dphi = 2 * math.pi - dphi
                dr = math.hypot(tp.eta() - m.etaAtVtx(), dphi)
                ismu = abs(tp.pdgId()) == 13
                if (ismu and dr < 0.5) or (tp.pt() > 1.5 and dr < 0.3):
                    v = tp.vertex()
                    rows.append((dr, '    %s pdgId %6d pt %6.2f eta %6.3f phi %6.3f charge %+d | bx %d, collision %d (0 = the signal) | produced at r %.2f cm, z %.1f cm | hits %d (tracker %d) | dR %.2f' % (
                        'MUON   ' if ismu else 'charged', tp.pdgId(), tp.pt(), tp.eta(), tp.phi(), tp.charge(), tp.eventId().bunchCrossing(), tp.eventId().event(),
                        math.hypot(v.x(), v.y()), v.z(), tp.numberOfHits(), tp.numberOfTrackerHits(), dr)))
            for _, r in sorted(rows)[:12]:
                print(r)
            if not rows:
                print('    no simulated muon within dR 0.5 and no charged particle above 1.5 GeV within dR 0.3')
    break
