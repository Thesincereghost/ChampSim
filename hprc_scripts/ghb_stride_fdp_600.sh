#!/bin/bash
#SBATCH --job-name=ghb_stride_fdp_600   # Job name
#SBATCH --mail-type=END,FAIL         # Mail Events (NONE,BEGIN,FAIL,END,ALL)
#SBATCH --mail-user=vaishnav.g@tamu.edu   # Replace with your email address
#SBATCH --ntasks=1                   # Run on a single CPU
#SBATCH --mem=2560M                  # Request 2560MB (2.5GB) per node
#SBATCH --time=12:00:00              # Time limit hh:mm:ss
#SBATCH --output=%x.log              # Standard output and error log

module load GCCcore/13.2.0
echo "Running 600.perlbench_s-210B.champsimtrace.xz"
../bin/ghb_stride_fdp \
--warmup_instructions 100000000 \
--simulation_instructions 500000000 \
../dpc3_traces/600.perlbench_s-210B.champsimtrace.xz \
> ../results/ghb_stride_fdp/ghb_stride_fdp_600.txt
echo "Finished running 600.perlbench_s-210B.champsimtrace.xz"
