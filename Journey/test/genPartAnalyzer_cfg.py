# Eta-phi maps of the generator record per generation stage (GenPartAnalyzer).
#
#   cmsRun genPartAnalyzer_cfg.py                                  # reads output_step1_GEN.root
#   cmsRun genPartAnalyzer_cfg.py inputFiles=file:other.root maxEvents=3
#
# Writes genPartAnalyzer_stageN_<name>_run<R>_event<E>.pdf per event for the 5 stages
# (hard process, parton shower, MPI / beam remnants, hadronisation, hadron decays),
# genPartAnalyzer.root with the TH2 (eta, phi, weight pT) per stage, and prints the
# per-stage b / bbar / net b+bbar pT to stdout.
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
options.parseArguments()

process = cms.Process('GENPART')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.cerr.FwkReport.reportEvery = 1

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(options.maxEvents))
process.source = cms.Source('PoolSource', fileNames=cms.untracked.vstring(options.inputFiles))

process.TFileService = cms.Service('TFileService', fileName=cms.string(options.outputFile))

process.genPartAnalyzer = cms.EDAnalyzer('GenPartAnalyzer',
    genParticles=cms.InputTag('genParticles', '', 'GEN'),
    outputPrefix=cms.string(options.outputFile.replace('.root', '')),
    etaMax=cms.double(options.etaMax),
    labelPtMin=cms.double(options.labelPtMin),
    savePDF=cms.bool(True),
    printParticles=cms.bool(options.printParticles),
)

process.p = cms.Path(process.genPartAnalyzer)
