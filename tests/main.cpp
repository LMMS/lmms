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
		failed += QTest::qExec(suite, argc, argv);
	}
	qDebug() << "<<" << failed << "out of"<<numsuites<<"test suites failed.";
	return failed;
}
