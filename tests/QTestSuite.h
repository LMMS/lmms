#ifndef QTESTSUITE_H
#define QTESTSUITE_H

#include <QTest>
#include <QObject>
#include <QList>

class QTestSuite : public QObject
{
	Q_OBJECT
public:
	explicit QTestSuite(QObject *parent = 0);
	~QTestSuite() override;

	static QList<QTestSuite*> suites();

private:
	static QList<QTestSuite*> m_suites;
};

#endif // QTESTSUITE_H
