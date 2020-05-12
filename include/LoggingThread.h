#ifndef LOGGINGTHREAD_H
#define LOGGINGTHREAD_H

#include <QThread>

class LoggingThread: public QThread
{
public:
	static LoggingThread& inst();
	~LoggingThread();
	void run() override;
	void setFlushInterval(unsigned int interval)
	{
		m_flushInterval = interval;
	}

	unsigned int flushInterval()
	{
		return m_flushInterval;
	}

private:
	LoggingThread();
	unsigned int m_flushInterval;
	bool m_active;
};

#endif // LOGGINGTHREAD_H
