# Menu Guide — Relief Load-and-Route Planner

A plain-language guide to every menu and option in the program.

---

## The idea in one paragraph

After a disaster, people are staying in **shelters**. A **warehouse** (the depot)
holds **supplies** — food, water, medicine, blankets — each meant for a particular
shelter. A few **trucks** can carry only so much weight, and some **roads** may be
damaged. The program decides **which supplies go on each truck, which shelter each
truck visits, and which road it takes**, then shows how well every shelter was served.

Every supply has an **importance score**:
**shelter urgency (1–10) × how badly the item is needed (1–10)**.
Medicine for a very urgent clinic scores high; blankets for a calm shelter score low.
The program tries to deliver as much total importance as possible.

---

## Main menu

```
1. Load Scenario
2. View Scenario
3. Plan Operation
4. Routes & Road Damage
5. Compare Algorithms (current data)
6. Benchmarks
7. Manual Entry
8. Export Reports
0. Exit
```

The top of the screen always says which disaster is active, or "No scenario loaded".
Options 2, 3, 4, 5 and 8 need a scenario loaded first. Options 6 and 7 work without one.

---

## 1. Load Scenario — "Choose the disaster"

Opens a ready-made disaster situation, or one you wrote yourself.

| Option | What it is |
|---|---|
| **1. Normal operations** | A calm day. 5 shelters, 28 supply items, 4 trucks, all roads open. |
| **2. Capacity shock + road closure** | The same area after a second disaster: several roads destroyed (one shelter is completely cut off), one truck broken, the other trucks carry less, and the stadium has extra evacuees. |
| **3. Large-data stress** | A big region: 40 shelters, 413 supply items, 25 trucks, 60 locations. Shows the program copes with large amounts of data. |
| **4. Load from CSV file** | Load your own disaster file. Type its location, e.g. `data/adversarial_greedy.csv`. |
| **0. Back** | Return to the main menu. |

**You will see:** a line confirming how many shelters, supplies, trucks and locations were read.

**If the file has a mistake** (for example an urgency of 15, or a supply for a shelter
that does not exist), the program names the line with the problem and keeps the
previous disaster loaded.

**The special file `adversarial_greedy.csv`** is a deliberate trick case: one truck,
one shelter, and three items arranged so that the quick planning method makes a
poor choice. Use it to show why careful methods matter.

---

## 2. View Scenario — "Look before deciding"

Only displays information; nothing is changed or calculated.

| Option | What it shows |
|---|---|
| **1. Shelters** | Each shelter's number, name, how many people, urgency (1–10), and its location. |
| **2. Packages** | Each supply item: type, weight, how badly needed, which shelter it is for, and its importance score. |
| **3. Vehicles** | Each truck: how much it can carry and whether it is available. |
| **4. Road network** | Every road: which two locations it joins, its length, and whether it is open or CLOSED. Also shows where the warehouse is. |
| **0. Back** | Return to the main menu. |

---

## 3. Plan Operation — "Make the delivery plan"

Makes one round of deliveries using the method you choose.

| Option | What it means |
|---|---|
| **1. Knapsack DP** | The careful, thorough method. Always finds the best load for each truck. Uses more computer memory when trucks are large. |
| **2. Greedy** | The quick method. Packs the "best value for its weight" items first. Very fast, but can miss a better combination. |
| **3. Branch & Bound** | Another careful method. Also always finds the best load, by smartly ruling out combinations that cannot win. Usually fast, but can become very slow on certain awkward data. |
| **4. Compare all three** | Runs all three methods on the same disaster and shows a scorecard (same as main menu option 5). |
| **0. Back** | Return to the main menu. |

**How the plan is made (options 1–3):**

1. Find the shortest road to every shelter, and which shelters cannot be reached at all.
2. Send the biggest truck first, to the most urgent shelter.
3. Send each next truck to a shelter that has not been served yet, so help is spread out.
4. Fill each truck with the most valuable set of supplies it can carry for that shelter.

**You will see:**

- **Vehicle assignments** — one line per truck: which shelter, how full (e.g. 59/60 kg),
  value carried, distance, the road taken, and the item numbers loaded.
- **Shelter outcomes** — each shelter's urgency, distance, how much it asked for, how
  much it got, and the percentage filled. A cut-off shelter shows **UNREACHABLE**.
- **Totals** — total value delivered, weight carried, items sent and items still waiting.
- **Fairness** — the worst-served shelter's percentage and the average.
- **Cost** — how long the computer took and how much work it did.

*Example (Normal, option 1):* 4 trucks go to 4 different shelters; total value 952;
Market Hall, the least urgent shelter, gets nothing this round because there are
only 4 trucks for 5 shelters.

---

## 4. Routes & Road Damage — "Check journeys and break roads"

| Option | What it does |
|---|---|
| **1. View routes** | Lists each truck's journey: shelter, distance and the locations it passes through. Also lists any shelter that cannot be reached. |
| **2. Close a road** | Pretend a road is destroyed. Type the two locations it joins. The current plan is cleared because it may now be wrong. |
| **3. Reopen a road** | Repair a closed road the same way. |
| **4. Re-plan** | Makes a fresh plan with the last method you used, taking the road changes into account. |
| **5. View road network** | Same as View Scenario → Road network: shows which roads are open or closed. |
| **0. Back** | Return to the main menu. |

**Try this:** load Normal, close the road between locations **4 and 6**, then Re-plan.
The truck to City Stadium now takes a longer detour (13 km becomes 16 km).
Reopen the road and Re-plan: it goes back to 13 km.

If every road to a shelter is closed, that shelter is marked **UNREACHABLE** and no truck is sent.

Road changes last until you load a disaster again (loading starts fresh from the file).

---

## 5. Compare Algorithms — "Scorecard of the three methods"

Runs all three planning methods on the current disaster and puts the results side by side.

| Column | Meaning |
|---|---|
| **Value** | Total importance delivered. Higher is better. |
| **Gap%** | How far behind the best result. 0% means "as good as the best". |
| **Worst%** | How well the worst-off shelter was served. 0% means some shelter got nothing. |
| **Pending** | Supply items still waiting at the warehouse. |
| **Time(ms)** | How long the computer took, in thousandths of a second. |
| **Operations** | How many basic steps the computer performed. |
| **Memory** | How much extra computer memory the method needed. |

*Typical results:*

| Disaster | Careful methods | Quick method |
|---|---|---|
| Normal | 952 | 938 (1.5% less) |
| Capacity shock | 758 | 728 (4% less) |
| Greedy trap file | 180 | 100 (44% less) |

**What it tells you:** the quick method is cheaper but delivers less, and it falls
further behind as trucks become scarce. The two careful methods always agree.

---

## 6. Benchmarks — "Stress-test the three methods"

Uses made-up test data (not the loaded disaster) to measure speed, memory and
quality at different sizes. Results are the same every time you run them.

| Option | What it tests |
|---|---|
| **1. Size sweep** | More and more supply items: 10, 50, 100, 250, 500, 1000. Shows how each method slows down as the job grows. |
| **2. Capacity sweep** | Same 200 items, bigger and bigger trucks. Shows that the thorough method (DP) needs more time and memory as trucks grow, while the quick method does not. |
| **3. Adversarial** | Deliberately difficult data: the greedy trap (quick method loses 44%) and "look-alike" items where Branch & Bound struggles. |
| **4. Run all + export CSV** | Runs all three tests and saves the results to `results/benchmark.csv`. |
| **0. Back** | Return to the main menu. |

**Table columns:** test name, method, type of data, number of items (n), truck size (W),
value achieved, best possible value, gap %, time, memory, and **OK** ("yes" means
the answer was checked and is valid).

**Key findings to remember:**

- With 1000 items, DP takes about 50 ms and 48 MB; the quick method under 1 ms.
- On the trap data the quick method delivers 44% less.
- On look-alike data Branch & Bound slows sharply (40 items: 27 ms vs DP's 0.04 ms).

Options 1–3 ask afterwards whether to save the results to a file.

---

## 7. Manual Entry — "Type in your own small example"

Works without loading a disaster. The program asks for:

1. The shelter's urgency (1–10)
2. How many supply items (1–50)
3. For each item: its name, weight in kg, and how badly it is needed (1–10)
4. The truck's carrying capacity in kg

**You will see:** for each of the three methods, the items it would load, the total
value and weight, and how long it took — followed by how much less the quick method delivered.

**Limit:** one shelter and one truck, no roads. Use it to try numbers someone gives
you on the spot.

---

## 8. Export Reports — "Save the results to files"

Saves results as spreadsheet files (CSV) in the `results` folder. They open in Excel.

| Option | File created | Contains |
|---|---|---|
| **1. Plan** | `results/plan.csv` | One row per shelter: asked, received, percentage, reachable or not |
| **2. Routes** | `results/routes.csv` | One row per truck: shelter, load, items, distance, route |
| **3. Algorithm comparison** | `results/comparison.csv` | The scorecard from option 5 |
| **4. Everything** | All three files | — |
| **0. Back** | — | Return to the main menu |

If no plan has been made yet, the program makes one with the careful method (DP) first.
Saving again replaces the old file.

---

## 0. Exit

Closes the program.

---

## Typing rules (all menus)

- Type a number and press Enter. Letters, blanks or numbers outside the allowed
  range are refused with a message, and the question is asked again.
- Names may not contain commas (they would break the saved spreadsheet files).
- **0** always means Back (or Exit on the main menu).

---

## A good first walk-through

1. **1 → 1** load Normal
2. **2 → 1** look at the shelters
3. **3 → 1** make a plan with the careful method
4. **5** compare all three methods
5. **4 → 2** close road 4–6, then **4 → 4** re-plan and see the detour
6. **1 → 2** load the Capacity shock, then **5** — one shelter cut off, quick method falls further behind
7. **1 → 4** load `data/adversarial_greedy.csv`, then **5** — the quick method's big mistake
8. **7** try your own small example
9. **6 → 4** run all tests
10. **8 → 4** save everything
