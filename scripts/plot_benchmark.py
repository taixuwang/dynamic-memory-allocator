#!/usr/bin/env python3
"""
plot_benchmark.py — Generate performance comparison charts from benchmark CSV.

Usage:
    python3 scripts/plot_benchmark.py [benchmark_results.csv]

Generates:
    benchmark_throughput.png  — Throughput comparison (M ops/sec)
    benchmark_latency.png     — Latency comparison (µs per 100K ops)
    benchmark_summary.png     — Combined summary chart

Copyright 2026 Taixu Wang
"""

import sys
import csv
import os

try:
    import matplotlib
    matplotlib.use('Agg')  # Non-interactive backend
    import matplotlib.pyplot as plt
    import matplotlib.ticker as ticker
except ImportError:
    print("Error: matplotlib is required. Install with: pip3 install matplotlib")
    sys.exit(1)


def load_csv(filepath):
    """Load benchmark CSV and return structured data."""
    data = {}
    with open(filepath, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            wl = row['workload']
            alloc = row['allocator']
            if wl not in data:
                data[wl] = {}
            data[wl][alloc] = {
                'ops':        int(row['ops']),
                'allocs':     int(row['allocs']),
                'frees':      int(row['frees']),
                'elapsed_us': float(row['elapsed_us']),
                'throughput':  float(row['throughput_mops']),
                'peak_kb':    int(row['peak_held_kb']),
            }
    return data


def setup_style():
    """Apply a clean, modern chart style."""
    plt.rcParams.update({
        'figure.facecolor':   '#0d1117',
        'axes.facecolor':     '#161b22',
        'axes.edgecolor':     '#30363d',
        'axes.labelcolor':    '#c9d1d9',
        'text.color':         '#c9d1d9',
        'xtick.color':        '#8b949e',
        'ytick.color':        '#8b949e',
        'grid.color':         '#21262d',
        'grid.linestyle':     '--',
        'grid.alpha':         0.7,
        'font.family':        'sans-serif',
        'font.size':          11,
        'figure.dpi':         150,
    })


# Color palette
COLOR_SYSTEM  = '#58a6ff'   # Blue for system malloc
COLOR_MYMALLOC = '#f78166'  # Orange for mymalloc
COLOR_RATIO    = '#7ee787'  # Green for ratio


def plot_throughput(data, output_dir):
    """Bar chart comparing throughput (M ops/sec)."""
    workloads = list(data.keys())
    sys_tp = [data[w].get('system', {}).get('throughput', 0) for w in workloads]
    my_tp  = [data[w].get('mymalloc', {}).get('throughput', 0) for w in workloads]

    fig, ax = plt.subplots(figsize=(10, 5))

    x = range(len(workloads))
    width = 0.35

    bars1 = ax.bar([i - width/2 for i in x], sys_tp, width,
                   label='system malloc', color=COLOR_SYSTEM, edgecolor='none',
                   alpha=0.9, zorder=3)
    bars2 = ax.bar([i + width/2 for i in x], my_tp, width,
                   label='mymalloc', color=COLOR_MYMALLOC, edgecolor='none',
                   alpha=0.9, zorder=3)

    ax.set_xlabel('Workload')
    ax.set_ylabel('Throughput (M ops / sec)')
    ax.set_title('Throughput Comparison: mymalloc vs System malloc',
                 fontsize=14, fontweight='bold', pad=15)
    ax.set_xticks(x)
    ax.set_xticklabels([w.replace('_', '\n') for w in workloads])
    ax.legend(loc='upper right', framealpha=0.8,
              facecolor='#161b22', edgecolor='#30363d')
    ax.grid(axis='y', zorder=0)
    ax.set_axisbelow(True)

    # Value labels on bars
    for bar in bars1:
        h = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., h + 0.01,
                f'{h:.2f}', ha='center', va='bottom', fontsize=8, color='#8b949e')
    for bar in bars2:
        h = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., h + 0.01,
                f'{h:.2f}', ha='center', va='bottom', fontsize=8, color='#8b949e')

    plt.tight_layout()
    path = os.path.join(output_dir, 'benchmark_throughput.png')
    plt.savefig(path, bbox_inches='tight')
    plt.close()
    print(f"  ✓ {path}")
    return path


def plot_latency(data, output_dir):
    """Bar chart comparing latency (µs per 100K operations)."""
    workloads = list(data.keys())
    sys_lat = [data[w].get('system', {}).get('elapsed_us', 0) /
               (data[w].get('system', {}).get('ops', 1) / 100000)
               for w in workloads]
    my_lat = [data[w].get('mymalloc', {}).get('elapsed_us', 0) /
              (data[w].get('mymalloc', {}).get('ops', 1) / 100000)
              for w in workloads]

    fig, ax = plt.subplots(figsize=(10, 5))
    x = range(len(workloads))
    width = 0.35

    ax.bar([i - width/2 for i in x], sys_lat, width,
           label='system malloc', color=COLOR_SYSTEM, edgecolor='none',
           alpha=0.9, zorder=3)
    ax.bar([i + width/2 for i in x], my_lat, width,
           label='mymalloc', color=COLOR_MYMALLOC, edgecolor='none',
           alpha=0.9, zorder=3)

    ax.set_xlabel('Workload')
    ax.set_ylabel('Latency (µs per 100K ops)')
    ax.set_title('Latency Comparison: mymalloc vs System malloc',
                 fontsize=14, fontweight='bold', pad=15)
    ax.set_xticks(x)
    ax.set_xticklabels([w.replace('_', '\n') for w in workloads])
    ax.legend(loc='upper right', framealpha=0.8,
              facecolor='#161b22', edgecolor='#30363d')
    ax.grid(axis='y', zorder=0)

    plt.tight_layout()
    path = os.path.join(output_dir, 'benchmark_latency.png')
    plt.savefig(path, bbox_inches='tight')
    plt.close()
    print(f"  ✓ {path}")
    return path


def plot_summary(data, output_dir):
    """Combined summary: throughput bars + speedup line on secondary axis."""
    workloads = list(data.keys())
    sys_tp = [data[w].get('system', {}).get('throughput', 0) for w in workloads]
    my_tp  = [data[w].get('mymalloc', {}).get('throughput', 0) for w in workloads]

    # Calculate speedup ratio (>1 means mymalloc is faster)
    ratios = []
    for s, m in zip(sys_tp, my_tp):
        if s > 0 and m > 0:
            ratios.append(m / s)
        else:
            ratios.append(1.0)

    fig, ax1 = plt.subplots(figsize=(11, 5.5))

    x = range(len(workloads))
    width = 0.30

    ax1.bar([i - width/2 for i in x], sys_tp, width,
            label='system malloc', color=COLOR_SYSTEM, alpha=0.85, zorder=3)
    ax1.bar([i + width/2 for i in x], my_tp, width,
            label='mymalloc', color=COLOR_MYMALLOC, alpha=0.85, zorder=3)

    ax1.set_xlabel('Workload', fontsize=12)
    ax1.set_ylabel('Throughput (M ops/sec)', fontsize=12)
    ax1.set_xticks(x)
    ax1.set_xticklabels([w.replace('_', '\n') for w in workloads])
    ax1.grid(axis='y', zorder=0)

    # Secondary axis for speedup ratio
    ax2 = ax1.twinx()
    ax2.plot(x, ratios, 'o-', color=COLOR_RATIO, linewidth=2.5,
             markersize=8, label='mymalloc / system ratio', zorder=5)
    ax2.axhline(y=1.0, color='#484f58', linestyle=':', linewidth=1, zorder=2)
    ax2.set_ylabel('Speed Ratio (mymalloc / system)', fontsize=12,
                   color=COLOR_RATIO)
    ax2.tick_params(axis='y', labelcolor=COLOR_RATIO)

    # Annotate ratio values
    for i, r in enumerate(ratios):
        label = f'{r:.2f}x'
        ax2.annotate(label, (i, r), textcoords="offset points",
                     xytext=(0, 12), ha='center', fontsize=9,
                     color=COLOR_RATIO, fontweight='bold')

    # Combined legend
    lines1, labels1 = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    ax1.legend(lines1 + lines2, labels1 + labels2,
               loc='upper left', framealpha=0.8,
               facecolor='#161b22', edgecolor='#30363d')

    ax1.set_title('Memory Allocator Performance Summary',
                  fontsize=15, fontweight='bold', pad=15)

    plt.tight_layout()
    path = os.path.join(output_dir, 'benchmark_summary.png')
    plt.savefig(path, bbox_inches='tight')
    plt.close()
    print(f"  ✓ {path}")
    return path


def main():
    csv_path = sys.argv[1] if len(sys.argv) > 1 else 'benchmark_results.csv'

    if not os.path.exists(csv_path):
        print(f"Error: {csv_path} not found. Run `make benchmark` first.")
        sys.exit(1)

    output_dir = os.path.dirname(csv_path) or '.'
    data = load_csv(csv_path)

    print(f"\nLoaded {len(data)} workloads from {csv_path}")
    print("Generating charts...\n")

    setup_style()
    plot_throughput(data, output_dir)
    plot_latency(data, output_dir)
    plot_summary(data, output_dir)

    print(f"\nDone! Charts saved to {output_dir}/\n")


if __name__ == '__main__':
    main()
