typedef struct {
    int writePos;
    int bufferSize;
    int readPos;
    int readPosFrac;
    int readPosFrac_inc;
    int dummy;
    int seedVal;
    int randLine_cnt;
    SPFLOAT filterState;
    SPFLOAT *buf;
} sp_revsc_dl;

typedef struct  {
    SPFLOAT feedback, lpfreq;
    SPFLOAT iSampleRate, iPitchMod, iSkipInit;
    SPFLOAT sampleRate;
    SPFLOAT dampFact;
    SPFLOAT prv_LPFreq;
    int initDone;
    sp_revsc_dl delayLines[8];
    sp_auxdata aux;
} sp_revsc;

int sp_revsc_create(sp_revsc **p);
int sp_revsc_destroy(sp_revsc **p);
int sp_revsc_init(sp_data *sp, sp_revsc *p);
int sp_revsc_compute(sp_data *sp, sp_revsc *p, SPFLOAT *in1, SPFLOAT *in2, SPFLOAT *out1, SPFLOAT *out2);
