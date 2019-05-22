#include "QTestSuite.h"

#include <QtTest/QSignalSpy>

#include "UpdatingValue.h"
#include "Threading.h"
typedef UpdatingValue<int> Value;

class _QThreadUpdatingValueTest : public QObject
{
	Q_OBJECT

public:
	_QThreadUpdatingValueTest(Value::Notifier *notifier)
		: notifier{notifier}
	{
	}

public slots:
	void emitNotifier() {
		notifier->onValueUpdated(1);
	}

private:
	Value::Notifier *notifier;
};

class UpdatingValueTest : QTestSuite
{
	Q_OBJECT



	std::unique_ptr<Value::Notifier> createNotifierOnExternalThread() {
		auto future = runAsync([] {
			return new Value::Notifier();
		});

		future.waitForFinished();

		std::unique_ptr<Value::Notifier> ptr;
		ptr.reset(future.result());

		return ptr;
	}
private slots:

	void SameThreadUpdatingValueTest() {
		Value::Notifier notifier;
		Value value{notifier, 0, this};
		Value value2{notifier, 0, this};

		QCOMPARE(*value, 0);
		QCOMPARE(*value2, 0);

		notifier.onValueUpdated(1);
		QCOMPARE(*value, 1);
		QCOMPARE(*value2, 1);
	}

	void ExternalThreadUpdatingValueTest() {
		auto notifier = createNotifierOnExternalThread();
		Value value{*notifier, 0, this};
		Value value2{*notifier, 0, this};

		runAsync([&notifier] {
			notifier->onValueUpdated(1);
		}).waitForFinished();

		QCOMPARE(*value, 1);
		QCOMPARE(*value2, 1);
	}
	void QThreadUpdatingValueTest() {
		// First of all, create the notifier on another thread.
		auto notifier = createNotifierOnExternalThread();

		Value value{*notifier, 0, this};
		Value value2{*notifier, 0, this};

		QThread *thread = new QThread;

		QObject *emittingThreadObject = new _QThreadUpdatingValueTest(notifier.get());
		emittingThreadObject->moveToThread(thread);

		QSignalSpy valueChangedSpy{notifier.get(), &internal::UpdatingValueNotifier_Untyped::rawOnValueUpdated};

		thread->start();

		// Invoke emitNotifier on the objects thread.
		QMetaObject::invokeMethod(emittingThreadObject,
								  "emitNotifier");


		// Wait for the signal to come.
		QVERIFY(valueChangedSpy.wait());

		thread->quit();
		thread->wait();

		QCOMPARE(*value, 1);
		QCOMPARE(*value2, 1);

		delete thread;
		delete emittingThreadObject;

	}
} updatingValueTest;

#include "UpdatingValueTest.moc"
