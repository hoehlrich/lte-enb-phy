% Known vectors for the qpsk block

rng(1);
bits = randi([0 1], 1024, 1);
symbols = lteSymbolModulate(bits, 'QPSK');

vec.write('mod/qpsk_bits', bits, 'u8', struct('note', 'fixed pattern'));
vec.write('mod/qpsk_symbols', symbols, 'cf32');