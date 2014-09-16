FrameN = 100;
tic
for i=1:FrameN,
    DwPhyLab_TxBurst( 1, 6, 500, 33, 0);        
    pause(0.1);
end
toc