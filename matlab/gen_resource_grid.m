% Known vectors for the resource grid block

cfg = struct('NDLRB',6,'CellRefP',1);
grid = lteDLResourceGrid(cfg);

% Set each resource element to a predictable value
[N, M] = size(grid);
grid(:, :) = (1:N)' + 1i*(1:M);

vec.write('common/resource_grid', grid, 'cf32', struct('note', '72 cubcarriers by 14 symbols (column major)'));