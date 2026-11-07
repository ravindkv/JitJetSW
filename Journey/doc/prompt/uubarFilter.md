Pythia 8 has no such switch. HardQCD:qqbar2bbbar sums over all light q qbar initial states, there is no uubar2bbbar, and quark beams are not allowed for Beams:idA/idB. The only ways to get pure u ubar -> b bbar are to generate q qbar -> b bbar and veto the other flavours on the event record, or to produce the process in MadGraph (generate u u~ > b b~) and hadronise the LHE file with Pythia. I implemented the first, since it needs no external generator.

What I changed

- New plugin Journey/plugins/HardProcessIncomingFilter.cc: an EDFilter reading the HepMC product from the generator. It collects the particles with Pythia status 21, the two hard-process incoming partons, and keeps the event only if their pdg ids match the configured list in any order. Parameters are src, incomingIds, absoluteIds and status.
- Journey/test/step1_GEN_cfg.py: the process line is now HardQCD:qqbar2bbbar = on, and a uubarFilter module with ids 2 and -2 runs right after the generator in the production filter sequence. Rejected events never reach the output file,ts the efficiency.
- Rebuilt the package and refreshed the plugin cache, which was stale and initially gave a
  PluginNotFound error.

Test result (30 events in the scratch

┌──────────────────┬────────────────┬
│ events generated │ kept by filter │ efficiency │
├──────────────────┼────────────────┼
│ 30               │ 11             │ 0.37       │
└──────────────────┴────────────────┴

Reading the output file back, all 11 rtons u and ubar and status-23 partons b and bbar. Status 21 is preserved through HepMC and genParticles, so the selection works on the actual
hard process, independent of ISR.

Two notes. The status-21 partons are the ones entering the hard scattering, so events where a u comes from an ISR splitting of a gluon are still counted as u ubar, which is the standard definition of the subprocess. Roughly two thirds of the q qbar events are discarded, so for a fixed number of output events raise maxEvents by about 3x, or set filterEfficiency on the generator to about 0.37 if you want the bookkeeping to reflect it.
