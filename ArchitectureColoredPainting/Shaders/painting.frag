#version 450 core

layout(location = 0) out vec4 gBaseColor;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec3 gPosition;
layout(location = 3) out vec2 gMetallicRoughness;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

layout(std430, binding = 1) buffer bvhBuffer
{
	uint bvhLength;
	uvec2 bvhChildren[];
};
layout(std430, binding = 2) buffer bvhBoundBuffer
{
	vec4 bvhBound[];
};
layout(std430, binding = 3) buffer elementOffsetBuffer
{
	/**********************
	** @x elementBvhRoot
	** @y elementBvhLength
	** @z pointsOffset
	** @w linesOffset
	**********************/
	uvec4 elementOffset[];
};
layout(std430, binding = 4) buffer elementIndexBuffer
{
	uint elementIndexs[]; //线和面
};
layout(std430, binding = 5) buffer elementDataBuffer
{
	float elementData[]; //点和Style
};

const float PI = 3.14159265358979;

////////////////////////////////////////////////////////////////////////////

float border = 0;

mat2 inv(mat2 m)
{
	return mat2(m[1][1], -m[0][1], -m[1][0], m[0][0]) / (m[0][0] * m[1][1] - m[1][0] * m[0][1]);
}

float abs_min(float a, float b)
{
	if (abs(a) < abs(b))
	{
		return a;
	}
	else
	{
		return b;
	}
}

int i_mod(int a, int m)
{
	return int(mod(float(a), float(m)));
}

float line_dist(vec2 uv, const vec2 p0, vec2 p1)
{
	vec2 tang = p1 - p0;
	vec2 nor = normalize(vec2(tang.y, -tang.x));

	if (dot(tang, uv) < dot(tang, p0))
	{
		return distance(p0, uv);
	}
	else if (dot(tang, uv) > dot(tang, p1))
	{
		return distance(p1, uv);
	}
	else
	{
		return dot(nor, uv) - dot(nor, p0);
	}
}

bool int_test(vec2 uv, vec2 last_point, vec2 p0, vec2 p1)
{
	last_point -= uv;
	p0 -= uv;
	p1 -= uv;

	bool ret;
	if (p0.y == 0.)
	{
		ret = (p0.x >= 0. && p1.y * last_point.y < 0.);
	}
	else if (p1.y == 0.)
	{
		ret = false;
	}
	else if (p0.y * p1.y < 0.)
	{
		if (p0.x >= 0. && p1.x >= 0.)
		{
			ret = true;
		}
		else if (p0.x < 0. && p1.x < 0.)
		{
			ret = false;
		}
		else
		{
			vec2 nor;
			if (p0.y > p1.y)
			{
				nor = p0 - p1;
			}
			else
			{
				nor = p1 - p0;
			}

			nor = vec2(nor.y, -nor.x);
			if (dot(nor, p0) < 0.)
			{
				ret = false;
			}
			else
			{
				ret = true;
			}
		}
	}
	else
	{
		ret = false;
	}

	return ret;
}

bool tri_test(vec2 uv, vec2 p0, vec2 p1, vec2 p2, bool inside)
{
	vec2 nor1 = normalize(p0 - p1);
	nor1 = vec2(nor1.y, -nor1.x);
	vec2 nor2 = normalize(p1 - p2);
	nor2 = vec2(nor2.y, -nor2.x);
	vec2 tan3 = normalize(p2 - p0);
	vec2 nor3 = vec2(tan3.y, -tan3.x);

	if (inside)
	{
		if (dot(tan3, p0) >= dot(tan3, uv) || dot(tan3, p2) <= dot(tan3, uv))
		{
			return false;
		}

		float brd = max(dot(nor3, nor1), dot(nor3, nor2)) * border;
		return (dot(uv, nor1) >= dot(p0, nor1) && dot(uv, nor2) >= dot(p1, nor2) &&
				dot(uv, nor3) >= dot(p2, nor3) + brd) ||
			   (dot(uv, nor1) <= dot(p0, nor1) && dot(uv, nor2) <= dot(p1, nor2) &&
				dot(uv, nor3) <= dot(p2, nor3) - brd);
	}
	else
	{
		float brd1 = dot(nor1, tan3) * border;
		float brd2 = dot(nor2, tan3) * border;

		if (dot(tan3, p0) >= dot(tan3, uv) - brd1 || dot(tan3, p2) <= dot(tan3, uv) - brd2)
		{
			return false;
		}
		return (dot(uv, nor1) >= dot(p0, nor1) - border && dot(uv, nor2) >= dot(p1, nor2) - border &&
				dot(uv, nor3) >= dot(p2, nor3)) ||
			   (dot(uv, nor1) <= dot(p0, nor1) + border && dot(uv, nor2) <= dot(p1, nor2) + border &&
				dot(uv, nor3) <= dot(p2, nor3));
	}
}

float bezier_sd(vec2 uv, vec2 p0, vec2 p1, vec2 p2)
{

	const mat2 trf1 = mat2(-1, 2, 1, 2);
	mat2 trf2 = inv(mat2(p0 - p1, p2 - p1));
	mat2 trf = trf1 * trf2;

	uv -= p1;
	vec2 xy = trf * uv;
	xy.y -= 1.;

	vec2 gradient;
	gradient.x = 2. * trf[0][0] * (trf[0][0] * uv.x + trf[1][0] * uv.y) - trf[0][1];
	gradient.y = 2. * trf[1][0] * (trf[0][0] * uv.x + trf[1][0] * uv.y) - trf[1][1];

	return (xy.x * xy.x - xy.y) / length(gradient);
}

////////////////////////////////

// Modified from http://tog.acm.org/resources/GraphicsGems/gems/Roots3And4.c
// Credits to Doublefresh for hinting there
int solve_quadric(vec2 coeffs, inout vec2 roots)
{

	// normal form: x^2 + px + q = 0
	float p = coeffs[1] / 2.;
	float q = coeffs[0];

	float D = p * p - q;

	if (D < 0.)
	{
		return 0;
	}
	else
	{
		roots[0] = -sqrt(D) - p;
		roots[1] = sqrt(D) - p;

		return 2;
	}
}

// From Trisomie21
// But instead of his cancellation fix i'm using a newton iteration
int solve_cubic(vec3 coeffs, inout vec3 r)
{

	float a = coeffs[2];
	float b = coeffs[1];
	float c = coeffs[0];

	float p = b - a * a / 3.0;
	float q = a * (2.0 * a * a - 9.0 * b) / 27.0 + c;
	float p3 = p * p * p;
	float d = q * q + 4.0 * p3 / 27.0;
	float offset = -a / 3.0;
	if (d >= 0.0)
	{ // Single solution
		float z = sqrt(d);
		float u = (-q + z) / 2.0;
		float v = (-q - z) / 2.0;
		u = sign(u) * pow(abs(u), 1.0 / 3.0);
		v = sign(v) * pow(abs(v), 1.0 / 3.0);
		r[0] = offset + u + v;

		// Single newton iteration to account for cancellation
		float f = ((r[0] + a) * r[0] + b) * r[0] + c;
		float f1 = (3. * r[0] + 2. * a) * r[0] + b;

		r[0] -= f / f1;

		return 1;
	}
	float u = sqrt(-p / 3.0);
	float v = acos(-sqrt(-27.0 / p3) * q / 2.0) / 3.0;
	float m = cos(v), n = sin(v) * 1.732050808;

	// Single newton iteration to account for cancellation
	//(once for every root)
	r[0] = offset + u * (m + m);
	r[1] = offset - u * (n + m);
	r[2] = offset + u * (n - m);

	vec3 f = ((r + a) * r + b) * r + c;
	vec3 f1 = (3. * r + 2. * a) * r + b;

	r -= f / f1;

	return 3;
}

float cubic_bezier_normal_iteration(float t, vec2 a0, vec2 a1, vec2 a2, vec2 a3)
{
	// horner's method
	vec2 a_2 = a2 + t * a3;
	vec2 a_1 = a1 + t * a_2;
	vec2 b_2 = a_2 + t * a3;

	vec2 uv_to_p = a0 + t * a_1;
	vec2 tang = a_1 + t * b_2;

	float l_tang = dot(tang, tang);
	return t - dot(tang, uv_to_p) / l_tang;
}

float cubic_bezier_dis_approx_sq(vec2 uv, vec2 p0, vec2 p1, vec2 p2, vec2 p3)
{
	vec2 a3 = (-p0 + 3. * p1 - 3. * p2 + p3);
	vec2 a2 = (3. * p0 - 6. * p1 + 3. * p2);
	vec2 a1 = (-3. * p0 + 3. * p1);
	vec2 a0 = p0 - uv;

	float d0 = 1e38;

	float t;
	vec3 params = vec3(0, .5, 1);

	for (int i = 0; i < 3; i++)
	{
		t = params[i];
		for (int j = 0; j < 3; j++)
		{
			t = cubic_bezier_normal_iteration(t, a0, a1, a2, a3);
		}
		t = clamp(t, 0., 1.);
		vec2 uv_to_p = ((a3 * t + a2) * t + a1) * t + a0;
		d0 = min(d0, dot(uv_to_p, uv_to_p));
	}

	return d0;
}

// segment_dis_sq by iq
float length2(vec2 v)
{
	return dot(v, v);
}

float segment_dis_sq(vec2 p, vec2 a, vec2 b)
{
	vec2 pa = p - a, ba = b - a;
	float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
	return length2(pa - ba * h);
}

int segment_int_test(vec2 uv, vec2 p0, vec2 p1)
{
	p0 -= uv;
	p1 -= uv;

	int ret;

	if (p0.y * p1.y < 0.)
	{
		vec2 nor = p0 - p1;
		nor = vec2(nor.y, -nor.x);

		float sgn;

		if (p0.y > p1.y)
		{
			sgn = 1.;
		}
		else
		{
			sgn = -1.;
		}

		if (dot(nor, p0) * sgn < 0.)
		{
			ret = 0;
		}
		else
		{
			ret = 1;
		}
	}
	else
	{
		ret = 0;
	}

	return ret;
}

int cubic_bezier_int_test(vec2 uv, vec2 p0, vec2 p1, vec2 p2, vec2 p3)
{

	float cu = (-p0.y + 3. * p1.y - 3. * p2.y + p3.y);
	float qu = (3. * p0.y - 6. * p1.y + 3. * p2.y);
	float li = (-3. * p0.y + 3. * p1.y);
	float co = p0.y - uv.y;

	vec3 roots = vec3(1e38);
	int n_roots;

	int n_ints = 0;

	if (uv.x < min(min(p0.x, p1.x), min(p2.x, p3.x)))
	{
		if (uv.y >= min(p0.y, p3.y) && uv.y <= max(p0.y, p3.y))
		{
			n_ints = 1;
		}
	}
	else
	{

		if (abs(cu) < .0001)
		{
			n_roots = solve_quadric(vec2(co / qu, li / qu), roots.xy);
		}
		else
		{
			n_roots = solve_cubic(vec3(co / cu, li / cu, qu / cu), roots);
		}

		for (int i = 0; i < n_roots; i++)
		{
			if (roots[i] >= 0. && roots[i] <= 1.)
			{
				float x_pos = -p0.x + 3. * p1.x - 3. * p2.x + p3.x;
				x_pos = x_pos * roots[i] + 3. * p0.x - 6. * p1.x + 3. * p2.x;
				x_pos = x_pos * roots[i] + -3. * p0.x + 3. * p1.x;
				x_pos = x_pos * roots[i] + p0.x;

				if (x_pos > uv.x)
				{
					n_ints++;
				}
			}
		}
	}

	return n_ints;
}

float path2_dis_sq(vec2 uv)
{
	float dis_sq = 1e38;

	int num_its = 0;

	vec2[45] p = vec2[](vec2(-0.124919, 0.0496896), vec2(-0.127105, 0.0391554), vec2(-0.127105, 0.0405467),
						vec2(-0.127105, 0.0463107), vec2(-0.131876, 0.0506833), vec2(-0.143205, 0.0506833),
						vec2(-0.177789, 0.0506833), vec2(-0.194286, 0.00795032), vec2(-0.194286, -0.018882),
						vec2(-0.194286, -0.0425342), vec2(-0.181366, -0.0508821), vec2(-0.167851, -0.0508821),
						vec2(-0.153739, -0.0508821), vec2(-0.144397, -0.0417392), vec2(-0.138236, -0.0325963),
						vec2(-0.137043, -0.0445217), vec2(-0.129888, -0.0508821), vec2(-0.118758, -0.0508821),
						vec2(-0.108025, -0.0508821), vec2(-0.0901364, -0.0465093), vec2(-0.0788071, -0.0141118),
						vec2(-0.087155, -0.0141118), vec2(-0.0901364, -0.0240497), vec2(-0.0955028, -0.0327951),
						vec2(-0.103254, -0.0327951), vec2(-0.10882, -0.0327951), vec2(-0.111403, -0.0298137),
						vec2(-0.111403, -0.0242485), vec2(-0.111403, -0.0224597), vec2(-0.111205, -0.0204721),
						vec2(-0.110609, -0.0178882), vec2(-0.0962985, 0.0496896), vec2(-0.137043, 0.0383603),
						vec2(-0.130683, 0.0383603), vec2(-0.128894, 0.0331926), vec2(-0.128894, 0.0308075),
						vec2(-0.138435, -0.0141118), vec2(-0.14082, -0.0256398), vec2(-0.149168, -0.0316026),
						vec2(-0.154931, -0.0316026), vec2(-0.158509, -0.0316026), vec2(-0.164869, -0.0314042),
						vec2(-0.164869, -0.0160994), vec2(-0.164869, 0.00258385), vec2(-0.153938, 0.0383603));

	ivec2[6] seg = ivec2[](ivec2(0, 1), ivec2(1, 2), ivec2(20, 21), ivec2(30, 31), ivec2(31, 0), ivec2(35, 36));

	ivec4[13] c_bez =
		ivec4[](ivec4(2, 3, 4, 5), ivec4(5, 6, 7, 8), ivec4(8, 9, 10, 11), ivec4(11, 12, 13, 14), ivec4(14, 15, 16, 17),
				ivec4(17, 18, 19, 20), ivec4(21, 22, 23, 24), ivec4(24, 25, 26, 27), ivec4(27, 28, 29, 30),
				ivec4(32, 33, 34, 35), ivec4(36, 37, 38, 39), ivec4(39, 40, 41, 42), ivec4(42, 43, 44, 32));

	if (all(lessThan(uv, vec2(-0.0788071, 0.0506833) + border)) &&
		all(greaterThan(uv, vec2(-0.194286, -0.0508821) - border)))
	{
		for (int i = 0; i < 6; i++)
		{
			dis_sq = min(dis_sq, segment_dis_sq(uv, p[seg[i][0]], p[seg[i][1]]));
			num_its += segment_int_test(uv, p[seg[i][0]], p[seg[i][1]]);
		}
		for (int i = 0; i < 13; i++)
		{
			dis_sq = min(
				dis_sq, cubic_bezier_dis_approx_sq(uv, p[c_bez[i][0]], p[c_bez[i][1]], p[c_bez[i][2]], p[c_bez[i][3]]));
			num_its += cubic_bezier_int_test(uv, p[c_bez[i][0]], p[c_bez[i][1]], p[c_bez[i][2]], p[c_bez[i][3]]);
		}
	}

	float sgn = 1.;

	if (num_its % 2 == 1)
	{
		sgn = -1.;
	}

	return sgn * dis_sq;
}

void mainImage(out vec3 fragColor, in vec2 fragCoord)
{
	border = 0.01;
	vec2 uv = fragCoord;
	uv -= .5;

	float dis_sq = 1e38;

	if (all(lessThan(uv, vec2(0.4, 0.0993791) + border)) && all(greaterThan(uv, vec2(-0.4, -0.0993791) - border)))
	{
		dis_sq = min(dis_sq, path2_dis_sq(uv));
	}

	float dis = sign(dis_sq) * sqrt(abs(dis_sq));

	fragColor = vec3(smoothstep(0., border, dis));
	if (dis > 0 && dis <= border)
		fragColor = vec3(1, 1, 0);
}
///////////////////////////////

const uint STACK_SIZE = 10;

struct Stack
{
	uint top;
	uint data[STACK_SIZE];

	bool empty()
	{
		return top == 0;
	}
	bool full()
	{
		return top == STACK_SIZE;
	}
	bool getTop(out uint x)
	{
		if (empty())
			return false;
		x = data[top - 1];
		return true;
	}
	bool pop()
	{
		if (empty())
			return false;
		top--;
		return true;
	}
	bool push(in uint x)
	{
		if (full())
			return false;
		data[top] = x;
		top++;
		return true;
	}
} stack, elementStack;

void main()
{
	// gBaseColor = vec4(TexCoords,1,1);
	// gBaseColor = vec4(240./255, 220./255,157./255,1);
	// gBaseColor = vec4(bvh[0].bound==vec4(0,0,1,1));

	vec2 uv = vec2(1., 1.) - TexCoords * 2;
	vec3 debugBVH = vec3(0);
	bool debugHit = false;
	stack.top = 0;
	uint index = 0;
	while (index < bvhLength || !stack.empty())
	{
		while (index < bvhLength)
		{
			vec4 bound = bvhBound[index];
			uint leftChild = bvhChildren[index].x;
			if (leftChild >= bvhLength)
			{
				float angle = float(bvhChildren[index].y) / 4294967296. * 2 * PI;
				mat2 rotation = {{cos(angle), -sin(angle)}, {sin(angle), cos(angle)}};
				vec2 localUV = uv - (bound.xy + bound.zw) / 2;
				localUV = rotation * localUV;
				localUV /= (bound.zw - bound.xy) / 2;
				if (all(lessThan(vec2(-1), localUV)) && all(lessThan(localUV, vec2(1))))
				{
					uint elementIndex = leftChild - bvhLength;
					//debugBVH.bg += 0.5 * (localUV + vec2(1));
					debugBVH = vec3(0);
					////////////////////////////图元内BVH/////////////////////////////////
	
					uvec4 currentOffset =  elementOffset[elementIndex];
					uint elementBvhRoot = currentOffset.x;
					uint elementBvhLength = currentOffset.y;
					uint pointsOffset = currentOffset.z;
					uint linesOffset = currentOffset.w;

					elementStack.top = 0;
					uint elementBvhIndex = 0;
					while (elementBvhIndex < elementBvhLength || !elementStack.empty())
					{
				
						while (elementBvhIndex < elementBvhLength)
						{
							vec4 bound = bvhBound[elementBvhRoot + elementBvhIndex];
							uint leftChild = bvhChildren[elementBvhRoot + elementBvhIndex].x;
					
							if (all(lessThan(bound.xy, localUV)) && all(lessThan(localUV, bound.zw)))
							{
								if (leftChild >= elementBvhLength )
								{
									debugBVH.g += 0.5;
									

									uint styleIndex = bvhChildren[elementBvhRoot + elementBvhIndex].y-elementBvhLength;
							
									if(elementData[styleIndex]==0. )//面
									{
										
										uint lineCount = elementIndexs[leftChild-elementBvhLength];
										uint num_its = 0;
										for(uint contourIterator = leftChild-elementBvhLength+1; contourIterator<=leftChild-elementBvhLength+lineCount; contourIterator++  )
										{
											uint lineIndex = elementIndexs[contourIterator];
											uint p0Index = elementIndexs[linesOffset+4*lineIndex];
											uint p1Index = elementIndexs[linesOffset+4*lineIndex+1];
											uint p2Index = elementIndexs[linesOffset+4*lineIndex+2];
											uint p3Index = elementIndexs[linesOffset+4*lineIndex+3];
											
											vec2 p0 = vec2(elementData[pointsOffset+2*p0Index], elementData[pointsOffset+2*p0Index+1]);
											vec2 p1 = vec2(elementData[pointsOffset+2*p1Index], elementData[pointsOffset+2*p1Index+1]);
											vec2 p2 = vec2(elementData[pointsOffset+2*p2Index], elementData[pointsOffset+2*p2Index+1]);
											vec2 p3 = vec2(elementData[pointsOffset+2*p3Index], elementData[pointsOffset+2*p3Index+1]);

											

											if(p0==p1 && p2==p3)
											{											
												num_its += segment_int_test(localUV, p0, p3);
											}
												
											else
												num_its += cubic_bezier_int_test(localUV, p0, p1, p2, p3);
										}
	

								

										if (num_its % 2 == 1)
										{
											debugHit = true;
										}
									
										 
										
									}
									else if(elementData[styleIndex]==1)//线
									{
									
									}

									elementBvhIndex = elementBvhLength;
								}
								else 
								{
									//debugBVH.b += 0.2;
									elementStack.push(elementBvhIndex);
									elementBvhIndex = leftChild;
								}
							  
							}
							else
								elementBvhIndex = elementBvhLength;
						}
						if (!elementStack.empty())
						{
							elementStack.getTop(elementBvhIndex);
							elementStack.pop();
							elementBvhIndex = bvhChildren[elementBvhRoot + elementBvhIndex].y;
						}
					}
					//////////////////////////////////////////////


					// mainImage(debugBVH,localUV);

					//					for(uint i=elementOffset[elementIndex][1];i<elementOffset[elementIndex][2];i+=7)
					//					{
					//						if(tri_test(localUV, vec2(elementData[i],elementData[i+1]),
					//vec2(elementData[i+2],elementData[i+3]), vec2(elementData[i+4],elementData[i+5]), true))
					//						{
					//							if(elementData[i+6]==0)
					//							{
					//								debugHit=true;
					//							}
					//							else if(elementData[i+6]==1)
					//							{
					//								if(-bezier_sd(localUV, vec2(elementData[i],elementData[i+1]),
					//vec2(elementData[i+2],elementData[i+3]), vec2(elementData[i+4],elementData[i+5]))>0)
					//									debugHit=true;
					//							}
					//							else
					//							{
					//								if(bezier_sd(localUV, vec2(elementData[i],elementData[i+1]),
					//vec2(elementData[i+2],elementData[i+3]), vec2(elementData[i+4],elementData[i+5]))>0)
					//									debugHit=true;
					//							}
					//
					//						}
					//					}
				}

				index = bvhLength;
			}
			else if (all(lessThan(bound.xy, uv)) && all(lessThan(uv, bound.zw)))
			{
				debugBVH.r += 0.2;
				stack.push(index);
				index = leftChild;
			}
			else
				index = bvhLength;
		}
		if (!stack.empty())
		{
			stack.getTop(index);
			stack.pop();
			index = bvhChildren[index].y;
		}
	}
	if (debugHit)
		gBaseColor = vec4(vec3(1, 1, 0), 1);
	else
		gBaseColor = vec4(debugBVH, 1);
	// gBaseColor = vec4( vec3(elementData[6]<0.5),1 );
	// mainImage(gBaseColor, vec2(1.,1.)-TexCoords);
	gPosition = WorldPos;
	gNormal = normalize(Normal);
	// gMetallicRoughness = vec2(1, 46./255);
	gMetallicRoughness = vec2(0 /*金属度*/, 0.8 /*粗糙度*/);
}