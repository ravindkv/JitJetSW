# Per-event text dumps for the whole GEN -> NANO chain.
#
# Usage (from cmsDriver, appended to the config through --customise_commands):
#   from ChainDump.Dumper.chainDump_cff import customiseChainDump
#   process = customiseChainDump(process, level=1, maxElements=-1, maxSub=-1)
#
# The step is detected from the process name (GEN, SIM, DIGI, L1, DIGI2RAW, HLT, RECO, PAT,
# NANO). Every dump module prints to stdout, so the cmsRun log holds the numbers.
#
# level 1: every physics object of the step (gen particles, sim tracks/hits, digis, L1 objects,
#          trigger results, tracks, PF candidates, jets, MET, MiniAOD objects, NanoAOD tables)
#          with the links back to the previous step (barcodes, SimTrack ids, TrackingParticles,
#          PF keys, Delta-R gen matches, JEC levels, ...).
# level 2: level 1 + Pythia8 event listings and HepMC dump (GEN), the full HepMC particle list,
#          every L1 algorithm bit (fired or not).
# level 3: level 2 + Geant4 track/step printout for tracks above g4Threshold GeV (SIM, very
#          large) and the list of every product present in the event (all steps).
#
# maxElements caps the number of elements printed per collection (-1 = all).
# maxSub caps the number of sub-elements (jet constituents, hits per collection, digis per
# detector unit, cluster rechits, ...) (-1 = all).
import FWCore.ParameterSet.Config as cms


def _vtags(*names):
    return cms.VInputTag(*names)


def customiseChainDump(process, level=1, maxElements=-1, maxSub=-1, g4Threshold=1.0):
    step = process.name_()
    common = dict(step=cms.string(step), maxElements=cms.int32(int(maxElements)), maxSub=cms.int32(int(maxSub)))
    names = []
    task = cms.Task()

    def add(name, module):
        setattr(process, name, module)
        names.append(name)

    # Gen collections available in the input of every step (kept in all tiers down to AODSIM)
    genParticles = cms.InputTag('genParticles')
    genJets = cms.InputTag('ak4GenJetsNoNu')

    if step == 'GEN':
        add('chainDumpGen', cms.EDAnalyzer('GenDumper',
            genEventInfo=cms.InputTag('generator'),
            hepmcRaw=cms.InputTag('generator', 'unsmeared'),
            hepmcSmeared=cms.InputTag('generatorSmeared'),
            genParticles=genParticles,
            genBarcodes=cms.InputTag('genParticles'),
            genJets=_vtags('ak4GenJets', 'ak4GenJetsNoNu', 'ak8GenJets', 'ak8GenJetsNoNu'),
            genMETs=_vtags('genMetTrue', 'genMetCalo'),
            printHepMCParticles=cms.bool(level >= 2),
            **common))
        # decay-tree view of the same particles (prints with std::cout)
        add('chainDumpParticleList', cms.EDAnalyzer('ParticleListDrawer',
            src=genParticles,
            maxEventsToPrint=cms.untracked.int32(-1),
            printVertex=cms.untracked.bool(True),
            printFlags=cms.untracked.bool(True),
            printOnlyHardInteraction=cms.untracked.bool(False),
            useMessageLogger=cms.untracked.bool(False)))
        if level >= 2 and hasattr(process, 'generator'):
            n = process.maxEvents.input.value()
            process.generator.maxEventsToPrint = cms.untracked.int32(n if n > 0 else 1000000)
            process.generator.pythiaPylistVerbosity = cms.untracked.int32(1)   # Pythia8 info.list() + event.list()
            process.generator.pythiaHepMCVerbosity = cms.untracked.bool(True)  # HepMC GenEvent::print()

    elif step == 'SIM':
        tracker = ['TrackerHitsPixelBarrelLowTof', 'TrackerHitsPixelBarrelHighTof', 'TrackerHitsPixelEndcapLowTof',
                   'TrackerHitsPixelEndcapHighTof', 'TrackerHitsTIBLowTof', 'TrackerHitsTIBHighTof', 'TrackerHitsTIDLowTof',
                   'TrackerHitsTIDHighTof', 'TrackerHitsTOBLowTof', 'TrackerHitsTOBHighTof', 'TrackerHitsTECLowTof',
                   'TrackerHitsTECHighTof', 'MuonDTHits', 'MuonCSCHits', 'MuonRPCHits', 'MuonGEMHits']
        calo = ['EcalHitsEB', 'EcalHitsEE', 'EcalHitsES', 'HcalHits']
        add('chainDumpSim', cms.EDAnalyzer('SimDumper',
            simTracks=cms.InputTag('g4SimHits'),
            simVertices=cms.InputTag('g4SimHits'),
            genParticles=genParticles,
            genBarcodes=cms.InputTag('genParticles'),
            trackerHits=_vtags(*['g4SimHits:' + x for x in tracker]),
            caloHits=_vtags(*['g4SimHits:' + x for x in calo]),
            **common))
        if level >= 3 and hasattr(process, 'g4SimHits'):
            # Geant4 track/step printout for every track with Ekin >= g4Threshold GeV in every event
            process.g4SimHits.SteppingVerbosity = cms.untracked.int32(3)
            process.g4SimHits.StepVerboseThreshold = cms.untracked.double(float(g4Threshold))
            process.g4SimHits.VerboseEvents = cms.untracked.vint32()
            process.g4SimHits.VerboseTracks = cms.untracked.vint32()

    elif step == 'DIGI':
        add('chainDumpDigi', cms.EDAnalyzer('DigiDumper',
            pileupSummary=_vtags('addPileupInfo'),
            trackingParticles=_vtags('mix:MergedTrackTruth'),
            trackingVertices=_vtags('mix:MergedTrackTruth'),
            pixelDigis=_vtags('simSiPixelDigis'),
            pixelDigiSimLinks=_vtags('simSiPixelDigis'),
            stripDigis=_vtags('simSiStripDigis:ZeroSuppressed'),
            stripDigiSimLinks=_vtags('simSiStripDigis'),
            ebDigis=_vtags('simEcalDigis:ebDigis'),
            eeDigis=_vtags('simEcalDigis:eeDigis'),
            esDigis=_vtags('simEcalPreshowerDigis'),
            hcalQIE11Digis=_vtags('simHcalDigis:HBHEQIE11DigiCollection'),
            hcalQIE10Digis=_vtags('simHcalDigis:HFQIE10DigiCollection'),
            hoDigis=_vtags('simHcalDigis'),
            ecalTrigPrims=_vtags('simEcalTriggerPrimitiveDigis'),
            hcalTrigPrims=_vtags('simHcalTriggerPrimitiveDigis'),
            dtDigis=_vtags('simMuonDTDigis'),
            cscWireDigis=_vtags('simMuonCSCDigis:MuonCSCWireDigi'),
            cscStripDigis=_vtags('simMuonCSCDigis:MuonCSCStripDigi'),
            cscComparatorDigis=_vtags('simMuonCSCDigis:MuonCSCComparatorDigi'),
            rpcDigis=_vtags('simMuonRPCDigis'),
            gemDigis=_vtags('simMuonGEMDigis'),
            **common))

    elif step == 'L1':
        add('chainDumpL1', cms.EDAnalyzer('L1Dumper',
            genParticles=genParticles, genJets=genJets,
            jets=_vtags('simCaloStage2Digis', 'simCaloStage2Digis:MP'),
            egammas=_vtags('simCaloStage2Digis', 'simCaloStage2Digis:MP'),
            taus=_vtags('simCaloStage2Digis', 'simCaloStage2Digis:MP'),
            muons=_vtags('simGmtStage2Digis'),
            etSums=_vtags('simCaloStage2Digis', 'simCaloStage2Digis:MP'),
            caloTowers=_vtags('simCaloStage2Layer1Digis', 'simCaloStage2Digis:MP'),
            caloClusters=_vtags('simCaloStage2Digis:MP'),
            algBlk=cms.InputTag('simGtStage2Digis'),
            printAllAlgos=cms.bool(level >= 2),
            **common))
        add('chainDumpL1Summary', cms.EDAnalyzer('L1TGlobalSummary',
            AlgInputTag=cms.InputTag('simGtStage2Digis'),
            ExtInputTag=cms.InputTag('simGtExtFakeStage2Digis'),
            MinBx=cms.int32(0), MaxBx=cms.int32(0),
            DumpTrigResults=cms.bool(True), DumpRecord=cms.bool(False), DumpTrigSummary=cms.bool(True),
            ReadPrescalesFromFile=cms.bool(False), psFileName=cms.string('prescale_L1TGlobal.csv'), psColumn=cms.int32(0)))

    elif step == 'DIGI2RAW':
        add('chainDumpRaw', cms.EDAnalyzer('DigiDumper', fedRawData=_vtags('rawDataCollector'), **common))

    elif step == 'HLT':
        add('chainDumpHLTL1', cms.EDAnalyzer('L1Dumper',
            genParticles=genParticles, genJets=genJets,
            jets=_vtags('hltGtStage2Digis:Jet'),
            egammas=_vtags('hltGtStage2Digis:EGamma'),
            taus=_vtags('hltGtStage2Digis:Tau'),
            muons=_vtags('hltGtStage2Digis:Muon'),
            etSums=_vtags('hltGtStage2Digis:EtSum'),
            algBlk=cms.InputTag('hltGtStage2Digis'),
            printAllAlgos=cms.bool(level >= 2),
            **common))
        add('chainDumpHLTLocalReco', cms.EDAnalyzer('LocalRecoDumper',
            pixelClusters=_vtags('hltSiPixelClusters'),
            stripClusters=_vtags('hltSiStripRawToClustersFacility'),
            ecalRecHits=_vtags('hltEcalRecHit:EcalRecHitsEB', 'hltEcalRecHit:EcalRecHitsEE'),
            hbheRecHits=_vtags('hltHbhereco'),
            hfRecHits=_vtags('hltHfreco'),
            hoRecHits=_vtags('hltHoreco'),
            pfRecHits=_vtags('hltParticleFlowRecHitECALUnseeded', 'hltParticleFlowRecHitHBHE', 'hltParticleFlowRecHitHF'),
            pfClusters=_vtags('hltParticleFlowClusterECALUnseeded', 'hltParticleFlowClusterHBHE', 'hltParticleFlowClusterHCAL', 'hltParticleFlowClusterHF'),
            superClusters=_vtags('hltParticleFlowSuperClusterECALUnseeded:hltParticleFlowSuperClusterECALBarrel',
                                 'hltParticleFlowSuperClusterECALUnseeded:hltParticleFlowSuperClusterECALEndcapWithPreshower'),
            caloTowers=_vtags('hltTowerMakerForAll'),
            **common))
        add('chainDumpHLTReco', cms.EDAnalyzer('RecoDumper',
            genParticles=genParticles, genJets=genJets,
            vertices=_vtags('hltPixelVertices', 'hltTrimmedPixelVertices', 'hltVerticesPF'),
            tracks=_vtags('hltPixelTracks', 'hltIter0PFlowTrackSelectionHighPurity', 'hltMergedTracks', 'hltPFMuonMerging'),
            pfCandidates=_vtags('hltParticleFlow'),
            jets=_vtags('hltAK4CaloJets', 'hltAK4CaloJetsCorrected', 'hltAK4PFJets', 'hltAK4PFJetsCorrected',
                        'hltAK4PFJetsTightIDCorrected', 'hltAK8PFJets', 'hltAK8PFJetsCorrected'),
            mets=_vtags('hltPFMETProducer'),
            candidates=_vtags('hltIterL3Muons', 'hltEgammaCandidates'),
            doubles=_vtags('hltAK4PFJets:rho', 'hltAK4CaloJets:rho'),
            **common))
        add('chainDumpHLTTrigger', cms.EDAnalyzer('TriggerDumper',
            genParticles=genParticles, genJets=genJets,
            triggerResults=cms.InputTag('TriggerResults'),
            triggerSummary=cms.InputTag('hltTriggerSummaryAOD'),
            onlyAcceptedPaths=cms.bool(False),
            **common))

    elif step == 'RECO':
        # true track <-> TrackingParticle association (hit based), to follow every track back to its genParticle
        process.load('SimTracker.TrackerHitAssociation.tpClusterProducer_cfi')
        process.load('SimTracker.TrackAssociatorProducers.quickTrackAssociatorByHits_cfi')
        process.load('SimTracker.TrackAssociation.trackingParticleRecoTrackAsssociation_cfi')
        task.add(process.tpClusterProducer, process.quickTrackAssociatorByHits, process.trackingParticleRecoTrackAsssociation)
        add('chainDumpLocalReco', cms.EDAnalyzer('LocalRecoDumper',
            pixelClusters=_vtags('siPixelClusters'),
            stripClusters=_vtags('siStripClusters'),
            ecalRecHits=_vtags('ecalRecHit:EcalRecHitsEB', 'ecalRecHit:EcalRecHitsEE'),
            hbheRecHits=_vtags('hbhereco'),
            hfRecHits=_vtags('hfreco'),
            hoRecHits=_vtags('horeco'),
            pfRecHits=_vtags('particleFlowRecHitECAL', 'particleFlowRecHitHBHE', 'particleFlowRecHitHF', 'particleFlowRecHitHO', 'particleFlowRecHitPS'),
            pfClusters=_vtags('particleFlowClusterECAL', 'particleFlowClusterHBHE', 'particleFlowClusterHCAL', 'particleFlowClusterHF',
                              'particleFlowClusterHO', 'particleFlowClusterPS'),
            superClusters=_vtags('particleFlowSuperClusterECAL:particleFlowSuperClusterECALBarrel',
                                 'particleFlowSuperClusterECAL:particleFlowSuperClusterECALEndcapWithPreshower'),
            caloTowers=_vtags('towerMaker'),
            **common))
        add('chainDumpReco', cms.EDAnalyzer('RecoDumper',
            genParticles=genParticles, genJets=genJets,
            vertices=_vtags('offlinePrimaryVertices', 'offlinePrimaryVerticesWithBS', 'inclusiveSecondaryVertices'),
            tracks=_vtags('generalTracks', 'globalMuons', 'standAloneMuons', 'electronGsfTracks'),
            recoToSim=cms.InputTag('trackingParticleRecoTrackAsssociation'),
            recoToSimTrackLabel=cms.string('generalTracks'),
            pfCandidates=_vtags('particleFlow'),
            jets=_vtags('ak4PFJets', 'ak4PFJetsCHS', 'ak4PFJetsPuppi', 'ak8PFJetsPuppi', 'ak8PFJetsPuppiSoftDrop', 'ak4CaloJets'),
            mets=_vtags('pfMet', 'pfMetPuppi', 'pfChMet', 'caloMet'),
            candidates=_vtags('muons', 'gedGsfElectrons', 'gedPhotons', 'hpsPFTauProducer'),
            doubles=_vtags('fixedGridRhoFastjetAll', 'fixedGridRhoFastjetCentral', 'fixedGridRhoFastjetCentralChargedPileUp',
                           'fixedGridRhoFastjetCentralNeutral', 'fixedGridRhoFastjetAllCalo'),
            **common))

    elif step == 'PAT':
        add('chainDumpPat', cms.EDAnalyzer('PatDumper',
            genParticles=genParticles, genJets=genJets,
            prunedGenParticles=cms.InputTag('prunedGenParticles'),
            packedGenParticles=cms.InputTag('packedGenParticles'),
            slimmedGenJets=_vtags('slimmedGenJets', 'slimmedGenJetsAK8'),
            vertices=_vtags('offlineSlimmedPrimaryVertices'),
            packedCandidates=_vtags('packedPFCandidates', 'lostTracks'),
            packedToPF=cms.InputTag('packedPFCandidates'),
            jets=_vtags('slimmedJets', 'slimmedJetsPuppi', 'slimmedJetsAK8'),
            mets=_vtags('slimmedMETs', 'slimmedMETsPuppi'),
            candidates=_vtags('slimmedMuons', 'slimmedElectrons', 'slimmedPhotons', 'slimmedTaus', 'slimmedSecondaryVertices'),
            triggerObjects=cms.InputTag('slimmedPatTrigger'),
            triggerResults=cms.InputTag('TriggerResults', '', 'HLT'),
            doubles=_vtags('fixedGridRhoFastjetAll', 'fixedGridRhoFastjetCentralNeutral', 'fixedGridRhoFastjetCentralChargedPileUp',
                           'fixedGridRhoFastjetCentralCalo'),
            **common))

    elif step == 'NANO':
        add('chainDumpNano', cms.EDAnalyzer('FlatTableDumper', **common))

    else:
        print('customiseChainDump: unknown process name %s, nothing added' % step)
        return process

    if level >= 3:
        # list of every product present in the event (type, module label, instance, process).
        # The reflection dump of the product contents (verbose=True) segfaults in CMSSW_15_0_5
        # (checked on DIGI2RAW and SIM), so it is not enabled; the ChainDump analyzers above
        # print the contents instead.
        add('chainDumpEventContent', cms.EDAnalyzer('EventContentAnalyzer',
            verbose=cms.untracked.bool(False),
            getData=cms.untracked.bool(True),
            listContent=cms.untracked.bool(True),
            listProvenance=cms.untracked.bool(False)))

    seq = None
    for n in names:
        seq = getattr(process, n) if seq is None else seq + getattr(process, n)
    process.chainDumpSequence = cms.Sequence(seq)
    process.chainDumpPath = cms.EndPath(process.chainDumpSequence, task)
    if hasattr(process, 'schedule') and process.schedule is not None:
        process.schedule.append(process.chainDumpPath)

    # make the MessageLogger based summaries of the borrowed modules visible
    if hasattr(process, 'MessageLogger'):
        for cat in ('L1TGlobalSummary', 'EventContent', 'G4cout', 'G4cerr', 'SimG4CoreApplication'):
            setattr(process.MessageLogger.cerr, cat, cms.untracked.PSet(limit=cms.untracked.int32(-1)))
    print('customiseChainDump: step=%s level=%d modules=%s' % (step, level, ','.join(names)))
    return process
