# Pythia8 inclusive b-bbar production (HardQCD gg->bbbar and qqbar->bbbar), tune CP5.
# Modelled on Configuration/Generator/python/TTbar_14TeV_TuneCP5_cfi.py; the com
# energy is overridden by the driver script (13.6 TeV for 2024 conditions).
# A pTHat cut is applied so that the b quarks produce jets reconstructable
# for JME studies; lower it (or remove it) for a more inclusive sample.
import FWCore.ParameterSet.Config as cms
from Configuration.Generator.Pythia8CommonSettings_cfi import *
from Configuration.Generator.MCTunes2017.PythiaCP5Settings_cfi import *

generator = cms.EDFilter("Pythia8ConcurrentGeneratorFilter",
                         pythiaHepMCVerbosity = cms.untracked.bool(False),
                         maxEventsToPrint = cms.untracked.int32(0),
                         pythiaPylistVerbosity = cms.untracked.int32(0),
                         filterEfficiency = cms.untracked.double(1.0),
                         comEnergy = cms.double(14000.0),
                         PythiaParameters = cms.PSet(
        pythia8CommonSettingsBlock,
        pythia8CP5SettingsBlock,
        processParameters = cms.vstring(
            'HardQCD:gg2bbbar = on ',
            'HardQCD:qqbar2bbbar = on ',
            'PhaseSpace:pTHatMin = 30. ',
            ),
        parameterSets = cms.vstring('pythia8CommonSettings',
                                    'pythia8CP5Settings',
                                    'processParameters',
                                    )
        )
                         )
ProductionFilterSequence = cms.Sequence(generator)
