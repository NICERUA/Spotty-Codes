/***************************************************************************************/
/*                                      Chi.h

    This is the header file for Chi.cpp, which holds the computational methods/functions 
    called in Spot.cpp and probably elsewhere.
    
    And now it computes chi^2 too!
*/
/***************************************************************************************/

#define NDIM 5  //
#define MPTS 6  //

#define MAX_NUMBINS 128 // REMEMBER TO CHANGE THIS IN STRUCT.H AS WELL!!
#define NCURVES 4      // REMEMBER TO CHANGE THIS IN STRUCT.H AS WELL!! number of different light curves that it will calculate



// Calculates chi^2
double ChiSquare( class DataStruct* obsdata, class LightCurve* curve );


// Calculates angles
class LightCurve ComputeAngles( class LightCurve* incurve,
				                class OblDeflectionTOA* defltoa );


// Calculates the light curve, when given all the angles
class LightCurve ComputeCurve( class LightCurve* angles );


//
int FitCurve( class LightCurve* angles, class DataStruct* obsdata,
	          class OblDeflectionTOA* defltoa, char vert[180], char out_dir[80], 
	          double rspot, double omega, double mass, int run, char min_file[180], 
	          double ftol );
	 
	 
//	      
void amoeba(double p[][NDIM+1], double y[], double ftol, int *nfunk, 
            class LightCurve* curve, class DataStruct* obsdata, 
            class OblDeflectionTOA* defltoa, double omega, double mass, double theta, 
            double rspot, double chi_store[], double i_store[], double a_store[], 
            double t_store[], double b_store[], double p_store[], int *k_value );   //GC


//
double amotry( double p[][NDIM+1], double y[], double psum[], int ihi, double fac, 
               class LightCurve* curve, class DataStruct* obsdata, 
               class OblDeflectionTOA* defltoa,  double omega, double mass, double theta, 
               double rspot );  //GC -change psum


//
class OblDeflectionTOA* recalc( class LightCurve* curve,  double omega, double mass, 
                                double theta, double rspot );



// flux from a blackbody (bolometric, p = 0)
double BlackBody( double T, double E );

// Reading NSATMOS tables
void Read_NSATMOS(double T, double M, double R);

// Reading NSX tables
void Read_NSX(double T, double M, double R);

// Linear Interpolation
double Linear(double x,double Xa, double Ya, double Xb, double Yb);

// Linear Interpolation in log-log space
double LogLinear(double x,double Xa, double Ya, double Xb, double Yb);

// Hydrogen
double Hydrogen(double E, double cos_theta);

// Helium
double Helium(double E, double cos_theta);

// Find value in array
int Find(double val, std::vector<double> array);

int Findval(double x, std::vector<double> y);

// Linear Interpolation
double Interpolate(double X_INT, std::vector<double> X, std::vector<double> Y);

// flux from a specific energy band (E1 is lower bound, E2 is upper bound, both in keV) 
//(p = NCURVES-1)
double EnergyBandFlux( double T, double E1, double E2 );

// flux from a specific energy band, for atmosphere models
double AtmosEBandFlux( double model, double cos_theta, double E1, double E2 );

// Does the integral for the flux from a specific energy band
double Bradt_flux_integrand( double x );



// Calculates the graybody factor, if not negligible
double Gray( double cosine );



// Normalizes the light curve flux to 1
class LightCurve Normalize( double Flux[NCURVES][MAX_NUMBINS], unsigned int numbins );



//Calculates Legendre polynomial P2 for equation 8, MLCB
double LegP2( double costheta );



// Calculates Legendre polynomial P4 for equation 8, MLCB
double LegP4( double costheta );



// Calculates the equatorial radius of the neutron star
double calcreq( double omega, double mass, double theta, double rspot );
