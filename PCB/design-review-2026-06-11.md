# AmidalaShield Design Review — 2026-06-11

**Board:** AmidalaShield v1.1  
**Size:** 100.05 × 75.05mm (4-layer)  
**Stackup:** F.Cu (signal) / In1.Cu (GND) / In2.Cu (GND) / B.Cu (+3.3V + +5V zones)  
**Verdict: CLEAR TO FAB — no open blockers**

---

## Analyses Run

| Analyzer | Status |
|----------|--------|
| Schematic (`analyze_schematic.py`) | ✓ Complete |
| PCB (`analyze_pcb.py --full`) | ✓ Complete |
| Gerbers (`analyze_gerbers.py`) | ✓ Complete — fresh export 2026-06-11 |
| Cross-analysis (`cross_analysis.py`) | ✓ Complete |
| Thermal (`analyze_thermal.py`) | ✓ Complete — score 100/100, 0 findings |
| SPICE | Skipped — no simulator installed |
| Datasheet sync | Skipped — MPNs not required (see below) |

---

## Board Summary

- **Components:** 32 (26 unique BOM lines)
- **Nets:** 55
- **Footprints on PCB:** 37 (includes REF** mounting/logo and G*** Amidala logo)
- **Vias:** 359 total (333 GND stitching)
- **Total holes:** 506
- **Layers:** 4
- **Fab tier:** ≤100×100mm standard pricing

---

## BOM

| Value | Qty | Footprint | Refs |
|-------|-----|-----------|------|
| 100n | 3 | C_0603_1608Metric | C1, C2, C3 |
| 1µ | 1 | C_0603_1608Metric | C5 |
| 8.2p | 1 | C_0603_1608Metric | C6 |
| 10µ | 2 | C_0805_2012Metric | C4, C7 |
| 100µF | 1 | CP_EIA-3216-18_Kemet-A (tantalum) | C9 |
| 10k | 4 | R_0603_1608Metric | R1, R2, R3, R5 |
| ESP32-S3-DevKitC | 1 | Custom (verified) | ESP32-S3 |
| XB3-24Z8UT-J | 1 | XB324Z8UTJ | XBee3 |
| Micro SD | 1 | Hirose DM3AT-SF-PEJM5 | J3 |
| Screw terminal 2-pos | 1 | Phoenix MKDS-1.5-2 5.08mm | J1 |
| Headers (various) | 11 | PinHeader 2.54mm | I2C1, SPI1, Serial0/1, SWSerial1, J_Servo1–4, J_Digital1–4, J_Analog1–2, J_PPMIN1 |

---

## Findings

### Blockers
**None.**

---

### Warnings (all accepted / known false positives)

| ID | Finding | Disposition |
|----|---------|-------------|
| RS-001 | +3.3V and +5V have no declared source | **False positive** — PWR_FLAGs correctly stacked: #FLG04 on +5V, #FLG05 on GND, #FLG01 on +3.3V |
| SS-001 | 1/26 BOM lines have MPNs | **Accepted** — fab picks basic passives; remaining parts are headers/connectors |
| DS-002 | No datasheets directory | **Accepted** — MPNs not required for this build |
| PS-002 | GND plane: 334 islands | **False positive** — 333 GND stitching vias counted individually by island detector |
| PS-002 | +3.3V plane: 3 islands | **False positive** — 3 intentional B.Cu pour regions, verified correct in KiCad |
| RP-002 | 8 SPI signals crossing GND plane gap | **Structural / accepted** — no GND zone on outer layers by design; return path through inner planes; acceptable at <10MHz SPI |
| PM-001 | Courtyard overlaps: C1/C2/C5/C6/C7 ↔ XBee3 | **Accepted** — XBee3 is a through-hole footprint; overlaps suppressed in `.kicad_dru` |
| GR-002 | Copper/edge layer alignment variance (6.0mm W, 3.9mm H) | **Expected** — copper clearance from board edge |
| GR-004 | F.Paste: 76 flashes vs 1096 copper (7%) | **Expected** — THT-dominant design |
| TE-001 | 0/60 nets have test points | **Accepted** — deferred to future revision |
| VS-002 | Via stitching sparse on 50% of board | **Accepted** — 333 stitching vias present; sparse areas are connectors/headers |

---

### Info (no action)

| ID | Finding | Note |
|----|---------|------|
| CERT-001 | Wireless module detected: ESP32-S3 | Expected — core MCU |
| CP-002 | No opposite-layer copper under THT/header parts | Expected — inner plane design |
| FD-001 | No fiducials on F.Cu | Acceptable — coarsest SMD pitch is 0.70mm, no fine-pitch parts |
| XV-001 | REF** and G*** in PCB but not schematic | Expected — mounting holes and Amidala logo graphic |
| B.SilkS overrun | Logo extends past board edge | **Accepted** — cosmetic; fab will trim |
| RC-DET | R1/C1 RC filter at 159.15Hz | Correct — XBee3 power-on reset delay |
| TS-DET | 333 GND stitching vias, 1 +3.3V via | Correct — matches intent |
| EP-AUD | No ESD protection on headers/connectors | Accepted — internal RC robot controller |

---

## Thermal

Score: **100/100** — 0 findings. No thermal concerns at expected operating loads.

---

## Gerbers

Regenerated 2026-06-11. Files in `gerbers/AmidalaShield-gerbers.zip`.

| Layer | File |
|-------|------|
| F.Cu | AmidalaShield-F_Cu.gtl |
| In1.Cu (GND) | AmidalaShield-In1_Cu.g1 |
| In2.Cu (GND) | AmidalaShield-In2_Cu.g2 |
| B.Cu (+3.3V/+5V) | AmidalaShield-B_Cu.gbl |
| F.Mask | AmidalaShield-F_Mask.gts |
| B.Mask | AmidalaShield-B_Mask.gbs |
| F.Silkscreen | AmidalaShield-F_Silkscreen.gto |
| B.Silkscreen | AmidalaShield-B_Silkscreen.gbo |
| F.Paste | AmidalaShield-F_Paste.gtp |
| B.Paste | AmidalaShield-B_Paste.gbp |
| Edge.Cuts | AmidalaShield-Edge_Cuts.gm1 |
| PTH drill | AmidalaShield-PTH-drl.gbr |
| NPTH drill | AmidalaShield-NPTH-drl.gbr |

---

## Design Decisions on Record

- **R4 removed:** SPI CLK pull-up (GPIO12) removed in v1.1. CPOL=1 constraint lifted; firmware can select any SPI mode. GPIO12 floats during boot before SPI peripheral initializes — bring-up note for XBee3.
- **C9 added:** 100µF tantalum on +5V for servo transient suppression.
- **Zone stack:** GND on In1+In2, power (+3.3V/+5V) on B.Cu, signal only on F.Cu. All SMD on F.Cu for single-side reflow.
- **ESP32-S3 footprint:** Custom footprint verified by designer — do not flag.
- **XBee3 courtyard:** THT footprint; courtyard warnings suppressed in `.kicad_dru` and should always be ignored.
