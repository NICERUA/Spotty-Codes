/***************************************************************************************/
/*                                     Chi.cpp

    This holds the computational methods/functions called in Spot.cpp and probably
    elsewhere.
    
    And now it computes chi^2 too!
    
    PGxx means equation xx in Poutanen & Gierlinski 2003, MNRAS 343
    MLCBxx means equation xx in Morsink, Leahy, Cadeau & Braga 2007, ApJ 663 
*/
/***************************************************************************************/

#include "matpack.h"
#include <exception>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "Chi.h"
#include "OblDeflectionTOA.h"
#include "OblModelBase.h"
#include "PolyOblModelNHQS.h"
#include "Exception.h"
#include "Units.h"
#include "Struct.h"
#include "time.h"
#include <stdio.h>
using namespace std;

/**************************************************************************************/
/* ChiSquare:                                                                         */
/*           computes the chi^2 fit of data vs a simulation                           */
/*																					  */
/* pass: obsdata = light curve fluxes from observational data                         */
/*       ts = time shift or phase shift (normalized)                                  */
/**************************************************************************************/
double ChiSquare ( class DataStruct* obsdata, class LightCurve* curve) {
        
    /***************************************/
   	/* VARIABLE DECLARATIONS FOR ChiSquare */
    /***************************************/
    
    unsigned int numbins;  // Number of phase (or time) bins the light curve is cut into
    
    int k,      // Array index variable
    	new_b,  // 
    	n;      // Array index variable
    
    double ts,                              // time shift, so the phase of the simulation matches the phase of the data
           new_shift,                       // A time shift (for rebinning?)
    	   ja_check,                        // 
    	   chisquare(0.0),                  // Computed chi^2
    	   tempflux[NCURVES][MAX_NUMBINS],  // Temporary array to store the flux
    	   min_location;                    // 
    
    numbins = obsdata->numbins;
    ts = curve->para.ts;
    
    // the following is used to make 'todaysdate' be today's date in string format, for filename writing purposes
	/*time_t tim;
	struct tm *ptr;
	char todaysdate[80];
	time( &tim );
	ptr = localtime( &tim );
	strftime(todaysdate,80,"%d-%b-%Y",ptr);
	
	char everything_out_file[265];
	// Assigning the filename with todaysdate to '****_out_file'
	strcpy(everything_out_file, "contours/");
	strcat(everything_out_file, todaysdate);
	strcat(everything_out_file, "_everything.txt");*/
	
    //for (unsigned int i(0); i < numbins; i++) {
    	//std::cout << obsdata->t[i] << "\t" << obsdata->f[2][i] << "\t" << obsdata->err[2][i] << std::endl;
    //}
    for ( unsigned int z(1); z<=1 ; z++ ) { // for different epochs
        
        while ( ts < 0.0 ) {
            ts += 1.0;
        }
        while ( ts > 1.0 ) {
            ts -= 1.0;
        }
        
        min_location = ts * numbins; // real version of the bin with the num
        new_shift = modf(min_location, &ja_check) / (numbins*1.0); // makes min_location an int
        new_b = ja_check;
        
        /*if ( ts < 1.0 ) { // ?? looks like it's for cycling around
            if ( ( ts+1 / (numbins*1.0) ) > 1.0 ) { // changed to numbins from 64 // ???
                new_b = 0; // ??
                new_shift = ts - 1.0; // ??
            }
        }*/
        
        // Rebinning the data and store shifted data back in Flux
        
        for ( unsigned int i(0); i < numbins; i++ ) {
            k = i - new_b; //May changed
            if (k > static_cast<int>(numbins)-1) k -= numbins;
            if (k < 0) k += numbins;
            
            unsigned int p = 1;
        	unsigned int q = 2;
            tempflux[p][i] = curve->f[q][k]; // putting things from curve->f's k bin into tempflux's i bin
            p = 2;
        	q = 3;
        	tempflux[p][i] = curve->f[q][k]; // putting things from curve->f's k bin into tempflux's i bin
        }
        
        for ( unsigned int i(0); i < numbins; i++ ) {
            n = i - 1;
            if ( n < 0 ) n += numbins;
            // n = i+1;
            if ( n > static_cast<int>(numbins) - 1 ) n -= numbins;
            
            unsigned int p = 1;
        	unsigned int q = 2;
            curve->f[q][i] = tempflux[p][i] + (tempflux[p][n]-tempflux[p][i]) * new_shift * numbins;
            p = 2;
        	q = 3;
        	curve->f[q][i] = tempflux[p][i] + (tempflux[p][n]-tempflux[p][i]) * new_shift * numbins;
        }

        
        // Compute chisquare for shifted data
        
        // energy band 1
        unsigned int p = 1;
        unsigned int q = 2;
        for ( unsigned int i(0); i < numbins; i++ ) {
            chisquare += pow( (obsdata->f[p][i] - curve->f[q][i])/obsdata->err[p][i], 2);
            // using different 'p' and 'q' because we're not comparing the exact same columns of obsdata.f and Flux
        }

		// energy band 2
        p = 2;
        q = 3;
        for ( unsigned int i(0); i < numbins; i++ ) {
            chisquare += pow( (obsdata->f[p][i] - curve->f[q][i])/obsdata->err[p][i], 2);
            // using different 'p' and 'q' because we're not comparing the exact same columns of obsdata.f and Flux
        }
    }
    
    //chisquare += pow( (bbrat - 0.32)/(0.064) , 2);
    
    //std::cout << "M = " << Units::nounits_to_cgs(curve->para.mass, Units::MASS)/Units::MSUN << ", R = " << Units::nounits_to_cgs(curve->para.radius, Units::LENGTH )*1.0e-5 << ", i = " 
    //<< curve->para.incl * 180.0 / Units::PI <<", e = " << curve->para.theta * 180.0 / Units::PI <<", ts = " << ts << std::endl;

    //std::cout << "\nChisquare = " << chisquare << std::endl;

    return chisquare;
    
}

/**************************************************************************************/
/* ComputeAngles:                                                                     */
/*              computes all angles necessary to create the x-ray light curve         */
/*																					  */
/* pass: incurve = contains needed values like mass, radius, inclination, theta_0     */
/*       defltoa =                                                                    */
/**************************************************************************************/
class LightCurve ComputeAngles ( class LightCurve* incurve,
				                 class OblDeflectionTOA* defltoa) {

	/*******************************************/
	/* VARIABLE DECLARATIONS FOR ComputeAngles */
	/*******************************************/
	
    class LightCurve curve;
    curve = *incurve;

    double theta_0,                   // Emission angle (latitude) of the spot, in radians          
           incl,                      // inclination angle of the observer, in radians
           mass,                      // Mass of the star, in M_sun
           radius,                    // Radius of the star (at whatever little bit we're evaluating at)
           omega,                     // Spin frequency of the neutron star, in Hz
           cosgamma,                  // Cos of the angle between the radial vector and the surface normal vector
           shift_t,                   // Time off-set from data
           b_guess(0.0),              // Impact parameter; starting off with reasonable guess then refining it
           mu(1.0),                   // = cos(theta_0), unitless
           speed(0.0),                // Velocity of the spot, defined in MLCB34
           alpha(0.0),                // Zenith angle, in radians
           sinalpha(0.0),             // Sin of zenith angle, defined in MLCB19
           cosalpha(1.0),             // Cos of zenith angle, used in MLCB30
           b(0.0),                    // Photon's impact parameter
           toa_val(0.0),              // Time of arrival, MLCB38
           dpsi_db_val(0.0),          // Derivative of MLCB20 with respect to b
           phi_0,                     // Azimuthal location of the spot, in radians
           dS,                        // Surface area of the spot, defined in MLCB2; computed in Spot.cpp
           distance;                  // Distance from Earth to NS, inputted in meters
           
    unsigned int numbins(MAX_NUMBINS);// Number of phase bins the light curve is split into; same as number of flux data points
    numbins = curve.numbins;


    bool ingoing(false);
    bool infile_is_set(false);

    std::vector< double > phi_em(numbins, 0.0);   // Azimuth as measured from the star's emission frame; one for each phase bin
    std::vector< double > psi(numbins, 0.0);      // Bending angle, as defined in MLCB20

    // These are calculated in the second loop.
    std::vector< double > dcosalpha_dcospsi(numbins, 0.0);    // Used in MLCB30
    std::vector< double > cosdelta(numbins, 0.0);             // 
    std::vector< double > cosxi(numbins, 0.0);                // Used in Doppler boost factor, defined in MLCB35

    // vectors for 4-point interpolation
    std::vector< double > psi_k(4, 0.0);       // Bending angle, sub k?
    std::vector< double > b_k(4, 0.0);         // Impact parameter, sub k?
    
    /************************************************************************************/
    /* SETTING THINGS UP - keep in mind that these variables are in dimensionless units */
    /************************************************************************************/
    
    dS = curve.para.dS;
    theta_0 = curve.para.theta;
    incl = curve.para.incl;
    phi_0 = curve.para.phi_0;
    mass = curve.para.mass;
    radius = curve.para.radius;
    omega = curve.para.omega;
    cosgamma = curve.para.cosgamma;  // for spherical, cosgamma = 0
    distance = curve.para.distance;
	shift_t = curve.para.ts;
	infile_is_set = curve.flags.infile_is_set;
	speed = omega*radius*sin(theta_0) / sqrt( 1.0 - 2.0*mass/radius ); // MLCB34
	//std::cout << "Speed = " << speed << std::endl;
	mu = cos(theta_0);

    //initial assumptions
    curve.eclipse = false;
    curve.ingoing = false;
    curve.problem = false;
	
	/********************************************************/
    /* COMPUTE EMISSION TIME, PHASE BINS, AND LIGHT BENDING */
    /********************************************************/

    for ( unsigned int i(0); i < numbins; i++ ) { // opening For-Loop-1
        // SMM: Added an offset of phi_0
        // SMM: Time is normalized to the spin period so 0 < t_e < 1 
        // curve.t[i] = i/(1.0*numbins) + shift_t; (This is computed in Spot.cpp)
        phi_em.at(i) = phi_0 + (2.0*Units::PI) * curve.t[i]; // phi_em is the same thing as "phi" in PG; changes with t
        psi.at(i) = acos(cos(incl)*cos(theta_0) + sin(incl)*sin(theta_0)*cos(phi_em.at(i))); // PG1; this theta_0 is the theta_0 from spot.cpp
        /*if (i == 0) {
        std::cout << "\ncos(incl) = " << cos(incl) << std::endl;
        std::cout << "cos(theta_0) = " << cos(theta_0) << std::endl;
        //std::cout << "cos(incl)*cos(theta_0) = " << cos(incl)*cos(theta_0) << std::endl;
        std::cout << "sin(incl) = " << sin(incl) << std::endl;
        std::cout << "sin(theta_0) = " << sin(theta_0) << std::endl;
        std::cout << "cos(phi_em.at(i)) = " << cos(phi_em.at(i)) << std::endl;
        //std::cout << "sin(incl)*sin(theta_0)*cos(phi_em.at(i)) = " << sin(incl)*sin(theta_0)*cos(phi_em.at(i)) << std::endl;
        //std::cout << "cos(incl)*cos(theta_0) + sin(incl)*sin(theta_0)*cos(phi_em.at(i)) = " << cos(incl)*cos(theta_0) + sin(incl)*sin(theta_0)*cos(phi_em.at(i)) << std::endl;
        std::cout << "acos(above) = " << acos(cos(incl)*cos(theta_0) + sin(incl)*sin(theta_0)*cos(phi_em.at(i))) << std::endl;
        }*/
    } // closing For-Loop-1
	
	
    for ( unsigned int i(0); i < numbins; i++ ) { // opening For-Loop-2
        int sign(0);
        double bval(0.0);
        bool result(false);
        double b1(0.0), b2(0.0), psi1(0.0), psi2(0.0);
        double xb(0.0);
        int j(0), k(0);
        
        /**************************************************************************/
		/* TEST FOR VISIBILITY FOR EACH VALUE OF b, THE PHOTON'S IMPACT PARAMETER */
		/**************************************************************************/

        if ( psi.at(i) < curve.defl.psi_max ) {
            if ( psi.at(i) > curve.defl.psi_b[j] )
	            while ( psi.at(i) > curve.defl.psi_b[j] ) {
	                j++;      
                }
            else {
	            while ( psi.at(i) < curve.defl.psi_b[j] ) {
	              j--;
	            }
	           j++;
            }
      
            b1 = curve.defl.b_psi[j-1];
            b2 = curve.defl.b_psi[j];
            psi1 = curve.defl.psi_b[j-1];
            psi2 = curve.defl.psi_b[j];
            k = j - 2;
      
            if ( j == 1 ) k = 0;
            if ( j == 3 * NN ) k = 3 * NN - 3;

            for ( j = 0; j < 4; j++ ) {
	            b_k.at(j) = curve.defl.b_psi[k+j];
	            psi_k.at(j) = curve.defl.psi_b[k+j];
            }
  
            // 4-pt interpolation to find the correct value of b given psi.
            xb = psi.at(i);
            b_guess = (xb-psi_k.at(1))*(xb-psi_k.at(2))*(xb-psi_k.at(3))*b_k.at(0)/
	                  ((psi_k.at(0)-psi_k.at(1))*(psi_k.at(0)-psi_k.at(2))*(psi_k.at(0)-psi_k.at(3)))
	                  +(xb-psi_k.at(0))*(xb-psi_k.at(2))*(xb-psi_k.at(3))*b_k.at(1)/
	                  ((psi_k.at(1)-psi_k.at(0))*(psi_k.at(1)-psi_k.at(2))*(psi_k.at(1)-psi_k.at(3)))
	                  +(xb-psi_k.at(0))*(xb-psi_k.at(1))*(xb-psi_k.at(3))*b_k.at(2)/
	                  ((psi_k.at(2)-psi_k.at(0))*(psi_k.at(2)-psi_k.at(1))*(psi_k.at(2)-psi_k.at(3)))
	                  +(xb-psi_k.at(0))*(xb-psi_k.at(1))*(xb-psi_k.at(2))*b_k.at(3)/
                	  ((psi_k.at(3)-psi_k.at(0))*(psi_k.at(3)-psi_k.at(1))*(psi_k.at(3)-psi_k.at(2)));
        } // ending psi.at(i) < curve.defl.psi_max
        
    	//std::cout << "psi_max = " << curve.defl.psi_max * 180.0 / Units::PI << std::endl;
        //std::cout << "psi = " << psi.at(i) << " b_guess = " << b_guess << std::endl;
        
        /***********************************************/
		/* FINDING IF A SOLUTION EXISTS, SETTING FLAGS */
		/***********************************************/
		
        result = defltoa->b_from_psi( fabs(psi.at(i)), mu, bval, sign, curve.defl.b_max, 
        		 curve.defl.psi_max, b_guess, fabs(psi.at(i)), b2, fabs(psi.at(i))-psi2, 
        		 &curve.problem );
        if ( result == false ) { 
            /*std::cout << "i = " << i 
                        << ", Not visible at phi_em = " << 180.0 * phi_em.at(i) / Units::PI
      	                << " (deflection not available)." << std::endl; */
            curve.visible[i] = false;
            curve.t_o[i] = curve.t[i] ;
            curve.dOmega_s[i] = 0.0;
            curve.eclipse = true;
        }
		
        else { // there is a solution
            b = bval;
            if ( sign < 0 ) { // if the photon is initially ingoing (only a problem in oblate models)
	            ingoing = true;
	            curve.ingoing = true;
	            //std::cout << "ingoing!"<< std::endl;
            }
            else if ( sign > 0 ) {
	            ingoing = false;
            }
            else {
	            throw( Exception("Chi.cpp: sign not returned as + or - with success.") ); // used to say "ObFluxApp.cpp"
            }
            
			double b_maximum = radius/sqrt(1.0 - 2.0*mass/radius);
			if ( (fabs(b-b_maximum) < 1e-7) && (b > 0.0) && (b > b_maximum) ) { 
			// this corrects for b being ever so slightly over bmax, which yields all kinds of errors in OblDeflectionTOA
				std::cout << "Setting b = b_max." << std::endl;
				b = b_maximum - DBL_EPSILON;
			}
            curve.b[i] = b/radius;
            
            /*******************************************************/
			/* IF A SOLUTION EXISTS, CALCULATING ANGLES:  sinalpha */
		    /*                    						  cosalpha */
		    /*                    						  alpha    */
		    /*                   						  cosdelta */
		    /*                							  cosbeta  */
			/*******************************************************/

            sinalpha =  b * sqrt( 1.0 - 2.0 * mass / radius ) / radius;  // PG4, reconfigured
            cosalpha = sqrt( 1.0 - sinalpha * sinalpha ); 
            alpha    = asin( sinalpha );
            
            if (std::isnan(sinalpha) || sinalpha == 0.0) std::cout << "sinalpha = " << sinalpha << " at i = " << i << std::endl;
            if (std::isnan(cosalpha) || cosalpha == 0.0) std::cout << "cosalpha = " << cosalpha << " at i = " << i << std::endl;
            if (std::isnan(alpha) || alpha == 0.0) std::cout << "alpha = " << alpha << " at i = " << i << std::endl;
			
			//std::cout << "\ni = " << i << std::endl;
			//std::cout << "sinalpha = " << sinalpha << std::endl;
			//std::cout << "cosalpha = " << cosalpha << std::endl;
			//std::cout << "b = " << b << std::endl;

			
            if ( sign < 0 ) { // alpha is greater than pi/2
	            alpha = Units::PI - alpha;
            }

            cosdelta.at(i) =  (cos(incl) - cos(theta_0)*cos(psi.at(i))) / (sin(theta_0)*sin(psi.at(i))) ;
            if ( theta_0 == 0.0 )  // cosdelta needs to be redone if theta_0 = 0
            	cosdelta.at(i) = sqrt( 1 - pow( sin(incl) * sin(phi_em.at(i)) / sin(psi.at(i)) ,2) ); // law of sines from MLCB Fig 1 and in MLCB17 and just above
     
            if ( (cos(theta_0) < 0) && (cos(incl) < 0 ) ) {
	            cosdelta.at(i) *= -1.0;
            }
            if ( sin(psi.at(i)) != 0.0 ) {
	            curve.cosbeta[i] = cosalpha * cosgamma + sinalpha * sqrt( 1.0 - pow( cosgamma, 2.0 )) * cosdelta.at(i);
	            //if( std::isnan(curve.cosbeta[i]) ) std::cout << "cosdelta.at(i="<<i<<") = " << cosdelta.at(i) << std::endl;
            }
            else {
	            curve.cosbeta[i] = cosalpha * cosgamma;
            }

            if ( cosalpha < 0.01 ) {
	            curve.cosbeta[i] = (Units::PI/2.0 - alpha + sqrt(2) * sqrt(1.0-cosgamma) * cosdelta.at(i));
  			}
            if ( curve.cosbeta[i] < 0.0 && curve.cosbeta[i] > 1.0 ) { // cosbeta > 1.0 added by Abbie, Feb 22 2013
	            std::cerr << "i = " << i
	        	          << ", Not visible at phi_em = " << 180.0 * phi_em.at(i) / Units::PI 
	                   	  << ", cos(beta) = " << curve.cosbeta[i] 
		                  << ", cos(alpha) = " << cosalpha
		                  << ", cos(gamma) = " << cosgamma
		                  << ", cos(delta) = " << cosdelta.at(i)
		                  << " (visibility condition)." << std::endl << std::endl;
	            curve.visible[i] = false;
            }
            else {
	            curve.visible[i] = true;
            }
			
			/********************************************************/
			/* IF THE PHOTON IS VISIBLE, COMPUTE: dpsi_db           */     
			/*                                    toa               */
			/*									  cosxi             */
			/*									  eta               */
			/*									  t_o               */
			/*									  psi               */
			/*									  R_dpsi_db         */
			/*									  dcosalpha_dcospsi */
			/*									  dOmega_s          */
			/********************************************************/
			
            if ( curve.visible[i] ) { // visible 
            	//if (alpha == 0.0 && psi.at(i) == 0.0 & phi_em.at(i) == 0.0) 
                if (alpha == 0.0 && psi.at(i) == 0.0) 
            		cosxi.at(i) = 0.0; // to avoid NAN errors from dividing by 0; appears when incl = theta at i=0
	            else 
	            	cosxi.at(i) = - sinalpha * sin(incl) * sin(phi_em.at(i)) / sin(fabs(psi.at(i)));  // PG11
	            curve.eta[i] = sqrt( 1.0 - speed*speed ) / (1.0 - speed*cosxi.at(i) ); // Doppler boost factor, MLCB33
	            //std::cout << "eta(i="<<i<<") = " << curve.eta[i] << std::endl;
                //std::cout << cosxi.at(i) << " " << alpha << " " << psi.at(i) << " " << phi_em.at(i) << std::endl;
	            //std::cout << sinalpha << " " << sin(incl) << " " << sin(phi_em.at(i)) << " " << sin(fabs(psi.at(i))) << std::endl;

	            if ((1.0 - speed*cosxi.at(i)) == 0.0) 
	            	std::cout << "dividing by zero" << std::endl;
	            if((std::isnan(speed) || speed == 0.0) && theta_0 != 0.0 )
	            	std::cout << "speed = " << speed << " at i = " << i << std::endl;
	            //if(std::isnan(cosxi.at(i)) || cosxi.at(i) == 0.0) 
	            //	std::cout << "cosxi(i="<<i<<") = " << cosxi.at(i) << std::endl;
	            

	            if ( ingoing ) {
	             //  std::cout << "Ingoing b = " << b << std::endl;
	                dpsi_db_val = defltoa->dpsi_db_ingoing( b, mu, &curve.problem );
	                toa_val = defltoa->toa_ingoing( b, mu, &curve.problem );
	            }
                else {
	                //std::cout << "Outgoing b = " << b << std::endl;
	                //double b_maximum = radius/sqrt(1.0 - 2.0*mass/radius); //this had get_mass() instead of mass originally in OblDeflectionTOA.cpp
	                
	                //std::cout << "bmax = " << b_maximum << std::endl;
	                //std::cout << "|b - bmax| = " << fabs(b-b_maximum) << std::endl;
	             	//if ( fabs(b-b_maximum) < 1e-9 && b > 0.0 ) b = b_maximum - 1e-9;
	                dpsi_db_val = defltoa->dpsi_db_outgoing( b, radius, &curve.problem );
	                //if (i == 0) std::cout << "b = " << b <<", dpsi_db = " << dpsi_db_val << std::endl;

	                toa_val = defltoa->toa_outgoing( b, radius, &curve.problem );
	                //std::cout << "dpsi_db_val = " << dpsi_db_val << ", toa_val = " << toa_val << std::endl;
	            }

	            //std::cout << "Done computing TOA " << std::endl;
	            curve.t_o[i] = curve.t[i] + (omega * toa_val) / (2.0 * Units::PI);
	            curve.psi[i] = psi.at(i);
	            curve.R_dpsi_db[i] = dpsi_db_val * radius;
				if ( psi.at(i) == 0 && alpha == 0 ) 
					curve.dcosalpha_dcospsi[i] = fabs( (1.0 - 2.0 * mass / radius) / curve.R_dpsi_db[i]);
				//if (psi.at(i) == 0 && alpha == 0 ) curve.dcosalpha_dcospsi[i] = 0.0;
	            else 
	            	curve.dcosalpha_dcospsi[i] = fabs( sinalpha/cosalpha * sqrt(1.0 - 2.0*mass/radius) / (sin(fabs(psi.at(i))) * curve.R_dpsi_db[i]) );
	            curve.dOmega_s[i] = (dS / (distance * distance)) 
	                               * (1.0 / (1.0 - 2.0 * mass / radius)) 
	                               * curve.cosbeta[i] 
	                               * curve.dcosalpha_dcospsi[i];  // PG8
	            //if (i == 0) std::cout << "CHI dOmega = " << curve.dOmega_s[i] << std::endl;

	            /***********************************/
				/* FLAGS IF A VALUE IS NAN OR ZERO */
				/***********************************/
	            if (std::isnan(dpsi_db_val) || dpsi_db_val == 0) std::cout << "dpsi_db_val = " << dpsi_db_val << "at i = " << i << std::endl;
				if (std::isnan(psi.at(i))) std::cout << "psi.at(i="<<i<<") = " << psi.at(i) << std::endl;
				if (std::isnan(curve.dOmega_s[i])) std::cout << "dOmega is NAN at i = " << i << std::endl;
				if (std::isnan(curve.cosbeta[i])) std::cout << "cosbeta is NAN at i = " << i << std::endl;
				if (std::isnan(curve.dcosalpha_dcospsi[i])) std::cout << "dcosalpha_dcospsi is NAN at i = " << i<< std::endl;
				if (std::isnan(sinalpha)) std::cout << "sinalpha is NAN at i = " << i << std::endl;
				if (std::isnan(cosalpha)) std::cout << "cosalpha is NAN at i = " << i << std::endl;
				if (std::isnan(psi.at(i))) std::cout << "psi is NAN at i = " << i << std::endl;
				if (std::isnan(mass)) std::cout << "mass is NAN at i = " << i << std::endl;
				if (std::isnan(radius)) std::cout << "radius is NAN at i = " << i << std::endl;
				


	         //    std::cout << "t  = " << curve.t[i] 
	           //  	         << " dcosalpha/dcospsi = " << curve.dcosalpha_dcospsi[i]
	             //	         << "  = " << sinalpha/cosalpha
	             	//         << std::endl;

            } // end visible
            
            /**************************************************************/
			/* IF THE PHOTON IS NOT VISIBLE, POLITELY CRASH OR GO TO ZERO */
			/**************************************************************/
      
            else { // not visible; we think that it shouldn't matter if it's not visible at i=0
	        	//if( i == 0 ) {
	            //	std::cerr << "Not visible at i = 0, which I can't handle yet." << std::endl;
	            //	curve.problem = true;
	            //	return curve;
	        	//}
	        	//else {
	            	//phi_o.at(i) = phi_o.at(i-1) + (2.0*Units::PI/numbins); // commented out since phase and time are treated the same now
	            	curve.t_o[i] = curve.t[i] ; // t_o is not defined properly, so we'll set it to emission time
	            	curve.dOmega_s[i] = 0.0;    // don't see the spot, so dOmega = 0
	            	curve.cosbeta[i] = 0.0;     // doesn't matter, doesn't enter into calculation
	            	curve.eta[i] = 1.0;	        // doesn't matter, doesn't enter into calculation
	            //}
            } // end not visible
        } // end "there is a solution"
    }  // closing For-Loop-2

    //  std::cout << "Done ComputeAngles " << std::endl;
    
/*    std::cout << "Phi_em: " << phi_em.at(numbins-2)*180/Units::PI << std::endl;
    std::cout << "Phi_em: " << phi_em.at(numbins-1)*180/Units::PI << std::endl;
    std::cout << "Psi: " << psi.at(numbins-2) << std::endl;
    std::cout << "Psi: " << psi.at(numbins-1) << std::endl;
    std::cout << "dcosalpha_dcospsi: " << curve.dcosalpha_dcospsi[numbins-2] << std::endl;
    std::cout << "dcosalpha_dcospsi: " << curve.dcosalpha_dcospsi[numbins-1] << std::endl;
    std::cout << "cosdelta: " << cosdelta.at(numbins-2) << std::endl;
    std::cout << "cosdelta: " << cosdelta.at(numbins-1) << std::endl;
    std::cout << "dOmega: " << curve.dOmega_s[numbins-2] << std::endl;
    std::cout << "dOmega: " << curve.dOmega_s[numbins-1] << std::endl;
    std::cout << "eta: " << curve.eta[numbins-2] << std::endl;
    std::cout << "eta: " << curve.eta[numbins-1] << std::endl; 
    std::cout << "cosxi: " << cosxi.at(numbins-2) << std::endl;
    std::cout << "cosxi: " << cosxi.at(numbins-1) << std::endl;*/

    return curve;

} // End ComputeAngles



/**************************************************************************************/
/* ComputeCurve:                                                                      */
/*              computes the flux of each light curve                                 */
/*																					  */
/* pass: angles = all the angles necessary to compute the flux correctly;             */
/*                computed in the routine/method/function above [radians or unitless] */
/**************************************************************************************/
class LightCurve ComputeCurve( class LightCurve* angles ) {
	
	/******************************************/
	/* VARIABLE DECLARATIONS FOR ComputeCurve */
	/******************************************/
	
    class LightCurve curve;

    std::ifstream input;
    std::ofstream output;


    bool infile_is_set;   // If the user has specified an input file
    double //Gamma,              // Currently unused; used in some of the models
           //aniso,              // Anisotropy parameter
           mass,               // Mass of the star, in M_sun
           radius,             // Radius of the star at the spot, in km
           temperature,        // Temperature of the spot, in keV
           E_mono,             // Energy observed, in keV, for each monochromatic energy
           //Gamma1,             // Currently unused; used in some of the spectral models
           //Gamma2,             // Currently unused; used in some of the spectral models
           //Gamma3,             // Currently unused; used in some of the spectral models
           //bbrat,              // Ratio of effect of blackbody to comptonization
           E_band_lower_1,     // Lower bound of energy band for flux integration, in keV
           E_band_upper_1,     // Upper bound of energy band for flux integration, in keV
           E_band_lower_2,     // Lower bound of energy band for flux integration, in keV
           E_band_upper_2,     // Upper bound of energy band for flux integration, in keV
           redshift,           // Gravitational redshift = 1 + z = (1-2M/R)^{-1/2}
           bolo,               // Bolometric flux; bolo = sigma T^4/pi
           gray(1.0);          // Graybody factor (when = 1, not effective)
        
    unsigned int numbins(MAX_NUMBINS);  // Time bins of light curve (usually 128)
        
    std::vector< double > totflux(MAX_NUMBINS, 0.0); // integrated flux
    std::vector< double > softbb(MAX_NUMBINS, 0.0);  // blackbody soft flux
    std::vector< double > softcm(MAX_NUMBINS, 0.0);  // compton soft flux

    // double softbbave(0.0), softcmave(0.0), hardave(0.0);  // average value of softbb and softcm and high energy band

	/*********************/
    /* SETTING THINGS UP */
    /*********************/
    
    // One monochromatic energy, is set a few lines below

    //output.open(out_file); 

    curve = (*angles);
    //Gamma1 = curve.para.Gamma1;
    //aniso = curve.para.aniso;                   // who knows
    mass = curve.para.mass;                     // unitless
    radius = curve.para.radius;                 // unitless
    temperature = curve.para.temperature;       // T in keV 
    infile_is_set = curve.flags.infile_is_set;
    numbins = curve.numbins;
    E_band_lower_1 = curve.para.E_band_lower_1;     // in keV
    E_band_upper_1 = curve.para.E_band_upper_1;     // in keV
    E_band_lower_2 = curve.para.E_band_lower_2;     // in keV
    E_band_upper_2 = curve.para.E_band_upper_2;     // in keV
    E_mono = curve.para.E_mono;                     // in keV

    redshift = 1.0 / sqrt( 1 - 2.0 * mass / radius);


    if ( curve.flags.atmosphere == 1 ){
        Read_NSX(curve.para.temperature, curve.para.mass, curve.para.radius);     // Reading NSX File
        //cout << "Using helium atmosphere" << endl;
    }
    else{
        Read_NSATMOS(curve.para.temperature, curve.para.mass, curve.para.radius); // Reading NSATMOS FILES Files
        //cout << "Using hydrogen atmosphere" << endl;
    }
    
    if ( curve.flags.beaming_model == 1){
        if (curve.flags.atmosphere == 1){
            cout << "Using helium atmosphere" << endl;
        }
        else{
        cout << "Using hydrogen atmosphere" << endl;            
        }
    }
    else{
        cout << "Using blackbody model" << endl;
    }
    

    //bolo = 2.0e12/15.0 * pow(Units::H_PLANCK,-3) * pow(Units::C,-2) * pow( Units::PI * Units::EV * temperature, 4);
    //bolo *= 1.0e-3/Units::EV;

    bolo = 2.0e9 * 2.404 * Units::C * pow(temperature * Units::EV/(Units::H_PLANCK * Units::C) , 3); // use this one! probably!
    // the e9 in the beginning is for changing T^3 from keV to eV
    // 2.404 comes from evaluating Bradt equation 6.17 (modified, for photon number count units), using the Riemann zeta function for z=3

    for ( unsigned int i(0); i < numbins; i++ ) { // doing the bolometric flux
        //std::cout << curve.dOmega_s[i] << std::endl; // Note: dOmega should NEVER be negative.

        if ( curve.dOmega_s[i] != 0.0 ) {
            
            if ( curve.flags.beaming_model == 1 ) {
            	//std::cout << "graybody " << std::endl;
	            //gray = Gray(curve.cosbeta[i]*curve.eta[i]); // gets the graybody factor
                gray = 1.0; // Graybody factor not used in atmospheric models
	        }
            else
	            gray = 1.0; // Isotropic, the same at all angles
 			
 			/*******************************************/
   			/* COMPUTING BOLOMETRIC LIGHT CURVE, p = 0 */
   			/* Units: photons/(cm^2 s)                 */
    		/*******************************************/
    		
            // Bolometric Light Curve for energy flux erg/(cm^2 s)
            //curve.f[0][i] =  bolo * gray * curve.dOmega_s[i] * pow(curve.eta[i],5) * pow(redshift,-4); // Units: erg/(cm^2 s)

            // Bolometric Light Curve for photon number flux photons/(cm^2 s)
            
            if ( curve.flags.beaming_model == 1 ) {
                curve.f[0][i] = curve.dOmega_s[i] * pow(curve.eta[i],4) * pow(redshift,-3) * AtmosEBandFlux(curve.flags.atmosphere, curve.cosbeta[i]*curve.eta[i], 0.05, 15); // Units: photon/(s cm^2)
                //if the log(T)<5.65, change the upper boundary to 14.6754! The hydrogen model of log(T) = 5.55 gives spectra up to this energy.
            }
            else
                curve.f[0][i] = bolo * gray * curve.dOmega_s[i] * pow(curve.eta[i],4) * pow(redshift,-3); // Units: photons/(cm^2 s)
            //curve.f[0][i] = 0; //set to 0 for now. Probably not useful for our purpose.

            /*if (i <= 2) { // for troubleshooting
            std::cout << "i = " << i << std::endl;
            std::cout << "flux = " << curve.f[0][i] << std::endl;
            std::cout << "bolo = " << bolo << std::endl;
            //std::cout << "gray = " << gray << std::endl;
            std::cout << "dOmega_s = " << curve.dOmega_s[i] << std::endl;
            std::cout << "eta = " << curve.eta[i] << std::endl;
            //std::cout << "redshift = " << redshift << std::endl;
            std::cout << std::endl;
            }*/
            
		//	if (std::isnan(gray) || gray == 0) std::cout << "gray = " << gray << std::endl;
		//	if (std::isnan(curve.dOmega_s[i]) || curve.dOmega_s[i] == 0) std::cout << "dOmega is NAN or 0." << std::endl;
			if (std::isnan(curve.eta[i]) || curve.eta[i] == 0) std::cout << "eta[i=" << i << "] = " << curve.eta[i] << std::endl;
			if (std::isnan(redshift) || redshift == 0) std::cout << "redshift = " << redshift << std::endl;
			//if (i == 28 || i == 29) { std::cout << "Eta [i="<<i<<"]= " << curve.eta[i] << std::endl;
			//std::cout << "domega [i="<<i<<"]= " << curve.dOmega_s[i] << std::endl; }
	        /*******************************************************************************/
   			/* COMPUTING LIGHT CURVE FOR MONOCHROMATIC ENERGY BLACKBODY OR WITH ATMOSPHERE */
   			/*      First computes in [erg/(s cm^2 Hz), converts to                        */
   			/*		photons/(s cm^2 keV)                                                   */
    		/*******************************************************************************/
    		
            // Moonochromatic light curve in energy flux erg/(s cm^2 Hz)
            if ( curve.flags.atmosphere == 1 ){
                //helium atmosphere
                curve.f[1][i] = curve.dOmega_s[i] * pow(curve.eta[i],4) * pow(redshift,-3) * Helium(E_mono * redshift/curve.eta[i], curve.cosbeta[i]*curve.eta[i]);
                curve.f[1][i] *= (1.0 / ( E_mono * Units::H_PLANCK ));// Units: photons/(s cm^2 keV)
                //H_spec << i << "   " << E_he << "   " << curve.f[1][i] << " " << Helium(E_he * redshift/curve.eta[i], curve.cosbeta[i]*curve.eta[i]) << endl;
            }
            else{
                //hydrogen atmosphere
                curve.f[1][i] = curve.dOmega_s[i] * pow(curve.eta[i],4) * pow(redshift,-3) * Hydrogen(E_mono * redshift/curve.eta[i], curve.cosbeta[i]*curve.eta[i]);
                curve.f[1][i] *= (1.0 / ( E_mono * Units::H_PLANCK ));
                if (std::isnan(curve.f[1][i])) std::cout << "raw flux E_mono = " << curve.f[1][i] << " at bin " << i << std::endl;
                //H_spec << i << "   " << E_hyd << "   " << curve.f[1][i] << " " << Hydrogen(E_hyd * redshift/curve.eta[i], curve.cosbeta[i]*curve.eta[i]) << endl;
            }

            if ( curve.flags.beaming_model == 0 ) {
                curve.f[1][i] = gray * curve.dOmega_s[i] * pow(curve.eta[i],4) * pow(redshift,-3) * BlackBody(temperature,E_mono*redshift/curve.eta[i]); // Units: erg/(s cm^2 Hz)
                curve.f[1][i] *= (1.0 / ( E_mono * Units::H_PLANCK )); // Units: photons/(s cm^2 keV)
                if (std::isnan(curve.f[1][i])) std::cout << "raw flux E_mono = " << curve.f[1][i] << " at bin " << i << std::endl;
            }

	        //curve.f[1][i] = gray * curve.dOmega_s[i] * pow(curve.eta[i],4) * pow(redshift,-3) * BlackBody(temperature,E_mono*redshift/curve.eta[i]); // Units: erg/(s cm^2 Hz)
	        //Convert to photons/(s cm^2 keV)
	        //DON'T DO THIS!: curve.f[j][i] *= 1.0/(E_mono[j]*(redshift/curve.eta[i]) * Units::H_PLANCK); // DON'T DO THAT!
	        //curve.f[1][i] *= (1.0 / ( E_mono * Units::H_PLANCK )); // Units: photons/(s cm^2 keV)
            
            // multiplying by dOmega_s elsewhere -- wait no i'm not? i'm multiplying by it here. this isn't a problem anymore?
            
            /***************************************************/
   			/* COMPUTING PHOTON NUMBER FLUX FOR AN ENERGY BAND */
   			/*   	1 < p < NCURVES-1                          */
   			/*      Units: photons/(s cm^2)                    */
    		/***************************************************/
    		
    		// First energy band
            if ( curve.flags.beaming_model == 0 ) {
                curve.f[NCURVES-2][i] = gray * curve.dOmega_s[i] * pow(curve.eta[i],4) * pow(redshift,-3) * EnergyBandFlux(temperature, E_band_lower_1*redshift/curve.eta[i], E_band_upper_1*redshift/curve.eta[i]); // Units: photon/(s cm^2)
                //cout << "bb used" << endl;
            }
            else{
                curve.f[NCURVES-2][i] = curve.dOmega_s[i] * pow(curve.eta[i],4) * pow(redshift,-3) * AtmosEBandFlux(curve.flags.atmosphere, curve.cosbeta[i]*curve.eta[i], E_band_lower_1*redshift/curve.eta[i], E_band_upper_1*redshift/curve.eta[i]); // Units: photon/(s cm^2)        
                //cout << curve.f[NCURVES-2][i] << endl;
                if (std::isnan(curve.f[NCURVES-2][i]) || curve.f[NCURVES-2][i] == 0) std::cout << "raw flux band 1 = " << curve.f[NCURVES-2][i] << " at bin " << i << std::endl;
                //cout << pow(redshift,-1) * AtmosEBandFlux(curve.flags.atmosphere, curve.cosbeta[i], E_band_lower_1*redshift/curve.eta[i], E_band_upper_1*redshift/curve.eta[i]) << " " << temperature << endl;
                //cout << E_band_lower_1*redshift/curve.eta[i] << " " << E_band_upper_1*redshift/curve.eta[i] << endl;
                //cout << pow(curve.eta[i],4) << endl;
            }
            //curve.f[NCURVES-2][i] = 0;

            // Second energy band
            
            if ( curve.flags.beaming_model == 0 ) {
                curve.f[NCURVES-1][i] = gray * curve.dOmega_s[i] * pow(curve.eta[i],4) * pow(redshift,-3) * EnergyBandFlux(temperature, E_band_lower_2*redshift/curve.eta[i], E_band_upper_2*redshift/curve.eta[i]); // Units: photon/(s cm^2)
            }
            else{
                curve.f[NCURVES-1][i] = curve.dOmega_s[i] * pow(curve.eta[i],4) * pow(redshift,-3) * AtmosEBandFlux(curve.flags.atmosphere, curve.cosbeta[i]*curve.eta[i], E_band_lower_2*redshift/curve.eta[i], E_band_upper_2*redshift/curve.eta[i]); // Units: photon/(s cm^2)
                if (std::isnan(curve.f[NCURVES-1][i]) || curve.f[NCURVES-1][i] == 0) std::cout << "raw flux band 2 = " << curve.f[NCURVES-1][i] << " at bin " << i << std::endl;
                //cout << curve.cosbeta[i]*curve.eta[i] << endl;
            }
            
            //curve.f[NCURVES-1][i] = 0;

   			//if ( i == 1 ) std::cout << "Energy band flux: " << curve.f[0][i] << std::endl;
			//if (std::isnan(curve.f[NCURVES-1][i]) || curve.f[NCURVES-1][i] == 0) std::cout << "curve.f band is NAN or 0." << std::endl;
			
        }
        else { // if curve.dOmega_s[i] == 0.0
            for ( unsigned int p(0); p < NCURVES; p++) {
	            curve.f[p][i] = 0.0;
	            //std::cout << curve.f[p][i] << std::endl;
	        }
        }
   		
   		//if (std::isnan(curve.f[NCURVES-1][i]) || curve.f[NCURVES-1][i] == 0) std::cout << "curve.f band is NAN or 0." << std::endl;
	
    } // ending the for(i) loop
    
    //std::cout << "dOmega = " << curve.dOmega_s[0] << std::endl;
	//std::cout << "\ncurve.f[0][0] = " << curve.f[0][0] << std::endl;
	
    /***********************************************************/
   	/* DEALING WITH THE TIME DELAYS, REBINNING THE LIGHT CURVE */
   	/* This is where the jumpy problems tend to be.            */
    /***********************************************************/
    		
    if ( !curve.flags.ignore_time_delays ) { // if we are not ignoring the time delays        
        int k(0), j(0); // index placeholders; approximately, k is i+1 and j is i-1
                        // but time delays mean that j isn't always i-1
                        // used in the linear interpolation
        
        /********************************/
   		/* LOOP THROUGH THE LIGHTCURVES */
    	/********************************/
    	
        for (unsigned int p(0); p < NCURVES; p++) {
        	
        	/*********************/
   			/* MORE DECLARATIONS */
    		/*********************/
    		
	    	std::vector< double > newflux(MAX_NUMBINS, 0.0);                  // rebinned flux (to account for photon travel time)
            unsigned int imax(0), imin(0);                                    // index of element with maximum flux value, minimum flux value
            double max_discrete_flux(0.0), min_discrete_flux(curve.f[p][0]);  // maximum flux value and minimum flux value assigned to a grid point (discrete)
			double tx(0), ta(0), tb(0), tc(0);                                // a,b,c: three-point interpolation on a parabola for min and max areas (time a, time b, time c)
            double fa(0), fb(0), fc(0);                                       // corresponding fluxes for three-point interpolation
            int ia(0), ic(0);                                                 // indices in array for where flux is fa, fc
            double temporary1(0.0), temporary2(0.0);                          // makes math easier
            double maximum(0.0), minimum(100000.0);                           // true (continuous) maximum and minimum flux values
            double tmin(0.0);                                                 // value of t at the true minimum

			/**********************************************************/
   			/* START WITH FINDING THE MAXIMUM FLUX OF THE LIGHT CURVE */
    		/**********************************************************/
			
			/********************************************************/
   			/* FINDING THE DISCRETE MAXIMUM FLUX OF THE LIGHT CURVE */
    		/********************************************************/
    		
            for ( unsigned int i(0); i < numbins; i++ ) {
	            if ( curve.f[p][i] > max_discrete_flux ) { // tells you where the maximum is
	                imax = i;  
	                max_discrete_flux = curve.f[p][i];
	            }
	        }
			//if (p == NCURVES-1) std::cout << "Max discrete flux = " << max_discrete_flux << std::endl;
	        

            /******************************************/
   			/* FINDING THE TRUE MAXIMUM VALUE OF FLUX */
    		/******************************************/
    		
            if ( imax == 0 ) {
	            ia = numbins - 1;
	            ta = curve.t_o[ia] - 1.0;
            }
            else { // else imax == -1
	            ia = imax - 1;
	            ta = curve.t_o[ia];
            }
            tb = curve.t_o[imax];
            if ( imax == numbins - 1 ) {
	            ic = 0;
	            tc = curve.t_o[ic] + 1.0;
            }
            else { //else imax == 0
	            ic = imax + 1;
	            tc = curve.t_o[ic];
            }
            fa = curve.f[p][ia];
            fb = curve.f[p][imax];
            fc = curve.f[p][ic];
            
           	/*if (p == 0) { 
           	std::cout << "ta: " << ta << std::endl;
            std::cout << "tc: " << tc << std::endl;
            std::cout << "tb: " << tb << std::endl;
            std::cout << "fa: " << fa << std::endl;
            std::cout << "fc: " << fc << std::endl;
            std::cout << "fb: " << fb << std::endl; 
            std::cout << "imax = " << imax << std::endl;
			}*/
            
            /**********************************************/
   			/* NUMERICAL RECIPES, PARABOLIC INTERPOLATION */
   			/* Equation 10.3.1 (our t is their x)         */
    		/**********************************************/
    		
            if ( ( (tb-ta)*(fb-fc) - (tb-tc)*(fb-fa) ) != 0.0 ) { // otherwise you get a big fat NAN for a flux
            	tx = tb - 0.5 * (pow(tb-ta,2)*(fb-fc) - pow(tb-tc,2)*(fb-fa)) / ((tb-ta)*(fb-fc) - (tb-tc)*(fb-fa));
            	temporary1 =  (fa-fc)/( pow(tc-tx,2) - pow(ta-tx,2)) ;
            }
            if ( ( (tb-ta)*(fb-fc) - (tb-tc)*(fb-fa) ) == 0.0 ) { // to avoid dividing by zero
/********/      tx = 0.0; // is this what it should be?
            }
            if ( ( pow(tc-tx,2) - pow(ta-tx,2) ) != 0.0 ) {
            	temporary1 =  (fa-fc)/( pow(tc-tx,2) - pow(ta-tx,2)) ;
            }
            if ( ( pow(tc-tx,2) - pow(ta-tx,2) ) == 0.0 ) {
            	temporary1 = 0.0;
            }
            maximum = fb - temporary1*pow(tb-tx,2);

            //if (p == NCURVES-1) std::cout << "max!  tx = " << tx << ", temp1 = " << temporary1 << ", true max = " << maximum << std::endl;

            /************************************************/
   			/* NOW FIND THE MINIMUM FLUX OF THE LIGHT CURVE */
   			/* Note: real light curves won't have eclipses  */
   			/* If eclipsed, then min = 0                    */
    		/************************************************/
    		
            if ( !curve.eclipse ){// && !curve.ingoing ) {   
            
            	/********************************************************/
   				/* FINDING THE DISCRETE MINIMUM FLUX OF THE LIGHT CURVE */
   				/* If not eclipsed                                      */
    			/********************************************************/
    			
	            for ( unsigned int i(0); i < numbins; i++) {
	                if (curve.f[p][i] < min_discrete_flux) {
	                    imin = i;
	                    min_discrete_flux = curve.f[p][i];
	                }
	            }
	            
	            /******************************************/
   				/* FINDING THE TRUE MINIMUM VALUE OF FLUX */
   				/* If not eclipsed                        */
    			/******************************************/
    			
	            if ( imin == 0 ) {
                    ia = numbins - 1;
	                ta = curve.t_o[ia] - 1.0;
	            }
	            else {  //else imin == 1
	                ia = imin - 1;
	                ta = curve.t_o[ia];
	            }
                tb = curve.t_o[imin];
                if ( imin == numbins-1 ) {
	                ic = 0;
	                tc = curve.t_o[ic]+1.0;
                }
                else {  //else imin == 2
	                ic = imin+1;
	                tc = curve.t_o[ic];
                }
                fa = curve.f[p][ia];
                fb = curve.f[p][imin];
                fc = curve.f[p][ic];
                
        		/*if (p == 0) {
        		std::cout << "ta: " << ta << std::endl;
        		std::cout << "tb: " << tb << std::endl;
        		std::cout << "tc: " << tc << std::endl;
        		std::cout << "fa: " << fa << std::endl;
        		std::cout << "fb: " << fb << std::endl;
        		std::cout << "fc: " << fc << std::endl;
        		std::cout << "imin = " << imin << std::endl;
        		}*/
        		
        		/**********************************************/
   				/* NUMERICAL RECIPES, PARABOLIC INTERPOLATION */
   				/* Equation 10.3.1 (our t is their x)         */
    			/**********************************************/

        		if ( ( (tb-ta)*(fb-fc) - (tb-tc)*(fb-fa) ) != 0.0 ) { // otherwise you get a big fat NAN for a flux
                	tmin = tb - 0.5*(pow(tb-ta,2)*(fb-fc) - pow(tb-tc,2)*(fb-fa)) / ((tb-ta)*(fb-fc) - (tb-tc)*(fb-fa));  
                	//if (p == NCURVES-1) std::cout << "tmin = " << tmin << std::endl;
            	}
            	if ( ( pow(tc-tmin,2) - pow(ta-tmin,2) ) != 0.0 ) {
            		temporary2 =  (fa-fc)/( pow(tc-tmin,2) - pow(ta-tmin,2)) ;
            		//if ( p == 0 ) std::cout << "temporary2 = " << temporary2 << std::endl;
            	}
           		if ( ( (tb-ta)*(fb-fc) - (tb-tc)*(fb-fa) ) == 0.0) {  // what should happen for dividing by zero?
            		tmin = 0.0;
           	 	}
           	 	if ( ( pow(tc-tmin,2) - pow(ta-tmin,2) ) == 0.0 ) { // what should happen for dividing by zero?
            		temporary2 = 0.0;
            	}
           	 	
           	 	minimum = fb - temporary2 * pow(tb-tmin,2);
           	 	//if ( p == 0 ) std::cout << "minimum = " << minimum << std::endl;

                /*if ( minimum < DBL_EPSILON ) {  // do not want this!! causes flux spikedowns near the min with certain inclinations
                    minimum = 0.0;
                }*/ 
                //if (p == NCURVES-1) std::cout << "min! tmin = " << tmin << ", temp2 = " << temporary2 << ", true min = " << minimum << std::endl;
            
            } // ending "not eclipsed, not ingoing" section
            
            /*******************************************/
   			/* FOR ECLIPSING LIGHT CURVES, MINIMUM = 0 */
    		/*******************************************/
    		
            else if ( curve.eclipse ) { //&& !curve.ingoing ) { // eclipsed -- it shouldn't matter if it's ingoing, it's still eclipsed!
                minimum = 0.0;
                //std::cout << "The numerical interpolation doesn't always behave properly when there is an eclipse in the light curve." << std::endl;
            } // ending "yes eclipsed, ingoing doesn't matter" section
            
            /****************************************************************************/
   			/* FOR NOT ECLIPSED AND YES INGOING PHOTONS, POLITELY SET TO ZERO AND CRASH */
    		/****************************************************************************/
    		
            else {    //if there is no eclipse and the photon is intially ingoing
                throw( Exception(" Photons are ingoing and not eclipsing. This is an issue that the code cannot handle.") );
                minimum = 0.0;
                maximum = 0.0;
                break;
            } // ending "not eclipsed, yes ingoing" section
            
            /****************************************************/
   			/* COMPUTING THE PULSE FRACTION FOR THE LIGHT CURVE */
            /****************************************************/
   			
            curve.minFlux[p] = minimum;
            curve.maxFlux[p] = maximum;
            curve.pulseFraction[p] = (curve.maxFlux[p] - curve.minFlux[p]) / (curve.maxFlux[p] + curve.minFlux[p]);

			// Initializing totflux
            for ( unsigned int i(0); i < numbins; i++ )
                totflux.at(i) = 0.0;
			
			
            /*if ( p == 3 ) std::cout << "comp_curve_prebinned curve.f[3][28] = " << curve.f[3][28] << std::endl;
            if ( p == 3 ) {
            	char prebinned_file[256]="run_data/05-Mar-2013_prebinned_flux_high_2.txt";
				std::ofstream prebinnedout;  // testing output stream;
				prebinnedout.open(prebinned_file, std::ios_base::trunc); // to append to the file
    			//if( prebinnedout.bad() || prebinnedout.fail() ){
    			//	throw (Exception("Prebinnedout failed."));
    			//	return -1;
    			//}
    			for (unsigned int i(0); i < numbins; i++)
    				prebinnedout <<curve.t[i] << "\t" << curve.f[p][i] << std::endl;
    			prebinnedout.close();
    		}
    		*/
    		
    		/**************************************************************/
   			/* ADDING FLUXES FROM ALL THE PHASE BINS AND OTHER FUN THINGS */
            /**************************************************************/

            for ( unsigned int i(0); i < numbins; i++ ) {  // for-i-loop, looping through the phase bins
            	//if (std::isnan(curve.f[NCURVES-1][i]) || curve.f[NCURVES-1][i] == 0) std::cout << "curve.f band is NAN or 0." << std::endl;
	            //if ( i <= 2 && p == 0 ) std::cout << "i = " << i << ", j = " << j << ", f at j = " << curve.f[p][j] << ", k = " << k << ", f at k = " << curve.f[p][k] << std::endl;

	            k = i + 1;
	            j = i;		 
                if ( k == static_cast<int>(numbins) ) 
                    k = 0;
	 			//if (p == 0) std::cout << "i = " << i << " j = " << j << " k = " << k << std::endl;
	           
	            if (curve.eclipse) {  // begin eclipse section
	                if (curve.f[p][k] == 0.0) {
	                    if (curve.f[p][j] != 0.0) {
	                        k = i;
	                        j = i-1;
	                    }
	                    else if (curve.f[p][j-1] != 0.0) {
		                    k = j - 1;
		                    j = j - 2;
	                    }
	                }
	                else {
	                    if ( curve.f[p][j] == 0.0 ){
	                        j = k;
	                        k = k + 1;
	                    }
	                } 
	             } // finished eclipse section

	            // Check to see if we're near the maximum
                if ( (i <= imax + 1 && i >= imax -1) || 
                     (imax == 0 && (i <= 1 || i == numbins-1)) || 
                     (imax == numbins-1 && (i==0 || i >= numbins-2))) { // parabolic interpolation near the maximum
	                if ( imax == numbins-1 && i == 0 ) {
	                    newflux.at(i) = maximum - temporary1 * pow(curve.t[i] + 1.0 - tx,2);  // intermediate value of flux
	                }
	                else {
	                    if ( imax == 0 && i == numbins-1 )
	                        newflux.at(i) = maximum - temporary1 * pow(curve.t[i] - 1.0 - tx,2);
	                    else
	                        newflux.at(i) = maximum - temporary1 * pow(curve.t[i] - tx,2);
	                }
	            }
	            
	            // Not near the maximum
	            else {
	            
	                // Check to see if we're near the minimum
	                if ( (!curve.eclipse) && (!curve.ingoing) && 
	                     ((i <= imin + 1 && i >= imin - 1) || 
	                     (imin == 0 && (i <= 1 || i == numbins-1)) || 
	                     (imin == numbins-1 && (i<=2 || i >= numbins-2)))) { // parabolic interpolation near the minimum
	                    //if ( p == 0 ) std::cout << "Near the minimum at i = " << i << std::endl;
	                    if ( i == numbins-1 ) 
	                    	newflux.at(i) = minimum - temporary2 * pow(curve.t[i] - 1 - tmin,2); // i've put this in here to eliminate the weird newflux jumping problem
	                    else {
	                    	newflux.at(i) = minimum - temporary2 * pow(curve.t[i] - tmin,2);
	                    	/*if ( p == 0 ) {
	                    		std::cout << "minimum = " << minimum << std::endl;
	                    		std::cout << "temporary2 = " << temporary2 << std::endl;
	                    		std::cout << "pow(curve.t[i]-tmin, 2) = " << pow(curve.t[i] - tmin,2) << std::endl;
	                    		std::cout << "newflux.at(i="<<i<<") = " << newflux.at(i) << std::endl;
	                    	}*/
	                    }
	                }
	                
	                // Not near the minimum
	                else {
	                	//std::cout << "In 2-pt interpolation" << std::endl;
	                    // 2 pt interpolation if not near the maximum
	                    double t1, t2;
	                    // Find point to the left of the "ith" point 
	                    // Time delays mean that j isn't always i-1
	                    if ( i == 0 ) {
	                    	j = numbins -1;
	                    	t1 = curve.t_o[j]-1.0;
	                    }
	                    else { // else4; if i != 0
	                        j = 0;
	                        while ( (curve.t_o[j] < curve.t[i]) && (j < static_cast<int>(numbins)) ) 
	                        	j++;
	                        j--;
	                        if ( j < 0 ) // because otherwise the computer gives us garbage and the flux looks ridiculous
	                        	j = numbins - abs(j);
	                        //if ( p == 0) std::cout << "i = " << i << ", j = " << j << ", t_o = " << curve.t_o[j] << ", t = " << curve.t[i] << std::endl;
	                        t1 = curve.t_o[j]; // time to the left of the time we're interested in
	                    }
                        // Find point to the right of the "ith" point
	                    if ( j == static_cast<int>(numbins) - 1 ) {
	                    	k = 0;
	                    }
	                    else { // else5; effectively, if i != 0 because that would make j != numbins-1
		                    k = j + 1;
		                    while (curve.t_o[k] <= curve.t[i] && k < static_cast<int>(numbins)) 
		                    	k++;
	                    }
	                    t2 = curve.t_o[k]; // time to the right of the point we're interested in
	                    newflux.at(i) = curve.f[p][j] + (curve.f[p][k]-curve.f[p][j])/(t2-t1) * (curve.t[i]-t1); // linear interpolation!
	                    //if ( i <= 2 && p == 0 ) std::cout << "i = " << i << ", newflux = " << newflux.at(i) << ", j = " << j << ", f at j = " << curve.f[p][j] << ", k = " << k << ", f at k = " << curve.f[p][k] << ", t2 - t1 = " << (t2-t1) << ", t at i = " << curve.t[i] << ", t1 = " << t1 << std::endl;

	            }// end else-not-near the minimum
	        	//if (std::isnan(curve.f[NCURVES-1][i])) std::cout << "curve.f band is NAN." << std::endl;

            }
            //if (p == 0) std::cout << "flux: " << curve.f[0][i] << " , newflux: " << newflux.at(i) << std::endl;
			//if (p == 0 && (i == 0 || i == 1) && imin >= 122) std::cout << "i = " << i << ", imin = " << imin << ", imax = " << imax << ", newflux = " << newflux.at(i) << std::endl;

	        /* newflux vs t_e corresponds to the new re-binned light curve.
	           It corresponds to the same light curve as bolflux vs t_o */

            if ( newflux.at(i) < 0.0 )
	            newflux.at(i) = 0.0;
	    	//if (std::isnan(curve.f[p][i])) std::cout << "curve.f band down here is NAN." << std::endl;

        } // closes the for(p) loop, going through the curves.
    	
    	/****************************************************************/
   		/* SUMMING THE FLUXES AT EACH PHASE BIN ACROSS ALL LIGHT CURVES */
        /****************************************************************/
    	
        for ( unsigned int i(0); i < numbins; i++ ) {
        	//std::cout << newflux.at(i) << std::endl;
            totflux.at(i) += newflux.at(i);   // setting totflux = newflux
            //if (i <= 2 && p == 0) std::cout << "curve.f[0][i="<<i<<"] = " << curve.f[0][i] << std::endl;

        }
      /*  if (p == 0) {
        	std::cout << "Newflux: " << newflux.at(numbins-2) << std::endl;
        	std::cout << "Newflux: " << newflux.at(numbins-1) << std::endl;
        }*/
     
        for ( unsigned int i(0); i < numbins; i++ ) {
            curve.f[p][i] = totflux.at(i);
            //if (i <= 2 && p == 0) std::cout << "curve.f[0][i="<<i<<"] = " << curve.f[0][i] << std::endl;
        }
        // only plotting versus evenly spaced time -- not using t_o
    
        }// end for-p-loop
    } // end time delay section
    
   /* for ( int i(0); i < numbins; i++ )
    	if(curve.f[NCURVES-1][i] == 0) std::cout << "curve.f is 0 at p="<<NCURVES-1 << ", i="<< i<<std::endl; */
    	
    //std::cout << "comp_curve_end curve.f[3][28] = " << curve.f[3][28] << std::endl;

    return curve;

} // end ComputeCurve


/**************************************************************************************/
/* Normalize:                                                                         */
/*           normalizes the fluxes to 1 by dividing by the average flux               */
/*           of each light curve													  */
/*																					  */
/* pass: Flux = the flux for each part of the light curve [units don't matter]        */
/*       numbins = number of phase bins the light curve is divided into [Unitless]    */
/**************************************************************************************/
class LightCurve Normalize ( double Flux[NCURVES][MAX_NUMBINS], unsigned int numbins ) {

	/***************************************/
   	/* VARIABLE DECLARATIONS FOR Normalize */
    /***************************************/
	
    class LightCurve newcurve;  // new curve structure to hold the normalized lightcurve flux values
    unsigned int i, p;          // loop variables
    double norm[NCURVES];       // normalization factor, one for each curve

    for ( p = 0; p < NCURVES; p++ ) {
        norm[p] = 0.0;
        for ( i = 0; i < numbins; i++ ) {
            norm[p] += Flux[p][i]; 
            newcurve.f[p][i] = Flux[p][i];
        }
        if ( norm[p] != 0.0) norm[p] /= (numbins*1.0); // makes norm the average value for each curve
    }
    //std::cout << "norm = " << norm[0] << std::endl;

    for ( p = 0; p < NCURVES; p++ ) {
        for ( i = 0; i < numbins; i++ ) {
        	//std::cout << norm[p] << std::endl;
            if ( norm[p] != 0.0)  newcurve.f[p][i] /= norm[p]; 
            else newcurve.f[p][i] = 1.0;
        }
    }
    
    //std::cout << "norm newcurve.f[3][28] = " << newcurve.f[3][28] << std::endl;
    return newcurve;
} // end Normalize


#define TINY 1.0e-10
#define NMAX 300
#define SWAP(a,b) {swap=(a);(a)=(b);(b)=swap;}
#define NDIM 5
#define MPTS 6

/**************************************************************************************/
/* recalc: 																			  */
/*        this does not get called in Spot.cpp										  */
/*																					  */
/* pass: curve = flux in the light curve?                                             */
/*       omega = spin frequency of the NS [unitless]                                  */
/*       mass = mass of the NS [unitless]                                             */
/*       theta_0 = angle from spin axis to center of spot bit [radians]               */
/*       rspot = radius of the NS at the spot [unitless]                              */
/**************************************************************************************/
class OblDeflectionTOA* recalc( class LightCurve* curve, double omega, double mass, 
                                double theta_0, double rspot ) {

    /************************************/
   	/* VARIABLE DECLARATIONS FOR recalc */
    /************************************/
    
    double mu,        // = cos(theta_0), unitless
	       cosgamma,  // Cos of the angle between the radial vector and surface normal vector, defined in MLCB13
	       req,       // Radius of the neutron star at the equator, in km (but maybe unitless here)
	       zeta,      // Defined in MLCB9
	       epsilon,   // Doppler boost factor, defined in MLCB33
	       P2,        // Legendre polynomial
	       P4,        // Legendre polynomial
	       b0,        // Coefficient in MLCB Table 1
	       b2,        // Coefficient in MLCB Table 1
	       b4,        // Coefficient in MLCB Table 1
	       c2,        // Coefficient in MLCB Table 1
	       c4;        // Coefficient in MLCB Table 1
	           
	//G = 1.32746E+11;
	mu = cos( theta_0 );
	//    pi = 4.0 * atan(1.0);
	zeta = mass / rspot; // Since G = c = 1, with mass in gravitational units
    epsilon = pow( rspot * omega, 2.0 ) / zeta;

    b0 = 0.18*epsilon - 0.23*zeta*epsilon + 0.18*pow( epsilon, 2 );
    b2 = 0.39*epsilon - 0.29*zeta*epsilon + 0.42*pow( epsilon, 2 );
    //b4 = -0.04*epsilon + 0.15*zeta*epsilon - 0.13*pow(epsilon, 2);
    b4 = -8.0/3.0 * (b0 - 0.5*b2);

    c2 = 0.60 * pow( epsilon, 2 );
    c4 = -0.12 * pow( epsilon, 2 );

    P2 = LegP2( mu );
    P4 = LegP4( mu );

    req = rspot * (1.0 + b0 + b2*P2 + b4*P4 + P2*(c2*P2 + c4*P4));
	/*
	std::cout << "recalc: theta_0 = " << theta_0*180.0/Units::PI;
    std::cout << "rspot_recalc = " << rspot <<std::endl;
    std::cout << "req_recalc = " << req << std::endl;
    */
    OblModelBase* model;
    model = new PolyOblModelNHQS( rspot, req,
				PolyOblModelBase::zetaparam(mass,req),
				PolyOblModelBase::epsparam(omega, mass, req)
				);

    cosgamma = model->cos_gamma( mu );
    curve->para.cosgamma = cosgamma;
    // defltoa is a structure that "points" to routines in the file "OblDeflectionTOA.cpp"
    // used to compute deflection angles and times of arrivals 
    
    class OblDeflectionTOA* dt;

    dt = new OblDeflectionTOA( model, mass );

    return (dt);

} // end recalc



/**************************************************************************************/
/* Hydrogen:                                                                          */
/*           computes the monochromatic blackbody flux in units of erg/cm^2           */
/*                                                                                    */
/*                                                                                    */
/**************************************************************************************/

// Bi-section search for a value in an array
int Find(double val, std::vector<double> array){
    
    bool found = false;
    int low = 0;
    int high = array.size();
    int mid = 0;
    
    while(low <= high){
        mid  = (low + high) / 2;   // midpoint
        if(array[mid] <= val && array[mid+1] >= val){
            found = true;
            break;
        }else if(val < array[mid]){
            high = mid - 1; // decrease the higher edge 1 step
        }else{
            low = mid + 1;  // increase the lower edge 1 step
        }
    }
    return mid;
}

// Linear interpolation
double Linear(double x,double Xa, double Ya, double Xb, double Yb){
    
    return  (Ya + ((Yb - Ya) * (x - Xa) / (Xb - Xa)));
}

// Linear interpolation in log-log space
double LogLinear(double x,double Xa, double Ya, double Xb, double Yb){
    
    return  (exp(log(Ya) + ((log(Yb) - log(Ya)) * (log(x) - log(Xa)) / (log(Xb) - log(Xa)))));
}

// Find and calculate Linear Interpolation
double Interpolate(double X_INT, std::vector<double> X, std::vector<double> Y){
    
    int RowNum = Find(X_INT,X);
    double d_X1 = X_INT - X[RowNum];
    double d_X2 = X[RowNum+1] - X[RowNum];
    double d_Y = Y[RowNum+1] - Y[RowNum];
    
    return  (Y[RowNum] + (d_Y * d_X1 / d_X2));
}

// Find and calculate Linear Interpolation in log-log space
double LogInterpolate(double X_INT, std::vector<double> X, std::vector<double> Y){
    
    int RowNum = Find(X_INT,X);
    double Ya = Y[RowNum];
    double Yb = Y[RowNum+1];
    double Xa = X[RowNum];
    double Xb = X[RowNum+1];
    return  (exp(log(Ya) + ((log(Yb) - log(Ya)) * (log(X_INT) - log(Xa)) / (log(Xb) - log(Xa)))));
}


// Round value to the nearest value in an array
int Round(int n, double z, std::vector<double> v){
    double mid = (v[n] + v[n+1])/2.0;
    if (z >= mid) {
        return n + 1;
    }else{
        return n;
    }
}

// values at which the hydrogen intensities are calculated (from Slavko)
const std::vector<double> mu { 1.000,0.950,0.900,0.800,0.700,0.600,0.500,0.400,0.300,0.200,0.100,0.005 };
const std::vector<double> logt { 5.55E0,5.65E0,5.75E0,5.85E0,5.95E0,6.05E0,6.15E0,6.25E0,6.35E0,6.45E0 };
const std::vector<double> lsgrav { 13.50E0,13.70E0,13.90E0,14.10E0,14.30E0,14.50E0,14.70E0,14.9E0,15.10E0,15.30E0,15.50E0 };
std::vector<double> F,FF,FFF,FFFF,I,II,III,IIII;
double X, Y, X1, Y1, X2, Y2;


// Read the four hydrogen intensity files to peform the four point interpolation
void Read_NSATMOS(double T, double M, double R){
    
    double delta, lgrav, lt, temp;
    int i_lgrav, i_lt, n_lgrav, n_lt;
    char s1[40],s2[40],s3[40],s4[40];
    
    M = Units::nounits_to_cgs(M, Units::MASS);
    R = Units::nounits_to_cgs(R, Units::LENGTH);
    delta = 1 / sqrt(1 - (2 * Units::G * M / (R * Units::C * Units::C)));
    lgrav = log10(delta * Units::G * M / (R * R));
    lt = log10(1E3 * (T * Units::EV / Units::K_BOLTZ));
    
    i_lt = Find(lt,logt);
    i_lgrav = Find(lgrav,lsgrav);
    
    n_lgrav = i_lgrav + 1;
    n_lt = i_lt + 1;
    
    sprintf(s1,"nsatmos_edd4.emerg%02d%02d01",n_lt,n_lgrav);
    sprintf(s2,"nsatmos_edd4.emerg%02d%02d01",n_lt+1,n_lgrav);
    sprintf(s3,"nsatmos_edd4.emerg%02d%02d01",n_lt,n_lgrav+1);
    sprintf(s4,"nsatmos_edd4.emerg%02d%02d01",n_lt+1,n_lgrav+1);
    ifstream H_table1;
    H_table1.open(s1);
    ifstream H_table2;
    H_table2.open(s2);
    ifstream H_table3;
    H_table3.open(s3);
    ifstream H_table4;
    H_table4.open(s4);
    
    if(H_table1.is_open()){
        while (H_table1 >> temp) {
            F.push_back(temp);
            H_table1 >> temp;
            I.push_back(temp);
        }
    }else{
        cout << "NSATMOS files is not found  " << s1 << endl;
    }
    H_table1.close();
    
    if(H_table2.is_open()){
        while (H_table2 >> temp) {
            FF.push_back(temp);
            H_table2 >> temp;
            II.push_back(temp);
        }
    }else{
        cout << "NSATMOS files is not found  " << s2 << endl;
    }
    H_table2.close();
    
    if(H_table3.is_open()){
        while (H_table3 >> temp) {
            FFF.push_back(temp);
            H_table3 >> temp;
            III.push_back(temp);
        }
    }else{
        cout << "NSATMOS files is not found  " << s3 << endl;
    }
    H_table3.close();
    
    if(H_table4.is_open()){
        while (H_table4 >> temp) {
            FFFF.push_back(temp);
            H_table4 >> temp;
            IIII.push_back(temp);
        }
    }else{
        cout << "NSATMOS files is not found  " << s4 << endl;
    }
    H_table4.close();
    
    X = lt;
    Y = lgrav;
    
    X1 = logt[i_lt];
    Y1 = lsgrav[i_lgrav];
    X2 = logt[i_lt + 1];
    Y2 = lsgrav[i_lgrav + 1];
/*
    cout << s1 << endl;
    cout << s2 << endl;
    cout << s3 << endl;
    cout << s4 << endl;
*/    
}

// Calculate the final interpolated intensity
double Hydrogen(double E, double cos_theta){
    double freq, P;
    double I_int[8],Q[4],R[2];
    int i_mu(0), n_mu, down, up;
    
    std::vector<double> F_temp,FF_temp,FFF_temp,FFFF_temp,I_temp,II_temp,III_temp,IIII_temp;

    freq = 1E3 * E * Units::EV / Units::H_PLANCK;
    
    for (int m = 0; m < 12; ++m) {
        if (cos_theta <= mu[m]) {
            i_mu = m;
            }
        } 
    n_mu = i_mu + 1;
    //cout << i_mu+1 << " " << mu[i_mu+1] << " " << cos_theta << " " << mu[i_mu] << " " << i_mu << endl;

    
   for (int i = 0; i < 2; ++i) {
       down = (i_mu + i) * 128;
       up = down + 128;
       //cout << down << endl;
       
        for (int j = down; j < up; ++j) {
            F_temp.push_back(F[j]);
            I_temp.push_back(I[j]);
        }
        I_int[i] = LogInterpolate(freq,F_temp,I_temp);
        //cout << i_mu << " " << down << " " << I[down] << endl;
   }
    
                
                
    for (int i = 0; i < 2; ++i) {
        down = (i_mu + i) * 128;
        up = down + 128;
        
        for (int j = down; j < up; ++j) {
            FF_temp.push_back(FF[j]);
            II_temp.push_back(II[j]);
        }
        I_int[i+2] = LogInterpolate(freq,FF_temp,II_temp);
    }
    
    for (int i = 0; i < 2; ++i) {
        down = (i_mu + i) * 128;
        up = down + 128;

        for (int j = down; j < up; ++j) {
            FFF_temp.push_back(FFF[j]);
            III_temp.push_back(III[j]);
        }
        I_int[i+4] = LogInterpolate(freq,FFF_temp,III_temp);
    }
    
    for (int i = 0; i < 2; ++i) {
        down = (i_mu + i) * 128;
        up = down + 128;

        for (int j = down; j < up; ++j) {
            FFFF_temp.push_back(FFFF[j]);
            IIII_temp.push_back(IIII[j]);
        }
        I_int[i+6] = LogInterpolate(freq,FFFF_temp,IIII_temp);
    }


    // Perform the four point linear interpolation
    Q[0] = LogLinear(cos_theta,mu[i_mu],I_int[0],mu[i_mu+1],I_int[1]);
    Q[1] = LogLinear(cos_theta,mu[i_mu],I_int[2],mu[i_mu+1],I_int[3]);
    Q[2] = LogLinear(cos_theta,mu[i_mu],I_int[4],mu[i_mu+1],I_int[5]);
    Q[3] = LogLinear(cos_theta,mu[i_mu],I_int[6],mu[i_mu+1],I_int[7]); 

    R[0] = LogLinear(Y,Y1,Q[0],Y2,Q[2]);
    R[1] = LogLinear(Y,Y1,Q[1],Y2,Q[3]);

    P = LogLinear(X,X1,R[0],X2,R[1]);

    if (cos_theta < 0.005) P = 0;


/*
    R = exp((log(Q[0])*(log(X2) - log(X))/(log(X2) - log(X1))) + (log(Q[1])*(log(X) - log(X1))/(log(X2) - log(X1))));
    R = exp((log(Q[2])*(log(X2) - log(X))/(log(X2) - log(X1))) + (log(Q[3])*(log(X) - log(X1))/(log(X2) - log(X1))));
    
    P = exp((log(R1)*(log(Y2) - log(Y))/(log(Y2) - log(Y1))) + (log(R2)*(log(Y) - log(Y1))/(log(Y2) - log(Y1))));
 
    R1 = (Q[0]*(X2 - X)/(X2 - X1)) + (Q[1]*(X - X1)/(X2 - X1));
    R2 = (Q[2]*(X2 - X)/(X2 - X1)) + (Q[3]*(X - X1)/(X2 - X1));
    
    P = (R1*(Y2 - Y)/(Y2 - Y1)) + (R2*(Y - Y1)/(Y2 - Y1));
*/   
    return P;
}

/**************************************************************************************/
/* Helium:                                                                            */
/*           mirroring of hydrogen routines                                           */
/*                                                                                    */
/**************************************************************************************/

//double X, Y, X1, Y1, X2, Y2;
//std::vector<double> F, FF, FFF, FFFF, I, II, III, IIII;

// Read the helium intensity file, set to 4 sets of intensity based on combinations of parameters of nearest 2 logt and 2 lgrav,
// which can be used to peform the four point interpolation.
const std::vector<double> mu_he {1,0.99995,0.9998,0.99875,0.996195,0.984808,0.965926,0.939693,0.906308,0.866025,0.819152,0.766044,0.707107,0.642787,0.573577,0.499998,0.422622,0.342021,0.258816,0.173652,0.0871556,0.00999616,9.63268e-05};
const std::vector<double> logt_he {5.50515,5.60206,5.69897,5.79934,5.89763,6,6.11394,6.20412,6.30103,6.39794,6.50515,6.60206,6.69897};
const std::vector<double> lsgrav_he {13.6021,13.7993,14,14.2041,14.3802,14.6021,14.7993,14.8976};
void Read_NSX(double T, double M, double R){
 
    double delta, temp, dump, lt, lgrav;
    int i_lt, i_lgrav, n_lt, n_lgrav, skip_to, skip_two, size_logt, size_lsgrav, size_mu, size_ener, size_set;
    char name[40];
    std::vector<double> freq;

    //setting values of lt and lgrav based on input T, M, and R. Also sets to load using first mu value.   
    M = Units::nounits_to_cgs(M, Units::MASS);
    R = Units::nounits_to_cgs(R, Units::LENGTH);
    delta = 1 / sqrt(1 - (2 * Units::G * M / (R * Units::C * Units::C)));
    lgrav = log10(delta * Units::G * M / (R * R));
    lt = log10(1E3 * (T * Units::EV / Units::K_BOLTZ));
    //cout << lt << " " << lgrav << endl;
    
    //Use Find to determine i_lt, i_lgrav, n_lt, n_grav.
    //i_lt is the index of the lower lt value,
    //n_lt is the index of the upper lt value,
    //i_lgrav is the index of the lower lgrav value,
    //n_lgrav is the index of the upper lgrav value,
    i_lt = Find(lt,logt_he);
    i_lgrav = Find(lgrav,lsgrav_he);
    n_lt = i_lt+1;
    n_lgrav = i_lgrav+1;
 

    //Open helium spectra file
    ifstream He_table1;
    sprintf(name, "nsx_He_2.out");
    He_table1.open(name);


    if(He_table1.is_open()){
        //discarding logt choices
        He_table1 >> size_logt;
        for (int i = 1; i <= size_logt; i++){
            He_table1 >> dump;
         }

        //discarding local gravity choices
        He_table1 >> size_lsgrav;
        for (int i = 1; i <= size_lsgrav; i++){
            He_table1 >> dump;
        }

        //discarding mu choices
        He_table1 >> size_mu;
        for (int i = 1; i <= size_mu; i++){
            He_table1 >> dump;
         }

        //loading energy points
        He_table1 >> size_ener;
        for (int i = 1; i <= size_ener; i++){
            He_table1 >> temp;
            temp = temp * 1E3 * Units::EV / Units::H_PLANCK;
            freq.push_back(temp);
        }

        F = freq;
        FF = freq;
        FFF = freq;
        FFFF = freq;

        //calculate size of each parameter set
        //should be equal to size_ener*size_mu = 125*23 = 2875
        size_set = size_ener*size_mu;

  
        //Calculate how many values to skip until desired parameter set
        skip_to = i_lt*size_lsgrav*size_mu*size_ener+i_lgrav*size_mu*size_ener;
        skip_two = (size_lsgrav-2)*size_mu*size_ener;
  
        //Skipping and loading helium spectrum with chosen parameters/condition.
        int k = 0;
        while (k < skip_to) {
            He_table1 >> dump;
            //cout << "Value " << dump << " has been dumped." << endl;
            k++;
        }
        for (int i = 1; i <= size_set; i++) {
            He_table1 >> temp;
            I.push_back(temp);
        }

        //loading second set
        for (int i = 1; i <= size_set; i++) {
            He_table1 >> temp;
            III.push_back(temp);
        }

        //loading third set
        k = 0;
        while (k < skip_two) {
            He_table1 >> dump;
            k++;
        }
        for (int i = 1; i <= size_set; i++) {
            He_table1 >> temp;
            II.push_back(temp);
        }

        //loading fourth set
        for (int i = 1; i <= size_set; i++) {
            He_table1 >> temp;
            IIII.push_back(temp);
        }
        /*
        cout << "The choices are [" << logt_he[i_lt] << " " << lsgrav_he[i_lgrav] << "]" << endl;
        cout << "The choices are [" << logt_he[n_lt] << " " << lsgrav_he[i_lgrav] << "]" << endl;
        cout << "The choices are [" << logt_he[i_lt] << " " << lsgrav_he[n_lgrav] << "]" << endl;
        cout << "The choices are [" << logt_he[n_lt] << " " << lsgrav_he[n_lgrav] << "]" << endl;
        cout << "Start reading in line " << skip_to/125+9 << endl;
        cout << "The first Inu value for first set is " << I[0] << endl;
        cout << "The first Inu value for second set is " << II[0] << endl;
        cout << "The first Inu value for third set is " << III[0] << endl;
        cout << "The first Inu value for last set is " << IIII[0] << endl;
        */      
    }else{
        cout << "NSX_file is not found" << endl;
    }
    He_table1.close();
    
    X = lt;
    Y = lgrav;
    
    X1 = logt_he[i_lt];
    Y1 = lsgrav_he[i_lgrav];
    X2 = logt_he[i_lt + 1];
    Y2 = lsgrav_he[i_lgrav + 1];  

    /***TEST OUTPUT OF SPECTRA READIN******/
/*
    ofstream set1;
    set1.open("he_set1", ios::out);
    for (int i = 0; i < 300; ++i){
        set1 << I[i] << endl;
    }
    set1.close();

    ofstream set2;
    set2.open("he_set2", ios::out);
    for (int i = 0; i < 300; ++i){
        set2 << II[i] << endl;
    }
    set2.close();

    ofstream set3;
    set3.open("he_set3", ios::out);
    for (int i = 0; i < 300; ++i){
        set3 << III[i] << endl;
    }
    set3.close();

    ofstream set4;
    set4.open("he_set4", ios::out);
    for (int i = 0; i < 300; ++i){
        set4 << IIII[i] << endl;
    }
    set4.close();
*/

}

// Calculate the final interpolated intensity
double Helium(double E, double cos_theta){
    double freq, P;
    double I_int[8],Q[4],R[2];
    int i_mu(0), down, mid, up;
    
    std::vector<double> F_temp,FF_temp,FFF_temp,FFFF_temp,I_temp,II_temp,III_temp,IIII_temp,Iv_temp,IIv_temp,IIIv_temp,IIIIv_temp;

    freq = 1E3 * E * Units::EV / Units::H_PLANCK;

    
    //finding mu value 
    for (int m = 0; m < 23; ++m) {

        if (cos_theta <= mu_he[m]) {
            i_mu = m;
            }
        }
        //cout << i_mu+1 << " " << mu_he[i_mu+1] << " " << cos_theta << " " << mu_he[i_mu] << " " << i_mu << endl;

        down = i_mu * 125;
        mid = down + 125;
        up = mid + 125;
        //cout << down << " " << mid << " " << up << endl;
       

        for (int j = down; j < mid; ++j) {
            I_temp.push_back(I[j]);
        }
        for (int j = mid; j < up; ++j){
            Iv_temp.push_back(I[j]);
        }
        F_temp = F;
        I_int[0] = LogInterpolate(freq,F_temp,I_temp);
        I_int[1] = LogInterpolate(freq,F_temp,Iv_temp);
        //cout << i_mu << " " << down << " " << I[down] << endl;

        for (int j = down; j < mid; ++j) {
            II_temp.push_back(II[j]);
        }
        for (int j = mid; j < up; ++j){
            IIv_temp.push_back(II[j]);
        }
        FF_temp = F;
        I_int[2] = LogInterpolate(freq,FF_temp,II_temp);
        I_int[3] = LogInterpolate(freq,FF_temp,IIv_temp);   


        for (int j = down; j < mid; ++j) {
            III_temp.push_back(III[j]);
        }
        for (int j = mid; j < up; ++j) {
            IIIv_temp.push_back(III[j]);
        }
        FFF_temp = F;
        I_int[4] = LogInterpolate(freq,FFF_temp,III_temp);
        I_int[5] = LogInterpolate(freq,FFF_temp,IIIv_temp);
    

        for (int j = down; j < mid; ++j) {
            IIII_temp.push_back(IIII[j]);
        }
        for (int j = mid; j < up; ++j) {
            IIIIv_temp.push_back(IIII[j]);
        }
        FFFF_temp = F;
        I_int[6] = LogInterpolate(freq,FFFF_temp,IIII_temp);
        I_int[7] = LogInterpolate(freq,FFFF_temp,IIIIv_temp);
        //cout << I_temp[0] << " " << Iv_temp[0] << " " << II_temp[0] << " " << IIv_temp[0] << " " << III_temp[0] << " " << IIIv_temp[0] << " " << IIII_temp[0] << " " << IIIIv_temp[0] << endl;


    // Perform the four point linear interpolation
    Q[0] = LogLinear(cos_theta,mu_he[i_mu],I_int[0],mu_he[i_mu+1],I_int[1]);
    Q[1] = LogLinear(cos_theta,mu_he[i_mu],I_int[2],mu_he[i_mu+1],I_int[3]);
    Q[2] = LogLinear(cos_theta,mu_he[i_mu],I_int[4],mu_he[i_mu+1],I_int[5]);
    Q[3] = LogLinear(cos_theta,mu_he[i_mu],I_int[6],mu_he[i_mu+1],I_int[7]); 

    R[0] = LogLinear(Y,Y1,Q[0],Y2,Q[2]);
    R[1] = LogLinear(Y,Y1,Q[1],Y2,Q[3]);

    P = LogLinear(X,X1,R[0],X2,R[1]);

    if (cos_theta < 9.63268e-05) P = 0;

/*
    R1 = exp((log(Q[0])*(log(X2) - log(X))/(log(X2) - log(X1))) + (log(Q[1])*(log(X) - log(X1))/(log(X2) - log(X1))));
    R2 = exp((log(Q[2])*(log(X2) - log(X))/(log(X2) - log(X1))) + (log(Q[3])*(log(X) - log(X1))/(log(X2) - log(X1))));
    
    P = exp((log(R1)*(log(Y2) - log(Y))/(log(Y2) - log(Y1))) + (log(R2)*(log(Y) - log(Y1))/(log(Y2) - log(Y1))));
*/
    return P;
}


/**************************************************************************************/
/* Blackbody:                                                                         */
/*           computes the monochromatic blackbody flux in units of erg/cm^2			  */
/*																					  */
/* pass: T = the temperature of the hot spot, in keV                                  */
/*       E = monochromatic energy in keV * redshift / eta                             */
/**************************************************************************************/
double BlackBody( double T, double E ) {   // Blackbody flux in units of erg/cm^2
    return ( 2.0e9 / pow(Units::C * Units::H_PLANCK, 2) * pow(E * Units::EV, 3) / (exp(E/T) - 1) ); // shouldn't it have a pi?
    // the e9 is to switch E from keV to eV; Units::EV gets it from eV to erg, since it's first computed in erg units.
    // the switch from erg units to photon count units happens above just after this is called.
} // end Blackbody

/************************************************************************************/
/* EnergyBandFlux:                                                                  */
/*                computes the blackbody flux in units of counts/s/cm^2             */
/*                using trapezoidal rule for approximating an integral              */
/*                variant of Bradt equation 6.6                                     */
/*                T, E1, E2 put into eV in this routine                             */
/*																				    */
/* pass: T = the temperature of the hot spot, in keV                                */
/*       E1 = lower bound of energy band in keV * redshift / eta                    */
/*       E2 = upper bound of energy band in keV * redshift / eta                    */
/************************************************************************************/
double EnergyBandFlux( double T, double E1, double E2 ) {
	T *= 1e3; // from keV to eV
	// x = E / T
	E1 *= 1e3; // from keV to eV
	E2 *= 1e3; // from keV to eV
	
	/********************************************/
   	/* VARIABLE DECLARATIONS FOR EnergyBandFlux */
    /********************************************/
	
	// a, b, x, n, h as defined by Mathematical Handbook eqn 15.16 (Trapezoidal rule to approximate definite integrals)
	double a = E1 / T;          // lower bound of integration
	double b = E2 / T;          // upper bound of integration
	double current_x(0.0);      // current value of x, at which we are evaluating the integrand; x = E / T; unitless
	unsigned int current_n(0);  // current step
	unsigned int n_steps(3000); // total number of steps
	double h = (b - a) / n_steps;     // step amount for numerical integration; the size of each step
	double integral_constants = 2.0 * pow(T*Units::EV,3) / pow(Units::C,2) / pow(Units::H_PLANCK,3); // what comes before the integral when calculating flux using Bradt eqn 6.6 (in units of photons/cm^2/s)
	double flux(0.0);           // the resultant energy flux density; Bradt eqn 6.17
	
	// begin trapezoidal rule
	current_x = a + h * current_n;
	flux = Bradt_flux_integrand(current_x);

	for ( current_n = 1; current_n < n_steps-1; current_n++ ) {
		current_x = a + h * current_n;
		flux += 2.0 * Bradt_flux_integrand(current_x);
	}
	
	current_x = a + h * current_n;
	flux += Bradt_flux_integrand(current_x);

	flux *= h/2.0;	
	// end trapezoidal rule; numerical integration complete!
	
    flux *= integral_constants;

	return flux;
} // end EnergyBandFlux


/**************************************************************************************/
/* AtmosEBandFlux:                                                                    */
/*                computes the flux of atmosphere models in units of counts/s/cm^2    */
/*                using Simpson's rule for approximating an integral                  */
/*                See Numerical Recipes eq 4.1.4                                      */
/*                requires usage of Hydrogen/Helium routines                          */
/*                lT, lgrav are already set by which intensity files to load          */
/*                cos_theta is used in Hydrogen/Helium routines                       */
/*                E1, E2 are in keV, will be converted                                */
/*                                                                                    */
/* pass: model = atmospheric model (0 = Hydrogen, 1 = Helium)                         */
/*       cos_theta = angle at one particular time bin                                 */
/*       E1 = lower bound of energy band in keV * redshift / eta                      */
/*       E2 = upper bound of energy band in keV * redshift / eta                      */
/**************************************************************************************/
double AtmosEBandFlux( double model, double cos_theta, double E1, double E2 ) {
    //double f1 = E1 * 1000 * Units::EV / Units::H_PLANCK; //lower frequency bound
    //double f2 = E2 * 1000 * Units::EV / Units::H_PLANCK; //upper frequency bound

    /********************************************/
    /* VARIABLE DECLARATIONS FOR AtmosEBandFlux */
    /********************************************/
    
    int n_steps(3000);       // total number of steps
    //double step_size;           // step size in frequency scale
    double step_size;           // step size in energy scale
    //double current_f;           // central frequnecy of current step
    double current_e;           // central energy of current step
    double e_l, e_m, e_u;       // lower, middle, and upper energy values of Simpson's rule
    double current_n;           // integrated flux in current step
    double flux(0.0);           // total integrated flux
    //std::vector<double> flux_grid; // for finding outliers  
    
    //step_size = (f2-f1)/n_steps;
    step_size = (E2-E1)/n_steps;
    //cout << E2 << " " << E1 << " " << step_size << endl;

    if (model == 0){
        for (int j = 1; j <= n_steps; j++){
            
            current_e = step_size*(j-1+1/2) + E1;
            e_l = (current_e-step_size/2);
            e_m = (current_e);
            e_u = (current_e+step_size/2);
            current_n = step_size * (1/3*Hydrogen(e_l,cos_theta)/e_l + 4/3*Hydrogen(e_m,cos_theta)/e_m + 1/3*Hydrogen(e_u,cos_theta)/e_u);
            /*
            while (std::isnan(current_n)){
                std::cout << "current_n fails at E = " << current_e << std::endl;
                std::cout << "contributions: " << Hydrogen(e_l,cos_theta)/e_l << " " << Hydrogen(e_m,cos_theta)/e_m << " " << Hydrogen(e_u,cos_theta)/e_u << std::endl;
                std::cout << E2 << " " << E1 << " " << " " << cos_theta << " " << std::endl;
                current_n = step_size * (1/3*Hydrogen(e_l,cos_theta)/e_l + 4/3*Hydrogen(e_m,cos_theta)/e_m + 1/3*Hydrogen(e_u,cos_theta)/e_u);
 
            }
            */
            
            /*
            current_e = step_size*(j-1+1/2) + E1;
            e_l = current_e - step_size/2;
            e_m = current_e;
            e_u = current_e + step_size/2;
            current_n = step_size * (1/3*BlackBody(0.1,e_l)/e_l + 4/3*BlackBody(0.1,e_m)/e_m + 1/3*BlackBody(0.1,e_u)/e_u);
            */
            /*
            current_e = step_size*(j-1+1/2) + E1;
            e_l = (current_e);
            e_m = 0;
            e_u = (current_e+step_size);
            current_n = step_size/2 * (BlackBody(0.1,e_l)/e_l+BlackBody(0.1,e_u)/e_u);
            */
            flux += current_n;
            
        }
    }else{
        for (int j = 1; j <= n_steps; j++){
            current_e = step_size*(j-1+1/2) + E1;
            e_l = (current_e-step_size/2);
            e_m = (current_e);
            e_u = (current_e+step_size/2);
            current_n = step_size * (1/3*Helium(e_l,cos_theta)/e_l + 4/3*Helium(e_m,cos_theta)/e_m + 1/3*Helium(e_u,cos_theta)/e_u);
            /*
            while (std::isnan(current_n)){
                std::cout << "current_n fails at E = " << current_e << std::endl;
                std::cout << "contributions: " << Helium(e_l,cos_theta)/e_l << " " << Helium(e_m,cos_theta)/e_m << " " << Helium(e_u,cos_theta)/e_u << std::endl;
                std::cout << E2 << " " << E1 << " " << " " << cos_theta << " " << std::endl;
                current_n = step_size * (1/3*Helium(e_l,cos_theta)/e_l + 4/3*Helium(e_m,cos_theta)/e_m + 1/3*Helium(e_u,cos_theta)/e_u);
            }
            */
            //cout << j-1+1/2 << " " << step_size << " " << current_e << endl;
            //cout << e_l << " " << e_m << " " << e_u << endl;
            //cout << Helium(e_l,cos_theta)/e_l << " " << Helium(e_m,cos_theta)/e_m << " " << Helium(e_u,cos_theta)/e_u << endl;
            flux += current_n;
            //if (j == 100) cout << j << " " << flux << endl;
        }
    }

    flux = flux/Units::H_PLANCK;
    return flux;
}

/**************************************************************************************/
/* Bradt_flux_integrand:                                                              */
/*                      integrand of Bradt eqn 6.6 when integrating over nu, modified */
/*                      so the exponent is 2 not 3, so that it comes out as photon    */
/*                      number flux, instead of erg flux                              */
/*																					  */
/* pass: x = current_x from above routine                                             */
/**************************************************************************************/
double Bradt_flux_integrand( double x ) {
	return ( pow(x,2) / (exp(x) - 1) );  // 2 (not 3) for photon number flux
} // end Bradt_flux_integrand

/**************************************************************************************/
/* Gray:																			  */
/*		computes and returns the limb darkening factors for a Gray electron -         */
/*		scattering atmosphere (Hopf function) using values from Chandrasekhar's       */
/*		"Radiative Transfer", table 13												  */
/*																					  */
/* pass: cosine = curve.cosbeta[i] * curve.eta[i]                                     */
/**************************************************************************************/
double Gray( double cosine ) {
	
	/**********************************/
   	/* VARIABLE DECLARATIONS FOR Gray */
    /**********************************/
	
    double F[11],   // limb darkening values for electron-scattering atmosphere
           mu[11];  // a table of cos(theta) values
    int i;          // loop variable

    for ( i = 0; i <= 10; i++ ) {
        mu[i] = 0.0 + i*0.1;
    }
    /*
    // Value from Mihalas's "Stellar Atmospheres"
    F[0] = 0.4330;
    F[1] = 0.5401;
    F[2] = 0.6280;
    F[3] = 0.7112;
    F[4] = 0.7921;
    F[5] = 0.8716;
    F[6] = 0.9501;
    F[7] = 1.0280;
    F[8] = 1.1053;
    F[9] = 1.1824;
    F[10] = 1.2591;
    */
  
    // Values from Chandrasekhar's "Radiative Transfer", Table 13
    F[0] = 1.0;
    F[1] = 1.2647;
    F[2] = 1.48009;
    F[3] = 1.68355;
    F[4] = 1.88105;
    F[5] = 2.07496;
    F[6] = 2.2665;
    F[7] = 2.45639;
    F[8] = 2.64503;
    F[9] = 2.83274;
    F[10] = 3.01973;
  
    for ( i = 0; i <= 10; i++ ) {
        F[i] *= (0.5/1.194);
    }

    int index(0);
    //i=1;

    if (cosine == 1.0)
    	index = 10;
    else {
        while ( cosine >= mu[index]) 
        	index++;
    }
    // cosine lies in the range   mu[index-1] <= cosine <= mu[index]

    if (cosine == mu[index-1]) 
    	return F[index-1];
    else {
        if (cosine == mu[index]) 
        	return F[index];
        else {   
            return ( F[index-1] + (cosine - mu[index-1]) * (F[index] - F[index-1])/(mu[index]-mu[index-1]) );
        }
    }
} // end Gray


/**************************************************************************************/
/* LegP2:																			  */
/*       calculating a Legendre polynomial P2										  */
/*       called in the routine/method/function recalc								  */
/*																					  */
/* pass: costheta = cos of angle of the hotspot down from geom. north pole [unitless] */
/**************************************************************************************/
double LegP2( double costheta ) {
    double P2;
    P2 = 0.5 * ( 3.0 * pow( costheta, 2 ) - 1.0 );
    return P2;
} // end LegP2

/**************************************************************************************/
/* LegP4:																			  */
/*       calculating a Legendre polynomial P4										  */
/*       called in the routine/method/function recalc								  */
/*																					  */
/* pass: costheta = cos of angle of the hotspot down from geom. north pole [unitless] */
/**************************************************************************************/
double LegP4( double costheta ) {
    double P4;
    P4 = 0.125 * ( 35 * pow( costheta, 4 ) - 30 * pow( costheta, 2 ) + 3);
    return P4;
} // end LegP4

/**************************************************************************************/
/* calcreq: 																		  */
/*		   calculating the radius of the NS at its equator                            */
/*		   for oblate stars, useful; for spherical stars, redundant                   */
/*																					  */		   
/* pass: omega = spin frequency of the NS [unitless]                                  */
/*       mass = mass of the NS [unitless]											  */
/*       theta_0 = angle of the hotspot down from the geometric north pole [radians]  */
/*       rspot = radius of the NS at the hotspot [unitless]							  */
/**************************************************************************************/
double calcreq( double omega, double mass, double theta_0, double rspot ) {
   
    /*************************************/
   	/* VARIABLE DECLARATIONS FOR calcreq */
    /*************************************/
   
	double mu,       // = cos(theta_0)
           req,      // Radius of the neutron star at the equator
           pi,       // PI, the constant (3.1415926...)
           zeta,     // Defined in MLCB9
           epsilon,  // Doppler boost factor, defined in MLCB33
           G,        // Gravitational constant
           P2,       // Legendre polynomial
           P4,       // Legendre polynomial
           b0,       // Coefficient from MLCB Table 1
           b2,       // Coefficient from MLCB Table 1
           b4,       // Coefficient from MLCB Table 1
           c2,       // Coefficient from MLCB Table 1
           c4;       // Coefficient from MLCB Table 1
	    
	G = 1.32746E+11;
    mu = cos(theta_0);
    pi = 4.0 * atan(1.0);
    zeta = mass / rspot;
    epsilon = pow( rspot * omega, 2.0 ) / zeta;

    b0 = 0.18*epsilon - 0.23*zeta*epsilon + 0.18*pow(epsilon,2);
    b2 = 0.39*epsilon - 0.29*zeta*epsilon + 0.42*pow(epsilon,2);
    //	    b4 = -0.04*epsilon + 0.15*zeta*epsilon - 0.13*pow(epsilon,2);
    b4 = -8.0/3.0 * (b0 - 0.5*b2);

    c2 = 0.60*pow(epsilon,2);
    c4 = -0.12*pow(epsilon,2);
    
    P2 = LegP2( mu );
    P4 = LegP4( mu );

    req = rspot * ( 1.0 + b0 + b2*P2 + b4*P4 + P2*(c2*P2 + c4*P4) );
    	    
    //std::cout << "rspot_calcreq = " << rspot <<std::endl;
    //std::cout << "req_calcreq = " << req <<std::endl;
	  
	return req;
} // end calcreq

// end Chi.cpp
