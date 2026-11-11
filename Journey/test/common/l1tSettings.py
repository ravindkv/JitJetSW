#!/usr/bin/env python3
# l1tSettings.py: the parameters of every module of the Level-1 emulation (chapter 12), read from the configurations
# themselves so that the book quotes what CMSSW actually ran.
#   python3 l1tSettings.py <step4_L1_..._cfg.py> [<step3_DIGI_..._cfg.py>]
# Section 1: the modules of the L1 path in the order of the hardware chain, with their C++ type. Section 2: every
# parameter of every module. Section 3 (with the DIGI configuration of chapter 11): the two trigger-primitive
# emulators that run inside the DIGI step, simEcalTriggerPrimitiveDigis and simHcalTriggerPrimitiveDigis, and the
# event-setup producers of the HCAL trigger-primitive scale. What is not here is in the database: the menu
# (L1TUtmTriggerMenuRcd), the calorimeter-trigger parameters (L1TCaloParamsRcd), the muon track-finder parameters;
# the analyzers print those (out/l1t_<case>_12_L1T.log).
import os, sys

def load(cfg):
    argv = sys.argv
    sys.argv = [cfg]                      # the configuration must not see our arguments
    ns = {}
    exec(compile(open(cfg).read(), cfg, 'exec'), ns)
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
                if depth < 4 and i < 4:
                    yield from walk(p, '%s%s[%d].' % (prefix, n, i), depth + 1)
            if len(v) > 4:
                yield prefix + n, '(%d parameter sets, the first 4 listed)' % len(v)
        elif hasattr(v, 'parameterNames_'):
            if depth < 4:
                yield from walk(v, prefix + n + '.', depth + 1)
        else:
            try:
                val = v.value() if hasattr(v, 'value') else v
            except Exception:
                val = v
            s = str(val)
            yield prefix + n, s if len(s) < 150 else s[:147] + '...'

def dump(label, mod):
    print('\n-- %s (%s)' % (label, mod.type_() if hasattr(mod, 'type_') else 'PSet'))
    for name, val in walk(mod):
        print('   %-48s %s' % (name, val))

l1cfg = sys.argv[1]
process = load(l1cfg)
print('== l1tSettings: %s' % os.path.basename(l1cfg))
print('process %s' % process.name_())
# the order of the hardware chain rather than the alphabetical order of the task
order = ['simCaloStage2Layer1Digis', 'simCaloStage2Layer1Summary', 'simCaloStage2Digis',
         'simDtTriggerPrimitiveDigis', 'simCscTriggerPrimitiveDigis', 'simCscTriggerPrimitiveDigisRun3', 'simMuonGEMPadDigis', 'simMuonGEMPadDigiClusters',
         'simTwinMuxDigis', 'simBmtfDigis', 'simKBmtfStubs', 'simKBmtfDigis', 'simOmtfDigis', 'simEmtfDigis', 'simEmtfShowers',
         'simGmtCaloSumDigis', 'simGmtStage2Digis', 'simGmtShowerDigis', 'simGtStage2Digis']
in_task = sorted(process.SimL1EmulatorTask.moduleNames())
labels = [m for m in order if m in in_task] + [m for m in in_task if m not in order]
print('\n== 1. The modules of SimL1Emulator (%d), in the order of the hardware chain' % len(labels))
for m in labels:
    print('   %-34s %s' % (m, getattr(process, m).type_()))
print('\n== 2. The parameters')
for m in labels:
    dump(m, getattr(process, m))
if len(sys.argv) > 2:
    digi = load(sys.argv[2])
    print('\n== 3. The trigger-primitive emulators of the DIGI step (%s)' % os.path.basename(sys.argv[2]))
    for m in ('simEcalTriggerPrimitiveDigis', 'simHcalTriggerPrimitiveDigis'):
        if hasattr(digi, m):
            dump(m, getattr(digi, m))
    for m in ('HcalTPGCoderULUT', 'CaloTPGTranscoder', 'HcalTrigTowerGeometryESProducer', 'tpScales', 'EcalTrigPrimESProducer'):
        if hasattr(digi, m):
            dump(m + ' (event setup)', getattr(digi, m))
