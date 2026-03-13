#!/bin/bash
#SBATCH --job-name=bsbs   # Job name
#SBATCH --mail-type=NONE            # Mail events (NONE, BEGIN, END, FAIL, ALL)
#SBATCH --mail-user=dwt@cs.unh.edu   # Where to send mail	
#SBATCH --ntasks=1                  # Run a single task
#SBATCH --array=1-12300                 # Array range
#SBATCH --time=0:5:00
#SBATCH --mem=60G
#SBATCH --no-kill
#SBATCH -p compute
eval "$(head -${SLURM_ARRAY_TASK_ID} bsbs_test.sh | tail -1)"
