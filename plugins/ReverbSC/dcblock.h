typedef struct {
    SPFLOAT gg;
    SPFLOAT outputs;
    SPFLOAT inputs;
    SPFLOAT gain;
} sp_dcblock;

int sp_dcblock_create(sp_dcblock **p);
int sp_dcblock_destroy(sp_dcblock **p);
int sp_dcblock_init(sp_data *sp, sp_dcblock *p);
int sp_dcblock_compute(sp_data *sp, sp_dcblock *p, SPFLOAT *in, SPFLOAT *out);
