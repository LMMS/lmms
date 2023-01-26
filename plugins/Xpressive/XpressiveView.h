#ifndef XPRESSIVEVIEW_H
#define XPRESSIVEVIEW_H

#include <QTextEdit>
#include <QSyntaxHighlighter>
#include <QRegularExpression>

#include "InstrumentView.h"
#include "Knob.h"
#include "LedCheckbox.h"
#include "PixmapButton.h"
#include "Graph.h"

#ifndef QT_NO_SYNTAXHIGHLIGHTER

class Highlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	Highlighter(QTextDocument *parent = 0);

protected:
	void highlightBlock(const QString &text) override;

private:
	struct HighlightingRule
	{
		QRegularExpression pattern;
		QTextCharFormat format;
	};
	QVector<HighlightingRule> highlightingRules;


	QTextCharFormat keywordClass1Format;
	QTextCharFormat keywordClass2Format;
	QTextCharFormat keywordClass3Format;
	QTextCharFormat keywordClass4Format;
	QTextCharFormat numericFormat;
	QTextCharFormat badFormat;


};
#endif

class XpressiveView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	XpressiveView( Instrument* _instrument,
					QWidget* _parent );

	virtual ~XpressiveView();
protected:


protected slots:
	void updateLayout();

	void sinWaveClicked();
	void triangleWaveClicked();
	void sqrWaveClicked();
	void sawWaveClicked();
	void noiseWaveClicked();
	void moogSawWaveClicked();
	void expWaveClicked();
	void usrWaveClicked();
	void helpClicked();
	void expressionChanged( );
	void smoothChanged( );
	void graphDrawn( );

private:
	virtual void modelChanged();

	Knob *m_generalPurposeKnob[3];
	Knob *m_panningKnob[2];
	Knob *m_relKnob;
	Knob *m_smoothKnob;
	QTextEdit * m_expressionEditor;

	automatableButtonGroup *m_selectedGraphGroup;
	PixmapButton *m_w1Btn;
	PixmapButton *m_w2Btn;
	PixmapButton *m_w3Btn;
	PixmapButton *m_o1Btn;
	PixmapButton *m_o2Btn;
	PixmapButton *m_sinWaveBtn;
	PixmapButton *m_triangleWaveBtn;
	PixmapButton *m_sqrWaveBtn;
	PixmapButton *m_sawWaveBtn;
	PixmapButton *m_whiteNoiseWaveBtn;
	PixmapButton *m_usrWaveBtn;
	PixmapButton *m_moogWaveBtn;
	PixmapButton *m_expWaveBtn;
	QWidget *m_smoothOverlay;
#ifndef QT_NO_SYNTAXHIGHLIGHTER
	Highlighter *m_highlighter;
#endif

	static QPixmap *s_artwork;

	Graph *m_graph;
	graphModel *m_raw_graph;
	LedCheckBox *m_expressionValidToggle;
	LedCheckBox *m_waveInterpolate;
	bool m_output_expr;
	bool m_wave_expr;
} ;

class XpressiveHelpView: public QTextEdit
{
	Q_OBJECT
public:
	static XpressiveHelpView* getInstance()
	{
		static XpressiveHelpView instance;
		return &instance;
	}
	static void finalize()
	{
	}

private:
	XpressiveHelpView();
	static QString s_helpText;
};


#endif // XPRESSIVEVIEW_H
