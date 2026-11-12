#!/usr/bin/env python3
# hltMenu.py: the HLT menu that chapter 13 ran, read from the configuration itself so that the book quotes what
# CMSSW actually ran (like l1tSettings.py for chapter 12).
#   python3 hltMenu.py <step5_HLT_..._cfg.py> [path ...]
# Section 1: the menu: its name in the configuration database, the paths by kind, the datasets and streams, what
# python/hltL1Menu_cff.py changed to run it on the Level-1 menu of chapter 12. Section 2: the anatomy of the jet paths of the
# book (default HLT_PFJet110, HLT_PFJetFwd140, HLT_DiPFJetAve100_HFJEC, HLT_PFJet40): every module in the order it
# runs, with its C++ type. Section 3: the parameters of the modules that the chapter describes, step by step. What is
# not here is in the database: the jet energy corrections (payloads AK4CaloHLT, AK4PFHLT of JetCorrectionsRecord), the
# calibrations of the detectors; the analyzers print those (out/hlt_13_HLT.log).
import os, re, sys

def load(cfg):
    argv = sys.argv
    sys.argv = [cfg]
    ns = {}
    stdout = sys.stdout
    sys.stdout = open(os.devnull, 'w')     # the customise prints its report; it is in section 1
    try:
        exec(compile(open(cfg).read(), cfg, 'exec'), ns)
    finally:
        sys.stdout.close()
        sys.stdout = stdout
    sys.argv = argv
    return ns['process']

import FWCore.ParameterSet.Config as cms

def walk(ps, prefix='', depth=0):
    try:
        names = ps.parameterNames_()
    except Exception:
        return
    for n in sorted(names):
        try:
            v = getattr(ps, n)
        except Exception:
            continue
        if isinstance(v, cms.VPSet):
            for i, p in enumerate(v):
                if depth < 4 and i < 6:
                    yield from walk(p, '%s%s[%d].' % (prefix, n, i), depth + 1)
            if len(v) > 6:
                yield prefix + n, '(%d parameter sets, the first 6 listed)' % len(v)
        elif hasattr(v, 'parameterNames_'):
            if depth < 4:
                yield from walk(v, prefix + n + '.', depth + 1)
        else:
            try:
                val = v.value() if hasattr(v, 'value') else v
            except Exception:
                val = v
            s = str(val)
            yield prefix + n, s if len(s) < 160 else s[:157] + '...'

def dump(process, label):
    if not hasattr(process, label):
        print('\n-- %s: not in the menu' % label)
        return
    mod = getattr(process, label)
    print('\n-- %s (%s)' % (label, mod.type_() if hasattr(mod, 'type_') else 'PSet'))
    for name, val in walk(mod):
        print('   %-52s %s' % (name, val))

cfg = sys.argv[1]
paths = sys.argv[2:] or ['HLT_PFJet110', 'HLT_PFJetFwd140', 'HLT_DiPFJetAve100_HFJEC', 'HLT_PFJet40']
process = load(cfg)
print('== hltMenu: %s' % os.path.basename(cfg))

# ------------------------------------------------------------------------------------------------------------- 1
print('\n== 1. The menu')
print('process %s, table %s' % (process.name_(), process.HLTConfigVersion.tableName.value()))
allPaths = process.paths_()
kinds = {}
for p in allPaths:
    k = p.split('_')[0] if '_' in p else p
    kinds[k] = kinds.get(k, 0) + 1
print('paths: %d | %s' % (len(allPaths), ' '.join('%s=%d' % kv for kv in sorted(kinds.items()))))
print('endpaths: %s' % ' '.join(process.endpaths_()))
if hasattr(process, 'datasets'):
    print('datasets: %d' % len(process.datasets.parameterNames_()))
if hasattr(process, 'streams'):
    print('streams: %d' % len(process.streams.parameterNames_()))
types = {}
for label, m in list(process.producers_().items()) + list(process.filters_().items()) + list(process.analyzers_().items()):
    types[m.type_()] = types.get(m.type_(), 0) + 1
print('modules: %d producers, %d filters, %d analyzers; %d C++ types' % (len(process.producers_()), len(process.filters_()), len(process.analyzers_()), len(types)))
print('event-setup: %d producers, %d sources' % (len(process.es_producers_()), len(process.es_sources_())))
for name in ('HLTL1TSeed', 'HLTPrescaler', 'HLT1PFJet', 'HLT1CaloJet', 'TriggerResultsFilter'):
    print('   %-24s %d' % (name, types.get(name, 0)))
if hasattr(process, 'hltL1MenuReport'):
    lines = list(process.hltL1MenuReport.lines)
    print('adapted to the L1 menu of chapter 12 (python/hltL1Menu_cff.py): %d seeds changed, %d paths removed'
          % (len([l for l in lines if l.startswith('seed')]), len([l for l in lines if l.startswith('path')])))
    for l in lines:
        print('   ' + l)
print('prescales: %s' % ('PrescaleService present' if hasattr(process, 'PrescaleService') else
                         'no PrescaleService: every HLTPrescaler has prescale 1'))
# the datasets and streams of the jet paths of the book
for p in paths:
    ds = [d for d in process.datasets.parameterNames_() if any(re.match(p + r'_v\d+$', x) for x in getattr(process.datasets, d))] if hasattr(process, 'datasets') else []
    st = [s for s in process.streams.parameterNames_() if set(getattr(process.streams, s)) & set(ds)] if hasattr(process, 'streams') else []
    print('%s: datasets %s; streams %s' % (p, ' '.join(ds) or '-', ' '.join(st) or '-'))

# ------------------------------------------------------------------------------------------------------------- 2
print('\n== 2. The anatomy of the jet paths: the modules in the order they run')
for p in paths:
    names = [n for n in allPaths if re.match(p + r'_v\d+$', n)]
    if not names:
        print('\n-- %s: not in the menu' % p)
        continue
    path = getattr(process, names[0])
    labels = []
    class V:
        def enter(self, node):
            if hasattr(node, 'label_') and hasattr(node, 'type_'):
                try:
                    if node.label_() not in labels:  # a module met again in a later sequence runs only once
                        labels.append(node.label_())
                except Exception:
                    pass
        def leave(self, node):
            pass
    path.visit(V())
    print('\n-- %s: %d modules' % (names[0], len(labels)))
    for i, l in enumerate(labels):
        m = getattr(process, l)
        extra = ''
        if m.type_() == 'HLTL1TSeed':
            extra = 'seed "%s"' % m.L1SeedsLogicalExpression.value()
        elif m.type_() in ('HLT1CaloJet', 'HLT1PFJet'):
            extra = 'MinPt %g MinEta %g MaxEta %g input %s' % (m.MinPt.value(), m.MinEta.value(), m.MaxEta.value(), m.inputTag.value())
        elif m.type_() == 'HLTDiPFJetAveEtaFilter':
            extra = 'minPtAve %g tag |eta| < %g probe %g < |eta| < %g minDphi %g minPtJet %g' % (m.minPtAve.value(), m.maxTagEta.value(), m.minProbeEta.value(), m.maxProbeEta.value(), m.minDphi.value(), m.minPtJet.value())
        elif m.type_() == 'CandViewSelector':
            extra = 'cut "%s"' % m.cut.value()
        elif m.type_() == 'HLTPFJetsMatchedToFilteredCaloJetsProducer':
            extra = 'maxDeltaR %g' % m.maxDeltaR.value()
        print('   %3d %-58s %-44s %s' % (i, l, m.type_(), extra))

# ------------------------------------------------------------------------------------------------------------- 3
print('\n== 3. The parameters of the modules the chapter describes')
steps = [
    ('step 1: the Level-1 record, the seed, the prescale', ['hltGtStage2Digis', 'hltGtStage2ObjectMap', 'hltL1sSingleJet90', 'hltPrePFJet110']),
    ('step 2: calorimeter jets', ['hltHbhereco', 'hltHfreco', 'hltTowerMakerForAll', 'hltAK4CaloJets', 'hltAK4CaloJetsIDPassed', 'hltFixedGridRhoFastjetAllCalo',
                                  'hltAK4CaloFastJetCorrector', 'hltAK4CaloRelativeCorrector', 'hltAK4CaloAbsoluteCorrector', 'hltAK4CaloResidualCorrector',
                                  'hltAK4CaloCorrector', 'hltAK4CaloJetsCorrected', 'hltSingleAK4CaloJet80']),
    ('step 3: tracking and vertices', ['hltSiPixelClustersSoA', 'hltPixelTracksSoA', 'hltPixelTracks', 'hltPixelVerticesSoA', 'hltPixelVertices', 'hltTrimmedPixelVertices',
                                       'hltIter0PFLowPixelSeedsFromPixelTracks', 'hltIter0PFlowTrackCutClassifier', 'hltDoubletRecoveryPFlowPixelHitDoublets',
                                       'hltDoubletRecoveryPFlowTrackCutClassifier', 'hltMergedTracks', 'hltVerticesPF']),
    ('step 4: particle flow and PF jets', ['hltParticleFlowBlock', 'hltParticleFlow', 'hltAK4PFJets', 'hltFixedGridRhoFastjetAll', 'hltAK4PFFastJetCorrector',
                                           'hltAK4PFRelativeCorrector', 'hltAK4PFAbsoluteCorrector', 'hltAK4PFResidualCorrector', 'hltAK4PFJetsCorrected']),
    ('step 5: the decision', ['hltPFJetsCorrectedMatchedToCaloJets80', 'hltSinglePFJet110', 'hltSinglePFFwdJet140', 'hltDiPFJetAve100ForHFJEC',
                              'hltTriggerSummaryAOD', 'hltDatasetJetMET']),
]
for title, labels in steps:
    print('\n=== %s' % title)
    for l in labels:
        dump(process, l)
# the event-setup products of the jet corrections
print('\n=== the jet energy corrections (event setup)')
for label, m in process.es_producers_().items():
    if 'JetCorrection' in m.type_() or 'Corrector' in label:
        print('   %-48s %s' % (label, m.type_()))
