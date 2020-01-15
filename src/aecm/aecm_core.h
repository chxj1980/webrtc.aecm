/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// Performs echo control (suppression) with fft routines in fixed-point.

#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AECM_AECM_CORE_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AECM_AECM_CORE_H_

#include "signal_processing_library.h"
#include "aecm_defines.h"
#include "ring_buffer.h"
#include "typedefs.h"

#ifdef _MSC_VER  // visual c++
#define ALIGN8_BEG __declspec(align(8))
#define ALIGN8_END
#else  // gcc or icc
#define ALIGN8_BEG
#define ALIGN8_END __attribute__((aligned(8)))
#endif

typedef struct {
    int16_t real;
    int16_t imag;
} complex16_t;

typedef struct {
    int farBufWritePos;
    int farBufReadPos;
    int knownDelay;
    int lastKnownDelay;
    int firstVAD;  // Parameter to control poorly initialized channels

    RingBuffer* farFrameBuf;
    RingBuffer* nearNoisyFrameBuf;
    RingBuffer* nearCleanFrameBuf;
    RingBuffer* outFrameBuf;

    int16_t farBuf[FAR_BUF_LEN];

    int16_t mult;
    uint32_t seed;

    // Delay estimation variables
    void* delay_estimator_farend;
    void* delay_estimator;
    uint16_t currentDelay;
    // Far end history variables
    // TODO(bjornv): Replace |far_history| with ring_buffer.
    uint16_t far_history[PART_LEN1 * MAX_DELAY];
    int far_history_pos;
    int far_q_domains[MAX_DELAY];

    int16_t nlpFlag;
    int16_t fixedDelay;

    uint32_t totCount;

    int16_t dfaCleanQDomain;
    int16_t dfaCleanQDomainOld;
    int16_t dfaNoisyQDomain;
    int16_t dfaNoisyQDomainOld;

    int16_t nearLogEnergy[MAX_BUF_LEN];
    int16_t farLogEnergy;
    int16_t echoAdaptLogEnergy[MAX_BUF_LEN];
    int16_t echoStoredLogEnergy[MAX_BUF_LEN];

    // The extra 16 or 32 bytes in the following buffers are for alignment based
    // Neon code.
    // It's designed this way since the current GCC compiler can't align a
    // buffer in 16 or 32 byte boundaries properly.
    int16_t channelStored_buf[PART_LEN1 + 8];
    int16_t channelAdapt16_buf[PART_LEN1 + 8];
    int32_t channelAdapt32_buf[PART_LEN1 + 8];
    int16_t xBuf_buf[PART_LEN2 + 16];  // farend
    int16_t dBufClean_buf[PART_LEN2 + 16];  // nearend
    int16_t dBufNoisy_buf[PART_LEN2 + 16];  // nearend
    int16_t outBuf_buf[PART_LEN + 8];

    // Pointers to the above buffers
    int16_t *channelStored;
    int16_t *channelAdapt16;
    int32_t *channelAdapt32;
    int16_t *xBuf;
    int16_t *dBufClean;
    int16_t *dBufNoisy;
    int16_t *outBuf;

    int32_t echoFilt[PART_LEN1];
    int16_t nearFilt[PART_LEN1];
    int32_t noiseEst[PART_LEN1];
    int           noiseEstTooLowCtr[PART_LEN1];
    int           noiseEstTooHighCtr[PART_LEN1];
    int16_t noiseEstCtr;
    int16_t cngMode;

    int32_t mseAdaptOld;
    int32_t mseStoredOld;
    int32_t mseThreshold;

    int16_t farEnergyMin;
    int16_t farEnergyMax;
    int16_t farEnergyMaxMin;
    int16_t farEnergyVAD;
    int16_t farEnergyMSE;
    int currentVADValue;
    int16_t vadUpdateCount;

    int16_t startupState;
    int16_t mseChannelCount;
    int16_t supGain;
    int16_t supGainOld;

    int16_t supGainErrParamA;
    int16_t supGainErrParamD;
    int16_t supGainErrParamDiffAB;
    int16_t supGainErrParamDiffBD;

    struct RealFFT* real_fft;

#ifdef AEC_DEBUG
    FILE *farFile;
    FILE *nearFile;
    FILE *outFile;
#endif
} AecmCore_t;

////////////////////////////////////////////////////////////////////////////////
// WebRtcAecm_CreateCore(...)
//
// Allocates the memory needed by the AECM. The memory needs to be
// initialized separately using the WebRtcAecm_InitCore() function.
//
// Input:
//      - aecm          : Instance that should be created
//
// Output:
//      - aecm          : Created instance
//
// Return value         :  0 - Ok
//                        -1 - Error
//
int WebRtcAecm_CreateCore(AecmCore_t **aecm);

////////////////////////////////////////////////////////////////////////////////
// WebRtcAecm_InitCore(...)
//
// This function initializes the AECM instant created with
// WebRtcAecm_CreateCore(...)
// Input:
//      - aecm          : Pointer to the AECM instance
//      - samplingFreq  : Sampling Frequency
//
// Output:
//      - aecm          : Initialized instance
//
// Return value         :  0 - Ok
//                        -1 - Error
//
int WebRtcAecm_InitCore(AecmCore_t * const aecm, int samplingFreq);

////////////////////////////////////////////////////////////////////////////////
// WebRtcAecm_FreeCore(...)
//
// This function releases the memory allocated by WebRtcAecm_CreateCore()
// Input:
//      - aecm          : Pointer to the AECM instance
//
// Return value         :  0 - Ok
//                        -1 - Error
//           11001-11016: Error
//
int WebRtcAecm_FreeCore(AecmCore_t *aecm);

int WebRtcAecm_Control(AecmCore_t *aecm, int delay, int nlpFlag);

////////////////////////////////////////////////////////////////////////////////
// WebRtcAecm_InitEchoPathCore(...)
//
// This function resets the echo channel adaptation with the specified channel.
// Input:
//      - aecm          : Pointer to the AECM instance
//      - echo_path     : Pointer to the data that should initialize the echo
//                        path
//
// Output:
//      - aecm          : Initialized instance
//
void WebRtcAecm_InitEchoPathCore(AecmCore_t* aecm,
                                 const int16_t* echo_path);

////////////////////////////////////////////////////////////////////////////////
// WebRtcAecm_ProcessFrame(...)
//
// This function processes frames and sends blocks to
// WebRtcAecm_ProcessBlock(...)
//
// Inputs:
//      - aecm          : Pointer to the AECM instance
//      - farend        : In buffer containing one frame of echo signal
//      - nearendNoisy  : In buffer containing one frame of nearend+echo signal
//                        without NS
//      - nearendClean  : In buffer containing one frame of nearend+echo signal
//                        with NS
//
// Output:
//      - out           : Out buffer, one frame of nearend signal          :
//
//
int WebRtcAecm_ProcessFrame(AecmCore_t * aecm, const int16_t * farend,
                            const int16_t * nearendNoisy,
                            const int16_t * nearendClean,
                            int16_t * out);

////////////////////////////////////////////////////////////////////////////////
// WebRtcAecm_ProcessBlock(...)
//
// This function is called for every block within one frame
// This function is called by WebRtcAecm_ProcessFrame(...)
//
// Inputs:
//      - aecm          : Pointer to the AECM instance
//      - farend        : In buffer containing one block of echo signal
//      - nearendNoisy  : In buffer containing one frame of nearend+echo signal
//                        without NS
//      - nearendClean  : In buffer containing one frame of nearend+echo signal
//                        with NS
//
// Output:
//      - out           : Out buffer, one block of nearend signal          :
//
//
int WebRtcAecm_ProcessBlock(AecmCore_t * aecm, const int16_t * farend,
                            const int16_t * nearendNoisy,
                            const int16_t * noisyClean,
                            int16_t * out);

////////////////////////////////////////////////////////////////////////////////
// WebRtcAecm_BufferFarFrame()
//
// Inserts a frame of data into farend buffer.
//
// Inputs:
//      - aecm          : Pointer to the AECM instance
//      - farend        : In buffer containing one frame of farend signal
//      - farLen        : Length of frame
//
void WebRtcAecm_BufferFarFrame(AecmCore_t * const aecm,
                               const int16_t * const farend,
                               const int farLen);

////////////////////////////////////////////////////////////////////////////////
// WebRtcAecm_FetchFarFrame()
//
// Read the farend buffer to account for known delay
//
// Inputs:
//      - aecm          : Pointer to the AECM instance
//      - farend        : In buffer containing one frame of farend signal
//      - farLen        : Length of frame
//      - knownDelay    : known delay
//
void WebRtcAecm_FetchFarFrame(AecmCore_t * const aecm,
                              int16_t * const farend,
                              const int farLen, const int knownDelay);

///////////////////////////////////////////////////////////////////////////////
// Some function pointers, for internal functions shared by ARM NEON and
// generic C code.
//
typedef void (*CalcLinearEnergies)(
    AecmCore_t* aecm,
    const uint16_t* far_spectrum,
    int32_t* echoEst,
    uint32_t* far_energy,
    uint32_t* echo_energy_adapt,
    uint32_t* echo_energy_stored);
extern CalcLinearEnergies WebRtcAecm_CalcLinearEnergies;

typedef void (*StoreAdaptiveChannel)(
    AecmCore_t* aecm,
    const uint16_t* far_spectrum,
    int32_t* echo_est);
extern StoreAdaptiveChannel WebRtcAecm_StoreAdaptiveChannel;

typedef void (*ResetAdaptiveChannel)(AecmCore_t* aecm);
extern ResetAdaptiveChannel WebRtcAecm_ResetAdaptiveChannel;

// For the above function pointers, functions for generic platforms are declared
// and defined as static in file aecm_core.c, while those for ARM Neon platforms
// are declared below and defined in file aecm_core_neon.s.
#if (defined WEBRTC_DETECT_ARM_NEON) || defined (WEBRTC_ARCH_ARM_NEON)
void WebRtcAecm_CalcLinearEnergiesNeon(AecmCore_t* aecm,
                                       const uint16_t* far_spectrum,
                                       int32_t* echo_est,
                                       uint32_t* far_energy,
                                       uint32_t* echo_energy_adapt,
                                       uint32_t* echo_energy_stored);

void WebRtcAecm_StoreAdaptiveChannelNeon(AecmCore_t* aecm,
                                         const uint16_t* far_spectrum,
                                         int32_t* echo_est);

void WebRtcAecm_ResetAdaptiveChannelNeon(AecmCore_t* aecm);
#endif

#endif
