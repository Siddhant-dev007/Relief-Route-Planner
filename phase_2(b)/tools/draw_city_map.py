"""
Draws the road maps of the Normal and Shock scenarios with each
truck's route from the DP plan. Output: docs/graphs/city_map_*.png

Node positions are a schematic layout (not to scale); the number on
each road is its real length in km from the scenario file.

Run from the project folder:  py tools/draw_city_map.py
"""
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

# Schematic position of every road node
POS = {0: (0, 0), 1: (3.5, -2.2), 2: (4, 2.6), 3: (7.2, -3), 4: (7.6, 0.8),
       5: (9.2, 4.4), 6: (10.4, -1.6), 7: (12.8, 2.4)}

# node -> (shelter name, urgency) ; node 0 is the warehouse
NORMAL_SHELTERS = {2: ("North Clinic", 10), 3: ("Riverside School", 9),
                   6: ("City Stadium", 8), 5: ("Hilltop Temple", 6),
                   7: ("Market Hall", 5)}
SHOCK_SHELTERS = dict(NORMAL_SHELTERS)
SHOCK_SHELTERS[6] = ("City Stadium", 10)

NORMAL_ROADS = [(0, 1, 4), (0, 2, 6), (1, 2, 3), (1, 3, 5), (2, 3, 8), (2, 4, 4),
                (3, 4, 2), (3, 6, 7), (4, 5, 6), (4, 6, 3), (5, 7, 5), (6, 7, 4)]
DESTROYED = {(1, 3), (4, 5), (5, 7)}   # missing from scenario_shock.csv

# DP plan routes printed by the program (Plan Operation -> Knapsack DP)
NORMAL_ROUTES = [("Truck 11 -> North Clinic, 6 km", [0, 2]),
                 ("Truck 14 -> Riverside School, 9 km", [0, 1, 3]),
                 ("Truck 12 -> City Stadium, 13 km", [0, 2, 4, 6]),
                 ("Truck 13 -> Hilltop Temple, 16 km", [0, 2, 4, 5])]
SHOCK_ROUTES = [("Truck 11 -> City Stadium, 13 km", [0, 2, 4, 6]),
                ("Truck 14 -> North Clinic, 6 km", [0, 2]),
                ("Truck 13 -> Riverside School, 12 km", [0, 2, 4, 3])]

COLORS = ["#d62728", "#1f77b4", "#2ca02c", "#9467bd"]


def draw(filename, title, shelters, roads, destroyed, routes, note):
    fig, ax = plt.subplots(figsize=(11, 6.6), dpi=130)

    # roads
    for a, b, km in roads:
        (x1, y1), (x2, y2) = POS[a], POS[b]
        gone = (a, b) in destroyed
        ax.plot([x1, x2], [y1, y2], color="#d62728" if gone else "#b0b0b0",
                linewidth=2 if gone else 3, linestyle="--" if gone else "-",
                zorder=1)
        mx, my = (x1 + x2) / 2, (y1 + y2) / 2
        label = f"{km} km" + (" X" if gone else "")
        ax.text(mx, my, label, fontsize=8.5, ha="center", va="center",
                color="#d62728" if gone else "#444",
                bbox=dict(boxstyle="round,pad=0.15", fc="white", ec="none"),
                zorder=4)

    # truck routes, each slightly offset so overlapping routes stay visible
    for k, (label, path) in enumerate(routes):
        off = (k - (len(routes) - 1) / 2) * 0.09
        xs = [POS[n][0] + off for n in path]
        ys = [POS[n][1] + off for n in path]
        ax.plot(xs, ys, color=COLORS[k], linewidth=2.2, zorder=2, label=label)
        ax.annotate("", xy=(xs[-1], ys[-1]), xytext=(xs[-2], ys[-2]),
                    arrowprops=dict(arrowstyle="-|>", color=COLORS[k], lw=2,
                                    shrinkA=0, shrinkB=14), zorder=3)

    # nodes
    for n, (x, y) in POS.items():
        if n == 0:
            ax.scatter(x, y, s=650, marker="s", color="#333", zorder=5)
            ax.text(x, y, "0", color="white", ha="center", va="center",
                    fontsize=10, weight="bold", zorder=6)
            ax.text(x, y - 0.75, "WAREHOUSE\n(depot)", ha="center", va="top",
                    fontsize=9, weight="bold")
        elif n in shelters:
            name, urg = shelters[n]
            cut = n == 5 and destroyed
            ax.scatter(x, y, s=650, color="#ffcccc" if cut else "#ffe9a8",
                       edgecolor="#d62728" if cut else "#b8860b",
                       linewidth=2, zorder=5)
            ax.text(x, y, str(n), ha="center", va="center", fontsize=10,
                    weight="bold", zorder=6)
            tag = f"{name}\nurgency {urg}" + ("\nUNREACHABLE" if cut else "")
            below = n == 3                      # keep label off the roads
            ax.text(x, y - 0.62 if below else y + 0.62, tag, ha="center",
                    va="top" if below else "bottom", fontsize=9,
                    color="#d62728" if cut else "black",
                    weight="bold" if cut else "normal")
        else:
            ax.scatter(x, y, s=330, color="white", edgecolor="#777",
                       linewidth=1.5, zorder=5)
            ax.text(x, y, str(n), ha="center", va="center", fontsize=9,
                    zorder=6)
            ax.text(x, y - 0.55, "junction", ha="center", va="top",
                    fontsize=8, color="#777")

    ax.set_title(title, fontsize=13, weight="bold")
    ax.legend(loc="lower right", fontsize=9, title="DP plan routes")
    ax.text(0.01, 0.01, note, transform=ax.transAxes, fontsize=8.5,
            color="#555", va="bottom")
    ax.set_xlim(-1.5, 14.2)
    ax.set_ylim(-4.6, 6.3)
    ax.axis("off")
    fig.tight_layout()
    fig.savefig(filename)
    plt.close(fig)


def main():
    out = os.path.join("docs", "graphs")
    os.makedirs(out, exist_ok=True)

    draw(os.path.join(out, "city_map_normal.png"),
         "Normal scenario: road map and truck routes",
         NORMAL_SHELTERS, NORMAL_ROADS, set(), NORMAL_ROUTES,
         "Schematic layout, not to scale; road labels are real distances.\n"
         "Market Hall (node 7) gets no truck: 4 trucks, 5 shelters.")

    draw(os.path.join(out, "city_map_shock.png"),
         "Shock scenario: destroyed roads and new routes",
         SHOCK_SHELTERS, NORMAL_ROADS, DESTROYED, SHOCK_ROUTES,
         "Red dashed = destroyed (1-3, 4-5, 5-7). Hilltop Temple is cut off;\n"
         "truck 12 is broken; Riverside now 12 km via 0>2>4>3 (was 9 km).")

    print("Saved city_map_normal.png and city_map_shock.png in", out)


if __name__ == "__main__":
    main()
