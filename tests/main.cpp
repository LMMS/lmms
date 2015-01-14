#include "QTestSuite.h"

#include <QtTest/QTest>

#include <QDebug>

int main(int argc, char* argv[])
{
	for (QTestSuite*& suite : QTestSuite::suites())
	{
		QTest::qExec(suite, argc, argv);
	}
}
