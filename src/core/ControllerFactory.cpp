#include "ControllerFactory.h"

#include "LfoController.h"
#include "MidiController.h"

void
ControllerFactory::addFactory(Controller::ControllerTypes controllerType,
		ControllerFactory::FunctionType &&factory) {
	auto isInsertedAndIterator = m_controllerTypeToFactory.emplace(controllerType, std::move(factory));

	if (! isInsertedAndIterator.second) {
		// We already have a factory with this ControllerType.
		// Just ignore it.
	}
}

void ControllerFactory::removeFactory(Controller::ControllerTypes controllerType) {
	m_controllerTypeToFactory.erase(controllerType);
}

Controller *ControllerFactory::create(Controller::ControllerTypes controllerType,
		Model *parent,
		const QDomElement *element) {
	auto iterator = m_controllerTypeToFactory.find(controllerType);

	if(iterator == m_controllerTypeToFactory.end()) {
		return nullptr;
	} else {
		return iterator->second(parent, element);
	}
}

ControllerFactory::ControllerFactory()
{
	setupBuiltinControllerFactories();
}

void ControllerFactory::setupBuiltinControllerFactories() {
	addFactory(Controller::DummyController, [] (Model *parent, const QDomElement *element)
	{
		return restoreStateHelper(new Controller(Controller::DummyController, parent, QString()),
				element);
	});

	addFactory(Controller::LfoController, [] (Model *parent, const QDomElement *element)
	{
		return restoreStateHelper(new LfoController(parent), element);
	});

	addFactory(Controller::MidiController, [] (Model *parent, const QDomElement *element)
	{
		return restoreStateHelper(new MidiController(parent), element);
	});
}

Controller *ControllerFactory::restoreStateHelper(Controller *controller, const QDomElement *element) {
	if (element) {
		controller->restoreState(*element);
	}

	return controller;
}
