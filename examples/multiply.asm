; multiply two numbers by repeated addition: r3 = r1 * r2.
; the machine has no multiply instruction, so we build it from add and a loop.
; r0 is zero, r4 is one, r5 holds the loop address for an unconditional jmp.

  set r0, 0
  set r1, 6        ; first factor
  set r2, 7        ; second factor
  set r3, 0        ; running product
  set r4, 1
  set r5, loop
loop:
  beq r2, r0, done ; when the second factor reaches zero we are finished
  add r3, r3, r1   ; product += first factor
  sub r2, r2, r4   ; second factor -= 1
  jmp r5
done:
  out r3           ; prints 42
  hlt
