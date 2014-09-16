    %plot_dwpm

    figure;
    pmax = DwPhyLab_ReadRegister(32768 - 128 + 219);
    %plot((pmax - (63-data_pout.TXPWRLVL))/2, data_pout.Pout_dBm, '.-');
    SUBPLOT(3,1,1), 
    plot((pmax - (63-data_pout.TXPWRLVL))/2, data_pout.Pout_dBm, '.-',(pmax - (63-data_pout.TXPWRLVL))/2,(pmax - (63-data_pout.TXPWRLVL))/2, '--g',(pmax - (63-data_pout.TXPWRLVL))/2,data_pout.ReadPmeas/2,'-ko'); 
    grid on;
    legend('Pout','Target','Pmeas','Location','NorthEastOutside')
    XLABEL('dbm')
    YLABEL('dbm')
    %SUBPLOT(2,1,2), plot((pmax - (63-data_pout.TXPWRLVL))/2,abs((pmax - (63-data_pout.TXPWRLVL))/2- data_pout.Pout_dBm),  '-r',(pmax - (63-data_pout.TXPWRLVL))/2,abs((pmax - (63-data_pout.TXPWRLVL))/2- data_pout.ReadPmeas/2),  '-m',data_pout.ReadCurve); 
    SUBPLOT(3,1,2), plot((pmax - (63-data_pout.TXPWRLVL))/2,abs((pmax - (63-data_pout.TXPWRLVL))/2- data_pout.Pout_dBm),  '-r',(pmax - (63-data_pout.TXPWRLVL))/2,abs((pmax - (63-data_pout.TXPWRLVL))/2- data_pout.ReadPmeas/2),  '-m',(pmax - (63-data_pout.TXPWRLVL))/2,data_pout.ReadCurve,'-c');
    hold on; grid on;
    legend('Pout','Pmeas','Curve','Location','NorthEastOutside')
    XLABEL('dbm')
    YLABEL('dbm')
    text(30,0,'Error')
    SUBPLOT(3,1,3), plot((pmax - (63-data_pout.TXPWRLVL))/2,data_pout.ReadDPL,'-ko');
    hold on; grid on;
    legend('    DPL  ','Location','NorthEastOutside')
    XLABEL('dbm')
