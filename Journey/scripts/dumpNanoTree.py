#!/usr/bin/env python3
"""Print every branch of every entry of a NanoAOD file as text.

usage: dumpNanoTree.py file.root [maxEvents]

Per event, branches are grouped per collection (the part of the branch name before the
first underscore, when an nX counter branch exists); every object of a collection is printed
on one line with all its variables, scalar branches one per line, array branches without a
collection as a list. The Runs and LuminosityBlocks trees are printed first.
"""
import sys
import ROOT

ROOT.gROOT.SetBatch(True)
ROOT.gErrorIgnoreLevel = ROOT.kWarning

INT_TYPES = {'Int_t', 'UInt_t', 'Short_t', 'UShort_t', 'Char_t', 'UChar_t', 'Bool_t', 'Long64_t', 'ULong64_t'}


def fmt(leaf, index=0):
    v = leaf.GetValue(index)
    if leaf.GetTypeName() in INT_TYPES:
        return str(int(v))
    return repr(float(v))


def dump_tree(tree, label, max_events=-1):
    branches = [b.GetName() for b in tree.GetListOfBranches()]
    leaves = {b: tree.GetLeaf(b) for b in branches}
    counters = {b for b in branches if b.startswith('n') and len(b) > 1 and b[1].isupper()}
    groups, scalars = {}, []
    for b in branches:
        if b in counters:
            continue
        coll = b.split('_', 1)[0] if '_' in b else None
        if coll and ('n' + coll) in counters:
            groups.setdefault(coll, []).append(b)
        else:
            scalars.append(b)
    n = tree.GetEntries()
    if max_events >= 0:
        n = min(n, max_events)
    print('==== tree %s: entries=%d branches=%d collections=%d scalars=%d' % (label, tree.GetEntries(), len(branches), len(groups), len(scalars)))
    for i in range(n):
        tree.GetEntry(i)
        head = ' '.join('%s=%s' % (k, fmt(leaves[k])) for k in ('run', 'luminosityBlock', 'event') if k in leaves)
        print('\n#### %s entry %d %s' % (label, i, head))
        for b in scalars:
            if b in ('run', 'luminosityBlock', 'event'):
                continue
            leaf = leaves[b]
            if leaf.GetLeafCount() or leaf.GetLen() > 1:
                print('  %s[%d]=[%s]' % (b, leaf.GetLen(), ' '.join(fmt(leaf, j) for j in range(leaf.GetLen()))))
            else:
                print('  %s=%s' % (b, fmt(leaf)))
        for coll in sorted(groups):
            cnt = int(leaves['n' + coll].GetValue())
            print('  == %s n=%d' % (coll, cnt))
            for j in range(cnt):
                print('  %s[%d] %s' % (coll, j, ' '.join('%s=%s' % (b[len(coll) + 1:], fmt(leaves[b], j)) for b in groups[coll])))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    max_events = int(sys.argv[2]) if len(sys.argv) > 2 else -1
    f = ROOT.TFile.Open(sys.argv[1])
    for name in ('Runs', 'LuminosityBlocks'):
        t = f.Get(name)
        if t:
            dump_tree(t, name)
    dump_tree(f.Get('Events'), 'Events', max_events)


if __name__ == '__main__':
    main()
