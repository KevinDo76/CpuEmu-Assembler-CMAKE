<p>This is an assembler for the cpu emulator from <a href="https://github.com/KevinDo76/CpuEmu-CMAKE">here</a>. The assembly syntax is inspired by NASM</p>

<p>Design Goals</p>
<ul>
  <li>Assemble to bytecode for execution</li>
  <li>Labels</li>
  <li>A way to create string</li>
  <li>A way to create bytes, dword, etc</li>
  <li>A way to create arrays</li>
  
</ul>

<p>
start:
    #setting up the stack
    mov sp 0
    mov bp 0x250

    #start
    mov ra message
    call print

    halt

# address of the message in ra
print:
    pushreg
.print_loop:
    mov cmpreg 0
    mov rc 0

    readptr1 rb ra
    out rb 0

    cmp rb rb rc
    mov rc .print_done
    jmpif rb rc

    inc ra
    jmpimm .print_loop

.print_done:
    popreg
    ret


message:
    string "Hello World!" 10 0
</p>
