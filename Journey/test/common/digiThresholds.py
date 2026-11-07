#!/usr/bin/env python3
# digiThresholds.py: the thresholds, noise and zero-suppression settings of every digitiser of a DIGI configuration
# (chapter 11), read from the configuration itself so that the book quotes the numbers CMSSW actually used.
#   python3 digiThresholds.py <step3_DIGI_..._cfg.py>
# Section 1 lists, per digitiser (process.mix.digitizers.<name>) and per digi producer after the mixing (simEcalDigis,
# simEcalPreshowerDigis, simHcalDigis, simHcalUnsuppressedDigis, simMuon*Digis, simSiStripDigis ...), every parameter whose name
# suggests a threshold, a noise, a zero suppression, a readout window or a cut; section 2 dumps the full parameter set
# of the tracker, ECAL and HCAL digitisers and of the selective-readout / zero-suppression producers for the companion.
import os, re, sys

cfg = sys.argv[1]
sys.argv = [cfg]                      # the configuration must not see our arguments
sys.path.insert(0, os.path.dirname(os.path.abspath(cfg)))
src = open(cfg).read()
ns = {}
exec(compile(src, cfg, 'exec'), ns)
process = ns['process']
import FWCore.ParameterSet.Config as cms

keywords = re.compile(r'thresh|noise|zero|suppr|level|cut|tof|window|pedestal|gain|electronperadc|adc|eff|dead|kill|srp|zs|lsb|min|max|sigma|smear|frame|sample|bin', re.I)

def walk(ps, prefix, depth=0):
    """yield (dotted name, value) of every parameter of a PSet / module, recursively"""
    try:
        names = ps.parameterNames_()
    except Exception:
        return
    for n in sorted(names):
        try:
            v = getattr(ps, n)
        except Exception:
            continue
        if isinstance(v, cms.PSet) or (hasattr(v, 'parameterNames_') and not isinstance(v, cms.VPSet)):
            if depth < 4:
                yield from walk(v, prefix + n + '.', depth + 1)
        elif isinstance(v, cms.VPSet):
            for i, p in enumerate(v):
                if depth < 4:
                    yield from walk(p, '%s%s[%d].' % (prefix, n, i), depth + 1)
        else:
            try:
                val = v.value() if hasattr(v, 'value') else v
            except Exception:
                val = v
            yield prefix + n, val

def fmt(val):
    s = str(val)
    return s if len(s) < 140 else s[:137] + '...'

print('== digiThresholds: %s' % os.path.basename(cfg))
print('process %s, era modifiers: %s' % (process.name_(), ', '.join(getattr(process, 'era', None) and [process.era] or [])))
mods = []
if hasattr(process, 'mix') and hasattr(process.mix, 'digitizers'):
    for n in sorted(process.mix.digitizers.parameterNames_()):
        mods.append(('mix.digitizers.' + n, getattr(process.mix.digitizers, n)))
    mods.append(('mix.input (the pileup)', process.mix.input))
    mods.append(('mix (bunch crossings)', process.mix))
for n in ('simEcalDigis', 'simEcalPreshowerDigis', 'simHcalDigis', 'simHcalUnsuppressedDigis', 'simSiStripDigis', 'simSiPixelDigis',
          'simMuonDTDigis', 'simMuonCSCDigis', 'simMuonRPCDigis', 'simMuonGEMDigis', 'simEcalTriggerPrimitiveDigis', 'simHcalTriggerPrimitiveDigis'):
    if hasattr(process, n):
        mods.append((n, getattr(process, n)))

print('\n-- 1. parameters that look like thresholds, noise, zero suppression, readout windows or cuts')
for label, m in mods:
    rows = [(k, v) for k, v in walk(m, '') if keywords.search(k) and not k.startswith('mixObjects')]
    if not rows:
        continue
    print('[%s] %s' % (label, getattr(m, 'type_', lambda: '')() if hasattr(m, 'type_') else ''))
    for k, v in rows:
        print('   %-60s %s' % (k, fmt(v)))

print('\n-- 2. the full parameter sets of the tracker, calorimeter and zero-suppression modules')
for label, m in mods:
    if not any(x in label for x in ('pixel', 'strip', 'ecal', 'hcal', 'simEcalDigis', 'simHcalDigis', 'simEcalPreshowerDigis', 'mix (bunch')):
        continue
    print('[%s] %s' % (label, getattr(m, 'type_', lambda: '')() if hasattr(m, 'type_') else ''))
    for k, v in walk(m, ''):
        if k.startswith('mixObjects') or k.startswith('digitizers'):
            continue
        print('   %-60s %s' % (k, fmt(v)))
