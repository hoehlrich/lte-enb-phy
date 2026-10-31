% Known vectors for the OFDM modulator
% Modulate deterministic noise using QPSK and OFDM
rng(1);

enb = struct('NDLRB', 6, 'CyclicPrefix', 'Normal', 'CellRefP', 1, 'Windowing', 0);
dims = lteDLResourceGridSize(enb);   % [72 14 1]: subcarriers, symbols, antenna ports

bits = randi([0 1], prod(dims) * 2, 1);
syms = lteSymbolModulate(bits, 'QPSK');
grid = reshape(syms, dims);

[samples, info] = lteOFDMModulate(enb, grid);
assert(info.Nfft == 128, 'Nfft is %d, expected 128', info.Nfft);
assert(isequal(info.CyclicPrefixLengths, repmat([10 9 9 9 9 9 9], 1, 2)), 'unexpected cyclic prefix lengths');

vec.write('ofdm/resource_grid', grid, 'cf32', enb);
vec.write('ofdm/samples', samples, 'cf32', enb);