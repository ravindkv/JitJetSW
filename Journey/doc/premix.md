
https://chatgpt.com/s/p_6af43da4c7bc81919a60b04e8482a2b0

The main difference is when the pileup events are combined and digitized. In classical mixing, CMS starts from individual pileup SimHits for every signal event. In premixing, CMS prepares a reusable library of pileup detector responses in advance.


## 1. Classical mixing vs. premixing

A. Classical mixing — SimHit-level overlay

Signal SimHits

1 hard collision

PU SimHits

N minimum-bias events

Mix SimHits

Digitization of combined hits

Final mixed DIGI → RECO

B. Premixing — detector-response-level overlay

Signal SimHits

Signal digitization

N PU events

Premix stage 1

Saved PU digi library

Premix stage 2

Combine detector signals with subdetector-specific processing

Final mixed DIGI → RECO

The key idea is that digitizing many minimum-bias events is expensive. Premixing does that work once for a library of pileup configurations, which can then be reused for many signal samples.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://cms-pdmv.gitbook.io\&sz=32)

Monte Carlo production tools+2



## 2. Practical differences

| Feature             | Classical mixing                | Premixing                                            |
| ------------------- | ------------------------------- | ---------------------------------------------------- |
| PU input            | Individual minimum-bias events  | Precombined PU event                                 |
| PU digitization     | Repeated for each signal event  | Performed in advance                                 |
| Final overlay       | SimHit level                    | Detector-response level                              |
| CPU and I/O         | Higher                          | Much lower                                           |
| Reusability         | Individual minimum-bias samples | Entire premixed PU library                           |
| Physics performance | Reference approach              | Designed and validated to reproduce classical mixing |

A 2022 CMS computing report found that premixing required less than half the CPU time and nearly two orders of magnitude less I/O under the conditions studied.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://cds.cern.ch\&sz=32)

CERN Document Server+1



## 3. Is premixing exactly equivalent physically?

This is the interesting part!

Consider an ECAL crystal with two contributions:

\\[ E\_{\mathrm{signal}}=8\ \mathrm{GeV},\qquad E\_{\mathrm{PU}}=2\ \mathrm{GeV} \\]

Classical mixing combines the energy deposits and their timing before simulating the electronics:

\\[ D\_{\mathrm{classic}}=f(H\_{\mathrm{signal}}+H\_{\mathrm{PU}}) \\]

Premixing processes the pileup detector response in advance and combines it with the separately simulated signal response:

\\[ D\_{\mathrm{premix}} =M\bigl(f\_{\mathrm{signal}}(H\_{\mathrm{signal}}), f\_{\mathrm{PU}}(H\_{\mathrm{PU}})\bigr) \\]

Here, \\(H\\) represents simulated hits, \\(f\\) represents digitization, and \\(M\\) represents the subdetector-specific combination procedure.

In general,

\\[ f(A+B)\ne f(A)+f(B) \\]

because of nonlinear electronics, saturation, thresholds, noise, and zero suppression.

Consequently, premixing is not simply adding two finished ADC values. CMS uses special processing to preserve low-level signals and reproduce the combined detector response accurately.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://twiki.cern.ch\&sz=32)

twiki.cern.ch+1



## 4. Does premixing change signal–pileup separation?

Yes, at the intermediate processing stage:

- Classical: The individual PU SimHits are available to the mixing process alongside signal SimHits.
- Premixing: The PU contributions have already been combined into one detector-response representation. Their individual SimHits are not normally needed during stage 2.
- Both: The final detector digis contain the combined signal and pileup response, with MC truth associations potentially retained separately.

Takeaway: Classical mixing is mix first, digitize once. Premixing is digitize and aggregate pileup in advance, then combine it with the signal response. The big advantage is computing efficiency, not a different intended physics result.

The official CMS documentation is particularly useful here: [CMS Simulation Guide — Premixing](https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideSimulation?utm_source=chatgpt.com).
