#ifndef ARSERVERINFOTRAILDRAWING
#define ARSERVERINFOTRAILDRAWING

#include "Aria.h"
#include "ArNetworking.h"
#include <deque>

/** Stores a history of ArPose objects, and provides them as drawings to a
 * client (such as MobileEyes) via ArServerInfoDrawings.  Positions are obtained
 * via the supplied functor, so any object providing positions as ArPose objects
 * may be used as the source of the trail.  This functor is called when
 * triggered by calling pull(). 
 *
 * Example usage:
 * @code
   ArServerInfoTrailDrawing trailDrawing(
    "MyRobotTrail", 
    new ArRetFunctorC<ArPose, ArRobot>(&robot, ArRobot::getPose),
    200,
    ArDrawingData(),
    drawingServer
  );
  robot.addSensorInterpTask("MyRobotTrailDrawing", 20, trailDrawing.getPullFunc());
 * @endcode
 * Note: <tt>trailDrawing</tt> must remain in scope. If <tt>trailDrawing</tt> is
 * destroyed (e.g. because a block where it was created returns), the drawing
 * will be removed.   You can use <tt>new</tt> to allocate a new object which
 * will persist outside the local stack until explicitly deleted, or create and store the
 * trail drawing object as a member of another object which is not destroyed.
 * 
 * @todo A way to automatically lock a mutex on the target object when triggered.
 * @todo Don't add new points that are the same or very close to existing points in buffer.
 */
class ArServerInfoTrailDrawing 
{
public:

  /** 
    @param name Name to use for this drawing (displayed in MobileEyes Map menu, legend, log messages, etc.)
    @param func Functor to invoke to poll for a position.
    @param max  Maximum number of past points to include in the trail
    @param dd   ArDrawingData.  Typically, use "polyDots" type. Specify color, size, layer etc.
    @param drawingServer ArServerInfoDrawings object
    @param interval Poll interval.  When ArServerInfoTrailDrawing::pull() is
called, a counter is updated, and new points are obtained only at the given
interval.  A value of 1 causes a new point to be saved every time pull() is
called.
    @param minDist If a new point obtained is closer than this distance to the
previous point, it is not saved in the trail.
  */
  ArServerInfoTrailDrawing(const char *name, ArRetFunctor<ArPose> *func, size_t max, const ArDrawingData &dd, ArServerInfoDrawings &drawingServer, unsigned int interval = 1, float minDist = 0) : 
      myFunc(func), 
      myNumPoints(max), 
      myDrawingData(dd), 
      myDrawFunc(this, &ArServerInfoTrailDrawing::draw),
      myPullFunc(this, &ArServerInfoTrailDrawing::pull), 
      myName(name), 
      myInterval(interval), 
      myCounter(0),
      myMinDist(minDist)
  {
    drawingServer.addDrawing(&myDrawingData, name, &myDrawFunc);
  }

  /** Add this method as a callback to an object (e.g. ArRobot task), or call it
   * when you want a new position added to the trail. You can get a functor
   * to use as a callback with getPullFunc().  Example:
   * @code
   *   robot.addSensorInterpTask("Trail", 20, trailDrawings.getPullFunc());
   * @endcode
   * 
   * In Python, you can reference the function directly:
   * @code
   *   robot.addSensorInterpTask('Trail', 20, trailDrawings.pull)
   * @endcode
   */
  void pull() 
  {
    //printf("XXX trails myCounter=%d myInterval=%d\n", myCounter, myInterval);
    if(++myCounter < myInterval) 
      return;
    myCounter = 0;
    ArPose p = myFunc->invokeR();
    if(!myPoints.empty() && p.findDistanceTo(myPoints.back()) < myMinDist)
      return;
    while(myPoints.size() >= myNumPoints)
      myPoints.pop_front();
    //printf("XXX trails saving point\n");
    myPoints.push_back(p);
  } 
  
  ArFunctor *getPullFunc() { return &myPullFunc; }

  void setPullInterval(unsigned int i) { myInterval = i; }
  void setMinDist(float d) { myMinDist = d; }
  void setMaxNumPoints(unsigned int n) { myNumPoints = n; }

protected:

  void draw(ArServerClient *client, ArNetPacket *pkt)
  {
    ArNetPacket reply;
    reply.byte4ToBuf(myPoints.size());
    //printf("XXX trail %d points\n", myPoints.size());
    for(std::deque<ArPose>::const_iterator i = myPoints.begin(); i != myPoints.end(); ++i)
    {
      reply.byte4ToBuf((int)(i->getX()));
      reply.byte4ToBuf((int)(i->getY()));
    }
    client->sendPacketUdp(&reply);
  }

  ArRetFunctor<ArPose> *myFunc;
  size_t myNumPoints;
  ArDrawingData myDrawingData;
  std::deque<ArPose> myPoints;
  ArFunctor2C<ArServerInfoTrailDrawing, ArServerClient*, ArNetPacket*> myDrawFunc;
  ArFunctorC<ArServerInfoTrailDrawing> myPullFunc;
  std::string myName;
  unsigned int myInterval;
  unsigned int myCounter;
  float myMinDist;
};

#endif
