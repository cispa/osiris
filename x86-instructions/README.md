# Creating x86 Instruction Set

The following instructions generate a machine-readable list of all x86 instructions that Osiris
uses to generate instruction sequence triples.

## Run
Assemble all instructions provided by the instruction.xml from [uops.info](https://www.uops.info/xml.html)
using the following command:
```bash
./fetch-uops-info-xml.sh
./xmlToBytes.py instructions.b64
```
(as this step takes a couple minutes, we already did that for you and added the resulting `instructions.b64` to this repo)

If you want to add the sleep pseudo-instruction to the instruction set, you have to use the following command instead:
```bash
./fetch-uops-info-xml.sh
./xmlToBytes.py instructions.b64 --include-sleep
```
