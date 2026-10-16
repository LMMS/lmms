#pragma once
#include <QApplication>
#include <QtTest>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
inline void configureDpiTestAttributes()
{
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
}
#else
inline void configureDpiTestAttributes() {}
#endif

#define LMMS_DPI_TEST_MAIN(TestClass) \
int main(int argc, char** argv) \
{ \
	configureDpiTestAttributes(); \
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough); \
	QApplication app(argc, argv); \
	TestClass test; \
	return QTest::qExec(&test, argc, argv); \
}
