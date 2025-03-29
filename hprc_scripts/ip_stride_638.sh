#!/bin/bash
#SBATCH --job-name=ip_stride_638   # Job name
#SBATCH --mail-type=END,FAIL         # Mail Events (NONE,BEGIN,FAIL,END,ALL)
#SBATCH --mail-user=vaishnav.g@tamu.edu   # Replace with your email address
#SBATCH --ntasks=1                   # Run on a single CPU
#SBATCH --mem=2560M                  # Request 2560MB (2.5GB) per node
#SBATCH --time=12:00:00              # Time limit hh:mm:ss
#SBATCH --output=%x.log              # Standard output and error log

module load GCCcore/13.2.0
echo "Running 638.imagick_s-10316B.champsimtrace.xz"
../bin/ip_stride \
--warmup_instructions 100000000 \
--simulation_instructions 500000000 \
../dpc3_traces/638.imagick_s-10316B.champsimtrace.xz \
> ../results/ip_stride/ip_stride_638.txt
echo "Finished running 638.imagick_s-10316B.champsimtrace.xz"
