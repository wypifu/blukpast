#ifndef BKP_EXPORT_H_
#define BKP_EXPORT_H_

/*********************************************************************
 * Defines
*********************************************************************/
#ifdef BKP_EXPORT
 #ifdef _MSC_VER 
   #define BKP_EXPORTED __declspec(dllexport)
 #else
   #define BKP_EXPORTED __attribute__((visibility("default")))
 #endif
#else
 #ifdef _MSC_VER 
   //#define BKP_EXPORTED __declspec(dllimport)
   #define BKP_EXPORTED __declspec(dllexport)
 #else
   #define BKP_EXPORTED
 #endif
#endif
  
#endif // BKP_EXPORT_H_

