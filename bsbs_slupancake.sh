#!/bin/bash
#SBATCH --job-name=bsbspk   # Job name
#SBATCH --mail-type=NONE            # Mail events (NONE, BEGIN, END, FAIL, ALL)
#SBATCH --mail-user=dwt@cs.unh.edu   # Where to send mail	
#SBATCH --ntasks=1                  # Run a single task
#SBATCH --array=1-33900                 # Array range
#SBATCH --time=0:5:00
#SBATCH --mem=60G
#SBATCH --no-kill
#SBATCH -p compute
eval "$(head -${SLURM_ARRAY_TASK_ID} pancakeexp.sh | tail -1)"
