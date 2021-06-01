#! /usr/bin/python3

"""
Modified version of Andreas Abel's xmlToAssembler.py [1].
[1]: https://github.com/andreas-abel/XED-to-XML/blob/master/xmlToAssembler.py
"""

import xml.etree.ElementTree as ET
from multiprocessing import Process, Queue
import base64
from pwn import *

THREAD_COUNT = 10


class AssemblyInstruction:
    base64_byte_representation = b""

    def __init__(self, assembly_code, category, extension, isa_set):
        self.assembly_code = assembly_code.encode()
        self.category = category.encode()
        self.extension = extension.encode()
        self.isa_set = isa_set.encode()

    def get_assembly_code(self):
        return self.assembly_code.decode()

    def set_base64_encoded_byte_representation(self, byte_representation):
        self.base64_byte_representation = byte_representation

    def get_csv_representation(self, delim=b";"):
        assert self.base64_byte_representation != b""
        assert delim not in self.assembly_code
        assert delim not in self.category
        assert delim not in self.extension
        assert delim not in self.isa_set

        csv_line = self.base64_byte_representation
        csv_line += delim
        csv_line += self.assembly_code
        csv_line += delim
        csv_line += self.category
        csv_line += delim
        csv_line += self.extension
        csv_line += delim
        csv_line += self.isa_set

        return csv_line


def worker_func(input_queue, output_queue):
    while True:
        assembly_instruction = input_queue.get()
        asm_code = assembly_instruction.get_assembly_code()
        try:
            asm_bytes = asm(asm_code)
            asm_bytes_encoded = base64.b64encode(asm_bytes)
            assembly_instruction.set_base64_encoded_byte_representation(asm_bytes_encoded)
            output_queue.put((True, assembly_instruction))
        except pwnlib.exception.PwnlibException as e:
            print(f"Error on assembling instruction '{asm_code}'")
            print(f"Error was: {e}")
            output_queue.put((False, None))


def main():
    if len(sys.argv) != 2:
        print(f"USAGE: {sys.argv[0]} <output-filename>")
        exit(0)
    output_fname = sys.argv[1]

    context.arch = "amd64"
    root = ET.parse('instructions.xml')

    pseudo_instructions = list()

    # add custom pseudoinstruction
    busy_sleep = b"ULgAABAASP/ISIXAdfhY"
    busy_sleep_pseudo_inst = AssemblyInstruction("busy-sleep",
                                                 "pseudo-instruction",
                                                 "pseudo-instruction",
                                                 "pseudo-instruction")
    busy_sleep_pseudo_inst.set_base64_encoded_byte_representation(busy_sleep)
    pseudo_instructions.append(busy_sleep_pseudo_inst)

    assembly_instructions = list()
    for instrNode in root.iter('instruction'):
        # Future instruction set extensions
        if instrNode.attrib['extension'] in ['CLDEMOTE', 'ENQCMD', 'MCOMMIT', 'MOVDIR', 'PCONFIG',
                                             'RDPRU', 'SERIALIZE', 'SNP', 'TSX_LDTRK', 'WAITPKG',
                                             'WBNOINVD']:
            continue
        if any(x in instrNode.attrib['isa-set'] for x in ['BF16_', 'VP2INTERSECT']):
            continue

        asm_code = instrNode.attrib['asm']
        first = True
        # blacklist instructions that GAS cannot assemble
        blacklisted_instructions = ["SYSENTER", "SYSEXIT"]
        # blacklist instructions that Osiris cant handle
        blacklisted_instructions += ["RET", "LEAVE", "LEAVEW", "ENTER", "ENTERW", "LFS", "POPFQ",
                                     "POPFW", "WRFSBASE", "WRGSBASE"]
        blacklisted_iforms = ["POP_FS"]
        # blacklist instructions that osiris can't handle
        blacklisted_instruction_categories = ["UNCOND_BR", "COND_BR", "SYSCALL", "CALL", "RET"]
        if asm_code in blacklisted_instructions:
            continue
        if instrNode.attrib['iform'] in blacklisted_iforms:
            continue
        if instrNode.attrib['category'] in blacklisted_instruction_categories:
            continue

        for operandNode in instrNode.iter('operand'):
            operandIdx = int(operandNode.attrib['idx'])

            if operandNode.attrib.get('suppressed', '0') == '1':
                continue

            if not first and not operandNode.attrib.get('opmask', '') == '1':
                asm_code += ', '
            else:
                asm_code += ' '
                first = False

            if operandNode.attrib['type'] == 'reg':
                registers = operandNode.text.split(',')
                register = registers[min(operandIdx, len(registers) - 1)]
                if not operandNode.attrib.get('opmask', '') == '1':
                    asm_code += register
                else:
                    asm_code += '{' + register + '}'
                    if instrNode.attrib.get('zeroing', '') == '1':
                        asm_code += '{z}'
            elif operandNode.attrib['type'] == 'mem':
                memoryPrefix = operandNode.attrib.get('memory-prefix', '')
                if memoryPrefix:
                    asm_code += memoryPrefix + ' '

                if operandNode.attrib.get('VSIB', '0') != '0':
                    asm_code += '[' + operandNode.attrib.get('VSIB') + '0]'
                else:
                    asm_code += '[R8]'

                memorySuffix = operandNode.attrib.get('memory-suffix', '')
                if memorySuffix:
                    asm_code += ' ' + memorySuffix
            elif operandNode.attrib['type'] == 'agen':
                agen = instrNode.attrib['agen']
                address = []

                if 'R' in agen: address.append('R8')
                if 'B' in agen: address.append('R8')
                if 'I' in agen: address.append('2*RBX')
                if 'D' in agen: address.append('8')

                asm_code += ' [' + '+'.join(address) + ']'
            elif operandNode.attrib['type'] == 'imm':
                if instrNode.attrib.get('roundc', '') == '1':
                    asm_code += '{rn-sae}, '
                elif instrNode.attrib.get('sae', '') == '1':
                    asm_code += '{sae}, '
                width = int(operandNode.attrib['width'])
                if operandNode.attrib.get('implicit', '') == '1':
                    imm = operandNode.text
                else:
                    imm = (1 << (width - 8)) + 1
                asm_code += str(imm)
            elif operandNode.attrib['type'] == 'relbr':
                asm_code = '1: ' + asm_code + '1b'

        if not 'sae' in asm_code:
            if instrNode.attrib.get('roundc', '') == '1':
                asm_code += ', {rn-sae}'
            elif instrNode.attrib.get('sae', '') == '1':
                asm_code += ', {sae}'

        extension = instrNode.attrib['extension']
        category = instrNode.attrib['category']
        isa_set = instrNode.attrib['isa-set']
        assembly_instruction = AssemblyInstruction(asm_code, category, extension, isa_set)
        assembly_instructions.append(assembly_instruction)

    # load work
    input_queue = Queue()
    output_queue = Queue()
    for instruction in assembly_instructions:
        input_queue.put(instruction)

    # start workers
    workers = list()
    instructions_written = 0
    try:
        for _ in range(THREAD_COUNT):
            p = Process(target=worker_func, args=(input_queue, output_queue))
            p.start()
            workers.append(p)

        # get worker results
        progress_steps = len(assembly_instructions) // 1000
        progress = 0

        with open(output_fname, "wb") as fd:
            headerline = b"byte_representation;assembly_code;category;extension;isa_set"
            fd.write(headerline + b"\n")
            for i in range(len(assembly_instructions)):
                if i % progress_steps == 0:
                    print("[*] Progress {:.1f}%".format(progress))
                    progress += 0.1
                succ, assembly_instruction = output_queue.get()
                if succ:
                    fd.write(assembly_instruction.get_csv_representation() + b"\n")
                    instructions_written += 1
            for instruction in pseudo_instructions:
                fd.write(instruction.get_csv_representation() + b"\n")
                instructions_written += 1

    except KeyboardInterrupt:
        print("[+] Aborting!")
    finally:
        for worker in workers:
            worker.terminate()
    print(f"[+] {instructions_written}/{len(assembly_instructions)} instructions stored in %s"
          % output_fname)


if __name__ == "__main__":
    main()
