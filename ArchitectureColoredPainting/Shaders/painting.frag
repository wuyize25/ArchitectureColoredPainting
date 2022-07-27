#version 450 core

layout (location = 0) out vec4 gBaseColor;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gPosition;
layout (location = 3) out vec2 gMetallicRoughness;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;



////////////////////////////////////////////////////////////////////////////

float border;

mat2 inv(mat2 m){
	return mat2(m[1][1],-m[0][1],-m[1][0],m[0][0])/(m[0][0]*m[1][1]-m[1][0]*m[0][1]);
}

float abs_min(float a, float b){
	if(abs(a)<abs(b)){
		return a;
	}
	else{
		return b;
	}
}

int i_mod(int a, int m){
	return int(mod(float(a), float(m)));
}

float line_dist(vec2 uv, const vec2 p0, vec2 p1){
	vec2 tang=p1-p0;
	vec2 nor=normalize(vec2(tang.y,-tang.x));

	if(dot(tang,uv)<dot(tang,p0)){
		return distance(p0,uv);
	}
	else if(dot(tang,uv)>dot(tang,p1)){
		return distance(p1,uv);
	}
	else{
		return dot(nor,uv)-dot(nor,p0);
	}
}

bool int_test(vec2 uv, vec2 last_point, vec2 p0, vec2 p1){
	last_point-=uv;
	p0-=uv;
	p1-=uv;

	bool ret;
	if(p0.y==0.){
		ret=(p0.x>=0. && p1.y*last_point.y<0.);
	}
	else if(p1.y==0.){
		ret=false;
	}
	else if(p0.y*p1.y<0.){
		if(p0.x>=0. && p1.x>=0.){
			ret=true;
		}
		else if (p0.x<0. && p1.x<0.){
			ret=false;
		}
		else{
			vec2 nor;
			if(p0.y>p1.y){
				nor=p0-p1;
			}
			else{
				nor=p1-p0;
			}

			nor=vec2(nor.y,-nor.x);
			if(dot(nor,p0)<0.){
				ret=false;
			}
			else{
				ret=true;
			}
		}
	}
	else{
		ret=false;
	}

	return ret;
}

bool tri_test(vec2 uv, vec2 p0, vec2 p1, vec2 p2, bool inside){
	vec2 nor1=normalize(p0-p1);
	nor1=vec2(nor1.y,-nor1.x);
	vec2 nor2=normalize(p1-p2);
	nor2=vec2(nor2.y,-nor2.x);
	vec2 tan3=normalize(p2-p0);
	vec2 nor3=vec2(tan3.y,-tan3.x);

	if(inside){
		if(dot(tan3,p0)>=dot(tan3,uv) || dot(tan3,p2)<=dot(tan3,uv)){
			return false;
		}
		
		float brd=max(dot(nor3,nor1),dot(nor3,nor2))*border;
		return (dot(uv,nor1)>=dot(p0,nor1) && dot(uv,nor2)>=dot(p1,nor2) && dot(uv,nor3)>=dot(p2,nor3)+brd) ||
		(dot(uv,nor1)<=dot(p0,nor1) && dot(uv,nor2)<=dot(p1,nor2) && dot(uv,nor3)<=dot(p2,nor3)-brd);
	}
	else{
		float brd1=dot(nor1,tan3)*border;
		float brd2=dot(nor2,tan3)*border;

		if(dot(tan3,p0)>=dot(tan3,uv)-brd1 || dot(tan3,p2)<=dot(tan3,uv)-brd2){
			return false;
		}
		return (dot(uv,nor1)>=dot(p0,nor1)-border && dot(uv,nor2)>=dot(p1,nor2)-border && dot(uv,nor3)>=dot(p2,nor3)) ||
		(dot(uv,nor1)<=dot(p0,nor1)+border && dot(uv,nor2)<=dot(p1,nor2)+border && dot(uv,nor3)<=dot(p2,nor3));
	}

}

float bezier_sd(vec2 uv, vec2 p0, vec2 p1, vec2 p2){

	const mat2 trf1 = mat2(-1, 2, 1, 2);
	mat2 trf2 = inv(mat2(p0-p1, p2-p1));
	mat2 trf=trf1*trf2;

	uv-=p1;
	vec2 xy=trf*uv;
	xy.y-=1.;

	vec2 gradient;
	gradient.x=2.*trf[0][0]*(trf[0][0]*uv.x+trf[1][0]*uv.y)-trf[0][1];
	gradient.y=2.*trf[1][0]*(trf[0][0]*uv.x+trf[1][0]*uv.y)-trf[1][1];

	return (xy.x*xy.x-xy.y)/length(gradient);
}


float render_serif(vec2 uv){

	uv.y-=.5;
	uv*=1.8;
	border*=1.8;
	uv.y+=.5;

	uv.x+=.1;

	float d = 1e38;
	float poly_d = 1e38;
	float d1 = 1e38;

	vec2 p0,p1,p2;
	/*
	if(all(lessThan(abs(uv-vec2(-1.29705090246,0.49556210191)),vec2(0.203622751405,0.194877434267)+vec2(border)))){
		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-1.50067365386,0.690439536177),vec2(-1.50067365386,0.662506649919)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.34847869375,0.690439536177),vec2(-1.50067365386,0.690439536177)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.34847869375,0.662506649919),vec2(-1.34847869375,0.690439536177)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.50067365386,0.662506649919),vec2(-1.34847869375,0.662506649919)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-1.50067365386,0.328356479174),vec2(-1.50067365386,0.300684667642)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.50067365386,0.300684667642),vec2(-1.34847869375,0.300684667642)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.34847869375,0.300684667642),vec2(-1.34847869375,0.328356479174)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.34847869375,0.328356479174),vec2(-1.50067365386,0.328356479174)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-1.24562309793,0.328356479174),vec2(-1.24562309793,0.300684667642)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.09342815105,0.328356479174),vec2(-1.24562309793,0.328356479174)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.09342815105,0.300684667642),vec2(-1.09342815105,0.328356479174)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.24562309793,0.300684667642),vec2(-1.09342815105,0.300684667642)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-1.24562309793,0.690439536177),vec2(-1.24562309793,0.662506649919)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.09342815105,0.690439536177),vec2(-1.24562309793,0.690439536177)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.09342815105,0.662506649919),vec2(-1.09342815105,0.690439536177)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.24562309793,0.662506649919),vec2(-1.09342815105,0.662506649919)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-1.45107323965,0.300684667642),vec2(-1.39807911127,0.300684667642)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.39807911127,0.300684667642),vec2(-1.39807911127,0.690439536177)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.39807911127,0.690439536177),vec2(-1.45107323965,0.690439536177)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.45107323965,0.690439536177),vec2(-1.45107323965,0.300684667642)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-1.45107323965,0.527802348895),vec2(-1.45107323965,0.495953672637)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.45107323965,0.495953672637),vec2(-1.14302855203,0.495953672637)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.14302855203,0.495953672637),vec2(-1.14302855203,0.527802348895)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.14302855203,0.527802348895),vec2(-1.45107323965,0.527802348895)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-1.19602268041,0.300684667642),vec2(-1.14302855203,0.300684667642)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.14302855203,0.300684667642),vec2(-1.14302855203,0.690439536177)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.14302855203,0.690439536177),vec2(-1.19602268041,0.690439536177)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.19602268041,0.690439536177),vec2(-1.19602268041,0.300684667642)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}
	}
	*/
	if(all(lessThan(abs(uv-vec2(-0.905207605282,0.43943531671)),vec2(0.131571631799,0.146321237064)+vec2(border)))){
		p0=vec2(-0.980652472562,0.432256320122);p1=vec2(-0.980652472562,0.376129514241);p2=vec2(-0.959444621888,0.347459653556);
		if(tri_test(uv, p0, p1, p2, true)){
			d=min(d,-bezier_sd(uv, p0, p1, p2));
		}
		p0=vec2(-0.959444621888,0.347459653556);p1=vec2(-0.938361593128,0.318958533541);p2=vec2(-0.897114929876,0.318958533541);
		if(tri_test(uv, p0, p1, p2, true)){
			d=min(d,-bezier_sd(uv, p0, p1, p2));
		}
		p0=vec2(-0.897114929876,0.318958533541);p1=vec2(-0.865527286983,0.318958533541);p2=vec2(-0.845437632053,0.335419078254);
		if(tri_test(uv, p0, p1, p2, true)){
			d=min(d,-bezier_sd(uv, p0, p1, p2));
		}
		p0=vec2(-0.845437632053,0.335419078254);p1=vec2(-0.825063806547,0.352112459345);p2=vec2(-0.81697111046,0.384744318419);
		if(tri_test(uv, p0, p1, p2, true)){
			d=min(d,-bezier_sd(uv, p0, p1, p2));
		}
		p0=vec2(-0.778334987661,0.384744318419);p1=vec2(-0.789821407019,0.339059715715);p2=vec2(-0.82088690046,0.316086918361);
		if(tri_test(uv, p0, p1, p2, false)){
			d=min(d,bezier_sd(uv, p0, p1, p2));
		}
		p0=vec2(-0.82088690046,0.316086918361);p1=vec2(-0.851691401898,0.293114079646);p2=vec2(-0.902074977419,0.293114079646);
		if(tri_test(uv, p0, p1, p2, false)){
			d=min(d,bezier_sd(uv, p0, p1, p2));
		}
		p0=vec2(-0.902074977419,0.293114079646);p1=vec2(-0.962900714754,0.293114079646);p2=vec2(-0.999970513281,0.333055493352);
		if(tri_test(uv, p0, p1, p2, false)){
			d=min(d,bezier_sd(uv, p0, p1, p2));
		}
		p0=vec2(-0.999970513281,0.333055493352);p1=vec2(-1.03677923708,0.373257899062);p2=vec2(-1.03677923708,0.439565833392);
		if(tri_test(uv, p0, p1, p2, false)){
			d=min(d,bezier_sd(uv, p0, p1, p2));
		}
		p0=vec2(-1.03677923708,0.439565833392);p1=vec2(-1.03677923708,0.505351659632);p2=vec2(-1.00049262137,0.545554106703);
		if(tri_test(uv, p0, p1, p2, false)){
			d=min(d,bezier_sd(uv, p0, p1, p2));
		}
		p0=vec2(-1.00049262137,0.545554106703);p1=vec2(-0.964206005662,0.585756553774);p2=vec2(-0.905207625963,0.585756553774);
		if(tri_test(uv, p0, p1, p2, false)){
			d=min(d,bezier_sd(uv, p0, p1, p2));
		}
		p0=vec2(-0.905207625963,0.585756553774);p1=vec2(-0.842293414903,0.585756553774);p2=vec2(-0.808617339647,0.546859356249);
		if(tri_test(uv, p0, p1, p2, false)){
			d=min(d,bezier_sd(uv, p0, p1, p2));
		}
		p0=vec2(-0.808617339647,0.546859356249);p1=vec2(-0.774941264391,0.508223274812);p2=vec2(-0.773635973483,0.434344752485);
		if(tri_test(uv, p0, p1, p2, false)){
			d=min(d,bezier_sd(uv, p0, p1, p2));
		}
		p0=vec2(-0.830284887455,0.462016564017);p1=vec2(-0.831851211727,0.510572740539);p2=vec2(-0.850763015272,0.535223303252);
		if(tri_test(uv, p0, p1, p2, true)){
			d=min(d,-bezier_sd(uv, p0, p1, p2));
		}
		p0=vec2(-0.850763015272,0.535223303252);p1=vec2(-0.869704151709,0.559912099879);p2=vec2(-0.905207625963,0.559912099879);
		if(tri_test(uv, p0, p1, p2, true)){
			d=min(d,-bezier_sd(uv, p0, p1, p2));
		}
		p0=vec2(-0.905207625963,0.559912099879);p1=vec2(-0.938361593128,0.559912099879);p2=vec2(-0.957418587479,0.535111893535);
		if(tri_test(uv, p0, p1, p2, true)){
			d=min(d,-bezier_sd(uv, p0, p1, p2));
		}
		p0=vec2(-0.957418587479,0.535111893535);p1=vec2(-0.976475566475,0.510311707175);p2=vec2(-0.980652472562,0.462016564017);
		if(tri_test(uv, p0, p1, p2, true)){
			d=min(d,-bezier_sd(uv, p0, p1, p2));
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-0.966462321887,0.559912099879),vec2(-0.841030169481,0.559912099879)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.905207625963,0.585756553774),vec2(-0.966462321887,0.559912099879)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.841030169481,0.559912099879),vec2(-0.905207625963,0.585756553774)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-0.82088690046,0.316086918361),vec2(-0.81910715467,0.318958533541)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.81910715467,0.318958533541),vec2(-0.965419171536,0.318958533541)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.965419171536,0.318958533541),vec2(-0.902074977419,0.293114079646)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.902074977419,0.293114079646),vec2(-0.82088690046,0.316086918361)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-0.773817587078,0.434928897242),vec2(-0.808617339647,0.546859356249)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.808617339647,0.546859356249),vec2(-0.882523641701,0.576621645388)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.882523641701,0.576621645388),vec2(-0.773817587078,0.434928897242)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-0.81697111046,0.384744318419),vec2(-0.834987634664,0.312096998846)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.834987634664,0.312096998846),vec2(-0.82088690046,0.316086918361)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.82088690046,0.316086918361),vec2(-0.778334987661,0.384744318419)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.778334987661,0.384744318419),vec2(-0.81697111046,0.384744318419)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-0.82088690046,0.316086918361),vec2(-0.778334987661,0.384744318419)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.778334987661,0.384744318419),vec2(-0.785237494603,0.384744318419)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.785237494603,0.384744318419),vec2(-0.894429408392,0.295277456717)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.894429408392,0.295277456717),vec2(-0.82088690046,0.316086918361)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-0.829392250654,0.434344752485),vec2(-0.773635973483,0.434344752485)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.833342939816,0.556816429474),vec2(-0.829392250654,0.434344752485)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.808617339647,0.546859356249),vec2(-0.833342939816,0.556816429474)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.773635973483,0.434344752485),vec2(-0.808617339647,0.546859356249)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-1.00049262137,0.545554106703),vec2(-1.03677923708,0.439565833392)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.03677923708,0.439565833392),vec2(-1.03493619641,0.434232780007)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.03493619641,0.434232780007),vec2(-0.924880247944,0.577456322027)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.924880247944,0.577456322027),vec2(-1.00049262137,0.545554106703)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-1.033868039,0.448069047365),vec2(-1.03677923708,0.439565833392)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.03677923708,0.439565833392),vec2(-0.999970513281,0.333055493352)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.999970513281,0.333055493352),vec2(-0.926665359012,0.303146964146)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.926665359012,0.303146964146),vec2(-1.033868039,0.448069047365)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-1.02909290529,0.462016564017),vec2(-1.03677923708,0.439565833392)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.03677923708,0.439565833392),vec2(-1.03497489278,0.434344752485)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.03497489278,0.434344752485),vec2(-0.773635973483,0.434344752485)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.773635973483,0.434344752485),vec2(-0.782239281314,0.462016564017)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.782239281314,0.462016564017),vec2(-1.02909290529,0.462016564017)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-0.972402534183,0.557405817862),vec2(-1.00049262137,0.545554106703)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.00049262137,0.545554106703),vec2(-1.03677923708,0.439565833392)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.03677923708,0.439565833392),vec2(-0.999970513281,0.333055493352)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.999970513281,0.333055493352),vec2(-0.992084221177,0.329837883348)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.992084221177,0.329837883348),vec2(-0.972402534183,0.557405817862)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}

		d1=1e38;
		d1=abs_min(d1,line_dist(uv,vec2(-0.980652472562,0.553925021021),vec2(-1.00049262137,0.545554106703)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.00049262137,0.545554106703),vec2(-1.03677923708,0.439565833392)));
		d1=abs_min(d1,line_dist(uv,vec2(-1.03677923708,0.439565833392),vec2(-0.999970513281,0.333055493352)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.999970513281,0.333055493352),vec2(-0.980652472562,0.325173725818)));
		d1=abs_min(d1,line_dist(uv,vec2(-0.980652472562,0.325173725818),vec2(-0.980652472562,0.553925021021)));

		if(d1<=0.){
			return d1;
		}
		else{
			poly_d=min(d1,poly_d);
		}
	}
	

	d=min(poly_d,d);

	return d;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord ){
	 border = 0.0;
	
	vec2 uv = fragCoord.xy;

	uv.x-=1;
	//uv/=10;
	float d = 1e38;

	if(all(lessThan(abs(uv-vec2(0.0873228569516,0.500000020681)),vec2(1.58799651081,0.206885941035)+vec2(border))))
		d=min(d,render_serif(uv));


	fragColor=vec4(smoothstep(0., 0.0, d));
}

void main()
{      
	//gBaseColor = vec4(TexCoords,1,1);
	//gBaseColor = vec4(240./255, 220./255,157./255,1);
	mainImage(gBaseColor, vec2(1.,1.)-TexCoords);
	gPosition = WorldPos;
	gNormal = normalize(Normal);
	//gMetallicRoughness = vec2(1, 46./255);
	gMetallicRoughness = vec2( 0 /*½ðÊô¶È*/, 0.8 /*´Ö²Ú¶È*/);
}