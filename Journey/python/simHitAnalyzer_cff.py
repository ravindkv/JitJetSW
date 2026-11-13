# SimHitAnalyzer configuration shared by test/common/simHitAnalyzer_cfg.py (a SIM file, one event drawn station by
# station) and the mixing job of chapter 10 (python/mixOnly_cff.py, test/10_SIM/run_10_SIM.sh), where the analyzer
# runs inside the mixing step and reads the CrossingFrame collections of the MixingModule (signal + mixed pileup).
#
#   from JitJetSW.Journey.simHitAnalyzer_cff import simHitAnalyzer, addSimHitAnalyzer
#   process.simHitAnalyzer = simHitAnalyzer(outputDir='SIM_Phase2', sampleLabel='...', phase2=True)
#   process = addSimHitAnalyzer(process, outputDir='mixed', sampleLabel='...')     # DIGI job with pileup (customise_commands)
#
# The stations are TRK (tracker), ECAL (EB + HGCal EE for Phase2, EB + EE + ES for Run 3), HCAL (HB, HO, HF +
# HGCal HSi / HSc, or HB, HE, HO, HF), HGCAL (Phase2 only), MUON (DT, CSC, RPC, GEM) and ALL; see plugins/SimHitAnalyzer.cc.
import FWCore.ParameterSet.Config as cms


def simHitStations(phase2=True, crossingFrame=False, stations=''):
    """The station PSets. With crossingFrame the hit collections are the CrossingFrame products of the MixingModule
    (mix:g4SimHits<Collection>) instead of the SIM collections (g4SimHits:<Collection>)."""
    def tags(names):
        if crossingFrame:
            return cms.VInputTag(*['mix:g4SimHits' + n for n in names])
        return cms.VInputTag(*['g4SimHits:' + n for n in names])

    pixel = ['TrackerHitsPixelBarrelLowTof', 'TrackerHitsPixelBarrelHighTof', 'TrackerHitsPixelEndcapLowTof', 'TrackerHitsPixelEndcapHighTof']
    strip = [] if phase2 else ['TrackerHitsTIBLowTof', 'TrackerHitsTIBHighTof', 'TrackerHitsTIDLowTof', 'TrackerHitsTIDHighTof',
                               'TrackerHitsTOBLowTof', 'TrackerHitsTOBHighTof', 'TrackerHitsTECLowTof', 'TrackerHitsTECHighTof']
    muon = ['MuonDTHits', 'MuonCSCHits', 'MuonRPCHits', 'MuonGEMHits']
    ecal = ['EcalHitsEB'] if phase2 else ['EcalHitsEB', 'EcalHitsEE', 'EcalHitsES']
    hgcal = ['HGCHitsEE', 'HGCHitsHEfront', 'HGCHitsHEback'] if phase2 else []
    ecalGeo = ['EB'] if phase2 else ['EB', 'EE', 'ES']
    hcalGeo = ['HB', 'HO', 'HF'] if phase2 else ['HB', 'HE', 'HO', 'HF']
    hgcGeo = ['HGCEE', 'HGCHSi', 'HGCHSc'] if phase2 else []

    def station(name, title, psim=[], calo=[], geometry=[], zMax=1300., rMax=800., xyMax=800., etaMax=5., caloEntry=False):
        return cms.PSet(name=cms.string(name), title=cms.string(title), psimHits=tags(psim), caloHits=tags(calo),
                        geometry=cms.vstring(*geometry), zMax=cms.double(zMax), rMax=cms.double(rMax), xyMax=cms.double(xyMax),
                        etaMax=cms.double(etaMax), drawCaloEntry=cms.bool(caloEntry))

    result = [
        station('TRK', 'tracker', psim=pixel + strip, geometry=['tracker'], zMax=300., rMax=125., xyMax=125., etaMax=4.5),
        station('ECAL', 'electromagnetic calorimeter' + (' (EB + HGCal EE)' if phase2 else ' (EB, EE, ES)'),
                calo=ecal + (['HGCHitsEE'] if phase2 else []), geometry=['tracker'] + ecalGeo + (['HGCEE'] if phase2 else []),
                zMax=420., rMax=200., xyMax=200., etaMax=3.2, caloEntry=True),
        station('HCAL', 'hadron calorimeter' + (' (HB, HO, HF + HGCal HSi, HSc)' if phase2 else ' (HB, HE, HO, HF)'),
                calo=['HcalHits'] + (['HGCHitsHEfront', 'HGCHitsHEback'] if phase2 else []),
                geometry=['tracker'] + ecalGeo + hcalGeo + hgcGeo, zMax=1300., rMax=450., xyMax=450., etaMax=5.2),
    ]
    if phase2:
        result.append(station('HGCAL', 'high-granularity endcap calorimeter (EE, HSi, HSc)', calo=hgcal, geometry=hgcGeo,
                              zMax=560., rMax=280., xyMax=0., etaMax=3.2, caloEntry=True))
    result += [
        station('MUON', 'muon system (DT, CSC, RPC, GEM)', psim=muon, geometry=['DT', 'CSC', 'RPC', 'GEM'],
                zMax=1150., rMax=760., xyMax=760., etaMax=3.0),
        station('ALL', 'all subdetectors', psim=pixel + strip + muon, calo=ecal + ['HcalHits'] + hgcal,
                geometry=['tracker'] + ecalGeo + hcalGeo + hgcGeo + ['DT', 'CSC', 'RPC', 'GEM'],
                zMax=1300., rMax=800., xyMax=800., etaMax=5.2),
    ]
    if stations:
        wanted = stations.split(',')
        result = [s for s in result if s.name.value() in wanted]
    return result


def simHitAnalyzer(outputDir, sampleLabel='', phase2=True, crossingFrame=False, stations='', highlight=1, highlightPtMin=2.,
                   savePNG=True, printHits=False, hcalTestNumbering=True, prefix='simHits', maxGeoPoints=12000):
    sim = cms.InputTag('mix', 'g4SimHits') if crossingFrame else cms.InputTag('g4SimHits')
    return cms.EDAnalyzer('SimHitAnalyzer',
        simTracks=sim,
        simVertices=sim,
        crossingFrame=cms.bool(bool(crossingFrame)),
        stations=cms.VPSet(*simHitStations(phase2, crossingFrame, stations)),
        outputDir=cms.string(outputDir),
        prefix=cms.string(prefix),
        sampleLabel=cms.string(sampleLabel),
        savePNG=cms.bool(bool(savePNG)),
        hcalTestNumbering=cms.bool(bool(hcalTestNumbering)),
        printHits=cms.bool(bool(printHits)),
        highlight=cms.int32(int(highlight)),
        highlightPtMin=cms.double(float(highlightPtMin)),
        maxGeoPoints=cms.int32(int(maxGeoPoints)),
    )


def addSimHitAnalyzer(process, outputDir, sampleLabel='', phase2=None, **kwargs):
    """Add SimHitAnalyzer to a DIGI job with pileup mixing (cmsDriver --customise_commands). The MixingModule is
    told to build the CrossingFrames of every hit collection and of the SimTracks / SimVertices (by default it
    only keeps the ones its digitisers need), and the analyzer reads them in an EndPath after the digitisation.
    phase2 is detected from the HGCal digitiser of the mix module when not given."""
    if not hasattr(process, 'mix'):
        raise RuntimeError('addSimHitAnalyzer: no MixingModule "mix" in the process (run it in a DIGI step with --pileup)')
    if phase2 is None:
        phase2 = hasattr(process.mix.digitizers, 'hgceeDigitizer')
    mo = process.mix.mixObjects
    mo.mixSH.crossingFrames = cms.untracked.vstring(*mo.mixSH.subdets.value())
    mo.mixCH.crossingFrames = cms.untracked.vstring(*mo.mixCH.subdets.value())
    mo.mixTracks.makeCrossingFrame = cms.untracked.bool(True)
    mo.mixVertices.makeCrossingFrame = cms.untracked.bool(True)
    process.simHitAnalyzer = simHitAnalyzer(outputDir, sampleLabel, phase2=phase2, crossingFrame=True, **kwargs)
    process.simHitAnalyzer_step = cms.EndPath(process.simHitAnalyzer)
    if hasattr(process, 'schedule') and process.schedule is not None:
        process.schedule.append(process.simHitAnalyzer_step)
    return process
