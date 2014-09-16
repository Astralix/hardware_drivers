//--< Nevada_DFS.c >-------------------------------------------------------------------------------
//=================================================================================================
//
//  Nevada - DFS Radar Detection
//  Copyright 2006-2009 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wiType.h"
#include "wiUtil.h"
#include "Nevada.h"

#define ClockRegister(arg) {(arg).Q=(arg).D;};

// ================================================================================================
// FUNCTION  : Nevada_CheckFCS
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute and check the 802.11 FCS (algorithm from FrameGen2 program)
// Parameters:
// ================================================================================================
void Nevada_CheckFCS(wiUInt nBytes, wiUWORD X[], wiUInt *FCSPass)
{
    unsigned int i, j;
    unsigned int p = 0xEDB88320UL;
    unsigned int c = 0xFFFFFFFFUL;
    unsigned int a, b;
    wiUWORD d = {0};

    for (i=0; i<nBytes; i++)
    {
        a = (c & 0xFF) ^ X[i].w8;
        for (j=0; j<8; j++)
            a = p*(a&1) ^ (a>>1);
        b = c >> 8;
        c = a ^ b;
    }

    for (i=0; i<32; i++) 
    {
       d.word = d.word << 1;
       d.b0 = (c&1);
       c = c>>1;
    }
    *FCSPass = (d.word == 0xC704DD7BUL) ? 1 : 0;
}
// end of Nevada_CheckFCS()

// ================================================================================================
// FUNCTION  : Nevada_DFS
// ------------------------------------------------------------------------------------------------
// Purpose   : Detect the presence of non-802.11 signals (specifically radar pulses)
// Timing    : Operates on 20 MHz clock
// Parameters: DFSEnB    -- enable for DFS module (TBD...probably follow DFEEnB)
//             SigIn     -- input signal array
//             SigOut    -- output sinal array
//             pRX, pReg -- pointers to WiSE RX state and configuration register array
// ================================================================================================
void Nevada_DFS( wiBoolean DFSEnB, Nevada_aSigState_t SigIn, Nevada_aSigState_t *SigOut, 
                 Nevada_RxState_t *pRX, NevadaRegisters_t *pReg )
{
    if (DFSEnB) return;
    else 
    {
        static wiUReg state;        // state machine; 3 bits
        static wiUReg timer;        // master timer; 19 bits
        static wiUReg wCount;       // pulse width counter; 12 bits
        static wiUReg gCount;       // pulse gap width counter; 12 bits
        static wiUReg pCount;       // in packet detector counter; 8 bits
        static wiUReg inPacket;     // during packet receiving
        static wiUReg restart;      // delayed restart of the FSM when inPacket
        static wiUReg FCSPass;      // FCS (CRC) correct
        static wiUReg radarDet;     // radar detected flag

        wiUWORD PulseStart;         // PulseDet rising edge/RSSI increases
        wiUWORD PulseEnd;           // PulseDet falling edge/RSSI decreases
        wiUWORD dInterval;          // interval error; 18 bits
        wiWORD  dInterval2;         // interval error; 18 bits
        wiUWORD dInterval05;        // interval error; 18 bits
        wiBoolean match;            // shift register pattern matched
        wiBoolean checkFCS=0;       // check (latch) FCS [BB: added initialization]

        int pass = 0;               // pulse qualifies for favored candidate
        int i, j, bits, zeros[2];
        unsigned int mask;

        static struct {
            wiUReg interval;        // recorded pulse interval; 18 bits
            wiUReg iCount;          // pulse interval counter; 18 bits
            wiUReg tInterval;       // temporary interval memory; 18 bits
            wiUReg pulseSR;         // shift register of pulse candidates, 17 bits
            wiUReg tPulses;         // temporary pulse counter; 3 bits
        } pulse[2]; // pulse candidates

        // ----------------
        // Basic Indicators
        // ----------------
        if (state.Q.word > 0) 
        {
            if (SigIn.HeaderValid) pReg->detHeader.b0   = 1; // indicate valid PLCP header
            if (SigIn.SyncFound)   pReg->detPreamble.b0 = 1; // indicate valid OFDM preamble
            if (SigIn.StepUp)      pReg->detStepUp.b0   = 1; // indicate RSSI step-up event
            if (SigIn.StepDown)    pReg->detStepDown.b0 = 1; // indicate RSSI step-down event
        }

        // clock registers
        ClockRegister(state);
        ClockRegister(timer);
        ClockRegister(wCount);
        ClockRegister(gCount);
        ClockRegister(pCount);
        ClockRegister(inPacket);
        ClockRegister(restart);
        ClockRegister(FCSPass);
        ClockRegister(radarDet);
        for (i=0; i<2; i++) 
        {
            ClockRegister(pulse[i].interval);
            ClockRegister(pulse[i].iCount);
            ClockRegister(pulse[i].tInterval);
            ClockRegister(pulse[i].pulseSR);
            ClockRegister(pulse[i].tPulses);
        }

        // assign default D input of registers
        state.D.word    = state.Q.word;
        timer.D.word    = timer.Q.word;
        wCount.D.word   = wCount.Q.word;
        gCount.D.word   = gCount.Q.word;
        pCount.D.word   = pCount.Q.word;
        inPacket.D.word = inPacket.Q.word;
        restart.D.word  = restart.Q.word;
        FCSPass.D.word  = FCSPass.Q.word;
        radarDet.D.word = radarDet.Q.word;
        for (i=0; i<2; i++) 
        {
            pulse[i].interval.D.word  = pulse[i].interval.Q.word;
            pulse[i].iCount.D.word    = pulse[i].iCount.Q.word;
            pulse[i].tInterval.D.word = pulse[i].tInterval.Q.word;
            pulse[i].pulseSR.D.word   = pulse[i].pulseSR.Q.word;
            pulse[i].tPulses.D.word   = pulse[i].tPulses.Q.word;
        }

        // detect pulse edges
        if ((!SigIn.PulseDet) && SigOut->PulseDet) PulseStart.word = 1;
        else PulseStart.word = 0;
        if (SigIn.PulseDet && (!SigOut->PulseDet)) PulseEnd.word = 1;
        else PulseEnd.word = 0;

        // pattern matching for the candidate shift register
        mask = (1<<pReg->DFSminPPB.w3)-1;
        bits = pReg->DFSminPPB.w3+(4-pReg->DFSPattern.w2);
        for (i=0; i<2; i++) {
            zeros[i] = 0;
            for (j=0; j<bits; j++)
             zeros[i] += !((pulse[i].pulseSR.Q.w17>>j)&1);
        }
        if ((pulse[0].pulseSR.Q.w10&mask) == mask ||
        (pulse[1].pulseSR.Q.w10&mask) == mask) {
            match = 1;
        } else if (pReg->DFSPattern.w2>0) {
            if (zeros[0]==1 || zeros[1]==1)
             match = 1;
            else
             match = 0;
        } else {
            match = 0;
        }

        // interval counters, timers
        if (pReg->DFSOn.b0) {
            timer.D.word = timer.Q.w19  + 1;
            for (i=0; i<2; i++) {
             pulse[i].iCount.D.word = (pulse[i].iCount.Q.word==(1<<18)-1)? (1<<18)-1 : pulse[i].iCount.Q.w18+1;
             if (pulse[i].pulseSR.Q.word > 1) {
                 dInterval2.word = (signed)pulse[i].iCount.Q.w18-(signed)pulse[i].interval.Q.w18;
                 if ((dInterval2.w18>>3) > (signed)(pReg->DFSdTPRF.w4)) {
                            //printf("Pulse erased for %d @ %d!\n", i, pRX->k20);
                            pulse[i].iCount.D.word = pulse[i].iCount.Q.w18-pulse[i].interval.Q.w18+1;
                            if (!(state.Q.word == 1 && match)) {
                         pulse[i].pulseSR.D.word = pulse[i].pulseSR.Q.w17<<1;
                         if (inPacket.Q.word) pulse[i].tPulses.D.word = (pulse[i].tPulses.Q.word==7)? 7 : pulse[i].tPulses.Q.w3+1;
                            }
                 }
             }
            }
        }

        // detect packet receiving
        if (pRX->DFEState[pRX->k20].word == 17) {
            inPacket.D.word = 1;
            checkFCS = 0;
            pCount.D.word = 0;
        } else if (pCount.Q.word == 200) {       // wait 10us after out of 17
            inPacket.D.word = 0;
            checkFCS = 1;
            pCount.D.word = 0;
        } else if (inPacket.Q.word) {
            inPacket.D.word = 1;
            checkFCS = 0;
            pCount.D.word = (pCount.Q.w8==255)? 255 : (pCount.Q.w8+1);
        }

        // ----------------
        // Radar Classifier
        // ----------------
        if (!pReg->DFSOn.b0 || timer.Q.word==((1<<19)-1)) {
            state.D.word = 0;
            timer.D.word = 0;
            pReg->detRadar.word    = 0;
        } 
        else 
        {
            switch (state.Q.word)
            {
                case 0: // idle state/clear indicators and initialize for DFS scan
              pReg->detHeader.word   = 0;
              pReg->detPreamble.word = 0;
              pReg->detStepUp.word   = 0;
              pReg->detStepDown.word = 0;

              timer.D.word   = 0;
              state.D.word   = 1;
              wCount.D.word  = 0;
              gCount.D.word  = 0;
              FCSPass.D.word = 0;
              for (i=0; i<2; i++) {
                        pulse[i].interval.D.word  = 0;
                        pulse[i].iCount.D.word    = 0;
                        pulse[i].pulseSR.D.word   = 0;
                        pulse[i].tInterval.D.word = 0;
                        pulse[i].tPulses.D.word   = 0;
              }
              break;

                case 1: // start scan
              wCount.D.word = 0;
              if (match) {
                        gCount.D.word = 0;
                        if (!inPacket.Q.word) state.D.word = 7;
              } else if (PulseStart.word) {
                        //printf("Pulse found @ %d.\n", pRX->k20);
                        pulse[0].iCount.D.word = 1;
                        pulse[1].iCount.D.word = 1;
                        pulse[0].tInterval.D.word = pulse[0].iCount.Q.w18;
                        pulse[1].tInterval.D.word = pulse[1].iCount.Q.w18;
                        state.D.word = 2;
              }
              break;

                case 2: // end scan
              gCount.D.word = 0;
              wCount.D.word = (wCount.Q.word == 4095)? 4095 : wCount.Q.w12+1;
              if (PulseEnd.word) {
                        if ((wCount.Q.word>>4) > pReg->DFSmaxWidth.w8) {
                      pulse[0].iCount.D.word = pulse[0].iCount.Q.w18+pulse[0].tInterval.Q.w18+1;
                      pulse[1].iCount.D.word = pulse[1].iCount.Q.w18+pulse[1].tInterval.Q.w18+1;
                      state.D.word = pReg->DFSSmooth.b0? 3 : 1;
                        } else {
                      state.D.word = pReg->DFSSmooth.b0? 3 : 4;
                        }
              }
              break;

                case 3: // merge immediately follwoing pulses
              if (!SigIn.PulseDet) gCount.D.word = (gCount.Q.word == 4095)? 4095 : gCount.Q.w12+1;
              else gCount.D.word = 0;
              if (PulseStart.word) {
                        wCount.D.word = (wCount.Q.word+gCount.Q.word >= 4095)? 4095 : wCount.Q.w12+gCount.Q.w12+1;
              } else if (SigIn.PulseDet) {
                        wCount.D.word = (wCount.Q.word == 4095)? 4095 : wCount.Q.w12+1;
              }

              if ((gCount.Q.word>>5) == pReg->DFSminGap.w7) {
                        if ((wCount.Q.word>>4) > pReg->DFSmaxWidth.w8) {
                      pulse[0].iCount.D.word = pulse[0].iCount.Q.w18+pulse[0].tInterval.Q.w18+1;
                      pulse[1].iCount.D.word = pulse[1].iCount.Q.w18+pulse[1].tInterval.Q.w18+1;
                      state.D.word = 1;
                        } else {
                      state.D.word = 4;
                        }
              }
              break;

                case 4: // qualify pulse
              wCount.D.word = 0;
              gCount.D.word = 0;
              state.D.word = pReg->DFS2Candidates.b0? 5:1;
              pass = 0;
              for (i=0; i<=0; i++) {
                        if (pulse[i].pulseSR.Q.word>1) {
                      dInterval.word = abs(pulse[i].tInterval.Q.w18-pulse[i].interval.Q.w18);
                      dInterval05.word = abs(pulse[i].tInterval.Q.w18-pulse[i].interval.Q.w18/2);
                        } else {
                      dInterval.word = 0;
                      dInterval05.word = 0;
                        }
                        //printf("pulse[%0d,%02hX]: ", i, pulse[i].pulseSR.Q.word);
                        //printf("wc;(p,pc,dp) = %d;(%d,%d,%d)\n", wCount.Q.w12,pulse[i].interval.Q.w18,pulse[i].tInterval.Q.w18,dInterval.w18);
                        //printf("dTPRF=(%d,%d),%d,%d,%d\n",dInterval.word,dInterval05.word,((pulse[i].tInterval.Q.word>>5) >= pReg->DFSminTPRF.w8),((pulse[i].tInterval.Q.word>>10) <= pReg->DFSmaxTPRF.w8),((dInterval.w18>>3) <= pReg->DFSdTPRF.word));
                        //printf("bits=%d,mask=%d,zeros=%d\n",bits,mask,zeros[0]);

                        if ((!pulse[i].pulseSR.Q.word ||
                      (((pulse[i].tInterval.Q.word>>5) >= pReg->DFSminTPRF.w8) &&
                       ((pulse[i].tInterval.Q.word>>10) <= pReg->DFSmaxTPRF.w8))) &&
                            ((dInterval.w18>>3) <= pReg->DFSdTPRF.word)) {
                      pass = 1;
                      pulse[i].pulseSR.D.word = (pulse[i].pulseSR.Q.w17<<1)+1;
                      if (inPacket.Q.word) pulse[i].tPulses.D.word = (pulse[i].tPulses.Q.word==7)? 7 : pulse[i].tPulses.Q.w3+1;
                      if (pulse[i].pulseSR.Q.word==1) {
                          pulse[i].interval.D.word = pulse[i].tInterval.Q.w18;
                      }
                        } else if ((pulse[i].pulseSR.Q.word > 1) &&
                            ((pulse[i].tInterval.Q.word>>5) >= pReg->DFSminTPRF.w8) &&
                            ((dInterval05.w18>>3) <= (pReg->DFSdTPRF.word))) {
                      //printf("Erased pulse!\n");
                      pass = 1;
                      pulse[i].pulseSR.D.word = (pulse[i].pulseSR.Q.b7<<15)+(pulse[i].pulseSR.Q.b6<<13)+(pulse[i].pulseSR.Q.b5<<11)+(pulse[i].pulseSR.Q.b4<<9)+(pulse[i].pulseSR.Q.b3<<7)+(pulse[i].pulseSR.Q.b2<<5)+(pulse[i].pulseSR.Q.b1<<3)+(pulse[i].pulseSR.Q.b0<<1)+1;
                      if (inPacket.Q.word) pulse[i].tPulses.D.word = (pulse[i].tPulses.Q.word==7)? 7 : pulse[i].tPulses.Q.w3+1;
                      pulse[i].interval.D.word = pulse[i].tInterval.Q.w18;
                        } else {
                      pulse[i].iCount.D.word = pulse[i].iCount.Q.w18+pulse[i].tInterval.Q.w18+1;
                        }
              }
              if (!pass) {
                        if (pReg->DFS2Candidates.b0) state.D.word = 5;
                        else if (!inPacket.Q.word) state.D.word = 0;
                        else restart.D.word = 1;
              } else if (!pulse[1].pulseSR.Q.word) {
                        state.D.word = 1;
              }
              break;

                case 5: // qualify pulse
              wCount.D.word = 0;
              gCount.D.word = 0;
              state.D.word = 1;
              pass = 0;
              for (i=1; i<=(int)pReg->DFS2Candidates.b0; i++) {
                        if (pulse[i].pulseSR.Q.word>1) {
                      dInterval.word = abs(pulse[i].tInterval.Q.w18-pulse[i].interval.Q.w18);
                      dInterval05.word = abs(pulse[i].tInterval.Q.w18-pulse[i].interval.Q.w18/2);
                        } else {
                      dInterval.word = 0;
                      dInterval05.word = 0;
                        }
                        //printf("pulse[%0d,%02hX]: ", i, pulse[i].pulseSR.Q.word);
                        //printf("wc;(p,pc,dp) = %d;(%d,%d,%d)\n", wCount.Q.w12,pulse[i].interval.Q.w18,pulse[i].tInterval.Q.w18,dInterval.w18);
                        //printf("dTPRF=(%d,%d),%d,%d,%d\n",dInterval.word,dInterval05.word,((pulse[i].tInterval.Q.word>>5) >= pReg->DFSminTPRF.w8),((pulse[i].tInterval.Q.word>>10) <= pReg->DFSmaxTPRF.w8),((dInterval.w18>>3) <= pReg->DFSdTPRF.word));
                        //printf("bits=%d,mask=%d,zeros=%d\n",bits,mask,zeros[0]);

                        if ((!pulse[i].pulseSR.Q.word ||
                      (((pulse[i].tInterval.Q.word>>5) >= pReg->DFSminTPRF.w8) &&
                       ((pulse[i].tInterval.Q.word>>10) <= pReg->DFSmaxTPRF.w8))) &&
                            ((dInterval.w18>>3) <= pReg->DFSdTPRF.word)) {
                      pass = 1;
                      pulse[i].pulseSR.D.word = (pulse[i].pulseSR.Q.w17<<1)+1;
                      if (inPacket.Q.word) pulse[i].tPulses.D.word = (pulse[i].tPulses.Q.word==7)? 7 : pulse[i].tPulses.Q.w3+1;
                      if (pulse[i].pulseSR.Q.word==1) {
                          pulse[i].interval.D.word = pulse[i].tInterval.Q.w18;
                      }
                        } else if ((pulse[i].pulseSR.Q.word > 1) &&
                            ((pulse[i].tInterval.Q.word>>5) >= pReg->DFSminTPRF.w8) &&
                            ((dInterval05.w18>>3) <= (pReg->DFSdTPRF.word))) {
                      //printf("Erased pulse!\n");
                      pass = 1;
                      pulse[i].pulseSR.D.word = (pulse[i].pulseSR.Q.b7<<15)+(pulse[i].pulseSR.Q.b6<<13)+(pulse[i].pulseSR.Q.b5<<11)+(pulse[i].pulseSR.Q.b4<<9)+(pulse[i].pulseSR.Q.b3<<7)+(pulse[i].pulseSR.Q.b2<<5)+(pulse[i].pulseSR.Q.b1<<3)+(pulse[i].pulseSR.Q.b0<<1)+1;
                      if (inPacket.Q.word) pulse[i].tPulses.D.word = (pulse[i].tPulses.Q.word==7)? 7 : pulse[i].tPulses.Q.w3+1;
                      pulse[i].interval.D.word = pulse[i].tInterval.Q.w18;
                        } else {
                      pulse[i].iCount.D.word = pulse[i].iCount.Q.w18+pulse[i].tInterval.Q.w18+1;
                        }
              }
              if (!pass) {
                        if (!inPacket.Q.word) state.D.word = 0;
                        else restart.D.word = 1;
              }
              break;

                case 7: // check FCS and record radar
              FCSPass.D.word = 0;
              if (FCSPass.Q.word == 1) {
                        state.D.word = 1;
              } else {
                        if (match) {
                      //printf("My_Checker: Radar found! SR=(%02hX,%02hX)\n", pulse[0].pulseSR.Q.w8,pulse[1].pulseSR.Q.w8);
                      radarDet.D.word = 1;
                      pReg->detRadar.word = 1;
                      state.D.word = 0;
                        } else {
                      state.D.word = 1;
                        }
              }
              break;

                default: state.D.word = 0;
            }
        }

        // roll back pulse counter if FCS passes
        if (checkFCS && pRX->N_PHY_RX_WR >= 8) {
            Nevada_CheckFCS(pRX->N_PHY_RX_WR-8, pRX->PHY_RX_D+8, &FCSPass.D.word);
            if (FCSPass.D.word) {
             pulse[0].pulseSR.D.word = pulse[0].pulseSR.Q.w17>>pulse[0].tPulses.Q.w4;
             pulse[1].pulseSR.D.word = pulse[1].pulseSR.Q.w17>>pulse[1].tPulses.Q.w4;
            } else if (restart.Q.word) {
             restart.D.word = 0;
             state.D.word = 0;
            }
            pulse[0].tPulses.D.word = 0;
            pulse[1].tPulses.D.word = 0;
        }

        // -------------
        // Trace Signals
        // -------------
        if (pRX->EnableTrace)
        {
            pRX->traceDFS[pRX->k20].State     = state.Q.w3;
            pRX->traceDFS[pRX->k20].PulseDet  = SigIn.PulseDet;
            pRX->traceDFS[pRX->k20].PulsePass = pass;
            pRX->traceDFS[pRX->k20].CheckFCS  = checkFCS;
            pRX->traceDFS[pRX->k20].FCSPass   = FCSPass.Q.b0;
            pRX->traceDFS[pRX->k20].InPacket  = inPacket.Q.b0;;
            pRX->traceDFS[pRX->k20].Match     = match;
            pRX->traceDFS[pRX->k20].RadarDet  = radarDet.Q.b0;
            pRX->traceDFS[pRX->k20].Counter   = wCount.Q.w12;
            pRX->traceDFS[pRX->k20].PulseSR   = pulse[0].pulseSR.Q.w10;
        }
    }
}
// end of Nevada_DFS()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------
