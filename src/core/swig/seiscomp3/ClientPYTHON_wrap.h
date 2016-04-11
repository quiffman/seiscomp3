/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.4a
 * 
 * This file is not intended to be easily readable and contains a number of 
 * coding conventions designed to improve portability and efficiency. Do not make
 * changes to this file unless you know what you are doing--modify the SWIG 
 * interface file instead. 
 * ----------------------------------------------------------------------------- */

#ifndef SWIG_Client_WRAP_H_
#define SWIG_Client_WRAP_H_

#include <map>
#include <string>


class SwigDirector_Application : public Seiscomp::Client::Application, public Swig::Director {

public:
    SwigDirector_Application(PyObject *self, int argc, char **argv);
    virtual char const *className() const;
    virtual Seiscomp::Core::RTTI const &typeInfo() const;
    virtual void serialize(Seiscomp::Core::BaseObject::Archive &arg0);
    virtual ~SwigDirector_Application();
    virtual Seiscomp::Core::BaseObject *clone() const;
    virtual Seiscomp::Core::BaseObject &operator =(Seiscomp::Core::BaseObject const &arg0);
    virtual void handleInterrupt(int arg0) throw();
    virtual void handleInterruptSwigPublic(int arg0) throw() {
      Seiscomp::Client::Application::handleInterrupt(arg0);
    }
    virtual void handleAlarm() throw();
    virtual void handleAlarmSwigPublic() throw() {
      Seiscomp::Core::_private::Alarmable::handleAlarm();
    }
    virtual void exit(int returnCode);
    virtual void printUsage() const;
    virtual void createCommandLineDescription();
    virtual void createCommandLineDescriptionSwigPublic() {
      Seiscomp::Client::Application::createCommandLineDescription();
    }
    virtual bool validateParameters();
    virtual bool validateParametersSwigPublic() {
      return Seiscomp::Client::Application::validateParameters();
    }
    virtual bool init();
    virtual bool initSwigPublic() {
      return Seiscomp::Client::Application::init();
    }
    virtual bool run();
    virtual bool runSwigPublic() {
      return Seiscomp::Client::Application::run();
    }
    virtual void idle();
    virtual void idleSwigPublic() {
      Seiscomp::Client::Application::idle();
    }
    virtual void done();
    virtual void doneSwigPublic() {
      Seiscomp::Client::Application::done();
    }
    virtual bool initConfiguration();
    virtual bool initConfigurationSwigPublic() {
      return Seiscomp::Client::Application::initConfiguration();
    }
    virtual bool initPlugins();
    virtual bool initPluginsSwigPublic() {
      return Seiscomp::Client::Application::initPlugins();
    }
    virtual bool initDatabase();
    virtual bool initDatabaseSwigPublic() {
      return Seiscomp::Client::Application::initDatabase();
    }
    virtual bool initSubscriptions();
    virtual bool initSubscriptionsSwigPublic() {
      return Seiscomp::Client::Application::initSubscriptions();
    }
    virtual void printVersion();
    virtual void printVersionSwigPublic() {
      Seiscomp::Client::Application::printVersion();
    }
    virtual bool handleInitializationError(Seiscomp::Client::Application::Stage stage);
    virtual bool handleInitializationErrorSwigPublic(Seiscomp::Client::Application::Stage stage) {
      return Seiscomp::Client::Application::handleInitializationError(stage);
    }
    virtual bool dispatch(Seiscomp::Core::BaseObject *arg0);
    virtual bool dispatchSwigPublic(Seiscomp::Core::BaseObject *arg0) {
      return Seiscomp::Client::Application::dispatch(arg0);
    }
    virtual bool dispatchNotification(int type, Seiscomp::Core::BaseObject *arg0);
    virtual bool dispatchNotificationSwigPublic(int type, Seiscomp::Core::BaseObject *arg0) {
      return Seiscomp::Client::Application::dispatchNotification(type,arg0);
    }
    virtual void showMessage(char const *arg0);
    virtual void showMessageSwigPublic(char const *arg0) {
      Seiscomp::Client::Application::showMessage(arg0);
    }
    virtual void showWarning(char const *arg0);
    virtual void showWarningSwigPublic(char const *arg0) {
      Seiscomp::Client::Application::showWarning(arg0);
    }
    virtual void handleTimeout();
    virtual void handleTimeoutSwigPublic() {
      Seiscomp::Client::Application::handleTimeout();
    }
    virtual void handleAutoShutdown();
    virtual void handleAutoShutdownSwigPublic() {
      Seiscomp::Client::Application::handleAutoShutdown();
    }
    virtual void handleMonitorLog(Seiscomp::Core::Time const &timestamp);
    virtual void handleMonitorLogSwigPublic(Seiscomp::Core::Time const &timestamp) {
      Seiscomp::Client::Application::handleMonitorLog(timestamp);
    }
    virtual void handleSync(char const *ID);
    virtual void handleSyncSwigPublic(char const *ID) {
      Seiscomp::Client::Application::handleSync(ID);
    }
    virtual void handleDisconnect();
    virtual void handleDisconnectSwigPublic() {
      Seiscomp::Client::Application::handleDisconnect();
    }
    virtual void handleReconnect();
    virtual void handleReconnectSwigPublic() {
      Seiscomp::Client::Application::handleReconnect();
    }
    virtual void handleMessage(Seiscomp::Core::Message *msg);
    virtual void handleMessageSwigPublic(Seiscomp::Core::Message *msg) {
      Seiscomp::Client::Application::handleMessage(msg);
    }
    virtual void handleNetworkMessage(Seiscomp::Communication::NetworkMessage const *msg);
    virtual void handleNetworkMessageSwigPublic(Seiscomp::Communication::NetworkMessage const *msg) {
      Seiscomp::Client::Application::handleNetworkMessage(msg);
    }
    virtual void addObject(std::string const &parentID, Seiscomp::DataModel::Object *arg0);
    virtual void addObjectSwigPublic(std::string const &parentID, Seiscomp::DataModel::Object *arg0) {
      Seiscomp::Client::Application::addObject(parentID,arg0);
    }
    virtual void removeObject(std::string const &parentID, Seiscomp::DataModel::Object *arg0);
    virtual void removeObjectSwigPublic(std::string const &parentID, Seiscomp::DataModel::Object *arg0) {
      Seiscomp::Client::Application::removeObject(parentID,arg0);
    }
    virtual void updateObject(std::string const &parentID, Seiscomp::DataModel::Object *arg0);
    virtual void updateObjectSwigPublic(std::string const &parentID, Seiscomp::DataModel::Object *arg0) {
      Seiscomp::Client::Application::updateObject(parentID,arg0);
    }


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* swig_protected_method_name) const {
      std::map<std::string, bool>::const_iterator iv = swig_inner.find(swig_protected_method_name);
      return (iv != swig_inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* swig_protected_method_name, bool val) const
    { swig_inner[swig_protected_method_name] = val;}

private:
    mutable std::map<std::string, bool> swig_inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::SwigVar_PyObject name = SWIG_Python_str_FromChar(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (!method) {
          std::string msg = "Method in class Application doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::SwigVar_PyObject vtable[34];
#endif

};


class SwigDirector_StreamApplication : public Seiscomp::Client::StreamApplication, public Swig::Director {

public:
    SwigDirector_StreamApplication(PyObject *self, int argc, char **argv);
    virtual char const *className() const;
    virtual Seiscomp::Core::RTTI const &typeInfo() const;
    virtual void serialize(Seiscomp::Core::BaseObject::Archive &arg0);
    virtual ~SwigDirector_StreamApplication();
    virtual Seiscomp::Core::BaseObject *clone() const;
    virtual Seiscomp::Core::BaseObject &operator =(Seiscomp::Core::BaseObject const &arg0);
    virtual void handleInterrupt(int arg0) throw();
    virtual void handleInterruptSwigPublic(int arg0) throw() {
      Seiscomp::Client::Application::handleInterrupt(arg0);
    }
    virtual void handleAlarm() throw();
    virtual void handleAlarmSwigPublic() throw() {
      Seiscomp::Core::_private::Alarmable::handleAlarm();
    }
    virtual void exit(int returnCode);
    virtual void exitSwigPublic(int returnCode) {
      Seiscomp::Client::StreamApplication::exit(returnCode);
    }
    virtual void printUsage() const;
    virtual void createCommandLineDescription();
    virtual void createCommandLineDescriptionSwigPublic() {
      Seiscomp::Client::Application::createCommandLineDescription();
    }
    virtual bool validateParameters();
    virtual bool validateParametersSwigPublic() {
      return Seiscomp::Client::Application::validateParameters();
    }
    virtual bool init();
    virtual bool initSwigPublic() {
      return Seiscomp::Client::StreamApplication::init();
    }
    virtual bool run();
    virtual bool runSwigPublic() {
      return Seiscomp::Client::StreamApplication::run();
    }
    virtual void idle();
    virtual void idleSwigPublic() {
      Seiscomp::Client::Application::idle();
    }
    virtual void done();
    virtual void doneSwigPublic() {
      Seiscomp::Client::StreamApplication::done();
    }
    virtual bool initConfiguration();
    virtual bool initConfigurationSwigPublic() {
      return Seiscomp::Client::Application::initConfiguration();
    }
    virtual bool initPlugins();
    virtual bool initPluginsSwigPublic() {
      return Seiscomp::Client::Application::initPlugins();
    }
    virtual bool initDatabase();
    virtual bool initDatabaseSwigPublic() {
      return Seiscomp::Client::Application::initDatabase();
    }
    virtual bool initSubscriptions();
    virtual bool initSubscriptionsSwigPublic() {
      return Seiscomp::Client::Application::initSubscriptions();
    }
    virtual void printVersion();
    virtual void printVersionSwigPublic() {
      Seiscomp::Client::Application::printVersion();
    }
    virtual bool handleInitializationError(Seiscomp::Client::Application::Stage stage);
    virtual bool handleInitializationErrorSwigPublic(Seiscomp::Client::Application::Stage stage) {
      return Seiscomp::Client::Application::handleInitializationError(stage);
    }
    virtual bool dispatch(Seiscomp::Core::BaseObject *obj);
    virtual bool dispatchSwigPublic(Seiscomp::Core::BaseObject *obj) {
      return Seiscomp::Client::StreamApplication::dispatch(obj);
    }
    virtual bool dispatchNotification(int type, Seiscomp::Core::BaseObject *arg0);
    virtual bool dispatchNotificationSwigPublic(int type, Seiscomp::Core::BaseObject *arg0) {
      return Seiscomp::Client::Application::dispatchNotification(type,arg0);
    }
    virtual void showMessage(char const *arg0);
    virtual void showMessageSwigPublic(char const *arg0) {
      Seiscomp::Client::Application::showMessage(arg0);
    }
    virtual void showWarning(char const *arg0);
    virtual void showWarningSwigPublic(char const *arg0) {
      Seiscomp::Client::Application::showWarning(arg0);
    }
    virtual void handleTimeout();
    virtual void handleTimeoutSwigPublic() {
      Seiscomp::Client::Application::handleTimeout();
    }
    virtual void handleAutoShutdown();
    virtual void handleAutoShutdownSwigPublic() {
      Seiscomp::Client::Application::handleAutoShutdown();
    }
    virtual void handleMonitorLog(Seiscomp::Core::Time const &timestamp);
    virtual void handleMonitorLogSwigPublic(Seiscomp::Core::Time const &timestamp) {
      Seiscomp::Client::StreamApplication::handleMonitorLog(timestamp);
    }
    virtual void handleSync(char const *ID);
    virtual void handleSyncSwigPublic(char const *ID) {
      Seiscomp::Client::Application::handleSync(ID);
    }
    virtual void handleDisconnect();
    virtual void handleDisconnectSwigPublic() {
      Seiscomp::Client::Application::handleDisconnect();
    }
    virtual void handleReconnect();
    virtual void handleReconnectSwigPublic() {
      Seiscomp::Client::Application::handleReconnect();
    }
    virtual void handleMessage(Seiscomp::Core::Message *msg);
    virtual void handleMessageSwigPublic(Seiscomp::Core::Message *msg) {
      Seiscomp::Client::Application::handleMessage(msg);
    }
    virtual void handleNetworkMessage(Seiscomp::Communication::NetworkMessage const *msg);
    virtual void handleNetworkMessageSwigPublic(Seiscomp::Communication::NetworkMessage const *msg) {
      Seiscomp::Client::Application::handleNetworkMessage(msg);
    }
    virtual void addObject(std::string const &parentID, Seiscomp::DataModel::Object *arg0);
    virtual void addObjectSwigPublic(std::string const &parentID, Seiscomp::DataModel::Object *arg0) {
      Seiscomp::Client::Application::addObject(parentID,arg0);
    }
    virtual void removeObject(std::string const &parentID, Seiscomp::DataModel::Object *arg0);
    virtual void removeObjectSwigPublic(std::string const &parentID, Seiscomp::DataModel::Object *arg0) {
      Seiscomp::Client::Application::removeObject(parentID,arg0);
    }
    virtual void updateObject(std::string const &parentID, Seiscomp::DataModel::Object *arg0);
    virtual void updateObjectSwigPublic(std::string const &parentID, Seiscomp::DataModel::Object *arg0) {
      Seiscomp::Client::Application::updateObject(parentID,arg0);
    }
    virtual void acquisitionFinished();
    virtual void acquisitionFinishedSwigPublic() {
      Seiscomp::Client::StreamApplication::acquisitionFinished();
    }
    virtual bool storeRecord(Seiscomp::Record *rec);
    virtual bool storeRecordSwigPublic(Seiscomp::Record *rec) {
      return Seiscomp::Client::StreamApplication::storeRecord(rec);
    }
    virtual void handleRecord(Seiscomp::Record *rec);


/* Internal Director utilities */
public:
    bool swig_get_inner(const char* swig_protected_method_name) const {
      std::map<std::string, bool>::const_iterator iv = swig_inner.find(swig_protected_method_name);
      return (iv != swig_inner.end() ? iv->second : false);
    }

    void swig_set_inner(const char* swig_protected_method_name, bool val) const
    { swig_inner[swig_protected_method_name] = val;}

private:
    mutable std::map<std::string, bool> swig_inner;


#if defined(SWIG_PYTHON_DIRECTOR_VTABLE)
/* VTable implementation */
    PyObject *swig_get_method(size_t method_index, const char *method_name) const {
      PyObject *method = vtable[method_index];
      if (!method) {
        swig::SwigVar_PyObject name = SWIG_Python_str_FromChar(method_name);
        method = PyObject_GetAttr(swig_get_self(), name);
        if (!method) {
          std::string msg = "Method in class StreamApplication doesn't exist, undefined ";
          msg += method_name;
          Swig::DirectorMethodException::raise(msg.c_str());
        }
        vtable[method_index] = method;
      };
      return method;
    }
private:
    mutable swig::SwigVar_PyObject vtable[35];
#endif

};


#endif
