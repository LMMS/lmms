#ifndef UPDATINGVALUE_HPP
#define UPDATINGVALUE_HPP

#include <QObject>

#include "MemoryUtils.h"
#include "lmms_export.h"

namespace internal {
	class LMMS_EXPORT SignalArgument {
	protected:
		virtual ~SignalArgument() {}
	};
}

Q_DECLARE_METATYPE(std::shared_ptr<internal::SignalArgument>);


namespace internal {
	class LMMS_EXPORT UpdatingValueNotifier_Untyped : public QObject {
	Q_OBJECT

	signals:

		/**
		 * @brief Notify the listeners (UpdatingValue instances) that we've
		 *		  changed the value.
		 */
		void rawOnValueUpdated(std::shared_ptr<internal::SignalArgument> value);
	};

	class LMMS_EXPORT UpdatingValue_Untyped : public QObject {
	Q_OBJECT

	protected:
		UpdatingValue_Untyped(UpdatingValueNotifier_Untyped &notifier, QObject *parent)
				: QObject{parent} {
			// Run in the parents thread.
			this->moveToThread(parent->thread());

			qRegisterMetaType<std::shared_ptr<internal::SignalArgument>>();

			// Notify us about changes.
			auto connection = QObject::connect(
					&notifier,
					&UpdatingValueNotifier_Untyped::rawOnValueUpdated,
					this,
					&UpdatingValue_Untyped::onValueUpdated);
		}

		virtual void onValueUpdated(std::shared_ptr<internal::SignalArgument> value) = 0;
	};

	template<class T>
	class LMMS_EXPORT UpdatingValueImpl : public internal::UpdatingValue_Untyped {
	public:
		/**
		 * @brief Sender of updates for this UpdatingValue.
		 */
		class Notifier : public internal::UpdatingValueNotifier_Untyped {
			friend class UpdatingValueImpl;

			struct TypedSignalArgument : public SignalArgument {
				TypedSignalArgument(const T &value)
						: value{value} {
				}

				const T value;
			};

		public:
			void onValueUpdated(const T &newValue) {
				auto newArgument = std::make_shared<TypedSignalArgument>(newValue);
				emit rawOnValueUpdated(newArgument);
			}
		};

		using TypedSignalArugment=typename Notifier::TypedSignalArgument;


		UpdatingValueImpl(Notifier &notifier, const T &initialValue, QObject *parent)
				: UpdatingValue_Untyped{notifier, parent},
				  m_ourCopy{std::make_shared<TypedSignalArugment>(std::move(initialValue))} {
		}


		const T *operator->() const {
			return &(get());
		}

		const T &operator*() const {
			return get();
		}

		const T &get() const {
			return m_ourCopy->value;
		}

	private:
		std::shared_ptr<TypedSignalArugment> m_ourCopy;

	private:
		void onValueUpdated(std::shared_ptr<internal::SignalArgument> value) override final {
			m_ourCopy = std::static_pointer_cast<TypedSignalArugment>(std::move(value));
		}
	};
} // namespace internal

/**
  UpdatingValue is a lockless method of getting info
  from core components.

  Instead of locking for read and reading the value,
  we'll get notified when we have any change in the value.
  */
template<class T>
class LMMS_EXPORT UpdatingValue {
	typedef internal::UpdatingValueImpl<T> Impl;

public:
	typedef typename Impl::Notifier Notifier;

	UpdatingValue(Notifier &notifier, const T &initialValue, QObject *parent)
			: m_updatingValueImpl{makeUniqueQObject<Impl>(notifier, initialValue, parent)} {
	}

	const T *operator->() const {
		return &(this->operator*());
	}

	const T &operator*() const {
		return m_updatingValueImpl->get();
	}

private:
	UniqueQObjectPointer<internal::UpdatingValueImpl<T>> m_updatingValueImpl;
};


#endif // UPDATINGVALUE_HPP
