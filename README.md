# Osiris
This repository contains the implementation of the Osiris framework discussed 
in the research paper "Osiris: Automated Discovery of Microarchitectural Side Channels" (USENIX Security'21). You can find the paper at the [USENIX website](https://www.usenix.org/system/files/sec21-weber.pdf).

The framework is developed to find microarchitectural side channels in an automated manner.
Currently the implementation supports only x86 processors.

## Supported Platforms
Osiris is developed and tested on Arch Linux and Ubuntu. 
Hence, we expect Osiris to work on other Linux distributions as well but we did not test it.

## Dependencies
#### g++ (requires atleast version 8.0)
- Arch-Linux-package: `gcc`
- Ubuntu-package: `g++`

#### make
- Arch-Linux-package: `make`
- Ubuntu-package: `make`

#### CMake
- Arch-Linux-package: `cmake`  
- Ubuntu-package: `cmake`

#### Capstone  
- Arch-Linux-package: `capstone`  
- Ubuntu-packages: `libcapstone-dev, libcapstone3`

#### OpenSSL
- Arch-Linux-package: `openssl`  
- Ubuntu-package: `libssl-dev`

#### Pwntools (required only for `x86-instructions/xmlToBytes_multi.py`)
On Ubuntu:
```bash
apt-get update
apt-get install python3 python3-pip python3-dev git libssl-dev libffi-dev build-essential
python3 -m pip install --upgrade pip
python3 -m pip install --upgrade pwntools
```
(taken from: https://github.com/Gallopsled/pwntools)

On Arch-Linux:
```bash
pip install pwntools --user
```

## Building
Just install all listed dependencies and execute `./build.sh INTEL` or `./build.sh AMD` for Intel and AMD processors, respectively.

## Noise Reduction
To get precise results Osiris relies on the operating system to reduce the noise of its
measurements. This is done via fixing the processor frequency and by isolating a CPU core.
The script `./run.sh` (described later in this README) will ask for the isolated CPU core.
Osiris will also run without going through these steps but the results will be less reliable, 
hence we **strongly recommend following these steps**.

### Isolating CPU core
One can isolate single CPU cores using the Linux kernel parameter `isolcpus`, e.g.,
adding the parameter `isolcpus=5,11` variable `GRUB_CMDLINE_LINUX_DEFAULT` in `/etc/default/grub` will isolate
the CPU cores 5 and 11. Note that one has to update GRUB after applying these changes, i.e, run `grub-mkconfig -o /boot/grub/grub.cfg`.
It is beneficial to isolate both hardware threads belonging to one CPU core.

You can get a list of your hardware threads and their CPU cores via the following command:
```bash
cat /proc/cpuinfo |grep -E "(core id)|(processor)|(^$)"
```
Two hardware threads (called processors in the above output) belong to the same CPU core if they have the same core id.

### Fixing CPU Frequency
One can fix the CPU frequency of a specific hardware thread using the following command by replacing
`1337` with the number referring to the hardware thread:
```bash
echo "performance" | sudo tee /sys/devices/system/cpu/cpu1337/cpufreq/scaling_governor
```
Note that you should fix the frequency for the same hardware thread which was isolated in the previous
step.
Further note that it can be helpful to note down the content of the `scaing_governor` file to revert
these changes. 
If you missed this opportunity, a simple restart of your operating system should also do the trick.


## Run
### Create x86 Instruction Set (optional)
This step is optional as we already provide the output of it inside `./x86-instructions`.
The instruction set provided by us does not include the sleep pseudo-instruction.
For fuzzing with the sleep pseudo-instruction you have to rerun this step with the corresponding
parameter. 
You can find instructions for this step in the [README](https://github.com/cispa/osiris/tree/main/x86-instructions) of `./x86-instructions`

### Start Fuzzing
You can start the main part of the framework using `./run.sh $CPUCORE` where `$CPUCORE` is
a decimal number representing the CPU core Osiris will be executed on.
We highly recommend using an isolated CPU core with a fixed frequency (see "Noise Reduction").

Executing `./run.sh $CPUCORE` will fuzz with the assumption that the trigger sequence is equal
to the measurement sequence and will execute the trigger sequence architecturally (refer to the research paper for details).
The behavior can be changed with the following parameters:

| Parameter        | Description                                                                            |
| ---------------- | -------------------------------------------------------------------------------------- |
| `--all`          | Removes the assumption that the trigger sequence is equal to the measurement sequence. |
| `--speculation`  | Executes the trigger sequence only in transient execution.                             |
| `--filter-cache` | Removes cache-based side channels from the final report.                               |

(Please note that the positional parameter for the CPU core always has to be the first argument, e.g., `./run.sh 11 --speculation --all`)




The previous script then will leave you with the following (or similar, depending on your parameters) contents in the folder `./build`:
```bash
  # can be ignored (created and needed by the build system)
CMakeCache.txt                                          
CMakeFiles                                              
cmake_install.cmake                                     
Makefile                                                

  # compiled binary
osiris                                                 

  # can be ignored [1]
triggerpairs                                            
triggerpairs-formatted                                  

  # results before confirmation stage (see Output Format)
triggerpairs.csv

  # results of the first confirmation stage (see Output Format)
triggerpairs_confirmed_iter1.csv                        

  # results that passed the first confirmation stage (see Output Format)
triggerpairs_confirmed_iter1_cleaned.csv                

  # results of the second confirmation stage (see Output Format)
triggerpairs_confirmed_iter2.csv                        

  # results that passed the second confirmation stage (see Output Format)
triggerpairs_confirmed_iter2_cleaned.csv                

  # results after confirmation without cache-related side channels
triggerpairs_confirmed_iter2_cleaned_nocache.csv        

  # results after confirmation without cache-related side channels and only 1 
  # instance per unique triple of instruction categories is kept
triggerpairs_confirmed_iter2_cleaned_nocache_filtered_by_all.csv

  # results after confirmation without cache-related side channels and only 1 
  # instance per unique triple of instruction categories is kept. 
  # Additionally, we only keep 1 instance per unique pair of measurement and trigger extensions
triggerpairs_confirmed_iter2_cleaned_nocache_filtered_by_all_mt_extensionpair.csv
```
1: These files are additional outputs from Osiris which exist in 2 forms (machine-readable in `triggerpairs`; human-readable in `triggerpairs-formatted`). 
As these files are only used for development we will not describe them further.

## Output Format

### Instruction File
The file (`./x86-instructions/instructions.b64`) contains descriptions of instruction variants. One per line (excluding the first line).
The Format is as follows:
```
BR;AC;CATEGORY;EXTENSION;ISA-SET
```
With:
- `BR`: A base64-encoded representation of the bytes for the instruction
- `AC`: The human-readable assembly code for the instruction
- `CATEGORY`: The category of the instruction (e.g., arithmetic)
- `EXTENSION`: The extension of the instruction (e.g., 3DNow)
- `ISA-SET`: The ISA-set of the instruction. This is most of the time the same as the extension.

### Osiris Output CSV Files
```
timing;M;T;R
```
With:
- `timing`: The observed timing difference in cycles between executions with and without the trigger sequence
- `M`: the measurement sequence consisting of an internal UID and the properties already mentioned previously (see "Instruction File")
- `T`: the trigger sequence consisting of an internal UID and the properties already mentioned previously (see "Instruction File")
- `R`: the reset sequence consisting of an internal UID and the properties already mentioned previously (see "Instruction File")

### Visualize Output
Many programs exist which can parse these CSV files. 
We like to use Microsoft Excel or LibreOffice Calc.
#### Example
The following is an example of a reported sequence triple. The output is formatted as a table for better readability.

|timing|measurement-uid|measurement-sequence|measurement-category|measurement-extension|measurement-isa-set|trigger-uid|trigger-sequence|trigger-category|trigger-extension|trigger-isa-set|reset-uid|reset-sequence                 |reset-category|reset-extension|reset-isa-set|
|------|---------------|--------------------|--------------------|---------------------|-------------------|-----------|----------------|----------------|-----------------|---------------|---------|-------------------------------|--------------|--------------|--------------|
|182   |551c05b2	   |`INC byte ptr [R8]`	|        BINARY	     |               BASE  |             I86   |551c05b2   |`INC byte ptr [R8]`|	   BINARY|          BASE   |         I86   |551c08ca |`CLFLUSHOPT zmmword ptr [R8]`	 |   CLFLUSHOPT	|   CLFLUSHOPT |   CLFLUSHOPT |


We can see that Osiris observed a timing difference of 182 CPU cycles and that the
measurement sequence, i.e., the sequence whose timing was observed, 
and the trigger sequence, i.e. the instruction sequence that lead to a timing delta, 
were `INC byte ptr [R8]`. 
The reset sequence, i.e. the sequence that reset the microarchitectural state, was `CLFLUSH zmmword ptr [R8]`.
Flushing a memory address from the CPU cache and triggering a side channel by loading it again is a well-known side channel known as [Flush+Reload](https://www.usenix.org/system/files/conference/usenixsecurity14/sec14-paper-yarom.pdf).


## Contact
If there are questions regarding this tool, please send an email to `daniel.weber (AT) cispa.saarland` or message `@weber_daniel` on Twitter.

## Research Paper
The paper is available at the [USENIX website](https://www.usenix.org/system/files/sec21-weber.pdf). 
You can cite our work with the following BibTeX entry:
```latex
@inproceedings{Weber2021,
 author = {Weber, Daniel and Ibrahim, Ahmad and Nemati, Hamed and Schwarz, Michael and Rossow, Christian},
 booktitle = {USENIX Security Symposium},
 title = {{Osiris: Automatic Discovery of Microarchitectural Side Channels}},
 year = {2021}
}
```

## Disclaimer
We are providing this code as-is. 
You are responsible for protecting yourself, your property and data, and others from any risks caused by this code. 
This code may cause unexpected and undesirable behavior to occur on your machine. 
