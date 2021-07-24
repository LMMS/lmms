#ifndef QTESTSUITE_H
#define QTESTSUITE_H

#include <QList>
#include <QObject>
#include <QtTest/QTest>

class QTestSuite : public QObject {
	Q_OBJECT
public:
	explicit QTestSuite(QObject* parent = 0);
	~QTestSuite();

	static QList<QTestSuite*> suites();

private:
	static QList<QTestSuite*> m_suites;
};

#endif // QTESTSUITE_H
