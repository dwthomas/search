#! /usr/bin/env python

#!/usr/bin/env python3

import argparse
import os
import stat
import sys
from pathlib import Path
import numpy as np

def split_evenly(lines, n_jobs):
    # Randomly shuffle the lines
    indices = np.random.permutation(len(lines))
    shuffled_lines = [lines[i] for i in indices]
    
    total = len(shuffled_lines)
    base = total // n_jobs
    remainder = total % n_jobs

    chunks = []
    start = 0
    for i in range(n_jobs):
        size = base + (1 if i < remainder else 0)
        end = start + size
        chunks.append(shuffled_lines[start:end])
        start = end
    return chunks


def main():
    parser = argparse.ArgumentParser(
        description="Split a shell script from stdin into N job scripts."
    )
    parser.add_argument(
        "n_jobs",
        type=int,
        help="Number of output job scripts to create",
    )

    parser.add_argument(
        "--timeout",
        type=float,
        default=5.0,
        help="Timeout for each job in minutes (default: 5.0)",
    )
    parser.add_argument(
        "--memory",
        type=float,
        default=60.0,
        help="Memory limit for each job in GB (default: 60.0)",
    )

    parser.add_argument(
        "jobs_dir",
        type=str,
        help="Directory to write job scripts to",
    )
    args = parser.parse_args()
    if args.n_jobs <= 0:
        print("Error: n_jobs must be a positive integer.", file=sys.stderr)
        sys.exit(2)

    # Preserve original newlines exactly as read.
    lines = sys.stdin.read().splitlines(keepends=True)
    jobs_dir = Path(args.jobs_dir)
    jobs_dir.mkdir(exist_ok=True)

    chunks = split_evenly(lines, args.n_jobs)

    for i, chunk in enumerate(chunks, start=1):
        out_path = jobs_dir / f"job_{i}.sh"
        with out_path.open("w", encoding="utf-8") as f:
            for line in chunk:
                memory_limit = int(args.memory * 1024 * 1024)
                f.write(f"ulimit -v {memory_limit} && timeout {args.timeout}m " + line)

        # Ensure executable bit is set.
        mode = out_path.stat().st_mode
        out_path.chmod(mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

        # Create SLURM script
        slurm_path = jobs_dir / "slurm_submit.sh"
        with slurm_path.open("w", encoding="utf-8") as f:
            f.write("#!/bin/bash\n")
            f.write(f"#SBATCH --job-name={jobs_dir.name}\n")
            f.write("#SBATCH --mail-type=NONE\n")
            f.write("#SBATCH --mail-user=dwt@cs.unh.edu\n")
            f.write("#SBATCH --ntasks=1\n")
            f.write(f"#SBATCH --array=1-{args.n_jobs}\n")
            f.write("#SBATCH --time=15:5:00\n")
            f.write("#SBATCH --mem=60G\n")
            f.write("#SBATCH --no-kill\n")
            f.write("#SBATCH -p compute\n")
            f.write('eval "job_${SLURM_ARRAY_TASK_ID}.sh"\n')
        
        mode = slurm_path.stat().st_mode
        slurm_path.chmod(mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
if __name__ == "__main__":
    main()