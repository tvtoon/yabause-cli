#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* All that to satisfy macros... */
#include "../cdbase.h"
#include "../config.h"
#include "../cs0.h"
#include "../m68kcore.h"
#include "../osdcore.h"
#include "../scsp.h"
#include "../sh2core.h"
#include "../sh2int.h"
#include "../smpc.h"
#include "../sndsdl.h"
#include "../vidsoft.h"
#include "../yabause.h"

#include "cfgfile.h"

/* All strings, numbers, additionals (spaces, "=", and line break). */

#define MAXCFGSIZE ( 9 * PATH_MAX ) + ( 25 * ( ( sizeof(size_t) / 2 ) * 5 ) ) + ( 34 * ( NAME_MAX + 5 ) )

static void GetValue( const char *src, const char *name, char *outvar )
{
 char *tmp = '\0';

 *outvar = 0;
 tmp = strstr(src, name);

 if ( tmp != NULL )
{
  tmp += strlen(name);

  while ((*tmp == ' ') || (*tmp == '=')) tmp++;

  while (*tmp != '\n' && *tmp != 0) *outvar++ = *tmp++;

  *outvar = 0;
}

}

static int GetValuei( const char *src, const char *name )
{
 char *tmp = strstr(src, name);

 if (tmp != NULL)
{
  tmp += strlen(name);

  while ((*tmp == ' ') || (*tmp == '=')) tmp++;

  if (*tmp != '\n') return atoi(tmp);
}

 return 0;
}

int cfg_checks( const yabauseinit_struct cfg )
{
 const char *cfgcv[7] = { cfg.biospath, cfg.cdpath, cfg.ssfpath, cfg.buppath, cfg.mpegpath, cfg.cartpath, cfg.sh1rompath };
 const char *cfgcv2[2] = { cfg.modemip, cfg.modemport };
 const int cfgnv[25] = { cfg.percoretype, cfg.sh1coretype, cfg.sh2coretype, cfg.vidcoretype, cfg.sndcoretype, cfg.m68kcoretype, cfg.cdcoretype, cfg.carttype, cfg.regionid, cfg.videoformattype, cfg.frameskip, cfg.clocksync, cfg.basetime, cfg.usethreads, cfg.numthreads, cfg.osdcoretype, cfg.skip_load, cfg.play_ssf, cfg.use_new_scsp, cfg.use_cd_block_lle, cfg.use_sh2_dma_timing, cfg.use_scu_dma_timing, cfg.sh2_cache_enabled, cfg.use_scsp_dsp_dynarec, cfg.use_scu_dsp_jit };
/*
 const unsigned char cfgev[2] = { cfg.Cpu, cfg.PsxType };
 const unsigned char cfgevv[2] = { 1, 2 };
*/
 int erro = 0, i = 0;
 size_t t = 0;

 for ( ; i < 7; i++ )
{
  t = strlen(cfgcv[i]);
  if ( t > PATH_MAX ) erro++;
}

 for ( i = 0; i < 2; i++ )
{
  t = strlen(cfgcv2[i]);
  if ( t > NAME_MAX ) erro++;
}

 for ( i = 0; i < 25; i++ )
{
  if ( cfgnv[i] > 255 ) erro++;
}
/*
 for ( i = 0; i < 2; i++ )
{
  if ( cfgev[i] > cfgevv[i] ) erro++;
}
*/
 return(erro);
}

size_t cfg_loadf( FILE *f, yabauseinit_struct *cfg )
{
 char cfgbuf[MAXCFGSIZE + 1];
 size_t t = 0;

 t = fread( cfgbuf, 1, MAXCFGSIZE, f );
 cfgbuf[t] = '\0';

 GetValue( cfgbuf, "biospath", cfg->biospath );
 GetValue( cfgbuf, "cdpath", cfg->cdpath );
 GetValue( cfgbuf, "ssfpath", cfg->ssfpath );
 GetValue( cfgbuf, "buppath", cfg->buppath );
 GetValue( cfgbuf, "mpegpath", cfg->mpegpath );
 GetValue( cfgbuf, "cartpath", cfg->cartpath );
 GetValue( cfgbuf, "modemip", cfg->modemip );
 GetValue( cfgbuf, "modemport", cfg->modemport );
 GetValue( cfgbuf, "sh1rompath", cfg->sh1rompath );

 cfg->percoretype = GetValuei( cfgbuf, "percoretype" );
 cfg->sh1coretype = GetValuei( cfgbuf, "sh1coretype" );
 cfg->sh2coretype = GetValuei( cfgbuf, "sh2coretype" );
 cfg->vidcoretype = GetValuei( cfgbuf, "vidcoretype" );
 cfg->sndcoretype = GetValuei( cfgbuf, "sndcoretype" );
 cfg->m68kcoretype = GetValuei( cfgbuf, "m68kcoretype" );
 cfg->cdcoretype = GetValuei( cfgbuf, "cdcoretype" );
 cfg->carttype = GetValuei( cfgbuf, "carttype" );
 cfg->regionid = GetValuei( cfgbuf, "regionid" );
 cfg->videoformattype = GetValuei( cfgbuf, "videoformattype" );
 cfg->frameskip = GetValuei( cfgbuf, "frameskip" );
 cfg->clocksync = GetValuei( cfgbuf, "clocksync" );
 cfg->basetime = GetValuei( cfgbuf, "basetime" );
 cfg->usethreads = GetValuei( cfgbuf, "usethreads" );
 cfg->numthreads = GetValuei( cfgbuf, "numthreads" );
 cfg->osdcoretype = GetValuei( cfgbuf, "osdcoretype" );
 cfg->skip_load = GetValuei( cfgbuf, "skip_load" );
 cfg->play_ssf = GetValuei( cfgbuf, "play_ssf" );
 cfg->use_new_scsp = GetValuei( cfgbuf, "use_new_scsp" );
 cfg->use_cd_block_lle = GetValuei( cfgbuf, "use_cd_block_lle" );
 cfg->use_sh2_dma_timing = GetValuei( cfgbuf, "use_sh2_dma_timing" );
 cfg->use_scu_dma_timing = GetValuei( cfgbuf, "use_scu_dma_timing" );
 cfg->sh2_cache_enabled = GetValuei( cfgbuf, "sh2_cache_enabled" );
 cfg->use_scsp_dsp_dynarec = GetValuei( cfgbuf, "use_scsp_dsp_dynarec" );
 cfg->use_scu_dsp_jit = GetValuei( cfgbuf, "use_scu_dsp_jit" );
 return(t);
}

size_t cfg_savef( FILE *f, const yabauseinit_struct cfg )
{
 const char *cfgc[9] = { "biospath", "cdpath", "ssfpath", "buppath", "mpegpath", "cartpath", "modemip", "modemport", "sh1rompath" };
 const char *cfgcv[9] = { cfg.biospath, cfg.cdpath, cfg.ssfpath, cfg.buppath, cfg.mpegpath, cfg.cartpath, cfg.modemip, cfg.modemport, cfg.sh1rompath };
 const char *cfgn[25] = { "percoretype", "sh1coretype", "sh2coretype", "vidcoretype", "sndcoretype", "m68kcoretype", "cdcoretype", "carttype", "regionid", "videoformattype", "frameskip", "clocksync", "basetime", "usethreads", "numthreads", "osdcoretype", "skip_load", "play_ssf", "use_new_scsp", "use_cd_block_lle", "use_sh2_dma_timing", "use_scu_dma_timing", "sh2_cache_enabled", "use_scsp_dsp_dynarec", "use_scu_dsp_jit" };
 const int cfgnv[25] = { cfg.percoretype, cfg.sh1coretype, cfg.sh2coretype, cfg.vidcoretype, cfg.sndcoretype, cfg.m68kcoretype, cfg.cdcoretype, cfg.carttype, cfg.regionid, cfg.videoformattype, cfg.frameskip, cfg.clocksync, cfg.basetime, cfg.usethreads, cfg.numthreads, cfg.osdcoretype, cfg.skip_load, cfg.play_ssf, cfg.use_new_scsp, cfg.use_cd_block_lle, cfg.use_sh2_dma_timing, cfg.use_scu_dma_timing, cfg.sh2_cache_enabled, cfg.use_scsp_dsp_dynarec, cfg.use_scu_dsp_jit };

 int i = 0;
 size_t t = 0;

 for ( ; i < 9; i++ )
{
  t += (size_t)fprintf( f, "%s = %s\n", cfgc[i], cfgcv[i] );
}

 for ( i = 0 ; i < 25; i++ )
{
  t += (size_t)fprintf( f, "%s = %d\n", cfgn[i], cfgnv[i] );
}

 return(t);
}

void cfg_inis( yabauseinit_struct *cfg )
{
 cfg->percoretype = PERCORE_CLI;
 cfg->sh1coretype = 0;
 cfg->sh2coretype = SH2CORE_INTERPRETER;
 cfg->vidcoretype = VIDCORE_SOFT;
 cfg->sndcoretype = SNDCORE_SDL;
 cfg->m68kcoretype = M68KCORE_MUSASHI;
/* DC
  yinit.cdcoretype = CDCORE_ARCH;
*/
 cfg->cdcoretype = CDCORE_DEFAULT;
 cfg->carttype = CART_NONE;
 cfg->regionid = REGION_AUTODETECT;
/*
 cfg->biospath = '\0';
 cfg->cdpath = '\0';
 cfg->ssfpath = '\0';
 cfg->buppath = '\0';
 cfg->mpegpath = '\0';
 cfg->cartpath = '\0';
 cfg->modemip = '\0';
 cfg->modemport = '\0';
 cfg->sh1rompath = '\0';
*/
 cfg->videoformattype = VIDEOFORMATTYPE_NTSC;
 cfg->frameskip = 0;
 cfg->clocksync = 0;
 cfg->basetime = 0;
 cfg->usethreads = 0;
 cfg->numthreads = 1;
 cfg->osdcoretype = OSDCORE_DUMMY;
 cfg->skip_load = 0;
 cfg->play_ssf = 0;
 cfg->use_new_scsp = 0;
 cfg->use_cd_block_lle = 0;
 cfg->use_sh2_dma_timing = 0;
 cfg->use_scu_dma_timing = 0;
 cfg->sh2_cache_enabled = 0;
 cfg->use_scsp_dsp_dynarec = 0;
 cfg->use_scu_dsp_jit = 0;
}
