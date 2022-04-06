#ifndef _T3D_STOCKPHYSICSPLUGIN_H_
#define _T3D_STOCKPHYSICSPLUGIN_H_

#ifndef _T3D_PHYSICS_PHYSICSPLUGIN_H_
#include "T3D/physics/physicsPlugin.h"
#endif

class StockPlugin : public PhysicsPlugin
{
public:

   StockPlugin();
   ~StockPlugin();

   static PhysicsPlugin * create();

   // PhysicsPlugin
   virtual void destroyPlugin();
   virtual void reset() {}
   virtual PhysicsCollision* createCollision();
   virtual PhysicsBody* createBody();
   virtual PhysicsPlayer* createPlayer();
   virtual bool isSimulationEnabled() const;
   virtual void enableSimulation(const String &worldName, bool enable);
   virtual void setTimeScale(const F32 timeScale);
   virtual const F32 getTimeScale() const;
   virtual bool createWorld(const String &worldName);
   virtual void destroyWorld(const String &worldName);
   virtual PhysicsWorld* getWorld(const String &worldName) const;
   virtual PhysicsWorld* getWorld() const;
   virtual U32 getWorldCount() const;

};

#endif // !_T3D_STOCKPHYSICSPLUGIN_H_
