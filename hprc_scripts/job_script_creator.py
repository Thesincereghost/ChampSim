import os

# Ensure the script is executed from the hprc_scripts folder
scripts = []
executables = ['ip_stride', 'ghb_stride', 'ghb_stride_fdp']
benchmarks = [
    '600.perlbench_s-210B.champsimtrace.xz',
    '602.gcc_s-734B.champsimtrace.xz',
    '603.bwaves_s-3699B.champsimtrace.xz',
    '605.mcf_s-665B.champsimtrace.xz',
    '607.cactuBSSN_s-2421B.champsimtrace.xz',
    '619.lbm_s-4268B.champsimtrace.xz',
    '620.omnetpp_s-874B.champsimtrace.xz',
    '621.wrf_s-575B.champsimtrace.xz',
    '623.xalancbmk_s-700B.champsimtrace.xz',
    '625.x264_s-18B.champsimtrace.xz',
    '627.cam4_s-573B.champsimtrace.xz',
    '628.pop2_s-17B.champsimtrace.xz',
    '631.deepsjeng_s-928B.champsimtrace.xz',
    '638.imagick_s-10316B.champsimtrace.xz',
    '641.leela_s-800B.champsimtrace.xz',
    '644.nab_s-5853B.champsimtrace.xz',
    '648.exchange2_s-1699B.champsimtrace.xz',
    '649.fotonik3d_s-1176B.champsimtrace.xz',
    '654.roms_s-842B.champsimtrace.xz',
    '657.xz_s-3167B.champsimtrace.xz'
]

# Create the results folder if it doesn't exist
results_folder = "../results"
os.makedirs(results_folder, exist_ok=True)

for executable in executables:
    # Create a subfolder for each executable in the results folder
    executable_results_folder = os.path.join(results_folder, executable)
    os.makedirs(executable_results_folder, exist_ok=True)

    for benchmark in benchmarks:
        job_script_sample = f"""#!/bin/bash
#SBATCH --job-name={executable}_{benchmark[0:3]}   # Job name
#SBATCH --mail-type=END,FAIL         # Mail Events (NONE,BEGIN,FAIL,END,ALL)
#SBATCH --mail-user=vaishnav.g@tamu.edu   # Replace with your email address
#SBATCH --ntasks=1                   # Run on a single CPU
#SBATCH --mem=2560M                  # Request 2560MB (2.5GB) per node
#SBATCH --time=12:00:00              # Time limit hh:mm:ss
#SBATCH --output=%x.log              # Standard output and error log

module load GCCcore/13.2.0
echo "Running {benchmark}"
../bin/{executable} \\
--warmup_instructions 100000000 \\
--simulation_instructions 500000000 \\
/scratch/user/vaishnav.g/ecen676/hw1/ChampSim/dpc3_traces/{benchmark} \\
> {results_folder}/{executable}/{executable}_{benchmark[0:3]}.txt
echo "Finished running {benchmark}"
"""
        # Save the job script in the hprc_scripts folder
        script_path = f"{executable}_{benchmark[0:3]}.sh"
        with open(script_path, "w", newline='\n') as f:
            f.write(job_script_sample)
        scripts.append(f"sbatch {script_path}\n")

# Save the submit_all_jobs.sh script in the hprc_scripts folder
with open("submit_all_jobs.sh", "w", newline='\n') as f:
    f.writelines(scripts)