function DwPhyLab_RecordParameters
%Record variables from the caller function's workspace to its data structure

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.


ExcludedLabels = {'data','varargin','vargout','ans','Verbose','Concise', ...
    'hBar','hBarClose','hBarOfs','hBarStr','hBarStrIndex'};
S = evalin('caller','who');

for i=1:length(S),
   
   SkipVariable = 0;
   for Label = ExcludedLabels ,
       if strcmp(S{i}, Label),
           SkipVariable = 1;
       end
   end
   
   if ~SkipVariable,
       evalin('caller', sprintf('data.%s = %s;',S{i},S{i}) );
   end

end

