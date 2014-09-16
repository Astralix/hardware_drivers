function [B, A] = wiFilter_Butterworth(N, Fc, Method)
%wiFilter_Butterworth -- Digital Butterworth LPF design
%
%   [B,A] = wiFilter_Butterworth(N, Fc, Method) returns a digital Butterworth
%   filter with transfer function H(z) = B(z)/A(z). The filter order is N with
%   cutoff Fc times the sample rate, e.g., Fc/T. Method is 1 for impulse 
%   invariance and 2 for bilinear transform.

%   Developed by Barrett Brickner
%   Copyright 2001 Bergana Communications, Inc. All rights reserved.

if (nargin ~= 3), error('Incorrect number of arguments.'); end
[B,A] = wiseMex('wiFilter_Butterworth()',N,Fc,Method);
