//-----------------------------------------------------------------------------
// GuiInspectorComponentGroup.cpp
//-----------------------------------------------------------------------------
#include "gui/editor/guiInspector.h"
#include "gui/editor/inspector/guiInspectorComponentGroup.h"
#include "gui/editor/inspector/field.h"
#include "sim/component/simComponent.h"
#include "sim/simObject.h"
#include "console/engineAPI.h"
#include "gui/containers/guiRolloutCtrl.h"
#include "gui/containers/guiStackCtrl.h"

IMPLEMENT_CONOBJECT(GuiInspectorComponentGroup);

ConsoleDocClass(GuiInspectorComponentGroup,
   "@brief Displays the fields of a single attached SimComponent instance.\n\n"
   "Editor use only.\n\n"
   "@internal"
);

//-----------------------------------------------------------------------------

bool GuiInspectorComponentGroup::createContent()
{
   return Parent::createContent();
}

//-----------------------------------------------------------------------------

bool GuiInspectorComponentGroup::inspectGroup()
{
   // We can't inspect a group without a target - here that means both
   // the component (whose field list we enumerate) and the owner (which
   // every constructed field will actually target).
   if (!mParent || !mComponent || !mOwner)
      return false;

   mStack->freeze(true);

   // Just delete all fields and recreate them (mirrors the base class
   // and GuiInspectorDynamicGroup's own approach - simpler than diffing,
   // and this is only called on refresh()/re-inspect, not per-frame).
   clearFields();

   bool bNewItems = false;
   bool bMakingArray = false;
   GuiStackControl* pArrayStack = NULL;
   GuiRolloutCtrl* pArrayRollout = NULL;

   // Unlike the base GuiInspectorGroup::inspectGroup(), which walks
   // findCommonAncestorClass()->mFieldList (a class shared across every
   // object in mParent's target list), here the field list comes
   // directly from the ONE component instance this group represents -
   // and every GuiInspectorField constructed below targets mComponent
   // directly too (see setTargetObject(mComponent) further down), since
   // there's exactly one component in play here, not a list to match up.
   const AbstractClassRep::FieldList& fieldList = mComponent->getFieldList();

   bool bGrabItems = false;

   for (AbstractClassRep::FieldList::const_iterator itr = fieldList.begin();
      itr != fieldList.end(); ++itr)
   {
      AbstractClassRep::Field* field = const_cast<AbstractClassRep::Field*>(&(*itr));

      if (field->type == AbstractClassRep::StartGroupFieldType)
      {
         // A component's own initPersistFields may use addGroup/endGroup
         // internally to organize its own fields into sub-sections;
         // since this whole GuiInspectorComponentGroup IS already one
         // section for the component, nested group markers are simply
         // skipped rather than creating further nested rollouts.
         bGrabItems = true;
         continue;
      }
      else if (field->type == AbstractClassRep::EndGroupFieldType)
      {
         bGrabItems = false;
         continue;
      }

      if (field->flag.test(AbstractClassRep::FIELD_HideInInspectors))
         continue;

      if (field->type == AbstractClassRep::DeprecatedFieldType)
         continue;

      String searchText = mParent->getSearchText();
      if (searchText != String::EmptyString)
      {
         if (String(field->pFieldname).find(searchText, 0, String::NoCase | String::Left) == String::NPos)
            continue;
      }

      // --- Shape 1: StartArrayFieldType / EndArrayFieldType ---
      if ((field->type == AbstractClassRep::StartArrayFieldType || field->type == AbstractClassRep::EndArrayFieldType) && mForcedArrayIndex != -1)
      {
         continue;
      }

      if (field->type == AbstractClassRep::StartArrayFieldType)
      {
         GuiRolloutCtrl* arrayRollout = new GuiRolloutCtrl();
         GuiControlProfile* arrayRolloutProfile = dynamic_cast<GuiControlProfile*>(Sim::findObject("GuiInspectorRolloutProfile0"));

         arrayRollout->setControlProfile(arrayRolloutProfile);
         arrayRollout->setCaption(field->pGroupname);
         arrayRollout->registerObject();

         GuiStackControl* arrayStack = new GuiStackControl();
         arrayStack->registerObject();
         arrayStack->freeze(true);
         arrayRollout->addObject(arrayStack);

         for (U32 i = 0; i < field->elementCount; i++)
         {
            GuiRolloutCtrl* elementRollout = new GuiRolloutCtrl();
            GuiControlProfile* elementRolloutProfile = dynamic_cast<GuiControlProfile*>(Sim::findObject("GuiInspectorRolloutProfile0"));

            char buf[256];
            dSprintf(buf, 256, "  [%i/%i]", i, field->elementCount);

            elementRollout->setControlProfile(elementRolloutProfile);
            elementRollout->setCaption(buf);
            elementRollout->registerObject();

            GuiStackControl* elementStack = new GuiStackControl();
            elementStack->registerObject();
            elementRollout->addObject(elementStack);
            elementRollout->instantCollapse();

            arrayStack->addObject(elementRollout);

            mArrayElements.push_back({ elementRollout, (S32)i, field });
         }

         pArrayRollout = arrayRollout;
         pArrayStack = arrayStack;
         arrayStack->freeze(false);
         pArrayRollout->instantCollapse();
         mStack->addObject(arrayRollout);

         bMakingArray = true;
         continue;
      }
      else if (field->type == AbstractClassRep::EndArrayFieldType)
      {
         bMakingArray = false;
         continue;
      }

      if (bMakingArray)
      {
         // Add a GuiInspectorField for this field, for every element in
         // the array's rollout stack, each targeting mOwner.
         for (U32 i = 0; i < pArrayStack->size(); i++)
         {
            FrameTemp<char> intToStr(64);
            dSprintf(intToStr, 64, "%d", i);

            GuiRolloutCtrl* pRollout = dynamic_cast<GuiRolloutCtrl*>(pArrayStack->at(i));
            GuiStackControl* pStack = dynamic_cast<GuiStackControl*>(pRollout->at(0));

            GuiInspectorField* fieldGui = constructField(field->type);
            if (fieldGui == NULL)
               fieldGui = new GuiInspectorField();

            fieldGui->init(mParent, this);
            fieldGui->setTargetObject(mComponent);
            StringTableEntry caption = field->pFieldname;
            fieldGui->setInspectorField(field, caption, intToStr);

            if (fieldGui->registerObject())
            {
               mChildren.push_back(fieldGui);
               pStack->addObject(fieldGui);
            }
            else
               delete fieldGui;
         }

         continue;
      }

      // --- Shapes 2 and 3: flat field->elementCount > 1 ---
      if (field->elementCount > 1)
      {
         if (mForcedArrayIndex == -1)
         {
            // Shape 2: one rollout, one GuiInspectorField per index.
            GuiRolloutCtrl* rollout = new GuiRolloutCtrl();
            rollout->setDataField(StringTable->insert("profile"), NULL, "GuiInspectorRolloutProfile0");
            rollout->setCaption(String::ToString("%s (%i)", field->pFieldname, field->elementCount));
            rollout->setMargin(14, 0, 0, 0);
            rollout->registerObject();
            mArrayCtrls.push_back(rollout);

            GuiStackControl* stack = new GuiStackControl();
            stack->setDataField(StringTable->insert("profile"), NULL, "GuiInspectorStackProfile");
            stack->registerObject();
            stack->freeze(true);
            rollout->addObject(stack);

            mStack->addObject(rollout);

            for (S32 nI = 0; nI < field->elementCount; nI++)
            {
               FrameTemp<char> intToStr(64);
               dSprintf(intToStr, 64, "%d", nI);

               String fieldName = String::ToString("%s%d", field->pFieldname, nI);

               GuiInspectorField* fieldGui = findField(fieldName);
               if (fieldGui != NULL)
               {
                  fieldGui->updateValue();
                  continue;
               }

               bNewItems = true;

               fieldGui = constructField(field->type);
               if (fieldGui == NULL)
                  fieldGui = new GuiInspectorField();

               fieldGui->init(mParent, this);
               fieldGui->setTargetObject(mComponent);
               StringTableEntry caption = StringTable->insert(String::ToString("   [%i]", nI));
               fieldGui->setInspectorField(field, caption, intToStr);

               if (fieldGui->registerObject())
               {
                  mChildren.push_back(fieldGui);
                  stack->addObject(fieldGui);
               }
               else
                  delete fieldGui;
            }

            stack->freeze(false);
            stack->updatePanes();
            rollout->instantCollapse();
         }
         else
         {
            // Shape 3: only the field at mForcedArrayIndex, no rollout.
            FrameTemp<char> intToStr(64);
            dSprintf(intToStr, 64, "%d", mForcedArrayIndex);

            String fieldName = String::ToString("%s%d", field->pFieldname, mForcedArrayIndex);

            GuiInspectorField* fieldGui = findField(fieldName);
            if (fieldGui != NULL)
            {
               fieldGui->updateValue();
               continue;
            }

            bNewItems = true;

            fieldGui = constructField(field->type);
            if (fieldGui == NULL)
               fieldGui = new GuiInspectorField();

            fieldGui->init(mParent, this);
            fieldGui->setTargetObject(mComponent);
            fieldGui->setInspectorField(field, field->pFieldname, intToStr);

            if (fieldGui->registerObject())
            {
               mChildren.push_back(fieldGui);
               mStack->addObject(fieldGui);
            }
            else
               delete fieldGui;
         }

         continue;
      }

      // --- Non-array field (the common case) ---
      GuiInspectorField* fieldGui = findField(field->pFieldname);
      if (fieldGui != NULL)
      {
         fieldGui->updateValue();
         continue;
      }

      bNewItems = true;

      fieldGui = constructField(field->type);
      if (fieldGui == NULL)
         fieldGui = new GuiInspectorField();

      fieldGui->init(mParent, this);

      // Target the COMPONENT directly
      fieldGui->setTargetObject(mComponent);
      fieldGui->setInspectorField(field);

      if (fieldGui->registerObject())
      {
         mChildren.push_back(fieldGui);
         mStack->addObject(fieldGui);
      }
      else
      {
         delete fieldGui;
      }
   }

   mStack->freeze(false);
   mStack->updatePanes();

   if (bNewItems == false && !mChildren.empty())
      return true;

   sizeToContents();
   setUpdate();

   return true;
}

//-----------------------------------------------------------------------------

void GuiInspectorComponentGroup::updateAllFields()
{
   if (!mComponent || !mOwner)
      return;

   Parent::updateAllFields();
}
