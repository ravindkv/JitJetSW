# Eta-phi maps of the generator jets drawn over the particles they were clustered from (GenJetAnalyzer).
#
#   cmsRun genJetAnalyzer_cfg.py                                   # reads output_step1_GEN.root
#   cmsRun genJetAnalyzer_cfg.py inputFiles=file:other.root maxEvents=3 jets=ak4GenJets,ak4GenJetsNoNu
#
# Writes genJetAnalyzer_<collection>_run<R>_event<E>.pdf per jet collection and event (every
# jet as a circle of radius R around its axis with its constituents in the jet colour, the
# b / bbar jets matched to the hard-process quarks in red / blue), genJetAnalyzer.root with
# the jet histograms per collection, and prints the jets of every event to stdout.
# With maxStage > 0 (default 5) GenPartAnalyzer runs first in the same job, so that, as in the
# PDFs of the generator stages, every event starts with the eta-phi maps of the generator-record
# stages 1..maxStage (its PDFs / pages, histograms and printout are added to the same outputs).
#
# The jet collections are read from the most recent process that produced them: the GEN step
# (pgen runs the same anti-kT clustering) or the GENJET step of the ME_PS_MPI_Had_Decay_GenJet stage
# of runCMSDrivers_mc2024_BBbar_GEN.sh. The radius, input and pT threshold of every collection
# are taken from the producer configuration stored in the file. For the stage samples of that
# script, collect the pages in one PDF, e.g. for the parton-level ME_PS sample (parton jets):
#   cmsRun genJetAnalyzer_cfg.py inputFiles=file:GEN/step1_GEN_ME_PS_out.root maxEvents=1 \
#          pdfFile=GEN/genJetAnalyzer_ME_PS_out.pdf sampleLabel="sample ME_PS|ISR on, FSR on|MPI off, Had off"
import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing('analysis')
options.inputFiles = 'file:output_step1_GEN.root'
options.outputFile = 'genJetAnalyzer.root'
options.maxEvents = -1
options.register('jets', 'ak4GenJets,ak4GenJetsNoNu,ak8GenJets', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'comma-separated jet collections, one page per collection and event')
options.register('maxStage', 5, VarParsing.multiplicity.singleton, VarParsing.varType.int,
                 'generator-record stages drawn by GenPartAnalyzer before the jet pages of every event (0: none)')
options.register('etaMax', 6.0, VarParsing.multiplicity.singleton, VarParsing.varType.float, 'eta axis range of the plots')
options.register('labelPtMin', 5.0, VarParsing.multiplicity.singleton, VarParsing.varType.float,
                 'jet pT above which the pT value is written next to the jet axis (b, bbar jets always)')
options.register('printConstituents', False, VarParsing.multiplicity.singleton, VarParsing.varType.bool,
                 'print the constituents (genParticles indices) of every jet to stdout')
options.register('pdfFile', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 'if set, all plots go as pages into this single PDF instead of one PDF per collection and event')
options.register('sampleLabel', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "text printed in the right panel of every plot, '|' separates lines, e.g. the generator stage")
options._tags, options._tagOrder = {}, []  # keep outputFile as given (VarParsing would append _numEvent<N> when maxEvents > 0)
options.parseArguments()

process = cms.Process('GENJETANA')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.cerr.FwkReport.reportEvery = 1

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(options.maxEvents))
process.source = cms.Source('PoolSource', fileNames=cms.untracked.vstring(options.inputFiles))

process.TFileService = cms.Service('TFileService', fileName=cms.string(options.outputFile))

prefix = options.outputFile.replace('.root', '')
if options.maxStage > 0:
    process.genPartAnalyzer = cms.EDAnalyzer('GenPartAnalyzer',
        genParticles=cms.InputTag('genParticles', '', 'GEN'),
        outputPrefix=cms.string(prefix),
        etaMax=cms.double(options.etaMax),
        labelPtMin=cms.double(options.labelPtMin),
        savePDF=cms.bool(True),
        printParticles=cms.bool(False),
        maxStage=cms.int32(options.maxStage),
        pdfFile=cms.string(options.pdfFile),
        sampleLabel=cms.string(options.sampleLabel),
        openPdf=cms.bool(True),    # opens the shared multi-page PDF (first module to print)
        closePdf=cms.bool(False),  # GenJetAnalyzer closes it
    )

process.genJetAnalyzer = cms.EDAnalyzer('GenJetAnalyzer',
    genParticles=cms.InputTag('genParticles', '', 'GEN'),
    genJets=cms.VInputTag(*[cms.InputTag(j.strip()) for j in options.jets.split(',') if j.strip()]),
    outputPrefix=cms.string(prefix),
    etaMax=cms.double(options.etaMax),
    labelPtMin=cms.double(options.labelPtMin),
    savePDF=cms.bool(True),
    printConstituents=cms.bool(options.printConstituents),
    pdfFile=cms.string(options.pdfFile),
    sampleLabel=cms.string(options.sampleLabel),
    openPdf=cms.bool(options.maxStage <= 0),
    closePdf=cms.bool(True),
)

process.p = cms.Path(process.genJetAnalyzer)
if options.maxStage > 0:
    # the framework orders the modules of a Path only across EDFilters (modules between two
    # filters may run in any order), so an always-true filter keeps the generator-stage pages
    # of every event in front of the jet pages
    process.genPartPagesFirst = cms.EDFilter('HLTBool', result=cms.bool(True))
    process.p = cms.Path(process.genPartAnalyzer + process.genPartPagesFirst + process.genJetAnalyzer)
