#!/bin/bash

# Configuration - All parameters are optional
QUERIES_PER_MODEL=${1:-5}
THREADS=${2:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)}  # Use all available CPU cores by default
OUTPUT_DIR=${3:-"./query_results_$(date +%Y%m%d_%H%M%S)"}  # Default to current directory
DURATION_HOURS=${4:-24}  # Run for 24 hours by default
QUERY_GENERATOR="./src/query_generator/query_generator"

# Calculate end time (current time + duration in seconds)
END_TIME=$(($(date +%s) + (DURATION_HOURS * 3600)))

echo "=== Enhanced Query Runner ==="
echo "Duration: $DURATION_HOURS hours"
echo "Threads: $THREADS"
echo "Queries per model per cycle: $QUERIES_PER_MODEL"
echo "Output directory: $OUTPUT_DIR"
echo "End time: $(date -r $END_TIME 2>/dev/null || date -jf %s $END_TIME 2>/dev/null || date)"  # macOS and Linux compatibility
echo "================================"

# Validation checks
if [[ ! -x "$QUERY_GENERATOR" ]]; then
    echo "Error: Query generator '$QUERY_GENERATOR' not found or not executable!"
    exit 1
fi

if ! command -v ollama &> /dev/null; then
    echo "Error: Ollama not found!"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"
if [[ $? -ne 0 ]]; then
    echo "Error: Cannot create output directory '$OUTPUT_DIR'"
    exit 1
fi

# Get available models
MODELS=($(ollama list | tail -n +2 | awk '{print $1}' | grep -v '^$'))

if [[ ${#MODELS[@]} -eq 0 ]]; then
    echo "Error: No Ollama models found!"
    exit 1
fi

echo "Found models: ${MODELS[*]}"

# Initialize counters and statistics
declare -A MODEL_COUNTERS
declare -A MODEL_SUCCESSES
declare -A MODEL_FAILURES

for model in "${MODELS[@]}"; do
    MODEL_COUNTERS["$model"]=0
    MODEL_SUCCESSES["$model"]=0
    MODEL_FAILURES["$model"]=0
done

TOTAL_QUERIES=0
CYCLE_COUNT=0

# Function to run a single query (no stats update here)
run_query() {
    local model="$1"
    local query_num="$2"
    local cycle="$3"
    local timestamp
    timestamp=$(date '+%Y%m%d_%H%M%S')
    local output_file="${OUTPUT_DIR}/output_${model//:/_}_cycle${cycle}_query${query_num}_${timestamp}.txt"
    local log_file="${OUTPUT_DIR}/execution.log"
    local status_file="${OUTPUT_DIR}/status_${model//:/_}_cycle${cycle}_query${query_num}_${timestamp}.txt"

    echo "[$(date)] Thread-$query_num: Starting query with model: $model (Cycle: $cycle)" | tee -a "$log_file"

    if timeout 300 "$QUERY_GENERATOR" generate --model="$model" > "$output_file" 2>&1; then
        echo "SUCCESS" > "$status_file"
        echo "[$(date)] Thread-$query_num: SUCCESS - Completed query with model: $model (Cycle: $cycle)" | tee -a "$log_file"
    else
        echo "FAILURE" > "$status_file"
        echo "[$(date)] Thread-$query_num: FAILED - Query with model: $model (Cycle: $cycle)" | tee -a "$log_file"
        echo "Error occurred at $(date)" >> "$output_file"
    fi
}

# Function to print statistics
print_stats() {
    local stats_file="${OUTPUT_DIR}/statistics.txt"
    {
        echo "=== Query Statistics ==="
        echo "Report generated: $(date)"
        echo "Total cycles completed: $CYCLE_COUNT"
        echo "Total queries executed: $TOTAL_QUERIES"
        echo ""
        echo "Per-model statistics:"
        for model in "${MODELS[@]}"; do
            local total=${MODEL_COUNTERS["$model"]}
            local success=${MODEL_SUCCESSES["$model"]}
            local failure=${MODEL_FAILURES["$model"]}
            local success_rate=0
            if [[ ${total:-0} -gt 0 ]]; then
                success_rate=$(echo "scale=2; $success * 100 / $total" | bc -l 2>/dev/null || echo "0")
            fi
            echo "  $model: Total=${total:-0}, Success=${success:-0}, Failed=${failure:-0}, Success Rate=${success_rate}%"
        done
        echo ""
        echo "Output files are stored in: $OUTPUT_DIR"
    } | tee "$stats_file"
}

# Function to handle cleanup on exit
cleanup() {
    echo ""
    echo "Shutting down gracefully..."
    print_stats
    echo "Final statistics saved to ${OUTPUT_DIR}/statistics.txt"
    echo "All output files are in: $OUTPUT_DIR"
    exit 0
}

# Set up signal handlers for graceful shutdown
trap cleanup SIGINT SIGTERM

# Main execution loop
echo "Starting continuous execution for $DURATION_HOURS hours..."
echo "Press Ctrl+C to stop gracefully"

while [[ $(date +%s) -lt $END_TIME ]]; do
    CYCLE_COUNT=$((CYCLE_COUNT + 1))
    echo ""
    echo "=== Starting Cycle $CYCLE_COUNT ==="
    local_now=$(date +%s)
    time_left=$((END_TIME - local_now))
    echo "Time remaining: $(( time_left / 3600 )) hours $(( (time_left % 3600) / 60 )) minutes"

    for model in "${MODELS[@]}"; do
        (
            echo "Cycle $CYCLE_COUNT: Starting $QUERIES_PER_MODEL queries with model: $model"
            for ((i=1; i<=QUERIES_PER_MODEL; i++)); do
                run_query "$model" "$i" "$CYCLE_COUNT" &
                # No stats update here
                if (( i % THREADS == 0 )); then
                    wait
                fi
            done
            wait
            echo "Cycle $CYCLE_COUNT: Completed all queries for $model"
        ) &
    done

    wait

    # Aggregate stats after all queries in this cycle
    for model in "${MODELS[@]}"; do
        for status_file in "${OUTPUT_DIR}/status_${model//:/_}_cycle${CYCLE_COUNT}_query"*".txt"; do
            if [[ -f "$status_file" ]]; then
                status=$(cat "$status_file")
                if [[ "$status" == "SUCCESS" ]]; then
                    MODEL_SUCCESSES["$model"]=$(( ${MODEL_SUCCESSES["$model"]:-0} + 1 ))
                elif [[ "$status" == "FAILURE" ]]; then
                    MODEL_FAILURES["$model"]=$(( ${MODEL_FAILURES["$model"]:-0} + 1 ))
                fi
                MODEL_COUNTERS["$model"]=$(( ${MODEL_COUNTERS["$model"]:-0} + 1 ))
                TOTAL_QUERIES=$((TOTAL_QUERIES + 1))
                rm -f "$status_file"
            fi
        done
    done

    echo "=== Completed Cycle $CYCLE_COUNT ==="

    # Print periodic statistics
    if ((CYCLE_COUNT % 5 == 0)); then
        print_stats
    fi

    # Small delay between cycles to prevent overwhelming the system
    sleep 2
done

echo ""
echo "=== Execution completed after $DURATION_HOURS hours ==="
print_stats

# Create a summary file
{
    echo "Query Runner Execution Summary"
    echo "============================="
    start_time=$(date -r $((END_TIME - DURATION_HOURS * 3600)) 2>/dev/null || date -jf %s $((END_TIME - DURATION_HOURS * 3600)) 2>/dev/null || date)
    echo "Start time: $start_time"
    echo "End time: $(date)"
    echo "Duration: $DURATION_HOURS hours"
    echo "Total cycles: $CYCLE_COUNT"
    echo "Total queries: $TOTAL_QUERIES"
    echo "Models used: ${MODELS[*]}"
    echo "Threads used: $THREADS"
    echo "Output directory: $OUTPUT_DIR"
    echo ""
    echo "All results and logs are stored in the output directory."
} > "${OUTPUT_DIR}/execution_summary.txt"

echo "Execution summary saved to ${OUTPUT_DIR}/execution_summary.txt"
echo "All files are organized in: $OUTPUT_DIR"