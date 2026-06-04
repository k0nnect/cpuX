; print the first ten fibonacci numbers: 1 1 2 3 5 8 13 21 34 55.
; r1 and r2 are the sliding pair (a, b), r3 counts down from ten,
; r6 is scratch for the next term, r5 holds the loop address.

  set r0, 0
  set r1, 0        ; a
  set r2, 1        ; b
  set r3, 10       ; how many to print
  set r4, 1
  set r5, loop
loop:
  out r2           ; print b
  add r6, r1, r2   ; next = a + b
  mov r1, r2       ; a = b
  mov r2, r6       ; b = next
  sub r3, r3, r4   ; count -= 1
  beq r3, r0, done ; stop once we have printed ten
  jmp r5
done:
  hlt
