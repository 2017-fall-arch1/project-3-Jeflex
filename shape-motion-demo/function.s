  .file "function.s"
.text
  .arch msp430g2553
  .extern makeSong

makeSong:
  mov #1500, R12
  CALL #buzzer_set_period
