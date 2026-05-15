#ifndef commands_h
#define commands_h
/** ---------------------------------------------------------------------------
 * @file      geofence.h
 * 
 * @details   Header for the GeoFence class
 * 
 * @copyright (c)2026, Alan P. Summerlin
 * 
 * @author    Alan P. Summerlin
 * 
 ** ------------------------------------------------------------------------ */

#define MAX_POINTS 8

struct GeoPoint_t {
    int     id;
    double  lng;
    double  lat;
};

class GeoFence {

    private:
        int pointIdx;
        GeoPoint_t  points[MAX_POINTS];

        double toRadians(double degrees);
        double toDegrees(double radians);

    public:
        GeoFence();
        void addPoint(GeoPoint_t p);
        double calcDistance(GeoPoint_t a, GeoPoint_t b);
        double calculateBearing(double lat1, double lon1, double lat2, double lon2);
        double calculateAngle(double bearingAB, double bearingAC);
        bool loadFenceFromFile(char * file);
        bool isInsideFence(void);
        void resetFence(void);
};

#endif
