#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <zlib.h>
#include <SDL/SDL.h>

#include "../cdbase.h"
#include "../cs0.h"
#include "../cs2.h"
#include "../debug.h"
#include "../m68kcore.h"
#include "../m68kc68k.h"
#include "../peripheral.h"
#ifdef ARCH_IS_LINUX
#include "../perlinuxjoy.h"
#endif
#include "../persdljoy.h"
#include "../scsp.h"
#include "../sh2core.h"
#include "../sh2int.h"
#include "../sndsdl.h"
#include "../sndal.h"
#include "../vdp2.h"
#include "../vidogl.h"
#include "../vidsoft.h"
#include "../yabause.h"
#include "../yui.h"

#include "cfgfile.h"

#ifdef WORDS_BIGENDIAN
#define RMASK 0xFF000000
#define GMASK 0x00FF0000
#define BMASK 0x0000FF00
#define AMASK 0x00000000
#else
#define RMASK 0x000000FF
#define GMASK 0x0000FF00
#define BMASK 0x00FF0000
#define AMASK 0x00000000
#endif

SDL_Surface *rgbs = 0, *screen = 0;

/* Major workarounder... */
yabauseinit_struct Config;
char staten[PATH_MAX + 1];
int emurun = 1, halt = 0, statec = 0;
size_t h = 0;
/* Last character from the state name. */
unsigned char ultcs = 6;

int PCLIInit( void );
void PCLIDeInit( void );
int PCLIEvents( void );
u32 PCLIScan( u32 flags );
void PCLIFlush( void );
void PCLIKeyName( u32 key, char *name, int size );

PerInterface_struct PERCLI =
{
 PERCORE_CLI, "Command Line Input Interface",
 PCLIInit, PCLIDeInit, PCLIEvents, PCLIScan, 0, PCLIFlush, PCLIKeyName
};

int PCLIInit(void)
{
 PerPad_struct *pads[2];
 int i = 0, j = 0;

 PerPortReset();
 pads[0] = PerPadAdd(&PORTDATA1);
 pads[1] = PerPadAdd(&PORTDATA2);

 const unsigned int keys[2][14] =
{
 { SDLK_UP, SDLK_RIGHT, SDLK_DOWN, SDLK_LEFT, SDLK_v, SDLK_f, SDLK_RETURN, SDLK_z, SDLK_x, SDLK_c, SDLK_a, SDLK_s, SDLK_d, 0 },
 { SDLK_KP8, SDLK_KP6, SDLK_KP2, SDLK_KP4, SDLK_KP9, SDLK_KP7, SDLK_KP5, SDLK_DELETE, SDLK_END, SDLK_PAGEDOWN, SDLK_INSERT, SDLK_HOME, SDLK_PAGEUP, 0 }
};

 for ( ; i < 2; i++ )
{

  for ( j = 0; j < 13; j++ )
{
   PerSetKey( keys[i][j], j, pads[i] );
}

}

 return(0);
}

void PCLIDeInit(void) {}

int PCLIEvents(void)
{
 SDL_Event evk;
 int i = 0, j = 1, r = 0, t = 0;

 i = SDL_PollEvent( &evk );

 if ( i == 1 )
{

  switch ( evk.type )
{
   case SDL_KEYDOWN:
    if ( emurun == 1 ) PerKeyDown( (u32)evk.key.keysym.sym );
    break;

   case SDL_KEYUP:
    switch( evk.key.keysym.sym )
{
/* XK_Fx*/
     case SDLK_F1:
      t = YabSaveState( staten );
      if ( t != 0 ) fprintf( stderr, "Couldn't save state file \"%s\"!\n", staten );
      break;

      case SDLK_F2:
       if ( statec < 9 ) statec++;
       else statec = 0;
       staten[h + ultcs] = statec;
       printf( "State slot %d.\n", statec );
       break;

      case SDLK_F3:
       t = YabLoadState( staten );
       if ( t != 0 ) fprintf( stderr, "Couldn't load state file \"%s\"!\n", staten );
       break;
/*
      case SDLK_F4:
       GPU_makeSnapshot();
       break;
*/
      case SDLK_F5:
       if ( Config.frameskip == 1 ) EnableAutoFrameSkip();
       else DisableAutoFrameSkip();
       Config.frameskip = !Config.frameskip;
       break;
       /*
      case SDLK_F6:
       break;

      case SDLK_F7:
       break;

      case SDLK_F8:
       break;
       */
/* Reserved for CD change...
      case SDLK_F9:
       break;

      case SDLK_F10:
       break;
*/
      case SDLK_F12:
       YabauseReset();
       printf( "System RESET!\n" );
       break;

/* XK_Escape:*/
      case SDLK_ESCAPE:
       r = 1;
       break;

      case SDLK_PAUSE:
       if ( emurun == 1 ) ScspMuteAudio(SCSP_MUTE_SYSTEM);
       else ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
       emurun = !emurun;
       break;

      default:
       if ( emurun == 1 ) PerKeyUp( (u32)evk.key.keysym.sym );
       break;
}

     break;

    case SDL_QUIT:
     r = 1;
     break;

    default:
     break;
}

}

 if ( emurun == 1 ) YabauseExec();
 return(r);
}

void PCLIFlush(void) {}
u32 PCLIScan(u32 flags) { return 0; }
void PCLIKeyName(u32 key, char *name, int size) { snprintf( name, size, "%x", (unsigned int)key ); }

/* Not null, unlike others... */
M68K_struct * M68KCoreList[4] =
{
 &M68KDummy,
#ifdef HAVE_MUSASHI
 &M68KMusashi,
#else
 &M68KDummy,
#endif

#ifdef HAVE_C68K
 &M68KC68K,
#else
 &M68KDummy,
#endif

#ifdef HAVE_Q68
 &M68KQ68
#else
 &M68KDummy
#endif
};

SH2Interface_struct *SH2CoreList[3] =
{
 &SH2Interpreter,
 &SH2DebugInterpreter,
#ifdef SH2_DYNAREC
 &SH2Dynarec
#else
 &SH2Interpreter
#endif
};

PerInterface_struct *PERCoreList[4] =
{
 &PERDummy,
 &PERCLI,
 &PERSDLJoy,

#ifdef ARCH_IS_LINUX
 &PERLinuxJoy
#else
 &PERDummy
#endif
};

CDInterface *CDCoreList[3] =
{
 &DummyCD,
 &ISOCD,
#ifndef UNKNOWN_ARCH
 &ArchCD
#else
 &DummyCD
#endif
};

SoundInterface_struct *SNDCoreList[2] = { &SNDDummy, &SNDSDL };
VideoInterface_struct *VIDCoreList[2] = { &VIDDummy, &VIDSoft };

#ifdef YAB_PORT_OSD
OSD_struct *OSDCoreList[3] =
{
 &OSDDummy,
#ifdef HAVE_LIBGLUT
 &OSDGlut,
#else
 &OSDDummy,
#endif
 &OSDSoft
};
#endif

void YuiSwapBuffers(void)
{
 int bufh = 0, bufw = 0, i = 0, j = 0;

 i = SDL_LockSurface( rgbs );

 if ( i != 0 )
{
  fprintf( stderr, "Screen lock error!\n");
  halt = 1;
}
 else
{
/*  memcpy( rgbs->pixels, dispbuffer, 704 * 512 * 4 );*/
  VIDCore->GetGlSize( &bufw, &bufh );
/* We don't know the control flow yet, avoid bug... */
  if (bufw > 704) bufw = 704;
  if (bufh > 512) bufh = 512;

  for ( i = 0, j = 0; i < ( bufw * bufh * 4 ); i += bufw * 4, j += ( 704 - bufw ) * 4 )
{
   memcpy( &rgbs->pixels[i + j], &dispbuffer[i], bufw * 4 );
}

  SDL_UnlockSurface( rgbs );
  i = SDL_BlitSurface( rgbs, 0, screen, 0 );

  if ( i != 0 )
{
   fprintf( stderr, "Screen blit error!\n");
   halt = 1;
}
  else
{
   SDL_UpdateRect( screen, 0, 0, 0, 0 );
}

}

}

void YuiErrorMsg(const char *string)
{
  fprintf( stderr, "%s", string );
}

#ifdef __DEBUG__
void printcfg( const yabauseinit_struct *cfg )
{
 printf( "Peripheral core: \"%d\".\n", cfg->percoretype );
 printf( "SH1 core: \"%d\".\n", cfg->sh1coretype );
 printf( "SH2 core: \"%d\".\n", cfg->sh2coretype );
 printf( "Video core: \"%d\".\n", cfg->vidcoretype );
 printf( "Sound core: \"%d\".\n", cfg->sndcoretype );
 printf( "M68K core: \"%d\".\n", cfg->m68kcoretype );
 printf( "CD core: \"%d\".\n", cfg->cdcoretype );
 printf( "Cartridge type: \"%d\".\n", cfg->carttype );
 printf( "Region ID: \"%d\".\n", cfg->regionid );
/**/
 printf( "BIOS path: \"%s\".\n", cfg->biospath );
 printf( "CD path: \"%s\".\n", cfg->cdpath );
 printf( "SSF path: \"%s\".\n", cfg->ssfpath );
 printf( "Backup RAM path: \"%s\".\n", cfg->buppath );
 printf( "MPEG card path: \"%s\".\n", cfg->mpegpath );
 printf( "Cartridge path: \"%s\".\n", cfg->cartpath );
 printf( "Modem IP: \"%s\".\n", cfg->modemip );
 printf( "Modem port: \"%s\".\n", cfg->modemport );
 printf( "SH1 ROM path: \"%s\".\n", cfg->sh1rompath );
/**/
 printf( "Video format: \"%d\".\n", cfg->videoformattype );
 printf( "Automatic frameskip: \"%d\".\n", cfg->frameskip );
 printf( "Clocksync: \"%d\".\n", cfg->clocksync );
 printf( "Default RTC time: \"%d\".\n", cfg->basetime );
 printf( "Thread: \"%d\".\n", cfg->usethreads );
 printf( "Threads number: \"%d\".\n", cfg->numthreads );
 printf( "On-screen display core: \"%d\".\n", cfg->osdcoretype );
 printf( "Skip BIOS loading: \"%d\".\n", cfg->skip_load );
 printf( "Play SSF: \"%d\".\n", cfg->play_ssf );
 printf( "SCSP2: \"%d\".\n", cfg->use_new_scsp );
 printf( "Low level CD block: \"%d\".\n", cfg->use_cd_block_lle );
 printf( "SH2 DMA timing: \"%d\".\n", cfg->use_sh2_dma_timing );
 printf( "SCU DMA timing: \"%d\".\n", cfg->use_scu_dma_timing );
 printf( "SH2 cache: \"%d\".\n", cfg->sh2_cache_enabled );
 printf( "SCSP digital signal processor recompiler: \"%d\".\n", cfg->use_scsp_dsp_dynarec );
 printf( "SCU digital signal processor recompiler: \"%d\".\n", cfg->use_scu_dsp_jit );
}
#endif

int main( int argc, char **argv )
{
 FILE *cfgf = 0;
 char *exef = '\0', *pchar = '\0', *tchar = '\0';
 char cfgn[PATH_MAX + 1], biospath[PATH_MAX + 1], cdpath[PATH_MAX + 1], ssfpath[PATH_MAX + 1], buppath[PATH_MAX + 1], mpegpath[PATH_MAX + 1], cartpath[PATH_MAX + 1], modemip[NAME_MAX + 1], modemport[NAME_MAX + 1], sh1rompath[PATH_MAX + 1];
 int i = 0;
 size_t t = 0;
 unsigned int exeaddr = 0x06004000;

 printf( "Yabause version %s (%s).\n", _VERSION_, __DATE__ );

 cfg_inis( &Config );
 Config.biospath = biospath;
 Config.cdpath = cdpath;
 Config.ssfpath = ssfpath;
 Config.buppath = buppath;
 Config.mpegpath = mpegpath;
 Config.cartpath = cartpath;
 Config.modemip = modemip;
 Config.modemport = modemport;
 Config.sh1rompath = sh1rompath;
 pchar = getenv("HOME");
 h = strlen( pchar );
/*
 The threats: "/yabause.ini", "/TG-50000  .0" (state file), and "/yabause00000.png".
*/
 if ( h > ( PATH_MAX - 17 ) )
{
  fprintf( stderr, "HOME path is too big...\n" );
  return(1);
}

 strncpy( cfgn, pchar, h );
 strncpy( &cfgn[h], "/yabause.ini", 12 );
 t = h + 12;
 cfgn[t] = '\0';
 sprintf( staten, "%s/BIOS.%1d", pchar, statec );
 staten[h + 7] = '\0';
 cfgf = fopen( cfgn, "rb");

 if ( cfgf == 0 )
{
  fprintf( stderr, "Config file not found, using default.\n" );
}

 printf( "Config path: \"%s\".\n", cfgn );
 t = cfg_loadf( cfgf, &Config );
 fclose(cfgf);
#ifdef __DEBUG__
 printf( "Bytes read: %ld.\n", t );
 printcfg( &Config );
#endif
 i = cfg_checks( Config );

 if ( i != 0 )
{
  fprintf( stderr, "Config file errors count: %d.\n", i );
  return(1);
}

 do
{
  i = getopt( argc, argv, "a:e:fhi:m:" );

  switch ( i )
{
   case 'a':
    t = strtoul( optarg, &tchar, 16 );

    if ( tchar[0] != '\0' )
{
     fprintf( stderr, "Endereço incorreto: \"%s\"!\n", optarg );
     return(2);
}

    exeaddr = t;
    break;

   case 'e':
    exef = optarg;
    Config.cdpath = '\0';
    t = strlen(exef);

    if ( t > PATH_MAX )
{
     fprintf( stderr, "Executable path is too big.\n" );
     return(2);
}
    break;

   case 'f':
    Config.frameskip = 1;
    break;

   case 'h':
    printf( "%s [-e executable] -f (frameskip) -h [-i ISO_file] -m (mute sound) --\n\n", argv[0] );
    return(0);

   case 'i':
    exef = '\0';
    Config.cdpath = optarg;
    t = strlen(Config.cdpath);

    if ( t > PATH_MAX )
{
     fprintf( stderr, "ISO path is too big.\n" );
     return(2);
}
    Config.cdcoretype = CDCORE_ISO;
    break;

   case 'm':
    Config.sndcoretype = SNDCORE_DUMMY;
    break;

   case '?':
    fprintf( stderr, "Unknown option \"%c\".\n", optopt );
    return(2);

   default:
    break;
}

} while ( i != -1 );

 i = SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK );

 if ( i == -1 )
{
  fprintf( stderr, "SDL couldn't be initialised!\n" );
  return(3);
}

 screen = SDL_SetVideoMode( 704, 512, 32, 0 );

 if ( screen == 0 )
{
  fprintf( stderr, "Couldn't set video mode.\n" );
  return(3);
}

 rgbs = SDL_CreateRGBSurface( 0, 704, 512, 32, RMASK, GMASK, BMASK, AMASK );

 if ( rgbs == 0 )
{
  fprintf( stderr, "Couldn't set surface.\n" );
  return(3);
}

 i = YabauseInit( &Config );

 if ( i != 0 )
{
  fprintf( stderr, "Error at inialization.\n" );
  return(3);
}

/*
 Should be checked by the options, but CS2ChangeCore doesn't set it properly...
*/
 if ( Cs2Area->cdi->id != CDCORE_DUMMY )
{
  ultcs = 12;
/* Default state filename... */
  sprintf( staten, "%s/%s.%1d", pchar, cdip->itemnum, statec );
  staten[h + 13] = '\0';
  printf( "Setting default state filename.\n%s\n", staten );

  if ( exef != '\0' )
{
   printf( "Loading executable...\n" );
   MappedMemoryLoadExec( exef, exeaddr );
}
  else
{
   printf( "Loading ISO or CDROM...\n" );
}

}

 printf( "Ready, set, go!\n" );

 for ( ; halt == 0; )
{
  halt = PERCore->HandleEvents();
}

/* Final... */
 YabauseDeInit();

 printf( "Saving configuration.\n" );
 cfgf = fopen( cfgn, "wb");

 if ( cfgf == 0 )
{
  fprintf( stderr, "Error opening config file for write: \"%s\".\n", cfgn );
  return(3);
}

 t = cfg_savef( cfgf, Config );
#ifdef __DEBUG__
 printf( "Bytes written: %ld.\n", t );
#endif
 fclose(cfgf);
 printf( "BYE!\n" );
 SDL_Quit();
 return(0);
}
