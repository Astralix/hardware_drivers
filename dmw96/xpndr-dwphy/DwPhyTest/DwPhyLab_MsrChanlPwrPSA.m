function MsrPwr = DwPhyLab_MsrChanlPwrPSA
% MsrPwr = DwPhyLab_MsrChanlPwrPSA

fcMHz = DwPhyLab_Parameters('fcMHz');
if isempty(fcMHz),
    Ploss = 0.0;
else
    Ploss = DwPhyLab_TxCableLoss(fcMHz);
end

PSA = DwPhyLab_OpenPSA;
DwPhyLab_SendCommand(PSA,':SENSE:CHPOWER:AVERAGE:COUNT 10');
DwPhyLab_SendCommand(PSA,':SENSE:CHPOWER:BANDWIDTH:INTEGRATION 20.0 MHz');
DwPhyLab_SendCommand(PSA,':SENSE:CHPOWER:FREQ:SPAN 20 MHz');
DwPhyLab_SendCommand(PSA,':INIT:CHPOWER');

result = '';
while ~strcmp(result, '+1'), % wait for the measurement to complete
    pause(0.5);
    result = DwPhyLab_SendQuery(PSA,'*OPC?');
end

result = DwPhyLab_SendQuery(PSA,':FETCH:CHPOWER:CHPOWER?');
DwPhyLab_ClosePSA(PSA);

MsrPwr = str2double(result) + Ploss;
