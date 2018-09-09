#include <cstdio>
#include <cmath>

#include "Lv2Plugin.h"

#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/port-props/port-props.h"

#include <qglobal.h>

#if QT_VERSION >= 0x050000
#    include <QAction>
#    include <QApplication>
#    include <QDial>
#    include <QGroupBox>
#    include <QLabel>
#    include <QLayout>
#    include <QMainWindow>
#    include <QMenu>
#    include <QMenuBar>
#    include <QScrollArea>
#    include <QStyle>
#    include <QTimer>
#    include <QWidget>
#    include <QWindow>
#else
#    include <QtGui>
#endif

class FlowLayout : public QLayout
{
public:
	FlowLayout(QWidget* parent,
	           int      margin   = -1,
	           int      hSpacing = -1,
	           int      vSpacing = -1);

	FlowLayout(int margin = -1, int hSpacing = -1, int vSpacing = -1);

	~FlowLayout();

	void             addItem(QLayoutItem* item);
	int              horizontalSpacing() const;
	int              verticalSpacing() const;
	Qt::Orientations expandingDirections() const;
	bool             hasHeightForWidth() const;
	int              heightForWidth(int) const;
	int              count() const;
	QLayoutItem*     itemAt(int index) const;
	QSize            minimumSize() const;
	void             setGeometry(const QRect &rect);
	QSize            sizeHint() const;
	QLayoutItem*     takeAt(int index);

private:
	int doLayout(const QRect &rect, bool testOnly) const;
	int smartSpacing(QStyle::PixelMetric pm) const;

	QList<QLayoutItem*> itemList;
	int m_hSpace;
	int m_vSpace;
};

class PresetAction : public QAction
{
	Q_OBJECT

public:
	PresetAction(QObject* parent, Lv2Plugin* jalv, LilvNode* preset);

public slots:
	void presetChosen();

private:
	Lv2Plugin*     _jalv;
	LilvNode* _preset;
};

typedef struct {
	Lv2Plugin*        jalv;
	Port* port;
} PortContainer;

class Control : public QGroupBox
{
	Q_OBJECT

public:
	Control(PortContainer portContainer, QWidget* parent = 0);

	void setValue(float value);

	QDial* dial;

public slots:
	void dialChanged(int value);

private:
	void    setRange(float min, float max);
	QString getValueLabel(float value);
	float   getValue();

	const LilvPlugin* plugin;
	Port*      port;

	QLabel* label;
	QString name;
	int     steps;
	float   max;
	float   min;
	bool    isInteger;
	bool    isEnum;
	bool    isLogarithmic;

	std::vector<float>           scalePoints;
	std::map<float, const char*> scaleMap;
};
