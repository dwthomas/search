#!/bin/bash
#SBATCH --job-name=pkjobs
#SBATCH --mail-type=NONE
#SBATCH --mail-user=dwt@cs.unh.edu
#SBATCH --ntasks=1
#SBATCH --array=1-256
#SBATCH --time=15:5:00
#SBATCH --mem=60G
#SBATCH --no-kill
#SBATCH -p compute
eval "/home/aifs2/dwr29/search/pkjobs/job_${SLURM_ARRAY_TASK_ID}.sh"
