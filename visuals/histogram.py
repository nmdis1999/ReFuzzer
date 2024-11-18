import os

import matplotlib.pyplot as plt

MAX_FILES = 1200


def histogram(file_path, buckets=[50, 100, 200, 500]):
    bucket_counts = [0] * (len(buckets) + 1)
    file_counts = []
    processed_files = 0
    skipped_files = 0

    # Get all .c files first
    c_files = [f for f in os.listdir(file_path) if f.endswith(".c")]

    # If more than MAX_FILES, take only the first MAX_FILES
    if len(c_files) > MAX_FILES:
        print(
            f"Warning: Found {len(c_files)} .c files. Processing only the first {MAX_FILES} files."
        )
        c_files = c_files[:MAX_FILES]
        skipped_files = len(c_files) - MAX_FILES

    for file in c_files:
        full_path = os.path.join(file_path, file)
        if os.path.isfile(full_path):
            try:
                with open(full_path, "r", encoding="utf-8") as f:
                    loc = sum(1 for line in f if line.strip())
                file_counts.append(loc)

                for i, threshold in enumerate(buckets):
                    if loc < threshold:
                        bucket_counts[i] += 1
                        break
                else:
                    bucket_counts[-1] += 1

                processed_files += 1

            except Exception as e:
                print(f"Error processing {file}: {e}")

    bucket_labels = []
    for i, threshold in enumerate(buckets):
        if i == 0:
            bucket_labels.append(f"<{threshold}")
        else:
            bucket_labels.append(f"{buckets[i-1]}-{threshold}")
    bucket_labels.append(f"≥{buckets[-1]}")

    # Create figure with higher DPI for better quality
    plt.figure(figsize=(10, 6), dpi=300)

    # Create bars with enhanced styling
    bars = plt.bar(
        bucket_labels,
        bucket_counts,
        color="#2E86C1",  # Deeper blue
        edgecolor="black",
        linewidth=1,
        alpha=0.8,
    )

    # Enhance title and labels
    plt.title(
        f"Distribution of C Files by Lines of Code\n(Max {MAX_FILES} files)",
        fontsize=14,
        pad=20,
    )
    plt.xlabel("Lines of Code", fontsize=12)
    plt.ylabel("Number of Files", fontsize=12)

    # Enhanced grid
    plt.grid(True, axis="y", linestyle="--", alpha=0.3)

    # Rotate and align the tick labels so they look better
    plt.xticks(rotation=45, ha="right")

    # Add value labels on top of each bar
    for bar in bars:
        height = bar.get_height()
        if height > 0:
            plt.text(
                bar.get_x() + bar.get_width() / 2.0,
                height,
                f"{int(height)}",
                ha="center",
                va="bottom",
            )

    # Adjust layout to prevent label cutoff
    plt.tight_layout()

    # Save both PNG and PDF versions
    # First, ensure we're saving in the correct directory (visuals)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    pdf_path = os.path.join(script_dir, "c_files_loc_histogram.pdf")
    png_path = os.path.join(script_dir, "c_files_loc_histogram.png")

    plt.savefig(pdf_path, format="pdf", bbox_inches="tight")
    plt.savefig(png_path, format="png", dpi=300, bbox_inches="tight")

    print(f"Saved histogram images to:\n{pdf_path}\n{png_path}")

    # Close the figure to free memory
    plt.close()

    # Print statistics
    total_files = sum(bucket_counts)
    total_loc = sum(file_counts)

    print(f"\nAnalysis Results:")
    print(f"Processed .c files: {processed_files}")
    if skipped_files > 0:
        print(f"Skipped files (over {MAX_FILES} limit): {skipped_files}")
    print(f"Total lines of code: {total_loc}")

    if total_files > 0:
        print(f"\nFile Statistics:")
        print(f"Average lines per file: {total_loc/total_files:.1f}")
        print(f"Smallest file: {min(file_counts)} lines")
        print(f"Largest file: {max(file_counts)} lines")
        print(f"\nDistribution by Size:")
        for label, count in zip(bucket_labels, bucket_counts):
            percentage = count / total_files * 100
            print(f"{label}: {count} files ({percentage:.1f}%)")

    return bucket_counts, file_counts


if __name__ == "__main__":
    home = os.path.expanduser("~")
    directory = os.path.join(home, "research", "ollama", "generate_c_code", "test")
    buckets = [50, 100, 200, 500]
    bucket_counts, file_counts = histogram(directory, buckets)
