﻿#define NOMINMAX

#include "our_gl.h"
#include <iostream>
#include <algorithm>
mat4 View;
mat4 Viewport;
mat4 Projection;

const float depth = 255.f;

//initialized operators and constructors
Vec2i operator -(const Vec2i& a, const Vec2i& b)
{
	return { a.x - b.x, a.y - b.y };
}

Vec2i operator +(const Vec2i& a, const Vec2f& b)
{
	return { a.x + (int)(b.x + .5), a.y + (int)(b.y + .5) };
}

Vec3i operator -(const Vec3i& a, const Vec3i& b)
{
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec3i operator +(const Vec3i& a, const Vec3f& b)
{
	return { a.x + (int)(b.x + .5), a.y + (int)(b.y + .5), a.z + (int)(b.z + .5) };
}

void viewport(int x, int y, int w, int h) {
	Viewport._14 = x + w / 2.f;
	Viewport._24 = y + h / 2.f;
	Viewport._34 = depth / 2.f;

	Viewport._11 = w / 2.f;
	Viewport._22 = h / 2.f;
	Viewport._33 = depth / 2.f;
}


void projection(float coeff)
{
	Projection._43 = coeff;
}

void lookat(Vec3f eye, Vec3f center, Vec3f up) {
	Vec3f z = (eye - center);
	z.Normalize();
	Vec3f x = up.Cross(z);
	x.Normalize();
	Vec3f y = z.Cross(x);
	y.Normalize();
	for (int i = 0; i < 3; i++) {
		View(0, i) = x[i];
		View(1, i) = y[i];
		View(2, i) = z[i];
	}
}

Vec3f barycentric(Vec4f coords[3], Vec2f P) {
	for (int i = 0; i < 3; i++)
	{
		coords[i] /= coords[i].w;
	}
	Vec2f A = Vec2f(coords[0].x, coords[0].y);
	Vec2f B = Vec2f(coords[1].x, coords[1].y);
	Vec2f C = Vec2f(coords[2].x, coords[2].y);
	Vec2f vx = B - A;
	Vec2f vy = C - A;
	Vec2f vz = A - P;
	Vec3f u = Vec3f(vx.x, vy.x, vz.x).Cross(Vec3f(vx.y, vy.y, vz.y));
	if (std::abs(u.z) < 1) return { -1, 1, 1 };
	u /= u.z;
	return { 1 - u.x - u.y, u.x, u.y };
}

Vec3f barycentric(Vec2i coords[3], Vec2f P) {
	Vec2f A = Vec2f(coords[0].x, coords[0].y);
	Vec2f B = Vec2f(coords[1].x, coords[1].y);
	Vec2f C = Vec2f(coords[2].x, coords[2].y);
	Vec2f vx = B - A;
	Vec2f vy = C - A;
	Vec2f vz = A - P;
	Vec3f u = Vec3f(vx.x, vy.x, vz.x).Cross(Vec3f(vx.y, vy.y, vz.y));
	if (std::abs(u.z) < 1) return { -1, 1, 1 };
	u /= u.z;
	return { 1 - u.x - u.y, u.x, u.y };
}

void triangle(Vec4f screen_coords[3], IShader& shader, TGAImage& image, TGAImage& zbuffer, Model* model)
{
	Vec2f bboxmin(FLT_MAX, FLT_MAX);
	Vec2f bboxmax(FLT_MIN, FLT_MIN);
	Vec2f clamp(image.get_width() - 1, image.get_height() - 1);

	for (int i = 0; i < 3; i++) {
		bboxmin.x = std::max(0.0f, std::min(bboxmin.x, screen_coords[i].x/screen_coords[i].w));
		bboxmin.y = std::max(0.0f, std::min(bboxmin.y, screen_coords[i].y / screen_coords[i].w));
		bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, screen_coords[i].x / screen_coords[i].w));
		bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, screen_coords[i].y / screen_coords[i].w));
	}
	Vec2i P;
	TGAColor color;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			Vec3f bar = barycentric(screen_coords, Vec2f(P.x, P.y));
			float z = Vec3f(screen_coords[0].z, screen_coords[1].z, screen_coords[2].z).Dot(bar);
			float w = Vec3f(screen_coords[0].w, screen_coords[1].w, screen_coords[2].w).Dot(bar);
			int frag_depth = std::max(0, std::min((int)depth, int(z / w + .5)));
			if (bar.x < 0 || bar.y < 0 || bar.z<0 || zbuffer.get(P.x, P.y).b>frag_depth) continue;
			bool discard = shader.fragment(bar, color, model);
			if (!discard) {
				zbuffer.set(P.x, P.y, TGAColor(frag_depth));
				image.set(P.x, P.y, color);
			}
		}
	}
}

IShader::~IShader()
{}


bool FogShader::fragment(Vec3f bar, TGAColor& color, Model* model)
{
	int fogness = zbuffer->get(bar.x, bar.y).b;

	TGAColor fog = TGAColor(255 - fogness);

	color = image->get(bar.x, bar.y) + fog;
	return false;
}

void triangle(Vec2i screen_coords[3], IShader& shader, TGAImage& image, TGAImage& zbuffer, TGAImage& output)
{
	Vec2i bboxmin(FLT_MAX, FLT_MAX);
	Vec2i bboxmax(FLT_MIN, FLT_MIN);
	Vec2i clamp(image.get_width(), image.get_height());

	for (int i = 0; i < 3; i++) {
		bboxmin.x = std::max(0, std::min(bboxmin.x, screen_coords[i].x));
		bboxmin.y = std::max(0, std::min(bboxmin.y, screen_coords[i].y));
		bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, screen_coords[i].x));
		bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, screen_coords[i].y));
	}
	Vec2i P;
	TGAColor color;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			Vec3f bar = barycentric(screen_coords, Vec2f(P.x, P.y));
			if (bar.x < 0 || bar.y < 0 || bar.z < 0) continue;
			bool discard = shader.fragment(Vec3f(P.x, P.y, 1), color, nullptr);
			if (!discard) {
				output.set(P.x, P.y, color);
			}
		}
	}
}

bool NoiseShader::fragment(Vec3f bar, TGAColor& color, Model* model)
{
	color = image->get(bar.x, bar.y) + TGAColor(50) * ((float)rand() / (float)RAND_MAX);
	return false;
}

bool NegativeShader::fragment(Vec3f bar, TGAColor& color, Model* model)
{
	color = -(image->get(bar.x, bar.y));
	return false;
}

bool DesaturatingShader::fragment(Vec3f bar, TGAColor& color, Model* model)
{
	TGAColor с = image->get(bar.x, bar.y);
	color = TGAColor((с.r + с.g + с.b) / 3);
	return false;
}
