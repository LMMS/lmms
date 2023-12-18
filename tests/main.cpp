#include "QTestSuite.h"

#include <QTest>

#include <QDebug>

#include "Engine.h"

int main(int argc, char* argv[])
{
	new QCoreApplication(argc, argv);
	lmms::Engine::init(true);

	int numsuites = QTestSuite::suites().size();
	qDebug() << ">> Will run" << numsuites << "test suites";
	int failed = 0;
	for (QTestSuite*& suite : QTestSuite::suites())
	{
		if (QTest::qExec(suite, argc, argv) != 0) { ++failed; }
	}
	qDebug() << "<<" << failed << "out of"<<numsuites<<"test suites failed.";
	return failed;
}
