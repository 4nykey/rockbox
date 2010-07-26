#include "codeclib.h"
#include "../libasf/asf.h"

#if   (CONFIG_CPU == MCF5250) || defined(CPU_S5L870X)
/* Enough IRAM but performance suffers with ICODE_ATTR. */
#define IBSS_ATTR_WMAPRO_LARGE_IRAM   IBSS_ATTR
#define ICODE_ATTR_WMAPRO_LARGE_IRAM
#define ICONST_ATTR_WMAPRO_LARGE_IRAM ICONST_ATTR
#define ICONST_ATTR_WMAPRO_WIN_VS_TMP

#elif (CONFIG_CPU == PP5022) || (CONFIG_CPU == PP5024)
/* Enough IRAM to move additional data and code to it. */
#define IBSS_ATTR_WMAPRO_LARGE_IRAM   IBSS_ATTR
#define ICODE_ATTR_WMAPRO_LARGE_IRAM  ICODE_ATTR
#define ICONST_ATTR_WMAPRO_LARGE_IRAM ICONST_ATTR
#define ICONST_ATTR_WMAPRO_WIN_VS_TMP

#else
/* Not enough IRAM available. */
#define IBSS_ATTR_WMAPRO_LARGE_IRAM
#define ICODE_ATTR_WMAPRO_LARGE_IRAM
#define ICONST_ATTR_WMAPRO_LARGE_IRAM
/* Models with large IRAM put tmp to IRAM rather than window coefficients as
 * this is the fastest option. On models with smaller IRAM the 2nd-best option
 * is to move the window coefficients to IRAM. */
#define ICONST_ATTR_WMAPRO_WIN_VS_TMP ICONST_ATTR
#endif

int decode_init(asf_waveformatex_t *wfx);
int decode_packet(asf_waveformatex_t *wfx,
                  void *data, int *data_size, void* pktdata, int size);
