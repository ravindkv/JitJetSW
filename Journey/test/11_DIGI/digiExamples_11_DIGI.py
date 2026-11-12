#!/usr/bin/env python3
# digiExamples_11_DIGI.py: the worked examples of chapter 11 (DIGI), read with FWLite from the DIGI files of the chapter
# and from the release (the APV pulse shape). No cmsRun: it only reads what run_11_DIGI.sh wrote.
#   python3 digiExamples_11_DIGI.py <test/11_DIGI/out>  > out/examples_11_DIGI.txt
# Sections:
#   1. the silicon: sensor thickness and charge of a crossing (PSimHits of the signal), what the pixel and strip
#      digitisers make of it (ADC of the digis, linked and unlinked), the Phase-1 pixel ADC response
#   2. the APV25 deconvolution pulse of the release, read at the times of bunch crossings -1, 0, +1
#   3. the ECAL: the largest barrel and endcap data frames of the signal, sample by sample, with the energy the
#      particles left in that crystal (PCaloHits), and the pedestal / noise of all the frames written
#   4. the HCAL: the largest HB/HE (QIE11) and HF (QIE10) frames of the signal, time slice by time slice
#   5. the muon chambers of the mixed event: DT drift times, CSC strip samples, RPC and GEM bunch crossings
import math, os, sys
import ROOT
ROOT.gROOT.SetBatch(True)
ROOT.gSystem.Load('libFWCoreFWLite')
ROOT.FWLiteEnabler.enable()
from DataFormats.FWLite import Events, Handle

out = sys.argv[1] if len(sys.argv) > 1 else '.'
files = {c: os.path.join(out, 'step3_DIGI_%s_11_DIGI.root' % c) for c in ('signal', 'pileup', 'mixed')}
GEV_PER_E = 3.61e-9  # SiPixelDigitizerAlgorithm.cc / SiStripSimParameters: 3.61 eV per electron-hole pair


def first_event(path):
    for ev in Events(path):
        return ev


def get(ev, typ, label):
    h = Handle(typ)
    ev.getByLabel(label, h)
    return h.product() if h.isValid() else None


def median(v):
    v = sorted(v)
    n = len(v)
    if n == 0:
        return float('nan')
    return v[n // 2] if n % 2 else 0.5 * (v[n // 2 - 1] + v[n // 2])


def quant(v, q):
    v = sorted(v)
    return v[min(len(v) - 1, int(q * len(v)))] if v else float('nan')


ev = first_event(files['signal'])
print('== digiExamples 11_DIGI: worked examples from %s' % out)

# ---------------------------------------------------------------- 1. silicon
print('\n-- 1. The silicon: the PSimHits of the signal, the digis made of them')
subs = {'PXB': 'PixelBarrel', 'PXF': 'PixelEndcap', 'TIB': 'TIB', 'TID': 'TID', 'TOB': 'TOB', 'TEC': 'TEC'}
for sub, name in subs.items():
    hits = []
    for tof in ('LowTof', 'HighTof'):
        c = get(ev, 'std::vector<PSimHit>', ('g4SimHits', 'TrackerHits%s%s' % (name, tof)))
        if c:
            hits += list(c)
    dz = [abs(h.exitPoint().z() - h.entryPoint().z()) for h in hits]
    thick = max(dz) if dz else 0.
    full = [h for h, d in zip(hits, dz) if d > 0.98 * thick and h.energyLoss() > 0]
    el = [h.energyLoss() / GEV_PER_E for h in full]
    # charge per micrometre of path (3D length entry -> exit) of the crossings of the full thickness: the median of a
    # Landau-like distribution; times the thickness it is the charge of a crossing at normal incidence
    perum = [h.energyLoss() / GEV_PER_E / ((h.exitPoint() - h.entryPoint()).mag() * 1e4) for h in full]
    print('  %-3s PSimHits=%5d  thickness(max |dz local|)=%.0f um  crossings of the full thickness=%d: charge median=%.0f e '
          '(25%%: %.0f, 75%%: %.0f) | per um of path: median %.1f e (25%%: %.1f) -> at normal incidence %.0f e' % (
              sub, len(hits), thick * 1e4, len(full), median(el), quant(el, 0.25), quant(el, 0.75), median(perum),
              quant(perum, 0.25), median(perum) * thick * 1e4))

pix = get(ev, 'edm::DetSetVector<PixelDigi>', ('simSiPixelDigis', ''))
adcs = [d.adc() for ds in pix for d in ds]
print('  pixel digis=%d  ADC median=%.0f  25%%=%.0f 75%%=%.0f  at full scale (255)=%d  below 50=%d' % (
    len(adcs), median(adcs), quant(adcs, .25), quant(adcs, .75), sum(1 for a in adcs if a >= 255), sum(1 for a in adcs if a < 50)))
# the Phase-1 pixel ADC response of the digitiser (missCalibrate, parameters of thresholds_11_DIGI.txt)
p0, p1, p2, p3 = 0.00171, 0.711, 203.0, 148.0
def pixel_adc(e, layer1=False):
    vcal = (e - (-670 if layer1 else -60)) / (50 if layer1 else 47)
    return min(255, int(p3 + p2 * math.tanh(p0 * vcal - p1)))
for e in (1600, 2000, 5000, 10000, 15000, 20000, 25000, 30000, 40000):
    print('  pixel ADC response (BPix L2-4, FPix): %6d e -> ADC %3d ; layer 1: ADC %3d' % (e, pixel_adc(e), pixel_adc(e, True)))
# saturation: tanh argument where p3 + p2 tanh = 255
x = math.atanh((255 - p3) / p2)
print('  pixel ADC reaches 255 at %.0f e (L2-4, FPix)' % (((x + p1) / p0) * 47 - 60))

strip = get(ev, 'edm::DetSetVector<SiStripDigi>', ('simSiStripDigis', 'ZeroSuppressed'))
links = get(ev, 'edm::DetSetVector<StripDigiSimLink>', ('simSiStripDigis', ''))
linked = {}
for ds in links:
    s = linked.setdefault(ds.detId(), set())
    for l in ds:
        s.add(l.channel())
lin, noi = [], []
for ds in strip:
    s = linked.get(ds.detId(), set())
    for d in ds:
        (lin if d.strip() in s else noi).append(d.adc())
print('  strip digis linked=%d ADC median=%.0f 25%%=%.0f 75%%=%.0f (>=254: %d) | not linked=%d ADC median=%.0f 25%%=%.0f 75%%=%.0f max=%d' % (
    len(lin), median(lin), quant(lin, .25), quant(lin, .75), sum(1 for a in lin if a >= 254),
    len(noi), median(noi), quant(noi, .25), quant(noi, .75), max(noi) if noi else 0))
# cluster of the strips: consecutive linked strips per module
nclu, nstr = 0, 0
for ds in strip:
    s = linked.get(ds.detId(), set())
    prev = None
    for d in ds:
        if d.strip() in s:
            if prev is None or d.strip() != prev + 1:
                nclu += 1
            nstr += 1
            prev = d.strip()
        else:
            prev = None
print('  linked strip clusters (runs of adjacent linked strips)=%d, strips per cluster=%.2f' % (nclu, nstr / max(1, nclu)))

# ---------------------------------------------------------------- 2. the APV pulse
print('\n-- 2. The APV25 deconvolution pulse of the release (SimTracker/SiStripDigitizer/data/APVShapeDeco_320.txt)')
base = os.environ.get('CMSSW_RELEASE_BASE', '')
path = os.path.join(base, 'src/SimTracker/SiStripDigitizer/data/APVShapeDeco_320.txt')
vals, res = [], None
for line in open(path):
    line = line.strip()
    if not line or line.startswith('#'):
        continue
    if line.startswith('resolution:'):
        res = float(line.split(':')[1])
        continue
    vals += [float(x) for x in line.split()]
i0 = vals.index(max(vals))
print('  %d values, resolution %.1f ns, maximum at index %d' % (len(vals), res, i0))
print('  SiLinearChargeDivider::TimeResponse: the charge is multiplied by shape[t0 + (r/c - tof)/resolution];'
      ' a hit of bunch crossing -1 has tof shifted by -25 ns, i.e. it is read 25 ns AFTER its peak')
for dt in (-30, -25, -20, -15, -12.5, -10, -5, 0, 5, 10, 12.5, 15, 20, 25, 30, 40, 50):
    k = i0 + int(dt / res)
    v = vals[k] if 0 <= k < len(vals) else 0.
    print('  read %+5.1f ns after the peak: %.3f' % (dt, v))
# the curve for the TikZ figure, every 2.5 ns
pts = []
for k in range(-int(40 / res), int(80 / res) + 1, int(2.5 / res)):
    j = i0 + k
    if 0 <= j < len(vals):
        pts.append('(%.1f,%.3f)' % (k * res, vals[j]))
print('  curve (t after the peak [ns], shape): ' + ' '.join(pts))
# where the rising edge starts (first value above 1 % of the peak)
first = next(j for j, v in enumerate(vals) if v > 0.01)
zero = next(j for j in range(i0, len(vals)) if vals[j] < 0)
print('  rises above 0.01 at %.1f ns before the peak; crosses zero %.1f ns after the peak; minimum %.3f at %.1f ns' % (
    (i0 - first) * res, (zero - i0) * res, min(vals), (vals.index(min(vals)) - i0) * res))

# ---------------------------------------------------------------- 3. ECAL
print('\n-- 3. The ECAL data frames of the signal (simEcalDigis), and the energy the particles left in the crystal')
for part, ctype, dtype, hitlabel in (('EB', 'EBDigiCollection', 'EBDataFrame', 'EcalHitsEB'),
                                     ('EE', 'EEDigiCollection', 'EEDataFrame', 'EcalHitsEE')):
    coll = get(ev, ctype, ('simEcalDigis', part.lower() + 'Digis'))
    hits = get(ev, 'std::vector<PCaloHit>', ('g4SimHits', hitlabel))
    edep = {}
    for h in hits:
        if 0 <= h.time() < 100:  # the in-time part; CaloSD keeps hits up to 500 ns
            edep[h.id()] = edep.get(h.id(), 0.) + h.energy()
    frames, peds = [], []
    gains = {}
    for i in range(coll.size()):
        df = getattr(ROOT, dtype)(coll[i])
        s = [df.sample(j).adc() for j in range(df.size())]
        g = [df.sample(j).gainId() for j in range(df.size())]
        for gg in g:
            gains[gg] = gains.get(gg, 0) + 1
        ped = sum(s[:3]) / 3.
        peds += s[:3]
        amp = max(s) - ped
        frames.append((amp, df.id().rawId(), s, g, ped))
    frames.sort(reverse=True)
    import statistics
    print('  %s: %d frames, %d samples each; gain ids of all samples: %s; first three samples of all frames: mean %.1f, rms %.2f ADC' % (
        part, len(frames), len(frames[0][2]) if frames else 0, dict(sorted(gains.items())), statistics.mean(peds),
        statistics.pstdev(peds)))
    for amp, raw, s, g, ped in frames[:5]:
        did = ROOT.EBDetId(raw) if part == 'EB' else ROOT.EEDetId(raw)
        where = ('ieta=%d iphi=%d' % (did.ieta(), did.iphi())) if part == 'EB' else ('ix=%d iy=%d z=%d' % (did.ix(), did.iy(), did.zside()))
        e = edep.get(raw, 0.)
        print('    %s amplitude=%.1f ADC (max - mean of first 3) deposit (0-100 ns)=%.3f GeV -> %.1f ADC/GeV | samples %s gains %s' % (
            where, amp, e, amp / e if e > 0 else float('nan'), s, g))
    # frames with a deposit: ADC per GeV over all crystals with > 0.5 GeV
    ratios = [(f[0] / edep[f[1]], edep[f[1]]) for f in frames if edep.get(f[1], 0) > 0.5]
    if ratios:
        print('    crystals with a deposit > 0.5 GeV and a frame: %d, ADC per GeV median %.1f (deposits %.2f-%.2f GeV)' % (
            len(ratios), median([r for r, _ in ratios]), min(e for _, e in ratios), max(e for _, e in ratios)))
    # the selective-readout flag of the tower / supercrystal of every frame: full readout (3) or zero suppression (1)
    srf = get(ev, 'edm::SortedCollection<%sSrFlag,edm::StrictWeakOrdering<%sSrFlag> >' % (part, part),
              ('simEcalDigis', part.lower() + 'SrFlags'))
    full = set(f.id().rawId() for f in srf if (f.value() & 3) == 3)
    if part == 'EB':
        infull = sum(1 for f in frames if ROOT.EBDetId(f[1]).tower().rawId() in full)
    else:
        infull = sum(1 for f in frames if ROOT.EEDetId(f[1]).sc().rawId() in full)
    print('    frames in a tower/supercrystal read in full: %d; frames that passed the zero suppression elsewhere: %d' % (
        infull, len(frames) - infull))
    ncells = len([k for k, v in edep.items()])
    nwith = len([1 for f in frames if f[1] in edep])
    print('    crystals with an in-time deposit: %d (%.2f GeV); of them with a frame: %d; frames with no deposit: %d' % (
        ncells, sum(edep.values()), nwith, len(frames) - nwith))
    written = set(f[1] for f in frames)
    if part == 'EE':
        # where the endcap crystals with more than 1 GeV are, written or not (eta approximate: ix, iy at z = 317 cm,
        # 2.862 cm per crystal, as in section 4.9 of the companion)
        rows = []
        for k, v in edep.items():
            if v > 1.0:
                e = ROOT.EEDetId(k)
                r = math.hypot(e.ix() - 50.5, e.iy() - 50.5) * 2.862
                eta = -math.log(math.tan(math.atan2(r, 317.) / 2)) * e.zside()
                rows.append((eta, v, k in written))
        for flag in (True, False):
            sel = [x for x in rows if x[2] == flag]
            if sel:
                print('    EE crystals > 1 GeV %s: %d, |eta| %.2f-%.2f (approx.), deposits %.2f-%.2f GeV' % (
                    'written' if flag else 'NOT written', len(sel), min(abs(x[0]) for x in sel), max(abs(x[0]) for x in sel),
                    min(x[1] for x in sel), max(x[1] for x in sel)))
    for thr in (0.1, 0.3325, 0.5, 1.0):
        n = sum(1 for k, v in edep.items() if v > thr)
        w = sum(1 for k, v in edep.items() if v > thr and k in written)
        print('    deposit > %.4f GeV: %d crystals, %d of them written' % (thr, n, w))

# ---------------------------------------------------------------- 4. HCAL
print('\n-- 4. The HCAL frames of the signal (simHcalDigis)')
for label, ftype, name in ((('simHcalDigis', 'HBHEQIE11DigiCollection'), 'QIE11DataFrame', 'HB/HE QIE11'),
                           (('simHcalDigis', 'HFQIE10DigiCollection'), 'QIE10DataFrame', 'HF QIE10')):
    coll = get(ev, 'HcalDataFrameContainer<%s>' % ftype, label)
    fr = []
    for i in range(coll.size()):
        df = getattr(ROOT, ftype)(coll[i])
        s = [df[j].adc() for j in range(df.samples())]
        t = [df[j].tdc() if hasattr(df[j], 'tdc') else -1 for j in range(df.samples())]
        soi = [j for j in range(df.samples()) if df[j].soi()]
        did = ROOT.HcalDetId(df.detid().rawId())
        fr.append((max(s) - min(s), s, t, soi, did.subdet(), did.ieta(), did.iphi(), did.depth()))
    fr.sort(reverse=True)
    allmin = [min(f[1]) for f in fr]
    print('  %s: %d frames of %d time slices, sample of interest at index %s; median of the smallest sample of each frame: %.0f' % (
        name, len(fr), len(fr[0][1]), fr[0][3], median(allmin)))
    for f in fr[:4]:
        print('    subdet=%d ieta=%d iphi=%d depth=%d  ADC %s  TDC %s' % (f[4], f[5], f[6], f[7], f[1], f[2]))
    # a typical frame kept with no signal: the median frame by its range
    mid = fr[len(fr) // 2]
    print('    median frame (by max - min): subdet=%d ieta=%d iphi=%d depth=%d  ADC %s  TDC %s' % (mid[4], mid[5], mid[6], mid[7], mid[1], mid[2]))

# ---------------------------------------------------------------- 5. muon chambers of the mixed event
print('\n-- 5. The muon digis of the mixed event (simMuon*Digis); the iteration is done by a small C++ helper')
ROOT.gInterpreter.Declare(r"""
#include "DataFormats/DTDigi/interface/DTDigiCollection.h"
#include "DataFormats/CSCDigi/interface/CSCStripDigiCollection.h"
#include "DataFormats/CSCDigi/interface/CSCWireDigiCollection.h"
#include "DataFormats/RPCDigi/interface/RPCDigiCollection.h"
#include "DataFormats/GEMDigi/interface/GEMDigiCollection.h"
#include <sstream>
std::vector<std::string> jjDT(const DTDigiCollection& c) {
  std::vector<std::string> v;
  for (auto it = c.begin(); it != c.end(); ++it) {
    const DTLayerId& id = (*it).first;
    for (auto d = (*it).second.first; d != (*it).second.second; ++d) {
      std::ostringstream o; o << id.wheel() << " " << id.station() << " " << id.sector() << " " << id.superlayer() << " "
        << id.layer() << " " << d->wire() << " " << d->time(); v.push_back(o.str()); } }
  return v; }
std::vector<std::string> jjCSCstrip(const CSCStripDigiCollection& c) {
  std::vector<std::string> v;
  for (auto it = c.begin(); it != c.end(); ++it) {
    const CSCDetId& id = (*it).first;
    for (auto d = (*it).second.first; d != (*it).second.second; ++d) {
      std::ostringstream o; o << id.endcap() << " " << id.station() << " " << id.ring() << " " << id.chamber() << " "
        << id.layer() << " " << d->getStrip();
      for (int a : d->getADCCounts()) o << " " << a;
      v.push_back(o.str()); } }
  return v; }
std::vector<std::string> jjCSCwire(const CSCWireDigiCollection& c) {
  std::vector<std::string> v;
  for (auto it = c.begin(); it != c.end(); ++it) {
    const CSCDetId& id = (*it).first;
    for (auto d = (*it).second.first; d != (*it).second.second; ++d) {
      std::ostringstream o; o << id.endcap() << " " << id.station() << " " << id.ring() << " " << id.chamber() << " "
        << id.layer() << " " << d->getWireGroup() << " " << d->getTimeBin(); v.push_back(o.str()); } }
  return v; }
std::vector<std::string> jjRPC(const RPCDigiCollection& c) {
  std::vector<std::string> v;
  for (auto it = c.begin(); it != c.end(); ++it) {
    const RPCDetId& id = (*it).first;
    for (auto d = (*it).second.first; d != (*it).second.second; ++d) {
      std::ostringstream o; o << id.region() << " " << id.station() << " " << id.ring() << " " << d->strip() << " " << d->bx();
      v.push_back(o.str()); } }
  return v; }
std::vector<std::string> jjGEM(const GEMDigiCollection& c) {
  std::vector<std::string> v;
  for (auto it = c.begin(); it != c.end(); ++it) {
    const GEMDetId& id = (*it).first;
    for (auto d = (*it).second.first; d != (*it).second.second; ++d) {
      std::ostringstream o; o << id.region() << " " << id.station() << " " << id.chamber() << " " << id.layer() << " "
        << d->strip() << " " << d->bx(); v.push_back(o.str()); } }
  return v; }
""")
evm = first_event(files['mixed'])
dt = [l.split() for l in ROOT.jjDT(get(evm, 'MuonDigiCollection<DTLayerId,DTDigi>', ('simMuonDTDigis', '')))]
print('  DT digis=%d (wheel station sector superlayer layer wire time[ns]):' % len(dt))
for d in sorted(dt, key=lambda x: float(x[6])):
    print('    ' + ' '.join(d))
cs = [l.split() for l in ROOT.jjCSCstrip(get(evm, 'MuonDigiCollection<CSCDetId,CSCStripDigi>', ('simMuonCSCDigis', 'MuonCSCStripDigi')))]
ch = {}
for d in cs:
    ch.setdefault(tuple(d[:4]), []).append(d)
print('  CSC strip digis=%d in %d chambers (endcap station ring chamber: strips)' % (len(cs), len(ch)))
for k, v in sorted(ch.items()):
    layers = sorted(set(x[4] for x in v))
    print('    %s: %d strip digis in layers %s' % (' '.join(k), len(v), ','.join(layers)))
# the strip with the largest pulse
best = max(cs, key=lambda d: max(int(a) for a in d[6:]) - min(int(a) for a in d[6:]))
print('  CSC strip with the largest pulse: endcap %s ME%s/%s chamber %s layer %s strip %s, 8 samples (50 ns each): %s' % (
    best[0], best[1], best[2], best[3], best[4], best[5], ' '.join(best[6:])))
amp = sorted(max(int(a) for a in d[6:]) - min(int(a) for a in d[6:]) for d in cs)
print('  CSC strip pulse heights (max - min of the 8 samples): median %d, 90%% %d, max %d' % (median(amp), quant(amp, 0.9), amp[-1]))
med = sorted(cs, key=lambda d: max(int(a) for a in d[6:]) - min(int(a) for a in d[6:]))[len(cs) // 2]
print('  a median strip: ME%s/%s chamber %s layer %s strip %s: %s' % (med[1], med[2], med[3], med[4], med[5], ' '.join(med[6:])))
cw = [l.split() for l in ROOT.jjCSCwire(get(evm, 'MuonDigiCollection<CSCDetId,CSCWireDigi>', ('simMuonCSCDigis', 'MuonCSCWireDigi')))]
tb = {}
for d in cw:
    tb[d[6]] = tb.get(d[6], 0) + 1
print('  CSC wire digis=%d in %d chambers; first time bin: %s' % (len(cw), len(set(tuple(d[:4]) for d in cw)), dict(sorted(tb.items(), key=lambda kv: int(kv[0])))))
rp = [l.split() for l in ROOT.jjRPC(get(evm, 'MuonDigiCollection<RPCDetId,RPCDigi>', ('simMuonRPCDigis', '')))]
print('  RPC digis=%d (region station ring strip bx): %s' % (len(rp), '; '.join(' '.join(d) for d in rp)))
gm = [l.split() for l in ROOT.jjGEM(get(evm, 'MuonDigiCollection<GEMDetId,GEMDigi>', ('simMuonGEMDigis', '')))]
print('  GEM digis=%d (region station chamber layer strip bx): %s' % (len(gm), '; '.join(' '.join(d) for d in gm)))
