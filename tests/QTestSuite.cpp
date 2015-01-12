#include "QTestSuite.h"

QList<QTestSuite*> QTestSuite::m_suites;

QTestSuite::QTestSuite(QObject *parent) : QObject(parent)
{
	m_suites << this;
}

QTestSuite::~QTestSuite()
{
	m_suites.removeAll(this);
}

QList<QTestSuite*> QTestSuite::suites()
{
	return m_suites;
}

