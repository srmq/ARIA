#ifndef NLDATARECEIVER_H
#define NLDATARECEIVER_H

#include "Aria.h"
#include "ArClientBase.h"
#include "ArPacketUtil.h"


// TODO: maybe divide into three classes representing the three ways to use it?
// * class  which takes class method and object 
// * class which takes global function
// * caching 

/** A client program may use this to receive responses to data requests and pass
 * the received data to a provided function or method, or save (cache) the most
 * recently received data.
 *
 * @sa ArServerDataProvider
 * 
 * @note If the server does not have the requested data item at time of
 * request, it will not be requested. A warning will be logged.
 *
 * @since 2.9.3
 * @unstable
*/

template <class C, typename T>
class ArClientDataReceiver 
{
  ArClientBase *myClient;
  const char *myReqName;
  ArFunctor1C<ArClientDataReceiver, ArNetPacket*> myUnpackFunctor;
  ArFunctor1<const T&> *myUserFunctor;
  bool myDeleteUserFunctor;
  void handler(ArNetPacket* pkt)
  {
    myUserFunctor->invoke(ArPacketUtil::getNextField<T>(*pkt));
  }
public:
    typedef enum { Once, OnceUDP, Repeat, None } RequestType;
    
  /** Register a function to handle data of a specific type received from the server.  
      This class will call client->request(), client->requestOnce() or client->requestOnceUdp() to request the data 
      after registering the handler.  
      @note the data sent by the server must be of the given type, or 
  */
  ArClientDataReceiver(ArClientBase *client, const char *requestName, ArFunctor1<const T&> *functor, long interval) :
      myClient(client),
      myReqName(requestName),
      myUnpackFunctor(this, &ArClientDataReceiver::handler),
      myUserFunctor(functor),
      myDeleteUserFunctor(false)
  {
    init(client, requestName, interval);
  }
  
private:
  bool init(ArClientBase *client, const char *requestName,  long interval)
  {
    if(!client->dataExists(requestName))
    {
      ArLog::log(ArLog::Normal, "ArClientDataReceiver for \"%s\": Server does not have this data. Not requesting.", requestName);
      // todo keep trying later?
      return false;
    }
    bool r = client->addHandler(requestName, &myUnpackFunctor);
    if(!r)
      return r;
    r = client->request(requestName, interval);
    return r;
  }
  
 public: 
  ArClientDataReceiver(ArClientBase *client, const char *requestName, C *obj,
void (C::*function)(const T&), long interval = -1) :
    myUnpackFunctor(this, &ArClientDataReceiver::handler)
  {
      myUserFunctor = new ArFunctor1CConst<C, const T&>(obj, function);
      myDeleteUserFunctor = true;
      init(client, requestName, interval);
  }

  /**
      The destructor calls client->remHandler() to remove the handler, and
      client->requestStop() to stop receiving the data.
  */
  virtual ~ArClientDataReceiver()
  {
    if(myClient) myClient->remHandler(myReqName, &myUnpackFunctor);
    if(myDeleteUserFunctor) delete myUserFunctor;
  }
};


#endif
