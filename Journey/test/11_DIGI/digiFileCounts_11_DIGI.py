#!/usr/bin/env python3
# digiFileCounts_11_DIGI.py: the counts of the three DIGI files of chapter 11 that the analyzers do not print (companion,
# section 4.5; on test/11_DIGI/out2024/ the 2024 numbers of section 4.9): the PileupSummaryInfo per bunch crossing, the
# ECAL selective-readout flags by value, the HCAL frames before and after the zero suppression, the ECAL frames written,
# the TrackingParticles of mix:MergedTrackTruth split by origin (signal, in-time pileup, early, late).
#   python3 11_DIGI/digiFileCounts_11_DIGI.py <out dir> [pileup event run:lumi:event, default 1:1:747 = SKIP + 1]
import sys, os, ROOT
from collections import Counter
from DataFormats.FWLite import Events, Handle
ROOT.gROOT.SetBatch(True)
out = sys.argv[1]
pu_event = tuple(int(x) for x in (sys.argv[2] if len(sys.argv) > 2 else '1:1:747').split(':'))
cases = [(c, ev) for c, ev in (('signal', (1, 1, 1)), ('pileup', pu_event), ('mixed', (1, 1, 1)))
         if os.path.exists(os.path.join(out, 'step3_DIGI_%s_11_DIGI.root' % c))]
H = lambda t: Handle(t)
hpu = H('std::vector<PileupSummaryInfo>')
heb, hee = H('edm::SortedCollection<EBSrFlag,edm::StrictWeakOrdering<EBSrFlag> >'), H('edm::SortedCollection<EESrFlag,edm::StrictWeakOrdering<EESrFlag> >')
hq11, hq10, hho = H('HcalDataFrameContainer<QIE11DataFrame>'), H('HcalDataFrameContainer<QIE10DataFrame>'), H('edm::SortedCollection<HODataFrame,edm::StrictWeakOrdering<HODataFrame> >')
hebd, heed = H('EBDigiCollection'), H('EEDigiCollection')
hes = H('edm::SortedCollection<ESDataFrame,edm::StrictWeakOrdering<ESDataFrame> >')
htp = H('std::vector<TrackingParticle>')
for case, evid in cases:
    for e in Events(os.path.join(out, 'step3_DIGI_%s_11_DIGI.root' % case)):
        a = e.eventAuxiliary()
        if (a.run(), a.luminosityBlock(), a.event()) != evid:
            continue
        print('== %s run %d event %d' % (case, a.run(), a.event()))
        e.getByLabel('addPileupInfo', hpu)
        pus = [(p.getBunchCrossing(), p.getPU_NumInteractions(), p.getTrueNumInteractions()) for p in hpu.product()]
        print('  pileup per bx (bx:n): %s; sum %d' % (' '.join('%d:%d' % (b, n) for b, n, t in pus), sum(n for b, n, t in pus)))
        e.getByLabel(('simEcalDigis', 'ebSrFlags'), heb); e.getByLabel(('simEcalDigis', 'eeSrFlags'), hee)
        print('  EB SR flags (value: n of the 2448 trigger towers): %s; EE SR flags (n of the 624 supercrystals): %s' % (
            sorted(Counter(f.value() for f in heb.product()).items()), sorted(Counter(f.value() for f in hee.product()).items())))
        n = {}
        for lab in ('simHcalUnsuppressedDigis', 'simHcalDigis'):
            e.getByLabel((lab, 'HBHEQIE11DigiCollection'), hq11); e.getByLabel((lab, 'HFQIE10DigiCollection'), hq10); e.getByLabel(lab, hho)
            n[lab] = (hq11.product().size(), hq10.product().size(), hho.product().size() if hho.isValid() else -1)
        u, z = n['simHcalUnsuppressedDigis'], n['simHcalDigis']
        print('  HCAL unsuppressed -> zero suppressed: HBHE(QIE11) %d -> %d, HF(QIE10) %d -> %d, HO %d -> %d' % (u[0], z[0], u[1], z[1], u[2], z[2]))
        e.getByLabel(('simEcalDigis', 'ebDigis'), hebd); e.getByLabel(('simEcalDigis', 'eeDigis'), heed); e.getByLabel(('simEcalPreshowerDigis', ''), hes)
        print('  ECAL frames written: EB %d, EE %d, ES %d' % (hebd.product().size(), heed.product().size(), hes.product().size() if hes.isValid() else -1))
        e.getByLabel(('mix', 'MergedTrackTruth'), htp)
        c = Counter()
        for tp in htp.product():
            ei = tp.eventId()
            c['signal' if (ei.bunchCrossing() == 0 and ei.event() == 0) else ('in-time pileup' if ei.bunchCrossing() == 0 else ('early' if ei.bunchCrossing() < 0 else 'late'))] += 1
        print('  TrackingParticles (mix:MergedTrackTruth): %d (signal %d, in-time pileup %d, out-of-time %d: early %d, late %d)' % (
            htp.product().size(), c['signal'], c['in-time pileup'], c['early'] + c['late'], c['early'], c['late']))
