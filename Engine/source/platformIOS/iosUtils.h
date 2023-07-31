//
//  iosUtils.h
//  TestProject
//
//  Created by ctrlintelligence on 30/07/2023.
//

#ifndef iosUtils_h
#define iosUtils_h

struct utsname;

class IOSUtils
{
public:
   IOSUtils();
   virtual ~IOSUtils();
   
   bool inBackground();
   
   const char* getOSName();
private:
   struct utsname *mUnameInfo;
};

extern IOSUtils *IOSUtils;

#endif /* iosUtils_h */
