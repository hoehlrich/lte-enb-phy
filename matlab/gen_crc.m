% Golden vectors for the CRC parity functions (36.212 §5.1.1).
% For each polynomial and input length, writes the input bits and the L
% parity bits lteCRCEncode appends. Lengths are whole bytes, matching the
% C++ contract (bits.size() % 8 == 0).
%   8    one byte: a single table step
%   40   a few steps, short enough to trace by hand
%   2048 long random input
%   6120 largest CRC-24B input (K = 6144 minus 24 parity bits)

rng(1);

polys = {'24A', '24B', '16', '8'};
lengths = [8 40 2048 6120];

for p = 1:numel(polys)
    poly = polys{p};
    for n = lengths
        bits = randi([0 1], n, 1);
        coded = lteCRCEncode(bits, poly);

        % lteCRCEncode is systematic: input first, then L parity bits.
        assert(isequal(double(coded(1:n)), bits), 'CRC output is not systematic');
        L = numel(coded) - n;
        parity = coded(n+1:end);

        name = sprintf('coding/crc%s_n%d', lower(poly), n);        cfg = struct('poly', poly, 'length', n, 'parity_bits', L);
        vec.write([name '_bits'], bits, 'u8', cfg);
        vec.write([name '_parity'], parity, 'u8', cfg);
    end
end