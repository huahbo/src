/* Velocity Scan by 2D Parametric Development of Reflections */
/*
  Copyright (C) 2012 University of Texas at Austin
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <rsf.h>
#ifdef _OPENMP
#include <omp.h>
#endif

// files
sf_file dataFile;
sf_file outFile;
sf_file auxFile;          // output file - semblance

int   tNum_;                 
float tStart_;
float tStep_;

int   recNum_;                 
float recStart_;
float recStep_;

int   shotNum_;                 
float shotStart_;
float shotStep_;

// stacking params
int   pNum_;                 
float pStart_;
float pStep_;

// scan parameters
int   vNum_;
float vStep_;
float vStart_;

int   vw_;

int main (int argc, char* argv[]) {

// Initialize RSF 
    sf_init (argc,argv);

// INPUT FILES
    dataFile = sf_input ("in");
    /* common-offset sections */
    outFile  = sf_output("out");
    /*  */

	if ( NULL != sf_getstring("aux") ) {
		/* output file containing semblance measure of CIGs stacking */ 
		auxFile  = sf_output ("aux");
	} else {
		sf_error ("Need output: partial zero-offset sections");
	}

    // data params

    if ( !sf_histint   (dataFile, "n1", &tNum_)   ) sf_error ("Need n1= in input");
    if ( !sf_histfloat (dataFile, "d1", &tStep_)  ) sf_error ("Need d1= in input");
    if ( !sf_histfloat (dataFile, "o1", &tStart_) ) sf_error ("Need o1= in input");

    if ( !sf_histint   (dataFile, "n2", &recNum_)   ) sf_error ("Need n2= in input");
    if ( !sf_histfloat (dataFile, "d2", &recStep_)  ) sf_error ("Need d2= in input");
    if ( !sf_histfloat (dataFile, "o2", &recStart_) ) sf_error ("Need o2= in input");

    if ( !sf_histint   (dataFile, "n3", &shotNum_)   ) sf_error ("Need n3= in input");
    if ( !sf_histfloat (dataFile, "d3", &shotStep_)  ) sf_error ("Need d3= in input");
    if ( !sf_histfloat (dataFile, "o3", &shotStart_) ) sf_error ("Need o3= in input");

//    if ( !sf_histint   (dataFile, "n4", &dp.yNum)   ) sf_error ("Need n4= in input");
//    if ( !sf_histfloat (dataFile, "d4", &dp.yStep)  ) sf_error ("Need d4= in input");
//    if ( !sf_histfloat (dataFile, "o4", &dp.yStart) ) sf_error ("Need o4= in input");

    //
    char* corUnit;
    char* unit;
	
    // data
    // time - in ms
//    corUnit = (char*) "ms"; unit = sf_histstring (dataFile, "unit1"); if (!unit) sf_error ("unit1 in data file is not defined");
//    if ( strcmp (corUnit, unit) ) { dp.zStep *= 1000; dp.zStart *= 1000; }
    // receiver - in m
    corUnit = (char*) "m"; unit = sf_histstring (dataFile, "unit2"); if (!unit) sf_error ("unit2 in data file is not defined");
    if ( strcmp (corUnit, unit) ) { recStep_ *= 1000; recStart_ *= 1000; }
    // source - in m
    corUnit = (char*) "m"; unit = sf_histstring (dataFile, "unit3"); if (!unit) sf_error ("unit3 in data file is not defined");
    if ( strcmp (corUnit, unit) ) { shotStep_ *= 1000; shotStart_ *= 1000; }

    // offset - in m
 //   corUnit = (char*) "m"; unit = sf_histstring (dataFile, "unit4"); if (!unit) sf_error ("unit4 in data file is not defined");
//    if ( strcmp (corUnit, unit) ) { dp.hStep *= 1000; dp.hStart *= 1000; }

    if ( !sf_getfloat ("po",    &pStart_) ) pStart_ = shotStart_;
    /* start position in stack section */
	if ( !sf_getint ("pn", &pNum_) ) pNum_ = recNum_;
    /* number of positions in stack section */
	if (!pNum_) {sf_warning ("vn value is changed to 1"); pNum_ = recNum_;}
    if ( !sf_getfloat ("pd",    &pStep_) ) pStep_ = recStep_;
    /* increment of positions in stack section */
	if (!pStep_) {sf_warning ("pd value is changed to 50"); pStep_ = recStep_;}


    if ( !sf_getint ("vn",    &vNum_) ) vNum_ = 1;
    /* number of scanned velocities  */
	if (!vNum_) {sf_warning ("vn value is changed to 1"); vNum_ = 1;}
    if ( !sf_getfloat ("vo",    &vStart_) ) vStart_ = 1500;
    /* start velocity */
	if (!vStart_) {sf_warning ("vn value is changed to 1500"); vStart_ = 1500.0;}
    if ( !sf_getfloat ("vd",    &vStep_) ) vStep_ = 50;
    /* increment of velocities */
	if (!vStep_) {sf_warning ("vd value is changed to 50"); vStep_ = 50.f;}

    if ( !sf_getint ("vw",   &vw_) )   vw_ = 11;
	/* height of a vertical window for semblance calculation */
	if (!vw_) {sf_warning ("vertical window size is changed to 1"); vw_ = 1;}

    sf_putint    (outFile, "n1", tNum_);
	sf_putint    (outFile, "n2", pNum_);
	sf_putint    (outFile, "n3", vNum_);
    sf_putfloat  (outFile, "d1", tStep_); 
	sf_putfloat  (outFile, "d2", pStep_);
	sf_putint    (outFile, "d3", vStep_);
    sf_putfloat  (outFile, "o1", tStart_); 
	sf_putfloat  (outFile, "o2", pStart_);
	sf_putfloat  (outFile, "o3", vStart_);
    sf_putstring (outFile, "label1", "time"); sf_putstring(outFile, "label2", "inline"); sf_putstring(outFile, "label3", "velocity");
    sf_putstring (outFile, "unit1", "ms"); sf_putstring(outFile, "unit2", "m"); sf_putstring(outFile, "unit3", "m/s");

    sf_putint    (auxFile, "n1", tNum_);
	sf_putint    (auxFile, "n2", pNum_);
	sf_putint    (auxFile, "n3", vNum_);
    sf_putfloat  (auxFile, "d1", tStep_); 
	sf_putfloat  (auxFile, "d2", pStep_);
	sf_putint    (auxFile, "d3", vStep_);
    sf_putfloat  (auxFile, "o1", tStart_); 
	sf_putfloat  (auxFile, "o2", pStart_);
	sf_putfloat  (auxFile, "o3", vStart_);
    sf_putstring (auxFile, "label1", "time"); sf_putstring (auxFile, "label2", "inline"); sf_putstring (auxFile, "label3", "velocity");
    sf_putstring (auxFile, "unit1", "ms"); sf_putstring(auxFile, "unit2", "m"); sf_putstring (auxFile, "unit3", "m/s");

	const int zoSize = pNum_ * tNum_;
	const int dataSize = shotNum_ * recNum_ * tNum_;
	const int tNumRed = tNum_ - 1;
	
    float* zo    = sf_floatalloc (zoSize);
    float* zoSq  = sf_floatalloc (zoSize);
    float* semb  = sf_floatalloc (zoSize);
    int*   count = sf_intalloc (zoSize);

	float* data = sf_floatalloc (dataSize);
    sf_floatread (data, dataSize, dataFile);

	for (int iv = 0; iv < vNum_; ++iv) {

		float vel = vStart_ + iv * vStep_;

		sf_warning ("scanning: velocity %d of %d - %g;", iv + 1, vNum_, vel);		
	    
		memset ( zo,    0, zoSize * sizeof (float) );   
	    memset ( zoSq,  0, zoSize * sizeof (float) );   
	    memset ( semb,  0, zoSize * sizeof (float) );   
		memset ( count, 0, zoSize * sizeof (int)   );   

		// loop over shots
		for (int is = 0; is < shotNum_; ++is) {				
			const float shotPos = shotStart_ + shotStep_ * is;
			// loop over receivers
			for (int ir = 0; ir < recNum_; ++ir) {						
				const float curOffset = recStart_ + recStep_ * ir;
				const float halfOffset = curOffset / 2.f;
				const float offsetSq = curOffset * halfOffset;
				const int forDataInd = (is * recNum_ + ir) * tNum_;
#ifdef _OPENMP 
#pragma omp parallel for
#endif					
				for (int it = 0; it < tNum_; ++it) {	
					const float curTime = tStart_ + it * tStep_;
					if (curTime < 1e-6) continue;
					// take sample
					const int dataInd = forDataInd + it;
					const float sample = data[dataInd];
					// calc function limits
					const float forLim = offsetSq / (vel * curTime);
					const float limitLeft  = halfOffset - forLim;
					const float limitRight = halfOffset + forLim;

					// loop over zero-offset positions
					for (int ip = 0; ip < pNum_; ++ip) {
						const float curPos = pStart_ + ip * pStep_;
						const float l0 = curPos - shotPos;			
						if (l0 < limitLeft || l0 > limitRight) continue;

						const float a = pow (curTime, 2) - pow (curOffset / vel, 2);
						const float b = pow (l0 - halfOffset, 2);
						const float c = pow (halfOffset, 2);

						const float t0 = c ? sqrt ( a * (1 - b / c) ) : curTime;
						const int tInd = (t0 - tStart_) / tStep_;
		
						if (tInd < 0 || tInd > tNumRed) continue; 
	
						const int indZO   = ip * tNum_ + tInd;
						zo    [indZO] += sample;
						zoSq  [indZO] += sample*sample;
						count [indZO] += 1;
					}
				}
			}
		}
		// semblance calculation
		const int vwhalf = vw_ / 2;
		for (int ip = 0; ip < pNum_; ++ip) {
			const int ts = ip * tNum_;
#ifdef _OPENMP 
#pragma omp parallel for
#endif					
			for (int it = 0; it < tNum_; ++it) {	

				float sampleSq = 0.f;	
				float sqSample = 0.f;
			
				int ccount = 0;
				int totalCount = 0;

				for (int ic = 0, iw = it - vwhalf; ic < vw_; ++ic, ++iw) {
					if (iw < 0 || iw > tNumRed) continue;
					const int ind = ts + iw;
					sampleSq   += pow (zo [ind], 2);
					sqSample   += zoSq [ind];
					totalCount += count [ind];
					ccount += 1;
				}
				totalCount /= ccount;
				const float curSemb = sqSample ? sampleSq / ( totalCount * sqSample ) : 0.f;
				semb [ts + it] = curSemb;
			}
		}	

		int offset = zoSize * iv * sizeof (float);
		sf_seek (outFile, offset, SEEK_SET);
		sf_floatwrite (semb, zoSize, outFile);
		sf_seek (auxFile, offset, SEEK_SET);
		sf_floatwrite (zo, zoSize, auxFile);
	}

    free (data);
    free (zo);
    free (zoSq);
    free (semb);
    free (count);

    sf_fileclose (dataFile);
    sf_fileclose (outFile);
    sf_fileclose (auxFile);

    return 0;
}