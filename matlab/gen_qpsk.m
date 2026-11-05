% Known vectors for the qpsk block

rng(1);
txBits = randi([0 1], 1024, 1);
txSymbols = lteSymbolModulate(txBits, 'QPSK');

vec.write('mod/qpsk_bits', txBits, 'u8', struct('note', 'fixed pattern'));
vec.write('mod/qpsk_symbols', txSymbols, 'cf32');

snr_db = 3;
rxSymbols = awgn(txSymbols, snr_db, 'measured');
rxBits = lteSymbolDemodulate(rxSymbols, 'QPSK', 'Hard');

vec.write('mod/demap_symbols', rxSymbols, 'cf32');
vec.write('mod/demap_bits', rxBits, 'u8');