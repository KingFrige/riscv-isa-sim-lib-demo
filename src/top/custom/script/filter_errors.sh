#!/bin/bash
# Filter quantization log for entries with errors, NaN, or Inf
# Usage: ./filter_errors.sh [log_file] [error_threshold] [filter_mode]
#   filter_mode: "errors" (default), "nan_inf", or "all"

LOG_FILE="${1:-./log/quant_output.log}"
ERROR_THRESHOLD="${2:-0.01}"
FILTER_MODE="${3:-errors}"
OUTPUT_FILE="./log/filter_quan.log"

echo "Filtering from: $LOG_FILE"
echo "Output will be saved to: $OUTPUT_FILE"

case "$FILTER_MODE" in
    nan_inf)
        echo "Mode: Show only NaN/Inf entries"
        ;;
    all)
        echo "Mode: Show errors > ${ERROR_THRESHOLD}% AND NaN/Inf entries"
        ;;
    errors|*)
        echo "Mode: Show errors > ${ERROR_THRESHOLD}%"
        ;;
esac
echo "=========================================="
awk -v threshold="$ERROR_THRESHOLD" -v mode="$FILTER_MODE" '
BEGIN {
    in_block = 0
    has_errors = 0
}

/^--- Block/ {
    in_block = 1
    has_errors = 0
    block_header = $0 "\n"
    next
}

in_block && (/^HW_Scale/ || /^Index \|/ || /^------\|/) {
    block_header = block_header $0 "\n"
    next
}

in_block && /^ *[0-9]+ \|/ {
    split($0, parts, "|")
    hw_dequant = parts[4]
    hw_err = parts[5]
    ref_dequant = parts[7]
    ref_err = parts[8]

    gsub(/^ +| +$/, "", hw_dequant)
    gsub(/^ +| +$/, "", hw_err)
    gsub(/^ +| +$/, "", ref_dequant)
    gsub(/^ +| +$/, "", ref_err)
    gsub(/%/, "", hw_err)
    gsub(/%/, "", ref_err)

    has_nan_inf = 0
    if (hw_dequant ~ /[nN][aA][nN]/ || hw_dequant ~ /[iI][nN][fF]/ ||
        ref_dequant ~ /[nN][aA][nN]/ || ref_dequant ~ /[iI][nN][fF]/) {
        has_nan_inf = 1
    }

    has_high_error = 0
    if ((hw_err + 0) > threshold || (ref_err + 0) > threshold) {
        has_high_error = 1
    }

    should_print = 0
    if (mode == "nan_inf" && has_nan_inf) {
        should_print = 1
    } else if (mode == "all" && (has_high_error || has_nan_inf)) {
        should_print = 1
    } else if (mode == "errors" && has_high_error) {
        should_print = 1
    }

    if (should_print) {
        if (has_errors == 0) {
            printf "%s", block_header
            has_errors = 1
        }
        print $0
    }
    next
}

/^===/ || /^Overall Statistics/ {
    if (has_errors) {
        print ""
    }
    in_block = 0
}

/^Overall Statistics/ || /^HW  Model:/ || /^Ref Model:/ {
    print $0
}
' "$LOG_FILE" > "$OUTPUT_FILE"

echo ""
echo "Done! Results saved to: $OUTPUT_FILE"
echo ""
echo "Summary:"
grep -E "^(HW|Ref) Model:" "$OUTPUT_FILE" | head -2
echo ""

FILTERED_COUNT=$(grep -c "^ *[0-9]* |" "$OUTPUT_FILE" 2>/dev/null)

case "$FILTER_MODE" in
    nan_inf)
        echo "Found $FILTERED_COUNT entries with NaN/Inf"
        ;;
    all)
        echo "Found $FILTERED_COUNT entries with error > ${ERROR_THRESHOLD}% or NaN/Inf"
        ;;
    errors|*)
        echo "Found $FILTERED_COUNT entries with error > ${ERROR_THRESHOLD}%"
        ;;
esac

NAN_INF_COUNT=$(grep -E "nan|NaN|inf|Inf|-nan|-inf" "$OUTPUT_FILE" 2>/dev/null | grep -c "^ *[0-9]* |")
if [ "$NAN_INF_COUNT" -gt 0 ]; then
    echo "  (including $NAN_INF_COUNT entries with NaN/Inf)"
fi
