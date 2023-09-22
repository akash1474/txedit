#include "imgui.h"
#include "math.h"

class Animation{
	float mTick{0.0f};
	float mDuration{0.0f};

	//Animation Function
	inline float EaseOutQuadraticFn(float t) { return 1.0f - pow(1.0f - t, 4);}

	bool mHasCompleted=false;
public:
	bool hasStarted=false;
	Animation(float duration=0.5f):mDuration(duration){}

	void start(){
		mHasCompleted=false;
		hasStarted=true;
	}

	float update(){
		if(mHasCompleted) return 1.0f;
	    mTick += ImGui::GetIO().DeltaTime;
	    float t = fminf(mTick / mDuration, 1.0f);
	    if(t>=1.0f){
	    	mHasCompleted=true;
	    	hasStarted=false;
	    	mTick=0.0f;
	    	return 1.0f;
	    }
	    return EaseOutQuadraticFn(t);
	}

};
