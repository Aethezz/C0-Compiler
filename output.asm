.data
  fmt: .asciz "%d\n"
.text
  .globl main
main:
  ld a0, 0(sp)
  addi sp, sp, -8
  sd a0, 0(sp)
  ld a0, 0(sp)
  addi sp, sp, -8
  sd a0, 0(sp)
  li a0, 0
  addi sp, sp, -8
  sd a0, 0(sp)
  ld a0, 0(sp)
  addi sp, sp, -8
  sd a0, 0(sp)
  ld a0, 0(sp)
  addi sp, sp, -8
  sd a0, 0(sp)
  ld a1, 0(sp)
  addi sp, sp, 8
  li a7, 93
  ecall
  ld a0, 0(sp)
  addi sp, sp, 8
  sub a0, a1, a0
  addi sp, sp, -8
  sd a0, 0(sp)
  ld a0, 0(sp)
  addi sp, sp, -8
  sd a0, 0(sp)
  li a7, 93
  ecall
  ld a1, 0(sp)
  addi sp, sp, 8
  ld a0, 0(sp)
  addi sp, sp, 8
