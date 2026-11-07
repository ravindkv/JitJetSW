# mixOnly_cff.py: the MixingModule of the DIGI step without the digitisers (chapter 10, SIM, case (a+b)).
#
# cmsDriver.py step3 -s DIGI:pdigi_valid --pileup ... --pileup_input ... --no_output builds the mixing job of the
# production: the MixingModule 'mix' reads the pileup library, stamps every pileup SimTrack / SimVertex / PSimHit /
# PCaloHit with its bunch crossing and event number, shifts their times and hands the signal and pileup objects
# together to the digitisers as CrossingFrames. This customisation keeps the mixing and removes the digitisers, so
# that the job stops exactly at the SimHit level of the mixed event: the hits of the signal and of the mixed
# minimum-bias collisions, before any detector electronics. SimHitAnalyzer draws them from the CrossingFrames
# (they are transient and never written to a file), as in simHitAnalyzer_cff.addSimHitAnalyzer.
# The output module of the job is kept: it writes the signal event again (the CrossingFrames are transient) with the
# CrossingFramePlaybackInfoNew of the mixing, which library events went into which bunch crossing: the record that
# the MixingModule of chapter 11 replays (process.mix.playback = True), so that the digitised event holds exactly the
# collisions drawn here (a new Poisson draw there would differ: the digitisers of the signal consume the engine first).
#
#   --customise_commands "from JitJetSW.Journey.mixOnly_cff import mixOnly; process = mixOnly(process, outputDir='mixed', sampleLabel='...', prefix='simHits_mixed_10_SIM')"
import FWCore.ParameterSet.Config as cms
from JitJetSW.Journey.simHitAnalyzer_cff import addSimHitAnalyzer


def mixOnly(process, outputDir=None, sampleLabel='', **kwargs):
    # outputDir=None: the mixing and the playback record only, no SimHitAnalyzer (a quick check job)
    if not hasattr(process, 'mix'):
        raise RuntimeError('mixOnly: no MixingModule "mix" in the process (run it in a DIGI step with --pileup)')
    process.mix.digitizers = cms.PSet()                       # no digitiser, no TrackingTruth accumulator
    # the DIGI step aliases (SimGeneral/MixingModule/python/aliases_cfi.py: simSiPixelDigis, simSiStripDigis,
    # simEcalUnsuppressedDigis, simHcalUnsuppressedDigis, simAPVsaturation, ...) point at products of the digitisers
    # inside 'mix'; without digitisers they would fail the EDAlias check at start-up, so they go too
    for name, alias in list(process.aliases_().items()):
        if hasattr(alias, 'mix'):
            delattr(process, name)
    process.digitisation_step = cms.Path(process.mix)         # instead of pdigi_valid (digitisers, pileup summary, ...)
    if outputDir is not None:
        process = addSimHitAnalyzer(process, outputDir, sampleLabel, phase2=False, **kwargs)
    return process
