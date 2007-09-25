#include "config.h"
#include "cpu.h"
#include "string.h"
#include "lcd.h"
#include "kernel.h"
#include "lcd-target.h"

#define LCDADDR(x, y) (&lcd_framebuffer[(y)][(x)])

static volatile bool lcd_on = true;
volatile bool lcd_poweroff = false;
static unsigned lcd_yuv_options = 0;
/*
** These are imported from lcd-16bit.c
*/
extern unsigned fg_pattern;
extern unsigned bg_pattern;

/* Copies a rectangle from one framebuffer to another. Can be used in
   single transfer mode with width = num pixels, and height = 1 which
   allows a full-width rectangle to be copied more efficiently. */
extern void lcd_copy_buffer_rect(fb_data *dst, const fb_data *src,
                                 int width, int height);

bool lcd_enabled()
{
    return lcd_on;
}

unsigned int LCDBANK(unsigned int address)
{
    return ((address >> 22) & 0xff);
}

unsigned int LCDBASEU(unsigned int address)
{
    return (address & ((1 << 22)-1)) >> 1;
}

unsigned int LCDBASEL(unsigned int address)
{
    address += 320*240*2;
    return (address & ((1 << 22)-1)) >> 1;
}

/* LCD init */
void lcd_init_device(void)
{
    int i;
#ifdef BOOTLOADER
    /* When the Rockbox bootloader starts, we are changing framebuffer address,
       but we don't want what's shown on the LCD to change until we do an
       lcd_update(), so copy the data from the old framebuffer to the new one */
    unsigned short *buf = (unsigned short*)FRAME;

    memcpy(FRAME, (short *)((LCDSADDR1)<<1), 320*240*2);

    /* The Rockbox bootloader is transitioning from RGB555I to RGB565 mode
       so convert the frambuffer data accordingly */
    for(i=0; i< 320*240; i++){
        *buf = ((*buf>>1) & 0x1F) | (*buf & 0xffc0);
        buf++;
    }
#endif

    LCDSADDR1 = (LCDBANK((unsigned)FRAME) << 21) | (LCDBASEU((unsigned)FRAME));
    LCDSADDR2 = LCDBASEL((unsigned)FRAME);
    LCDSADDR3 = 0x000000F0;

#if !defined(BOOTLOADER)
    lcd_poweroff = false;
#endif

    /* ENVID = 1, BPPMODE = 16 bpp, PNRMODE = TFT, MMODE = Each Frame, CLKVAL = 8 */
    LCDCON1 = 0x879;

    /* VCPW = 1, VFPD = 5, LINEVAL = 319, VBPD = 7 */
    LCDCON2 = 0x74FC141;
    
    /* HFPD = 9, HOZVAL = 239, HBPD = 7 */
    LCDCON3 = 0x38EF09;

    /* HSPW = 7 */
    LCDCON4 = 7;
    
    /* HWSWP = 1, INVVFRAM = 1, INVVLINE = 1, FRM565 = 1, All others = 0 */
    LCDCON5 = 0xB01;

    /* LCD controller reset */
    GPBCON = (GPBCON & ~((1<<15)|(1<<17))) | (1<<16)|(1<<14); /* GPB7=OUT, GPB8=OUT */
    GPBDAT |= (1<<7);               /* LCD reset */
    GPBUP  |= (1<<8) | (1<<7) | 1;  /* pullup GPB8, GPB7, GPB0(?) */
    CLKCON |= (1<<5);               /* enable LCD clock */

    /* SPI bus transfer */
    GPBDAT &= ~(1<<8);  /* LCD CS off */

    /* Start the SPI interface */
    CLKCON |= 1<<18;    /* enable SPI clock */
    SPCON0 = 0x3E;      /* enable iterrupt mode, master,active low,format B    */
    SPPRE0 = 0x18;      /* Baud rate = PCLK(50MHz) / 2 / (Prescaler value + 1) */

    /* SPI data - Right now we are not sure what each of these SPI writes is actually
     *    telling the lcd.  Many thanks to Alex Gerchanovsky for discovering them.
     */
    const unsigned char initbuf[] = {
        0,0x0F,1,0x01, 0,0x09,1,0x06, 0,0x16,1,0xA6, 0,0x1E,1,0x49, 0,0x1F,1,0x26, 
        0,0x0B,1,0x2F, 0,0x0C,1,0x2B, 0,0x19,1,0x5E, 0,0x1A,1,0x15, 0,0x1B,1,0x15, 
        0,0x1D,1,0x01, 0,0x00,1,0x03, 0,0x01,1,0x10, 0,0x02,1,0x0A, 0,0x06,1,0x04, 
        0,0x08,1,0x2E, 0,0x24,1,0x12, 0,0x25,1,0x3F, 0,0x26,1,0x0B, 0,0x27,1,0x00,
        0,0x28,1,0x00, 0,0x29,1,0xF6, 0,0x2A,1,0x03, 0,0x2B,1,0x0A, 0,0x04,1,0x01};

    /* Send the SPI data */
    for (i=0;i<(int)sizeof(initbuf);i++)
    {
        while ((SPSTA0&1)==0);
        SPRDAT0 = initbuf[i];
        do{int x;for(x=1000*51/2;x;x--);} while (0);
    }

    /* Stop the SPI interface */
    SPPRE0 = 0;
    SPCON0 = 0;
    CLKCON &= ~(1<<18); /* disable SPI clock */
    GPBDAT |= (1<<8);   /* LCD CS on */
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    fb_data *dst, *src;

    if (!lcd_on)
        return;

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* Clip right */
    if (x < 0)
        width += x, x = 0; /* Clip left */
    if (width <= 0)
        return; /* nothing left to do */

    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y; /* Clip bottom */
    if (y < 0)
        height += y, y = 0; /* Clip top */
    if (height <= 0)
        return; /* nothing left to do */

    /* TODO: It may be faster to swap the addresses of lcd_driver_framebuffer
     * and lcd_framebuffer */
    dst = (fb_data *)FRAME + LCD_WIDTH*y + x;
    src = &lcd_framebuffer[y][x];

    /* Copy part of the Rockbox framebuffer to the second framebuffer */
    if (width < LCD_WIDTH)
    {
        /* Not full width - do line-by-line */
        lcd_copy_buffer_rect(dst, src, width, height);
    }
    else
    {
        /* Full width - copy as one line */
        lcd_copy_buffer_rect(dst, src, LCD_WIDTH*height, 1);
    }
}

void lcd_enable(bool state)
{
    if(!lcd_poweroff)
        return;
    if(state)
    {
        if(!lcd_on)
        {
            lcd_on = true;
            lcd_update();
            LCDCON1 |= 1;
        }
    }
    else 
    {
        if(lcd_on) {
            lcd_on = false;
            LCDCON1 &= ~1;
        }
    }
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    if (!lcd_on)
        return;

    lcd_copy_buffer_rect((fb_data *)FRAME, &lcd_framebuffer[0][0],
                         LCD_WIDTH*LCD_HEIGHT, 1);
}

void lcd_bitmap_transparent_part(const fb_data *src, int src_x, int src_y,
                                 int stride, int x, int y, int width,
                                 int height)
{
    int w, px;
    fb_data *dst;

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* Clip right */
    if (x < 0)
        width += x, x = 0; /* Clip left */
    if (width <= 0)
        return; /* nothing left to do */

    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y; /* Clip bottom */
    if (y < 0)
        height += y, y = 0; /* Clip top */
    if (height <= 0)
        return; /* nothing left to do */

    src += stride * src_y + src_x; /* move starting point */
    dst = &lcd_framebuffer[y][x];

    asm volatile (
    ".rowstart:                             \r\n"
        "mov    %[w], %[width]              \r\n" /* Load width for inner loop */
    ".nextpixel:                            \r\n"
        "ldrh   %[px], [%[s]], #2           \r\n" /* Load src pixel */
        "add    %[d], %[d], #2              \r\n" /* Uncoditionally increment dst */
        "cmp    %[px], %[fgcolor]           \r\n" /* Compare to foreground color */
        "streqh %[fgpat], [%[d], #-2]       \r\n" /* Store foregroud if match */
        "cmpne  %[px], %[transcolor]        \r\n" /* Compare to transparent color */
        "strneh %[px], [%[d], #-2]          \r\n" /* Store dst if not transparent */
        "subs   %[w], %[w], #1              \r\n" /* Width counter has run down? */
        "bgt    .nextpixel                  \r\n" /* More in this row? */
        "add    %[s], %[s], %[sstp], lsl #1 \r\n" /* Skip over to start of next line */
        "add    %[d], %[d], %[dstp], lsl #1 \r\n"
        "subs   %[h], %[h], #1              \r\n" /* Height counter has run down? */
        "bgt    .rowstart                   \r\n" /* More rows? */
        : [w]"=&r"(w), [h]"+&r"(height), [px]"=&r"(px),
          [s]"+&r"(src), [d]"+&r"(dst)
        : [width]"r"(width),
          [sstp]"r"(stride - width),
          [dstp]"r"(LCD_WIDTH - width),
          [transcolor]"r"(TRANSPARENT_COLOR),
          [fgcolor]"r"(REPLACEWITHFG_COLOR),
          [fgpat]"r"(fg_pattern)
    );
}

void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(fb_data *dst,
                                   unsigned char const * const src[3],
                                   int width,
                                   int stride);
extern void lcd_write_yuv420_lines_odither(fb_data *dst,
                                           unsigned char const * const src[3],
                                           int width,
                                           int stride,
                                           int x_screen, /* To align dither pattern */
                                           int y_screen);
/* Performance function to blit a YUV bitmap directly to the LCD */
/* For the Gigabeat - show it rotated */
/* So the LCD_WIDTH is now the height */
void lcd_yuv_blit(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    /* Caches for chroma data so it only need be recaculated every other
       line */
    unsigned char const * yuv_src[3];
    off_t z;

    if (!lcd_on)
        return;

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height >>= 1;

    y = LCD_WIDTH - 1 - y;
    fb_data *dst = (fb_data*)FRAME + x * LCD_WIDTH + y;

    z = stride*src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    if (lcd_yuv_options & LCD_YUV_DITHER)
    {
        do
        {
            lcd_write_yuv420_lines_odither(dst, yuv_src, width, stride, y, x);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            dst -= 2;
            y -= 2;
        }
        while (--height > 0);
    }
    else
    {
        do
        {
            lcd_write_yuv420_lines(dst, yuv_src, width, stride);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            dst -= 2;
        }
        while (--height > 0);
    }
}

void lcd_set_contrast(int val) {
  (void) val;
  // TODO:
}

void lcd_set_invert_display(bool yesno) {
  (void) yesno;
  // TODO:
}

void lcd_blit(const fb_data* data, int bx, int y, int bwidth,
              int height, int stride)
{
  (void) data;
  (void) bx;
  (void) y;
  (void) bwidth;
  (void) height;
  (void) stride;
  //TODO:
}

void lcd_set_flip(bool yesno) {
  (void) yesno;
  // TODO:
}

