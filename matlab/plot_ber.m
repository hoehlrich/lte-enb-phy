function plot_ber(csvPath)
%PLOT_BER Plot a ber_sweep CSV against uncoded QPSK theory and check the match.
%   plot_ber('ber_thin_slice.csv')
%
%   Theory: BER = 0.5*erfc(sqrt(Eb/N0)). snr_db in the CSV is SNR per RE (Es/N0 after an
%   unscaled FFT) and ebn0_db = snr_db - 10*log10(2).
%   Pass rule: every uncapped point within 3 sigma of theory, where sigma = 1/sqrt(errors)
%   relative (about +/-30% at 100 errors). Also look for a consistent offset in one direction:
%   ~2.5 dB = per-sample vs per-RE SNR mix-up, 3 dB = Es vs Eb mix-up, a floor = a
%   deterministic bug.

if nargin < 1
    csvPath = 'ber_thin_slice.csv';
end

T = readtable(csvPath, 'CommentStyle', '#');
theory = @(ebn0Db) 0.5 * erfc(sqrt(10.^(ebn0Db / 10)));

capped = T.capped == 1;
measured = ~capped & T.errors > 0;
bound = capped;  % no reliable estimate: plot an upper bound instead
boundBer = max(T.ber, 1 ./ T.bits);  % 0 errors -> 1/bits

ebn0Fine = linspace(min(T.ebn0_db) - 0.5, max(T.ebn0_db) + 0.5, 400);

figure;
semilogy(ebn0Fine, theory(ebn0Fine), 'k-', 'LineWidth', 1.25, ...
    'DisplayName', 'Theory, uncoded QPSK');
hold on;
semilogy(T.ebn0_db(measured), T.ber(measured), 'o', 'MarkerFaceColor', [0 0.45 0.74], ...
    'MarkerEdgeColor', [0 0.45 0.74], 'DisplayName', 'C++ thin slice');
if any(bound)
    semilogy(T.ebn0_db(bound), boundBer(bound), 'v', 'MarkerEdgeColor', [0 0.45 0.74], ...
        'DisplayName', 'Capped (upper bound)');
end
hold off;
grid on;
xlabel('E_b/N_0 (dB)   [= SNR per RE - 3.01 dB]');
ylabel('Bit error rate');
title('Thin slice: QPSK over OFDM in AWGN');
legend('Location', 'southwest');
ylim([max(min([T.ber(measured); boundBer(bound)]) / 10, 1e-9), 1]);

% The check, per point
ratio = T.ber ./ theory(T.ebn0_db);
tol = 3 ./ sqrt(max(T.errors, 1));
ok = abs(ratio - 1) <= tol;
fprintf('%8s %8s %10s %10s %8s %7s  %s\n', 'SNR dB', 'Eb/N0', 'errors', 'BER', 'theory', ...
    'ratio', 'result');
for k = 1:height(T)
    if capped(k)
        result = 'capped, not judged';
    elseif ok(k)
        result = 'ok';
    else
        result = 'OUTSIDE 3 sigma';
    end
    fprintf('%8.2f %8.2f %10d %10.3e %8.2e %7.3f  %s\n', T.snr_db(k), T.ebn0_db(k), ...
        T.errors(k), T.ber(k), theory(T.ebn0_db(k)), ratio(k), result);
end
judged = ~capped;
fprintf('\n%d of %d uncapped points within 3 sigma; mean ratio %.3f\n', ...
    nnz(ok & judged), nnz(judged), mean(ratio(judged)));
end
