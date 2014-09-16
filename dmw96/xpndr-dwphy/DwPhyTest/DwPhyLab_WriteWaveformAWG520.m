function y = DwPhyLab_WriteWaveformAWG520(filename,x,freq,Marker1,Marker2)
%DwPhyLab_WriteWaveformAWG520 - Create a pattern file for the Tektronix AWG520
%
%       DwPhyLab_WriteWaveformAWG520(FILENAME,X,FREQ) create a file
%       FILENAME with the data in X for a sample frequency of FREQ (MHz). 
%       The data are quantized to 10-bit samples and the length is 
%       zero-padded to be a multiple of 8.
%
%       DwPhyLab_WriteWaveformAWG520(FILENAME,X,FREQ,MARKER1) works the 
%       same as above, but sets Marker1 to be on when MARKER1 is not 0.
%
%       DwPhyLab_WriteWaveformAWG520(FILENAME,X,FREQ,MARKER1,MARKER2) works
%       the same as above, but sets Marker2 to be on when MARKER2 is not 0.

%%% =============================
%%% CHECK THE NUMBER OF ARGUMENTS
%%% =============================
if((nargin<3)||(nargin>5)), error('Wrong number of arguments'); end

%%% ================================
%%% COVERT AND CHECK THE SAMPLE RATE
%%% ================================
if((freq<1)||(freq>100)), error('Frequency value out of range...must be 100 to 1000'); end
freq = 1.0E+6 * freq;

%%% ====================================================
%%% QUANTIZE x AND MAKE THE ARRAY LENGTH A MULTIPLE OF 8
%%% ====================================================
n = min(size(x));
x = reshape(x,n,length(x));
if max(abs(x-round(x))) > 1e-12,
    x = [x 0 0 0 0 0 0 0];
    L = 8*floor(length(x)/8);
    x = x / max(max(abs(x)));
    y = round((511*x(1:L))+512);
else
    x = [x 512*ones(1,7)];
    L = 8*floor(length(x)/8);
    y = x(1:L); % already quantized
    
    y = max(y, 0);
    y = min(y, 1023);
end

%%% =============
%%% ADD MARKER #1
%%% =============
if(nargin>3)
   Marker1 = reshape(Marker1,1,length(Marker1));
   Marker1 = [Marker1 zeros(1,L-length(Marker1))];
   Marker1 = (2^13) * (Marker1 ~= 0);
   y = y + Marker1;
end

%%% =============
%%% ADD MARKER #2
%%% =============
if(nargin>4)
   Marker2 = reshape(Marker2,1,length(Marker2));
   Marker2 = [Marker2 zeros(1,L-length(Marker2))];
   Marker2 = (2^14) * (Marker2 ~= 0);
   y = y + Marker2;
end

%%% ====================================================
%%% CREATE THE OUTPUT FILE
%%% ====================================================
F = fopen(filename,'w');
fprintf(F,'MAGIC 2000\n');
n = 2*length(y);
m = ceil(log10(n+0.001)); % +0.001 to prevent errors
fprintf(F,'#%d%d',m,n);
fwrite(F,y,'integer*2');
fprintf(F,'CLOCK %11.6e\n',freq);
fclose(F);

