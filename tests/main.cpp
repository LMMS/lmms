#include "QTestSuite.h"

#include <QtTest/QTest>

#include <QDebug>

#include "Engine.h"
#include "Oscillator.h"

int main(int argc, char* argv[])
{
	new QCoreApplication(argc, argv);
	Oscillator::waveTableInit();


	Engine::init(true);

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
