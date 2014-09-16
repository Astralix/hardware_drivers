
% raw measurement
figure;
hn(1) = plot(data_cal.Pmeas0, data_cal.dpl0, '.-');
hold on; grid on;
plot(data_cal.Pmeas1, data_cal.dpl1, '.-');

% post processing
hn(2) = plot(data_cal.rgf_curve_values, data_cal.dpl0_pp, 'm.-');
plot(data_cal.rgf_curve_values, data_cal.dpl1_pp, 'm.-');


legend(hn, 'raw measurement', 'post processing');