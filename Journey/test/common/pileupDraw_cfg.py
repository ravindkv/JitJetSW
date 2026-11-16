# The Poisson draw of the MixingModule at generator level: how many library collisions go into every bunch crossing
# of one signal event, and which ones (test/08_PU/run_08_PU.sh; the table is made by common/pileupDraw.py).
#
# The MixingModule of the overlay (JitJetSW/Journey/genJetPileup_cff.py: no digitiser, the HepMC record as the only
# mixed object, the library read in sequence) with the settings of the pileup scenario AVE_<PU>_BX_25ns of
# Configuration/StandardSequences/python/Mixing.py (mix_POISSON_average_cfi: type 'poisson', a Poisson draw of mean PU
# in every bunch crossing) and the window minBunch..maxBunch, seeded like every cmsDriver configuration
# (IOMC/RandomEngine/python/IOMC_cff.py: mix initialSeed = 12345, MixMaxRng). The draw is made bunch crossing by
# bunch crossing, early ones first, before anything is read, so it depends only on the seed, the mean and the window:
# with -1..1 it gives the 71 / 68 / 54 of the SimHit mixing of chapter 10. Only the CrossingFramePlaybackInfoNew of
# the mixing is written (the record of which library event went into which bunch crossing).
#
#   cd test/08_PU/out && cmsRun ../../common/pileupDraw_cfg.py inputFiles=file:../../07_GenJet/out/step2_GENJET_07_GenJet.root \
#          pileupFiles=file:step1_GEN_MinBias_08_PU.root eventsToProcess=1:1:1 nPileup=60 minBunch=-12 maxBunch=3 \
#          outputFile=draw_08_PU.root && python3 ../../common/pileupDraw.py draw_08_PU.root --bunches=-12,3
import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing('analysis')
options.outputFile = 'pileupDraw.root'
options.register('pileupFiles', 'file:step1_GEN_MinBias_08_PU.root', VarParsing.multiplicity.list, VarParsing.varType.string,
                 'GEN file(s) of the minimum-bias library (generatorSmeared HepMCProduct)')
options.register('eventsToProcess', '1:1:1', VarParsing.multiplicity.singleton, VarParsing.varType.string, 'signal event(s)')
options.register('nPileup', 60, VarParsing.multiplicity.singleton, VarParsing.varType.int, 'mean of the Poisson draw per bunch crossing')
options.register('minBunch', -12, VarParsing.multiplicity.singleton, VarParsing.varType.int, 'first bunch crossing mixed')
options.register('maxBunch', 3, VarParsing.multiplicity.singleton, VarParsing.varType.int, 'last bunch crossing mixed')
options.register('mixSeed', 12345, VarParsing.multiplicity.singleton, VarParsing.varType.int, 'initialSeed of the mix engine')
options._tags, options._tagOrder = {}, []  # keep outputFile as given
options.parseArguments()

process = cms.Process('DRAW')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.StandardSequences.Services_cff')
process.RandomNumberGeneratorService.mix = cms.PSet(initialSeed=cms.untracked.uint32(options.mixSeed),
                                                    engineName=cms.untracked.string('MixMaxRng'))
process.source = cms.Source('PoolSource', fileNames=cms.untracked.vstring(options.inputFiles),
                            eventsToProcess=cms.untracked.VEventRange(options.eventsToProcess))
process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(-1))

process.load('JitJetSW.Journey.genJetPileup_cff')
process.mix.minBunch = options.minBunch
process.mix.maxBunch = options.maxBunch
process.mix.input.type = 'poisson'
process.mix.input.nbPileupEvents.averageNumber = float(options.nPileup)
process.mix.input.skipEvents = 0
process.mix.input.fileNames = cms.untracked.vstring(options.pileupFiles)
process.p = cms.Path(process.mix)

process.out = cms.OutputModule('PoolOutputModule', fileName=cms.untracked.string(options.outputFile),
                               outputCommands=cms.untracked.vstring('drop *', 'keep CrossingFramePlaybackInfoNew_mix_*_*'))
process.e = cms.EndPath(process.out)
