.data
fmt: .asciz "%d\n" # Format string for printing integers

.text
.extern printf # Declare printf if used
.globl main

main:
  # Function Prologue (RV32)
  addi sp, sp, -4
  sw ra, 0(sp)
  addi sp, sp, -4
  sw s0, 0(sp)
  mv s0, sp
  addi sp, sp, -128 # Allocate initial stack space (adjust size as needed)

  # Start of generated code from AST
  # Variable Declaration: x at -4(s0)
  li a0, 42
  sw a0, -4(s0)
  # WHILE Loop
L0:
  lw a0, -4(s0)
  ble a0, 0, L1
  # WHILE Body
  # Entering Block
  lw a0, -4(s0)
  addi a0, a0, -1
  # Assignment: x = ...
  sw a0, -4(s0)
  # Exiting Block
  j L0
L1:
  # END WHILE
  li a0, 0
  li a7, 93
  ecall
  # End of generated code from AST

  # Function Epilogue (RV32)
  mv sp, s0
  lw s0, 0(sp)
  addi sp, sp, 4
  lw ra, 0(sp)
  addi sp, sp, 4
  ret

