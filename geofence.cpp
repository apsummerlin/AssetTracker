/** ---------------------------------------------------------------------------
 * @file      main.cpp
 * 
 * @details   Implements the GeoFence class which is used to identify a virtual 
 *            fence based on a set of GPS points.
 * 
 * @copyright (c)2026, Alan P. Summerlin
 * 
 * @author    Alan P. Summerlin
 * 
 * 
 ** ------------------------------------------------------------------------ */
#include "geofence.h"
#include <TinyGPSPlus.h>			// GPS Module Support Library for GT-U7
#include <math.h>



extern TinyGPSPlus gps;

GeoFence::GeoFence() {
    pointIdx = 0;
}

void GeoFence::addPoint(GeoPoint_t p) {

    if (pointIdx < MAX_POINTS) {
        p.id = pointIdx;
        points[pointIdx] = p;
        
        Serial.printf("Point %d saved.\n\r", pointIdx++);
    } else {
        Serial.println("Failed to set point.");
    }
}

double GeoFence::calcDistance(GeoPoint_t a, GeoPoint_t b) {

    return  gps.distanceBetween(a.lat, a.lng, b.lat, b.lng);
}



// Convert degrees to radians
double GeoFence::toRadians(double degrees) {
    return degrees * (PI / 180.0);
}

// Convert radians to degrees
double GeoFence::toDegrees(double radians) {
    return radians * (180.0 / PI);
}

double GeoFence::calculateBearing(double lat1, double lon1, double lat2, double lon2) {
    double phi1 = toRadians(lat1);
    double phi2 = toRadians(lat2);
    double deltaLambda = toRadians(lon2 - lon1);

    double y = sin(deltaLambda) * cos(phi2);
    double x = cos(phi1) * sin(phi2) - sin(phi1) * cos(phi2) * cos(deltaLambda);
    
    double theta = atan2(y, x);
    double bearing = fmod(toDegrees(theta) + 360.0, 360.0);
    
    return bearing;
}

double GeoFence::calculateAngle(double bearingAB, double bearingAC) {

    double angle = fabs(bearingAB - bearingAC);
    if (angle > 180.0) {
        angle = 360.0 - angle;
    }

    Serial.printf("Bearing AB: %.2f degrees\n", bearingAB);
    Serial.printf("Bearing AC: %.2f degrees\n", bearingAC);
    Serial.printf("Angle between them: %.2f degrees\n", angle);

    return angle;
}

bool GeoFence::loadFenceFromFile(char * file) {

    return true;
}


// If device is inside the fence, return true 
bool GeoFence::isInsideFence(void) {

    return true;
}