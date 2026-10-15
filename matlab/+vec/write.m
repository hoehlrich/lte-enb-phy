function write(name, x, dtype, cfg)
    %VEC.WRITE Write a golden vector as vectors/<name>.bin + vectors/<name>.json.
    %   vec.write('ofdm_mod/cell0_grid', grid, 'cf32', enb)
    %
    %   dtype: 'u8' (bits, one per byte), 'i32', 'f32', or 'cf32' (complex).
    %   cfg:   the parameters the vector was generated with; tests read it.
    %   Format: little-endian, column-major, complex interleaved re/im.

    if nargin < 4
        cfg = struct();
    end

    if ~strcmp(dtype, 'cf32') && ~isreal(x)
        error('vec:complex', 'Complex data needs dtype ''cf32''.');
    end

    % Convert to the stored type, refusing anything the conversion would change.
    switch dtype
        case 'u8'
            data = uint8(x);
        case 'i32'
            data = int32(x);
        case 'f32'
            data = single(x);
        case 'cf32'
            % fwrite silently drops the imaginary part of complex data, so
            % interleave by hand: a 2-by-N matrix is stored re0 im0 re1 im1 ...
            data = single([real(x(:)).'; imag(x(:)).']);
        otherwise
            error('vec:dtype', 'Unknown dtype ''%s''.', dtype);
    end
    if ~strcmp(dtype, 'cf32') && ~isequal(double(data), double(x))
        error('vec:lossy', 'Values do not fit dtype ''%s'' exactly.', dtype);
    end

    % Paths are relative to the repo, wherever MATLAB's current folder is.
    root = fileparts(fileparts(fileparts(mfilename('fullpath'))));
    base = fullfile(root, 'vectors', name);
    if ~exist(fileparts(base), 'dir')
        mkdir(fileparts(base));
    end

    % Data: raw little-endian, column-major (fwrite's native order).
    fid = fopen([base '.bin'], 'w', 'ieee-le');
    if fid < 0
        error('vec:open', 'Cannot open %s.bin for writing.', base);
    end
    fwrite(fid, data, class(data));
    fclose(fid);

    % Sidecar: everything a reader needs, plus where the data came from.
    meta.format = 1;
    meta.dtype = dtype;
    meta.shape = size(x);
    meta.generator = caller(root);
    meta.matlab = version;
    tb = ver('lte');
    if isempty(tb)
        meta.lte_toolbox = '';
    else
        meta.lte_toolbox = tb.Version;
    end
    meta.config = cfg;

    fid = fopen([base '.json'], 'w');
    fprintf(fid, '%s\n', jsonencode(meta, 'PrettyPrint', true));
    fclose(fid);
end

function name = caller(root)
    % The script or function that called vec.write, relative to the repo.
    st = dbstack('-completenames');
    if numel(st) < 3
        name = 'command line';
    else
        name = strrep(st(3).file, [root filesep], '');
    end
end
