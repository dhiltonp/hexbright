
unsigned int read_adc(unsigned char pin) {
  // a useful reference: http://www.protostack.com/blog/2011/02/analogue-to-digital-conversion-on-an-atmega168/

  // configure adc: use refs0, pin = some combination of MUX(0-3).
  //  Setting _BV(ADLAR) could be useful, but it just saves 16 bytes and reduces resolution by four.
  //  If you set _BV(ADLAR) here, return ADCH as an unsigned char.
  ADMUX = _BV(REFS0) | pin;

  // Wait for Vref to settle - this costs 2 bytes, and is crucial for APIN_BAND_GAP
  // 150 is usually enough for the band gap to stabilize, give us some room for error.
  delayMicroseconds(250);

  // Start analog to digital conversion (used to be the sbi macro)
  ADCSRA |= _BV(ADSC);

  // wait for the conversion to complete (ADSC bit of ADCSRA is cleared, aka ADSCRA & ADSC)
  while (bit_is_set(ADCSRA, ADSC));

  return ADC;
}
