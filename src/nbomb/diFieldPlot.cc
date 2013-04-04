/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diFieldPlot.cc,v 2.1 2006/05/30 15:59:11 ansteinf Exp $

  Copyright (C) 2006 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no
  
  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <diFieldPlot.h>
#include <diContouring.h>
#include <diFontManager.h>
#include <iostream>
#include <GL/gl.h>
#include <sstream>
#include <math.h>

using namespace std;

const int MaxWindsAuto=40;
const int MaxArrowsAuto=55;


// Default constructor
FieldPlot::FieldPlot()
:Plot(), pshade(false), pundefined(false), difference(false),
         vectorIndex(-1), vectorAnnotationSize(0.) {
#ifdef DEBUGPRINT
  cerr << "++ FieldPlot::Default Constructor" << endl;
#endif
}

// Destructor
FieldPlot::~FieldPlot(){
#ifdef DEBUGPRINT
  cerr << "++ FieldPlot::Destructor" << endl;
#endif
  cleanFields();
}


void FieldPlot::cleanFields(){
#ifdef DEBUGPRINT
  cerr << " FieldPlot::cleanFields fields.size():" << fields.size() << endl;
#endif
  int n= fields.size();
  for (int i=0; i<n; i++) delete fields[i];
  fields.clear();
}


Area& FieldPlot::getFieldArea(){
  if (fields.size() && fields[0])
    return fields[0]->area;
  else return area; // return master-area
}


//  return area for existing field
bool FieldPlot::getRealFieldArea(Area& a){
  if (fields.size() && fields[0] && fields[0]->data) {
    a= fields[0]->area;
    return true;
  }
  return false;
}


// check if current data from plottime
bool FieldPlot::updateNeeded(miString& pin){
  if (ftime.undef() || ftime != ctime || fields.size()==0){
    pin= pinfo;
    return true;
  }
  return false;
}


bool FieldPlot::updateLevelNeeded(const miString& levelSpec, miString& pin)
{
  if (fields.size()>0) {
    if (fields[0]->levelSpec.downcase()==levelSpec.downcase()) {
      pin= pinfo;
      return true;
    }
  } else {
    // was not found at previous level, try this...
    pin= pinfo;
    return true;
  }
  return false;
}


bool FieldPlot::updateIdnumNeeded(const miString& idnumSpec, miString& pin)
{
  if (fields.size()>0) {
    if (fields[0]->idnumSpec.downcase()==idnumSpec.downcase()) {
      pin= pinfo;
      return true;
    }
  } else {
    // was not found at previous idnum, try this...
    pin= pinfo;
    return true;
  }
  return false;
}


void FieldPlot::getFieldAnnotation(miString& s, Colour& c){
  c= poptions.linecolour;
  if (fields.size()>0) {
    if (fields[0]) {
      if (fields[0]->data) s= fields[0]->fulltext;
      return;
    }
  }
  s= "";
}


// Extract plotting-parameters from PlotInfo.
bool FieldPlot::prepare(const miString& pin)
{
  pinfo= pin;
  setPlotInfo(pin);

  vector<miString> tokens= pinfo.split();
  int n= tokens.size();
  miString fname= tokens[2];

  if (difference && n>3)
    fname= tokens[3];
  else if (n>2)
    fname= tokens[2];
  else
    return false;

  setup.fillFieldPlotOptions(fname,pin,poptions);

  ptype= poptions.fptype;

#ifdef DEBUGPRINT
  if       (ptype==fpt_contour)          cerr<<"FieldPlot "<<fname<<" : "<<"plotContour"<<endl;
  else if (ptype==fpt_wind)             cerr<<"FieldPlot "<<fname<<" : "<<"plotWind"<<endl;
  else if (ptype==fpt_wind_colour)      cerr<<"FieldPlot "<<fname<<" : "<<"plotWindColour"<<endl;
  else if (ptype==fpt_wind_temp_fl)     cerr<<"FieldPlot "<<fname<<" : "<<"plotWindTempFL"<<endl;
  else if (ptype==fpt_vector)           cerr<<"FieldPlot "<<fname<<" : "<<"plotVector"<<endl;
  else if (ptype==fpt_vector_colour)    cerr<<"FieldPlot "<<fname<<" : "<<"plotVectorColour"<<endl;
  else if (ptype==fpt_direction)        cerr<<"FieldPlot "<<fname<<" : "<<"plotDirection"<<endl;
  else if (ptype==fpt_direction_colour) cerr<<"FieldPlot "<<fname<<" : "<<"plotDirectionColour"<<endl;
  else if (ptype==fpt_box_pattern)      cerr<<"FieldPlot "<<fname<<" : "<<"plotBox_pattern"<<endl;
  else if (ptype==fpt_box_alpha_shade)  cerr<<"FieldPlot "<<fname<<" : "<<"plotBox_alpha_shade"<<endl;
  else if (ptype==fpt_alpha_shade)      cerr<<"FieldPlot "<<fname<<" : "<<"plotAlpha_shade"<<endl;
  else if (ptype==fpt_alarm_box)        cerr<<"FieldPlot "<<fname<<" : "<<"plotAlarmBox"<<endl;
  else                                  cerr<<"FieldPlot "<<fname<<" : "<<"ERROR"<<endl;
#endif

  pshade= (ptype==fpt_box_alpha_shade ||
	   ptype==fpt_alpha_shade ||
	   ptype==fpt_box_pattern ||
	   ptype==fpt_alarm_box);

  if (ptype==fpt_contour && poptions.contourShading>0)
    pshade= true;

  pundefined= (poptions.undefMasking>0);

  if      (ptype==fpt_wind)          vectorIndex=0;
  else if (ptype==fpt_wind_colour)   vectorIndex=0;
  else if (ptype==fpt_wind_temp_fl)  vectorIndex=0;
  else if (ptype==fpt_vector)        vectorIndex=0;
  else if (ptype==fpt_vector_colour) vectorIndex=0;
  else                               vectorIndex=-1;

  return true;
}

//  set list of field-pointers, update datatime
bool FieldPlot::setData(const vector<Field*>& vf, const miTime& t){
  cleanFields();
  int n= vf.size();
  for (int i=0; i<n; i++) fields.push_back(vf[i]);
  ftime= t;

  if (n>0) {
    plotname= fields[0]->fulltext;
    analysisTime= fields[0]->analysisTime;
  } else {
    plotname= "";
  }

  return true;
}

    struct aTable{
      miString colour;
      miString pattern;
      miString text;
    };

bool FieldPlot::getAnnotations(vector<miString>& anno)
{
  //  cerr <<"getAnnotations:"<<anno.size()<<endl;

  if (fields.size()==0 || !fields[0] || !fields[0]->data)
    return false;

  int nanno = anno.size();
  for(int j=0; j<nanno; j++){
    if(anno[j].contains("table")){
      if (!enabled || poptions.table==0  ||
	  (poptions.palettecolours.size()==0 && poptions.patterns.size()==0))
	continue;;

      miString endString;
      miString startString;
      if(anno[j].contains(",")){
	size_t nn = anno[j].find_first_of(",");
	endString = anno[j].substr(nn);
	startString =anno[j].substr(0,nn);
      } else {
	startString =anno[j];
      }

      //if asking for spesific field
      if(anno[j].contains("table=")){
	miString name = startString.substr(startString.find_first_of("=")+1);
	if( name[0]=='"' )
	  name.remove('"');
	name.trim();
	if(!fields[0]->fieldText.contains(name))	continue;
      }

      miString str  = "table=\"";
      str += fields[0]->fieldText;

      //find min/max
      float cmin=  fieldUndef;
      float cmax= -fieldUndef;
      int ndata = fields[0]->nx*fields[0]->ny;
      for (int i=0; i<ndata; ++i) {
	if (fields[0]->data[i]!=fieldUndef) {
	  if (cmin>fields[0]->data[i]) cmin= fields[0]->data[i];
	  if (cmax<fields[0]->data[i]) cmax= fields[0]->data[i];
	}
      }


      aTable table;
      vector<aTable> vtable;

      int ncolours  = poptions.palettecolours.size();
      int ncold     = poptions.palettecolours_cold.size();
      int npatterns = poptions.patterns.size();
      int nlines    = poptions.linevalues.size();
      int nloglines = poptions.loglinevalues.size();
      int ncodes = (ncolours>npatterns ? ncolours : npatterns);

      if(cmin>poptions.base) ncold=0;


      //discontinuous - no more entries than class specifications
      vector<miString> classSpec;
      if(poptions.discontinuous == 1 &&
	 poptions.lineinterval>0.99 && poptions.lineinterval<1.01){
	classSpec = poptions.classSpecifications.split(",");
	ncodes = classSpec.size();
      }

      if(nlines > 0 && nlines-1 < ncodes)
	ncodes = nlines-1;
      else if(nloglines > 0 && ncodes > 5*nloglines-1)
	ncodes = 5*nloglines-1;

      for(int i=ncold-1; i>=0; i--){
	table.colour = poptions.palettecolours_cold[i].Name();
	if(npatterns>0){
	  int ii = (npatterns-1) - i%npatterns;
	  table.pattern = poptions.patterns[ii];
	}
	vtable.push_back(table);
      }

      for (int i=0; i<ncodes; i++){
	if(ncolours>0){
	  int ii = i%ncolours;
	  table.colour = poptions.palettecolours[ii].Name();
	} else {
	  table.colour = poptions.fillcolour.Name();
	}
	if(npatterns>0){
	  int ii = i%npatterns;
	  table.pattern = poptions.patterns[ii];
	}
	vtable.push_back(table);
      }

      if(poptions.discontinuous == 1 &&
	 poptions.lineinterval>0.99 && poptions.lineinterval<1.01){
	for (int i=0; i<ncodes; i++){
	  vector<miString> tstr = classSpec[i].split(":");
	  if(tstr.size()>1) {
	    vtable[i].text = tstr[1];
	  }
	}

      } else if(nlines>0){
	for(int i=0; i<ncodes; i++){
	  float min = poptions.linevalues[i];
	  float max = poptions.linevalues[i+1];
	  ostringstream ostr;
	  ostr <<min<<" - "<<max;
	  vtable[i].text = ostr.str();
	}

      } else if(nloglines>0){
        vector<float> vlog;
	for (int n=0; n<5; n++) {
	  float slog= powf(10.0,n);
	  for (int i=0; i<nloglines; i++)
	    vlog.push_back(slog*poptions.loglinevalues[i]);
	}
	for(int i=0; i<ncodes; i++){
	  float min = vlog[i];
	  float max = vlog[i+1];
	  ostringstream ostr;
	  ostr <<min<<" - "<<max;
	  vtable[i].text = ostr.str();
	}

      } else {

	//cold colours
	float max=poptions.base;
	float min=max;
	for(int i=ncold-1; i>-1; i--){
	  min = max - poptions.lineinterval;
	  ostringstream ostr;
	  if(fabs(min)<poptions.lineinterval/10) min=0.;
	  if(fabs(max)<poptions.lineinterval/10) max=0.;
	  ostr <<min<<" - "<<max;
	  max=min;
	  vtable[i].text = ostr.str();
	}

	//colours
	if(cmin>poptions.base){
	  float step = ncodes*poptions.lineinterval;
	  min = int((cmin-poptions.base)/step)*step+poptions.base;
	  if(cmax+cmin > 2*(min+step)) min += step;
	}else{
	  min=poptions.base;
 	}
	for(int i=ncold; i<ncodes+ncold; i++){
	  max = min + poptions.lineinterval;
	  ostringstream ostr;
	  ostr <<min<<" - "<<max;
	  min=max;
	  vtable[i].text = ostr.str();
	}
      }


      int n= vtable.size();
      for( int i=n-1; i>-1; i--){
	str +=";";
	str +=vtable[i].colour;
	str +=";";
	str +=vtable[i].pattern;
	str +=";";
	str +=vtable[i].text;
      }
      str += "\"";
      str += endString;

      anno.push_back(str);

    }
  }
  return true;
}

bool FieldPlot::getDataAnnotations(vector<miString>& anno)
{
  //  cerr <<"getDataAnnotations:"<<anno.size()<<endl;

  if (fields.size()==0 || !fields[0] || !fields[0]->data)
    return false;

  int nanno = anno.size();
  for(int j=0; j<nanno; j++){
    if (anno[j].contains("arrow") && vectorAnnotationSize>0. && vectorAnnotationText.exists()) {
      if(anno[j].contains("arrow="))continue;

      miString endString;
      miString startString;
      if(anno[j].contains(",")){
        size_t nn = anno[j].find_first_of(",");
        endString = anno[j].substr(nn);
        startString =anno[j].substr(0,nn);
      } else {
        startString =anno[j];
      }
//       //if asking for spesific field
//       if(anno[j].contains("arrow=")){
//         miString name = startString.substr(startString.find_first_of("=")+1);
//         if( name[0]=='"' )
// 	  name.remove('"');
//         name.trim();
//         if(!fields[0]->fieldText.contains(name)) continue;
//       }

      miString str  = "arrow=" + miString (vectorAnnotationSize)
	+ ",tcolour=" + poptions.linecolour.Name() + endString;
      anno.push_back(str);
      str = "text=\" " + vectorAnnotationText + "\""
	+ ",tcolour=" + poptions.linecolour.Name() + endString ;
      anno.push_back(str);

    }
  }

  return true;
}


bool FieldPlot::plot(){
#ifdef DEBUGPRINT
  cerr << "++ FieldPlot::plot() ++" << endl;
#endif
  int n= fields.size();

  if (!enabled || n<1) return false;

  // possibly smooth all "single" fields
  for (int i=0; i<n; i++) {
    if (fields[i] && fields[i]->data) {
      if (fields[i]->numSmoothed<poptions.fieldSmooth) {
	int nsmooth= poptions.fieldSmooth - fields[i]->numSmoothed;
        fields[i]->smooth(nsmooth);
      } else if (fields[i]->numSmoothed>poptions.fieldSmooth) {
	cerr << endl;
	cerr << "FieldPlot::plot ERROR: field smoothed too much !!!" << endl;
	cerr << "    numSmoothed,fieldSmooth: "
	     << fields[i]->numSmoothed << " " << poptions.fieldSmooth << endl;
	cerr << endl;
      }
    }
  }

  // avoid background colour
  if (poptions.bordercolour==backgroundColour)
    poptions.bordercolour= backContrastColour;
  if (poptions.linecolour==backgroundColour)
    poptions.linecolour= backContrastColour;
  for (int i=0; i<poptions.colours.size(); i++)
    if (poptions.colours[i]==backgroundColour)
      poptions.colours[i]= backContrastColour;

  // should be below all real fields ???  colour etc ??????
  if (poptions.gridLines>0) plotGridLines();

  if       (ptype==fpt_contour)          return plotContour();
  else if (ptype==fpt_wind)             return plotWind();
  else if (ptype==fpt_wind_colour)      return plotWindColour();
  else if (ptype==fpt_wind_temp_fl)     return plotWindTempFL();
  else if (ptype==fpt_vector)           return plotVector();
  else if (ptype==fpt_vector_colour)    return plotVectorColour();
  else if (ptype==fpt_direction)        return plotDirection();
  else if (ptype==fpt_direction_colour) return plotDirectionColour();
  else if (ptype==fpt_box_pattern)      return plotBox_pattern();
  else if (ptype==fpt_box_alpha_shade)  return plotBox_alpha_shade();
  else if (ptype==fpt_alpha_shade)      return plotAlpha_shade();
  else if (ptype==fpt_alarm_box)        return plotAlarmBox();
  else return false;
}


vector<float*> FieldPlot::prepareVectors(int nfields, float* x, float* y)
{
#ifdef DEBUGPRINT
  cerr << "++ FieldPlot::prepareVectors() ++" << endl;
#endif
  vector<float*> uv;

  float *u=0, *v=0;

  int nf= fields.size();

  if (fields[0]->area.P() == area.P()) {
    u= fields[0]->data;
    v= fields[1]->data;
    if (nf==nfields+2) {
      delete fields[nf-1];
      delete fields[nf-2];
      fields.pop_back();
      fields.pop_back();
    }
  } else if (nf==nfields+2 &&
  	     fields[nfields]->numSmoothed == fields[0]->numSmoothed &&
  	     fields[nfields]->area.P() == area.P()) {
    u= fields[nfields  ]->data;
    v= fields[nfields+1]->data;
  } else {
    if (nf==nfields) {
      Field* utmp= new Field();
      Field* vtmp= new Field();
      fields.push_back(utmp);
      fields.push_back(vtmp);
    }
    *(fields[nfields  ])= *(fields[0]);
    *(fields[nfields+1])= *(fields[1]);
    u= fields[nfields  ]->data;
    v= fields[nfields+1]->data;
    int npos= fields[0]->nx * fields[0]->ny;
    gc.getVectors(fields[nfields]->area,area,npos,x,y,u,v);
    fields[nfields  ]->area.setP(area.P());
    fields[nfields+1]->area.setP(area.P());
    fixGeoVector(u,v);
  }
  uv.push_back(u);
  uv.push_back(v);
#ifdef DEBUGPRINT
  cerr << "++ FieldPlot::prepareVectors() finished ++" << endl;
#endif
  return uv;
}


vector<float*> FieldPlot::prepareDirectionVectors(int nfields, float* x, float* y)
{
#ifdef DEBUGPRINT
  cerr << "++ FieldPlot::prepareDirectionVectors() ++" << endl;
#endif
  vector<float*> uv;

  float *u=0, *v=0;

  int nf= fields.size();

  if (nf==nfields+2 &&
      fields[nfields]->numSmoothed == fields[0]->numSmoothed &&
      fields[nfields]->area.P() == area.P()) {
    u= fields[nfields  ]->data;
    v= fields[nfields+1]->data;
  } else {
    if (nf==nfields) {
      Field* utmp= new Field();
      Field* vtmp= new Field();
      fields.push_back(utmp);
      fields.push_back(vtmp);
    }
    *(fields[nfields  ])= *(fields[0]);
    *(fields[nfields+1])= *(fields[0]);
    u= fields[nfields  ]->data;
    v= fields[nfields+1]->data;
    int npos= fields[0]->nx * fields[0]->ny;
    for (int i=0; i<npos; i++)
      v[i]= 1.0f;

    //##################################################################
    // some ugly hardcoding until...
    // for ECMWF wave directions as "meteorological from-direction",
    // data interpolated to polarstereographic grids are usually turned
    // met.no WAM wave directions are "oceanographic to-direction".
    bool turn= (fields[0]->producer==98 &&
                fields[0]->area.P().Gridtype()==Projection::geographic);
    //##################################################################

    gc.getDirectionVectors(area,turn,npos,x,y,u,v);
    fields[nfields  ]->area.setP(area.P());
    fields[nfields+1]->area.setP(area.P());
    fixGeoVector(u,v);
  }
  uv.push_back(u);
  uv.push_back(v);
#ifdef DEBUGPRINT
  cerr << "++ FieldPlot::prepareDirectionVectors() finished ++" << endl;
#endif
  return uv;
}


void FieldPlot::setAutoStep(float* x, float* y, int ix1, int ix2, int iy1, int iy2,
			    int maxElementsX, int& step, float& dist, bool& xStepComp)
{
  int i,ix,iy;
  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  if (nx<3 || ny<3) {
    step= 1;
    dist= 1.;
    return;
  }

  int mx= ix2-ix1;
  int my= iy2-iy1;
  int ixstep = (mx>5) ? mx/5 : 1;
  int iystep = (my>5) ? my/5 : 1;

  if (mx>5) {
    if (ix1<2)    ix1= 2;
    if (ix2>nx-3) ix2= nx-3;
  }
  if (my>5) {
    if (iy1<2)    iy1= 2;
    if (iy2>ny-3) iy2= ny-3;
  }

  float dx, dy;
  float adx=0.0, ady= 0.0;

  mx= (ix2-ix1+ixstep-1)/ixstep;
  my= (iy2-iy1+iystep-1)/iystep;

  for (iy=iy1; iy<iy2; iy+=iystep) {
    for (ix=ix1; ix<ix2; ix+=ixstep) {
      i = iy*nx+ix;
      dx = x[i+1]-x[i];
      dy = y[i+1]-y[i];
      adx += sqrtf(dx*dx+dy*dy);
      dx = x[i+nx]-x[i];
      dy = y[i+nx]-y[i];
      ady += sqrtf(dx*dx+dy*dy);
    }
  }

  adx/=float(mx*my);
  ady/=float(mx*my);

  dist = (adx+ady)*0.5;
  // 40 winds or 55 arrows if 1000 pixels
  float numElements = float(maxElementsX)*float(pwidth)/1000.;
  float elementSize = fullrect.width()/numElements;
  step = int(elementSize/dist + 0.75);
  if (step<1) step=1;

  // probably too simple test...
  xStepComp= (fields[0]->area.P().Gridtype()==Projection::geographic ||
              fields[0]->area.P().Gridtype()==Projection::mercator) &&
             area.P().Gridtype()!=Projection::geographic &&
             area.P().Gridtype()!=Projection::mercator;
}


int FieldPlot::xAutoStep(float* x, float* y, int ix1, int ix2, int iy, float sdist)
{
  int xstep;
  int i,ix;
  int nx= fields[0]->nx;

  if (nx<3) {
    return 1;
  }

  int mx= ix2-ix1;
  int ixstep = (mx>5) ? mx/5 : 1;

  if (mx>5) {
    if (ix1<2)    ix1= 2;
    if (ix2>nx-3) ix2= nx-3;
  }

  float dx, dy;
  float adx=0.0f;

  mx= (ix2-ix1+ixstep-1)/ixstep;

  for (ix=ix1; ix<ix2; ix+=ixstep) {
    i = iy*nx+ix;
    dx = x[i+1]-x[i];
    dy = y[i+1]-y[i];
    adx += sqrtf(dx*dx+dy*dy);
  }

  adx/=float(mx);

  if(adx>0.0) {
    xstep= int(sdist/adx+0.75);
    if (xstep<1) xstep=1;
  } else {
    xstep=nx;
  }

  return xstep;
}


void FieldPlot::fixGeoVector(float* u, float* v)
{
#ifdef DEBUGPRINT
  cerr << "++ FieldPlot::fixGeoVector() ++" << endl;
#endif
  // (ECMWF) wind/vector in geographic grid on polarstereographic
  // or rotated spherical map:
  // A "line" of winds at the poles will be plotted in many
  // directions due to the grid longitudes.

  int maptype= area.P().Gridtype();

  if (fields[0]->area.P().Gridtype()!=Projection::geographic ||
      fields[0]->area.P().Gridtype()!=Projection::mercator   ||
      maptype==Projection::geographic ||
      maptype==Projection::mercator)
    return;

  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  float gs[Projection::speclen];
  fields[0]->area.P().Gridspecstd(gs);

  // check if wrapping the globe and avoid (near) doubleplotting
  if (fabsf(gs[2])*float(nx-1)-360.0f < fabsf(gs[2]*0.2f)) {
    int ix= nx-1;
    for (int iy=0; iy<ny; iy++)
      u[iy*nx+ix]= v[iy*nx+ix]= fieldUndef;
  }

  bool mapNorth= false;
  bool mapSouth= false;
  if (maptype==Projection::polarstereographic_60 ||
      maptype==Projection::polarstereographic) {
    if (gs[4]>=0.0) mapNorth= true;
    else            mapSouth= true;
  } else if (maptype==Projection::spherical_rotated) {
    if (gs[5]>=0.0) mapNorth= true;
    else            mapSouth= true;
  }

  float precis= fabsf(gs[3])*0.2f;

  for (int iy=0; iy<ny; iy+=(ny-1)) {
    // check if pole
    float glat= gs[1] + gs[3]*float(iy);
    if (glat>90.0-precis || glat<-90.0+precis) {
      if ((glat<0.0 && mapNorth) ||
          (glat>0.0 && mapSouth)) {
        // avoid impossible positions on polarstereographic maps
        for (int ix=0; ix<nx; ix++)
          u[iy*nx+ix]= v[iy*nx+ix]= fieldUndef;
      } else {
        int ix= 0;
        if (fields[0]->data[iy*nx+ix]!=fieldUndef &&
       	    fields[1]->data[iy*nx+ix]!=fieldUndef) {
          float uu= fields[0]->data[iy*nx+ix];
          float vv= fields[1]->data[iy*nx+ix];
          float rad= 3.141592654/180.;
          float deg= 180./3.141592654;
          float ff= sqrtf(uu*uu+vv*vv);
          // don't really know if it should be +gs[0] or -gs[0] !!!!!!!!
          float dd= 270.0 - deg*atan2f(vv,uu) + gs[0];
          uu= -ff*sin(dd*rad);
          vv= -ff*cos(dd*rad);
       	  for (int ix=0; ix<nx; ix++) {
       	    if (u[iy*nx+ix]!=fieldUndef) u[iy*nx+ix]= uu;
            if (v[iy*nx+ix]!=fieldUndef) v[iy*nx+ix]= vv;
    	  }
       	}
      }
    }
  }
#ifdef DEBUGPRINT
  cerr << "++ FieldPlot::fixGeoVector() finished ++" << endl;
#endif
}


//  plot vector field as wind arrows
bool FieldPlot::plotWind(){
#ifdef DEBUGPRINT
  cerr << "++ Plotter vind-felt.." << endl;
#endif
  int n= fields.size();
  if (n<2) return false;
  if (!fields[0] || !fields[1]) return false;

  if (!fields[0]->data || !fields[1]->data) return false;

  int i,ix,iy;
  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  // convert gridpoints to correct projection
  int npos=0;
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  gc.getGridPoints(fields[0]->area, area, maprect, false,
		   npos, &x, &y, ix1, ix2, iy1, iy2);
  if (ix1>ix2 || iy1>iy2) return false;

  // convert windvectors to correct projection
  vector<float*> uv= prepareVectors(2,x,y);
  if (uv.size()!=2) return false;
  float *u= uv[0];
  float *v= uv[1];

  int step= poptions.density;

  // automatic wind/vector density
  int autostep;
  float dist;
  bool xStepComp;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxWindsAuto, autostep, dist, xStepComp);
  if (step<1) step= autostep;
  float sdist= dist*float(step);
  int xstep= step;

  plotFrame(nx,ny,x,y,2,NULL);

  float relflaglen = poptions.relsize;

  int   n50,n10,n05;
  float ff,gu,gv,gx,gy,dx,dy,dxf,dyf;
  float flagl = sdist * relflaglen * 0.85;
  float flagstep = flagl/10.;
  float flagw = flagl * 0.35;
  float hflagw = 0.6;

  vector<float> vx,vy; // keep vertices for 50-knot flags

  ix1-=step;     if (ix1<0)  ix1=0;
  iy1-=step;     if (iy1<0)  iy1=0;
  ix2+=(step+1); if (ix2>nx) ix2=nx;
  iy2+=(step+1); if (iy2>ny) iy2=ny;

  maprect.setExtension( flagl );

  glLineWidth(poptions.linewidth+0.1);  // +0.1 to avoid MesaGL coredump
  glColor3ubv(poptions.linecolour.RGB());

  glBegin(GL_LINES);

  for (iy=iy1; iy<iy2; iy+=step){
    if (xStepComp) xstep= xAutoStep(x,y,ix1,ix2,iy,sdist);
    for (ix=ix1; ix<ix2; ix+=xstep){
      i= iy*nx+ix;
      gx= x[i]; gy= y[i];
      if (u[i]!=fieldUndef &&
          v[i]!=fieldUndef && maprect.isnear(gx,gy)){
	ff= sqrtf(u[i]*u[i]+v[i]*v[i]);
	if (ff>0.00001){

	  gu= u[i]/ff;
	  gv= v[i]/ff;

	  ff *= 3600.0/1852.0;

          // find no. of 50,10 and 5 knot flags
          if (ff<182.49) {
              n05  = int(ff*0.2 + 0.5);
              n50  = n05/10;
              n05 -= n50*10;
              n10  = n05/2;
	      n05 -= n10*2;
          } else if (ff<190.) {
              n50 = 3;  n10 = 3;  n05 = 0;
          } else if(ff<205.) {
              n50 = 4;  n10 = 0;  n05 = 0;
          } else if (ff<225.) {
              n50 = 4;  n10 = 1;  n05 = 0;
          } else {
	      n50 = 5;  n10 = 0;  n05 = 0;
          }

	  dx = flagstep*gu;
          dy = flagstep*gv;
	  dxf = -flagw*gv - dx;
          dyf =  flagw*gu - dy;

          // direction
	  glVertex2f(gx,gy);
	  gx = gx - flagl*gu;
          gy = gy - flagl*gv;
	  glVertex2f(gx,gy);

	  // 50-knot flags, store for plot below
          if (n50>0) {
 	    for (n=0; n<n50; n++) {
	      vx.push_back(gx);
	      vy.push_back(gy);
	      gx+=dx*2.;  gy+=dy*2.;
	      vx.push_back(gx+dxf);
	      vy.push_back(gy+dyf);
	      vx.push_back(gx);
	      vy.push_back(gy);
	    }
	    gx+=dx; gy+=dy;
	  }
	  // 10-knot flags
	  for (n=0; n<n10; n++) {
	    glVertex2f(gx,gy);
	    glVertex2f(gx+dxf,gy+dyf);
	    gx+=dx; gy+=dy;
	  }
	  // 5-knot flag
	  if (n05>0) {
	    if (n50+n10==0) { gx+=dx; gy+=dy; }
	    glVertex2f(gx,gy);
	    glVertex2f(gx+hflagw*dxf,gy+hflagw*dyf);
	  }
	}
      }
    }
  }
  glEnd();

  UpdateOutput();

  // draw 50-knot flags
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  int vi= vx.size();
  if (vi>=3){
    glBegin(GL_TRIANGLES);
    for (i=0; i<vi; i++)
      glVertex2f(vx[i],vy[i]);
    glEnd();
  }

  UpdateOutput();

  glDisable(GL_LINE_STIPPLE);

#ifdef DEBUGPRINT
  cerr << "++ Returning from FieldPlot::plotWind() ++" << endl;
#endif
  return true;
}


// plot vector field as wind arrows
// Fields u(0) v(1) colorfield(2)
bool FieldPlot::plotWindColour(){
#ifdef DEBUGPRINT
  cerr << "++ Plotter vind vector-felt.." << endl;
#endif
  int n= fields.size();
  if (n<3) return false;
  if (!fields[0] || !fields[1] || !fields[2]) return false;

  if (!fields[0]->data || !fields[1]->data
                       || !fields[2]->data) return false;

  int i,ix,iy,l;
  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  // convert gridpoints to correct projection
  int npos=0;
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  gc.getGridPoints(fields[0]->area, area, maprect, false,
		   npos, &x, &y, ix1, ix2, iy1, iy2);
  if (ix1>ix2 || iy1>iy2) return false;

  // convert windvectors to correct projection
  vector<float*> uv= prepareVectors(3,x,y);
  if (uv.size()!=2) return false;
  float *u= uv[0];
  float *v= uv[1];

  int step= poptions.density;

  // automatic wind/vector density
  int autostep;
  float dist;
  bool xStepComp;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxWindsAuto, autostep, dist, xStepComp);
  if (step<1) step= autostep;
  float sdist= dist*float(step);
  int xstep= step;

  plotFrame(nx,ny,x,y,2,NULL);

  float* limits=0;
  GLfloat* rgb=0;

  int nlim= poptions.limits.size();
  int ncol= poptions.colours.size();

  if (nlim>=1 && ncol>=2) {

    if (nlim>ncol-1) nlim= ncol-1;
    if (ncol>nlim+1) ncol= nlim+1;
    limits= new float[nlim];
    rgb=    new GLfloat[ncol*3];
    for (i=0; i<nlim; i++)
      limits[i]= poptions.limits[i];
    for (i=0; i<ncol; i++) {
      rgb[i*3+0]=  poptions.colours[i].fR();
      rgb[i*3+1]=  poptions.colours[i].fG();
      rgb[i*3+2]=  poptions.colours[i].fB();
    }

  } else {

    // default, should be handled when reading setup, if allowed...
    const int maxdef= 4;
    float rgbdef[maxdef][3]={{0.5,0.5,0.5},{0,0,0},{0,1,1},{1,0,0}};

    ncol= maxdef;
    nlim= maxdef-1;

    float fmin=fieldUndef, fmax=-fieldUndef;
    for (i=0; i<nx*ny; ++i) {
      if (fields[2]->data[i]!=fieldUndef) {
        if (fmin > fields[2]->data[i]) fmin=fields[2]->data[i];
        if (fmax < fields[2]->data[i]) fmax=fields[2]->data[i];
      }
    }
    if (fmin>fmax) return false;

    limits= new float[nlim];
    rgb=    new GLfloat[ncol*3];
    float dlim= (fmax-fmin)/float(ncol);
    for (i=0; i<nlim; i++)
      limits[i]= fmin + dlim*float(i+1);
    for (i=0; i<ncol; i++) {
      rgb[i*3+0]= rgbdef[i][0];
      rgb[i*3+1]= rgbdef[i][1];
      rgb[i*3+2]= rgbdef[i][2];
    }

  }

  float relflaglen = poptions.relsize;

  int   n50,n10,n05;
  float ff,gu,gv,gx,gy,dx,dy,dxf,dyf;
  float flagl = sdist * relflaglen * 0.85;
  float flagstep = flagl/10.;
  float flagw = flagl * 0.35;
  float hflagw = 0.6;

  vector<float> vx,vy; // keep vertices for 50-knot flags
  vector<int>   vc;    // keep the colour too

  ix1-=step;     if (ix1<0)  ix1=0;
  iy1-=step;     if (iy1<0)  iy1=0;
  ix2+=(step+1); if (ix2>nx) ix2=nx;
  iy2+=(step+1); if (iy2>ny) iy2=ny;

  maprect.setExtension( flagl );

  glLineWidth(poptions.linewidth+0.1);  // +0.1 to avoid MesaGL coredump

  glBegin(GL_LINES);

  for (iy=iy1; iy<iy2; iy+=step){
    if (xStepComp) xstep= xAutoStep(x,y,ix1,ix2,iy,sdist);
    for (ix=ix1; ix<ix2; ix+=xstep){
      i= iy*nx+ix;
      gx= x[i]; gy= y[i];
      if (u[i]!=fieldUndef && v[i]!=fieldUndef && maprect.isnear(gx,gy)){
	ff= sqrtf(u[i]*u[i]+v[i]*v[i]);
	if (ff>0.00001 && fields[2]->data[i]!=fieldUndef){

	  gu= u[i]/ff;
	  gv= v[i]/ff;

	  ff *= 3600.0/1852.0;

          // find no. of 50,10 and 5 knot flags
          if (ff<182.49) {
              n05  = int(ff*0.2 + 0.5);
              n50  = n05/10;
              n05 -= n50*10;
              n10  = n05/2;
	      n05 -= n10*2;
          } else if (ff<190.) {
              n50 = 3;  n10 = 3;  n05 = 0;
          } else if(ff<205.) {
              n50 = 4;  n10 = 0;  n05 = 0;
          } else if (ff<225.) {
              n50 = 4;  n10 = 1;  n05 = 0;
          } else {
	      n50 = 5;  n10 = 0;  n05 = 0;
          }

	  l=0;
	  while (l<nlim && fields[2]->data[i]>limits[l]) l++;
	  glColor3fv(&rgb[l*3]);

	  dx = flagstep*gu;
          dy = flagstep*gv;
	  dxf = -flagw*gv - dx;
          dyf =  flagw*gu - dy;

          // direction
	  glVertex2f(gx,gy);
	  gx = gx - flagl*gu;
          gy = gy - flagl*gv;
	  glVertex2f(gx,gy);

	  // 50-knot flags, store for plot below
          if (n50>0) {
 	    for (n=0; n<n50; n++) {
	      vc.push_back(l);
	      vx.push_back(gx);
	      vy.push_back(gy);
	      gx+=dx*2.;  gy+=dy*2.;
	      vx.push_back(gx+dxf);
	      vy.push_back(gy+dyf);
	      vx.push_back(gx);
	      vy.push_back(gy);
	    }
	    gx+=dx; gy+=dy;
	  }
	  // 10-knot flags
	  for (n=0; n<n10; n++) {
	    glVertex2f(gx,gy);
	    glVertex2f(gx+dxf,gy+dyf);
	    gx+=dx; gy+=dy;
	  }
	  // 5-knot flag
	  if (n05>0) {
	    if (n50+n10==0) { gx+=dx; gy+=dy; }
	    glVertex2f(gx,gy);
	    glVertex2f(gx+hflagw*dxf,gy+hflagw*dyf);
	  }
	}
      }
    }
  }
  glEnd();

  UpdateOutput();

  // draw 50-knot flags
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  int vi= vx.size(), j=0;
  if (vi>=3){
    glBegin(GL_TRIANGLES);
    for (i=0; i<vi; i+=3){
      glColor3fv(&rgb[vc[j++]*3]);
      glVertex2f(vx[i  ],vy[i  ]);
      glVertex2f(vx[i+1],vy[i+1]);
      glVertex2f(vx[i+2],vy[i+2]);
    }
    glEnd();
  }

  UpdateOutput();

  delete[] rgb;
  delete[] limits;

  glDisable(GL_LINE_STIPPLE);

#ifdef DEBUGPRINT
  cerr << "++ Returning from FieldPlot::plotWindColour() ++" << endl;
#endif
  return true;
}


/*
  ROUTINE:   FieldPlot::plotWindTempFL
  PURPOSE:   plot vector field as wind arrows and temp. as "number",
             a standard FlighLevel chart.
             T=-25  plott: 25
             T=+10  plott: ps10
  ALGORITHM: Fields u(0) v(1) temp(2)
*/

bool FieldPlot::plotWindTempFL(){
#ifdef DEBUGPRINT
  cerr << "++ Plotter vind+temp.fl" << endl;
#endif
  int n= fields.size();
  if (n<3) return false;
  if (!fields[0] || !fields[1] || !fields[2]) return false;

  if (!fields[0]->data || !fields[1]->data
                       || !fields[2]->data) return false;

  int i,ix,iy;
  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  // convert gridpoints to correct projection
  int npos=0;
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  gc.getGridPoints(fields[0]->area, area, maprect, false,
		   npos, &x, &y, ix1, ix2, iy1, iy2);
  if (ix1>ix2 || iy1>iy2) return false;

  // convert windvectors to correct projection
  vector<float*> uv= prepareVectors(3,x,y);
  if (uv.size()!=2) return false;
  float *u= uv[0];
  float *v= uv[1];

  int step= poptions.density;

  // automatic wind/vector density if step<1
  int autostep;
  float dist;
  bool xStepComp;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxWindsAuto, autostep, dist, xStepComp);
  if (step<1) step= autostep;
  float sdist= dist*float(step);
  int xstep= step;

  plotFrame(nx,ny,x,y,2,NULL);

  float relflaglen = poptions.relsize;

  int   n50,n10,n05;
  float ff,gu,gv,gx,gy,dx,dy,dxf,dyf;
  float flagl = sdist * relflaglen * 0.85;
  float flagstep = flagl/10.;
  float flagw = flagl * 0.35;
  float hflagw = 0.6;

  vector<float> vx,vy; // keep vertices for 50-knot flags

  ix1-=step;     if (ix1<0)  ix1=0;
  iy1-=step;     if (iy1<0)  iy1=0;
  ix2+=(step+1); if (ix2>nx) ix2=nx;
  iy2+=(step+1); if (iy2>ny) iy2=ny;

  if (xStepComp) {
    // avoid double plotting of wind and number
    iy=(iy1+iy2)/2;
    int i1= iy*nx+ix1;
    int i2= iy*nx+ix2-1;
    if (fabsf(x[i1]-x[i2])<0.01 &&
        fabsf(y[i1]-y[i2])<0.01) ix2--;
  }

  maprect.setExtension( flagl );

  float fontsize= 14. * poptions.labelSize;

//fp->set(poptions.fontname,poptions.fontface,fontsize);
  fp->set("Helvetica",poptions.fontface,fontsize);

  float chx,chy;
  fp->getStringSize("ps00", chx, chy);

  if (chx*1.4>sdist)
    fp->setFontSize(fontsize*sdist/(chx*1.4));
  else if (chy*0.8>dist)
    fp->setFontSize(fontsize*sdist/(chy*0.8));

  fp->getCharSize('0',chx,chy);

  // the real height for numbers 0-9 (width is ok)
  chy *= 0.75;

  // bit matrix used to avoid numbers plotted across wind and numbers
  const int nbitwd= sizeof(int)*8;
  float bres= 1.0/(chx*0.5);
  float bx= maprect.x1 - flagl*2.5;
  float by= maprect.y1 - flagl*2.5;
  if (bx>=0.0f) bx= float(int(bx*bres))/bres;
  else          bx= float(int(bx*bres-1.0f))/bres;
  if (by>=0.0f) by= float(int(by*bres))/bres;
  else          by= float(int(by*bres-1.0f))/bres;
  int nbx= int((maprect.x2 + flagl*2.5 - bx)*bres) + 3;
  int nby= int((maprect.y2 + flagl*2.5 - by)*bres) + 3;
  int nbmap= (nbx*nby+nbitwd-1)/nbitwd;
  int *bmap= new int[nbmap];
  for (i=0; i<nbmap; i++) bmap[i]= 0;
  int m,ib,jb,ibit,iwrd,nb;
  float xb[20][2], yb[20][2];

  vector<int> vxstep;

  glLineWidth(poptions.linewidth+0.1);  // +0.1 to avoid MesaGL coredump
  glColor3ubv(poptions.linecolour.RGB());

  // plot wind............................................

  glBegin(GL_LINES);

  //LB: Wind arrows are adjusted to lat=10 and Lon=10 if
  //poptions.density!=auto and proj=geographic
  bool adjustToLatLon = poptions.density
    && fields[0]->area.P().Gridtype() == Projection::geographic
    && step > 0;
  if(adjustToLatLon) iy1 = (iy1/step)*step;
  for (iy=iy1; iy<iy2; iy+=step){
    if (xStepComp) {
      xstep= xAutoStep(x,y,ix1,ix2,iy,sdist);
      if(adjustToLatLon){
	xstep = (xstep/step)*step;
	if(xstep==0) xstep=step;
	ix1 = (ix1/xstep)*xstep;
      }
      vxstep.push_back(xstep);
    }
    for (ix=ix1; ix<ix2; ix+=xstep){
      i= iy*nx+ix;
      gx= x[i]; gy= y[i];
      if (u[i]!=fieldUndef && v[i]!=fieldUndef && maprect.isnear(gx,gy)){
	ff= sqrtf(u[i]*u[i]+v[i]*v[i]);
	if (ff>0.00001){

	  gu= u[i]/ff;
	  gv= v[i]/ff;

	  ff *= 3600.0/1852.0;

          // find no. of 50,10 and 5 knot flags
          if (ff<182.49) {
              n05  = int(ff*0.2 + 0.5);
              n50  = n05/10;
              n05 -= n50*10;
              n10  = n05/2;
	      n05 -= n10*2;
          } else if (ff<190.) {
              n50 = 3;  n10 = 3;  n05 = 0;
          } else if(ff<205.) {
              n50 = 4;  n10 = 0;  n05 = 0;
          } else if (ff<225.) {
              n50 = 4;  n10 = 1;  n05 = 0;
          } else {
	      n50 = 5;  n10 = 0;  n05 = 0;
          }

	  dx = flagstep*gu;
          dy = flagstep*gv;
	  dxf = -flagw*gv - dx;
          dyf =  flagw*gu - dy;

	  nb= 0;
	  xb[nb][0]= gx;
	  yb[nb][0]= gy;

          // direction
	  glVertex2f(gx,gy);
	  gx = gx - flagl*gu;
          gy = gy - flagl*gv;
	  glVertex2f(gx,gy);

	  xb[nb]  [1]= gx;
	  yb[nb++][1]= gy;

	  // 50-knot flags, store for plot below
          if (n50>0) {
 	    for (n=0; n<n50; n++) {
	      xb[nb][0]= gx;
	      yb[nb][0]= gy;
	      vx.push_back(gx);
	      vy.push_back(gy);
	      gx+=dx*2.;  gy+=dy*2.;
	      vx.push_back(gx+dxf);
	      vy.push_back(gy+dyf);
	      vx.push_back(gx);
	      vy.push_back(gy);
	      xb[nb]  [1]= gx+dxf;
	      yb[nb++][1]= gy+dyf;
	      xb[nb][0]= gx+dxf;
	      yb[nb][0]= gy+dyf;
	      xb[nb]  [1]= gx;
	      yb[nb++][1]= gy;
	    }
	    gx+=dx; gy+=dy;
	  }

	  // 10-knot flags
	  for (n=0; n<n10; n++) {
	    glVertex2f(gx,gy);
	    glVertex2f(gx+dxf,gy+dyf);
	    xb[nb][0]= gx;
	    yb[nb][0]= gy;
	    xb[nb]  [1]= gx+dxf;
	    yb[nb++][1]= gy+dyf;
	    gx+=dx; gy+=dy;
	  }
	  // 5-knot flag
	  if (n05>0) {
	    if (n50+n10==0) { gx+=dx; gy+=dy; }
	    glVertex2f(gx,gy);
	    glVertex2f(gx+hflagw*dxf,gy+hflagw*dyf);
	    xb[nb][0]= gx;
	    yb[nb][0]= gy;
	    xb[nb]  [1]= gx+hflagw*dxf;
	    yb[nb++][1]= gy+hflagw*dyf;
	  }

	  // mark used space (lines) in bitmap
	  for (n=0; n<nb; n++) {
	    dx= xb[n][1] - xb[n][0];
	    dy= yb[n][1] - yb[n][0];
	    if (fabsf(dx)>fabsf(dy))
	      m= int(fabsf(dx)*bres) + 2;
	    else
	      m= int(fabsf(dy)*bres) + 2;
	    dx/=float(m-1);
	    dy/=float(m-1);
	    gx= xb[n][0];
	    gy= yb[n][0];
	    for (i=0; i<m; i++) {
              ib= int((gx-bx)*bres);
              jb= int((gy-by)*bres);
	      ibit= jb*nbx+ib;
	      iwrd= ibit/nbitwd;
	      ibit= ibit%nbitwd;
	      bmap[iwrd]= bmap[iwrd] | (1<<ibit);
              gx+=dx;
	      gy+=dy;
	    }
	  }

	}
      }
    }
  }
  glEnd();

  UpdateOutput();

  // draw 50-knot flags
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  int vi= vx.size();
  if (vi>=3){
    glBegin(GL_TRIANGLES);
    for (i=0; i<vi; i++)
      glVertex2f(vx[i],vy[i]);
    glEnd();
  }

  UpdateOutput();

  // plot numbers.................................................

  float *t= fields[2]->data;

  //-----------------------------------------------------------------------
  //
  //  -----------  standard:  (* = gridpunkt)
  //  |  3   0  |  vind i kvadrant 0 => tall i kvadrant 3   (pos. 6)
  //  |    *    |  vind i kvadrant 1 => tall i kvadrant 0   (pos. 0)
  //  |  2   1  |  vind i kvadrant 2 => tall i kvadrant 1   (pos. 2)
  //  -----------  vind i kvadrant 3 => tall i kvadrant 2   (pos. 4)
  //
  //  -----------  hvis standard-posisjon ikke kan benyttes.
  //  |  6 7 0  |  leter en etter o.k. posisjon 8 andre steder
  //  |  5 * 1  |  (alle 'opptatt' => benytter minst opptatte posisjon)
  //  |  4 3 2  |  hvis vind ikke er plottet (ikke vind eller udefinert)
  //  -----------  plottes tallet midt over grid-punktet (*, pos. 8)
  //
  //-----------------------------------------------------------------------
  //
  //..nedre venstre hj@rne for tall med 'nch' tegn:
  //       xn = x0 + adx[ipos] + cdx[ipos] * text_width
  //       yn = y0 + ady[ipos]

  float hchy= chy*0.5;
  float d= chx*0.5;

  float adx[9]= {    d,   d,     d,  0.0f,    -d,   -d,   -d, 0.0f, 0.0f };
  float cdx[9]= { 0.0f,0.0f,  0.0f, -0.5f, -1.0f,-1.0f,-1.0f,-0.5f,-0.5f };
  float ady[9]= {    d,  -d,-d-chy,-d-chy,-d-chy,-hchy,    d,    d,-hchy };

  int j,ipos0,ipos1,ipos,ib1,ib2,jb1,jb2,mused,nused,temp;
  float x1,x2,y1,y2,w,h;
  int ivx=0;

  for (iy=iy1; iy<iy2; iy+=step){
    if (xStepComp) xstep= vxstep[ivx++];
    for (ix=ix1; ix<ix2; ix+=xstep){
      i= iy*nx+ix;
      gx= x[i]; gy= y[i];
      if (t[i]!=fieldUndef && maprect.isinside(gx,gy)){
        if (u[i]!=fieldUndef && v[i]!=fieldUndef)
	  ff= sqrtf(u[i]*u[i]+v[i]*v[i]);
	else
	  ff= 0.0f;
	if (ff>0.00001){
	  if      (u[i]>=0.0f && v[i]>=0.0f) ipos0= 2;
	  else if (u[i]>=0.0f)               ipos0= 4;
	  else if (v[i]>=0.0f)	             ipos0= 0;
	  else             	             ipos0= 6;
	  m= 8;
	} else {
	  ipos0= 8;
	  m= 9;
        }

	temp= (t[i]>=0.0f) ? int(t[i]+0.5f) : int(t[i]-0.5f);
	ostringstream ostr;
	if (temp<=0) ostr<<-temp;
	else         ostr<<"ps"<<temp;
	miString str= ostr.str();
        fp->getStringSize(str.c_str(), w, h);
//###########################################################################
//	  x1= gx + adx[ipos0] + cdx[ipos0]*w;
//	  y1= gy + ady[ipos0];
//	  x2= x1 + w;
//	  y2= y1 + chy;
//	  glBegin(GL_LINE_LOOP);
//	  glVertex2f(x1,y1);
//	  glVertex2f(x2,y1);
//	  glVertex2f(x2,y2);
//	  glVertex2f(x1,y2);
//	  glEnd();
//###########################################################################

	mused= nbx*nby;
	ipos1= ipos0;

	for (j=0; j<m; j++) {
	  ipos= (ipos0+j)%m;
	  x1= gx + adx[ipos] + cdx[ipos]*w;
	  y1= gy + ady[ipos];
	  x2= x1 + w;
	  y2= y1 + chy;
	  ib1= int((x1-bx)*bres);
	  ib2= int((x2-bx)*bres)+1;
	  jb1= int((y1-by)*bres);
	  jb2= int((y2-by)*bres)+1;
	  nused= 0;
	  for (jb=jb1; jb<jb2; jb++) {
	    for (ib=ib1; ib<ib2; ib++) {
	      ibit= jb*nbx+ib;
	      iwrd= ibit/nbitwd;
	      ibit= ibit%nbitwd;
	      if (bmap[iwrd] & (1<<ibit)) nused++;
	    }
	  }
	  if (nused<mused) {
	    mused= nused;
	    ipos1=ipos;
	    if (nused==0) break;
	  }
	}

	x1= gx + adx[ipos1] + cdx[ipos1]*w;
	y1= gy + ady[ipos1];
	x2= x1 + w;
	y2= y1 + chy;
	if (maprect.isinside(x1,y1) && maprect.isinside(x2,y2)) {
          fp->drawStr(str.c_str(),x1,y1,0.0);
	  // mark used space for number (line around)
	  ib1= int((x1-bx)*bres);
	  ib2= int((x2-bx)*bres)+1;
	  jb1= int((y1-by)*bres);
	  jb2= int((y2-by)*bres)+1;
	  for (jb=jb1; jb<jb2; jb++) {
	    for (ib=ib1; ib<ib2; ib++) {
	      ibit= jb*nbx+ib;
	      iwrd= ibit/nbitwd;
	      ibit= ibit%nbitwd;
	      bmap[iwrd]= bmap[iwrd] | (1<<ibit);
	    }
	  }
	}

      }
    }
  }

  UpdateOutput();

  delete[] bmap;

  glDisable(GL_LINE_STIPPLE);

#ifdef DEBUGPRINT
  cerr << "++ Returning from FieldPlot::plotWindTempFL() ++" << endl;
#endif
  return true;
}


//  plot vector field as arrows (wind,sea current,...)
bool FieldPlot::plotVector(){
#ifdef DEBUGPRINT
  cerr << "++ Plotter vector-felt.." << endl;
#endif
  int n= fields.size();
  if (n<2) return false;
  if (!fields[0] || !fields[1]) return false;

  if (!fields[0]->data || !fields[1]->data) return false;

  int i,ix,iy;
  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  // convert gridpoints to correct projection
  int npos=0;
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  gc.getGridPoints(fields[0]->area, area, maprect, false,
		   npos, &x, &y, ix1, ix2, iy1, iy2);
  if (ix1>ix2 || iy1>iy2) return false;

  // convert vectors to correct projection
  vector<float*> uv= prepareVectors(2,x,y);
  if (uv.size()!=2) return false;
  float *u= uv[0];
  float *v= uv[1];

  int step= poptions.density;

  // automatic wind/vector density
  int autostep;
  float dist;
  bool xStepComp;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxArrowsAuto, autostep, dist, xStepComp);
  if (step<1) step= autostep;
  float sdist= dist*float(step);
  int xstep= step;

  plotFrame(nx,ny,x,y,2,NULL);

  float relarrowlen = poptions.relsize;
  float unitlength  = poptions.vectorunit;

  // length if abs(vector) = unitlength
  float arrowlength = sdist * relarrowlen;

  float scale = arrowlength / unitlength;

  // for annotations .... should probably be resized if very small or large...
  vectorAnnotationSize= arrowlength;
  vectorAnnotationText= miString(unitlength) + " m/s";

  // for arrow tip
  const float afac = -0.333333;
  const float sfac = afac * 0.5;

  float gx,gy,dx,dy;

  ix1-=step;     if (ix1<0)  ix1=0;
  iy1-=step;     if (iy1<0)  iy1=0;
  ix2+=(step+1); if (ix2>nx) ix2=nx;
  iy2+=(step+1); if (iy2>ny) iy2=ny;

  maprect.setExtension( arrowlength*1.5 );

  glLineWidth(poptions.linewidth+0.1);  // +0.1 to avoid MesaGL coredump
  glColor3ubv(poptions.linecolour.RGB());

  glBegin(GL_LINES);

  for (iy=iy1; iy<iy2; iy+=step){
    if (xStepComp) xstep= xAutoStep(x,y,ix1,ix2,iy,sdist);
    for (ix=ix1; ix<ix2; ix+=xstep){
      i= iy*nx+ix;
      gx= x[i]; gy= y[i];
      if (u[i]!=fieldUndef && v[i]!=fieldUndef &&
          (u[i]!=0.0f || v[i]!=0.0f) && maprect.isnear(gx,gy)){
	dx = scale * u[i];
	dy = scale * v[i];

        // direction
	glVertex2f(gx,gy);
	gx = gx + dx;
        gy = gy + dy;
	glVertex2f(gx,gy);

	// arrow (drawn as two lines)
	glVertex2f(gx,gy);
	glVertex2f(gx + afac*dx + sfac*dy,
	           gy + afac*dy - sfac*dx);
	glVertex2f(gx,gy);
	glVertex2f(gx + afac*dx - sfac*dy,
		   gy + afac*dy + sfac*dx);
      }
    }
  }
  glEnd();

  UpdateOutput();

  glDisable(GL_LINE_STIPPLE);

#ifdef DEBUGPRINT
  cerr << "++ Returning from FieldPlot::plotVector() ++" << endl;
#endif
  return true;
}


// PURPOSE:   plot vector field as arrows (wind,sea current,...)
// ALGORITHM: Fields u(0) v(1) colorfield(2)
bool FieldPlot::plotVectorColour(){
#ifdef DEBUGPRINT
  cerr << "++ Plotter vector-felt.." << endl;
#endif
  int n= fields.size();
  if (n<3) return false;
  if (!fields[0] || !fields[1] || !fields[2]) return false;

  if (!fields[0]->data || !fields[1]->data
                       || !fields[2]->data) return false;

  int i,ix,iy,l;
  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  // convert gridpoints to correct projection
  int npos=0;
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  gc.getGridPoints(fields[0]->area, area, maprect, false,
		   npos, &x, &y, ix1, ix2, iy1, iy2);
  if (ix1>ix2 || iy1>iy2) return false;

  // convert vectors to correct projection
  vector<float*> uv= prepareVectors(3,x,y);
  if (uv.size()!=2) return false;
  float *u= uv[0];
  float *v= uv[1];

  int step= poptions.density;

  // automatic wind/vector density
  int autostep;
  float dist;
  bool xStepComp;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxArrowsAuto, autostep, dist, xStepComp);
  if (step<1) step= autostep;
  float sdist= dist*float(step);
  int xstep= step;

  plotFrame(nx,ny,x,y,2,NULL);

  float* limits=0;
  GLfloat* rgb=0;

  int nlim= poptions.limits.size();
  int ncol= poptions.colours.size();

  if (nlim>=1 && ncol>=2) {

    if (nlim>ncol-1) nlim= ncol-1;
    if (ncol>nlim+1) ncol= nlim+1;
    limits= new float[nlim];
    rgb=    new GLfloat[ncol*3];
    for (i=0; i<nlim; i++)
      limits[i]= poptions.limits[i];
    for (i=0; i<ncol; i++) {
      rgb[i*3+0]=  poptions.colours[i].fR();
      rgb[i*3+1]=  poptions.colours[i].fG();
      rgb[i*3+2]=  poptions.colours[i].fB();
    }

  } else {

    // default, should be handled when reading setup, if allowed...
    const int maxdef= 4;
    float rgbdef[maxdef][3]={{0.5,0.5,0.5},{0,0,0},{0,1,1},{1,0,0}};

    ncol= maxdef;
    nlim= maxdef-1;

    float fmin=fieldUndef, fmax=-fieldUndef;
    for (i=0; i<nx*ny; ++i) {
      if (fields[2]->data[i]!=fieldUndef) {
        if (fmin > fields[2]->data[i]) fmin=fields[2]->data[i];
        if (fmax < fields[2]->data[i]) fmax=fields[2]->data[i];
      }
    }
    if (fmin>fmax) return false;

    limits= new float[nlim];
    rgb=    new GLfloat[ncol*3];
    float dlim= (fmax-fmin)/float(ncol);
    for (i=0; i<nlim; i++)
      limits[i]= fmin + dlim*float(i+1);
    for (i=0; i<ncol; i++) {
      rgb[i*3+0]= rgbdef[i][0];
      rgb[i*3+1]= rgbdef[i][1];
      rgb[i*3+2]= rgbdef[i][2];
    }

  }

  float relarrowlen = poptions.relsize;
  float unitlength  = poptions.vectorunit;

  // length if abs(vector) = unitlength
  float arrowlength = sdist * relarrowlen;

  float scale = arrowlength / unitlength;

  // for annotations .... should probably be resized if very small or large...
  vectorAnnotationSize= arrowlength;
  vectorAnnotationText= miString(unitlength) + " m/s";

  // for arrow tip
  const float afac = -0.333333;
  const float sfac = afac * 0.5;

  float gx,gy,dx,dy;

  ix1-=step;     if (ix1<0)  ix1=0;
  iy1-=step;     if (iy1<0)  iy1=0;
  ix2+=(step+1); if (ix2>nx) ix2=nx;
  iy2+=(step+1); if (iy2>ny) iy2=ny;

  maprect.setExtension( arrowlength*1.5 );

  glLineWidth(poptions.linewidth+0.1);  // +0.1 to avoid MesaGL coredump

  glBegin(GL_LINES);

  for (iy=iy1; iy<iy2; iy+=step){
    if (xStepComp) xstep= xAutoStep(x,y,ix1,ix2,iy,sdist);
    for (ix=ix1; ix<ix2; ix+=xstep){
      i= iy*nx+ix;
      gx= x[i]; gy= y[i];

      if (u[i]!=fieldUndef && v[i]!=fieldUndef &&
          (u[i]!=0.0f || v[i]!=0.0f) &&
	  fields[2]->data[i]!=fieldUndef && maprect.isnear(gx,gy)){

	l=0;
	while (l<nlim && fields[2]->data[i]>limits[l]) l++;
	glColor3fv(&rgb[l*3]);

	dx = scale * u[i];
	dy = scale * v[i];

        // direction
	glVertex2f(gx,gy);
	gx = gx + dx;
        gy = gy + dy;
	glVertex2f(gx,gy);

	// arrow (drawn as two lines)
	glVertex2f(gx,gy);
	glVertex2f(gx + afac*dx + sfac*dy,
	           gy + afac*dy - sfac*dx);
	glVertex2f(gx,gy);
	glVertex2f(gx + afac*dx - sfac*dy,
		   gy + afac*dy + sfac*dx);
      }
    }
  }
  glEnd();

  UpdateOutput();

  delete[] rgb;
  delete[] limits;

  glDisable(GL_LINE_STIPPLE);

#ifdef DEBUGPRINT
  cerr << "++ Returning from FieldPlot::plotVectorColour() ++" << endl;
#endif
  return true;
}


/* plot true north direction field as arrows (wave,...) */
bool FieldPlot::plotDirection(){
#ifdef DEBUGPRINT
  cerr << "++ Plotter retnings-felt.." << endl;
#endif
  int n= fields.size();
  if (n<1) return false;
  if (!fields[0]) return false;

  if (!fields[0]->data) return false;

  int i,ix,iy;
  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  // convert gridpoints to correct projection
  int npos=0;
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  gc.getGridPoints(fields[0]->area, area, maprect, false,
		   npos, &x, &y, ix1, ix2, iy1, iy2);
  if (ix1>ix2 || iy1>iy2) return false;

  // convert directions to vectors in correct projection
  vector<float*> uv= prepareDirectionVectors(1,x,y);
  if (uv.size()!=2) return false;
  float *u= uv[0];
  float *v= uv[1];

  int step= poptions.density;

  // automatic wind/vector density
  int autostep;
  float dist;
  bool xStepComp;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxArrowsAuto, autostep, dist, xStepComp);
  if (step<1) step= autostep;
  float sdist= dist*float(step);
  int xstep= step;

  plotFrame(nx,ny,x,y,2,NULL);

  float relarrowlen = poptions.relsize;

  // length if abs(vector) = 1
  float arrowlength = sdist * relarrowlen;

  float scale = arrowlength;

  // for arrow tip
  const float afac = -0.333333;
  const float sfac = afac * 0.5;

  float gx,gy,dx,dy;

  ix1-=step;     if (ix1<0)  ix1=0;
  iy1-=step;     if (iy1<0)  iy1=0;
  ix2+=(step+1); if (ix2>nx) ix2=nx;
  iy2+=(step+1); if (iy2>ny) iy2=ny;

  maprect.setExtension( arrowlength*1.0 );

  glLineWidth(poptions.linewidth+0.1);  // +0.1 to avoid MesaGL coredump
  glColor3ubv(poptions.linecolour.RGB());

  glBegin(GL_LINES);

  for (iy=iy1; iy<iy2; iy+=step){
    if (xStepComp) xstep= xAutoStep(x,y,ix1,ix2,iy,sdist);
    for (ix=ix1; ix<ix2; ix+=xstep){
      i= iy*nx+ix;
      gx= x[i]; gy= y[i];
      if (u[i]!=fieldUndef && v[i]!=fieldUndef && maprect.isnear(gx,gy)){
	dx = scale * u[i];
	dy = scale * v[i];

        // direction
	glVertex2f(gx,gy);
	gx = gx + dx;
        gy = gy + dy;
	glVertex2f(gx,gy);

	// arrow (drawn as two lines)
	glVertex2f(gx,gy);
	glVertex2f(gx + afac*dx + sfac*dy,
	           gy + afac*dy - sfac*dx);
	glVertex2f(gx,gy);
	glVertex2f(gx + afac*dx - sfac*dy,
		   gy + afac*dy + sfac*dx);
      }
    }
  }
  glEnd();

  UpdateOutput();

  glDisable(GL_LINE_STIPPLE);

#ifdef DEBUGPRINT
  cerr << "++ Returning from FieldPlot::plotDirection() ++" << endl;
#endif
  return true;
}

/*
  PURPOSE:   plot true north direction field as arrows (wave,...)
  * ALGORITHM: Fields: dd(0) colourfield(1)
  */
bool FieldPlot::plotDirectionColour(){
#ifdef DEBUGPRINT
  cerr << "++ Plotter retnings-felt m. farge.." << endl;
#endif
  int n= fields.size();
  if (n<2) return false;
  if (!fields[0] || !fields[1]) return false;

  if (!fields[0]->data || !fields[1]->data) return false;

  int i,ix,iy,l;
  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  // convert gridpoints to correct projection
  int npos=0;
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  gc.getGridPoints(fields[0]->area, area, maprect, false,
		   npos, &x, &y, ix1, ix2, iy1, iy2);
  if (ix1>ix2 || iy1>iy2) return false;

  // convert directions to vectors in correct projection
  vector<float*> uv= prepareDirectionVectors(2,x,y);
  if (uv.size()!=2) return false;
  float *u= uv[0];
  float *v= uv[1];

  int step= poptions.density;

  // automatic wind/vector density
  int autostep;
  float dist;
  bool xStepComp;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxArrowsAuto, autostep, dist, xStepComp);
  if (step<1) step= autostep;
  float sdist= dist*float(step);
  int xstep= step;

  plotFrame(nx,ny,x,y,2,NULL);

  float* limits=0;
  GLfloat* rgb=0;

  int nlim= poptions.limits.size();
  int ncol= poptions.colours.size();

  if (nlim>=1 && ncol>=2) {

    if (nlim>ncol-1) nlim= ncol-1;
    if (ncol>nlim+1) ncol= nlim+1;
    limits= new float[nlim];
    rgb=    new GLfloat[ncol*3];
    for (i=0; i<nlim; i++)
      limits[i]= poptions.limits[i];
    for (i=0; i<ncol; i++) {
      rgb[i*3+0]=  poptions.colours[i].fR();
      rgb[i*3+1]=  poptions.colours[i].fG();
      rgb[i*3+2]=  poptions.colours[i].fB();
    }

  } else {

    // default, should be handled when reading setup, if allowed...
    const int maxdef= 4;
    float rgbdef[maxdef][3]={{0.5,0.5,0.5},{0,0,0},{0,1,1},{1,0,0}};

    ncol= maxdef;
    nlim= maxdef-1;

    float fmin=fieldUndef, fmax=-fieldUndef;
    for (i=0; i<nx*ny; ++i) {
      if (fields[1]->data[i]!=fieldUndef) {
        if (fmin > fields[1]->data[i]) fmin=fields[1]->data[i];
        if (fmax < fields[1]->data[i]) fmax=fields[1]->data[i];
      }
    }
    if (fmin>fmax) return false;

    limits= new float[nlim];
    rgb=    new GLfloat[ncol*3];
    float dlim= (fmax-fmin)/float(ncol);
    for (i=0; i<nlim; i++)
      limits[i]= fmin + dlim*float(i+1);
    for (i=0; i<ncol; i++) {
      rgb[i*3+0]= rgbdef[i][0];
      rgb[i*3+1]= rgbdef[i][1];
      rgb[i*3+2]= rgbdef[i][2];
    }

  }
//##############################################################
//  cerr<<"--------------"<<endl;
//  cerr<<" nlim,ncol: "<<nlim<<" "<<ncol<<endl;
//  for (i=0; i<nlim; i++)
//    cerr<<"   RGB: "<<rgb[i*3+0]<<" "<<rgb[i*3+1]<<" "<<rgb[i*3+2]
//        <<"  lim= "<<limits[i]<<endl;
//  i=nlim;
//  cerr<<"   RGB: "<<rgb[i*3+0]<<" "<<rgb[i*3+1]<<" "<<rgb[i*3+2]
//      <<endl;
//##############################################################

  float relarrowlen = poptions.relsize;

  // length if abs(vector) = 1
  float arrowlength = sdist * relarrowlen;

  float scale = arrowlength;

  // for arrow tip
  const float afac = -0.333333;
  const float sfac = afac * 0.5;

  float gx,gy,dx,dy;

  ix1-=step;     if (ix1<0)  ix1=0;
  iy1-=step;     if (iy1<0)  iy1=0;
  ix2+=(step+1); if (ix2>nx) ix2=nx;
  iy2+=(step+1); if (iy2>ny) iy2=ny;

  maprect.setExtension( arrowlength*1.0 );

  glLineWidth(poptions.linewidth+0.1);  // +0.1 to avoid MesaGL coredump

  glBegin(GL_LINES);

  for (iy=iy1; iy<iy2; iy+=step){
    if (xStepComp) xstep= xAutoStep(x,y,ix1,ix2,iy,sdist);
    for (ix=ix1; ix<ix2; ix+=xstep){
      i= iy*nx+ix;
      gx= x[i]; gy= y[i];
      if (u[i]!=fieldUndef && v[i]!=fieldUndef &&
	  fields[1]->data[i]!=fieldUndef && maprect.isnear(gx,gy)){

	l=0;
	while (l<nlim && fields[1]->data[i]>limits[l]) l++;
	glColor3fv(&rgb[l*3]);

	dx = scale * u[i];
	dy = scale * v[i];

        // direction
	glVertex2f(gx,gy);
	gx = gx + dx;
        gy = gy + dy;
	glVertex2f(gx,gy);

	// arrow (drawn as two lines)
	glVertex2f(gx,gy);
	glVertex2f(gx + afac*dx + sfac*dy,
	           gy + afac*dy - sfac*dx);
	glVertex2f(gx,gy);
	glVertex2f(gx + afac*dx - sfac*dy,
		   gy + afac*dy + sfac*dx);
      }
    }
  }
  glEnd();

  UpdateOutput();

  delete[] rgb;
  delete[] limits;

  glDisable(GL_LINE_STIPPLE);

#ifdef DEBUGPRINT
  cerr << "++ Returning from FieldPlot::plotDirectionColour() ++" << endl;
#endif
  return true;
}


//  plot scalar field as contour lines
bool FieldPlot::plotContour(){
#ifdef DEBUGPRINT
  cerr << "++ Plotter konturert felt.." << endl;
#endif

  int n= fields.size();
  if (n<1) return false;
  if (!fields[0]) return false;

  if (!fields[0]->data) return false;

  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  int ipart[4];

  const int mmm = 2;
  const int mmmUsed = 100;

  float xylim[4], chxlab, chylab;
  int ismooth, labfmt[3], ibcol;
  int idraw, nlines, nlim;
  int ncol, icol[mmmUsed], ntyp, ityp[mmmUsed], nwid, iwid[mmmUsed];
  float zrange[2], zstep, zoff, rlines[mmmUsed], rlim[mmm];
  int idraw2, nlines2, nlim2;
  int ncol2, icol2[mmm], ntyp2, ityp2[mmm], nwid2, iwid2[mmm];
  float zrange2[2], zstep2, zoff2, rlines2[mmm], rlim2[mmm];
  int   ibmap, lbmap, kbmap[mmm], nxbmap, nybmap;
  float rbmap[4];

  int   mapconvert, ix1, ix2, iy1, iy2;
  float cvfield2map[6], cvmap2field[6];
  bool  res= true;

  // find gridpoint conversion method
  if(!gc.getGridConversion(fields[0]->area, area, maprect, mapconvert,
		           cvfield2map, cvmap2field, ix1, ix2, iy1, iy2))
	return false;

  // convert gridpoints to correct projection
  int npos=0;
  float *x, *y;
  if (mapconvert==2) {
    if(!gc.getGridPoints(fields[0]->area, area, maprect, false,
			 npos, &x, &y, ix1, ix2, iy1, iy2)) return false;
  }

  if (ix1>=ix2 || iy1>=iy2) return false;
  if (ix1>=nx || ix2<0 || iy1>=ny || iy2<0) return false;

  if (rgbmode) plotFrame(nx,ny,x,y,mapconvert,cvfield2map);

  ipart[0] = ix1;
  ipart[1] = ix2;
  ipart[2] = iy1;
  ipart[3] = iy2;

  xylim[0] = maprect.x1;
  xylim[1] = maprect.x2;
  xylim[2] = maprect.y1;
  xylim[3] = maprect.y2;

  if (poptions.valueLabel==0) labfmt[0] = 0;
  else                        labfmt[0] =-1;
  labfmt[1] = 0;
  labfmt[2] = 0;
  ibcol = -1;

  if (labfmt[0]!=0) {
    float fontsize= 12. * poptions.labelSize;

    fp->set(poptions.fontname,poptions.fontface,fontsize);
    fp->getCharSize('0',chxlab,chylab);

    // the real height for numbers 0-9 (width is ok)
    chylab *= 0.75;
  } else {
    chxlab= chylab= 1.0;
  }

  zstep= poptions.lineinterval;
  zoff = poptions.base;

  if (poptions.linevalues.size()>0) {
    nlines = poptions.linevalues.size();
    for (int ii=0; ii<nlines; ii++){
      rlines[ii]= poptions.linevalues[ii];
    }
    idraw = 3;
  } else if (poptions.loglinevalues.size()>0) {
    nlines = poptions.loglinevalues.size();
    for (int ii=0; ii<nlines; ii++){
      rlines[ii]= poptions.loglinevalues[ii];
    }
    idraw = 4;
  } else {
    nlines= 0;
    idraw= 1;
    if (poptions.zeroLine==0) {
      idraw= 2;
      zoff= 0.;
    }
  }


  zrange[0]= +1.;
  zrange[1]= -1.;
  zrange2[0] = +1.;
  zrange2[1] = -1.;

  if (poptions.valuerange.size()==1) {
    zrange[0]= poptions.valuerange[0];
    zrange[1]= fieldUndef*0.95;
  } else if (poptions.valuerange.size()==2){
    if( poptions.valuerange[0]<=poptions.valuerange[1]) {
      zrange[0]= poptions.valuerange[0];
      zrange[1]= poptions.valuerange[1];
    } else {
      zrange[0]= fieldUndef*-0.95;
      zrange[1]= poptions.valuerange[1];
    }
  }

  ncol = 1;
  icol[0] = -1; // -1: set colour below
		// otherwise index in poptions.colours[]
  ntyp = 1;
  ityp[0] = -1;
  nwid = 1;
  iwid[0] = -1;
  nlim = 0;
  rlim[0] = 0.;

  nlines2 = 0;
  ncol2 = 1;
  icol2[0] = -1;
  ntyp2 = 1;
  ityp2[0] = -1;
  nwid2 = 1;
  iwid2[0] = -1;
  nlim2 = 0;
  rlim2[0] = 0.;

  ismooth = poptions.lineSmooth;
  if (ismooth<0) ismooth=0;

  ibmap  = 0;
  lbmap  = 0;
  nxbmap = 0;
  nybmap = 0;

  if (poptions.contourShading==0 && !poptions.options_1)
    idraw=0;


  //Plot colour shading
  if (poptions.contourShading!=0) {

    int idraw2=0;

    res = contour(nx, ny, fields[0]->data, x, y,
		  ipart, mapconvert, cvfield2map, xylim,
		  idraw, zrange, zstep, zoff,
		  nlines, rlines,
		  ncol, icol, ntyp, ityp,
		  nwid, iwid, nlim, rlim,
		  idraw2, zrange2, zstep2, zoff2,
		  nlines2, rlines2,
		  ncol2, icol2, ntyp2, ityp2,
		  nwid2, iwid2, nlim2, rlim2,
		  ismooth, labfmt, chxlab, chylab,
		  ibcol,
		  ibmap, lbmap, kbmap,
		  nxbmap, nybmap, rbmap,
		  fp, poptions, psoutput,
		  fields[0]->area);

  }

  //Plot contour lines
  if(idraw>0 || idraw2>0){

    zstep2= poptions.lineinterval_2;
    zoff2 = poptions.base_2;

    if (!poptions.options_2)
      idraw2 = 0;
    else
      idraw2 = 1;

    if (rgbmode){
      if(poptions.colours.size()>1){
	if(idraw>0 && idraw2>0) {
	  icol[0] =0;
	  icol2[0]=1;
	} else {
	  ncol=poptions.colours.size();
	  for (int i=0; i<ncol; ++i) icol[i]= i;
	}
      } else if(idraw>0) {
	glColor3ubv(poptions.linecolour.RGB());
      } else  {
	glColor3ubv(poptions.linecolour_2.RGB());
      }
    } else if (!rgbmode) {
      // drawing in overlay ... NOT USED !!!!!!!!!!!!!!
      glIndexi(poptions.linecolour.Index());
    }

    //    if (idraw2==1 || idraw2==2) {
    if (poptions.valuerange_2.size()==1) {
      zrange2[0]= poptions.valuerange_2[0];
      zrange2[1]= fieldUndef*0.95;
    } else if (poptions.valuerange_2.size()==2){
      if(poptions.valuerange_2[0]<=poptions.valuerange_2[1]) {
	zrange2[0]= poptions.valuerange_2[0];
	zrange2[1]= poptions.valuerange_2[1];
      } else {
	zrange2[0]= fieldUndef*-0.95;
	zrange2[1]= poptions.valuerange_2[1];
      }
    }

    if (poptions.linewidths.size()==1) {
      glLineWidth(poptions.linewidth);
    } else {
      if(idraw2>0){ // two set of plot options
	iwid[0] = 0;
	iwid2[0]= 1;
      } else {      // one set of plot options, different lines
	nwid=poptions.linewidths.size();
	for (int i=0; i<nwid; ++i) iwid[i]= i;
      }
    }

    if (poptions.linetypes.size()==1 && poptions.linetype.stipple) {
      glLineStipple(poptions.linetype.factor,poptions.linetype.bmap);
      glEnable(GL_LINE_STIPPLE);
    } else {
      if(idraw2>0){ // two set of plot options
	ityp[0] =0;
	ityp2[0]=1;
      } else {      // one set of plot options, different lines
	ntyp=poptions.linetypes.size();
	for (int i=0; i<ntyp; ++i) ityp[i]= i;
      }
    }

    if (!poptions.options_1) idraw=0;

    //turn off contour shading
    bool contourShading = poptions.contourShading;
    poptions.contourShading = 0;

    res = contour(nx, ny, fields[0]->data, x, y,
		  ipart, mapconvert, cvfield2map, xylim,
		  idraw, zrange, zstep, zoff,
		  nlines, rlines,
		  ncol, icol, ntyp, ityp,
		  nwid, iwid, nlim, rlim,
		  idraw2, zrange2, zstep2, zoff2,
		  nlines2, rlines2,
		  ncol2, icol2, ntyp2, ityp2,
		  nwid2, iwid2, nlim2, rlim2,
		  ismooth, labfmt, chxlab, chylab,
		  ibcol,
		  ibmap, lbmap, kbmap,
		  nxbmap, nybmap, rbmap,
		  fp, poptions, psoutput,
		  fields[0]->area);

    //reset contour shading
    poptions.contourShading = contourShading;
  }

  if (poptions.extremeType.upcase()=="L+H" ||
      poptions.extremeType.upcase()=="C+W") markExtreme();

  UpdateOutput();

  glDisable(GL_LINE_STIPPLE);

  //if (!res) cerr<<"Contour error"<<endl;

#ifdef DEBUGPRINT
  cerr << "++ Returning from FieldPlot::plotContour() ++" << endl;
#endif
  return true;
}

// plot scalar field as boxes with filled with patterns (cloud)
bool FieldPlot::plotBox_pattern(){
#ifdef DEBUGPRINT
  cerr << "++ Plotter Box_pattern-felt.." << endl;
#endif
  int n= fields.size();
  if (n<1) return false;
  if (!fields[0]) return false;

  if (!fields[0]->data) return false;

  int i, ix, iy, i1, i2;

  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  // convert gridbox corners to correct projection
  int npos=0;
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  gc.getGridPoints(fields[0]->area, area, maprect, true, npos, &x, &y,
                   ix1, ix2, iy1, iy2);
  if (ix1>ix2 || iy1>iy2) return false;

  int nxc = nx + 1;
  int nyc = ny + 1;

  plotFrame(nxc,nyc,x,y,2,NULL);

  const int npattern=4;
//float clim[npattern+1] = { 30., 50., 70., 90., 0.9e+35 };
  float clim[npattern+1] = { 25., 50., 75., 95., 0.9e+35 };
  int   step[npattern]   = { 16,   8,   4,   1  };
  int   mark[npattern]   = {  2,   2,   2,   1  };
  GLubyte pattern[npattern][128],  bit=1;
  int   istart, jstart, j, b, w;
  float cmin, cmax;

  for (n=0; n<npattern; n++) {
    for (i=0; i<128; i++) pattern[n][i]=0;
    for (jstart=0; jstart<mark[n]; jstart++) {
      for (istart=0; istart<mark[n]; istart++) {
	for (j=jstart; j<32; j+=step[n]) {
	  for (i=istart; i<32; i+=step[n]) {
	    b = i+j*32;
	    w = b/8;
	    b = 7 - (b - w*8);
	    pattern[n][w] = pattern[n][w] | (bit << b);
	  }
	}
      }
    }
  }

  glShadeModel(GL_FLAT);
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  glEnable(GL_POLYGON_STIPPLE);

  glColor3ubv(poptions.linecolour.RGB());

  ix2++;
  iy2++;

  for (iy=iy1; iy<iy2; iy++) {
    i2 = ix1-1;

    while (i2<ix2-1) {

      i1 = i2+1;

      // find start of cloud area
      while (i1<ix2 && (fields[0]->data[iy*nx+i1]<clim[0]
	             || fields[0]->data[iy*nx+i1]==fieldUndef)) i1++;
      if (i1<ix2) {

	n=1;
	while (n<npattern && fields[0]->data[iy*nx+i1]>clim[n]) n++;
	n--;
	cmin = clim[n];
	cmax = clim[n+1];

        i2 = i1+1;
	// find end of cloud area
	while (i2<ix2 && fields[0]->data[iy*nx+i2]>=cmin
		      && fields[0]->data[iy*nx+i2]<cmax) i2++;
	i2++;

	glPolygonStipple (&pattern[n][0]);
	glBegin(GL_QUAD_STRIP);

	for (ix=i1; ix<i2; ix++) {
	    glVertex2f(x[(iy+1)*nxc+ix], y[(iy+1)*nxc+ix]);
	    glVertex2f(x[iy*nxc+ix], y[iy*nxc+ix]);
	}
	glEnd();
	i2-=2;
	UpdateOutput();
      }
      else i2=nx;

    }
  }

  UpdateOutput();

  glDisable(GL_POLYGON_STIPPLE);

#ifdef DEBUGPRINT
  cerr << "++ Returning from FieldPlot::plotBox_pattern() ++" << endl;
#endif
  return true;
}


// plot scalar field with RGBA (RGB=constant)
bool FieldPlot::plotBox_alpha_shade(){
#ifdef DEBUGPRINT
  cerr << "++ Plotter Box_alpha_shade-felt.." << endl;
#endif
  int n= fields.size();
  if (n<1) return false;
  if (!fields[0]) return false;

  if (!fields[0]->data) return false;

  int ix, iy, i1, i2;

  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  // convert gridpoints to correct projection
  int npos=0;
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  gc.getGridPoints(fields[0]->area, area, maprect, true,
                   npos, &x, &y, ix1, ix2, iy1, iy2);
  if (ix1>ix2 || iy1>iy2) return false;

  int nxc = nx + 1;
  int nyc = ny + 1;

  plotFrame(nxc,nyc,x,y,2,NULL);

  float cmin, cmax;

//cmin=0.;  cmax=100.;

  if (poptions.valuerange.size()==2 &&
      poptions.valuerange[0]<=poptions.valuerange[1]) {
    cmin = poptions.valuerange[0];
    cmax = poptions.valuerange[1];
  } else {
    //##### not nice in timeseries... !!!!!!!!!!!!!!!!!!!!!
    cmin=  fieldUndef;
    cmax= -fieldUndef;
    for (int i=0; i<nx*ny; ++i) {
      if (fields[0]->data[i]!=fieldUndef) {
	if (cmin>fields[0]->data[i]) cmin= fields[0]->data[i];
	if (cmax<fields[0]->data[i]) cmax= fields[0]->data[i];
      }
    }
  }

  glShadeModel(GL_FLAT);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  float red=poptions.linecolour.fR(),
      green=poptions.linecolour.fG(),
       blue=poptions.linecolour.fB(), alpha;

  ix2++;
  iy2++;

  for (iy=iy1; iy<iy2; iy++) {
    i2 = ix1-1;

    while (i2<ix2-1) {

    i1 = i2+1;

    // find start of cloud area
    while (i1<ix2 && (fields[0]->data[iy*nx+i1]<cmin
	           || fields[0]->data[iy*nx+i1]==fieldUndef)) i1++;
    if (i1<ix2) {

        i2 = i1+1;
	// find end of cloud area
	while (i2<ix2 && fields[0]->data[iy*nx+i2]>=cmin
	              && fields[0]->data[iy*nx+i2]!=fieldUndef) i2++;

        i2++;
	glBegin(GL_QUAD_STRIP);

	for (ix=i1; ix<i2; ix++) {
	    glVertex2f(x[(iy+1)*nxc+ix], y[(iy+1)*nxc+ix]);
	    if (ix>i1) {
	      alpha = (fields[0]->data[iy*nx+ix-1] - cmin) / (cmax-cmin);
	      //if (fields[0]->data[iy*nx+ix-1]==fieldUndef) alpha=0.;
	      //if (alpha < 0.) alpha=0.;
	      //if (alpha > 1.) alpha=1.;
	      glColor4f(red, green, blue, alpha);
	    }
	    glVertex2f(x[iy*nxc+ix], y[iy*nxc+ix]);
	}
	glEnd();
	i2-=2;
    }
    else i2=nx;

    }
  }

  UpdateOutput();

  glDisable(GL_BLEND);

#ifdef DEBUGPRINT
  cerr << "++ Returning from FieldPlot::plotBox_alpha_shade() ++" << endl;
#endif
  return true;
}


//  plot some scalar field values with RGBA (RGB=constant)
bool FieldPlot::plotAlarmBox(){
#ifdef DEBUGPRINT
  cerr << "++ Plotter AlarmBox-felt.." << endl;
#endif
  int n= fields.size();
  if (n<1) return false;
  if (!fields[0]) return false;

  if (!fields[0]->data) return false;

  int ix, iy, i1, i2;

  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  // convert gridpoints to correct projection
  int npos=0;
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  gc.getGridPoints(fields[0]->area, area, maprect, true,
                   npos, &x, &y, ix1, ix2, iy1, iy2);
  if (ix1>ix2 || iy1>iy2) return false;

  int nxc = nx + 1;
  int nyc = ny + 1;

  // vmin,vmax: ok range, without alarm !!!
  float vmin= -fieldUndef;
  float vmax=  fieldUndef;

  int sf= poptions.forecastLength.size();
  int s1= poptions.forecastValueMin.size();
  int s2= poptions.forecastValueMax.size();

  // rather print error message than do something wrong

  if (sf>1) {

    int fc= fields[0]->forecastHour; // forecast hour

    if (fc<poptions.forecastLength[0]) return false;
    if (fc>poptions.forecastLength[sf-1]) return false;

    int i= 1;
    while (i<sf && fc>poptions.forecastLength[i]) i++;
    if (i==sf) i=sf-1;

    float r1= float(poptions.forecastLength[i] - fc)/
	      float(poptions.forecastLength[i] - poptions.forecastLength[i-1]);
    float r2= 1.-r1;

    if (sf==s1 && s2==0) {

      vmin=   r1 * poptions.forecastValueMin[i-1]
	    + r2 * poptions.forecastValueMin[i];

    } else if (sf==s2 && s1==0) {

      vmax=   r1 * poptions.forecastValueMax[i-1]
	    + r2 * poptions.forecastValueMax[i];

    } else if (sf==s1 && sf==s2) {

      vmin=   r1 * poptions.forecastValueMin[i-1]
	    + r2 * poptions.forecastValueMin[i];
      vmax=   r1 * poptions.forecastValueMax[i-1]
	    + r2 * poptions.forecastValueMax[i];

    } else {

      cerr << "FieldPlot::plotAlarmBox ERROR in setup!" << endl;
      return false;

    }

  } else if (sf==0 && s1==0 && s2==0 &&
             poptions.valuerange.size()==1) {

    vmin= poptions.valuerange[0];

  } else if (sf==0 && s1==0 && s2==0 &&
             poptions.valuerange.size()==2 &&
             poptions.valuerange[0]<=poptions.valuerange[1]) {

    vmin= poptions.valuerange[0];
    vmax= poptions.valuerange[1];

  } else {

    cerr << "FieldPlot::plotAlarmBox ERROR in setup!" << endl;
    return false;

  }

  plotFrame(nxc,nyc,x,y,2,NULL);

  glShadeModel(GL_FLAT);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glColor3ubv(poptions.linecolour.RGB());

  ix2++;
  iy2++;

  for (iy=iy1; iy<iy2; iy++) {
    i2 = ix1-1;

    while (i2<ix2-1) {

    i1 = i2+1;

    // find start of alarm area
    while (i1<ix2 && (fields[0]->data[iy*nx+i1]==fieldUndef
                    || (fields[0]->data[iy*nx+i1]<vmin
	             || fields[0]->data[iy*nx+i1]>vmax))) i1++;
    if (i1<ix2) {

        i2 = i1+1;
	// find end of alarm area
	while (i2<ix2 && fields[0]->data[iy*nx+i2]!=fieldUndef
	              && fields[0]->data[iy*nx+i2]>=vmin
	              && fields[0]->data[iy*nx+i2]<=vmax) i2++;

        i2++;
	glBegin(GL_QUAD_STRIP);

	for (ix=i1; ix<i2; ix++) {
	    glVertex2f(x[(iy+1)*nxc+ix], y[(iy+1)*nxc+ix]);
	    glVertex2f(x[iy*nxc+ix], y[iy*nxc+ix]);
	}
	glEnd();
	i2-=2;
    }
    else i2=nx;

    }
  }

  UpdateOutput();

#ifdef DEBUGPRINT
  cerr << "++ Returning from FieldPlot::plotAlarmBox() ++" << endl;
#endif
  return true;
}


// plot scalar field with RGBA (RGB=constant)
bool FieldPlot::plotAlpha_shade(){
#ifdef DEBUGPRINT
  cerr << "++ Plotter Alpha_shade-felt.." << endl;
#endif
  int n= fields.size();
  if (n<1) return false;
  if (!fields[0]) return false;

  if (!fields[0]->data) return false;

  int ix,iy;
  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  // convert gridpoints to correct projection
  int npos=0;
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  gc.getGridPoints(fields[0]->area, area, maprect, false,
                   npos, &x, &y, ix1, ix2, iy1, iy2);
  if (ix1>ix2 || iy1>iy2) return false;

  plotFrame(nx,ny,x,y,2,NULL);

  float cmin, cmax;

  if (poptions.valuerange.size()==2 &&
      poptions.valuerange[0]<=poptions.valuerange[1]) {
    cmin = poptions.valuerange[0];
    cmax = poptions.valuerange[1];
  } else {
    //##### not nice in timeseries... !!!!!!!!!!!!!!!!!!!!!
    cmin=  fieldUndef;
    cmax= -fieldUndef;
    for (int i=0; i<nx*ny; ++i) {
      if (fields[0]->data[i]!=fieldUndef) {
	if (cmin>fields[0]->data[i]) cmin= fields[0]->data[i];
	if (cmax<fields[0]->data[i]) cmax= fields[0]->data[i];
      }
    }
  }

  glShadeModel(GL_SMOOTH);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  float red=poptions.linecolour.fR(),
      green=poptions.linecolour.fG(),
       blue=poptions.linecolour.fB(), alpha;

  ix2++;

  for (iy=iy1; iy<iy2; iy++) {
      glBegin(GL_QUAD_STRIP);
      for (ix=ix1; ix<ix2; ix++) {

	  alpha = (fields[0]->data[(iy+1)*nx+ix] - cmin) / (cmax-cmin);
	  if (fields[0]->data[(iy+1)*nx+ix]==fieldUndef) alpha=0.;
	  //if (alpha < 0.) alpha=0.;
	  //if (alpha > 1.) alpha=1.;
	  glColor4f(red, green, blue, alpha);
	  glVertex2f(x[(iy+1)*nx+ix], y[(iy+1)*nx+ix]);

	  alpha = (fields[0]->data[iy*nx+ix] - cmin) / (cmax-cmin);
	  if (fields[0]->data[iy*nx+ix]==fieldUndef) alpha=0.;
	  //if (alpha < 0.) alpha=0.;
	  //if (alpha > 1.) alpha=1.;
	  glColor4f(red, green, blue, alpha);
	  glVertex2f(x[iy*nx+ix], y[iy*nx+ix]);
      }
      glEnd();
  }

  UpdateOutput();

  glDisable(GL_BLEND);
  glShadeModel(GL_FLAT);

#ifdef DEBUGPRINT
  cerr << "++ Returning from FieldPlot::plotAlpha_shade() ++" << endl;
#endif
  return true;
}


// plot frame for complete field area
void FieldPlot::plotFrame(const int nx, const int ny,
			  float *x, float *y,
			  const int mapconvert,
			  float *cvfield2map){
#ifdef DEBUGPRINT
  cerr << "++ Plotter ramme.." << endl;
#endif

  if (!rgbmode) return;

  if (fields.empty()) return;
  if (!fields[0]) return;

  glColor3ubv(poptions.bordercolour.RGB());
  glLineWidth(1);
  // glLineWidth(poptions.linewidth);

  //if (poptions.linetype.bmap!=0xFFFF){
  //  glLineStipple(1,poptions.linetype.bmap);
  //  glEnable(GL_LINE_STIPPLE);
  //}

  if (mapconvert==0) {

    glBegin(GL_LINE_LOOP);

    glVertex2f(0., 0.);
    glVertex2f(float(nx-1), 0.);
    glVertex2f(float(nx-1), float(ny-1));
    glVertex2f(0., float(ny-1));

  } else if (mapconvert==1) {

    glBegin(GL_LINE_LOOP);

    float xf[4], yf[4], x, y;
    xf[0] = 0.; xf[1] = nx-1; xf[2] = nx-1; xf[3] = 0.;
    yf[0] = 0.; yf[1] = 0.;   yf[2] = ny-1; yf[3] = ny-1;
    for (int i=0; i<4; i++) {
      x = cvfield2map[0] + cvfield2map[1]*xf[i] + cvfield2map[2]*yf[i];
      y = cvfield2map[3] + cvfield2map[4]*xf[i] + cvfield2map[5]*yf[i];
      glVertex2f(x, y);
    }

  } else {

    if (fields[0]->area.P().Gridtype()==Projection::geographic &&
        area.P().Gridtype()!=Projection::geographic) {
      Projection pr= fields[0]->area.P();
      float gspec[Projection::speclen];
      pr.Gridspec(gspec);
      float glon1= gspec[0];
      float glon2= gspec[0] + gspec[2]*float(nx-1);
      float glat1= gspec[1];
      float glat2= gspec[1] + gspec[3]*float(ny-1);
      // get rid of some strange frame lines (maybe to few or too many...)
      if (fabsf(glon2-glon1)>360.-gspec[2]*0.5 &&
	  fabsf(glat2-glat1)>180.-gspec[3]*0.5) return;
    }

    bool drawx1=true, drawx2=true, drawy1=true, drawy2=true;
    int ix,iy,ixstep,iystep;
    float x1,x2,y1,y2,dx,dy,dxm,dym;
    ixstep= (nx>10) ? nx/10 : 1;
    iystep= (ny>10) ? ny/10 : 1;

    iy=0;
    x1=x2=x[iy*nx];
    y1=y2=y[iy*nx];
    for (int ix=0; ix<nx; ix+=ixstep) {
      if (x1>x[iy*nx+ix]) x1= x[iy*nx+ix];
      if (x2<x[iy*nx+ix]) x2= x[iy*nx+ix];
      if (y1>y[iy*nx+ix]) y1= y[iy*nx+ix];
      if (y2<y[iy*nx+ix]) y2= y[iy*nx+ix];
    }
    drawy1= (x1<x2 || y1<y2);

    iy=ny-1;
    x1=x2=x[iy*nx];
    y1=y2=y[iy*nx];
    for (int ix=0; ix<nx; ix+=ixstep) {
      if (x1>x[iy*nx+ix]) x1= x[iy*nx+ix];
      if (x2<x[iy*nx+ix]) x2= x[iy*nx+ix];
      if (y1>y[iy*nx+ix]) y1= y[iy*nx+ix];
      if (y2<y[iy*nx+ix]) y2= y[iy*nx+ix];
    }
    drawy2= (x1<x2-0.01 || y1<y2-0.01);

    dxm= 0.;
    dym= 0.;
    for (int iy=0; iy<ny; iy+=iystep) {
      dx= fabsf(x[iy*nx]-x[iy*nx+nx-1]);
      dy= fabsf(y[iy*nx]-y[iy*nx+nx-1]);
      if (dxm<dx) dxm= dx;
      if (dym<dy) dym= dy;
    }
    drawx1= drawx2= (dxm>0.01 || dym>0.01);

    glBegin(GL_LINE_LOOP);

    int i;
    if (drawy1) {
      iy=0;
      for (ix=0; ix<nx; ix++) { i=iy*nx+ix; glVertex2f(x[i],y[i]); }
    }
    if (drawx2) {
      ix=nx-1;
      for (iy=1; iy<ny; iy++) { i=iy*nx+ix; glVertex2f(x[i],y[i]); }
    }
    if (drawy2) {
      iy=ny-1;
      for (ix=nx-1; ix>=0; ix--) { i=iy*nx+ix; glVertex2f(x[i],y[i]); }
    }
    if (drawx1) {
      ix=0;
      for (iy=ny-1; iy>0; iy--) { i=iy*nx+ix; glVertex2f(x[i],y[i]); }
    }
  }

  glEnd();

  // glDisable(GL_LINE_STIPPLE);

  UpdateOutput();

#ifdef DEBUGPRINT
  cerr << "++ Returning from FieldPlot::plotFrame() ++" << endl;
#endif
  return;
}


/*
  Mark extremepoints in a field with L/H (Low/High)
  or C/W (Cold/Warm)
 */
bool FieldPlot::markExtreme(){
#ifdef DEBUGPRINT
  cerr << "++ FieldPlot::markExtreme start" << endl;
#endif

  if (fields.size()<1) return false;
  if (!fields[0]) return false;
  if (!fields[0]->data) return false;

  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  int   mapconvert, ix1, ix2, iy1, iy2;
  float cvfield2map[6], cvmap2field[6];

  // find gridpoint conversion method
  if(!gc.getGridConversion(fields[0]->area, area, maprect, mapconvert,
		           cvfield2map, cvmap2field, ix1, ix2, iy1, iy2))
	return false;

  // convert gridpoints to correct projection
  int npos=0;
  float *x, *y;
//####if (mapconvert==2) {
    if(!gc.getGridPoints(fields[0]->area, area, maprect, false,
			 npos, &x, &y, ix1, ix2, iy1, iy2)) return false;
//####}

  if (ix1>ix2 || iy1>iy2) return false;

  int   ix,iy,n,i,j,k;
  float dx,dy,avgdist;

  //#### lag metode i GridConverter for dette (her + wind/vector) #########
  if (mapconvert==0) {
    avgdist= 1.;
  } else if (mapconvert==1) {
    dx= sqrtf( cvfield2map[1]*cvfield2map[1]
              +cvfield2map[4]*cvfield2map[4]);
    dy= sqrtf( cvfield2map[2]*cvfield2map[2]
              +cvfield2map[5]*cvfield2map[5]);
    avgdist= (dx+dy)*0.5;
  } else {
    int i1= (ix1>1)    ? ix1 : 1;
    int i2= (ix2<nx-2) ? ix2 : nx-2;
    int j1= (iy1>1)    ? iy1 : 1;
    int j2= (iy2<ny-2) ? iy2 : ny-2;
    int ixstp= (i2-i1)/5;  if (ixstp<1) ixstp=1;
    int iystp= (j2-j1)/5;  if (iystp<1) iystp=1;
    avgdist= 0.;
    n= 0;
    for (iy=j1; iy<j2; iy+=iystp) {
      for (ix=i1; ix<i2; ix+=ixstp) {
	i = iy*nx+ix;
	dx = x[i+1]-x[i];
	dy = y[i+1]-y[i];
	avgdist+= sqrtf(dx*dx+dy*dy);
	dx = x[i+nx]-x[i];
	dy = y[i+nx]-y[i];
	avgdist+= sqrtf(dx*dx+dy*dy);
	n+=2;
      }
    }
    if (n>0) avgdist= avgdist/float(n);
    else     avgdist= 1.;
  }

  if (ix1<1)    ix1= 1;
  if (ix2>nx-2) ix2= nx-2;
  if (iy1<1)    iy1= 1;
  if (iy2>ny-2) iy2= ny-2;
  ix2++;
  iy2++;

  char marks[2];
  char* pmarks[2];
  float chrx[2], chry[2];

  if (poptions.extremeType.upcase()=="C+W") {
    marks[0]= 'C';  pmarks[0]= "C\0";
    marks[1]= 'W';  pmarks[1]= "W\0";
  } else {
    marks[0]= 'L';  pmarks[0]= "L\0";
    marks[1]= 'H';  pmarks[1]= "H\0";
  }

  float fontsize= 28. * poptions.extremeSize;
  fp->set(poptions.fontname,poptions.fontface,fontsize);

  float size= 0.;

  for (i=0; i<2; i++) {
    fp->getCharSize(marks[i],chrx[i],chry[i]);
    // approx. real height for the character (width is ok)
    chry[i]*= 0.75;
    if (size<chrx[i]) size= chrx[i];
    if (size<chry[i]) size= chry[i];
  }

  float radius= 3.0 * size * poptions.extremeRadius;

  //int nscan = int(radius/avgdist+0.9);
  int nscan = int(radius/avgdist);
  if (nscan<2) nscan=2;
  float rg= float(nscan);

  // for scan of surrounding positions
  int  mscan= nscan*2+1;
  int *iscan= new int[mscan];
  float rg2= rg*rg;
  iscan[nscan]= nscan;
  for (j=1; j<=nscan; j++) {
    dy= float(j);
    dx= sqrtf(rg2-dy*dy);
    i=  int(dx+0.5);
    iscan[nscan-j]= i;
    iscan[nscan+j]= i;
  }

  // for test of gradients in the field
  int igrad[4][6];
  int nscan45degrees= int(sqrtf(rg2*0.5) + 0.5);

  for (k=0; k<4; k++) {
    i= j= 0;
    if      (k==0) j= nscan;
    else if (k==1) i= j= nscan45degrees;
    else if (k==2) i= nscan;
    else { i= nscan45degrees; j= -nscan45degrees; }
    igrad[k][0]= -i;
    igrad[k][1]= -j;
    igrad[k][2]=  i;
    igrad[k][3]=  j;
    igrad[k][4]= -j*nx-i;
    igrad[k][5]=  j*nx+i;
  }

  // the nearest grid points
  int near[8]= { -nx-1,-nx,-nx+1, -1,1, nx-1,nx,nx+1 };


  float gx,gy,fpos,fmin,fmax,f,f1,f2,gbest,fgrad;
  int   p,pp,i1,i2,j1,j2,l,iend,jend,ibest,jbest,ngrad,etype;
  bool  ok;

  maprect.setExtension( -size*0.5 );

  for (iy=iy1; iy<iy2; iy++) {
    for (ix=ix1; ix<ix2; ix++) {
      p= iy*nx+ix;
      gx= x[p];
      gy= y[p];
      if (fields[0]->data[p]!=fieldUndef
	  && maprect.isnear(gx,gy)) {
	fpos= fields[0]->data[p];

	// first check the nearest gridpoints
        fmin= fieldUndef;
	fmax=-fieldUndef;
	for (j=0; j<8; j++) {
	  f= fields[0]->data[p+near[j]];
	  if (fmin>f) fmin=f;
	  if (fmax<f) fmax=f;
        }

        if (fmax!=fieldUndef && (fmin>=fpos || fmax<=fpos)
                             && fmin!=fmax) {

	  // scan a larger area

	  j=    (iy-nscan>0)    ? iy-nscan   : 0;
	  jend= (iy+nscan+1<ny) ? iy+nscan+1 : ny;
	  ok= true;
	  etype= -1;
	  vector<int> pequal;

	  if (fmin>=fpos) {
	    // minimum at pos ix,iy
	    etype= 0;
	    while (j<jend && ok) {
	      n= iscan[nscan+iy-j];
 	      i=    (ix-n>0)    ? ix-n   : 0;
	      iend= (ix+n+1<nx) ? ix+n+1 : nx;
	      for (; i<iend; i++) {
		f= fields[0]->data[j*nx+i];
		if (f!=fieldUndef) {
		  if      (f<fpos)  ok= false;
		  else if (f==fpos) pequal.push_back(j*nx+i);
		}
	      }
	      j++;
	    }
	  } else {
	    // maximum at pos ix,iy
	    etype= 1;
	    while (j<jend && ok) {
	      n= iscan[nscan+iy-j];
 	      i=    (ix-n>0)    ? ix-n   : 0;
	      iend= (ix+n+1<nx) ? ix+n+1 : nx;
	      for (; i<iend; i++) {
		f= fields[0]->data[j*nx+i];
		if (f!=fieldUndef) {
		  if      (f>fpos)  ok= false;
		  else if (f==fpos) pequal.push_back(j*nx+i);
		}
	      }
	      j++;
	    }
	  }

	  if (ok) {
	    n= pequal.size();
	    if (n<2) {
	      ibest= ix;
	      jbest= iy;
	    } else {
	      // more than one pos with the same extreme value,
	      // select the one with smallest surrounding
	      // gradients in the field (seems to work...)
	      gbest= fieldUndef;
	      ibest= jbest= -1;
	      for (l=0; l<n; l++) {
		pp= pequal[l];
		j= pp/nx;
		i= pp-j*nx;
		fgrad= 0.;
		ngrad= 0;
		for (k=0; k<4; k++) {
                  i1= i+igrad[k][0];
                  j1= j+igrad[k][1];
                  i2= i+igrad[k][2];
                  j2= j+igrad[k][3];
		  if (i1>=0 && j1>=0 && i1<nx && j1<ny &&
                      i2>=0 && j2>=0 && i2<nx && j2<ny) {
		    f1= fields[0]->data[pp+igrad[k][4]];
		    f2= fields[0]->data[pp+igrad[k][5]];
		    if (f1!=fieldUndef && f2!=fieldUndef) {
		      fgrad+= fabsf(f1-f2);
		      ngrad++;
		    }
		  }
		}
		if (ngrad>0) {
		  fgrad/= float(ngrad);
		  if (fgrad<gbest) {
		    gbest= fgrad;
		    ibest= i;
		    jbest= j;
		  } else if (fgrad==gbest) {
		    gbest= fieldUndef;
		    ibest= jbest= -1;
		  }
		}
	      }
	    }

	    if (ibest==ix && jbest==iy) {
	      // mark extreme point
	      fp->drawStr(pmarks[etype],
			  gx-chrx[etype]*0.5,gy-chry[etype]*0.5,0.0);
//#######################################################################
//		glBegin(GL_LINE_LOOP);
//		glVertex2f(gx-chrx[etype]*0.5,gy-chry[etype]*0.5);
//		glVertex2f(gx+chrx[etype]*0.5,gy-chry[etype]*0.5);
//		glVertex2f(gx+chrx[etype]*0.5,gy+chry[etype]*0.5);
//		glVertex2f(gx-chrx[etype]*0.5,gy+chry[etype]*0.5);
//		glEnd();
//#######################################################################
	    }
	  }
	}
      }
    }
  }

  delete[] iscan;

#ifdef DEBUGPRINT
  cerr << "++ FieldPlot::markExtreme end" << endl;
#endif
  return true;
}

// draw the grid lines in some density
bool FieldPlot::plotGridLines(){
#ifdef DEBUGPRINT
  cerr << "++ FieldPlot::plotGridLines start" << endl;
#endif

  if (fields.size()<1) return false;
  if (!fields[0]) return false;
  if (!fields[0]->data) return false;

  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  int   mapconvert, ix1, ix2, iy1, iy2;
  float cvfield2map[6], cvmap2field[6];

  // find gridpoint conversion method
  if(!gc.getGridConversion(fields[0]->area, area, maprect, mapconvert,
		           cvfield2map, cvmap2field, ix1, ix2, iy1, iy2))
	return false;

  // convert gridpoints to correct projection
  int npos=0;
  float *x, *y;
  if (mapconvert==2) {
    if(!gc.getGridPoints(fields[0]->area, area, maprect, false,
			 npos, &x, &y, ix1, ix2, iy1, iy2)) return false;
  }

  if (ix1>ix2 || iy1>iy2) return false;

  ix2++;
  iy2++;

  int step= poptions.gridLines;
  int maxlines= poptions.gridLinesMax;

  if (maxlines>0) {
    if ((ix2-ix1)/step > maxlines ||
        (iy2-iy1)/step > maxlines) return false;
  }

  glColor4f(poptions.bordercolour.fR(),poptions.bordercolour.fG(),
	    poptions.bordercolour.fB(),0.5);
  glLineWidth(1.0);

  int ix,iy;

  if (mapconvert==0) {

    float x1,x2,y1,y2;
    glBegin(GL_LINES);
    y1= float(iy1);
    y2= float(iy2-1);
    for (ix=ix1; ix<ix2; ix+=step) {
      glVertex2f(float(ix),y1);
      glVertex2f(float(ix),y2);
    }
    x1= float(ix1);
    x2= float(ix2-1);
    for (iy=iy1; iy<iy2; iy+=step) {
      glVertex2f(x1,float(iy));
      glVertex2f(x2,float(iy));
    }
    glEnd();

  } else if (mapconvert==1) {

    float gx,gx1,gx2,gy,gy1,gy2,px,py;
    glBegin(GL_LINES);
    gy1= float(iy1);
    gy2= float(iy2-1);
    for (ix=ix1; ix<ix2; ix+=step) {
      gx= float(ix);
      px= cvfield2map[0] + cvfield2map[1]*gx + cvfield2map[2]*gy1;
      py= cvfield2map[3] + cvfield2map[4]*gx + cvfield2map[5]*gy1;
      glVertex2f(px, py);
      px= cvfield2map[0] + cvfield2map[1]*gx + cvfield2map[2]*gy2;
      py= cvfield2map[3] + cvfield2map[4]*gx + cvfield2map[5]*gy2;
      glVertex2f(px, py);
    }
    gx1= float(ix1);
    gx2= float(ix2-1);
    for (iy=iy1; iy<iy2; iy+=step) {
      gy= float(iy);
      px= cvfield2map[0] + cvfield2map[1]*gx1 + cvfield2map[2]*gy;
      py= cvfield2map[3] + cvfield2map[4]*gx1 + cvfield2map[5]*gy;
      glVertex2f(px, py);
      px= cvfield2map[0] + cvfield2map[1]*gx2 + cvfield2map[2]*gy;
      py= cvfield2map[3] + cvfield2map[4]*gx2 + cvfield2map[5]*gy;
      glVertex2f(px, py);
    }
    glEnd();

  } else if (mapconvert==2) {

    int i;
    for (ix=ix1; ix<ix2; ix+=step) {
      glBegin(GL_LINE_STRIP);
      for (iy=iy1; iy<iy2; iy++) {
	i= iy*nx+ix;
        glVertex2f(x[i],y[i]);
      }
      glEnd();
    }
    for (iy=iy1; iy<iy2; iy+=step) {
      glBegin(GL_LINE_STRIP);
      for (ix=ix1; ix<ix2; ix++) {
	i= iy*nx+ix;
        glVertex2f(x[i],y[i]);
      }
      glEnd();
    }

  }

  UpdateOutput();

  return true;
}


// show areas with undefined field values
bool FieldPlot::plotUndefined(){
#ifdef DEBUGPRINT
  cerr << "++ FieldPlot::plotUndefined start" << endl;
#endif

  if (!enabled || fields.size()<1) return false;
  if (!fields[0]) return false;
  if (!fields[0]->data) return false;

  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  int   mapconvert, ix1, ix2, iy1, iy2;
  float cvfield2map[6], cvmap2field[6];

  // find gridpoint conversion method
  if(!gc.getGridConversion(fields[0]->area, area, maprect, mapconvert,
		           cvfield2map, cvmap2field, ix1, ix2, iy1, iy2))
	return false;

  // convert gridpoints to correct projection
  int npos=0;
  float *x, *y;
  if (mapconvert==2) {
    if(!gc.getGridPoints(fields[0]->area, area, maprect, true,
			 npos, &x, &y, ix1, ix2, iy1, iy2)) return false;
  }

  int nxc = nx + 1;
  //int nyc = ny + 1;

  if (ix1>ix2 || iy1>iy2) return false;
  if (ix1>=nx || ix2<0 || iy1>=ny || iy2<0) return false;

  if (poptions.undefMasking==1) {
    glShadeModel(GL_FLAT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  } else {
    glLineWidth(poptions.undefLinewidth+0.1);
    if (poptions.undefLinetype.bmap!=0xFFFF){
      glLineStipple(1,poptions.undefLinetype.bmap);
      glEnable(GL_LINE_STIPPLE);
    }
  }

  if (poptions.undefColour==backgroundColour)
    poptions.undefColour= backContrastColour;

  glColor3ubv(poptions.undefColour.RGB());

  int ix,iy,ixbgn,ixend,iybgn,iyend;
  float x1,x2,y1,y2;

  ix2++;
  iy2++;

  if (mapconvert==0 || mapconvert==1) {
    if (poptions.undefMasking==1)
      glBegin(GL_QUADS);
    else
      glBegin(GL_LINES);
  }

  for (iy=iy1; iy<iy2; iy++) {

    ix= ix1;

    while (ix<ix2) {

      while (ix<ix2 && fields[0]->data[iy*nx+ix]!=fieldUndef) ix++;

      if (ix<ix2) {

        ixbgn= ix++;
        while (ix<ix2 && fields[0]->data[iy*nx+ix]==fieldUndef) ix++;
        ixend= ix;

	if (poptions.undefMasking==1) {

	  if (mapconvert==0) {

	    x1= float(ixbgn) - 0.5;
	    x2= float(ixend) - 0.5;
	    y1= float(iy) - 0.5;
	    y2= float(iy) + 0.5;
            glVertex2f(x1,y2);
            glVertex2f(x1,y1);
            glVertex2f(x2,y1);
            glVertex2f(x2,y2);

	  } else if (mapconvert==1) {

	    x1= float(ixbgn) - 0.5;
	    x2= float(ixend) - 0.5;
	    y1= float(iy) - 0.5;
	    y2= float(iy) + 0.5;
            glVertex2f(cvfield2map[0]+cvfield2map[1]*x1+cvfield2map[2]*y2,
                       cvfield2map[3]+cvfield2map[4]*x1+cvfield2map[5]*y2);
            glVertex2f(cvfield2map[0]+cvfield2map[1]*x1+cvfield2map[2]*y1,
                       cvfield2map[3]+cvfield2map[4]*x1+cvfield2map[5]*y1);
            glVertex2f(cvfield2map[0]+cvfield2map[1]*x2+cvfield2map[2]*y1,
                       cvfield2map[3]+cvfield2map[4]*x2+cvfield2map[5]*y1);
            glVertex2f(cvfield2map[0]+cvfield2map[1]*x2+cvfield2map[2]*y2,
                       cvfield2map[3]+cvfield2map[4]*x2+cvfield2map[5]*y2);

	  } else if (mapconvert==2) {

	    glBegin(GL_QUAD_STRIP);
	    for (ix=ixbgn; ix<=ixend; ix++) {
              glVertex2f(x[(iy+1)*nxc+ix],y[(iy+1)*nxc+ix]);
              glVertex2f(x[iy*nxc+ix],    y[iy*nxc+ix]);
	    }
	    glEnd();

	  }

	} else {

	  // drawing lines
	  // here only drawing lines in grid x direction (y dir. below),
	  // avoiding plot of many short lines that are connected
	  // (but still drawing many "double" lines...)

	  if (mapconvert==0) {

	    x1= float(ixbgn) - 0.5;
	    x2= float(ixend) - 0.5;
	    y1= float(iy) - 0.5;
	    y2= float(iy) + 0.5;
            glVertex2f(x1,y1);
            glVertex2f(x2,y1);
            glVertex2f(x1,y2);
            glVertex2f(x2,y2);

	  } else if (mapconvert==1) {

	    x1= float(ixbgn) - 0.5;
	    x2= float(ixend) - 0.5;
	    y1= float(iy) - 0.5;
	    y2= float(iy) + 0.5;
            glVertex2f(cvfield2map[0]+cvfield2map[1]*x1+cvfield2map[2]*y1,
                       cvfield2map[3]+cvfield2map[4]*x1+cvfield2map[5]*y1);
            glVertex2f(cvfield2map[0]+cvfield2map[1]*x2+cvfield2map[2]*y1,
                       cvfield2map[3]+cvfield2map[4]*x2+cvfield2map[5]*y1);
            glVertex2f(cvfield2map[0]+cvfield2map[1]*x1+cvfield2map[2]*y2,
                       cvfield2map[3]+cvfield2map[4]*x1+cvfield2map[5]*y2);
            glVertex2f(cvfield2map[0]+cvfield2map[1]*x2+cvfield2map[2]*y2,
                       cvfield2map[3]+cvfield2map[4]*x2+cvfield2map[5]*y2);

	  } else if (mapconvert==2) {

	    for (int iyy=iy; iyy<=iy+1; iyy++) {
	      glBegin(GL_LINE_STRIP);
	      for (ix=ixbgn; ix<=ixend; ix++)
                glVertex2f(x[iyy*nxc+ix],y[iyy*nxc+ix]);
	      glEnd();
	    }
	  }
	}
        ix= ixend+1;
      }
    }
  }

  if (poptions.undefMasking>1) {

    // linedrawing in grid y direction (undefMasking>1)

    for (ix=ix1; ix<ix2; ix++) {

      iy= iy1;

      while (iy<iy2) {

        while (iy<iy2 && fields[0]->data[iy*nx+ix]!=fieldUndef) iy++;

        if (iy<iy2) {

          iybgn= iy++;
          while (iy<iy2 && fields[0]->data[iy*nx+ix]==fieldUndef) iy++;
          iyend= iy;

	  if (mapconvert==0) {

	    x1= float(ix) - 0.5;
	    x2= float(ix) + 0.5;
	    y1= float(iybgn) - 0.5;
	    y2= float(iyend) - 0.5;
            glVertex2f(x1,y1);
            glVertex2f(x1,y2);
            glVertex2f(x2,y1);
            glVertex2f(x2,y2);

	  } else if (mapconvert==1) {

	    x1= float(ix) - 0.5;
	    x2= float(ix) + 0.5;
	    y1= float(iybgn) - 0.5;
	    y2= float(iyend) - 0.5;
            glVertex2f(cvfield2map[0]+cvfield2map[1]*x1+cvfield2map[2]*y1,
                       cvfield2map[3]+cvfield2map[4]*x1+cvfield2map[5]*y1);
            glVertex2f(cvfield2map[0]+cvfield2map[1]*x1+cvfield2map[2]*y2,
                       cvfield2map[3]+cvfield2map[4]*x1+cvfield2map[5]*y2);
            glVertex2f(cvfield2map[0]+cvfield2map[1]*x2+cvfield2map[2]*y1,
                       cvfield2map[3]+cvfield2map[4]*x2+cvfield2map[5]*y1);
            glVertex2f(cvfield2map[0]+cvfield2map[1]*x2+cvfield2map[2]*y2,
                       cvfield2map[3]+cvfield2map[4]*x2+cvfield2map[5]*y2);

	  } else if (mapconvert==2) {

	    for (int ixx=ix; ixx<=ix+1; ixx++) {
	      glBegin(GL_LINE_STRIP);
	      for (iy=iybgn; iy<=iyend; iy++)
                glVertex2f(x[iy*nxc+ixx],y[iy*nxc+ixx]);
	      glEnd();
	    }
	  }
          iy= iyend+1;
	}
      }
    }
  }

  if (mapconvert==0 || mapconvert==1)
    glEnd();

  UpdateOutput();

  glDisable(GL_LINE_STIPPLE);

  return true;
}


/*
  plot field values as numbers in each gridpoint
  skip plotting if too many (or too small) numbers
*/
bool FieldPlot::plotNumbers(){
#ifdef DEBUGPRINT
  cerr << "++ plotNumbers.." << endl;
#endif
  int n= fields.size();
  if (n!=1) return false;
  if (!fields[0]) return false;

  if (!fields[0]->data) return false;

  int i,ix,iy;
  int nx= fields[0]->nx;
  int ny= fields[0]->ny;

  // convert gridpoints to correct projection
  int npos=0;
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  gc.getGridPoints(fields[0]->area, area, maprect, false,
		   npos, &x, &y, ix1, ix2, iy1, iy2);
  if (ix1>ix2 || iy1>iy2) return false;

  int autostep;
  float dist;
  bool xStepComp;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, 25, autostep, dist, xStepComp);
  if (autostep>1) return false;

  ix2++;
  iy2++;

  float fontsize= 16.;
//fp->set("Helvetica",poptions.fontface,fontsize);
  fp->set("Helvetica","bold",fontsize);

  float chx,chy;
  fp->getCharSize('0',chx,chy);
  // the real height for numbers 0-9 (width is ok)
  chy *= 0.75;

  float *field= fields[0]->data;
  float gx,gy,w,h;
  float hh= chy*0.5;
  float ww= chx*0.5;
  int iprec= -int(log10(fields[0]->storageScaling));
  if (iprec<0) iprec=0;
  miString str;

  glColor3f(0.0,0.0,0.0);

  for (iy=iy1; iy<iy2; iy++) {
    for (ix=ix1; ix<ix2; ix++) {
      i= iy*nx+ix;
      gx= x[i];
      gy= y[i];

      if (field[i]!=fieldUndef) {
	ostringstream ostr;
	ostr<<setprecision(iprec)<<setiosflags(ios::fixed)<<field[i];
	str= ostr.str();
	fp->getStringSize(str.c_str(), w, h);
	w*=0.5;
      } else {
	str="X";
	w=ww;
      }

      if (maprect.isinside(gx-w,gy-hh) && maprect.isinside(gx+w,gy+hh))
	fp->drawStr(str.c_str(),gx-w,gy-hh,0.0);
    }
  }

  UpdateOutput();

  // draw lines/boxes at borders between gridpoints..............................

  int   mapconvert;
  float cvfield2map[6], cvmap2field[6];

  // find gridpoint conversion method
  if (!gc.getGridConversion(fields[0]->area, area, maprect, mapconvert,
		            cvfield2map, cvmap2field, ix1, ix2, iy1, iy2))
    mapconvert= 2;

  // convert gridpoints to correct projection
  if (!gc.getGridPoints(fields[0]->area, area, maprect, true,
			npos, &x, &y, ix1, ix2, iy1, iy2)) return false;

  if (ix1>ix2 || iy1>iy2) return false;

  nx++;
  ny++;

  glColor3f(0.2,0.2,0.2);
  glLineWidth(1.0);

  if (mapconvert==0 || mapconvert==1) {

    glBegin(GL_LINES);
    for (ix=ix1; ix<=ix2; ix++) {
      glVertex2f(x[iy1*nx+ix],y[iy1*nx+ix]);
      glVertex2f(x[iy2*nx+ix],y[iy2*nx+ix]);
    }
    for (iy=iy1; iy<=iy2; iy++) {
      glVertex2f(x[iy*nx+ix1],y[iy*nx+ix1]);
      glVertex2f(x[iy*nx+ix2],y[iy*nx+ix2]);
    }
    glEnd();

  } else if (mapconvert==2) {

    ix2++;
    iy2++;

    for (ix=ix1; ix<ix2; ix++) {
      glBegin(GL_LINE_STRIP);
      for (iy=iy1; iy<iy2; iy++) {
        glVertex2f(x[iy*nx+ix],y[iy*nx+ix]);
      }
      glEnd();
    }
    for (iy=iy1; iy<iy2; iy++) {
      glBegin(GL_LINE_STRIP);
      for (ix=ix1; ix<ix2; ix++) {
	i= iy*nx+ix;
        glVertex2f(x[iy*nx+ix],y[iy*nx+ix]);
      }
      glEnd();
    }

  }

  UpdateOutput();

#ifdef DEBUGPRINT
  cerr << "++ Returning from FieldPlot::plotNumbers() ++" << endl;
#endif
  return true;
}


miString FieldPlot::getModelName()
{
  miString str;
  if (fields.size()>0)
    if (fields[0])
      if (fields[0]->data)
	str=fields[0]->modelName;
  return str;
}


miString FieldPlot::getTrajectoryFieldName()
{
  miString str;
  int nf= 0;
  if (ptype==fpt_wind)          nf= 2;
  if (ptype==fpt_wind_colour)   nf= 3;
  if (ptype==fpt_vector)        nf= 2;
  if (ptype==fpt_vector_colour) nf= 3;

  if (nf>=2 && fields.size()>=nf) {
    bool ok= true;
    for (int i=0; i<nf; i++) {
      if (!fields[i])
	ok= false;
      else if (!fields[i]->data)
	ok= false;
    }
    if (ok) str= fields[0]->fieldText;
  }
  return str;
}
