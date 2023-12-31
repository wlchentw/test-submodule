/*
 * HDMI support
 *
 * Copyright (C) 2013 ITE Tech. Inc.
 * Author: Hermes Wu <hermes.wu@ite.com.tw>
 *
 * HDMI TX driver for IT66121
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _CONFIG_H_
#define _CONFIG_H_
/* #pragma message("config.h") */

#ifdef EXTERN_HDCPROM
/* #pragma message("Defined EXTERN_HDCPROM") */
#endif /* EXTERN_HDCPROM */

#define SUPPORT_EDID
/*#define SUPPORT_HDCP*/
/* #define SUPPORT_SHA */
/* #define SUPPORT_AUDIO_MONITOR */
#define AudioOutDelayCnt 250

/* ////////////////////////////// */
/* Video Configuration */
/* //////////////////////////////*/


/* 2010/01/26 added a option to disable HDCP. */
#define SUPPORT_OUTPUTYUV
#define SUPPORT_OUTPUTRGB
/* #define DISABLE_HDMITX_CSC */

#define SUPPORT_INPUTRGB
/* #define SUPPORT_INPUTYUV444 */
/* #define SUPPORT_INPUTYUV422 */
/* #define SUPPORT_SYNCEMBEDDED */
/* #define SUPPORT_DEGEN */
#define NON_SEQUENTIAL_YCBCR422

#define INPUT_COLOR_MODE F_MODE_RGB444
/* #define INPUT_COLOR_MODE F_MODE_YUV422 */
/* #define INPUT_COLOR_MODE F_MODE_YUV444 */

#define INPUT_COLOR_DEPTH 24
/* #define INPUT_COLOR_DEPTH 30 */
/* #define INPUT_COLOR_DEPTH 36 */

/* #define OUTPUT_COLOR_MODE F_MODE_YUV422 */
/* #define OUTPUT_COLOR_MODE F_MODE_YUV444 */
#define OUTPUT_COLOR_MODE F_MODE_RGB444

/* #define OUTPUT_3D_MODE Frame_Pcaking */
/* #define OUTPUT_3D_MODE Top_and_Botton */
/* #define OUTPUT_3D_MODE Side_by_Side */

/* #define INV_INPUT_ACLK */
/* #define INV_INPUT_PCLK */

#ifdef SUPPORT_SYNCEMBEDDED
/* #define INPUT_SIGNAL_TYPE (T_MODE_SYNCEMB)      // 16 bit sync*/
/* embedded */
/* #define INPUT_SIGNAL_TYPE (T_MODE_SYNCEMB | T_MODE_CCIR656) */
/* 8 bit sync embedded */
#define INPUT_SIGNAL_TYPE (T_MODE_SYNCEMB | T_MODE_INDDR | T_MODE_PCLKDIV2)

/* 16 bit sync embedded DDR */
/* #define INPUT_SIGNAL_TYPE (T_MODE_SYNCEMB|T_MODE_INDDR)  */
/* 8  bit sync * embedded DDR */

#define SUPPORT_INPUTYUV422
#ifdef INPUT_COLOR_MODE
#undef INPUT_COLOR_MODE
#endif /* INPUT_COLOR_MODE */
#define INPUT_COLOR_MODE F_MODE_YUV422
#else
/* #pragma message ("Defined separated sync.") */
/* #define INPUT_SIGNAL_TYPE 0 // 24 bit sync separate */
/* #define INPUT_SIGNAL_TYPE ( T_MODE_DEGEN ) */
#define INPUT_SIGNAL_TYPE (T_MODE_INDDR)
/* #define INPUT_SIGNAL_TYPE ( T_MODE_SYNCEMB) */
/* #define INPUT_SIGNAL_TYPE ( T_MODE_CCIR656 | T_MODE_SYNCEMB ) */
#endif

#if defined(SUPPORT_INPUTYUV444) || defined(SUPPORT_INPUTYUV422)
#define SUPPORT_INPUTYUV
#endif

#ifdef SUPPORT_SYNCEMBEDDED
/* #pragma message("defined SUPPORT_SYNCEMBEDDED */
/*for Sync Embedded timing input or CCIR656 input.") */
#endif

/* ///////////////////////////////////// */
/* Audio Configuration */
/* /////////////////////////////////////*/



/* #define SUPPORT_HBR_AUDIO */
#define USE_SPDIF_CHSTAT
#ifndef SUPPORT_HBR_AUDIO
#define INPUT_SAMPLE_FREQ AUDFS_44p1KHz
#define INPUT_SAMPLE_FREQ_HZ 44100L
#define OUTPUT_CHANNEL 2 /* 3 // 4 // 5//6 //7 //8 */

#define CNOFIG_INPUT_AUDIO_TYPE T_AUDIO_LPCM
/* #define CNOFIG_INPUT_AUDIO_TYPE T_AUDIO_NLPCM */
#define CONFIG_INPUT_AUDIO_SPDIF FALSE /* I2S */
/* #define CONFIG_INPUT_AUDIO_SPDIF TRUE // SPDIF */

/* #define I2S_FORMAT 0x00 // 24bit I2S audio */
#define I2S_FORMAT 0x01 /* 32bit I2S audio    hh: 0 for standard I2S */
/* #define I2S_FORMAT 0x02 // 24bit I2S audio, right justify */
/* #define I2S_FORMAT 0x03 // 32bit I2S audio, right justify */

#else /* SUPPORT_HBR_AUDIO */

#define INPUT_SAMPLE_FREQ AUDFS_768KHz
#define INPUT_SAMPLE_FREQ_HZ 768000L
#define OUTPUT_CHANNEL 8
#define CNOFIG_INPUT_AUDIO_TYPE T_AUDIO_HBR
#define CONFIG_INPUT_AUDIO_SPDIF FALSE /* I2S */
/* #define CONFIG_INPUT_AUDIO_SPDIF TRUE // SPDIF */
#define I2S_FORMAT 0x47 /* 32bit audio */
#endif

/* //////////////////////////////////////*/
/* Audio Monitor Configuration */
/* //////////////////////////////////////*/


/* #define HDMITX_AUTO_MONITOR_INPUT */
/* #define HDMITX_INPUT_INFO */

#ifdef HDMITX_AUTO_MONITOR_INPUT
#define HDMITX_INPUT_INFO
#endif

#endif
