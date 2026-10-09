# Iași, Romania: real-world traffic infrastructure study

*Research for STREETFLOW: a traffic simulator for testing a traffic management system on Iași's real infrastructure.*
*Compiled 2026-10-09. Every factual claim links to a source in [References](#references). Items that could not be confirmed are marked **(unverified)**. Where sources disagree, both values are given.*

---

## Table of contents

1. [Executive summary](#1-executive-summary)
2. [City overview](#2-city-overview)
   - 2.1 Population and metropolitan area
   - 2.2 Geography and urban structure
   - 2.3 Main arteries, national roads and bypasses
   - 2.4 Chronic bottlenecks and congestion hotspots
   - 2.5 Measured traffic volumes at key intersections
   - 2.6 Commuter flows from the metropolitan ring
   - 2.7 Traffic rankings
3. [Traffic management infrastructure (ITS)](#3-traffic-management-infrastructure-its)
4. [Public transport (CTP Iași)](#4-public-transport-ctp-iași)
5. [Planning documents: PMUD Iași](#5-planning-documents-pmud-iași)
6. [Data availability for a simulator](#6-data-availability-for-a-simulator)
7. [Road works and planned projects](#7-road-works-and-planned-projects)
8. [Typical intersection types in Iași](#8-typical-intersection-types-in-iași)
9. [Implications for the simulator](#9-implications-for-the-simulator)
10. [Open questions and unverified items](#10-open-questions-and-unverified-items)
11. [References](#references)

---

## 1. Executive summary

- **City size.** Iași municipality had **271,692** residents at the 2021 census, down from 290,422 in 2011 [R2]. Fast-growing suburban communes such as Miroslava (28,534) and Valea Lupului (14,510, nearly three times its 2011 population) now send heavy commuter flows into a radial street network that sits on hills [R2][R1].
- **Signal control.** An **adaptive urban traffic control (UTC) system** paid for with EU money (POR 2007–2013) was finished in **March 2016**. It covers **88 intersections** (planned as about 90) with **1,242 signal heads**, video and inductive detection, a fiber-optic backbone and a Traffic Management Center (Centrul/Biroul de Management al Traficului) [R1][R10][R9]. The contractor was the UTI–IBC association [R12]. The project came with a corruption case and a payment dispute [R11][R12]. For years many intersections ran on old fixed plans. Real-time adaptive operation was only switched on at **Podu Roș on 9 July 2026**, with **Bd. Dimitrie Cantemir** next [R13][R14].
- **Worst hotspot.** **Podu Roș** and the **CUG–Nicolina–Podu Roș** corridor are the worst area. The single busiest counted intersection is **Bd. Nicolae Iorga – Bd. Socola – Bd. Primăverii** ("Cotnari"), with about **1.6 M vehicles in 39 days**, roughly 41,000 per day [R7].
- **Public transport.** CTP Iași runs **9 metre-gauge tram routes and 43 bus routes**. This was checked against the current GTFS feed [R34]. Trams share the roadway on many arterials. One bus/tram-only lane exists on Nicolina (since 2021) [R18]. Transit signal priority was announced in 2021 as operator-requested via the Traffic Management Center, not automatic [R18].
- **Planning model.** The PMUD (Sustainable Urban Mobility Plan, 2017 update) built a **PTV VISUM** 4-step macro model. It has **254 zones in the Iași growth pole, 194 of them in the city, and 297 zones in total**. Base year 2014, horizons 2020 and 2030. Inputs were a 1% household survey, 29 count stations and 5 roadside O-D stations [R1].
- **Open data.** A **GTFS feed** is public and current, valid through 2026-12-31 [R33][R34]. Real-time vehicle positions are available through the **Tranzy open-data API** [R35][R36]. OSM coverage is good for lanes and speed limits but poor for turn lanes. No signal timing plans or loop/camera counts are published.

## 2. City overview

### 2.1 Population and metropolitan area

| Unit | 2011 census | 2021 census | Notes |
|---|---|---|---|
| Iași municipality | 290,422 | 271,692 | Third-largest city in Romania [R2][R3] |
| Miroslava (SW) | 11,958 | 28,534 | [R2] |
| Ciurea (S) | 11,640 | 17,254 | [R2] |
| Valea Lupului (W) | 4,982 | 14,510 | [R2] |
| Holboca (E) | 11,971 | 13,697 | [R2] |
| Tomești (E) | 11,051 | 12,169 | [R2] |
| Rediu (N) | 4,577 | 8,295 | [R2] |
| Bârnova (S) | 5,782 | 7,913 | [R2] |
| Popricani (N) | 7,393 | 7,236 | [R2] |
| Lețcani (W) | 6,497 | 6,845 | [R2] |

- The Iași Metropolitan Zone (ZMI) covered by the PMUD is the municipality plus 18 communes: Aroneanu, Bârnova, Ciurea, Comarna, Holboca, Lețcani, Miroslava, Mogoșești, Movileni, Popricani, Prisăcani, Rediu, Schitu Duca, Tomești, Țuțora, Ungheni, Valea Lupului and Victoria [R1].
- Metropolitan population figures depend on the definition used. Wikipedia gives about **423,154** for the metro area and **93.9 km²** for the municipality [R3]. Population counted by *domicile* (registered address) is much higher: 416,613 for the municipality and 585,448 for the metropolitan zone in Feb 2022 [R4]. Daytime traffic demand is therefore much larger than the census resident count suggests. That includes students and residents registered elsewhere [R4].
- Car ownership (2013): **268 cars per 1,000 inhabitants** in the municipality and 228 in the growth pole, against a national average of 224 [R1].

### 2.2 Geography and urban structure

- The city spans about **320 m of elevation** between the Bahlui valley floodplain and the surrounding hills. It is traditionally said to sit on **seven hills**: Breazu, Bucium, Cetățuia, Copou, Galata, Repedea and Șorogari [R3]. The PMUD describes a **radial street grid constrained by topography**. This grid served the pre-industrial city but is now overloaded [R1].
- The **Bahlui river** crosses the city W→E south of the centre. Its embankment roads (Splai Bahlui Mal Stâng and Mal Drept) are key E–W links next to Podu Roș [R15][R17].
- The tram network includes one of the steepest adhesion tram grades in Europe: **8.8%** between Târgu Cucu and Bd. Tudor Vladimirescu [R32]. Gradients matter for vehicle dynamics in a simulator.
- The PMUD notes that capacity along major arterials is **discontinuous**. Causes include changes in cross-section, legal and illegal kerbside parking, and frequent permitted left turns into side streets [R1].
- Main districts on the arterial network include Centru, Copou (N), Păcurari (NW), Alexandru cel Bun and Dacia (W), Nicolina and CUG (SW/S), Socola and Bucium (S/SE), Tătărași (E) and Tudor Vladimirescu (E). These names appear throughout the sources [R1][R7][R22].

### 2.3 Main arteries, national roads and bypasses

- **National roads:** DN28 / E583 (west towards Lețcani and Târgu Frumos; east towards Tomești and the Moldova border), DN24 / E583 (south towards Vaslui via Bucium, and north), DN24C, and **DN28D**, which is a W–SW heavy-traffic bypass ("varianta de ocolire") linking DN28 and DJ248A [R1].
- **Urban arterials** are category I (6 lanes), II (4 lanes) and III [R1]. Arterials named in the sources include:
  - **E–W:** Șos. Păcurari → Bd. Independenței / Bd. Carol I → Târgu Cucu → Tătărași; Bd. Alexandru cel Bun and Bd. Dacia; Bd. Chimiei and Calea Chișinăului to the east [R1][R7].
  - **N–S:** Copou (Bd. Carol I) → Centre → Bd. Ștefan cel Mare → Bd. Independenței / Str. Palat / Elena Doamna → **Podu Roș** → Bd. Nicolae Iorga / Bd. Socola → Bucium (DN24), and Podu Roș → **Șos. Nicolina** → CUG [R7][R18][R22].
- **Overpasses and underpasses ("pasaje")** carry arterials over the railway and the Bahlui. Examples include Pasajul Alexandru cel Bun (6 lanes), Pasajul Socola/Bucium, Pasajul Nicolina, Pasajul Octav Băncilă (Păcurari) and Pasajul Mihai Eminescu [R1][R26][R19].
- **No full ring road exists yet.** The PMUD notes the lack of a northern bypass and therefore no alternative route for freight in the north [R1]. Planned bypasses are covered in §7.

### 2.4 Chronic bottlenecks and congestion hotspots

| Hotspot | Evidence | Source |
|---|---|---|
| **Podu Roș** (roundabout + tram junction at Bd. Socola / N. Iorga / Nicolina / Țuțora / Bahlui embankments) | Described as the city's most congested area, with queues in both directions from the periphery at peak. Signals added inside the roundabout in 2020. Adaptive signals activated 9 Jul 2026. Removing the roundabout and building a signalized junction or underpasses has been debated | [R15][R16][R17][R13] |
| **CUG – Nicolina – Podu Roș** corridor | "Near-total blockage" at morning peak (Sept 2025). Bus/tram lane since 2021. Left-turn conflicts with trams | [R24][R18][R30] |
| **Bucium entrance (DN24) / Pasaj Socola** | Main entry from the south. Queues of up to **7 km** during the 2026 overpass rehabilitation | [R22][R26] |
| **Păcurari exit to Valea Lupului (DN28 W)** | Queues of several km from the Păcurari roundabout to the ERA roundabout, 50 min for a short distance. Moara de Foc junction flagged | [R23][R22] |
| **Tomești (DN28 E / Calea Chișinăului)** | Long queues starting at the Penny store area | [R24][R22] |
| **Bd. C.A. Rosetti** | Proposed widening by one lane | [R22] |
| **Copou – Agronomie roundabout – Aleea Sadoveanu (to Breazu/Rediu)** | Called the city's "most congested roundabout" in 2021. Lane widening over 3.3 km planned | [R44][R45][R21] |
| **Elena Doamna – Str. Palat – Podu Roș** | At peak, buses and trams take 30–40 min from Elena Doamna to the first Podu Roș stop. A planned bus lane was dropped for lack of width | [R20] |
| **Tudor Vladimirescu** | Bumper-to-bumper traffic, especially during tram-track works | [R51] |
| **Network-wide gridlock** | 13–14 Nov 2023: 2 h from Păcurari to Podu Roș. Peak periods described as 7–9 and 15–18 | [R25] |

The PMUD measured the **peak hour in the afternoon (15:00–18:00)**. The single peak hour carries about **7.6% of daily traffic** [R1].

Some hotspots from the brief were **not specifically confirmed** in sources found during this study: Gara (railway station square), Piața Unirii, Podul de Fier / Bularga and Tătărași as standalone congestion points. Gara and Tătărași do appear as intermodal hubs and tram nodes [R1], and Tătărași as a one-way reorganization area [R19]. **(unverified as congestion hotspots)**

### 2.5 Measured traffic volumes at key intersections

The Traffic Management Bureau counted vehicles with its camera system at 8 intersections, 1 Dec 2020 – 8 Jan 2021 (39 days, winter, partly during the pandemic) [R7]:

| Intersection | Vehicles (39 days) | Peak day |
|---|---|---|
| Bd. Nicolae Iorga – Bd. Socola – Bd. Primăverii ("Cotnari") | 1,605,051 | 55,572 (18 Dec) |
| Bd. Nicolae Iorga – Str. Hatman Șendrea (pasaj) | 1,245,325 | 43,820 |
| Bd. Independenței – Str. Mihai Eminescu (– Brătianu) | 1,035,320 | 43,573 |
| Bd. Carol I – Str. Toma Cozma (– Berthelot) | 846,768 | 29,967 |
| Str. Palat – Str. Sf. Andrei | 838,211 | 19,409 |
| Șos. Nicolina – Bd. Poitiers (Rond Vechi, CUG) | 698,995 | 11,134 (as reported) |
| Bd. Alexandru cel Bun – Piața Voievozilor (Str. Olt) | 636,242 | 21,389 |
| Șos. Păcurari – Cimitirul Evreiesc (Aleea Păcurari/Popăuți) | 514,298 | 10,627 |
| **Total** | **7,420,210** | |

Pre-pandemic baseline: in the week of 13–19 Jan 2020 these 8 intersections carried **2,261,750** vehicles, of which **474,125** used the Cotnari junction (about 68,000 per day). Volumes fell by about 60% during the April 2020 lockdown [R8]. These are the **only intersection-level counts found in public sources**. They are useful for calibrating demand at boundary and core nodes.

### 2.6 Commuter flows from the metropolitan ring

| Corridor | Origin communes | Entry arterial | Evidence |
|---|---|---|---|
| West (DN28/E583) | Valea Lupului, Lețcani | Șos. Păcurari (Rond Păcurari, ERA, Moara de Foc) | [R23][R21] |
| South-west | Miroslava, Ciurea | CUG – Șos. Nicolina – Podu Roș | [R24][R21] |
| South (DN24) | Bârnova, Ciurea, Bucium | Bucium – Pasaj Socola – Podu Roș | [R26][R22] |
| East (DN28/E583) | Tomești, Holboca | Calea Chișinăului – Tudor Vladimirescu | [R24][R22] |
| North | Rediu (Breazu), Popricani | Aleea Sadoveanu – Copou; DN28/DN24 north | [R45][R21] |

The PMUD ran **roadside O-D surveys at 5 sections** in June 2015, recording inbound flows 07:00–19:00 with a 2–4% sample. Average car occupancy was **1.58** persons [R1]. Its zoning covers everything within about 40 min drive [R1].

### 2.7 Traffic rankings

- **TomTom Traffic Index:** the Romania country page lists **only Bucharest** (2025: 62.5% congestion, 171 h lost) [R5]. **Iași is not covered by the TomTom Traffic Index** in the pages found. A guessed city-page URL returned 404.
- **Storia T.R.A.I. index (2024):** Iași scored **15.4/100** (lower means more traffic). That ranks it the **second most congested** of the Romanian cities analysed, after Baia Mare. The index uses Google Traffic, Google Places, Airly and Storia data [R6].

## 3. Traffic management infrastructure (ITS)

### 3.1 The SMT/UTC project (POR 2007–2013)

| Item | Value | Source |
|---|---|---|
| Program | POR 2007–2013 (EU) | [R1] |
| Contractor | UTI–IBC association (UTI Grup) | [R12][R11] |
| Completion | March 2016 | [R10][R12] |
| Intersections | 88 (PMUD: "90 intersections and signalized pedestrian crossings") | [R10][R9][R1] |
| Signal heads | 1,242 | [R10] |
| Cameras | 59 PTZ cameras at 59 intersections. The center can also access 179 Local Police and 250 Salubris cameras | [R9] |
| Detection | Mostly video, some inductive loops | [R1] |
| Control modes | **Local adaptive "micro-regulation"** (phase demand/green adjustment at the controller) plus **centralized "macro-regulation"** from the central post | [R1] |
| Communications | Fiber optic. Disruptions during roadworks are reported | [R10][R1] |
| Cost | Reported as about 60 M lei, about 70 M lei, about 90 M lei or "€20 M" depending on the source | [R12][R9][R11][R13] |
| Vendor of controllers/central software | **Not stated** in any source. The contractor said it used the "same technology as Vienna, Prague, Warsaw, Berlin" | [R12] **(unverified)** |

- **Earlier CIVITAS intersections.** Before the SMT, intersections on the axis **Bd. Carol I – Bd. Independenței – Str. Elena Doamna – Bd. Tudor Vladimirescu** were refitted under **CIVITAS** with new controllers (ADCs), GPRS modules and sensors. Integrating them into the SMT required extra loops or video and communications equipment [R1].
- **Non-SMT signals.** Other signals installed in parallel are often **home-made controllers** that cannot connect centrally or run adaptive control. They are mounted on lamp posts, guy wires or old trolleybus poles and wired overhead. The PMUD recommends re-equipping them and bringing them into the SMT [R1].
- **Operating history.** Testing in autumn 2015 caused severe congestion, with a reported 50 min to cover 500 m [R11]. A court ruling in 2019 found that some intersections, "very important for traffic fluidity", still ran on old timing plans [R12]. Reported failure causes include power cuts, broken fiber, vandalism, crashes, weather and detector errors caused by bad parking or lane indiscipline [R10].
- **Traffic Management Center.** It is staffed **24/7** by municipal staff and Local Police. Operators can restart controllers remotely, dispatch technicians and **adjust signal timings during congestion**. Camera-based counting produced the volumes in §2.5 [R9].

### 3.2 2026: adaptive control finally switched on

- **Podu Roș.** Adaptive signals went live on **9 July 2026**: "signal timings are adapted in real time." Normally the full signalization at Podu Roș runs **only during the morning peak**. During the summer holiday it ran all day. The Traffic Management Center and Local Police watch the queues on each approach and adjust timings [R14]. Problems in the first days required manual adjustments, with fine-tuning planned through September 2026 [R13].
- **Next: Bd. Dimitrie Cantemir.** Three intersections whose signals are off or only partly working: × Bd. Regele Mihai I, × Șos. Națională, × Aleea/Str. Decebal. Other congested areas are under analysis [R13].
- **Responsibility.** The work is led by the City Hall Digitalization Department (director Cătălin Boghiu), which also runs "intelligent traffic management" and crowd-monitoring pilots using 5G, AI and Big Data [R13][R39].

### 3.3 Public-transport priority and green waves

- **Public transport priority.** Since 2021, trams and buses on **Nicolina → Podu Roș** have a dedicated lane. Vehicles are to be equipped with devices that **request** a signal change. The CTP director described this as communicating with the Traffic Management Center to *adjust* timings, **not** a forced green [R18]. In the PMUD, PT priority was a **future** function (project 7.1.1.4, transponders on the full fleet, €416k) [R1].
- **Green waves.** No source confirmed coordinated green-wave corridors in operation. **(unverified)**
- **Planned expansion (PMUD).** Phase 1 would re-equip 15 intersections, add 30 new intersections, add CCTV at 38 and lay 13.5 km of communications ducting (€4.79 M). Phase 2 would add 29 intersections and CCTV at 27 (€2.33 M). Further items are parking guidance (€2.35 M), VMS and cyclist signals [R1]. Implementation status was **not found (unverified)**.

## 4. Public transport (CTP Iași)

### 4.1 Network

- **Operator:** SC Compania de Transport Public Iași (CTP; formerly RATP). The tram network has been electric since 1900, on **1,000 mm gauge** [R32].
- **Current GTFS (verified 2026-10-09):** **52 routes = 9 tram + 43 bus**, **633 stops** in a bounding box of lat 47.065–47.288, lon 27.446–27.757 [R34]. Tram routes in the feed:

| Route | Itinerary (GTFS long name) |
|---|---|
| 1 | Canta – Târgu Cucu – Podu Roș – Tătărași – Canta |
| 3 | Dancu – Tătărași – Tg. Cucu – Gara |
| 5 | Dacia – N. Iorga – Baza 3 – Țuțora |
| 6 | Dacia – Gara – Târgu Cucu |
| 7 | Canta – Tg. Cucu – Podu Roș – Baza 3 – Rond Țuțora |
| 8 | Gara – Târgu Cucu – Tudor Vladimirescu – Țuțora |
| 9 | Gara – Târgu Cucu – Podu Roș – Spital Elytis |
| 11 | Dacia – N. Iorga – Baza 3 – Tătărași Nord |
| 13 | Canta – Târgu Cucu – Tătărași – Podu Roș – Canta |

- **Network length:** sources differ. Wikipedia gives 140 km of track and 449 km of bus routes [R32]. A search snippet citing a local paper gave **82.6 km** and 8 lines with 55 stops **(unverified, likely route-km vs track-km)**.
- **Fleet (Wikipedia, 2025):** about 130 trams, 214 buses and 8 minibuses [R32]. New additions:
  - **16 Pesa** trams (about 30 m, POR Axis IV, delivered 2021) [R38]
  - **16 Bozankaya** trams (POR, last delivered May 2023) [R37]
  - **18 Bozankaya** 20 m trams (PNRR, about €34 M). The first 3 entered service on 1 Oct 2025 [R36]
  - Electric buses, including 25 Anadolu 10 m buses [R36]
- **Ridership:** 50.36 M passengers in 2014, about 140,000 per day [R32].

### 4.2 Trams in the roadway and at signals

- Trams run **in mixed traffic or on unsegregated median tracks** along many arterials. Typical examples are Nicolina, Podu Roș, Bd. Carol I (Copou), Tudor Vladimirescu, Str. Palat and Independenței [R18][R30][R31]. The PMUD calls for a clear policy on dedicated tram tracks and PT priority at signals [R1].
- **Tram signals and conflicts.** At **Rond Vechi (Nicolina × Poitiers, CUG)** and **Copou (Carol I × Toma Cozma)**, tram drivers report daily conflicts with cars turning left across the tracks while the tram has a green/priority aspect [R30]. In 2020 a dedicated signal was added where the tram line crosses Șos. Națională at Podu Roș [R15]. Signals in the PMUD equipment list include **vehicle/pedestrian/tram/cyclist LED heads** and pedestrian/cyclist push-buttons [R1].
- **Dedicated lanes.** The Nicolina bus+tram lane runs from Str. Sălciilor to Podu Roș (2021), with left turns to Aleea Rozelor banned [R18][R19]. The Elena Doamna – Podu Roș lane was abandoned in 2023 because it would need 3 lanes per direction [R20]. In 2026 a Nicolina – CUG → Blocuri Ciurea extension and a lane on Aleea Sadoveanu were proposed [R21].
- **Track works affecting traffic:**
  - Tudor Vladimirescu track rehabilitation, about 2.5 km (2020) [R51]
  - **Bd. Carol I, BCU – Triumf (Copou):** 1,416 m double track plus 120 m single track at the Triumf roundabout, 18 months, 85.6 M lei, PR Nord-Est 2021–2027. Plans also move trams from Str. Palat to Str. Sf. Lazăr [R31]
  - Târgu Cucu section works in July 2026 [R51]
- **Passenger information:** a 2021 headline reported real-time displays at only 30 of 399 stops [R50].

## 5. Planning documents: PMUD Iași

**Document:** *Plan de Mobilitate Urbană Durabilă pentru Polul de Creștere Iași*, final updated version, September 2017. It was prepared for the EBRD-funded national PMUD programme (Lot 2) by a **PTV Transport Consult / Search Corporation** consortium, with the update by Traffic Audit SRL [R1]. Period 2016–2030. The plan itself recommends updating the model at least every 5 years [R1]. **No newer PMUD update was found** in this study. **(unverified)**

### 5.1 Transport model

| Aspect | Detail [R1] |
|---|---|
| Software | **PTV VISUM** (WebTAG / JASPERS compliant) |
| Type | Classic 4-step aggregate model. Multimodal (private + PT) |
| Base year / horizons | 2014 / 2020 and 2030. Average weekday, 24 h and peak-hour matrices |
| Zones | 254 in the growth-pole area (194 in the city per the figure caption; the table lists 191 municipal zones), **297 total** including national and foreign external zones (from the national transport master plan model, MPGTR) |
| Network source | **HERE (Navteq) Q2 2014** GIS: road types, speed limits, restrictions |
| Nodes | Signalized and roundabout node types. "Main nodes" are used for complex junctions so turn impedances are handled |
| Demand | 13 demand strata (person groups × activity pairs). Gravity model with logit utility. Multinomial logit mode choice. Separate urban and rural parameters |
| Assignment | **LUCE** (Linear User Cost Equilibrium) for road traffic. Timetable-based PT assignment |
| PT systems | Bus, trolleybus (historic), **tram**, train, PT-walk |
| Freight | Separate light/heavy goods vehicle model outside VISUM |

### 5.2 Data collection (2014–2015)

- **Traffic counts (Nov 2014, 2 weeks):** 1 automatic 24 h station (Vasile Lupu, volume and speed), 9 manual 12 h stations (07–19, 10 vehicle classes) and 19 manual 6 h stations (07–10, 13–16) [R1].
- **Travel-time runs:** floating-car runs with GPS and video in the AM, inter-peak and PM periods [R1].
- **Household interview survey:** Nov–Dec 2014, CAPI, a **1% population sample**, 24 h trip diaries for everyone over 6 [R1].
- **Roadside O-D:** 5 sections, June 2015, 07–19, inbound only, 2–4% sample [R1].
- **PT occupancy:** counts in June 2015, for example on Șos. Nicolina [R1].
- **Calibration:** count stations arranged in **two cordons**, plus 5 independent validation sections [R1].

### 5.3 Key results

| Indicator | Iași city | Suburbs | Source |
|---|---|---|---|
| Trips/person/day | 1.57 | 1.54 | [R1] Fig. 50 |
| Walk | 42.18% | 57.33% | [R1] Fig. 52 |
| Bicycle | 0.00% | 0.00% | [R1] Fig. 52 |
| Car driver | 10.71% | 7.21% | [R1] Fig. 52 |
| Car passenger | 4.64% | 7.51% | [R1] Fig. 52 |
| Public transport | 42.47% | 27.95% | [R1] Fig. 52 |

- **Trip purposes:** work 35.45%, education 17.99%, shopping 16.18%, business 0.85%, other 29.53% [R1].
- **Sustainable mode share** was 74.6% in 2014. In the do-minimum scenario it falls to 70.3% (2020) and 67.7% (2030) [R1].
- **Vehicle-km per year** (car): 845 M in 2014 → 1,348 M in 2030 (do-minimum) [R1].

These shares come from a 2014 household survey. Car use has very likely grown since then, given the suburban growth in §2.1. **Treat the shares as outdated.**

### 5.4 Scenarios and ITS functions

The PMUD compared **Scenario 1 (base)**, **Scenario 2 ("optimizing existing transport systems")** and **Scenario 3 ("a new mobility management")**. Each scenario includes or excludes ITS functions: intersection CCTV (74 more intersections in all three), **PT prioritization (Scenarios 2 and 3)** and VMS (Scenario 2) [R1]. The plan's network projects include the South light-traffic bypass, the South light-bypass – Sarmisegetuza link with a new Bahlui bridge, Bd. Țuțora (Podu Roș – Calea Chișinăului), a road and tram underpass Păcurari – Alexandru cel Bun (Canta – Strămoșilor), Pasaj Bucium/Socola rehabilitation with a tram link to Gara Socola, and the North bypass (CNADNR) [R1].

## 6. Data availability for a simulator

| Data | Availability | How to get it |
|---|---|---|
| **Road network** | Good. Fetched OSM data for the core (see stats below) | Geofabrik Romania extract [R42], then clip to a bbox. Or the OSM API `map?bbox=` call (≤50,000 nodes, so tile the area) [R43]. Or Overpass (public instances were overloaded during this study) |
| **GTFS schedule** | **Yes**, public, updated 2026-09-20, valid 2025-11-24 to 2026-12-31, includes `shapes.txt` | `https://external.gtfs.ro/iasi/IASI.zip` [R34]. Indexed as MobilityDatabase mdb-2116 [R33] |
| **Real-time PT positions** | Yes, via the Tranzy open-data API (agency "SCTP Iasi"). Free API key | `https://api.tranzy.dev/v1/opendata/docs` [R35]. Register at tranzy.ai [R36] |
| **Traffic counts** | Only press-published camera counts for 8 intersections (2020–21) [R7][R8], plus PMUD 2014 counts as figures and maps [R1] | Request from the City Hall Traffic Management Bureau under **Law 544/2001** (public information requests) |
| **Signal timing plans** | **Not public** | Law 544/2001 request to Primăria Iași (Digitalization Dept. / Traffic Management Bureau). Otherwise estimate by field observation (video timing of cycles) |
| **Open data portals** | data.gov.ro has no municipal traffic datasets. Only **Poliția Locală Iași** publishes there (8 datasets: fines, staffing, call volumes) [R40]. A national street nomenclature for Iași county also exists | data.gov.ro CKAN API |
| **O-D demand** | PMUD VISUM model (not public). TomTom O/D Analysis (commercial) [R41]. Google data is not open | Ask the municipality or ADI ZMI for the VISUM model. Otherwise synthesize demand from counts and GTFS |
| **Speeds / congestion** | TomTom Traffic Stats and Area Analytics (commercial MOVE portal) [R41]. Google Maps typical traffic (visual only) | Commercial license or academic agreement |

### 6.1 OSM coverage check (own analysis)

Data © OpenStreetMap contributors, fetched 2026-10-09 via the OSM API in 12 tiles covering **lat 47.145–47.175, lon 27.570–27.610** (Podu Roș → Piața Unirii/Gara) [R43]:

| Feature | Count in bbox | Comment |
|---|---|---|
| `highway=traffic_signals` nodes | **65** | 33 carry `traffic_signals=*`, 27 `traffic_signals:direction` |
| `highway=crossing` nodes | 531 | 178 `crossing=traffic_signals`, 255 uncontrolled, 22 zebra |
| Tram ways / tram stops | 136 / 44 | Tram network well mapped |
| `railway=tram_level_crossing` / `tram_crossing` | 296 / 116 | Road–tram and pedestrian–tram crossings mapped |
| Main road ways (primary/secondary/tertiary + links) | ~569 | |
| … with `lanes` | ~97% | Excellent |
| … with `maxspeed` | ~94% | Excellent |
| … with `turn:lanes` (any) | ~12% | **Weak**: turn-lane allocation must be added manually |
| Turn-restriction relations | 273 | Good |
| Roundabout ways | 31 | Includes Piața Podu Roș and Piața Mihail Eminescu |
| `lanes:psv` / `busway` | 1 / 0 | **The Nicolina bus/tram lane is essentially not mapped** |

Way counts include ways that cross the tile edges, so they are approximate.

**Practical recipe.**
1. Download `romania-latest.osm.pbf` from Geofabrik.
2. Clip it with `osmium extract -b <lon_min>,<lat_min>,<lon_max>,<lat_max>`.
3. Convert with the simulator's importer. For SUMO: `netconvert --osm-files … --tls.guess-signals --tls.join --junctions.join --ramps.guess`, and import trams through the `railway=tram` ways.
4. Add `turn:lanes`, the Nicolina PT lane and signal groups by hand for the pilot.
5. Map GTFS stops and shapes onto the network (for example SUMO `gtfs2pt.py`).

## 7. Road works and planned projects

| Project | Status (latest found) | Traffic relevance | Source |
|---|---|---|---|
| **A8 "Autostrada Unirii", section 3 Lețcani (DN28) – Iași (DN24)** | Contract signed **29 May 2026** with FCC Construcción, 17.7 km, 46 months (10 design + 36 build), 18 bridges/overpasses, 6 tunnels, junctions at DJ282 and DN24 | A northern motorway bypass. Will move through traffic off DN28/Păcurari | [R28] |
| **Centura Iași Est (eastern bypass)** | Prefeasibility stage (about 30 km: 25 km new plus 5 km on DN24). Connects to A8 near Ungheni. Includes a commuter car park at Bucium and an intermodal hub at Socola | Takes heavy traffic out of Bucium and Podu Roș | [R29] |
| **Southern bypass (light traffic)** | CNAIR design contract (8.175 km) signed in 2021 per a search snippet | Relief for Nicolina/CUG | [R1] (proposal), search snippet **(unverified)** |
| **Pasaj Socola / Bucium rehabilitation** | In works in 2026 (about 18 months, 64 M lei, Anghel Saligny programme). Detour via Trei Fântâni – Gara Socola – Mihail Sturdza – Bucium with new temporary signals. **7 km queues** reported. Financing contract of 51.8 M lei signed earlier | Major disruption on the southern entry | [R26][R27] |
| **Podu Roș redesign** | Overpasses were rejected by the council in 2022. Underpass feasibility studied. Design tender (about €450k) in 2020. Roundabout-to-signals conversion proposed in 2024, with PTV named as a possible consultant | Core of the pilot area | [R17][R16][R46] |
| **Bd. Dimitrie Cantemir signals** | Activation planned after Podu Roș (2026) | In the pilot area | [R13] |
| **Copou tram (BCU – Triumf)** | 18-month works, 85.6 M lei | Lane closures on Bd. Carol I | [R31] |
| **Aleea Sadoveanu widening** | +1 lane over 3.3 km (7 → 9–10 m). PT-only lane proposed. Tender planned for 2026 | Northern commuter corridor | [R45][R21] |
| **Păcurari extra right-turn lane to Valea Lupului**; Octav Băncilă extra lane (Arcu → Păcurari, 1.3 M lei, 2022) | 2026 / 2022 plans | Western exit | [R21] and search snippet |
| **Splai Bahlui one-way pair** | Planned since 2020 (left bank eastbound toward Tudor Vladimirescu, right bank westbound) | Bypasses the Podu Roș roundabout | [R15] |

## 8. Typical intersection types in Iași

1. **Large multi-lane signalized junctions on 4–6 lane boulevards**, often with **tram tracks in the median**. Examples are Cotnari (N. Iorga × Socola × Primăverii), Independenței × Mihai Eminescu, Carol I × Toma Cozma and Alexandru cel Bun × Piața Voievozilor [R7]. They have video/loop detection and SMT controllers [R1].
2. **Signalized roundabouts ("ronduri")** with tram lines through or around them. **Podu Roș** has had signals at entries and inside since 2020 [R15]. Others include Piața Mihail Eminescu, Agronomie (Copou), Tg. Cucu, Tătărași Nord, Rond Dacia/Canta, Rond Copou and Rond Dancu [R1][R44]. In OSM 31 roundabout ways fall inside the core bbox (§6.1).
3. **Tram–road crossings with tram signals and left-turn conflicts** across tracks, for example Rond Vechi CUG and Copou [R30].
4. **Signalized mid-block pedestrian crossings.** OSM shows 178 signalized crossings in the core bbox (§6.1). Many unsignalized crossings on multi-lane roads have no advance warning signs, which the PMUD flags as a safety risk [R1].
5. **Grade-separated overpasses and underpasses** over the railway and the Bahlui, with merges at both ends [R1].
6. **One-way systems** in the centre and neighbourhoods (Tătărași, Copou and others), with more added over time [R19][R15].
7. **Peripheral unsignalized and priority junctions** on DN, DJ and DC roads. The PMUD counts 72 classified-road intersections in the growth pole, only 24 of them engineered and 4 of those roundabouts [R1].
8. **Time-of-day signal operation.** Some intersections run full signalization only at peak (Podu Roș, morning peak) and are otherwise off or flashing amber [R14]. Several Cantemir intersections are switched off or only partly active [R13].

## 9. Implications for the simulator

### 9.1 Pilot area: "Podu Roș – Nicolina – Cantemir" corridor

**Why this area:**
- It is the city's worst-congested node [R15][R16][R24].
- It contains the busiest counted intersection, Cotnari, with about 41k vehicles/day in winter 2020–21 and about 68k/day in Jan 2020 [R7][R8].
- It is the **only place where the real adaptive system went live (July 2026)**, with Cantemir next [R13][R14]. Before/after comparison and realistic integration are therefore possible.
- It has a bus/tram lane with announced PT priority [R18], a roundabout+signal hybrid and dense tram crossings [R15][R30].
- 3 of the 8 counted intersections lie on it: Cotnari, N. Iorga × Hatman Șendrea and Nicolina × Poitiers [R7].

**Bounding box (WGS84):**

| Level | South | West | North | East | Contents |
|---|---|---|---|---|---|
| **Pilot core** | 47.140 | 27.575 | 47.158 | 27.600 | Podu Roș roundabout (47.1509, 27.5877), Cotnari junction / Bd. Socola / Primăverii (≈47.148–47.149, 27.590–27.593), Bd. Dimitrie Cantemir (47.1520, 27.5816), Șos. Nicolina north part, Bahlui embankments |
| **Pilot extended** | 47.130 | 27.565 | 47.170 | 27.610 | Adds Rond Vechi CUG / Bd. Poitiers (47.1339, 27.5780), Tudor Vladimirescu (47.1505, 27.6029), Târgu Cucu (47.1642, 27.5909), Piața Unirii (47.1667, 27.5801), Gara (47.1655, 27.5699) |

Coordinates were geocoded with Nominatim on 2026-10-09.

**Second corridors**, after the pilot:
- E–W **Păcurari – Independenței – Carol I – Târgu Cucu**: the CIVITAS/SMT axis with 3 counted intersections [R1][R7].
- **Bucium/Socola** during the 2026 overpass works, as a real disruption scenario [R26].

### 9.2 Features the simulator must support

1. **Trams as first-class vehicles:**
   - metre-gauge track on shared or median lanes
   - tram-specific signal heads/phases
   - stops in the roadway, often with passengers crossing to median platforms
   - long, double-articulated vehicles (20–30 m) [R38][R36]
   - slow acceleration and steep grades of up to 8.8% [R32]
   - single-track sections, such as Triumf [R31]
   - tram-vs-left-turner conflicts [R30]
2. **Signal control modes that mirror the real SMT:**
   - fixed-time plans
   - **local actuated/adaptive micro-regulation** (green extension, phase skipping driven by video/loop detectors)
   - **central macro-regulation** (plan selection, cycle/offset changes from the center)
   - **manual operator override** from the Traffic Management Center, as done at Podu Roș [R1][R14]
   - **time-of-day on/off and flashing amber** operation [R14][R13]
3. **Signalized roundabouts**: signals on entries and on the circulating carriageway, as at Podu Roș [R15].
4. **PT priority requests** sent from the vehicle to the center, with configurable, non-absolute priority [R18]. Bus/tram-only lanes with enforcement leakage, since fines were issued on Nicolina for lane abuse (headline) [R52].
5. **Detector models** that match the field: video zones and inductive loops, with failure modes such as outages, occlusion from illegal parking and fiber loss [R10]. A system tested only on perfect detectors will overestimate benefits.
6. **Disruption scenarios**: lane closures and detours from track works and overpass rehabilitation [R26][R31], plus incidents and weather (heavy rain is a known cause of PT delays) [R10].
7. **Demand inputs**: AM peak (07–09) and PM peak (15–18, about 7.6% of daily traffic in the peak hour) [R1][R25]. Boundary inflows from the 5 commuter corridors in §2.6. Kerbside parking friction and double parking reduce effective capacity [R1].

### 9.3 Data we can realistically get, and how

| Need | Realistic source | Effort |
|---|---|---|
| Geometry and lanes | OSM extract (good), manual `turn:lanes` fixes from satellite imagery | Low–medium |
| Tram/bus routes and timetables | GTFS from gtfs.ro [R34] | Low |
| Actual PT running times and delays | Tranzy API vehicle positions logged over weeks [R35] | Medium. Also yields **probe speeds** on tram/bus corridors, a free proxy for congestion |
| Demand calibration | 8-intersection camera counts [R7][R8]. Request recent counts (Law 544/2001). Own video counts at pilot nodes | Medium |
| Signal plans | Request from the City Hall. Otherwise measure cycle/phase lengths on site or from video | Medium–high |
| O-D matrix | Synthesize from counts with O-D estimation. Request the PMUD VISUM model from the municipality or ADI ZMI | High |
| Congestion ground truth | TomTom MOVE (commercial) [R41], Google typical traffic (visual), Tranzy probes | Variable |

### 9.4 Likely interface between the traffic management system and the real infrastructure

- **Architecture.** Iași runs a **centralized UTC**: field controllers with local adaptive logic, linked by fiber to the Traffic Management Center, where operators can change timings centrally [R1][R9]. A new traffic management system would most likely sit **at the center level**. It would read detector and CCTV-count data and send **plan-level commands**: cycle length, splits, offsets, plan selection and priority grants. It would not drive lamps directly.
- **Protocol.** The vendor and protocol of the installed central software are **unknown** **(unverified)**. Possible protocols are proprietary vendor APIs, OCIT-O (common in Central Europe, consistent with the "Vienna, Prague, Berlin" claim [R12]) or NTCIP/UTMC. **The STREETFLOW traffic management system should talk to the simulator through an abstract controller API** with operations such as `set_plan`, `set_phase`, `request_priority` and `get_detector_counts`, plus a thin adapter layer. A real deployment could then swap the simulator adapter for the vendor adapter.
- **Operation mode.** Given the 2015 failure history and the cautious 2026 roll-out [R11][R13], a realistic first deployment is **advisory/decision support for Traffic Management Center operators**: recommended plans and what-if simulations. Closed-loop control would come later, intersection by intersection, starting with Podu Roș and Cantemir.
- **Inputs available in production:** SMT detector counts (video/loop), PTZ camera counts, Tranzy/CTP AVL vehicle positions, and PT priority requests from CTP vehicles (once the 2021-announced priority system is fully in place) [R9][R18][R35].

## 10. Open questions and unverified items

- Manufacturer of the SMT controllers and central software, and the protocols they support.
- Exact number of intersections currently running adaptive mode versus fixed plans (2026).
- Whether coordinated green waves exist on any corridor.
- Whether a PMUD update after 2017 exists. The 2017 plan recommends 5-year updates.
- Status of PMUD ITS phases 7.1.1.1 / 7.1.1.2 / 7.1.1.4.
- Official tram network length: 82.6 km vs 140 km track.
- Gara, Piața Unirii, Podul de Fier / Bularga and Tătărași as congestion hotspots (no direct source found).
- Status of the southern bypass design (2021 contract per a search snippet only).

---

## References

All links accessed 2026-10-09.

- **[R1]** PTV TC / Search Corporation / Traffic Audit, *PMUD pentru Polul de Creștere Iași – Raport Final, Actualizare*, Sept 2017 (403 pp.). https://zmi.ro/wp-content/uploads/2026/06/01.PMUD-Iasi-Actualizare-finala_sept_2017.pdf (older mirror, now 404: https://zmi.ro/wp-content/uploads/2022/01/01.PMUD-Iasi-Actualizare-finala_sept_2017.pdf). Sections used: 2.2 (p.57–61), 2.3 (p.62–67), 2.6.3 ITS (p.81–82), 3 Transport model (p.92–111), 4.2 (p.134–136), action plan 2.3.1 (p.253–254), 2.5.1 ITS projects (p.280–282), 6.2.4 (p.186).
- **[R2]** City Population – Iași county (census 2011/2021). https://www.citypopulation.de/en/romania/iasi
- **[R3]** Wikipedia – Iași. https://en.wikipedia.org/wiki/Ia%C8%99i
- **[R4]** BZI, "Județul Iași are peste 1 milion de locuitori…", 22 Feb 2022. https://www.bzi.ro/judetul-iasi-are-peste-1-milion-de-locuitori-explozie-demografica-in-zona-metropolitana-tabel-4384292
- **[R5]** TomTom Traffic Index – Romania. https://www.tomtom.com/traffic-index/country/romania
- **[R6]** Economica.net, Storia T.R.A.I. index, 2 Jun 2024. https://www.economica.net/?p=751243
- **[R7]** BZI, "Lista celor mai aglomerate intersecții din Iași…", 1 Feb 2021. https://www.bzi.ro/lista-celor-mai-aglomerate-intersectii-din-iasi-iata-valorile-de-trafic-calculate-pe-fiecare-zona-din-tot-orasul-peste-74-milioane-de-masini-au-tranzitat-orasul-exclusiv-4118930
- **[R8]** BZI, traffic in main intersections during COVID-19, 21 May 2020. https://www.bzi.ro/exclusiv-cate-masini-au-circulat-prin-principalele-intersectii-din-iasi-in-timpul-pandemiei-de-covid-19-pe-perioada-starii-de-urgenta-traficul-auto-a-scazut-chiar-si-cu-60-la-suta-centralizator-3953433
- **[R9]** BZI, "Locul secret de unde sunt verificați ieșenii…" (Traffic Management Center), 16 Feb 2021. https://www.bzi.ro/locul-secret-de-unde-sunt-verificati-iesenii-toti-anchetatorii-preiau-imaginile-accidentele-rutiere-sunt-surprinse-secunda-cu-secunda-video-in-premiera-4129065
- **[R10]** BZI, "Sistemul de semaforizare din Iași se confruntă cu multiple provocări", 6 Apr 2023. https://www.bzi.ro/sistemul-de-semaforizare-din-iasi-se-confrunta-cu-multiple-provocari-foto-4686264
- **[R11]** Digi24, "Semafoarele 'inteligente' din Iași provoacă haos", 10 Nov 2015. https://www.digi24.ro/regional/digi24-iasi/semafoarele-inteligente-din-iasi-provoaca-haos-456297
- **[R12]** Profit.ro, "Grupul UTI îi cere public primarului Iașului să aprobe plata a 5,3 milioane lei…", 18 Dec 2019. https://profit.ro/profitul-tau/grupul-uti-ii-cere-public-primarului-iasului-sa-aprobe-plata-a-5-3-milioane-lei-pentru-un-proiect-de-management-al-traficului-executat-in-urma-cu-3-ani-explicatia-primariei-19204004
- **[R13]** BZI, "Primăria Iași va activa și semafoarele inteligente de pe bulevardul Dimitrie Cantemir…", Sept 2026. https://www.bzi.ro/primaria-iasi-va-activa-si-semafoarele-inteligente-de-pe-bulevardul-dimitrie-cantemir-specialistii-analizeaza-datele-din-trafic-si-pentru-alte-intersectii-aglomerate-din-oras-5570566
- **[R14]** BZI, "Schimbări în trafic la Podu Roș pe durata vacanței: semafoarele funcționează toată ziua", Jul 2026. https://www.bzi.ro/schimbari-in-trafic-la-podu-ros-pe-durata-vacantei-semafoarele-functioneaza-toata-ziua-5568419
- **[R15]** BZI, "Noi semafoare în rondul din Podu Roș…", 21 Mar 2020. https://www.bzi.ro/veste-importanta-pentru-soferi-noi-semafoare-in-rondul-din-podu-ros-de-maine-circulatia-se-schimba-total-foto-si-schita-3907734
- **[R16]** BZI, "Revoluție în traficul din Iași: autoritățile vor să radă rondul din Podu Roș…", 24 Apr 2024. https://www.bzi.ro/revolutie-in-traficul-din-iasi-autoritatile-vor-sa-rada-rondul-din-podu-ros-ce-ar-urma-sa-apara-in-cea-mai-aglomerata-zona-din-oras-foto-4966525
- **[R17]** BZI, "Pasaje subterane în Podu Roș…", 4 Mar 2022. https://www.bzi.ro/pasaje-subterane-in-podu-ros-primaria-iasi-schimba-proiectul-de-trafic-din-cea-mai-aglomerata-zona-a-orasului-4393893
- **[R18]** BZI, measure for public transport at Podu Roș / Nicolina, 23 Aug 2021. https://www.bzi.ro/surprize-in-lant-pentru-soferii-din-oras-o-noua-modificare-in-traficul-din-iasi-masura-vizeaza-transportul-public-de-calatori-in-cea-mai-aglomerata-intersectie-din-municipiu-4254810
- **[R19]** BZI, "Schimbări majore în traficul din Iași: intersecții semaforizate și noi sensuri unice…", 26 Mar 2021. https://www.bzi.ro/schimbari-majore-in-traficul-din-iasi-intersectii-semaforizate-si-noi-sensuri-unice-in-cartierele-din-municipiu-iata-lista-completa-4154637
- **[R20]** BZI, "Dispare banda unică pentru transportul în comun…", 25 Oct 2023. https://www.bzi.ro/atentie-soferi-dispare-banda-unica-pentru-transportul-in-comun-dintr-o-zona-aglomerata-a-iasului-pentru-a-putea-face-acest-lucru-ar-trebui-sa-existe-cate-3-benzi-pe-sens-foto-4834720
- **[R21]** BZI, "În 2026 se schimbă regulile în traficul din Iași: noi benzi unice și o nouă bandă în zona Păcurari", 2 Feb 2026. https://www.bzi.ro/in-2026-se-schimba-regulile-in-traficul-din-iasi-vor-aparea-noi-benzi-unice-si-o-noua-banda-de-circulatie-in-zona-pacurari-5460622
- **[R22]** BZI, "Un nou plan de fluidizare a traficului în Iași… trei axe principale", 13 May 2022. https://www.bzi.ro/un-nou-plan-de-fluidizare-a-traficului-in-iasi-expert-in-calcul-intra-trei-axe-principale-de-transport-in-municipiu-foto-4450981
- **[R23]** BZI, "Trafic blocat pe Șoseaua Păcurari la ieșirea din Iași". https://www.bzi.ro/trafic-blocat-pe-soseaua-pacurari-la-iesirea-din-iasi-nervii-soferilor-au-ajuns-la-limita-5236394
- **[R24]** BZI, "Trafic blocat în mai multe zone din Iași", 9 Sep 2025. https://www.bzi.ro/trafic-blocat-in-mai-multe-zone-din-iasi-soferii-inarmati-va-cu-rabdare-5345805
- **[R25]** Newsweek România, "Traficul din Iași paralizat la propriu…", Nov 2023. https://newsweek.ro/actualitate/video-traficul-din-iasi-paralizat-la-propriu-coloane-kilometrice-de-masini-si-nervi-la-maxim
- **[R26]** BZI, "Haos în traficul din Bucium din cauza lucrărilor…", 2026. https://www.bzi.ro/haos-in-traficul-din-bucium-din-cauza-lucrarilor-soferii-cer-demisia-lui-mihai-chirica-am-ajuns-la-capatul-rabdarii-daca-acum-facem-40-de-minute-din-septembrie-vom-sta-doua-ore-5578379
- **[R27]** BZI, "Pasajul de la Bucium a primit finanțarea…". https://www.bzi.ro/pasajul-de-la-bucium-a-primit-finantarea-incep-lucrarile-de-reabilitare-a-celei-mai-circulate-pasarele-din-iasi-foto-4773805
- **[R28]** Economedia, A8 section 3 Lețcani – Iași contract (FCC Construcción), 2026. https://economedia.ro/?p=351009
- **[R29]** Economedia, "Centura Iași Est: proiectantul a predat prima variantă a studiului de prefezabilitate…". https://economedia.ro/centura-iasi-est-proiectantul-a-predat-prima-varianta-a-studiului-de-prefezabilitate-pentru-cartierul-bucium-centura-cu-o-lungime-de-30-kilometri-se-va-conecta-cu-autostrada-unirii-a8-langa-u.html
- **[R30]** BZI, "Vatmanii din Iași se confruntă cu mari probleme în intersecția de la Rondul Vechi din CUG…", 25 Mar 2025. https://www.bzi.ro/vatmanii-din-iasi-se-confrunta-cu-mari-probleme-in-intersectia-de-la-rondul-vechi-din-cug-cum-a-fost-surprins-un-sofer-care-voia-sa-vireze-la-stanga-5500513
- **[R31]** BZI, "Se anunță un nou șantier de lungă durată… niciun tramvai nu va mai circula spre Copou". https://www.bzi.ro/se-anunta-un-nou-santier-de-lunga-durata-la-iasi-timp-de-18-luni-niciun-tramvai-nu-va-mai-circula-spre-copou-5334767
- **[R32]** Wikipedia – CTP Iași. https://en.wikipedia.org/wiki/CTP_Ia%C8%99i (also https://en.wikipedia.org/wiki/Trams_in_Ia%C8%99i)
- **[R33]** MobilityDatabase – CTP Iași GTFS (mdb-2116). https://mobilitydatabase.org/feeds/mdb-2116
- **[R34]** CTP Iași GTFS feed (downloaded and inspected 2026-10-09). https://external.gtfs.ro/iasi/IASI.zip
- **[R35]** Tranzy open-data API docs, as referenced by the `tranzy-stats` project. https://api.tranzy.dev/v1/opendata/docs and https://github.com/horace42/tranzy-stats
- **[R36]** Tranzy.ai open data. https://tranzy.ai/opendata. Fleet sources: Economica.net, 18 Bozankaya trams (PNRR), https://www.economica.net/?p=737728; BZI, three new trams in service (1 Oct 2025), https://www.bzi.ro/trei-tramvaie-noi-au-intrat-in-circulatie-la-iasi-ctp-iasi-sunt-parte-din-lotul-de-18-vehicule-moderne-de-20-de-metri-5362188; BZI, 25 Anadolu e-buses, https://www.bzi.ro/ctp-iasi-va-primi-inca-25-de-autobuze-electrice-firma-anadolu-din-turcia-va-livra-masinile-pentru-70-de-milioane-de-lei-4935122
- **[R37]** BZI, last two Bozankaya trams (16-unit POR batch), 3 May 2023. https://www.bzi.ro/ultimele-doua-tramvaie-bozankaya-ajunse-la-iasi-au-intrat-in-probe-acestea-vor-fi-verificate-si-testate-de-o-echipa-de-experti-turci-4706051
- **[R38]** TheMayor.eu, "Iași continues to modernize its public transport" (16 Pesa trams), Apr 2020. https://themayor.eu/en/a/view/iasi-continues-to-modernize-its-public-transport-5201
- **[R39]** The Diplomat Bucharest, interview with Cătălin Boghiu, 3 Oct 2023. https://www.thediplomat.ro/2023/10/03/catalin-boghiu-iasi-city-hall-we-want-to-implement-the-one-app-concept-next-year/
- **[R40]** data.gov.ro – Poliția Locală Iași organization (queried through the CKAN API). https://data.gov.ro/organization/politia-locala-iasi
- **[R41]** TomTom O/D Analysis and MOVE portal documentation. https://docs.tomtom.com/od-analysis/documentation/product-information/introduction ; https://developer.tomtom.com/move-portal/guides/introduction
- **[R42]** Geofabrik – Romania OSM extract. https://download.geofabrik.de/europe/romania.html
- **[R43]** OpenStreetMap API v0.6 `map` call (own analysis, data © OpenStreetMap contributors, ODbL). https://api.openstreetmap.org/api/0.6/map?bbox=27.570,47.145,27.580,47.155 (one of 12 tiles)
- **[R44]** BZI, changes at the Agronomie roundabout ("cel mai aglomerat rond"), 17 May 2021. https://www.bzi.ro/atentie-ieseni-schimbari-importante-in-traficul-din-cel-mai-aglomerat-rond-rutier-din-oras-deciziile-vor-intra-in-vigoare-in-aceste-zile-4188803
- **[R45]** BZI, "Bandă suplimentară de trafic pe Aleea Sadoveanu… 3,3 kilometri". https://www.bzi.ro/banda-suplimentara-de-trafic-pe-aleea-sadoveanu-din-iasi-soseaua-se-va-extinde-pe-33-kilometri-4455931
- **[R46]** BZI, Podu Roș overpasses design tender (about €450,000), Dec 2020. https://www.bzi.ro/primaria-iasi-duce-pasajale-supraterane-din-podu-ros-la-faza-de-proiectare-doua-sensuri-giratorii-vor-fi-infiintate-firmele-de-specialitate-pot-castiga-450-000-de-euro-4054588
- **[R50]** BZI headline: real-time displays at only 30 of 399 stops. https://www.bzi.ro/panourile-care-arata-in-cat-timp-ajunge-un-mijloc-de-transport-in-comun-lipsesc-din-majoritatea-statiilor-din-399-de-statii-din-iasi-doar-in-30-au-fost-montate-4296732
- **[R51]** BZI, Tudor Vladimirescu tram works and traffic: https://www.bzi.ro/au-inceput-lucrarile-la-linia-de-tramvai-din-tudor-vor-fi-gata-pana-la-finalul-anului-3915946 ; https://www.bzi.ro/trafic-infernal-in-tudor-vladimirescu-se-circula-bara-la-bara-galerie-foto-3970964 ; July 2026 interruption: https://www.bzi.ro/noi-lucrari-la-liniile-de-tramvai-din-iasi-circulatia-spre-tudor-vladimirescu-va-fi-intrerupta-5576183
- **[R52]** BZI, fines for drivers using the Nicolina bus lane (headline). https://www.bzi.ro/amenzi-pentru-soferii-din-iasi-care-au-ignorat-banda-unica-din-nicolina-imaginile-i-au-dat-de-gol-foto-4392552
