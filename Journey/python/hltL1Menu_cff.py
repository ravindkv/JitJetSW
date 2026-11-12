# hltL1Menu_cff.py: run the HLT menu of the release (GRun, /dev/CMSSW_15_0_0/GRun) on the Level-1 decision of chapter 12,
# which was taken with the 2024 L1 menu of the global tag (L1Menu_Collisions2024_v1_3_0, 408 algorithms).
#
# The GRun menu of CMSSW_15_0_5 is the 2025 menu: some of its L1 seeds (HLTL1TSeed modules, L1SeedsLogicalExpression)
# name algorithms of the 2025 L1 menu that the 2024 menu does not have, and HLTL1TSeed stops the job on them
# ("Algorithm L1_X, requested as seed by a HLT path, cannot be matched to a L1 algo name in any GlobalObjectMap").
# An algorithm that is not in the menu can never fire, so:
#   - in a seed that is an OR of algorithms, the missing algorithms are dropped from the OR (the decision is unchanged:
#     a missing algorithm is a false term);
#   - a seed made only of missing algorithms can never be true: the paths that use it are removed from the menu,
#     together with their entries in the dataset filters (TriggerResultsFilter) and in the datasets / streams PSets.
# Everything done is printed (the chapter quotes it) and written in process.hltL1MenuReport for the record.
#
# Usage (cmsDriver --customise_commands, after the HLT step is loaded):
#   from JitJetSW.Journey.hltL1Menu_cff import adaptSeedsToL1Menu
#   process = adaptSeedsToL1Menu(process, '<path to 12_L1T/out/l1t_12_L1T.log>')
# The list of the L1 algorithms is read from the 'algo:' lines that L1TGlobalAnalyzer wrote in chapter 12 (Rule E):
#   "  algo: bit=19 name=L1_SingleMu3 initial=1 prescaled=1 final=1"
import re
import FWCore.ParameterSet.Config as cms


def l1AlgorithmsFromLog(logFile):
    names = set()
    with open(logFile) as f:
        for line in f:
            if line.startswith('  algo:'):
                m = re.search(r'\bname=(\S+)', line)
                if m:
                    names.add(m.group(1))
    if not names:
        raise RuntimeError('no "algo:" lines in %s: run test/12_L1T/run_12_L1T.sh first' % logFile)
    return names


def adaptSeedsToL1Menu(process, logFile, verbose=True):
    menu = l1AlgorithmsFromLog(logFile)
    report = []
    deadSeeds = set()
    for label, module in process.filters_().items():
        if module.type_() != 'HLTL1TSeed':
            continue
        expr = module.L1SeedsLogicalExpression.value()
        tokens = re.findall(r'L1_\w+', expr)
        missing = [t for t in tokens if t not in menu]
        if not missing:
            continue
        operators = set(re.findall(r'\b(AND|NOT|OR)\b', expr))
        kept = [t for t in tokens if t in menu]
        if operators <= {'OR'} and kept:
            module.L1SeedsLogicalExpression = cms.string(' OR '.join(kept))
            report.append('seed %s: %d of %d algorithms not in the L1 menu, dropped from the OR: %s'
                          % (label, len(missing), len(tokens), ' '.join(missing)))
        else:
            deadSeeds.add(label)
            report.append('seed %s: "%s" cannot fire with this L1 menu (missing: %s)' % (label, expr, ' '.join(missing)))

    removed = []
    for name, path in list(process.paths_().items()):
        if deadSeeds & set(path.moduleNames()):
            removed.append(name)
    for name in removed:
        path = getattr(process, name)
        if process.schedule is not None and path in process.schedule:
            process.schedule.remove(path)
        delattr(process, name)
        report.append('path %s removed' % name)

    # the dataset filters and the datasets / streams PSets name the paths; patterns like HLT_X_v* must still match one
    stems = set(re.sub(r'_v\d+$', '', n) for n in removed)

    def keepCondition(c):
        return re.sub(r'_v(\d+|\*)$', '', c.split()[0].strip()) not in stems

    for label, module in process.filters_().items():
        if module.type_() == 'TriggerResultsFilter' and hasattr(module, 'triggerConditions'):
            before = list(module.triggerConditions)
            after = [c for c in before if keepCondition(c)]
            if len(after) != len(before):
                module.triggerConditions = cms.vstring(*after)
                report.append('dataset filter %s: %d paths removed from its conditions' % (label, len(before) - len(after)))
    for pset in ('datasets', 'streams'):
        if hasattr(process, pset):
            ps = getattr(process, pset)
            for key in ps.parameterNames_():
                values = list(getattr(ps, key))
                after = [v for v in values if keepCondition(v)]
                if len(after) != len(values):
                    setattr(ps, key, cms.vstring(*after))
    if verbose:
        print('hltL1Menu_cff: %d L1 algorithms in the menu of chapter 12; %d seeds changed, %d paths removed'
              % (len(menu), len([r for r in report if r.startswith('seed')]), len(removed)))
        for r in report:
            print('hltL1Menu_cff:   ' + r)
    process.hltL1MenuReport = cms.PSet(lines=cms.untracked.vstring(*report))
    return process
