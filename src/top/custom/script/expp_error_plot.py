#!/usr/bin/env python3
# +FHDR========================================================================
#  File Name:      expp_error_plot.py
#                  Rocky (luoqi754@gmail.com)
#  Description:    Plot BF16ExppUnit error distribution
# -FHDR========================================================================

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
import os

def plot_expp_error(csv_file='log/full_expp.csv', output_file='log/expp_error_plot.png', input_range=10.0):
    """Plot error distribution for BF16ExppUnit

    Args:
        csv_file: Path to CSV file containing test results
        output_file: Path to save the plot
        input_range: Range for input values [-range, +range]
    """

    # Check if file exists
    if not os.path.exists(csv_file):
        print(f"Error: File {csv_file} not found!")
        print("Please run 'make expp' first to generate the data.")
        return

    # Read CSV file
    print(f"Reading data from {csv_file}...")
    df = pd.read_csv(csv_file)

    # Extract columns (updated column names)
    input_dec = df['Input_Dec'].values
    expected_f32 = df['Expected_F32'].values
    hw_dec = df['HW_Dec'].values
    diff = df['Diff'].values

    print(f"Total data points loaded: {len(input_dec)}")

    # Filter out nan and inf values
    valid_mask = np.isfinite(input_dec) & np.isfinite(diff) & np.isfinite(expected_f32) & np.isfinite(hw_dec)
    input_dec = input_dec[valid_mask]
    diff = diff[valid_mask]
    expected_f32 = expected_f32[valid_mask]
    hw_dec = hw_dec[valid_mask]

    print(f"Valid data points after filtering nan/inf: {len(input_dec)}")

    # Filter by input range
    range_mask = (input_dec >= -input_range) & (input_dec <= input_range)
    input_dec_filtered = input_dec[range_mask]
    diff_filtered = diff[range_mask]
    expected_f32_filtered = expected_f32[range_mask]
    hw_dec_filtered = hw_dec[range_mask]

    print(f"Data points in range [{-input_range}, {input_range}]: {len(input_dec_filtered)}")

    if len(input_dec_filtered) == 0:
        print(f"Warning: No data points found in range [{-input_range}, {input_range}]")
        return

    # Create figure
    fig, ax = plt.subplots(1, 1, figsize=(14, 8))

    # Plot scatter
    ax.scatter(input_dec_filtered, diff_filtered, alpha=0.6, s=3, c='blue', label='Error Points')
    ax.axhline(y=0, color='r', linestyle='--', linewidth=1, label='Zero Error')

    # Labels and title
    ax.set_xlabel('Input Value (Decimal)', fontsize=14, fontweight='bold')
    ax.set_ylabel('Diff (HW - Expected)', fontsize=14, fontweight='bold')
    ax.set_title(f'BF16ExppUnit Error Distribution (Input Range: [{-input_range}, {input_range}])',
                 fontsize=16, fontweight='bold')
    ax.grid(True, alpha=0.3, linestyle='--')
    ax.legend(loc='upper right', fontsize=10)

    # Calculate and display statistics
    abs_diff = np.abs(diff_filtered)
    max_error = np.max(abs_diff)
    avg_error = np.mean(abs_diff)
    std_error = np.std(abs_diff)

    stats_text = (f'Statistics (in range):\n'
                  f'Max |Error|: {max_error:.6e}\n'
                  f'Avg |Error|: {avg_error:.6e}\n'
                  f'Std |Error|: {std_error:.6e}\n'
                  f'Total Points: {len(input_dec_filtered)}')

    ax.text(0.02, 0.98, stats_text, transform=ax.transAxes,
            verticalalignment='top', bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.9),
            fontsize=11, family='monospace')

    # Set tight layout
    plt.tight_layout()

    # Save figure
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"\nPlot saved to: {output_file}")
    print(f"  Range: [{-input_range}, {input_range}]")
    print(f"  Points plotted: {len(input_dec_filtered)}")
    print(f"  Max |Error|: {max_error:.6e}")
    print(f"  Avg |Error|: {avg_error:.6e}")

    # Show plot
    plt.show()

def main():
    """Main function"""
    csv_file = 'log/full_expp.csv'
    output_file = 'log/expp_error_plot.png'
    input_range = 10.0  # Default range: -10 to 10

    # Parse command line arguments
    if len(sys.argv) > 1:
        try:
            input_range = float(sys.argv[1])
            print(f"Using input range: [{-input_range}, {input_range}]")
        except ValueError:
            print(f"Error: Invalid range value '{sys.argv[1]}'. Using default range: [-10, 10]")
            input_range = 10.0

    if len(sys.argv) > 2:
        csv_file = sys.argv[2]

    if len(sys.argv) > 3:
        output_file = sys.argv[3]

    plot_expp_error(csv_file, output_file, input_range)

    print("\nUsage:")
    print("  python3 expp_error_plot.py [range] [csv_file] [output_file]")
    print("  Example: python3 expp_error_plot.py 4  # Plot range [-4, 4]")

if __name__ == '__main__':
    main()
