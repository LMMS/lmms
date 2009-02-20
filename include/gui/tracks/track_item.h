

#ifndef TRACK_ITEM_H_
#define TRACK_ITEM_H_

#include <QObject>
#include <QRectF>
#include <QMap>

class TrackContainerScene;
class track;
class trackContentObject;
class TrackContentObjectItem;

class TrackItem : public QObject
{
	Q_OBJECT;
public:
	TrackItem( TrackContainerScene * _scene, track * _track );
	virtual ~TrackItem( );

	float height();
	void setHeight( float _height );

	float y();
	void setY( float _x );

private slots:
	void addTCO( trackContentObject * _tco );
	void removeTCO( trackContentObject * _tco );

private:
	QRectF m_rect;

	TrackContainerScene * m_scene;
	track * m_track;
	QMap<trackContentObject*, TrackContentObjectItem *> m_tcoItems;

};


#endif
