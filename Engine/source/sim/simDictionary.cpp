//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "sim/simDictionary.h"
#include "sim/simBase.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
extern U32 HashPointer(StringTableEntry e);

SimNameDictionary::SimNameDictionary()
{
   hashTable = NULL;
   hashTableSize = DefaultTableSize;
   hashEntryCount = 0;
   mutex = Mutex::createMutex();
}

SimNameDictionary::~SimNameDictionary()
{
   delete[] hashTable;
   Mutex::destroyMutex(mutex);
}

void SimNameDictionary::insert(SimObject* obj)
{
   if (!obj || !obj->getName())
      return;

   SimObject* checkForDup = find(obj->getName());

   if (checkForDup)
      Con::warnf("Warning! You have a duplicate object name of %s. This can cause problems. You should rename one of them.", obj->getName());

   Mutex::lockMutex(mutex);
   if (!hashTable)
   {
      hashTable = new SimObject *[DefaultTableSize];
      hashTableSize = DefaultTableSize;
      hashEntryCount = 0;

      dMemset(hashTable, 0, sizeof(*hashTable) * DefaultTableSize);
   }

   S32 idx = HashPointer(obj->getName()) % hashTableSize;
   obj->nextNameObject = hashTable[idx];
   hashTable[idx] = obj;
   hashEntryCount++;

   // Rehash if necessary.

   if (hashEntryCount > hashTableSize)
   {
      // Allocate new table.

      U32 newHashTableSize = hashTableSize * 2 + 1;
      SimObject** newHashTable = new SimObject *[newHashTableSize];
      dMemset(newHashTable, 0, sizeof(newHashTable[0]) * newHashTableSize);

      // Move entries over.

      for (U32 i = 0; i < hashTableSize; ++i)
         for (SimObject* object = hashTable[i]; object != NULL; )
         {
            SimObject* next = object->nextNameObject;

            idx = HashPointer(object->getName()) % newHashTableSize;
            object->nextNameObject = newHashTable[idx];
            newHashTable[idx] = object;

            object = next;
         }

      // Switch tables.

      delete[] hashTable;
      hashTable = newHashTable;
      hashTableSize = newHashTableSize;
   }
   Mutex::unlockMutex(mutex);
}

SimObject* SimNameDictionary::find(StringTableEntry name)
{
   // NULL is a valid lookup - it will always return NULL
   if (!hashTable)
      return NULL;

   Mutex::lockMutex(mutex);

   S32 idx = HashPointer(name) % hashTableSize;
   SimObject *walk = hashTable[idx];
   while (walk)
   {
      if (walk->getName() == name)
      {
         Mutex::unlockMutex(mutex);
         return walk;
      }
      walk = walk->nextNameObject;
   }

   Mutex::unlockMutex(mutex);
   return NULL;
}

void SimNameDictionary::remove(SimObject* obj)
{
   if (!obj || !obj->getName())
      return;

   Mutex::lockMutex(mutex);
   SimObject **walk = &hashTable[HashPointer(obj->getName()) % hashTableSize];
   while (*walk)
   {
      if (*walk == obj)
      {
         *walk = obj->nextNameObject;
         obj->nextNameObject = NULL;
         hashEntryCount--;

         Mutex::unlockMutex(mutex);
         return;
      }
      walk = &((*walk)->nextNameObject);
   }
   Mutex::unlockMutex(mutex);
}

//----------------------------------------------------------------------------

SimManagerNameDictionary::SimManagerNameDictionary()
{
   hashTable = new SimObject *[DefaultTableSize];
   hashTableSize = DefaultTableSize;
   hashEntryCount = 0;

   dMemset(hashTable, 0, sizeof(hashTable[0]) * hashTableSize);
   mutex = Mutex::createMutex();
}

SimManagerNameDictionary::~SimManagerNameDictionary()
{
   delete[] hashTable;
   Mutex::destroyMutex(mutex);
}

void SimManagerNameDictionary::insert(SimObject* obj)
{
   if (!obj || !obj->getName())
      return;

   Mutex::lockMutex(mutex);
   S32 idx = HashPointer(obj->getName()) % hashTableSize;
   obj->nextManagerNameObject = hashTable[idx];
   hashTable[idx] = obj;
   hashEntryCount++;

   // Rehash if necessary.

   if (hashEntryCount > hashTableSize)
   {
      // Allocate new table.

      U32 newHashTableSize = hashTableSize * 2 + 1;
      SimObject** newHashTable = new SimObject *[newHashTableSize];
      dMemset(newHashTable, 0, sizeof(newHashTable[0]) * newHashTableSize);

      // Move entries over.

      for (U32 i = 0; i < hashTableSize; ++i)
         for (SimObject* object = hashTable[i]; object != NULL; )
         {
            SimObject* next = object->nextManagerNameObject;

            idx = HashPointer(object->getName()) % newHashTableSize;
            object->nextManagerNameObject = newHashTable[idx];
            newHashTable[idx] = object;

            object = next;
         }

      // Switch tables.

      delete[] hashTable;
      hashTable = newHashTable;
      hashTableSize = newHashTableSize;
   }
   Mutex::unlockMutex(mutex);
}

SimObject* SimManagerNameDictionary::find(StringTableEntry name)
{
   // NULL is a valid lookup - it will always return NULL

   Mutex::lockMutex(mutex);

   S32 idx = HashPointer(name) % hashTableSize;
   SimObject *walk = hashTable[idx];
   while (walk)
   {
      if (walk->getName() == name)
      {
         Mutex::unlockMutex(mutex);
         return walk;
      }
      walk = walk->nextManagerNameObject;
   }
   Mutex::unlockMutex(mutex);

   return NULL;
}

void SimManagerNameDictionary::remove(SimObject* obj)
{
   if (!obj || !obj->getName())
      return;

   Mutex::lockMutex(mutex);

   SimObject **walk = &hashTable[HashPointer(obj->getName()) % hashTableSize];
   while (*walk)
   {
      if (*walk == obj)
      {
         *walk = obj->nextManagerNameObject;
         obj->nextManagerNameObject = NULL;
         hashEntryCount--;

         Mutex::unlockMutex(mutex);
         return;
      }
      walk = &((*walk)->nextManagerNameObject);
   }
   Mutex::unlockMutex(mutex);
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

SimIdDictionary::SimIdDictionary()
{
   table = NULL;
   tableSize = DefaultTableSize;
   tableBitMask = DefaultTableBitMask;
   entryCount = 0;
   mutex = Mutex::createMutex();
}

SimIdDictionary::~SimIdDictionary()
{
   delete[] table;
   Mutex::destroyMutex(mutex);
}



void SimIdDictionary::insert(SimObject* obj)
{
   if (!obj)
      return;

   Mutex::lockMutex(mutex);
   // Lazily allocate on first use - mirrors SimNameDictionary::insert's
   // own lazy-allocation pattern exactly.
   if (!table)
   {
      table = new SimObject * [tableSize];
      dMemset(table, 0, sizeof(table[0]) * tableSize);
      entryCount = 0;
   }

   S32 idx = obj->getId() & tableBitMask;
   obj->nextIdObject = table[idx];
   AssertFatal(obj->nextIdObject != obj, "SimIdDictionary::insert - Creating Infinite Loop linking to self!");
   table[idx] = obj;
   entryCount++;

   // Rehash if necessary - mirrors SimNameDictionary::insert's own
   // rehash trigger (entryCount > tableSize) 
   if (entryCount > tableSize)
   {
      U32 newTableSize = tableSize * 2;
      U32 newTableBitMask = newTableSize - 1;
      SimObject** newTable = new SimObject * [newTableSize];
      dMemset(newTable, 0, sizeof(newTable[0]) * newTableSize);

      // Move entries over.

      for (U32 i = 0; i < tableSize; ++i)
         for (SimObject* object = table[i]; object != NULL; )
         {
            SimObject* next = object->nextIdObject;

            U32 newIdx = object->getId() & newTableBitMask;
            object->nextIdObject = newTable[newIdx];
            newTable[newIdx] = object;

            object = next;
         }

      // Switch tables.

      delete[] table;
      table = newTable;
      tableSize = newTableSize;
      tableBitMask = newTableBitMask;
   }
   Mutex::unlockMutex(mutex);
}

SimObject* SimIdDictionary::find(S32 id)
{
   Mutex::lockMutex(mutex);
   // NULL is a valid state (nothing ever inserted yet) - mirrors
   // SimNameDictionary::find's own guard for an unallocated table.
   if (!table)
   {
      Mutex::unlockMutex(mutex);
      return NULL;
   }

   S32 idx = id & tableBitMask;
   SimObject* walk = table[idx];
   while (walk)
   {
      if (walk->getId() == U32(id))
      {
         Mutex::unlockMutex(mutex);
         return walk;
      }
      walk = walk->nextIdObject;
   }
   Mutex::unlockMutex(mutex);

   return NULL;
}

void SimIdDictionary::remove(SimObject* obj)
{
   if (!obj)
      return;

   Mutex::lockMutex(mutex);
   if (!table)
   {
      Mutex::unlockMutex(mutex);
      return;
   }

   SimObject** walk = &table[obj->getId() & tableBitMask];
   while (*walk && *walk != obj)
      walk = &((*walk)->nextIdObject);
   if (*walk)
   {
      *walk = obj->nextIdObject;
      entryCount--;
   }
   Mutex::unlockMutex(mutex);
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

