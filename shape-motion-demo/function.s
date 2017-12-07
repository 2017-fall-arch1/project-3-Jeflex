  .file "function.s"
  .arch msp430g2553

  .global onDone

  .extern makeSong

  makeSong:
  mov #1500, R12
    CALL# buzzer_set_period
