/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _WM8721_H
#define _WM8721_H

/* volume/balance/treble/bass interdependency */
#define VOLUME_MIN -730
#define VOLUME_MAX  60

extern int tenthdb2master(int db);
extern int tenthdb2mixer(int db);

extern void audiohw_reset(void);
extern void audiohw_enable_output(bool enable);
extern int audiohw_set_master_vol(int vol_l, int vol_r);
extern void audiohw_set_nsorder(int order);
extern void audiohw_set_sample_rate(int sampling_control);

/* Register addresses and bits */
#define LOUTVOL                 0x02
#define LOUTVOL_LHPVOL_MASK     0x7f
#define LOUTVOL_LZCEN           (1 << 7)
#define LOUTVOL_LRHPBOTH        (1 << 8)

#define ROUTVOL                 0x03
#define ROUTVOL_RHPVOL_MASK     0x7f
#define ROUTVOL_RZCEN           (1 << 7)
#define ROUTVOL_RLHPBOTH        (1 << 8)

#define AAPCTRL                 0x04  /* Analog audio path control */
#define AAPCTRL_DACSEL          (1 << 4)

#define DAPCTRL                 0x05  /* Digital audio path control */
#define DAPCTRL_DEEMP_DISABLE   (0 << 2)
#define DAPCTRL_DEEMP_32KHz     (1 << 2)
#define DAPCTRL_DEEMP_44KHz     (2 << 2)
#define DAPCTRL_DEEMP_48KHz     (3 << 2)
#define DAPCTRL_DEEMP_MASK      (3 << 2)
#define DAPCTRL_DACMU           (1 << 3)

#define PDCTRL                  0x06
#define PDCTRL_DACPD            (1 << 3)
#define PDCTRL_OUTPD            (1 << 4)
#define PDCTRL_POWEROFF         (1 << 7)

#define AINTFCE                 0x07
#define AINTFCE_FORMAT_MSB_RJUST(0 << 0)
#define AINTFCE_FORMAT_MSB_LJUST(1 << 0)
#define AINTFCE_FORMAT_I2S      (2 << 0)
#define AINTFCE_FORMAT_DSP      (3 << 0)
#define AINTFCE_FORMAT_MASK     (3 << 0)
#define AINTFCE_IWL_16BIT       (0 << 2)
#define AINTFCE_IWL_20BIT       (1 << 2)
#define AINTFCE_IWL_24BIT       (2 << 2)
#define AINTFCE_IWL_32BIT       (3 << 2)
#define AINTFCE_IWL_MASK        (3 << 2)
#define AINTFCE_LRP_I2S_RLO     (0 << 4)
#define AINTFCE_LRP_I2S_RHI     (1 << 4)
#define AINTFCE_DSP_MODE_A      (0 << 4)
#define AINTFCE_DSP_MODE_B      (1 << 4)
#define AINTFCE_LRSWAP          (1 << 5)
#define AINTFCE_MS              (1 << 6)
#define AINTFCE_BCLKINV         (1 << 7)

#define SAMPCTRL                0x08
#define SAMPCTRL_USB            (1 << 0)
#define SAMPCTRL_BOSR_NOR_256fs (0 << 1)
#define SAMPCTRL_BOSR_NOR_384fs (1 << 1)
#define SAMPCTRL_BOSR_USB_250fs (0 << 1)
#define SAMPCTRL_BOSR_USB_272fs (1 << 1)
/* Bits 2-5:
 * Sample rate setting are device-specific. See WM8731(L) datasheet
 * for proper settings for the device's clocking */
#define SAMPCTRL_SR_MASK        (0xf << 2)
#define SAMPCTRL_CLKIDIV2       (1 << 6)

#define ACTIVECTRL              0x09
#define ACTIVECTRL_ACTIVE       (1 << 0)

#define RESET                   0x0f
#define RESET_RESET             0x0

/* SAMPCTRL values for the supported samplerates (24MHz MCLK/USB): */
#define WM8721_USB24_8000HZ     0x4d
#define WM8721_USB24_32000HZ    0x59
#define WM8721_USB24_44100HZ    0x63
#define WM8721_USB24_48000HZ    0x41
#define WM8721_USB24_88200HZ    0x7f
#define WM8721_USB24_96000HZ    0x5d

#endif /* _WM8721_H */
