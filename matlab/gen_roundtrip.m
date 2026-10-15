% Known vectors for the loader's round-trip test.
% Values are exact in float32, and every element differs, so a dropped
% imaginary part, a swapped re/im, or a transpose each shows up in the bytes.

vec.write('roundtrip/bits', [1 0 1 1 0 0 1 0].', 'u8', ...
          struct('note', 'fixed pattern'));

vec.write('roundtrip/samples', [1+2i, -3+4i; 0.5-0.25i, 8-1i], 'cf32', ...
          struct('note', '2x2, column-major'));
