#include "QTestSuite.h"

#include <QtTest/QTest>

#include <QDebug>

int main(int argc, char* argv[])
{
	qDebug() << ">> Will run" << QTestSuite::suites().size()  << "test suites";
	for (QTestSuite*& suite : QTestSuite::suites())
	{
		QTest::qExec(suite, argc, argv);
	}
}
