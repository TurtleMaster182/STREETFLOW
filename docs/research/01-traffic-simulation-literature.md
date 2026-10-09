# 01 — Traffic Simulation: Literature & Engineering Study

**Purpose.** This study looks at the scientific and engineering literature we need to build an accurate traffic simulator. The simulator is a testbed for a traffic management system: adaptive signal control, routing and congestion management. A separate study covers the real infrastructure of Iași, Romania. This document covers models, methods, tools, calibration and evaluation.

**Conventions.** Every non-trivial claim links to a source, and full references are in [References](#references). Equations use plain LaTeX-style notation. Symbols: $v$ = speed, $s$ = gap (bumper to bumper), $\Delta v = v - v_{\text{leader}}$ = approach rate, $\rho$ (or $k$) = density, $q$ = flow.

**Date of survey:** October 2026.

---

## Table of Contents

1. [Simulation scales: micro, meso, macro, hybrid](#1-simulation-scales-micro-meso-macro-hybrid)
2. [Car-following models](#2-car-following-models)
3. [Lane-changing models](#3-lane-changing-models)
4. [Intersection modeling](#4-intersection-modeling)
5. [Macroscopic and mesoscopic flow models](#5-macroscopic-and-mesoscopic-flow-models)
6. [Demand modeling and traffic assignment](#6-demand-modeling-and-traffic-assignment)
7. [Traffic signal control strategies to be tested](#7-traffic-signal-control-strategies-to-be-tested)
8. [Existing simulators and frameworks](#8-existing-simulators-and-frameworks)
9. [Calibration and validation](#9-calibration-and-validation)
10. [Evaluation metrics for traffic management](#10-evaluation-metrics-for-traffic-management)
11. [Recommendations for our simulator](#11-recommendations-for-our-simulator)
12. [References](#references)

---

## 1. Simulation scales: micro, meso, macro, hybrid

Traffic models are usually grouped by *granularity*, meaning how finely they represent vehicles and their interactions ([Barceló (ed.), *Fundamentals of Traffic Simulation*, 2010](https://www.springer.com/us/book/9781441961419); [Treiber & Kesting, *Traffic Flow Dynamics*, 2013](https://www.springer.com/gb/book/9783642324598)).

| Scale | State variables | Typical models | Speed / size | Strengths | Weaknesses |
|---|---|---|---|---|---|
| **Microscopic** | Each vehicle: position, speed, lane | Car-following (IDM, Krauss, Gipps, Wiedemann) + lane-changing (MOBIL, LC2013) + junction/gap acceptance | Slowest; ~10³–10⁵ vehicles in real time on one core | Represents queues per lane, signal phases, detectors, spillback, turning conflicts, emissions per vehicle | Many parameters to calibrate; stochastic, so it needs replications; sensitive to network coding errors |
| **Mesoscopic** | Individual vehicles (or packets) moved by aggregate link/segment dynamics (queues, speed–density) | Queue models, SUMO MESO ([Eissfeldt-based](https://sumo.dlr.de/docs/Simulation/Meso.html)), MATSim QSim ([Horni et al. 2016](http://dx.doi.org/10.5334/baw)), Aimsun Meso, Link Transmission Model | SUMO says MESO runs "up to 100 times faster" than micro ([SUMO MESO docs](https://sumo.dlr.de/docs/Simulation/Meso.html)) | City- or region-scale DTA, route choice, many scenario runs | No lane-level behaviour; intersections simplified; signal effects approximated |
| **Macroscopic** | Continuum density/flow/speed per cell or link | LWR, CTM, METANET-type second-order models | Very fast; good for optimization and MPC | Analytic insight, fits model-predictive control | No individual vehicles; weak at urban intersections; no per-vehicle emissions or routes |
| **Hybrid** | Micro in a focus area, meso/macro elsewhere | Aimsun hybrid meso–micro ([Aimsun docs](https://docs.aimsun.com/next/24.0.0/UsersManual/HybridSimulator.html)) | Between the two | Detail where control acts, scale elsewhere | Boundary consistency (vehicle hand-over, wave propagation across the interface) is hard |

**When each scale suits testing traffic management.**

- **Signal control (fixed-time, actuated, adaptive, RL):** use **microscopic**. The controller acts on per-lane queues, detector actuations, phase transitions, yellow/all-red intervals and start-up lost time, and only micro models produce these naturally. Almost all RL-for-signals benchmarks use micro simulators (SUMO, CityFlow); see RESCO ([Ault & Sharon 2021](https://datasets-benchmarks-proceedings.neurips.cc/paper/2021/hash/f0935e4cd5920aa6c7c996a5ee53a70f-Abstract-round1.html)) and LibSignal ([Mei et al. 2023](https://arxiv.org/abs/2211.10649)).
- **City-wide routing / DTA / demand-response studies:** use **mesoscopic**, or micro with a fast assignment loop. Iterative equilibrium needs tens of full-day simulations ([SUMO DUA docs](https://sumo.dlr.de/docs/Demand/Dynamic_User_Assignment.html)).
- **Perimeter / gating / MPC controllers:** use **macroscopic** (CTM-type) as the internal *prediction model* of the controller, and evaluate it in micro.
- **Hybrid:** use when the study area is large but the control is concentrated, for example an arterial corridor inside a city.

Caveat: mesoscopic implementations differ in fidelity. A 2026 analysis argues that SUMO's MESO model "does not fully comply with" LWR principles and systematically underestimates congestion. It proposes a discrete-time Link Transmission Model instead ([Ni, Akopian, Kouvelas & Makridis 2026](https://arxiv.org/abs/2606.09282)). FHWA's Traffic Analysis Toolbox Vol. II gives a formal procedure for choosing a tool category by analysis context, facility type, management strategy and performance measures ([FHWA-HRT-04-039](https://ops.fhwa.dot.gov/trafficanalysistools/tat_vol2/index.htm)).

---

## 2. Car-following models

A car-following model gives a vehicle's longitudinal acceleration (or next-step speed) from its own speed, the gap to its leader and the leader's speed.

### 2.1 Intelligent Driver Model (IDM) — Treiber, Hennecke & Helbing (2000)

Source: [Treiber, Hennecke & Helbing, *Phys. Rev. E* 62, 1805 (2000)](https://www.doi.org/10.1103/PHYSREVE.62.1805), [arXiv:cond-mat/0002177](https://arxiv.org/abs/cond-mat/0002177).

$$
\dot v = a\left[1 - \left(\frac{v}{v_0}\right)^{\delta} - \left(\frac{s^*(v,\Delta v)}{s}\right)^2\right],\qquad
s^*(v,\Delta v) = s_0 + vT + \frac{v\,\Delta v}{2\sqrt{ab}}
$$

([equation form as given in the IDM literature](https://en.wikipedia.org/wiki/Intelligent_driver_model))

| Parameter | Meaning | Original 2000 calibration (freeway) | EIDM paper "car" | Treiber's highway table |
|---|---|---|---|---|
| $v_0$ | desired speed | 120 km/h | 120 km/h | 120 km/h |
| $T$ | desired time gap | 1.6 s | 1.5 s | 1.5 s |
| $s_0$ | jam distance | 2 m | 2.0 m | 2.0 m |
| $a$ | max acceleration | 0.73 m/s² | 1.4 m/s² | 0.3 m/s² (stated as low; realistic 0.8–2.5) |
| $b$ | comfortable deceleration | 1.67 m/s² | 2.0 m/s² | 3.0 m/s² |
| $\delta$ | free-acceleration exponent | 4 | 4 | 4 |

Sources: original values from [Treiber et al. 2000, arXiv PDF](https://arxiv.org/pdf/cond-mat/0002177), where the authors note T = 1.6 s is slightly below the 1.8 s suggested by German authorities. EIDM values from [Kesting, Treiber & Helbing, "Enhanced IDM", arXiv:0912.3613](https://arxiv.org/abs/0912.3613), Table 1 (truck: 85 km/h, T = 2.0 s, s₀ = 4 m, a = 0.7, b = 2.0). Remarks from [traffic-simulation.de IDM page](https://traffic-simulation.de/info/info_IDM.html): for city traffic one mainly lowers $v_0$; realistic $a$ is 0.8–2.5 m/s²; time gaps observed range from about 2 s down to 0.8 s and below.

**Calibration evidence.** Kesting & Treiber calibrated IDM on radar-equipped vehicle trajectories and obtained trajectory errors of about 11–29%. Reaction time had negligible influence, and IDM parameters were more robust to the choice of objective function than the Velocity Difference Model's ([Kesting & Treiber 2008, TRR 2088, arXiv:0803.4063](https://arxiv.org/abs/0803.4063)). On reconstructed NGSIM I-80 data, global calibration errors were 8.3–12.5% and platoon calibration errors 12.8–32.4% ([Kurtc & Treiber, arXiv:1610.03124](https://arxiv.org/abs/1610.03124)).

**Strengths.** IDM has few parameters, each with a clear meaning. It is collision-free in normal operation, and its continuous-time dynamics give realistic stop-and-go waves and capacity drop. The *Enhanced IDM* (constant-acceleration heuristic) removes over-reactions to cut-ins ([Kesting et al. 2010](https://arxiv.org/abs/0912.3613)). It is also widely used for automated-vehicle and ACC studies; see the 25-year review [Zhou et al. 2025, arXiv:2506.05909](https://arxiv.org/abs/2506.05909).

**Weaknesses.** IDM is deterministic, so stochasticity has to be added (e.g. SUMO's EIDM adds driver-error terms; [SUMO vType docs](https://sumo.dlr.de/docs/Definition_of_Vehicles%2C_Vehicle_Types%2C_and_Routes.html)). It can produce unrealistic decelerations when the gap shrinks abruptly (cut-ins). Numerical integration needs small steps; SUMO's IDM uses an internal `stepping` of 0.25 s ([SUMO vType docs](https://sumo.dlr.de/docs/Definition_of_Vehicles%2C_Vehicle_Types%2C_and_Routes.html)).

### 2.2 Krauss model — SUMO default

Source: [S. Krauß, PhD thesis, Univ. Cologne 1998, DLR report](https://elib.dlr.de/8380/). For SUMO's evolved implementation see [Wagner & Erdmann, "SUMO's Interpretation of the Krauß Model", SUMO Conf. Proc. 6 (2025)](https://www.tib-op.org/ojs/index.php/scp/article/view/2638).

Krauss is a "safe-speed" model. Vehicles drive as fast as possible while always being able to stop behind the leader:

$$
v_{\text{safe}} = v_l + \frac{g - v_l\,\tau}{\dfrac{v_l + v_f}{2b} + \tau},\qquad
v_{\text{des}} = \min\{v_{\max},\ v + a\,\Delta t,\ v_{\text{safe}}\},\qquad
v_{t+\Delta t} = \max\{0,\ v_{\text{des}} - \sigma\, a\, \Delta t\, \eta\},\ \eta\sim U[0,1]
$$

(form as summarized in, e.g., [SUMO Krauss source](https://sumo.dlr.de/daily/doxygen/de/d67/_m_s_c_f_model___krauss_8cpp_source.html) and the safe-speed literature; the "dawdling" term $\sigma$ is the driver-imperfection parameter.)

**SUMO defaults (passenger car):** `accel` 2.6 m/s², `decel` 4.5 m/s², `emergencyDecel` 9.0 m/s², `sigma` 0.5, `tau` 1.0 s, `minGap` 2.5 m, `length` 5.0 m. These come from "a standard vehicle as used within the Stefan Krauß' thesis" ([SUMO vType docs](https://sumo.dlr.de/docs/Definition_of_Vehicles%2C_Vehicle_Types%2C_and_Routes.html)).

**Strengths.** Krauss is collision-free by construction, fast, and discrete-time (it fits 1 s or 0.1 s steps). Its stochastic dawdling produces spontaneous jams and capacity variability.
**Weaknesses.** With σ = 0.5, the default accelerations are fairly aggressive for urban start-up, and the dawdling noise strongly affects intersection discharge (saturation flow). Calibrated σ, τ and accel are therefore essential for signal studies. Speed-trajectory realism is lower than IDM's.

### 2.3 Gipps (1981)

Source: [Gipps, *Transp. Res. B* 15(2):105–111 (1981)](https://ideas.repec.org/a/eee/transb/v15y1981i2p105-111.html).

$$
v_n(t+\tau) = \min\Big\{\, v_n + 2.5\,a_n\tau\big(1-\tfrac{v_n}{V_n}\big)\sqrt{0.025+\tfrac{v_n}{V_n}},\;\;
b_n\tau + \sqrt{b_n^2\tau^2 - b_n\big[2(x_{n-1}-s_{n-1}-x_n) - v_n\tau - v_{n-1}^2/\hat b\big]}\,\Big\}
$$

([equation as reproduced in reviews, e.g. arXiv:2304.07143](https://arxiv.org/pdf/2304.07143)). Parameters: $a_n$ max acceleration, $b_n$ (negative) most severe desired braking, $V_n$ desired speed, $\tau$ reaction time, $s_{n-1}$ effective leader length, $\hat b$ the follower's estimate of the leader's braking.
**Strengths:** physically grounded safe-braking constraint; the basis of the AIMSUN car-following model and of the Krauss family. **Weaknesses:** the reaction-time step is coupled to the simulation step, and it is deterministic.

### 2.4 Wiedemann 74 / 99 — psycho-physical model (PTV Vissim)

Sources: Wiedemann, *Simulation des Straßenverkehrsflusses*, Univ. Karlsruhe 1974 ([SWOV record](https://swov.nl/en/publication/simulations-des-strassenverkehrsflusses)); [Fellendorf & Vortisch, "Microscopic Traffic Flow Simulator VISSIM", in Barceló (ed.) 2010, pp. 63–93](https://www.doi.org/10.1007/978-1-4419-6142-6_2); [PTV Vissim manual — W99 parameters](https://cgi.ptvgroup.com/vision-help/VISSIM_2025_ENG/Content/4_BasisdatenSim/FahrverhaltensparameterFolgeverh_Wied99.htm).

The model has four driving regimes: free driving, approaching, following (an unconscious oscillation around the desired distance) and emergency braking. Regime switches happen at *perception thresholds* in the (Δx, Δv) plane. In W99 these thresholds are:
$AX = CC0$; $ABX = CC0 + CC1\cdot v$; $SDX = ABX + CC2$; $SDV = CC5 + (DX - SDX)/CC3$; $CLDV$ and $OPDV$ depend on $CC4, CC5, CC6$ and $DX^2$. CC7–CC9 set acceleration oscillation and acceleration at standstill and at 80 km/h ([PTV W99 page](https://cgi.ptvgroup.com/vision-help/VISSIM_2025_ENG/Content/4_BasisdatenSim/FahrverhaltensparameterFolgeverh_Wied99.htm)). W74, recommended for urban roads, uses three parameters: standstill distance $ax$ (default 2.0 m), `bxAdd` (2.0) and `bxMult` (3.0) ([PTV W74 page](https://cgi.ptvgroup.com/vision-help/VISSIM_2025_ENG/Content/4_BasisdatenSim/FahrverhaltensparameterFolgeverh_Wied74.htm)). SUMO also implements `W99` and `Wiedemann` car-following ([SUMO vType docs](https://sumo.dlr.de/docs/Definition_of_Vehicles%2C_Vehicle_Types%2C_and_Routes.html); historical note: [Stebens 1995, Wiedemann in SUMO](https://eclipse.dev/sumo/documents/1995_Stebens_Traffic_Simulation_with_the_Wiedemann_Model.pdf)).
**Strengths:** the industry standard in consultancy and backed by practitioner calibration guidance; the CC1 (headway) and CC0 (standstill) parameters directly control saturation flow. **Weaknesses:** 10 parameters, often correlated and hard to identify; less analytically tractable.

### 2.5 Newell (2002) simplified car-following

Source: [Newell, *Transp. Res. B* 36(3):195–205 (2002)](https://ideas.repec.org/a/eee/transb/v36y2002i3p195-205.html).

$$x_n(t+\tau_n) = \min\{x_n(t) + v_f\,\tau_n,\ \ x_{n-1}(t) - d_n\}$$

Each follower copies the leader's trajectory, shifted by a time $\tau$ and a space $d$. This corresponds exactly to LWR with a **triangular fundamental diagram**: wave speed $w = d/\tau$, jam density $1/d$ ([Newell model overview](https://en.wikipedia.org/wiki/Newell%27s_car-following_model)).
**Strengths:** two parameters per driver; links micro to macro (CTM/LTM); excellent for validating that a simulator reproduces kinematic waves. **Weaknesses:** no acceleration bound (it produces infinite acceleration at queue discharge), and so no bounded-acceleration start-up loss unless extended.

### 2.6 Comparison summary

| Model | Params | Time | Collision-free | Stochastic | Best use for us |
|---|---|---|---|---|---|
| IDM / EIDM | 6 (+ noise) | continuous (integrate at ≤0.25 s) | yes (normal ops) | EIDM variant | Default for realism and AV studies |
| Krauss | 5–6 | discrete | yes | yes (σ) | Fast large networks; SUMO compatibility |
| Gipps | 5–6 | discrete (τ) | yes | no | Baseline / Aimsun parity |
| Wiedemann 74/99 | 3 / 10 | discrete | mostly | yes | Vissim parity, practitioner calibration |
| Newell | 2 | discrete | yes | no | Unit tests vs. kinematic wave theory |

---

## 3. Lane-changing models

### 3.1 Gipps (1986) — rule-based decision structure

[Gipps, *Transp. Res. B* 20(5):403–414 (1986)](https://ideas.repec.org/a/eee/transb/v20y1986i5p403-414.html) proposed a decision hierarchy for urban driving. Is a change possible (safe gap)? Is it necessary (turning intention, permanent obstruction, transit lane)? Is it desirable (speed advantage, heavy vehicles)? The paper notes that the *hierarchy* matters more than the specific formulas.

### 3.2 MOBIL — "Minimizing Overall Braking Induced by Lane changes"

Source: [Kesting, Treiber & Helbing, *TRR* 1999:86–94 (2007)](https://journals.sagepub.com/doi/abs/10.3141/1999-10).

The **safety criterion** keeps the new follower's braking acceptable: $\tilde a_n \ge -b_{\text{safe}}$.
The **incentive criterion** weighs one's own gain against the disadvantage to others, weighted by a **politeness factor** $p$:
$$\tilde a_c - a_c + p\big[(\tilde a_n - a_n) + (\tilde a_o - a_o)\big] > \Delta a_{th} \;(+\, a_{\text{bias}}\text{ for asymmetric/keep-right rules})$$
where $c$ is the changing vehicle, $n$ the new follower and $o$ the old follower, and tildes mark values after the change. MOBIL reuses whatever car-following model is in place, which is elegant for a custom engine. Order-of-magnitude values used by the authors are a politeness $p$ between 0 and 1 (often well below 1), $b_{\text{safe}}$ of a few m/s² (about 4) and a small threshold $\Delta a_{th}$ (about 0.1 m/s²). These are values to verify against the TRR paper before use. MOBIL handles *discretionary* changes; *mandatory* (route-driven) changes must be layered on top, for example with a bias that grows as the vehicle approaches the turn.

### 3.3 SUMO LC2013 (and SL2015 sub-lane)

LC2013 was developed by J. Erdmann, building on D. Krajzewicz's DK2004 model ([SUMO doxygen LC2013](https://sumo.dlr.de/doxygen/de/ddc/class_m_s_l_c_m___l_c2013.html); [Erdmann, SUMO2014 presentation](https://eclipse.dev/sumo/documents/2014/Presentation_LC2013_JakobErdmann.pdf); paper "SUMO's Lane-Changing Model" in *Modeling Mobility with Open Data*, Springer 2015, see [DLR elib](https://elib.dlr.de/93669) neighbourhood / [proceedings PDF](https://www.researchgate.net/profile/Laura_Bieker-Walz/publication/312193330_Modeling_Mobility_with_Open_Data/links/5a02c263aca2720df3cea0de/Modeling-Mobility-with-Open-Data.pdf)). It ranks motivations in a hierarchy: **strategic** (needed to follow the route) > **cooperative** (helping others merge) > **tactical / speed gain** > **regulatory / keep right**. Tuning parameters (defaults 1.0): `lcStrategic`, `lcCooperative`, `lcSpeedGain`, `lcKeepRight`, `lcAssertive` (gap acceptance) ([SUMO vType docs](https://sumo.dlr.de/docs/Definition_of_Vehicles%2C_Vehicle_Types%2C_and_Routes.html)). **SL2015** adds continuous lateral position (sub-lanes) for motorcycles, bikes and wide lanes, with `minGapLat` = 0.6 m.

For signal control, *strategic* lane changing dominates. Vehicles must reach the correct turn lane before the stop line, and wrong-lane blocking or short turn bays strongly affect queue spillback. This is a frequent source of unrealism in OSM-derived networks.

### 3.4 Gap acceptance

Gap acceptance (critical gap $t_c$, follow-up time $t_f$) drives both merging lane changes and unsignalized intersections (§4.2). In micro models it is either explicit (minimum lead and lag gaps, possibly drawn per driver) or implicit through car-following safety (MOBIL's $b_{\text{safe}}$, SUMO's `lcAssertive`, `jmTimegapMinor`).

---

## 4. Intersection modeling

### 4.1 Signalized intersections — HCM concepts

Main references: [*Highway Capacity Manual*, 7th ed. (TRB 2022)](https://nap.nationalacademies.org/26432) and [FHWA *Traffic Signal Timing Manual*, ch. 3](https://ops.fhwa.dot.gov/publications/fhwahop08024/chapter3.htm).

- **Saturation flow rate $s$:** the queue discharge rate during green. Field values "commonly range from 1,500 to 2,000 passenger cars per hour per lane", and the ideal value is "typically assumed to be 1,900 pc/h/ln". A 2.2 s saturation headway gives 3600/2.2 ≈ 1636 veh/h/ln ([FHWA STM ch. 3](https://ops.fhwa.dot.gov/publications/fhwahop08024/chapter3.htm)).
- **Start-up lost time:** about 2 s. **Clearance lost time** is part of the yellow + all-red interval, and the HCM default *total* lost time is 4 s per phase ([FHWA STM ch. 3](https://ops.fhwa.dot.gov/publications/fhwahop08024/chapter3.htm)).
- **Capacity of a lane group:** $c = s\cdot g/C$. **Degree of saturation:** $X = v/c$.
- **Delay.** Webster (1958) gives
  $d = \frac{C(1-\lambda)^2}{2(1-\lambda X)} + \frac{X^2}{2q(1-X)} - 0.65\left(\frac{C}{q^2}\right)^{1/3}X^{2+5\lambda}$,
  with $\lambda = g/C$: uniform delay + random delay − empirical correction ([Akgungor, analytical delay models review](https://nacto.org/wp-content/uploads/analytical_delay_models_for_signalized_intersections_akgungor.pdf)). The HCM uses a uniform + incremental + initial-queue delay formula that stays valid for $X \ge 1$.

**Validation implication.** A micro simulator should be tested so that a saturated approach discharges at about 1,700–1,900 veh/h/ln (or a locally measured value) with about 2 s start-up loss. Krauss/IDM parameters (τ/T, accel, minGap, σ) should be tuned to match this before any controller is evaluated. This is the single most important intersection calibration.

### 4.2 Unsignalized / priority intersections (TWSC, yield)

The HCM two-way stop control (TWSC) method uses gap acceptance (Harders / Siegloch type) with Poisson arrivals:
$$c_{p,x} = v_{c,x}\,\frac{e^{-v_{c,x}t_{c,x}/3600}}{1 - e^{-v_{c,x}t_{f,x}/3600}}$$
([HCM 7 formula as summarized in exam guide](https://open-exam-prep.com/study-guides/ptoe/managed-lanes-tsmo-its/unsignalized-intersections-roundabouts)).
Base critical headways (2-lane major road): major left 4.1 s, minor right 6.2 s, minor through 6.5 s, minor left 7.1 s. Follow-up times: 2.2 / 3.3 / 4.0 / 3.5 s ([PTV Visum help, HCM TWSC](https://cgi.ptvgroup.com/vision-help/VISUM_2025_ENG/Content/1_Benutzermodell%20IV/1_5_Vorfahrtsgeregelte%20Knoten.htm)).

In micro simulation this is done by *conflict-zone reservation*. SUMO computes for each link the set of foe links and grants a vehicle passage if the approaching foes' arrival/leave time windows do not overlap its own (with a time-gap parameter `jmTimegapMinor` = 1 s and an `impatience` mechanism that grows with waiting time) ([Erdmann & Krajzewicz, "SUMO's Road Intersection Model", LNCS 8594, 2014, doi:10.1007/978-3-662-45079-6_1](https://elib.dlr.de/93669); [SUMO vType docs](https://sumo.dlr.de/docs/Definition_of_Vehicles%2C_Vehicle_Types%2C_and_Routes.html)). Vissim uses "conflict areas" for the same purpose ([Fellendorf & Vortisch 2010](https://www.doi.org/10.1007/978-1-4419-6142-6_2)).

### 4.3 Roundabouts

HCM 6/7 models entry capacity as an exponential function of the conflicting circulating flow, $c_e = A\,e^{-B v_c}$. For a single-lane entry, $A = 1380$ pc/h and $B = 1.02\times10^{-3}$ ([exam-guide summary of HCM](https://open-exam-prep.com/study-guides/ptoe/managed-lanes-tsmo-its/unsignalized-intersections-roundabouts); equations 22-1 to 22-7 of HCM6, see [PTV Visum help](https://cgi.ptvgroup.com/vision-help/VISUM_2025_ENG/Content/1_Benutzermodell%20IV/1_5_Kreisverkehre%20nach%20der%20Methode%20des%20HCM.htm)). Design and operations guidance is in [NCHRP Report 672](https://www.nap.edu/catalog/22914/roundabouts-an-informational-guide-second-edition). In micro simulation, a roundabout is a ring of short links with yield (priority) at each entry. Calibrate by checking simulated entry capacity against the HCM curve. Large signalized roundabouts and "turbo" layouts need conflict areas inside the ring.

### 4.4 Conflict zones and turning movements — modelling checklist

- Separate **movements** (links / connections) per approach lane and turn. Internal junction lanes with geometry let vehicles occupy the junction. This is needed for blocking ("don't block the box") and for left-turn waiting positions ([SUMO intersection model](https://elib.dlr.de/93669)).
- **Permitted vs. protected left turns:** permitted turns yield to opposing through traffic (gap acceptance); protected turns have their own phase.
- **Pedestrian crossings** conflict with right/left turns. SUMO uses `jmCrossingGap` (10 m) ([SUMO vType docs](https://sumo.dlr.de/docs/Definition_of_Vehicles%2C_Vehicle_Types%2C_and_Routes.html)).
- **Turn speed reduction** comes from the curvature of junction geometry.
- **Spillback** from a downstream link must block upstream discharge. This is essential for evaluating congestion management.

---

## 5. Macroscopic and mesoscopic flow models

### 5.1 Fundamental diagram (FD)

$q = \rho\,v$. Greenshields (1935) proposed a linear speed–density relation $v = v_f(1-\rho/\rho_j)$, which gives a parabolic $q(\rho)$. It was famously fitted on very few data points ([TRID record](https://trid.trb.org/view/120649); [Greenshields model notes, U. Idaho](https://www.webpages.uidaho.edu/niatt_labmanual/Chapters/trafficflowtheory/professionalpractice/GreenshieldsModel.htm)). Modern practice favours the **triangular FD**, defined by free-flow speed $v_f$, capacity $q_{\max}$, jam density $\rho_j$ and backward wave speed $w$, which is consistent with Newell's car-following ([Newell 2002](https://ideas.repec.org/a/eee/transb/v36y2002i3p195-205.html)). Empirical wave speed is about 15 km/h upstream ([Treiber et al. 2000](https://arxiv.org/pdf/cond-mat/0002177)).

### 5.2 LWR kinematic wave model

[Lighthill & Whitham (1955), *Proc. R. Soc. A* 229:317–345](https://doi.org/10.1098/rspa.1955.0089) and [Richards (1956), *Oper. Res.* 4:42–51](https://doi.org/10.1287/opre.4.1.42):
$$\partial_t\rho + \partial_x Q(\rho) = 0$$
Shocks form at queue tails, and rarefaction fans form at queue heads (signal turning green).

### 5.3 Cell Transmission Model (Daganzo 1994, 1995)

[Daganzo, *Transp. Res. B* 28(4):269–287 (1994)](https://ideas.repec.org/a/eee/transb/v28y1994i4p269-287.html) introduced a Godunov-type discretization of LWR with a trapezoidal/triangular FD. With cell length = $v_f\Delta t$:
$$y_i(t) = \min\{\,n_{i-1}(t),\ Q_i(t),\ \tfrac{w}{v_f}\,[N_i - n_i(t)]\,\},\qquad n_i(t+1) = n_i(t) + y_i(t) - y_{i+1}(t)$$
Each update takes the minimum of upstream **sending** (demand) and downstream **receiving** (supply). The model captures queue formation, propagation and dissipation without explicit shock tracking ([Berkeley ITS abstract](https://its.berkeley.edu/publications/cell-transmission-model-dynamic-representation-highway-traffic-consistent-hydrodynamic)). [Part II (1995), *Transp. Res. B* 29(2):79–93](https://ideas.repec.org/a/eee/transb/v29y1995i2p79-93.html) extends it to networks with merge/diverge junctions and time-varying OD and turning proportions.

### 5.4 Link Transmission Model and queue models

- **Link Transmission Model (LTM)** (Yperman 2007) is an operational form of Newell's simplified kinematic-wave theory. It uses cumulative vehicle counts at link boundaries, so it needs no internal cells and is very fast for DTA (["Continuous formulations and analytical properties of the link transmission model", arXiv:1405.7080](https://arxiv.org/abs/1405.7080)). SUMO MESO has an optional LTM mode ([SUMO MESO docs](https://sumo.dlr.de/docs/Simulation/Meso.html)), and an improved discrete LTM for SUMO is proposed in [Ni et al. 2026](https://arxiv.org/abs/2606.09282).
- **Point queues** (vertical queues, no spillback) and **spatial / physical queues** (with storage capacity) are the simplest. MATSim's QSim is a spatial queue model with link storage and flow capacity ([Horni, Nagel & Axhausen 2016](http://dx.doi.org/10.5334/baw)). SUMO MESO splits edges into ~100 m segments with headway parameters `tauff`, `taufj`, `taujf` and `taujj` that depend on whether segments are free or jammed ([SUMO MESO docs](https://sumo.dlr.de/docs/Simulation/Meso.html)).
- Gawron's queue simulation was built for fast iterative assignment ([Gawron 1998, IJMPC 9(3):393–407](https://ideas.repec.org/a/wsi/ijmpcx/v09y1998i03ns0129183198000303.html)).

**Use for us.** CTM/LTM is the right internal *prediction model* for model-based controllers (perimeter control, MPC, max-pressure analysis) and a fast mode for demand/assignment loops. A CTM over the same network is also a cheap correctness check of the micro engine: queue lengths and wave speeds should roughly agree.

---

## 6. Demand modeling and traffic assignment

### 6.1 Four-step model

The classic sequential model has four steps ([McNally, "The Four Step Model", UCI-ITS-WP-00-17](https://escholarship.org/uc/item/1zb9n444); [book chapter, Handbook of Transport Modelling](https://dx.doi.org/10.1108/9780857245670-003)):
1. **Trip generation**: productions and attractions per zone from land use and socio-demographics.
2. **Trip distribution**: OD matrix, typically a gravity model.
3. **Mode choice**: typically logit.
4. **Route assignment**: static UE.

For a *traffic-management testbed* we usually need only the car OD matrix by time slice (for example 15 min) plus route choice. Steps 1–3 can be replaced by an existing municipal/regional model if one exists, or by OD estimation from counts (§9).

### 6.2 OD matrices and route generation

- **OD matrix → trips → routes:** SUMO `od2trips` + `duarouter`; `marouter` does macroscopic assignment from OD matrices with link resistance functions ([SUMO marouter](https://sumo.dlr.de/docs/marouter.html); [SUMO DUA docs](https://sumo.dlr.de/docs/Demand/Dynamic_User_Assignment.html)).
- **Counts → routes:** `routeSampler` (turn counts and edge counts, with a whitelist of plausible routes), `flowrouter` (sparse detectors) and `dfrouter` (full detector coverage, typically motorways) ([SUMO: Routes from observation points](https://sumo.dlr.de/docs/Demand/Routes_from_Observation_Points.html)).
- **Random demand** (`randomTrips`, used by OSMWebWizard) is fine for smoke tests but **not** for evaluating controllers. It produces unrealistic through traffic and turning ratios ([OSMWebWizard docs](https://sumo.dlr.de/docs/Tutorials/OSMWebWizard.html)).
- **Activity-based demand:** MATSim plans synthetic persons' daily activity chains and lets them replan (route, departure time, mode) towards equilibrium ([Horni et al. 2016](http://dx.doi.org/10.5334/baw)). Real-world SUMO city scenarios built this way include LuST, a 24-hour Luxembourg scenario ([Codecà, Frank & Engel, IEEE VNC 2015](https://orbilu.uni.lu/handle/10993/23011)). A practical OSM-to-scenario walkthrough is [Clemente 2022, SUMO Conf. Proc. 3](https://www.tib-op.org/ojs/index.php/scp/article/view/109).

### 6.3 Assignment: user equilibrium, stochastic UE, dynamic

- **Wardrop's principles** ([Wardrop 1952, Proc. ICE 1(3):325–362](https://doi.org/10.1680/ipeds.1952.11259)). (1) **User equilibrium**: all used routes between an OD pair have equal and minimal travel time. (2) **System optimum**: total travel time is minimized. Static UE is solved as a convex program (Beckmann), typically with Frank–Wolfe ([Sheffi 1985, *Urban Transportation Networks*, free PDF](https://sheffi.mit.edu/sites/sheffi.mit.edu/files/sheffi_urban_trans_networks_0.pdf)).
- **Stochastic UE (SUE):** drivers perceive costs with error (logit/probit), so no driver believes they can improve their *perceived* cost ([Daganzo & Sheffi 1977, *Transp. Sci.* 11(3):253–274](https://doi.org/10.1287/trsc.11.3.253)). SUE is more realistic and also numerically better behaved (unique flows).
- **Dynamic traffic assignment (DTA):** time-dependent route choice plus *dynamic network loading* by a traffic flow model (CTM, LTM, meso or micro) ([TRB Circular E-C153, *DTA: A Primer*, 2011](https://onlinepubs.trb.org/onlinepubs/circulars/ec153.pdf)). In simulation-based DTA, iterations alternate between routing on the last experienced costs and simulating. SUMO's `duaIterate.py` does this with **Gawron** (default, β = 0.3) or **Logit** route choice, 50 iterations by default, with optional convergence criteria. There is also a one-shot mode where vehicles reroute en-route at departure ([SUMO DUA docs](https://sumo.dlr.de/docs/Demand/Dynamic_User_Assignment.html); [Gawron 1998](https://ideas.repec.org/a/wsi/ijmpcx/v09y1998i03ns0129183198000303.html)).

**Testing implication.** When a controller changes signal timings or issues routing advice, demand *re-equilibrates* over days. A credible evaluation reports both (a) the immediate effect with fixed routes and (b) the effect after re-running DUA. Otherwise route diversion (traffic moving onto improved corridors) is missed. Routing/congestion-management modules additionally need en-route rerouting with a compliance rate.

---

## 7. Traffic signal control strategies to be tested

### 7.1 Fixed-time (Webster)

Webster's optimal cycle length is $C_0 = \frac{1.5L + 5}{1 - Y}$, where $L$ is total lost time per cycle and $Y=\sum$ critical flow ratios $(v/s)$. Green is split in proportion to the critical flow ratios ([U. Idaho signal timing lab manual](https://www.webpages.uidaho.edu/niatt_labmanual/Chapters/signaltimingdesign/theoryandconcepts/CycleLengthDetermination.htm); original: Webster 1958, Road Research Technical Paper 39). Coordination between intersections uses offsets and bandwidth-maximization methods. SUMO's netconvert generates default fixed programs with a 90 s cycle, equal green splits and yellow time derived from approach speed ([SUMO Traffic Lights docs](https://sumo.dlr.de/docs/Simulation/Traffic_Lights.html)). **Fixed-time (Webster-optimized) is the mandatory baseline.**

### 7.2 Actuated control

Green is extended while detectors see a continuous stream (headway gap < `max-gap`, default 3 s in SUMO), within the bounds [minDur, maxDur]. SUMO also provides delay-based actuation (on accumulated time loss) and **NEMA** dual-ring controllers ([SUMO Traffic Lights docs](https://sumo.dlr.de/docs/Simulation/Traffic_Lights.html)). Actuated control is the realistic "status quo" baseline in many cities.

### 7.3 SCOOT

SCOOT (Split, Cycle and Offset Optimisation Technique; TRRL, UK) measures upstream detector occupancy to build *cyclic flow profiles*. An online model predicts queues, and splits, offsets and cycle are adjusted in frequent small increments. Reported improvements were of the order of 15% over fixed-time systems ([TRID abstract](https://trid.trb.org/View/179439); [Hunt, Robertson, Bretherton & Winton 1981, TRRL LR1014](https://www.trl.co.uk/uploads/trl/documents/LR1014.pdf); [overview](https://en.wikipedia.org/wiki/Split_Cycle_Offset_Optimisation_Technique)). It is proprietary (TRL Software), so in research it is approximated by re-implementations of its principles.

### 7.4 SCATS

SCATS (Sydney Coordinated Adaptive Traffic System) has a hierarchical architecture: regional computers plus local controllers. It uses stop-line detectors to measure the *degree of saturation* and picks/adapts cycle length, splits and offsets from a library of plans, with "marriage/divorce" of subsystems ([Sims 1979, ARRB proceedings, TRID](https://trid.trb.org/View/147832); [overview](https://www.Wikipedia.org/wiki/SCATS)). It is also proprietary (Transport for NSW). Like SCOOT, research testbeds implement simplified emulations.

### 7.5 Max-pressure (Varaiya 2013)

[Varaiya, "Max pressure control of a network of signalized intersections", *Transp. Res. C* 36:177–195 (2013)](https://doi.org/10.1016/j.trc.2013.08.014). At each decision step, every intersection activates the phase $p$ that maximizes
$$P(p) = \sum_{(i,j)\in p} s_{ij}\Big(x_{ij} - \sum_{k} r_{jk}\,x_{jk}\Big),$$
i.e. upstream queue minus turning-ratio-weighted downstream queue, times saturation flow. It is **decentralized**, needs only local queue measurements, and is proven to **maximize network throughput** (stabilize any demand that any controller could stabilize). Practical variants address phase-switching losses and travel-time-based pressure ([Kouvelas et al., TRR 2014, max-pressure variant](https://www.research-collection.ethz.ch/bitstream/20.500.11850/275877/4/Kouvelas_EtAl_TRR_2014.pdf)). Max-pressure is the **strongest simple non-learning baseline** and must be included.

### 7.6 RL-based control

- **Survey:** [Wei, Zheng, Gayah & Li, "A Survey on Traffic Signal Control Methods", arXiv:1904.08117](https://arxiv.org/abs/1904.08117).
- **PressLight** (KDD 2019) uses deep RL with **pressure as the reward**, which links the RL objective to max-pressure throughput theory ([paper PDF](https://jhc.sjtu.edu.cn/~gjzheng/paper/kdd2019_presslight/kdd2019_presslight_paper.pdf); [code](https://github.com/wingsweihua/presslight)).
- **CoLight** (CIKM 2019) uses graph attention networks for communication between neighbouring intersections, so it scales to hundreds of intersections ([arXiv:1905.05717](https://arxiv.org/abs/1905.05717)).
- **Benchmarks:**
  - **RESCO**: SUMO-based, with realistic Cologne/Ingolstadt scenarios (single intersection, 7-intersection corridor, 21-intersection region). It found that prior algorithms "are not robust to varying sensing assumptions and non-stylized intersection layouts" ([Ault & Sharon, NeurIPS 2021 D&B](https://datasets-benchmarks-proceedings.neurips.cc/paper/2021/hash/f0935e4cd5920aa6c7c996a5ee53a70f-Abstract-round1.html)).
  - **CityFlow**: a fast RL simulator, reported >20× faster than SUMO ([Zhang et al., WWW 2019, arXiv:1905.05217](https://arxiv.org/abs/1905.05217)).
  - **LibSignal**: a cross-simulator (SUMO + CityFlow) library with unified metrics; the authors had to *calibrate* the two simulators against each other before results were comparable ([Mei et al. 2023, *Machine Learning*, arXiv:2211.10649](https://arxiv.org/abs/2211.10649)).
  - **sumo-rl**: Gymnasium/PettingZoo wrapper that includes RESCO networks ([GitHub](https://github.com/lucasalegre/sumo-rl)).
- **Sim-to-real gap:** policies trained in simulation degrade under real dynamics. Grounded action transformation is one mitigation ([Da, Mei, Sharma & Wei, UGAT, IEEE CDC 2023, arXiv:2307.12388](https://arxiv.org/abs/2307.12388)). For our simulator this means **stochasticity and parameter randomization** (driver heterogeneity, detector noise, demand variability) are features, not bugs.

### 7.7 How controllers are evaluated (common practice)

1. **Baselines:** fixed-time (Webster), actuated, max-pressure; for RL also SOTL / MaxPressure / FixedTime as in RESCO and LibSignal.
2. **Scenarios:** low / medium / high / over-saturated demand, peak-hour time-varying demand, incidents (lane closure) and detector failures.
3. **Metrics:** average travel time (most common in RL work), delay, queue length, throughput, stops, emissions, and fairness (max wait, Jain's index). See §10.
4. **Statistics:** multiple random seeds (micro is stochastic), reporting mean ± CI. FHWA/Hollander & Liu stress the need for repeated runs ([Hollander & Liu 2008, *Transportation* 35:347–362](https://iaorifors.com/paper/62434)).
5. **Warm-up** period excluded; network cleared (cool-down) so unfinished trips are not hidden. Report *teleports/gridlocks* (SUMO teleports stuck vehicles by default).
6. **Realism of sensing:** controllers should see only what real detectors or cameras see, not simulator ground truth. RESCO showed that this assumption changes the algorithm ranking.

---

## 8. Existing simulators and frameworks

| Tool | Type / license | Scale | Strengths | Limitations | Relevance |
|---|---|---|---|---|---|
| **Eclipse SUMO** | Open source (EPL-2.0), C++ | Micro (+ MESO, sub-lane) | Full toolchain: netconvert (OSM import), demand tools (od2trips, duarouter, duaIterate, routeSampler), many CF/LC models, actuated/NEMA TLS, detectors, HBEFA/PHEM emissions, TraCI/libsumo control ([Alvarez Lopez et al., ITSC 2018, doi:10.1109/ITSC.2018.8569938](https://doi.org/10.1109/ITSC.2018.8569938)) | Slower than CityFlow for pure RL; the MESO model has known LWR deviations ([Ni et al. 2026](https://arxiv.org/abs/2606.09282)); teleporting on gridlock can hide problems | **Primary candidate** |
| **TraCI / libsumo** | SUMO APIs | — | TraCI: socket client/server, any language; **libsumo**: same API linked in-process, much faster (no socket); bindings for Python, Java, C++, C#, Matlab ([libsumo docs](https://sumo.dlr.de/docs/Libsumo.html)) | libsumo: GUI highly experimental, one simulation per process (use multiprocessing) | Controller ↔ simulator interface |
| **CityFlow** | Open source, C++ + Python | Micro (simplified) | >20× faster than SUMO; designed for multi-agent RL ([Zhang et al. 2019](https://arxiv.org/abs/1905.05217)) | Simplified intersections/lane changing; weaker tooling for real networks and demand; needs cross-calibration with SUMO ([LibSignal](https://arxiv.org/abs/2211.10649)) | RL training accelerator, not the reference |
| **MATSim** | Open source (GPL), Java | Meso (queue) + activity-based demand | Agent plans, co-evolutionary replanning, large regions ([Horni et al. 2016](http://dx.doi.org/10.5334/baw)) | No lane-level signals/queues | Demand generation / long-term route effects |
| **Aimsun Next** | Commercial | Micro, meso, hybrid, macro | Consistent hybrid meso–micro with shared network and costs ([Aimsun hybrid docs](https://docs.aimsun.com/next/24.0.0/UsersManual/HybridSimulator.html)) | License cost; closed | Reference practice |
| **PTV Vissim** | Commercial | Micro (Wiedemann) | Industry standard; conflict areas; signal controller emulation; pedestrians ([Fellendorf & Vortisch 2010](https://www.doi.org/10.1007/978-1-4419-6142-6_2)) | License; closed | Reference practice / parity checks |
| **CARLA** | Open source, Unreal Engine | Sub-microscopic (vehicle dynamics, sensors) | Photorealistic sensors for AV research ([Dosovitskiy et al., CoRL 2017, arXiv:1711.03938](https://arxiv.org/abs/1711.03938)) | Not designed for city-scale traffic flow; heavy | Out of scope except camera-perception testing (co-simulation with SUMO exists) |

### 8.1 OSM import pipeline

- **netconvert** converts OSM into a SUMO network. Recommended options: `--geometry.remove --ramps.guess --junctions.join --tls.guess-signals --tls.discard-simple --tls.join --tls.default-type actuated`. Typemaps (`osmNetconvert.typ.xml`, `osmNetconvertUrbanDe.typ.xml`, pedestrian/bicycle variants) map OSM highway types to speeds and lanes. Known issues: dual carriageways produce duplicate junctions (fix with `--junctions.join`, review manually), and signals are only assigned where all incoming arms have an upstream signal ([SUMO OSM import docs](https://sumo.dlr.de/docs/Networks/Import/OpenStreetMap.html)).
- **OSMWebWizard** builds a complete scenario from a selected map area: network plus `randomTrips` demand parameterized by "through traffic factor" and vehicles/hour/lane-km. It is a quick start, but the demand is synthetic ([OSMWebWizard docs](https://sumo.dlr.de/docs/Tutorials/OSMWebWizard.html)).
- **Practical lesson from the literature:** OSM networks *always* need manual repair before signal studies. Check turn lanes, lane counts per approach, signal phases (OSM has no timing plans), junction clusters and turn restrictions ([Clemente 2022](https://www.tib-op.org/ojs/index.php/scp/article/view/109); [LuST, Codecà et al. 2015](https://orbilu.uni.lu/handle/10993/23011)).

### 8.2 Reuse vs. build

| Component | Reuse | Build |
|---|---|---|
| Network import from OSM | netconvert (mature, handles TLS guessing, junction joining) | Iași-specific corrections layer (lanes, phases, turn restrictions) |
| Microscopic engine | SUMO (validated models, years of bug fixes) | Only if we need GPU/massive-parallel RL or a fully differentiable engine |
| Demand | od2trips, routeSampler, duaIterate | OD estimation pipeline from Iași counts / FCD |
| Controller interface | TraCI/libsumo | Our own controller abstraction (sensor model → controller → actuation) that is *simulator-agnostic* |
| Emissions | SUMO HBEFA/PHEMlight | COPERT post-processing if needed for EU reporting |
| RL environments | sumo-rl, RESCO, LibSignal | Iași-specific scenario wrappers |
| Metrics / dashboards | SUMO outputs (tripinfo, edgeData, queue, emissions) | Unified KPI computation + fairness metrics |

---

## 9. Calibration and validation

### 9.1 Principles

Calibration adjusts model parameters (driver behaviour, demand OD, route choice) until simulated outputs match observations. **Validation** then checks the result against *independent* data that was not used for calibration ([Hollander & Liu 2008](https://iaorifors.com/paper/62434); [FHWA Traffic Analysis Toolbox Vol. III, 2019 update, ch. 5](https://ops.fhwa.dot.gov/publications/fhwahop18036/chapter5.htm)). Recommended order:
1. **Supply / capacity first:** bottleneck throughput, saturation flow at signals, discharge headways.
2. **Then demand and route choice:** OD flows, turning ratios.
3. **Then dynamics:** travel times, queue lengths, congestion onset and duration ([FHWA 2019, ch. 5](https://ops.fhwa.dot.gov/publications/fhwahop18036/chapter5.htm)).

### 9.2 Metrics and acceptance criteria

- **GEH statistic** (hourly counts $M$ = model, $C$ = observed): $GEH = \sqrt{\dfrac{2(M-C)^2}{M+C}}$. The UK DfT criterion is **GEH < 5 for > 85% of links**. Flow criteria: within 100 veh/h if observed < 700; within 15% for 700–2,700 veh/h; within 400 veh/h above 2,700 ([DfT TAG Unit M3.1](https://assets.publishing.service.gov.uk/media/6a033d074fb0713aa63ea802/tag-m3-1-highway-assignment-modelling.pdf)).
- **Journey times:** within 15% (or 1 minute if higher) for > 85% of routes (DfT practice, widely reused; e.g. [Camden model validation](https://contentauthortraining.camden.gov.uk/documents/20142/18572877/ONE+Model+Validation.pdf/ecbdab2b-5188-7c96-9b45-1eae811e732c)).
- **FHWA 2019** replaces the averaged "design day" with a *representative day* and time-varying bands. Its four criteria: (I) 95% of simulated values within ~2σ band; (II) two-thirds within 1σ, including critical intervals; (III) bounded dynamic absolute error; (IV) systematic error < ⅓ of that bound ([FHWA 2019, ch. 5](https://ops.fhwa.dot.gov/publications/fhwahop18036/chapter5.htm)). Some DOTs add tighter thresholds (e.g. [TxDOT GEH < 3 on state facilities](https://www.txdot.gov/manuals/des/tsp/chapter-13-microsimulation-analysis/13-5-calibration/13-5-2-acceptability-criteria/13-5-2-4-fhwa-traffic-analysis-toolbox--volume-iii.html)).
- **Others:** RMSE / RMSNE on counts, speeds and travel times; Theil's U; queue lengths at critical approaches; speed–flow scatter vs. detector data; trajectory error for car-following ([Kesting & Treiber 2008](https://arxiv.org/abs/0803.4063)).

### 9.3 Data sources

| Source | Gives | Notes |
|---|---|---|
| Inductive loops / signal detectors (from the city's signal system) | Counts, occupancy, sometimes speeds; actuations | Best for counts/GEH; also needed as controller inputs |
| Video / manual turning-movement counts | Turn ratios per intersection | Input to `routeSampler` ([SUMO docs](https://sumo.dlr.de/docs/Demand/Routes_from_Observation_Points.html)) |
| Floating car data (FCD), e.g. TomTom Traffic Stats | Historical segment speeds and travel times per time slice, from anonymized navigation-device FCD ([TomTom Traffic Stats](https://docs.tomtom.com/traffic-stats/documentation/product-information/introduction)); TomTom also sells O/D analysis ([TomTom O/D Analysis](https://docs.tomtom.com/od-analysis/documentation/product-information/introduction)) | Excellent for travel-time validation; commercial |
| Google Maps Routes API | Traffic-aware `duration` vs. `staticDuration` for specified OD pairs ([Google Routes API traffic options](https://developers.google.com/maps/documentation/routes/traffic-opt)) | Point sampling of current/predicted travel time; check license terms before storing data |
| Signal timing plans (from the city) | Phases, cycle, offsets, detector logic | Essential; OSM has none |
| Trajectory datasets (e.g. NGSIM) | Car-following calibration | Not local; use for priors ([Kurtc & Treiber](https://arxiv.org/abs/1610.03124)) |

### 9.4 Calibration methods

- **Manual / heuristic:** adjust global CF parameters (headway, σ, accel) to match saturation flow, then fix local coding errors. This is FHWA's practitioner approach.
- **Black-box optimization:** genetic algorithms ([Kesting & Treiber 2008](https://arxiv.org/abs/0803.4063)). **SPSA**, the standard for simultaneous demand + supply calibration of DTA models (Balakrishna 2006). Weighted **W-SPSA** for high-dimensional OD vectors with sparse correlations ([W-SPSA in practice, ISTTT 2015 preprint (MIT)](https://web.mit.edu/cami/Public/CLA_WSPSAinPractice_ISTTT_2015_preprint.pdf)). Bayesian optimization and metamodels for expensive simulations ([Osorio, dynamic OD calibration](https://web.mit.edu/osorioc/www/papers/osoDynamicOD.pdf)).
- **OD estimation from counts:** bilevel (assignment inside) or path-based sampling (`routeSampler`).
- **Replications:** micro models are stochastic, so determine the number of seeds needed for a target confidence interval ([Hollander & Liu 2008](https://iaorifors.com/paper/62434)).

---

## 10. Evaluation metrics for traffic management

| Metric | Definition | Notes / source |
|---|---|---|
| **Average travel time** | Mean of arrival − departure over completed trips (+ penalty or inclusion of unfinished trips) | Most common RL-TSC metric ([LibSignal](https://arxiv.org/abs/2211.10649)) |
| **Delay / time loss** | Actual − free-flow travel time; control delay per vehicle at intersections | HCM LOS is based on control delay ([HCM 7](https://nap.nationalacademies.org/26432)) |
| **Waiting time** | Time at speed < 0.1 m/s (SUMO definition) | Also max waiting time, important for fairness |
| **Queue length** | Vehicles (or metres) queued per lane; max and average; spillback events | Spillback into upstream junctions is critical in urban grids |
| **Throughput** | Vehicles served per hour (network exits or per intersection) | The max-pressure guarantee is about throughput ([Varaiya 2013](https://doi.org/10.1016/j.trc.2013.08.014)) |
| **Number of stops** | Speed drops below a threshold then recovers | Proxy for comfort, emissions and safety |
| **Emissions / fuel** | CO₂, NOx, PMx, fuel, energy | SUMO default `HBEFA3/PC_G_EU4`; also HBEFA 2.1/3.1/4.2-based, PHEMlight, PHEMlight5 and EV models ([SUMO emissions docs](https://sumo.dlr.de/docs/Models/Emissions.html)). HBEFA official: [hbefa.net](https://hbefa.net) (v5.1 published Oct 2025, [INFRAS](https://www.infras.ch/en/news/2025/10/23/comprehensive-update-of-the-handbook-of-emission-factors-for-road-transport/)). For EU inventory consistency: COPERT / [EMEP/EEA Guidebook 2023, 1.A.3.b](https://www.eea.europa.eu/publications/emep-eea-guidebook-2023/part-b-sectoral-guidance-chapters/1-energy/1-a-combustion/1-a-3-b-i) |
| **Fairness** | Delay- and throughput-based fairness ([Raeis et al., arXiv:2107.10146](https://arxiv.org/abs/2107.10146)); Jain's fairness index and waiting-time disparity across movements/approaches ([Wan et al. 2024](https://scholars.cityu.edu.hk/en/publications/fair-and-efficient-traffic-light-control-with-reinforcement-learn/)) | Average-travel-time optimizers can starve minor movements, so always report max wait and 95th-percentile delay per movement |
| **Robustness** | Performance under demand ±20%, detector failure, incidents, sensor noise | RESCO showed rankings change with sensing assumptions ([Ault & Sharon 2021](https://datasets-benchmarks-proceedings.neurips.cc/paper/2021/hash/f0935e4cd5920aa6c7c996a5ee53a70f-Abstract-round1.html)) |
| **Multimodal** | Pedestrian wait, bus/tram delay (if transit priority is tested) | HCM 7 multimodal methods ([HCM 7](https://nap.nationalacademies.org/26432)) |
| **Simulation health** | Teleports, gridlocks, unfinished trips, collisions/emergency braking | Must be ~0 for results to be trusted |

---

## 11. Recommendations for our simulator

### 11.1 Recommended model choices

| Concern | Recommendation | Rationale |
|---|---|---|
| Scale | **Microscopic** as the reference engine; **mesoscopic/LTM** fast mode for demand/assignment loops; **CTM** as an optional controller-internal model | Signal control needs lane-level queues and phases (§1) |
| Car-following | **IDM/EIDM** as primary (stochastic EIDM for realism); **Krauss** as a compatible alternative; keep the model pluggable | IDM is well-calibrated in the literature and its parameters are interpretable; Krauss is SUMO's default and fast (§2) |
| Lane changing | **Strategic (route-driven) + MOBIL** for discretionary changes, i.e. LC2013-like hierarchy | Correct turn-lane selection matters most at urban signals (§3) |
| Junctions | Conflict-zone reservation with gap acceptance ($t_c$, $t_f$ from HCM priors), internal junction lanes, spillback blocking | Matches SUMO/Vissim practice (§4) |
| Signals | Phase-based controller abstraction: fixed-time, actuated (gap-out/max-out), NEMA-style rings optional; controllers see only *sensor* data | Fair, realistic evaluation (§7.7) |
| Demand | Time-sliced OD (15 min) → routes via DUA (Gawron/logit) + `routeSampler`-style count matching | Turn counts and FCD travel times are what we can get (§6, §9) |
| Emissions | HBEFA-based (as in SUMO), optional COPERT post-processing | EU-relevant (§10) |
| Time step | 0.5 s default (0.1–1 s supported) | Balances accuracy of start-up/actuation against speed; IDM needs ≤ 0.25 s internal stepping or ballistic update |

### 11.2 Build on SUMO or write a custom engine?

**Recommendation: build on SUMO for the reference simulator (Phase 1). Wrap it behind our own simulator-agnostic interface. Write a custom lightweight engine only if and where SUMO is a bottleneck (Phase 2+).**

| Option | Pros | Cons |
|---|---|---|
| **A. SUMO + our layers (recommended)** | Validated models and decades of edge-case fixes (junctions, lane changes, teleports); netconvert OSM import; demand and DUA tools; actuated/NEMA signals; HBEFA emissions; TraCI/libsumo; existing RL ecosystem (sumo-rl, RESCO, LibSignal) for comparison with published results; citable ([Alvarez Lopez et al. 2018](https://doi.org/10.1109/ITSC.2018.8569938)) | Speed limits for massive RL training (CityFlow reported >20× faster, [Zhang et al. 2019](https://arxiv.org/abs/1905.05217)); C++ codebase is large if we must change core models; teleporting must be monitored; MESO fidelity concerns ([Ni et al. 2026](https://arxiv.org/abs/2606.09282)) |
| **B. Custom engine from scratch** | Full control and understanding; can be GPU-vectorized or differentiable; tailored data model for Iași; no external dependency | Re-implementing junction logic, lane changing, OSM import and demand tools takes months and brings subtle bugs; results not comparable with the literature until cross-validated; calibration credibility has to be earned |
| **C. Hybrid (recommended long-term)** | SUMO as the "ground truth" reference; a custom fast engine (e.g. vectorized IDM + MOBIL + simple junctions, or an LTM) for RL training and what-if sweeps, **cross-validated against SUMO** the way LibSignal cross-calibrated SUMO and CityFlow ([Mei et al. 2023](https://arxiv.org/abs/2211.10649)) | Two engines to maintain; needs a shared scenario format |

The task framing ("build from scratch") can be met with option C. The *system* (scenario pipeline, controller API, metrics, calibration, custom fast engine) is ours, while SUMO serves as the trusted reference and validator. If the project requires a fully custom engine, use SUMO outputs as the **test oracle**: matching saturation flow, queue lengths and travel times on the same Iași scenario.

### 11.3 Architecture sketch

```
            +--------------------------- Scenario layer ---------------------------+
 OSM  -->   | netconvert / importer -> Iași corrections (lanes, phases, turns) -> |
 city data  | network.(net.xml | own format)     signal plans    detector layout   |
            +-----------------------------------------------------------------------+
                    |                         |
            +-------v--------+        +-------v-----------------------------+
            | Demand module  |        | Simulation engine (pluggable)       |
            | OD(15 min) ->  |------->|  - SUMO via libsumo (reference)     |
            | routes (DUA,   |        |  - custom fast engine (later)       |
            | routeSampler)  |        +-------^-----------------+-----------+
            +----------------+                | actuation       | sensor data
                                      +-------+-----------------v-----------+
                                      | Controller API (sim-agnostic)       |
                                      |  fixed | actuated | max-pressure |  |
                                      |  SCOOT/SCATS-like | RL agents     |  |
                                      |  routing advice / congestion mgmt |  |
                                      +-------------------+-----------------+
                                                          |
                                      +-------------------v-----------------+
                                      | Metrics & calibration               |
                                      |  KPIs (§10), GEH/RMSE (§9),         |
                                      |  seeds/CI, dashboards               |
                                      +-------------------------------------+
```

Key design rules:
1. The controller sees **sensor observations** (loop counts, occupancy, camera queue estimates with noise and latency), never simulator internals.
2. **Deterministic seeds** and scenario versioning, so every result is reproducible.
3. **Batch runner** for N seeds × M demand levels × K controllers, with statistical reporting.
4. **Simulation-health gates:** fail the run on teleports, gridlock or emergency braking above set thresholds.

### 11.4 Minimal viable scope (MVP)

1. **One Iași corridor or small district** (about 5–15 signalized intersections) imported from OSM and manually corrected: lanes, turn bays, real phase plans if obtainable.
2. **Demand:** peak-hour, 15-min OD or turn-count-based routes. Use a realistic estimate, *not* randomTrips.
3. **Engine:** SUMO via libsumo, IDM or calibrated Krauss, LC2013, 0.5 s step.
4. **Controllers:** fixed-time (Webster), actuated (gap-out) and max-pressure. One RL agent (e.g. PressLight-style DQN with a pressure reward) as stretch.
5. **Calibration-lite:** saturation flow ≈ 1,700–1,900 veh/h/ln at a test approach ([FHWA STM](https://ops.fhwa.dot.gov/publications/fhwahop08024/chapter3.htm)); GEH < 5 on available counts; travel times within 15% vs. TomTom/Google samples.
6. **Metrics:** travel time, delay, queue (mean/max), throughput, stops, CO₂ (HBEFA), max wait / Jain's index; ≥ 10 seeds per configuration.
7. **Stress tests:** +20% demand, lane-closure incident, detector failure.

Then extend to city-wide meso/DUA, transit priority (trams/buses), routing advice with compliance rates, and a custom fast engine for RL.

### 11.5 Prioritized reading list (top ~10)

1. **Treiber & Kesting, *Traffic Flow Dynamics* (Springer 2013):** the single best textbook covering FD, LWR, CTM, IDM, MOBIL and calibration. [Springer](https://www.springer.com/gb/book/9783642324598)
2. **Alvarez Lopez et al., "Microscopic Traffic Simulation using SUMO" (ITSC 2018):** SUMO architecture and capabilities. [DLR elib](https://doi.org/10.1109/ITSC.2018.8569938), doi:10.1109/ITSC.2018.8569938
3. **Treiber, Hennecke & Helbing, IDM (Phys. Rev. E 2000).** [arXiv](https://arxiv.org/abs/cond-mat/0002177)
4. **Kesting, Treiber & Helbing, MOBIL (TRR 2007).** [SAGE](https://journals.sagepub.com/doi/abs/10.3141/1999-10)
5. **Daganzo, Cell Transmission Model (Transp. Res. B 1994; Part II 1995).** [Part I](https://ideas.repec.org/a/eee/transb/v28y1994i4p269-287.html), [Part II](https://ideas.repec.org/a/eee/transb/v29y1995i2p79-93.html)
6. **Varaiya, Max-pressure control (Transp. Res. C 2013).** [doi](https://doi.org/10.1016/j.trc.2013.08.014)
7. **Wei, Zheng, Gayah & Li, Survey on Traffic Signal Control Methods (2019).** [arXiv](https://arxiv.org/abs/1904.08117)
8. **Ault & Sharon, RESCO benchmarks (NeurIPS 2021):** realistic RL evaluation. [NeurIPS](https://datasets-benchmarks-proceedings.neurips.cc/paper/2021/hash/f0935e4cd5920aa6c7c996a5ee53a70f-Abstract-round1.html)
9. **FHWA Traffic Analysis Toolbox Vol. III (2019 update):** calibration/validation practice. [FHWA](https://ops.fhwa.dot.gov/publications/fhwahop18036/chapter5.htm)
10. **FHWA Traffic Signal Timing Manual (ch. 3):** saturation flow, lost time and timing fundamentals. [FHWA](https://ops.fhwa.dot.gov/publications/fhwahop08024/chapter3.htm)
11. *(bonus)* **TRB Circular E-C153, DTA: A Primer (2011).** [PDF](https://onlinepubs.trb.org/onlinepubs/circulars/ec153.pdf)
12. *(bonus)* **Mei et al., LibSignal (2023):** cross-simulator evaluation pitfalls. [arXiv](https://arxiv.org/abs/2211.10649)

---

## References

### Traffic flow theory and models
- Daganzo, C.F. (1994). The cell transmission model: A dynamic representation of highway traffic consistent with the hydrodynamic theory. *Transp. Res. B* 28(4):269–287. https://ideas.repec.org/a/eee/transb/v28y1994i4p269-287.html
- Daganzo, C.F. (1995). The cell transmission model, part II: Network traffic. *Transp. Res. B* 29(2):79–93. https://ideas.repec.org/a/eee/transb/v29y1995i2p79-93.html
- Gipps, P.G. (1981). A behavioural car-following model for computer simulation. *Transp. Res. B* 15(2):105–111. https://ideas.repec.org/a/eee/transb/v15y1981i2p105-111.html
- Gipps, P.G. (1986). A model for the structure of lane-changing decisions. *Transp. Res. B* 20(5):403–414. https://ideas.repec.org/a/eee/transb/v20y1986i5p403-414.html
- Greenshields, B.D. et al. (1935). A study of traffic capacity. *HRB Proceedings* 14:448–477. https://trid.trb.org/view/120649
- Continuous formulations and analytical properties of the link transmission model. https://arxiv.org/abs/1405.7080
- Kesting, A., Treiber, M., Helbing, D. (2007). General lane-changing model MOBIL for car-following models. *TRR* 1999:86–94. https://journals.sagepub.com/doi/abs/10.3141/1999-10
- Kesting, A., Treiber, M., Helbing, D. (2010). Enhanced intelligent driver model to access the impact of driving strategies on traffic capacity. https://arxiv.org/abs/0912.3613
- Kesting, A., Treiber, M. (2008). Calibrating car-following models using trajectory data: Methodological study. *TRR* 2088:148–156. https://arxiv.org/abs/0803.4063
- Krauß, S. (1998). Microscopic Modeling of Traffic Flow: Investigation of Collision Free Vehicle Dynamics. PhD thesis, Univ. Cologne / DLR. https://elib.dlr.de/8380/
- Kurtc, V., Treiber, M. Calibrating the local and platoon dynamics of car-following models on the reconstructed NGSIM data. https://arxiv.org/abs/1610.03124
- Lighthill, M.J., Whitham, G.B. (1955). On kinematic waves II. *Proc. R. Soc. A* 229:317–345. https://doi.org/10.1098/rspa.1955.0089
- Newell, G.F. (2002). A simplified car-following theory: a lower order model. *Transp. Res. B* 36(3):195–205. https://ideas.repec.org/a/eee/transb/v36y2002i3p195-205.html
- Richards, P.I. (1956). Shock waves on the highway. *Oper. Res.* 4(1):42–51. https://doi.org/10.1287/opre.4.1.42
- Treiber, M., Hennecke, A., Helbing, D. (2000). Congested traffic states in empirical observations and microscopic simulations. *Phys. Rev. E* 62:1805. https://arxiv.org/abs/cond-mat/0002177
- Treiber, M., Kesting, A. (2013). *Traffic Flow Dynamics: Data, Models and Simulation*. Springer. https://www.springer.com/gb/book/9783642324598
- Treiber, M. IDM information page (traffic-simulation.de). https://traffic-simulation.de/info/info_IDM.html
- Wiedemann, R. (1974). *Simulation des Straßenverkehrsflusses*. Univ. Karlsruhe. https://swov.nl/en/publication/simulations-des-strassenverkehrsflusses
- Zhou et al. (2025). Twenty-five years of the Intelligent Driver Model. https://arxiv.org/abs/2506.05909
- Car-following models: a multidisciplinary review (equation reference). https://arxiv.org/pdf/2304.07143
- Ni, Y.-C., Akopian, A., Kouvelas, A., Makridis, M.A. (2026). Revisiting mesoscopic traffic flow simulation in SUMO. https://arxiv.org/abs/2606.09282

### Intersections, signals, capacity
- TRB (2022). *Highway Capacity Manual, 7th Edition*. https://nap.nationalacademies.org/26432
- FHWA (2008). *Traffic Signal Timing Manual*, ch. 3. https://ops.fhwa.dot.gov/publications/fhwahop08024/chapter3.htm
- NCHRP Report 672 (2010). *Roundabouts: An Informational Guide, 2nd ed.* https://www.nap.edu/catalog/22914/roundabouts-an-informational-guide-second-edition
- PTV Visum help: HCM roundabouts; two-way stop nodes. https://cgi.ptvgroup.com/vision-help/VISUM_2025_ENG/Content/1_Benutzermodell%20IV/1_5_Vorfahrtsgeregelte%20Knoten.htm
- HCM formula summaries (TWSC, roundabouts). https://open-exam-prep.com/study-guides/ptoe/managed-lanes-tsmo-its/unsignalized-intersections-roundabouts
- Webster cycle length (U. Idaho NIATT lab manual). https://www.webpages.uidaho.edu/niatt_labmanual/Chapters/signaltimingdesign/theoryandconcepts/CycleLengthDetermination.htm
- Akgungor, analytical delay models for signalized intersections. https://nacto.org/wp-content/uploads/analytical_delay_models_for_signalized_intersections_akgungor.pdf
- Erdmann, J., Krajzewicz, D. (2014). SUMO's road intersection model. LNCS 8594, doi:10.1007/978-3-662-45079-6_1. https://elib.dlr.de/93669

### Signal control
- Hunt, P.B., Robertson, D.I., Bretherton, R.D., Winton, R.I. (1981). SCOOT – a traffic responsive method of coordinating signals. TRRL LR1014. https://www.trl.co.uk/uploads/trl/documents/LR1014.pdf
- Sims, A.G. (1979). The Sydney Co-ordinated Adaptive Traffic (SCAT) System. ARRB. https://trid.trb.org/View/147832
- Varaiya, P. (2013). Max pressure control of a network of signalized intersections. *Transp. Res. C* 36:177–195. https://doi.org/10.1016/j.trc.2013.08.014
- Kouvelas, A. et al. (2014). Max-pressure signal control variant, TRR (ETH Research Collection). https://www.research-collection.ethz.ch/bitstream/20.500.11850/275877/4/Kouvelas_EtAl_TRR_2014.pdf
- Wei, H. et al. (2019). PressLight. KDD'19. https://jhc.sjtu.edu.cn/~gjzheng/paper/kdd2019_presslight/kdd2019_presslight_paper.pdf
- Wei, H. et al. (2019). CoLight. CIKM'19. https://arxiv.org/abs/1905.05717
- Wei, H., Zheng, G., Gayah, V., Li, Z. (2019). A survey on traffic signal control methods. https://arxiv.org/abs/1904.08117
- Ault, J., Sharon, G. (2021). Reinforcement learning benchmarks for traffic signal control (RESCO). NeurIPS D&B. https://datasets-benchmarks-proceedings.neurips.cc/paper/2021/hash/f0935e4cd5920aa6c7c996a5ee53a70f-Abstract-round1.html
- Mei, H. et al. (2023). LibSignal: An open library for traffic signal control. *Machine Learning*. https://arxiv.org/abs/2211.10649
- Da, L., Mei, H., Sharma, R., Wei, H. (2023). Uncertainty-aware grounded action transformation (UGAT). IEEE CDC. https://arxiv.org/abs/2307.12388
- Raeis, M. et al. (2021). A deep RL approach for fair traffic signal control. https://arxiv.org/abs/2107.10146
- Wan et al. (2024). Fair and efficient traffic light control with RL. https://scholars.cityu.edu.hk/en/publications/fair-and-efficient-traffic-light-control-with-reinforcement-learn/
- sumo-rl (Alegre). https://github.com/lucasalegre/sumo-rl

### Demand and assignment
- Wardrop, J.G. (1952). Some theoretical aspects of road traffic research. *Proc. ICE* 1(3):325–362. https://doi.org/10.1680/ipeds.1952.11259
- Daganzo, C.F., Sheffi, Y. (1977). On stochastic models of traffic assignment. *Transp. Sci.* 11(3):253–274. https://doi.org/10.1287/trsc.11.3.253
- Sheffi, Y. (1985). *Urban Transportation Networks*. https://sheffi.mit.edu/sites/sheffi.mit.edu/files/sheffi_urban_trans_networks_0.pdf
- Gawron, C. (1998). An iterative algorithm to determine the dynamic user equilibrium in a traffic simulation model. *IJMPC* 9(3):393–407. https://ideas.repec.org/a/wsi/ijmpcx/v09y1998i03ns0129183198000303.html
- Chiu, Y.-C. et al. (2011). *Dynamic Traffic Assignment: A Primer*. TRB Circular E-C153. https://onlinepubs.trb.org/onlinepubs/circulars/ec153.pdf
- McNally, M.G. The four step model. UCI-ITS-WP-00-17. https://escholarship.org/uc/item/1zb9n444

### Simulators and tools
- Alvarez Lopez, P. et al. (2018). Microscopic traffic simulation using SUMO. IEEE ITSC, doi:10.1109/ITSC.2018.8569938. https://doi.org/10.1109/ITSC.2018.8569938
- SUMO documentation: vehicle types https://sumo.dlr.de/docs/Definition_of_Vehicles%2C_Vehicle_Types%2C_and_Routes.html · car-following https://sumo.dlr.de/docs/Car-Following-Models/index.html · traffic lights https://sumo.dlr.de/docs/Simulation/Traffic_Lights.html · MESO https://sumo.dlr.de/docs/Simulation/Meso.html · DUA https://sumo.dlr.de/docs/Demand/Dynamic_User_Assignment.html · routes from counts https://sumo.dlr.de/docs/Demand/Routes_from_Observation_Points.html · OSM import https://sumo.dlr.de/docs/Networks/Import/OpenStreetMap.html · OSMWebWizard https://sumo.dlr.de/docs/Tutorials/OSMWebWizard.html · libsumo https://sumo.dlr.de/docs/Libsumo.html · emissions https://sumo.dlr.de/docs/Models/Emissions.html · marouter https://sumo.dlr.de/docs/marouter.html
- Wagner, P., Erdmann, J. (2025). SUMO's interpretation of the Krauß model. SUMO Conf. Proc. 6. https://doi.org/10.52825/scp.v6i.2638
- Erdmann, J. (2014). LC2013 presentation, SUMO2014. https://eclipse.dev/sumo/documents/2014/Presentation_LC2013_JakobErdmann.pdf
- Clemente, M.L. (2022). Building a real-world traffic micro-simulation scenario from scratch with SUMO. SUMO Conf. Proc. 3. https://doi.org/10.52825/scp.v3i.109
- Codecà, L., Frank, R., Engel, T. (2015). LuST: 24 hours of mobility. IEEE VNC. https://orbilu.uni.lu/handle/10993/23011
- Zhang, H. et al. (2019). CityFlow. WWW'19 demo. https://arxiv.org/abs/1905.05217
- Horni, A., Nagel, K., Axhausen, K.W. (eds.) (2016). *The Multi-Agent Transport Simulation MATSim*. Ubiquity Press. http://dx.doi.org/10.5334/baw
- Barceló, J. (ed.) (2010). *Fundamentals of Traffic Simulation*. Springer. https://www.springer.com/us/book/9781441961419
- Fellendorf, M., Vortisch, P. (2010). Microscopic traffic flow simulator VISSIM. In Barceló (ed.), pp. 63–93. https://www.doi.org/10.1007/978-1-4419-6142-6_2
- PTV Vissim help: Wiedemann 74 / 99 parameters. https://cgi.ptvgroup.com/vision-help/VISSIM_2025_ENG/Content/4_BasisdatenSim/FahrverhaltensparameterFolgeverh_Wied99.htm
- Aimsun Next: Hybrid meso–micro simulator. https://docs.aimsun.com/next/24.0.0/UsersManual/HybridSimulator.html
- Dosovitskiy, A. et al. (2017). CARLA: An open urban driving simulator. CoRL. https://arxiv.org/abs/1711.03938

### Calibration, validation, data
- FHWA (2019). *Traffic Analysis Toolbox Vol. III (update)*, ch. 5. https://ops.fhwa.dot.gov/publications/fhwahop18036/chapter5.htm
- FHWA (2004). *Traffic Analysis Toolbox Vol. II: Decision Support Methodology*. https://ops.fhwa.dot.gov/trafficanalysistools/tat_vol2/index.htm
- UK DfT. TAG Unit M3.1 Highway Assignment Modelling. https://assets.publishing.service.gov.uk/media/6a033d074fb0713aa63ea802/tag-m3-1-highway-assignment-modelling.pdf
- TxDOT Microsimulation manual, acceptability criteria. https://www.txdot.gov/manuals/des/tsp/chapter-13-microsimulation-analysis/13-5-calibration/13-5-2-acceptability-criteria/13-5-2-4-fhwa-traffic-analysis-toolbox--volume-iii.html
- Hollander, Y., Liu, R. (2008). The principles of calibrating traffic microsimulation models. *Transportation* 35(3):347–362. https://iaorifors.com/paper/62434
- W-SPSA in practice: approximation of weight matrices and calibration of traffic simulation models (ISTTT 2015 preprint). https://web.mit.edu/cami/Public/CLA_WSPSAinPractice_ISTTT_2015_preprint.pdf
- Osorio, C. Dynamic OD calibration (metamodel). https://web.mit.edu/osorioc/www/papers/osoDynamicOD.pdf
- TomTom Traffic Stats. https://docs.tomtom.com/traffic-stats/documentation/product-information/introduction ; O/D Analysis https://docs.tomtom.com/od-analysis/documentation/product-information/introduction
- Google Maps Routes API, traffic options. https://developers.google.com/maps/documentation/routes/traffic-opt

### Emissions
- HBEFA — Handbook Emission Factors for Road Transport. https://hbefa.net ; HBEFA 5.1 release note https://www.infras.ch/en/news/2025/10/23/comprehensive-update-of-the-handbook-of-emission-factors-for-road-transport/
- EMEP/EEA Air Pollutant Emission Inventory Guidebook 2023, 1.A.3.b Road transport (COPERT methodology). https://www.eea.europa.eu/publications/emep-eea-guidebook-2023/part-b-sectoral-guidance-chapters/1-energy/1-a-combustion/1-a-3-b-i
