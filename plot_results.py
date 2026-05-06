import os
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.patches import Patch

CSV_FILE = "btree_experiment_results.csv"
OUTPUT_DIR = "figures"

TREE_ORDER = ["BTree", "BPlusTree", "BStarTree"]
TREE_LABELS = {
    "BTree": "B-tree",
    "BPlusTree": "B+-tree",
    "BStarTree": "B*-tree",
}

def load_results():
    if not os.path.exists(CSV_FILE):
        raise FileNotFoundError(f"Cannot find {CSV_FILE}")

    df = pd.read_csv(CSV_FILE)
    df["tree"] = pd.Categorical(df["tree"], categories=TREE_ORDER, ordered=True)
    return df

def plot_grouped_bar(df, metric, ylabel, title, filename):
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    summary = (
        df.groupby(["order", "tree"], observed=True)[metric]
        .mean()
        .reset_index()
    )

    orders = sorted(summary["order"].unique())
    x = list(range(len(orders)))
    width = 0.25

    plt.figure(figsize=(8, 5))

    for i, tree in enumerate(TREE_ORDER):
        values = []
        for order in orders:
            row = summary[(summary["order"] == order) & (summary["tree"] == tree)]
            values.append(row[metric].iloc[0] if not row.empty else 0)

        positions = [p + (i - 1) * width for p in x]
        plt.bar(positions, values, width=width, label=TREE_LABELS[tree])

    plt.xticks(x, [f"d={o}" for o in orders])
    plt.xlabel("Tree order $d$")
    plt.ylabel(ylabel)
    plt.title(title)
    plt.legend()
    plt.tight_layout()

    path = os.path.join(OUTPUT_DIR, filename)
    plt.savefig(path, dpi=300)
    plt.close()
    print(f"Saved: {path}")

def plot_deletion_events(df):
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    summary = (
        df.groupby(["order", "tree"], observed=True)[["merge_count", "borrow_count"]]
        .mean()
        .reset_index()
    )

    orders = sorted(summary["order"].unique())
    x = list(range(len(orders)))
    width = 0.25

    for metric, ylabel, filename in [
        ("merge_count", "Average merge count", "delete_merge_count.png"),
        ("borrow_count", "Average borrow count", "delete_borrow_count.png"),
    ]:
        plt.figure(figsize=(8, 5))

        for i, tree in enumerate(TREE_ORDER):
            values = []
            for order in orders:
                row = summary[(summary["order"] == order) & (summary["tree"] == tree)]
                values.append(row[metric].iloc[0] if not row.empty else 0)

            positions = [p + (i - 1) * width for p in x]
            plt.bar(positions, values, width=width, label=TREE_LABELS[tree])

        plt.xticks(x, [f"d={o}" for o in orders])
        plt.xlabel("Tree order $d$")
        plt.ylabel(ylabel)
        plt.title(ylabel + " after deletion workload")
        plt.legend()
        plt.tight_layout()

        path = os.path.join(OUTPUT_DIR, filename)
        plt.savefig(path, dpi=300)
        plt.close()
        print(f"Saved: {path}")

def plot_deletion_strategy_ratio(df):
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    summary = (
        df.groupby(["order", "tree"], observed=True)[["merge_count", "borrow_count"]]
        .mean()
        .reset_index()
    )

    summary["total"] = summary["merge_count"] + summary["borrow_count"]
    summary["merge_ratio"] = (summary["merge_count"] / summary["total"] * 100).fillna(0)
    summary["borrow_ratio"] = (summary["borrow_count"] / summary["total"] * 100).fillna(0)

    orders = sorted(summary["order"].unique())
    x = list(range(len(orders)))
    width = 0.25 

    tree_colors = {
        "BTree": "C0",
        "BPlusTree": "C1",
        "BStarTree": "C2",
    }

    fig, ax = plt.subplots(figsize=(9, 5.5))

    for i, tree in enumerate(TREE_ORDER):
        merge_vals = []
        borrow_vals = []

        for order in orders:
            row = summary[(summary["order"] == order) & (summary["tree"] == tree)]
            if row.empty:
                merge_vals.append(0)
                borrow_vals.append(0)
            else:
                merge_vals.append(row["merge_ratio"].iloc[0])
                borrow_vals.append(row["borrow_ratio"].iloc[0])

        positions = [p + (i - 1) * width for p in x]

        ax.bar(
            positions,
            merge_vals,
            width=width,
            color=tree_colors[tree],
            edgecolor="black",
            alpha=0.85,
        )

        ax.bar(
            positions,
            borrow_vals,
            width=width,
            bottom=merge_vals,
            color=tree_colors[tree],
            edgecolor="black",
            alpha=0.30,
            hatch="//",
        )

    ax.set_xticks(x)
    ax.set_xticklabels([f"d={o}" for o in orders])
    ax.set_xlabel("Tree order $d$")
    ax.set_ylabel("Deletion event ratio (%)")
    ax.set_ylim(0, 105)
    ax.set_title("Deletion Event Ratio: Merge vs. Borrow")

    tree_legend = [
        Patch(facecolor=tree_colors["BTree"], edgecolor="black", label="B-tree"),
        Patch(facecolor=tree_colors["BPlusTree"], edgecolor="black", label="B$^+$-tree"),
        Patch(facecolor=tree_colors["BStarTree"], edgecolor="black", label="B$^*$-tree"),
    ]

    event_legend = [
        Patch(facecolor="gray", edgecolor="black", alpha=0.85, label="Merge"),
        Patch(facecolor="gray", edgecolor="black", alpha=0.30, hatch="//", label="Borrow"),
    ]

    legend1 = ax.legend(
        handles=tree_legend,
        title="Tree type",
        loc="upper left",
        bbox_to_anchor=(1.02, 1.00),
    )
    ax.add_artist(legend1)

    ax.legend(
        handles=event_legend,
        title="Event type",
        loc="upper left",
        bbox_to_anchor=(1.02, 0.55),
    )

    plt.tight_layout()

    path = os.path.join(OUTPUT_DIR, "deletion_event_ratio_stacked.png")
    plt.savefig(path, dpi=300, bbox_inches="tight")
    plt.close()
    print(f"Saved: {path}")

def plot_merge_ratio_only(df):
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    summary = (
        df.groupby(["order", "tree"], observed=True)[["merge_count", "borrow_count"]]
        .mean()
        .reset_index()
    )

    summary["total"] = summary["merge_count"] + summary["borrow_count"]
    summary["merge_ratio"] = (summary["merge_count"] / summary["total"] * 100).fillna(0)

    orders = sorted(summary["order"].unique())
    x = list(range(len(orders)))
    width = 0.25

    plt.figure(figsize=(8, 5))

    for i, tree in enumerate(TREE_ORDER):
        values = []
        for order in orders:
            row = summary[(summary["order"] == order) & (summary["tree"] == tree)]
            values.append(row["merge_ratio"].iloc[0] if not row.empty else 0)

        positions = [p + (i - 1) * width for p in x]

        plt.bar(
            positions,
            values,
            width=width,
            label=TREE_LABELS[tree],
            edgecolor="black",
        )

    plt.xticks(x, [f"d={o}" for o in orders])
    plt.xlabel("Tree order $d$")
    plt.ylabel("Merge ratio (%)")
    plt.title("Merge Ratio Comparison Across B-tree Variants")
    plt.legend()
    plt.tight_layout()

    path = os.path.join(OUTPUT_DIR, "merge_ratio_only.png")
    plt.savefig(path, dpi=300)
    plt.close()
    print(f"Saved: {path}")

def print_deletion_analysis_report(df):
    print("\n" + "=" * 80)
    print(" [Additional Experiments: Deletion Behavior & Structural Analysis] ")
    print("=" * 80 + "\n")

    summary = (
        df.groupby(["order", "tree"], observed=True)[
            ["merge_count", "borrow_count", "utilization"]
        ]
        .mean()
        .reset_index()
    )

    summary["total_events"] = summary["merge_count"] + summary["borrow_count"]
    summary["merge_ratio"] = (
        summary["merge_count"] / summary["total_events"] * 100
    ).fillna(0).round(2)
    summary["borrow_ratio"] = (
        summary["borrow_count"] / summary["total_events"] * 100
    ).fillna(0).round(2)

    btree_merges = summary[summary["tree"] == "BTree"].set_index("order")["merge_count"]

    def calc_reduction(row):
        if row["tree"] == "BTree":
            return 0.0
        base_merge = btree_merges.get(row["order"], 1)
        if base_merge == 0:
            return 0.0
        return round((1 - row["merge_count"] / base_merge) * 100, 2)

    summary["merge_reduction"] = summary.apply(calc_reduction, axis=1)

    print("### A. Table Format")
    print("-" * 100)
    print(
        f"{'Tree Type':<12} | {'d':<3} | {'Merge Avg':<10} | "
        f"{'Borrow Avg':<10} | {'Merge Ratio (%)':<16} | "
        f"{'Borrow Ratio (%)':<16} | {'Merge Reduction vs B-tree (%)':<28}"
    )
    print("-" * 100)

    for _, row in summary.iterrows():
        tree_label = TREE_LABELS[row["tree"]]
        print(
            f"{tree_label:<12} | {row['order']:<3.0f} | "
            f"{row['merge_count']:<10.1f} | {row['borrow_count']:<10.1f} | "
            f"{row['merge_ratio']:<16.2f} | {row['borrow_ratio']:<16.2f} | "
            f"{row['merge_reduction']:<28.2f}"
        )

    print("-" * 100 + "\n")

    print("### B. Key Quantitative Insights")

    try:
        bstar_d3_red = summary[(summary["tree"] == "BStarTree") & (summary["order"] == 3)]["merge_reduction"].iloc[0]
        bplus_d3_red = summary[(summary["tree"] == "BPlusTree") & (summary["order"] == 3)]["merge_reduction"].iloc[0]
        bstar_avg_borrow = summary[summary["tree"] == "BStarTree"]["borrow_ratio"].mean().round(2)
        btree_d3_merge_ratio = summary[(summary["tree"] == "BTree") & (summary["order"] == 3)]["merge_ratio"].iloc[0]
        bplus_d10_merge_ratio = summary[(summary["tree"] == "BPlusTree") & (summary["order"] == 10)]["merge_ratio"].iloc[0]
        util_diff = (
            summary[summary["tree"] == "BStarTree"]["utilization"].mean()
            - summary[summary["tree"] == "BTree"]["utilization"].mean()
        ).round(2)

        print(f"1. At d=3, B*-tree reduced merge operations by {bstar_d3_red}% compared with B-tree.")
        print(f"2. At d=3, B+-tree reduced merge operations by {bplus_d3_red}% compared with B-tree.")
        print(f"3. B*-tree showed an average borrow ratio of {bstar_avg_borrow}%, indicating that redistribution dominates deletion handling.")
        print(f"4. B-tree had a merge ratio of {btree_d3_merge_ratio}% at d=3, which is much higher than the ratio observed in larger orders.")
        print(f"5. B+-tree had a merge ratio of only {bplus_d10_merge_ratio}% at d=10, showing highly stable deletion behavior with larger node capacity.")
        print(f"6. B*-tree maintained {util_diff}% higher average node utilization than B-tree across all tree orders.")

    except Exception as e:
        print(f"[Warning] Not enough data to generate all insights. Error: {e}")

    print("\n" + "=" * 80 + "\n")

def main():
    df = load_results()

    plot_grouped_bar(
        df,
        metric="insert_time",
        ylabel="Insertion time (s)",
        title="Insertion Time by Tree Type and Order",
        filename="insert_time.png",
    )

    plot_grouped_bar(
        df,
        metric="splits",
        ylabel="Average number of node splits",
        title="Node Splits During Insertion by Tree Type and Order",
        filename="split_count.png",
    )

    plot_grouped_bar(
        df,
        metric="avg_search_access",
        ylabel="Logical accesses per query",
        title="Point Search Cost by Tree Type and Order",
        filename="search_access.png",
    )

    plot_grouped_bar(
        df,
        metric="avg_range_access",
        ylabel="Logical accesses per range query",
        title="Range Query Cost by Tree Type and Order",
        filename="range_access.png",
    )

    plot_grouped_bar(
        df,
        metric="utilization",
        ylabel="Node utilization (%)",
        title="Node Utilization by Tree Type and Order",
        filename="utilization.png",
    )

    plot_grouped_bar(
        df,
        metric="height",
        ylabel="Tree height",
        title="Tree Height by Tree Type and Order",
        filename="height.png",
    )

    plot_deletion_events(df)
    plot_deletion_strategy_ratio(df)
    plot_merge_ratio_only(df)
    print_deletion_analysis_report(df)

if __name__ == "__main__":
    main()
