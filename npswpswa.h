enum AnimationCallType
{  AnimationInitialize, AnimationDraw, AnimationErase, AnimationTerminate
};

struct AnimationData
{  HPS hps;
   HWND hwnd;
   RECTL rectWindow;

   POINTL ptCenter, ptRelRightTop;
   BOOL fOpen;
   enum AnimationCallType animCallType;

   LONG lStep, cTotalSteps;
   LONG cAfterimages;
   LONG lParameter;

   LONG lVersion;
   RECTL rectScreen;

   LONG alReserved[27];  // do not use it

   CHAR achBuffer[4000];  // you can use it freely
};
   
typedef BOOL EXPENTRY (*PfnAnimation)(struct AnimationData *pAnimationData);
