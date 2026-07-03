#include "atmosphere.h"
#include "raylib.h"
#include "raymath.h"
#include "constants.h"

static constexpr float MIN_BRIGHTNESS = 0.06f;

static constexpr Color DAY_COLOR      = SKYBLUE;
static constexpr Color APRICOT        = {255, 178, 120, 255};
static constexpr Color MIDNIGHT_COLOR = {5, 5, 20, 255};


float CalculateSunBrightness(float timeOfDay){
    
    if (timeOfDay >= 0 && timeOfDay <= 12000.0f){
        return 1.0f;
    }
    if(timeOfDay > 12000.0f && timeOfDay < 13000.0f){
        float t = (timeOfDay - 12000.0f) / 1000;
        return Lerp(1.0f, MIN_BRIGHTNESS, t);
    }
    if(timeOfDay >= 13000.0f && timeOfDay <=23000.0f){
        return MIN_BRIGHTNESS;
    } else{
        float t = (timeOfDay - 23000.0f) / 1000;
        return Lerp(MIN_BRIGHTNESS, 1.0f, t);
    }
}

Color CalculateSkyColor(float timeOfDay){
    if (timeOfDay >= 0 && timeOfDay <= 12000.0f){
        return DAY_COLOR;
    }
    if(timeOfDay > 12000.0f && timeOfDay < 12500.0f){
        float t = (timeOfDay - 12000.0f) / 500;
        return ColorLerp(DAY_COLOR, APRICOT, t);
    }
    if(timeOfDay >= 12500.0f && timeOfDay < 13000.0f){
        float t = (timeOfDay - 12500.0f) / 500;
        return ColorLerp(APRICOT, MIDNIGHT_COLOR, t);
    }
    if(timeOfDay >= 13000.0f && timeOfDay <=23000.0f){
        return MIDNIGHT_COLOR;
    } 
    if(timeOfDay > 23000.0f && timeOfDay < 23500.0f){
        float t = (timeOfDay - 23000.0f) / 500;
        return ColorLerp(MIDNIGHT_COLOR, APRICOT, t);
    }
    else{
        float t = (timeOfDay - 23500.0f) / 500;
        return ColorLerp(APRICOT, DAY_COLOR, t);
    }
}