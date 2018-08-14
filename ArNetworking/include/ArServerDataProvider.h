
#ifndef NLDATAPROVIDER_H
#define NLDATAPROVIDER_H

#include "Aria.h"
#include "ArServerBase.h"
#include "ArPacketUtil.h"
#include <assert.h>

/** Tool to easily obtain and provide changing data values from an acessor method of a
 * class.   The data item is added to the server, and
 * when clients make requests, this object will call a lock method on a target
 * object (if provided), then obtain a value to reply with by
 * calling the given accessor method on the target object, then call the
 * unlock method (if provided).  You can supply these methods either as function
 * pointers (within the class of the target object), or ArFunctor objects.
 * 
 * 
 * Client programs can use ArClientDataReceiver to request and receive updates
 * on the value.
 *
 * @note This object must remain in scope and not be destroyed to remain
 * active. If this object is deleted (explicitly or if execution leaves 
 * scope in which the object was created)
 * then the data item is removed from the server and clients
 * will no longer receive updates.
 *
 * Replies with data are sent via TCP. @todo change to UDP.
 *
 * @sa ArClientDataReceiver
 *
 * @todo provide data logger and/or string info group to add data to as well
 * @todo option to provide mutex to lock/unlock
 * @todo alternate with pointer to variable
 * @todo option for udp
 *
 * @since 2.9.3
 * @unstable
 */
template <class C, typename T>
class ArServerDataProvider
{
  ArServerBase *myServer;
  const char *myReqName;
  ArFunctor2C<ArServerDataProvider, ArServerClient*, ArNetPacket*> *myPackFunctor;
  ArRetFunctor<T> *myAccessFunctor;
  ArFunctor *myLockFunctor;
  ArFunctor *myUnlockFunctor;
  bool myDeleteFunctors;

  void packAndSend(ArServerClient *client, ArNetPacket *req)
  {
    // TODO flag to select TCP or UDP
    ArNetPacket pkt;
    if(myLockFunctor) myLockFunctor->invoke();
    const T val = myAccessFunctor->invokeR();
    if(myUnlockFunctor) myUnlockFunctor->invoke();
    ArPacketUtil::addField<T>(pkt, val);
    client->sendPacketTcp(&pkt);
  }

  bool init(ArServerBase *server, const char *name, const char *description, ArRetFunctor<T> *accessFunctor, ArFunctor *lockFunctor, ArFunctor *unlockFunctor)
  {
    myServer = server;
    myReqName = name;
    const char *type_name = typeid(T).name();
    myAccessFunctor = accessFunctor;
    myLockFunctor = lockFunctor;
    myUnlockFunctor = unlockFunctor;
    myPackFunctor = new ArFunctor2C<ArServerDataProvider, ArServerClient*, ArNetPacket*>(this, &ArServerDataProvider::packAndSend);
    bool r = server->addDataAdvanced(
      name, 
      description, 
      myPackFunctor,
      "none", // argDesc, todo
      type_name ? type_name : "unknown", // return description
      "", // commandGroup TODO
      "RETURN_SINGLE", // dataFlags 
      0,    // commandNumber
      NULL,    // requestChangedFunctor TODO
      NULL    // requestOnceFunctor TODO
    );
    if(r)
      ArLog::log(ArLog::Verbose, "\"%s\" ArPublisher: Created", name);
    else
      ArLog::log(ArLog::Terse, "\"%s\" ArPublisher: Error adding data request to base server!", name);
    return r;
  }

public:
  /** If this constructor is used, then access data by invoking the @a
  * accessFunctor ArRetFunctor<T> object. If @a lockFunctor and
  * @a unlockFunctor are provided (not NULL), then these are invoked before and
  * after invoking @a accessFunctor.
  */
  ArServerDataProvider(ArServerBase *server, const char *name, const char *description, ArRetFunctor<T>* accessFunctor, ArFunctor* lockFunctor = NULL, ArFunctor *unlockFunctor = NULL) :
    myDeleteFunctors(false)
  {
    assert((lockFunctor && unlockFunctor) || (!lockFunctor && !unlockFunctor));
    init(server, name, description, accessFunctor, lockFunctor, unlockFunctor);
  }

  /** If this constructor is used, then access data by calling @a method on
  * object instance @a obj. If @a lockMethod and
  * @a unlockMethod are provided (not NULL), then these are called before and
  * after calling @a method.
  */
  ArServerDataProvider(ArServerBase *server, const char *name, const char *description, C* obj, T (C::*method)(void), void (C::*lockMethod)(void)= NULL, void (C::*unlockMethod)(void) = NULL) : 
    myDeleteFunctors(true)
  {
    ArRetFunctorC<T, C> *accessFunctor = new ArRetFunctorC<T, C>(obj, method);
    assert((lockMethod && unlockMethod) || (!lockMethod && !unlockMethod));
    ArFunctorC<C> *lockFunctor, *unlockFunctor;
    if(lockMethod) lockFunctor = new ArFunctorC<C>(obj, lockMethod);
    if(unlockMethod) unlockFunctor = new ArFunctorC<C>(obj, unlockMethod);
    init(server, name, description, accessFunctor, lockFunctor, unlockFunctor);
  }

  /** If this constructor is used, then access data by calling @a method on
  * object instance @a obj. If @a lockMethod and
  * @a unlockMethod are provided (not NULL), then these are called before and
  * after calling @a method.  This version allows @a method to be
  * <tt>const</tt>.
  */
  ArServerDataProvider(ArServerBase *server, const char *name, const char *description, C* obj, T (C::*method)(void) const, void (C::*lockMethod)(void) = NULL, void (C::*unlockMethod)(void) = NULL) :
    myDeleteFunctors(true)
  {
    ArConstRetFunctorC<T, C>* accessFunctor = new ArConstRetFunctorC<T, C>(obj, method);
    assert((lockMethod && unlockMethod) || (!lockMethod && !unlockMethod));
    ArFunctorC<C> *lockFunctor, *unlockFunctor;
    if(lockMethod) lockFunctor = new ArFunctorC<C>(obj, lockMethod);
    if(unlockMethod) unlockFunctor = new ArFunctorC<C>(obj, unlockMethod);
    init(server, name, description, accessFunctor, lockFunctor, unlockFunctor);
  }

  virtual ~ArServerDataProvider() 
  {
    if(myLockFunctor) myLockFunctor->invoke();
    if(myServer) myServer->remData(myReqName);
    if(myUnlockFunctor) myUnlockFunctor->invoke();
    if(myDeleteFunctors && myLockFunctor) delete myLockFunctor;
    if(myDeleteFunctors && myUnlockFunctor) delete myUnlockFunctor;
    if(myPackFunctor) delete myPackFunctor;
    if(myDeleteFunctors && myAccessFunctor) delete myAccessFunctor;
    ArLog::log(ArLog::Verbose, "%s ArPublisher: Deleted");
  }
};

#endif
