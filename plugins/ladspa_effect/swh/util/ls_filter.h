#ifndef LS_FILTER_H
#define LS_FILTER_H

#include <util/biquad.h>

#define FILT_MIDI_2_FREQ(m) (m * 80.0f + 10.0f)
#define FILT_MIDI_2_RESO(m) (m * 0.00787f)

#define LSF_BW 0.9
#define LSF_FB 0.9f

typedef struct {
    biquad filt;
    biquad bp_filt;
    bq_t scale;
    bq_t resonance;
} ls_filt;

typedef enum {
    LS_FILT_TYPE_LP = 0,
    LS_FILT_TYPE_BP = 1,
    LS_FILT_TYPE_HP = 2
} ls_filt_type;

static inline void ls_filt_init(ls_filt *f)
{
	biquad_init(&(f->filt));
	biquad_init(&(f->bp_filt));
}

static inline void ls_filt_setup(ls_filt *f, ls_filt_type t, bq_t cutoff,
				 bq_t resonance, bq_t fs)
{
	bp_set_params(&(f->bp_filt), cutoff, 0.7, fs);

	switch(t) {
	case LS_FILT_TYPE_LP:
		lp_set_params(&(f->filt), cutoff, 1.0 - resonance * LSF_BW, fs);
		break;
	case LS_FILT_TYPE_BP:
		bp_set_params(&(f->filt), cutoff, 1.0 - resonance * LSF_BW, fs);
		break;
	case LS_FILT_TYPE_HP:
		hp_set_params(&(f->filt), cutoff, 1.0 - resonance * LSF_BW, fs);
		break;
	default:
		/* oops, its not a known type - should really happen, but
		   lets make the output silent just in case */
		lp_set_params(&(f->filt), 1.0, 1.0, fs);
		break;
	}

	f->scale = 1.0f - resonance * 0.7f;
	f->resonance = resonance;
}

static inline bq_t ls_filt_run(ls_filt *f, bq_t in)
{
	return biquad_run(&(f->filt), in) * f->scale +
	  biquad_run_fb(&(f->bp_filt), in, f->resonance * LSF_FB) *
	  f->resonance;
}

#endif
