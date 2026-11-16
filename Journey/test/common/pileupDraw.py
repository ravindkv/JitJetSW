#!/usr/bin/env python3
"""The table of the Poisson draw of the MixingModule (the output of common/pileupDraw_cfg.py): for every bunch crossing
of the window, how many library collisions it got and which library events they are (the library is read in sequence,
early crossings first), then the early / in-time / late sums. The last line, 'skip <S> intime <N> total <T>', is read by
test/08_PU/run_08_PU.sh: the overlay of chapter 08 takes library events S+1 .. S+N.

  python3 pileupDraw.py draw_08_PU.root --bunches=-12,3 [--library 1000]

--library: the number of events of the library; a draw that needs more reads the file again from its first event
(EmbeddedRootSource in sequence wraps around), which is flagged.
"""
import argparse
import sys

import ROOT  # noqa: F401  (loads the FWLite dictionaries)
from DataFormats.FWLite import Events, Handle

parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('file')
parser.add_argument('--bunches', default='-12,3', help='minBunch,maxBunch of the mixing')
parser.add_argument('--library', type=int, default=0, help='events in the library (0: unknown)')
args = parser.parse_args()
lo, hi = (int(x) for x in args.bunches.split(','))

handle = Handle('CrossingFramePlaybackInfoNew')
for event in Events(args.file):
    aux = event.eventAuxiliary()
    event.getByLabel('mix', handle)
    info = handle.product()
    print(f"== the draw for signal event {aux.run()}:{aux.luminosityBlock()}:{aux.event()}: bunch crossings {lo} .. {hi}")
    print(f"{'bx':>4} {'t [ns]':>7} {'collisions':>10}  library events")
    offset, early, intime, late, wrapped = 0, 0, 0, 0, False
    for bx in range(lo, hi + 1):
        n = info.getNumberOfEvents(bx, 0)
        ids = [info.getEventId(offset + k).eventID().event() for k in range(n)]
        offset += n
        consecutive = all(b == a + 1 for a, b in zip(ids, ids[1:]))
        wrapped |= not consecutive or (args.library and ids and ids[-1] > args.library)
        span = f"{ids[0]}-{ids[-1]}" if ids else '-'
        print(f"{bx:>4} {25 * bx:>7} {n:>10}  {span}{'' if consecutive else '  (the library starts again)'}")
        if bx < 0:
            early += n
        elif bx == 0:
            intime = n
        else:
            late += n
    total = early + intime + late
    print(f"early (bx {lo} .. -1): {early}   in time (bx 0): {intime}, library events {early + 1}-{early + intime}   "
          f"late (bx 1 .. {hi}): {late}   all: {total}")
    if args.library:
        print(f"library: {args.library} events, {'too small: some collisions are used twice' if total > args.library or wrapped else 'every collision used at most once'}")
    print(f"skip {early} intime {intime} total {total}")
    break
else:
    sys.exit(f"no event in {args.file}")
