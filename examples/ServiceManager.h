#ifndef SERVICEMANAGER_H_
#define SERVICEMANAGER_H_

#include <stdint.h>
#include <mindroid/app/Service.h>
#include <mindroid/lang/Closure.h>
#include <mindroid/lang/Object.h>
#include <mindroid/os/Handler.h>
#include <mindroid/os/Message.h>
#include "HelloWorld.h"

class ServiceManager : public mindroid::Service, public mindroid::Handler {
public:
	ServiceManager();
	virtual void onCreate();
	virtual void onDestroy();

private:
	static const char* const LOG_TAG;

	Hello mHello;
	World mWorld;

	NO_COPY_CONSTRUCTOR_AND_ASSIGNMENT_OPERATOR(ServiceManager)
};

#endif /* SERVICEMANAGER_H_ */

