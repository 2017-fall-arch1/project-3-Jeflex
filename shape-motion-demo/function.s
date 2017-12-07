  .file "_function.c"
  .arch msp430g2553

  .global onDone

  .extern stateForMusic

  onDone:
    mov #1, &stateForMusic
    ret
