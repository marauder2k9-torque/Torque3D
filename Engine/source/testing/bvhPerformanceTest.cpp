#include "testing/unitTesting.h"
#include <chrono>
#include "platform/platform.h"
#include "console/simBase.h"
#include "console/consoleTypes.h"

#include "core/util/tBVH.h"

class MockBVHProxy final : public BVHProxy
{
public:
   Box3F bounds;
   BVHNode* node = nullptr;

   explicit MockBVHProxy(const Box3F& b) : bounds(b) {}

   Box3F getBounds() const override { return bounds; }
   BVHNode* getBVHNode() const override { return node; }

   bool castRay(const Point3F&, const Point3F&, RayInfo*) const override
   {
      return false;
   }

   bool castRayRendered(const Point3F&, const Point3F&, RayInfo*) const override
   {
      return false;
   }
};

class PerfTimer
{
public:
   using clock = std::chrono::steady_clock;

   void start() { mStart = clock::now(); }
   double stopMs()
   {
      auto end = clock::now();
      return std::chrono::duration<double, std::milli>(end - mStart).count();
   }

private:
   clock::time_point mStart;
};

inline void PrintPerfStats(
   const char* label,
   size_t objectCount,
   double avgTotalMs)
{
   const double usPerObject =
      (avgTotalMs * 1000.0) / static_cast<double>(objectCount);

   const double nsPerObject = usPerObject * 1000.0;

   std::cout
      << "[" << label << "] "
      << "Objects: " << objectCount
      << ", Avg Total: " << avgTotalMs << " ms"
      << ", Avg/Object: "
      << usPerObject << " us ("
      << nsPerObject << " ns)"
      << "\n";
}

class BVHPerfFixture : public ::testing::Test
{
protected:
   static constexpr size_t ObjectCount = 100000;
   static constexpr int WarmupRuns = 3;
   static constexpr int TimedRuns = 10;

   static constexpr int QueryCount = 10000;
   static constexpr int QueryRuns = 10;

   Vector<MockBVHProxy*> objects;

   void SetUp() override
   {
      objects.reserve(ObjectCount);

      MRandomLCG rng(42);
      for (size_t i = 0; i < ObjectCount; ++i)
      {
         Point3F min(rng.randF() * 1000, rng.randF() * 1000, rng.randF() * 1000);
         Point3F max = min + Point3F(1.f, 1.f, 1.f);
         objects.push_back(new MockBVHProxy(Box3F(min, max)));
      }
   }

   void TearDown() override
   {
      for (auto* o : objects)
         delete o;
      objects.clear();
   }
};

TEST_F(BVHPerfFixture, BuildBVH_InsertLeaf)
{
   PerfTimer timer;
   double totalMs = 0.0;

   // ---- Warm-up ----
   for (int w = 0; w < WarmupRuns; ++w)
   {
      BVH bvh;
      for (auto* obj : objects)
      {
         auto* leaf = bvh.createLeaf(obj);
         obj->node = leaf;
         bvh.insertLeaf(leaf);
      }
   }

   // ---- Timed runs ----
   for (int r = 0; r < TimedRuns; ++r)
   {
      BVH bvh;

      timer.start();
      for (auto* obj : objects)
      {
         auto* leaf = bvh.createLeaf(obj);
         obj->node = leaf;
         bvh.insertLeaf(leaf);
      }
      totalMs += timer.stopMs();
   }

   double avgMs = totalMs / TimedRuns;

   PrintPerfStats(
      "BVH InsertLeaf",
      ObjectCount,
      avgMs);
}

TEST_F(BVHPerfFixture, UpdateLeaf_Performance)
{
   BVH bvh;

   for (auto* obj : objects)
   {
      auto* leaf = bvh.createLeaf(obj);
      obj->node = leaf;
      bvh.insertLeaf(leaf);
   }

   PerfTimer timer;
   double totalMs = 0.0;

   // Slightly move bounds each run
   for (int r = 0; r < TimedRuns; ++r)
   {
      timer.start();

      for (auto* obj : objects)
      {
         obj->bounds.minExtents.x += 0.01f;
         obj->bounds.maxExtents.x += 0.01f;
         bvh.updateLeaf(obj->node);
      }

      totalMs += timer.stopMs();
   }

   const double avgMs = totalMs / TimedRuns;

   PrintPerfStats(
      "BVH UpdateLeaf",
      ObjectCount,
      avgMs);
}

TEST_F(BVHPerfFixture, QueryRegion_Performance)
{
   BVH bvh;

   for (auto* obj : objects)
   {
      auto* leaf = bvh.createLeaf(obj);
      obj->node = leaf;
      bvh.insertLeaf(leaf);
   }

   Vector<Box3F> queries;
   queries.reserve(QueryCount); 
   MRandomLCG rng(42);
   for (size_t i = 0; i < QueryCount; ++i)
   {
      Point3F min(rng.randF() * 1000, rng.randF() * 1000, rng.randF() * 1000);
      Point3F max = min + Point3F(1.f, 1.f, 1.f);
      queries.push_back(Box3F(min, max));
   }


   // ---- Warm-up ----
   Vector<BVHProxy*> results;
   for (int w = 0; w < WarmupRuns; ++w)
   {
      for (const auto& q : queries)
      {
         results.clear();
         bvh.queryFromNode(q, bvh.mRoot, &results);
      }
   }

   PerfTimer timer;
   double totalMs = 0.0;
   size_t totalHits = 0;

   for (int r = 0; r < QueryRuns; ++r)
   {
      size_t runHits = 0;

      timer.start();
      for (const auto& q : queries)
      {
         results.clear();
         bvh.queryFromNode(q, bvh.mRoot, &results);
         runHits += results.size();
      }
      totalMs += timer.stopMs();
      totalHits += runHits;
   }

   const double avgMs = totalMs / QueryRuns;
   const double avgQueryUs =
      (avgMs * 1000.0) / static_cast<double>(QueryCount);

   const double avgHitsPerQuery =
      static_cast<double>(totalHits) /
      static_cast<double>(QueryRuns * QueryCount);

   const double usPerHit =
      avgHitsPerQuery > 0.0
      ? avgQueryUs / avgHitsPerQuery
      : 0.0;

   std::cout
      << "[BVH QueryRegion]"
      << "  Queries/run: " << QueryCount
      << "  Avg total:   " << avgMs
      << "  Avg/query:  " << avgQueryUs
      << "  Avg hits/q: " << avgHitsPerQuery
      << "  Us/hit:     " << usPerHit << " us\n";
}
