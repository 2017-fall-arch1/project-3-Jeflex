.file "function.s"
.text
  .arch msp430g2553
  .extern makeSong
  .global makeSong

makeSong:
  mov #150, R12
  CALL &buzzer_set_period
  mov #100, R12
  CALL &buzzer_set_period
  mov #50, R12
  CALL &buzzer_set_period
  mov #20, R12
  CALL &buzzer_set_period
  mov #35, R12
  CALL &buzzer_set_period
