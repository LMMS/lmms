/* Generated file, do not edit */

#define IMPULSES      21

#include "impulses/01-unit.h"
#include "impulses/02-steves-flat.h"
#include "impulses/03-stk-m1.h"
#include "impulses/04-fender-68-vibrolux-sm57.h"
#include "impulses/05-fender-68-vibrolux-sm57-off.h"
#include "impulses/06-fender-68-vibrolux-at4050.h"
#include "impulses/07-fender-68-vibrolux-ui87.h"
#include "impulses/08-fender-bassman-sm57.h"
#include "impulses/09-fender-bassman-sm57-off.h"
#include "impulses/10-fender-bassman-at4050.h"
#include "impulses/11-fender-bassman-ui87.h"
#include "impulses/12-fender-superchamp-sm57.h"
#include "impulses/13-fender-superchamp-sm57-off.h"
#include "impulses/14-fender-superchamp-at4050.h"
#include "impulses/15-fender-superchamp-ui87.h"
#include "impulses/16-marshall-jcm2000-sm57.h"
#include "impulses/17-marshall-jcm2000-sm57-off.h"
#include "impulses/18-marshall-plexi-sm57.h"
#include "impulses/19-marshall-plexi-sm57-off.h"
#include "impulses/20-matchless-chieftain-sm57.h"
#include "impulses/21-matchless-chieftain-sm57-off.h"

#ifdef __clang__
void mk_imps(fftw_real **impulse_freq)
#else
inline void mk_imps(fftw_real **impulse_freq)
#endif
{
	int c = 0;
	MK_IMP(unit);
	MK_IMP(steves_flat);
	MK_IMP(stk_m1);
	MK_IMP(fender_68_vibrolux_sm57);
	MK_IMP(fender_68_vibrolux_sm57_off);
	MK_IMP(fender_68_vibrolux_at4050);
	MK_IMP(fender_68_vibrolux_ui87);
	MK_IMP(fender_bassman_sm57);
	MK_IMP(fender_bassman_sm57_off);
	MK_IMP(fender_bassman_at4050);
	MK_IMP(fender_bassman_ui87);
	MK_IMP(fender_superchamp_sm57);
	MK_IMP(fender_superchamp_sm57_off);
	MK_IMP(fender_superchamp_at4050);
	MK_IMP(fender_superchamp_ui87);
	MK_IMP(marshall_jcm2000_sm57);
	MK_IMP(marshall_jcm2000_sm57_off);
	MK_IMP(marshall_plexi_sm57);
	MK_IMP(marshall_plexi_sm57_off);
	MK_IMP(matchless_chieftain_sm57);
	MK_IMP(matchless_chieftain_sm57_off);
};
