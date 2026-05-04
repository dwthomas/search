#!/bin/bash
#SBATCH --job-name=pk20jobs
#SBATCH --mail-type=NONE
#SBATCH --mail-user=dwt@cs.unh.edu
#SBATCH --ntasks=1
#SBATCH --array=1-512
#SBATCH --time=15:5:00
#SBATCH --mem=60G
#SBATCH --no-kill
#SBATCH -p compute
eval "/home/aifs2/dwr29/search/pk20jobs/job_${SLURM_ARRAY_TASK_ID}.sh"
