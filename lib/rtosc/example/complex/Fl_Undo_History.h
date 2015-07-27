#include <FL/Fl_Browser.H>
#include <rtosc/rtosc.h>
#include <rtosc/undo-history.h>
#include <cassert>

class Fl_Undo_History:public Fl_Browser
{
    public:
        Fl_Undo_History(int X, int Y, int W, int H, const char *label=NULL)
            :Fl_Browser(X,Y,W,H,label), hist(NULL)
        {
        }

        void init(rtosc::UndoHistory &hist_)
        {
            hist = &hist_;
            totalRefresh();
        }

        void totalRefresh(void)
        {
            assert(hist);
            clear();
            column_char('\t');
            type(FL_MULTI_BROWSER);
            char buf[1024] = {0};

            for(unsigned i=0; i<hist->size(); ++i)
            {
                const char *info = hist->getHistory(i);
                const char *args = rtosc_argument_string(info);
                snprintf(buf, 1023, "%s\t(null)\t(null):%s",
                        rtosc_argument(info, 0).s,
                        args);
                if(!strcmp(args, "sff"))
                    snprintf(buf, 1023, "%s\t%f\t%f",
                        rtosc_argument(info, 0).s,
                        rtosc_argument(info, 1).f,
                        rtosc_argument(info, 2).f);

                add(buf);
            }
            select(hist->getPos(), 1);

        }

        int handle(int e) {
            int val = Fl_Browser::handle(e);
            unsigned choice = value();
            select_only(selection());

            if(choice != hist->getPos()) {
                printf("seek(%d)\n",choice-hist->getPos());
                hist->seekHistory(choice-hist->getPos());
            }
            return val;
        }


        rtosc::UndoHistory *hist;
};
