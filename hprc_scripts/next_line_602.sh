#!/bin/bash
#SBATCH --job-name=next_line_602   # Job name
#SBATCH --mail-type=END,FAIL         # Mail Events (NONE,BEGIN,FAIL,END,ALL)
#SBATCH --mail-user=vaishnav.g@tamu.edu   # Replace with your email address
#SBATCH --ntasks=1                   # Run on a single CPU
#SBATCH --mem=2560M                  # Request 2560MB (2.5GB) per node
#SBATCH --time=12:00:00              # Time limit hh:mm:ss
#SBATCH --output=%x.log              # Standard output and error log

module load GCCcore/13.2.0
echo "Running 602.gcc_s-734B.champsimtrace.xz"
../bin/next_line \
--warmup_instructions 100000000 \
--simulation_instructions 500000000 \
/scratch/user/vaishnav.g/ecen676/hw1/ChampSim/dpc3_traces/602.gcc_s-734B.champsimtrace.xz \
> ../results/next_line/next_line_602.txt
echo "Finished running 602.gcc_s-734B.champsimtrace.xz"
