  .file "function.s"
.text
  .arch msp430g2553

  .extern makeSong

  makeSong:
    mov #1500, R12
    #call buzzer_set_period
