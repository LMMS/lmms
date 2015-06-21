#include "VuMeter.h"
#define MIN_DB (-48)

class VuPartMeter: public VuMeter
{
    public:
        VuPartMeter(int x,int y, int w, int h, const char *label=0)
            :VuMeter(x,y,w,h,label), db(0.0f)
        {}

        void draw(void)
        {
            const int X = x(), Y = y(), W = w(), H = h();

            //XXX perhaps re-enable this later on
            //if (!active_r()){
            //    int fakedb=master->fakepeakpart[npart];
            //    fl_rectf(X,Y,W,H,140,140,140);
            //    if (fakedb>0){
            //        fakedb=(int)(fakedb/255.0*H)+4;
            //        fl_rectf(X+2,Y+H-fakedb,W-4,fakedb,0,0,0);
            //    }
            //    return;
            //}

            //draw the vu lines
            const int idb = db*(H-2); 

            fl_rectf(X,Y+H-idb,W,idb,0,200,255);
            fl_rectf(X,Y,W,H-idb,0,0,0);

            //draw the scales
            const float tmp=H*1.0/MIN_DB;
            for (int i = 1; i < 1 - MIN_DB; i++) {
                const int ty = H+(int) (tmp*i);
                if(i%5  == 0) fl_rectf(X, Y+H-ty, W, 1,0, 160, 200);
                if(i%10 == 0) fl_rectf(X, Y+H-ty, W, 1,0, 230, 240);
            }
        }
        
        void update(float x)
        {
            const float _db = limit((MIN_DB-rap2dB(x))/MIN_DB);

            if(db != _db) {
                db = _db;
                damage(FL_DAMAGE_USER1);
            }
        }

    private:
        float db;
};
