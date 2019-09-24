#include "InstrumentView.h"

class DiginstrumentView : public InstrumentViewFixedSize /*TMP: fixed size */
{
    Q_OBJECT
  public:
    DiginstrumentView( Instrument * _instrument, QWidget * _parent );
	  virtual ~DiginstrumentView();

    /*TODO*/
  private:
	  virtual void modelChanged( void );
    /*TODO*/
};
