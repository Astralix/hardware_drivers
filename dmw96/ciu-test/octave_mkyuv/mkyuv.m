function mkyuv(yuvfile, yfile, cfile)
% function mkyuv(yuvfile, yfile, cfile)
%	yuvfile= output filename
%	yfile= input luma filename
%	cfile= input chroma filename
% this function converts yuv from separate luma and chroma input files
% into a unified yuv output file.
% input files are assumed to contain 32b hex values (text).
% output is a binary file suitable for display/conversion using graphics magick.
% byte order is according to ccir601: yvyu (u=LSB).
% example:
%	octave --eval "mkyuv('b.yuv', 'a_y.txt', 'a_c.txt')"
%	gm display -size 2048x1536 -sampling-factor 4:2:2 b.yuv
%	gm convert -size 2048x1536 -sampling-factor 4:2:2 b.yuv -resize 800x600 b.jpg
%	gm display b.jpg

if nargin ~= 3; error('3 arguments expected'); end
if ~ischar(yfile) || ~ischar(yfile) || ~ischar(cfile);
    error('all arguments must be char strings');
end
%
f1=fopen(yfile);
if f1 < 0; error(sprintf('cannot open "%s" for reading',yfile)); end
fprintf('reading "%s" ...\n', yfile);
y1=fscanf(f1,'%x'); fclose(f1);
%
f1=fopen(cfile);
if f1 < 0; error(sprintf('cannot open "%s" for reading',cfile)); end
fprintf('reading "%s" ...\n', cfile);
c1 = fscanf(f1,'%x'); fclose(f1);
%
if numel(y1) < numel(c1)
    warning('data size mismatch: truncating uv data');
    c1 = c1(1:numel(y1));
elseif numel(y1) > numel(c1)
    warning('data size mismatch: truncating y data');
    y1 = y1(1:numel(c1));
end
%
h1 = sqrt(3*numel(y1));
if mod(h1,1)
    fprintf('%d pixels (cannot estimate dimensions)\n', 4*numel(y1));
else
    fprintf('%d pixels (%dx%d)\n', 4*numel(y1), 4*h1/3, h1);
end
%
y2=[]; c2=[];
for k=1:4
    y2(k,:) = bitand(bitshift(y1, -8*(k-1)), 0xff);
    c2(k,:) = bitand(bitshift(c1, -8*(k-1)), 0xff);
end
%
yuv=[];
yuv(1,:) = c2(1:2:end);
yuv(2,:) = y2(1:2:end);
yuv(3,:) = c2(2:2:end);
yuv(4,:) = y2(2:2:end);
%
f1=fopen(yuvfile,'wb');
if f1 < 0; error(sprintf('cannot open "%s" for writing',yuvfile)); end
fprintf('writing "%s" ...\n', yuvfile);
fwrite(f1, uint8(yuv), 'uint8'); fclose(f1);
fprintf('done\n');
