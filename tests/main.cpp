#include "QTestSuite.h"

#include <QtTest/QTest>

#include <QDebug>

#include "Engine.h"

void printAvailableSuits()
{
	QStringList suitNames;
	for (QTestSuite* suite : QTestSuite::suites()) {
		suitNames.push_back(suite->metaObject()->className());
	}
	qDebug().noquote() << "Available test suits are: " << suitNames.join(", ");
}

int runSuit(const QString& name, const QStringList& args)
{
	for (QTestSuite* suite : QTestSuite::suites()) {
		if (suite->metaObject()->className() == name) {
			return QTest::qExec(suite, args);
		}
	}
	throw std::invalid_argument("No such test suit " + name.toStdString());
}

int main(int argc, char* argv[])
{
	auto app = new QCoreApplication(argc, argv);
	Engine::init(true);

	int rc = 0;

	if (app->arguments().size() > 1) {
		try {
			runSuit(app->arguments()[1], app->arguments().mid(1));
		} catch (const std::invalid_argument& e) {
			qDebug().noquote() << e.what();
			printAvailableSuits();
			rc = -1;
		}
	} else {
		int numsuites = QTestSuite::suites().size();
		qDebug() << ">> Will run" << numsuites << "test suites";
		for (QTestSuite* suite : QTestSuite::suites()) {
			rc += QTest::qExec(suite, argc, argv);
		}
		qDebug() << "<<" << rc << "out of"<<numsuites<<"test suites failed.";
	}

	Engine::destroy();
	return rc;
}
