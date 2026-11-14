# Eta-phi maps of the generator record per generation stage (GenPartAnalyzer).
#
#   cmsRun genPartAnalyzer_cfg.py                                  # reads output_step1_GEN.root
#   cmsRun genPartAnalyzer_cfg.py inputFiles=file:other.root maxEvents=3
#
# Writes genPartAnalyzer_stageN_<name>_run<R>_event<E>.pdf per event for the 5 stages
# (hard process, parton shower, MPI / beam remnants, hadronisation, hadron decays),
# genPartAnalyzer.root with the TH2 (eta, phi, weight pT) per stage, and prints the
# per-stage b / bbar / net b+bbar pT to stdout.
#
# For a sample generated with later Pythia8 levels off (the generator chapters 01-06, gen_stage of
# test/common/jitjet_common.sh) limit the stages and collect the pages in one PDF, e.g. for chapter 02:
#   cd test/02_PS/out && cmsRun ../../common/genPartAnalyzer_cfg.py inputFiles=file:step1_GEN_02_PS.root maxEvents=1 \
#          maxStage=2 drawStages=2 pdfFile=genPartAnalyzer_02_PS.pdf sampleLabel="chapter 02_PS|ISR on, FSR on|MPI off, Had off"
import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing('analysis')
options.inputFiles = 'file:output_step1_GEN.root'
options.outputFile = 'genPartAnalyzer.root'
options.maxEvents = -1
options.register('etaMax', 6.0, VarParsing.multiplicity.singleton, VarParsing.varType.float, 'eta axis range of the plots')
options.register('labelPtMin', 5.0, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'pT above which the pT value is written next to every particle (b, bbar always)')
options.register('printParticles', False, VarParsing.multiplicity.singleton, VarParsing.varType.bool,
                 'print every drawn particle of every stage to stdout')
options.register('maxStage', 5, VarParsing.multiplicity.singleton, VarParsing.varType.int,
                 'last stage to analyse and draw (1 hard process, 2 parton shower, 3 MPI, 4 hadronisation, 5 decays)')
options.register('drawStages', '', VarParsing.multiplicity.list, VarParsing.varType.int,
                 'the analysed stages that get a page, e.g. 2 or 2,3 (default: all; the others are analysed and printed)')
options.register('pdfFile', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'if set, all plots go as pages into this single PDF instead of one PDF per stage and event')
options.register('sampleLabel', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "text printed in the right panel of every plot, '|' separates lines, e.g. the generator stage")
options.register('coneR', 0.0, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'if > 0: draw the cone Delta R < coneR around the b and the bbar system, print its pT, zoom on the fuller cone')
options.register('eventsToProcess', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'run:lumi:event (or a range) to analyse, e.g. 1:1:1; empty: the first maxEvents events')
options._tags, options._tagOrder = {}, []  # keep outputFile as given (VarParsing would append _numEvent<N> when maxEvents > 0)
options.parseArguments()

process = cms.Process('GENPART')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.cerr.FwkReport.reportEvery = 1

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(options.maxEvents))
process.source = cms.Source('PoolSource', fileNames=cms.untracked.vstring(options.inputFiles))
if options.eventsToProcess:
    process.source.eventsToProcess = cms.untracked.VEventRange(options.eventsToProcess)

process.TFileService = cms.Service('TFileService', fileName=cms.string(options.outputFile))

process.genPartAnalyzer = cms.EDAnalyzer('GenPartAnalyzer',
    genParticles=cms.InputTag('genParticles', '', 'GEN'),
    outputPrefix=cms.string(options.outputFile.replace('.root', '')),
    etaMax=cms.double(options.etaMax),
    labelPtMin=cms.double(options.labelPtMin),
    savePDF=cms.bool(True),
    printParticles=cms.bool(options.printParticles),
    maxStage=cms.int32(options.maxStage),
    drawStages=cms.vint32(options.drawStages),
    pdfFile=cms.string(options.pdfFile),
    sampleLabel=cms.string(options.sampleLabel),
    coneR=cms.double(options.coneR),
)

process.p = cms.Path(process.genPartAnalyzer)
