#!/usr/bin/env python3
"""
Visualize quantization absolute errors vs BF16 input values

Usage:
    python3 plot_errors.py [min_val] [max_val]
    python3 plot_errors.py [range_val]

Arguments:
    range_val: Single value creates symmetric range [-range_val, range_val]
    min_val max_val: Specify minimum and maximum values explicitly

Examples:
    python3 plot_errors.py          # All values
    python3 plot_errors.py 100      # Range [-100, 100]
    python3 plot_errors.py -3 3     # Range [-3, 3]
    python3 plot_errors.py 0 1000   # Range [0, 1000]
"""

import re
import sys
import numpy as np
import matplotlib.pyplot as plt

def parse_log_file(log_file):
    """Parse the quantization log file - auto-detect HW only, Ref only, or both"""
    data = {
        'index': [],
        'bf16_val': [],
        'hw_dequant': [],
        'ref_dequant': [],
        'model_type': None  # 'hw', 'ref', or 'both'
    }

    with open(log_file, 'r') as f:
        for line in f:
            # Try both models format: index | bf16 | hw_quant | hw_dequant | hw_err% | ref_quant | ref_dequant | ref_err%
            match_both = re.match(r'\s*(\d+)\s+\|\s+0x[0-9a-fA-F]+\(([+-][\d.e+-]+)\)\s+\|\s+0x[0-9a-fA-F]+\s+\|\s+([+-][\d.e+-]+)\s+\|\s+[\d.]+%\s+\|\s+0x[0-9a-fA-F]+\s+\|\s+([+-][\d.e+-]+)\s+\|\s+[\d.]+%\s*$', line)
            if match_both:
                idx, bf16_val, hw_dq, ref_dq = match_both.groups()
                data['index'].append(int(idx))
                data['bf16_val'].append(float(bf16_val))
                data['hw_dequant'].append(float(hw_dq))
                data['ref_dequant'].append(float(ref_dq))
                if data['model_type'] is None:
                    data['model_type'] = 'both'
                continue

            # Try HW only format: index | bf16_hex(value) | hw_quant | hw_dequant | hw_err%
            match_hw = re.match(r'\s*(\d+)\s+\|\s+0x[0-9a-fA-F]+\(([+-][\d.e+-]+)\)\s+\|\s+0x[0-9a-fA-F]+\s+\|\s+([+-][\d.e+-]+)\s+\|\s+[\d.]+%\s*$', line)
            if match_hw:
                idx, bf16_val, hw_dq = match_hw.groups()
                data['index'].append(int(idx))
                data['bf16_val'].append(float(bf16_val))
                data['hw_dequant'].append(float(hw_dq))
                if data['model_type'] is None:
                    data['model_type'] = 'hw'
                continue

            # Try Ref only format: index | bf16_hex(value) | ref_quant | ref_dequant | ref_err%
            match_ref = re.match(r'\s*(\d+)\s+\|\s+0x[0-9a-fA-F]+\(([+-][\d.e+-]+)\)\s+\|\s+0x[0-9a-fA-F]+\s+\|\s+([+-][\d.e+-]+|[+-]?(?:nan|inf))\s+\|\s+[\d.]+%\s*$', line)
            if match_ref:
                idx, bf16_val, dequant = match_ref.groups()
                data['index'].append(int(idx))
                data['bf16_val'].append(float(bf16_val))
                # Handle nan/inf strings
                try:
                    data['ref_dequant'].append(float(dequant))
                except ValueError:
                    data['ref_dequant'].append(float('nan'))
                if data['model_type'] is None:
                    data['model_type'] = 'ref'
                continue

    # Convert to numpy arrays
    for key in ['index', 'bf16_val', 'hw_dequant']:
        if len(data[key]) > 0:
            data[key] = np.array(data[key])

    if data['model_type'] == 'both' or data['model_type'] == 'ref':
        data['ref_dequant'] = np.array(data['ref_dequant'])

    print(f"Detected model type: {data['model_type']}")
    return data

def filter_by_range(data, min_val, max_val):
    """Filter data by value range"""
    if min_val is None or max_val is None:
        return data, "All values"

    # Filter: keep values where bf16_val is in [min_val, max_val]
    mask = (data['bf16_val'] >= min_val) & (data['bf16_val'] <= max_val)

    filtered_data = {'model_type': data['model_type']}
    for key in data:
        if key != 'model_type' and isinstance(data[key], np.ndarray):
            filtered_data[key] = data[key][mask]

    range_str = f"[{min_val}, {max_val}]"

    # Print filtering statistics
    print(f"\nFiltering data to range [{min_val}, {max_val}]:")
    print(f"  Original data points: {len(data['bf16_val'])}")
    print(f"  Filtered data points: {len(filtered_data['bf16_val'])}")
    if len(filtered_data['bf16_val']) > 0:
        bf16_nonzero = filtered_data['bf16_val'][filtered_data['bf16_val'] != 0]
        if len(bf16_nonzero) > 0:
            print(f"  Actual BF16 range: [{np.min(filtered_data['bf16_val']):.3e}, {np.max(filtered_data['bf16_val']):.3e}]")

    return filtered_data, range_str

def plot_error_scatter(data, range_str, min_val, max_val, output_file='error_scatter.png'):
    """Create scatter plot of absolute errors vs BF16 values"""

    # Calculate absolute errors (differences)
    hw_abs_err = np.abs(data['bf16_val'] - data['hw_dequant'])

    # Filter valid data (finite values) - include zero errors for linear scale
    hw_valid = np.isfinite(hw_abs_err) & np.isfinite(data['bf16_val'])

    # For statistics, we only count non-zero errors
    hw_valid_nonzero = hw_valid & (hw_abs_err > 0)

    # Create figure with single subplot
    fig, ax = plt.subplots(1, 1, figsize=(14, 8))

    # Color map based on BF16 sign
    hw_colors = ['red' if v < 0 else 'blue' for v in data['bf16_val'][hw_valid]]

    # HW Model scatter plot - show all valid points including zero errors
    ax.scatter(data['bf16_val'][hw_valid], hw_abs_err[hw_valid],
               c=hw_colors, s=10, alpha=0.6, edgecolors='none')
    ax.set_xlabel('BF16 Input Value', fontsize=12)
    ax.set_ylabel('Absolute Difference |Dequant - BF16_Input|', fontsize=12)

    # Set title based on model type
    model_name = 'HW Model' if data.get('model_type') == 'hw' else ('Ref Model' if data.get('model_type') == 'ref' else 'Model')
    ax.set_title(f'{model_name}: Quantization Error vs Input Value ({range_str})', fontsize=14, fontweight='bold')

    # Use linear scale for both axes
    # No need to set scale, linear is default

    # Set x-axis limits if range is specified
    if min_val is not None and max_val is not None:
        ax.set_xlim(min_val, max_val)

    ax.grid(True, alpha=0.3, which='both')
    ax.grid(True, alpha=0.15, which='minor', linestyle=':')
    ax.axvline(x=0, color='black', linestyle='--', linewidth=0.5, alpha=0.5)

    # Add legend
    from matplotlib.lines import Line2D
    legend_elements = [Line2D([0], [0], marker='o', color='w', markerfacecolor='blue',
                             markersize=8, label='Positive BF16'),
                      Line2D([0], [0], marker='o', color='w', markerfacecolor='red',
                             markersize=8, label='Negative BF16')]
    ax.legend(handles=legend_elements, loc='upper right')

    # Statistics for HW - calculate relative errors
    if len(hw_abs_err[hw_valid_nonzero]) > 0:
        # Only calculate relative error for points where |BF16| is not too small
        bf16_abs = np.abs(data['bf16_val'][hw_valid_nonzero])
        valid_for_rel_err = bf16_abs > 1e-10  # Exclude very small values

        if np.sum(valid_for_rel_err) > 0:
            hw_rel_err = np.abs(hw_abs_err[hw_valid_nonzero][valid_for_rel_err] / data['bf16_val'][hw_valid_nonzero][valid_for_rel_err]) * 100
            max_rel_err = np.max(hw_rel_err)
            mean_rel_err = np.mean(hw_rel_err)
            hw_stats = f"Samples: {len(hw_abs_err[hw_valid_nonzero])} | Max relative error: {max_rel_err:.2f}% | Mean relative error: {mean_rel_err:.2f}%"
        else:
            hw_stats = f"Samples: {len(hw_abs_err[hw_valid_nonzero])} | Relative error: N/A (values too small)"
    else:
        hw_stats = "No error samples in this range"
    ax.text(0.02, 0.98, hw_stats, transform=ax.transAxes,
            fontsize=9, verticalalignment='top',
            bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))

    plt.tight_layout()
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"\nScatter plot saved to: {output_file}")

    # Print detailed statistics
    print("\n" + "="*70)
    print(f"Statistics (Range: {range_str}):")
    print("="*70)

    model_name = 'HW Model' if data.get('model_type') == 'hw' else ('Ref Model' if data.get('model_type') == 'ref' else 'Model')
    print(f"\n{model_name}:")
    print(f"  Total samples with difference > 0: {len(hw_abs_err[hw_valid_nonzero])}")
    if len(hw_abs_err[hw_valid_nonzero]) > 0:
        print(f"  Max absolute difference: {np.max(hw_abs_err[hw_valid_nonzero]):.6e}")
        print(f"  Mean absolute difference: {np.mean(hw_abs_err[hw_valid_nonzero]):.6e}")

        # Calculate relative errors excluding very small BF16 values
        bf16_abs = np.abs(data['bf16_val'][hw_valid_nonzero])
        valid_for_rel_err = bf16_abs > 1e-10
        if np.sum(valid_for_rel_err) > 0:
            hw_rel_err = np.abs(hw_abs_err[hw_valid_nonzero][valid_for_rel_err] / data['bf16_val'][hw_valid_nonzero][valid_for_rel_err]) * 100
            print(f"  Max relative error: {np.max(hw_rel_err):.2f}% (excluding |BF16| < 1e-10)")
            print(f"  Mean relative error: {np.mean(hw_rel_err):.2f}% (excluding |BF16| < 1e-10)")
    print("="*70)

    return fig

def main():
    log_file = './log/quant_output.log'
    output_file = './log/error_scatter.png'

    # Parse command line arguments
    min_val = None
    max_val = None

    if len(sys.argv) == 1:
        # No arguments - use all values
        print("Using all values (no range filter)")
    elif len(sys.argv) == 2:
        # Single argument - symmetric range [-val, val]
        try:
            range_val = float(sys.argv[1])
            min_val = -abs(range_val)
            max_val = abs(range_val)
            print(f"Using symmetric range: [{min_val}, {max_val}]")
        except ValueError:
            print(f"Error: Invalid value '{sys.argv[1]}'. Must be a number.")
            print(__doc__)
            sys.exit(1)
    elif len(sys.argv) == 3:
        # Two arguments - explicit min and max
        try:
            min_val = float(sys.argv[1])
            max_val = float(sys.argv[2])
            if min_val > max_val:
                print(f"Error: min_val ({min_val}) must be <= max_val ({max_val})")
                sys.exit(1)
            print(f"Using range: [{min_val}, {max_val}]")
        except ValueError:
            print(f"Error: Invalid values. Both arguments must be numbers.")
            print(__doc__)
            sys.exit(1)
    else:
        print("Error: Too many arguments")
        print(__doc__)
        sys.exit(1)

    print(f"Reading log file: {log_file}")
    data = parse_log_file(log_file)
    print(f"Parsed {len(data['index'])} data points")

    # Filter data by range
    data, range_str = filter_by_range(data, min_val, max_val)
    print(f"After filtering: {len(data['index'])} data points in range {range_str}")

    print("Generating scatter plot...")
    plot_error_scatter(data, range_str, min_val, max_val, output_file)

    print("\nDone!")

if __name__ == '__main__':
    main()
