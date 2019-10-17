#ifndef LMMS_CONTROLLERFACTORY_H
#define LMMS_CONTROLLERFACTORY_H


#include <unordered_map>
#include <functional>
#include <memory>

#include "Controller.h"

class LMMS_EXPORT ControllerFactory {
public:
	typedef std::function<Controller*(Model *parent, const QDomElement *element)> FunctionType;


	void addFactory(Controller::ControllerTypes controllerType,
			FunctionType &&factory);

	void removeFactory(Controller::ControllerTypes controllerType);

	Controller *create(Controller::ControllerTypes controllerType,
			Model *parent,
			const QDomElement *element);

	ControllerFactory();

private:
	static Controller* restoreStateHelper(Controller *controller, const QDomElement *element);
	void setupBuiltinControllerFactories();

	std::unordered_map<uint32_t , FunctionType> m_controllerTypeToFactory;
};



#endif //LMMS_CONTROLLERFACTORY_H
