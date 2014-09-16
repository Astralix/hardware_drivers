% DwPhyLab ___________________________________________________________________________
%
% DwPhy PHY Interface
%   DwPhyLab_AddressFilter     - Enable/disable the nuisance packet address filter
%   DwPhyLab_ErrorDescription  - Returns a description for a DwPhy error code
%   DwPhyLab_Initialize        - Initialize the PHY
%   DwPhyLab_ReadRegister      - Read a register or register field
%   DwPhyLab_RegList           - Returns a list of PHY registers
%   DwPhyLab_SetChannelFreq    - Set channel frequency
%   DwPhyLab_SetDiversityMode  - Select diversity mode (single antenna, MRC,...)
%   DwPhyLab_SetParameterData  - Load programmable parameter data
%   DwPhyLab_SetRxMode         - Select receiver modulation compatibility
%   DwPhyLab_SetRxSensitivity  - Set the minimum sensitivity level
%   DwPhyLab_SetStationAddress - Set address used by baseband address filter
%   DwPhyLab_Shutdown          - Shutdown the DwPhy module
%   DwPhyLab_Sleep             - Put the PHY into sleep mode
%   DwPhyLab_Wake              - Wake the PHY
%   DwPhyLab_WriteRegister     - Write a register or register field
%   DwPhyLab_WriteUserReg      - Writes a list of registers from a cell array
%
% Instrument Control
%   DwPhyLab_OpenAWG520       - AWG520 Get instrument handle
%   DwPhyLab_ReadAWG520       - AWG520 Read from the instrument
%   DwPhyLab_WriteAWG520      - AWG520 Write to the instrument
%   DwPhyLab_CloseAWG520      - AWG520 Close the instrument session
%   DwPhyLab_OpenESG          - ESG/PSA Get instrument handle
%   DwPhyLab_CloseESG         - ESG/PSA Close the instrument session
%   DwPhyLab_EnableESG        - ESG/PSA Enable RF output (ESG only)
%   DwPhyLab_DisableESG       - ESG/PSA Enable RF output (ESG only)
%   DwPhyLab_TriggerESG       - ESG/PSA Trigger the instrument
%   DwPhyLab_OpenPSA          - PSA Get instrument handle
%   DwPhyLab_CloseESG         - PSA Close the instrument session
%   DwPhyLab_OpenScope        - LeCroy Scope Get instrument handle
%   DwPhyLab_CloseScope       - LeCroy Scope Close the instrument session
%   DwPhyLab_SendCommand      - Send a single command to an instrument
%   DwPhyLab_SendQuery        - Send a single query and retrieve the response
%   DwPhyLab_SetTxFreqPSA     - Set the spectrum analyzer center frequency
%   DwPhyLab_SetRxFreqESG     - Set the signal generator center frequency
%   DwPhyLab_SetRxPower       - Set the signal generator output power
%
% Waveform Generation
%   DwPhyLab_LoadWaveform        - Load an arbitrary waveform to the ESG
%   DwPhyLab_LoadWaveform_LTF    - Load a long training field waveform to the ESG
%   DwPhyLab_PacketWaveform      - Generate a waveform with 802.11 packet(s)
%   DwPhyLab_WriteWaveformAWG520 - Create AWG520 format waveform file
%
% Packet Transmission
%   DwPhyLab_TxBurst             - Transmit a burst of packets with minimum IFS
%   DwPhyLab_TxPacket            - Transmit a single packet
%
% Calibration Data
%   DwPhyLab_TxCableLoss         - Return station cable loss for TX
%   DwPhyLab_RxCableLoss         - Return station cable loss for RX
%
% Base Test Functions
%   DwPhyLab_AverageRSSI         - Return RSSI (not synchronized with packets)
%   DwPhyLab_CalcEVM_11b         - Consetllation error calculation for 802.11b (DQPSK)
%   DwPhyLab_GetEVM              - Retrieve a waveform from the scope and measure EVM
%   DwPhyLab_GetEVMb             - Same as DwPhyLab_GetEVM but for DSSS/CCK
%   DwPhyLab_GetPER              - Trigger waveform output and measure data for PER
%   DwPhyLab_LoadPER             - Load a waveform/sequence for PER testing
%
% Utilities
%   DwPhyLab_Setup               - Setup communication with the DUT
%   DwPhyLab_Parameters          - Interface to the private data structure
%   DwPhyLab_SetMacAddress       - Set the MAC STAtion address for the DUT
%   DwPhyCheck                   - Error checking wrapper for calls to DwPhyMex
%
% TEST FUNCTIONS _____________________________________________________________________
%
% Evaulation and Regression Tests
%   DwPhyTest_RunSuite             - Run a defined suite of tests
%   DwPhyTest_RunTest              - Run a single defined regression/evalulation test
%
% Register and Interface Tests
%   DwPhyTest_BasicIO              - Test MC, LNAGAIN, and AGAIN connectivity
%   DwPhyTest_BasebandRegisters    - Test baseband register read/write
%   DwPhyTest_RadioSerialRegisters - Test radio register read/write at all speeds
%   DwPhyTest_BasebandReset        - Test baseband register values after reset
%   DwPhyTest_PllLock              - Check PLL lock on all channels
%
% Receiver PER/Sensitivity Tests and Optimization
%   DwPhyTest_Sensitivity           - Basic sensitivity measurement
%   DwPhyTest_SensitivityVsChanl    - Sensitivity across a list of channels
%   DwPhyTest_PERvPrdBm             - Measure PER across a range of input powers
%   DwPhyTest_PERvChanl             - Measure PER across a range of channels
%   DwPhyTest_PERFloor              - Continuous PER measurement under fixed conditions
%   DwPhyTest_Blocking              - Receiver blocking with a single tone interferer
%   DwPhyTest_InterferenceRejection - Rejection of interference signal (requeires AWG520)
%
% Optimization Functions
%   DwPhyTest_InitAGain             - Determine band-specific InitAGain settings
%   DwPhyTest_ThSwitchLNA           - Optimize ThSwitchLNA setting
%   DwPhyTest_TuneDPLL_PER          - Tune DPLL using PER at a fixed receive power
%   DwPhyTest_SensitivityVsRegister - Measure sensitivity for a range of register values
%   DwPhyTest_PERvRegister          - Measure PER for a range of register values
%   DwPhyTest_NuisancePERvRegister  - Measure PER using alternating nuisance packets
%
% Receiver Gain, Offset, CFO Tests
%   DwPhyTest_GainVsChanl          - Measure gain and StepLNA across channels
%   DwPhyTest_InitAGain            - Measure gain and determine InitAGain setting
%   DwPhyTest_MeasureRxCFO         - CFO measured using baseband receiver
%   DwPhyTest_MeasureRxOffset      - Measure offset at ADC output
%   DwPhyTest_RSSIvPin             - Measure RSSI vs Pr(dBm) with fixed radio gain
%   DwPhyTest_RxGainProfile        - Gain for all AGAIN, LNAGAIN settings
%   DwPhyTest_RxIMR                - Receiver image rejection
%
% Transmitter and LO Tests
%   DwPhyTest_TxEVM                - Basic EVM measurement
%   DwPhyTest_SingleTxEVM          - Basic EVM measurement using single TX (not burst)
%   DwPhyTest_TxEVMvPout           - EVM vs TXPWRLVL (Pout)
%   DwPhyTest_TxPhaseNoise         - Measure phase noise using baseband +10 MHz tone
%   DwPhyTest_TxPower              - Transmit power vs fcMHz and TXPWRLVL
%   DwPhyTest_TxPowerRamp          - Pre-packet spurious transmission measurement
%   DwPhyTest_TxPowerVsReg         - Transmit power vs an arbitrary register field
%   DwPhyTest_TxPwrVsDACOFFSET     - Transmit power as a function of the set point (DACOFFSET) 
%   DwPhyTest_TxPwrVsPADGAIN       - Transmit power as a function of PA Driver Gain
%   DwPhyTest_TxSpectrumMask       - Transmit spectrum for 802.11a/b/g
%   DwPhyTest_TxIMR                - Transmit image rejection
%   DwPhyTest_Kvco                 - Measure carrier vs CAPSEL and VTune
%   DwPhyTest_SleepWakeALC         - Measure ALC operation with power save
%
% Test Utilities and Core Test Routines
%   DwPhyTest_AdjustInputPower     - Adjust RX input power to give a specified output
%   DwPhyTest_AdjustScopeAmplitude - Adjust signal levels on the oscilloscope (TX)
%   DwPhyTest_CalcSensitivity      - Compute sensitivity from PrdBm and PER data
%   DwPhyTest_ChannelList          - Return a list of supported channels
%   DwPhyTest_LoadAWG              - Load a waveform from the AWG520 disk to memory
%   DwPhyTest_RunTestCommon        - PHY initialization and configuration
%   DwPhyTest_SensitivityCore      - Core sensitivity calculator
%   DwPhyTest_SetBand              - Select the operating band (2.4 or 2.4+5 GHz)
%   DwPhyTest_Setup                - Basic station setup, including instruments
%
% PLOTTING FUNCTIONS _________________________________________________________________
%
% Test Utilities and Core Test Routines
%   DwPhyPlot_Blocking              - Plot result from DwPhyTest_Blocking
%   DwPhyPlot_InterferenceRejection - Plot result from DwPhyTest_InterferenceRejection
%   DwPhyPlot_Kvco                  - Plot results from DwPhyTest_Kvco
%   DwPhyPlot_PERvPrdBm             - Plot result from DwPhyTest_PERvPrdBm
%   DwPhyPlot_RxEval                - Plot results from DwPhyTest_RunSuite('RxEval')
%   DwPhyPlot_RxGain                - Plot results from DwPhyTest_RunTest(300.2)
%   DwPhyPlot_TxEval                - Plot results from DwPhyTest_RunSuite('TxEval')
%   DwPhyPlot_TxMask                - Plot results from DwPhyTxt_TxSpectrumMask
%   DwPhyPlot_TxPhaseNoise          - Plot results from DwPhyTest_TxPhaseNoise
%   DwPhyPlot_TxPower               - Plot results from DwPhyTest_TxPower
%
% Plotting Utilties and Core Plotting Routines
%   DwPhyPlot_Blocking
%   DwPhyPlot_Command               - Generate plotting expression from cell arrays
%   DwPhyPlot_LoadData              - Load data structure from file list
%   DwPhyPlot_PER                   - Generic PER plot
%   DwPhyPlot_SplitBand             - Plot data with 2.4/5 GHz as subplots

% Written by Barrett Brickner
% Copyright 2008-2010 DSP Group, Inc., All Rights Reserved.
